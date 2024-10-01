const V3G = v=> new THREE.Vector3(v.GetX(), v.GetY(), v.GetZ())              // => GUI vec3
const Q4G = q=> new THREE.Quaternion(q.GetX(), q.GetY(), q.GetZ(), q.GetW()) // => GUI quaternion
const RAD = d=> d*Math.PI/180.0

export default class {
    constructor(
        core,                         ///< jolt_core instance
        id, vtype,                    ///< vehicle id, and type
        w, h, l,                      ///< vehicle dimensions [width, height, length]
        pos, rot, color,              ///< dynaset and body color
        nwheel, ndiff, narbar,        ///< number of wheels, diffs, and anti-roll bars
        mass=1500,                    ///< body mass
        ang_pitch_roll=RAD(60)        ///< max pitch/roll angle
    ) {
        this.core    = core
        this.right   = new Jolt.Vec3(0, 1, 0)
        this.up      = new Jolt.Vec3(1, 0, 0)
        this.mati    = new THREE.MeshPhongMaterial({ color: 0xcccccc })
        this.mati.map= core.tex         // set checker texture
        ///
        ///> create physical body
        ///
        this.id      = id
        this.shape   = new Jolt.OffsetCenterOfMassShapeSettings(
            new Jolt.Vec3(0, -h, 0),    // below the body for stability
            new Jolt.BoxShapeSettings(new Jolt.Vec3(w/2, h/2, l/2))
        ).Create().Get()
        core.addShape(id, this.shape, pos, rot, color, mass)
        core.tick(id, false)            // deactivate body update for now
        ///
        ///> Vehicle configurations (constraint settings)
        ///
        let cfg = new Jolt.VehicleConstraintSettings()
        let ctl, ctype
        cfg.mMaxPitchRollAngle = ang_pitch_roll
        switch (vtype) {
        case '2':
            ctl   = new Jolt.MotorcycleControllerSettings()
            ctype = Jolt.MotorcycleController
            break
        case '4':
            ctl   = new Jolt.WheeledVehicleControllerSettings()
            ctype = Jolt.WheeledVehicleController
            break
        case 'T':
            ctl   = new Jolt.TrackedVehicleControllerSettings()
            ctype = Jolt.TrackedVehicleController
        }
        ///
        ///> initialize wheels, and anti-roll bars, and differentials
        ///
        const init_part = (a, n, p)=>{
            a.clear(); for (let i=0; i<n; i++) a.push_back(new p())
        }
        init_part(cfg.mWheels,        nwheel, Jolt.WheelSettingsWV)
        init_part(cfg.mAntiRollBars,  narbar, Jolt.VehicleAntiRollBar)
        init_part(ctl.mDifferentials, ndiff,  Jolt.VehicleDifferentialSettings)
        ///
        ///> init attributes
        ///
        this.ctrl   = cfg.mController = ctl          ///< VehicleControllerSettings
        this.config = cfg                            ///< VehicleConstraintSettings
        this.cnst   = core.setConstraint(id, cfg)    ///< VehicleConstraint
        this.handle = Jolt.castObject(this.cnst.GetController(), ctype) ///< VehicleController
        this.wheels = Array(nwheel)                  /// ref to GUI wheels
        this.xkey   = { F: 0.7, R: 0 }
        this.count  = 2
    }
    setCallback() {
        const ctrl = Jolt.castObject(cnst.GetController(), type)     // type=Jolt.MotorcycleController
        
        // Optional step: Set the vehicle constraint callbacks
        let cb = new Jolt.VehicleConstraintCallbacksJS()
        cb.GetCombinedFriction = (
            wheelIndex, tireFrictionDirection, tireFriction, body2, subShapeID2) => {
            body2 = Jolt.wrapPointer(body2, Jolt.Body)
                return Math.sqrt(tireFriction * body2.GetFriction()) // This is the default calculation
            }
        cb.OnPreStepCallback     = (vehicle, deltaTime, physicsSystem)=>{}
        cb.OnPostCollideCallback = (vehicle, deltaTime, physicsSystem)=>{}
        cb.OnPostStepCallback    = (vehicle, deltaTime, physicsSystem)=>{}
        
        cb.SetVehicleConstraint(cnst)
    }
    ai() {
        const body = this.cnst.GetVehicleBody()
        const rot  = Q4G(body.GetRotation().Conjugated())
        const lv   = V3G(body.GetLinearVelocity())
        const vz   = lv.applyQuaternion(rot).z      ///< local z velocity
        const f    = this.xkey.F
        if (f > 0 && vz < -0.1) {
            this.xkey.F *= -1
            this.xkey.R = Math.random() > 0.5 ? 0.3 : -0.3
        }
        else if (f < 0 && vz > 0.1) {
            this.xkey.F *= -1
        }
        else if (f > 0 && vz > 5.0) {
            this.xkey.R = 0
        }
        else if (f < 0 && vz < -5.0) {
            this.xkey.F *= -1
            this.xkey.R *= -1
            this.count  = 2
        }
        this.handle.SetDriverInput(this.xkey.F, this.xkey.R, 0, 0)
    }
    update() {                         ///> update GUI from physics
        this.ai()
        this.wheels.forEach(w=>w.sync())
    }
    follow() {
        const pos = V3G(this.body.GetPosition())
        this.orb.target = pos
        this.cam.position.add(pos.clone().sub(oldPos))
    }
    xkey_update(k, dv) {               // dv = steerSpeed * deltaTime
        let f0 = this.key.F, r0 = this.key.R
        let f1 = 0.0, r1 = 0.0, x1 = 0.0, handBrake = 0.0
        
        f1 = k.F ? 1.0 : (k.B ? -1.0 : 0.0)

        if (f0 * f1 < 0.0) {
            const body = this.cnst.GetVehicleBody()
            const rot  = Q4G(body.GetRotation().Conjugated())
            const lv   = V3G(body.GetLinearVelocity())
            const vz   = lv.applyQuaternion(rot).z
            if ((f1 > 0.0 && vz < -0.1) || (f1 < 0.0 && vz > 0.1)) {
                f1 = 0.0; x1 = 1.0    // Brake while we've not stopped yet
            }
            else this.key.F = f1      // When we've come to a stop, accept the new direction
        }
        
        r1 = k.R ? 1.0 : (k.L ? -1.0 : 0.0)
        this.key.R = r1 = r1 > r0
            ? Math.min(r0 + dv, r1)
            : (r1 < r0 ? Math.max(r0 - dv, r1) : r1)
        
        if (k.X) { f1 = 0.0; r1 = 0.0; x1 = 1.0; }
        
        this.handle.SetDriverInput(f1, r1, x1, handBrake);
        if (r1 != 0.0 || f1 != 0.0 || x1 != 0.0 || handBrake != 0.0) {
            this.core.tick(this.id)
        }
    }
    addTankBody(              // CC: move to jolt_vehicle or Forth
        w, h, l, wheelPos,    // body w, h, len, and wheel positions[]
        tw, th, tl,           // turret w, h, len
        br, bl                // barrel radius, len
    ) {
        let material = new THREE.MeshPhongMaterial({ color: 0x666666 })
        let tankMat1 = new THREE.MeshPhongMaterial({ color: 0x993333 })
        let tankMat2 = new THREE.MeshPhongMaterial({ color: 0x663333 })
        for (let t = 0; t < 2; t++) {
            const track = this.ctrl.get_mTracks(t)
            track.mDrivenWheel = this.config.mWheels.size() + (wheelPos.length - 1)
            wheelPos.forEach((loc, i)=>{
                const w = new Jolt.WheelSettingsTV();
                w.mPosition = new Jolt.Vec3(t == 0 ? w/2 : -w/2, loc[0], loc[1]);
                w.mRadius = r
                w.mWidth  = w
                w.mSuspensionMinLength         = sus_min
                w.mSuspensionMaxLength         = i==0 || i==wheelPos.length - 1 ? sus_min : sus_max
                w.mSuspensionSpring.mFrequency = sus_freq

                track.mWheels.push_back(vehicle.mWheels.size())
                this.config.mWheels.push_back(w)
            })
        }
        const turret = new THREE.Mesh(new THREE.BoxGeometry(tw, th, tl, 1, 1, 1), tankMat1)
        turret.position.set(0, h/2 + th, 0)
        this.core.scene.add(turret)

        const barrel = new THREE.Mesh(new THREE.CylinderGeometry(br, br, bl, 20, 1), tankMat2)
        barrel.quaternion.setFromAxisAngle(new THREE.Vector3(1, 0, 0), RAD(90));
        barrel.position.set(0, h/2 + th, tl + bl/2)
        this.core.scene.add(barrel)
    }
    setEngine(
        torque,                           ///< max engine torque
        rpm_max, rpm_min,                 ///< Engine max torque, max RPM, min RPM
        iner = 0.5, damp = 0.2            ///< inertial and angular damping
    ) {
        let eng = this.handle.GetEngine()
        eng.mMaxTorque      = torque      // 150
        eng.mMinRPM         = rpm_min     // 1000
        eng.mMaxRPM         = rpm_max     // 10000
//      eng.normalizedTorque= [ X-Axis, Y-Axis ]  // CC:Later
        eng.mInertial       = iner
        eng.mAngularDamping = damp
    }
    setTransmission(
        clutch   = 10,                    // Transmission clutch strength, shift up/down RPM
        rpm_up   = 4000,
        rpm_down = 2000,
    ) {
        let tran = this.handle.GetTransmission()
        // tran.mMode              = ETransmissionMode::Auto
        // tran.mGearRatios        = [ 2.66, 1.78, 1.3, 1.0, 0.74 ]
        // tran.mReverseGearRatios = [ -2.90 ]
        // tran.mSwitchTime        = 0.5
        // tran.mClutchReleaseTime = 0.3
        // tran.mSwitchLatency     = 0.5
        tran.mShiftUpRPM     = rpm_up     //8000
        tran.mShiftDownRPM   = rpm_down   //2000
        tran.mClutchStrength = clutch     //2
//        console.log(tran)
    }
    setDifferential(
        id, left, right,                ///< diff id, wheel index[left, right]
        diff_ratio           = 3.42,    ///< rotation speed between gearbox and wheel 3.42
        engine_torque_ratio  = 1.0,     ///< engine torque apply (sum should =1.0)
        lr_split             = 0.5,     ///< engine torque between l/r wheel 0.5
        lr_limited_slip_ratio= 1.4      ///< max/min between wheel speed 1.4
    ) {
        let d = this.handle.GetDifferentials().at(id)
        d.mLeftWheel         = left                   // -1=no wheel
        d.mRightWheel        = right                  // 1  
        d.mDifferentialRatio = diff_ratio             // 1.93 * 40.0 / 16.0 (gear/tire)
        d.mLeftRightSplit    = lr_split               // 0=left, 0.5=center, 1.0=right
        d.mLimitedSlipRatio  = lr_limited_slip_ratio  // max/min between two wheels
        d.mEngineTorqueRatio = engine_torque_ratio    // 1.0
//        console.log(d)
    }
    setWheel(                           ///> set wheel physical properties
        id, pos,                        ///< wheel id, positions, radius, width
        r1, r2, w,                      ///< wheel dimensions
        sus_freq,                       ///< suspension frequency
        sus_min       = 0.3,            ///< suspension min length
        sus_max       = 0.5,            ///< suspension max length
        ang_steer     = RAD(45),        ///< max steering angle
        ang_caster    = RAD(30),        ///< caster angle (steer direction)
        torque_break  = 1500,           ///< break torque, hand break torque
        torque_hbreak = 4000
    ) {
        let cfg = Jolt.castObject(
            this.cnst.GetWheel(id).GetSettings(), Jolt.WheelSettingsWV)
        /// WheelSettings attributes
        cfg.mPosition            = pos
        cfg.mSuspensionDirection = new Jolt.Vec3(0, -1, Math.tan(ang_caster)).Normalized()   // bike point opposites to mSteeringAxis
        cfg.mSteeringAxis        = new Jolt.Vec3(0, 1, -Math.tan(ang_caster)).Normalized()
        cfg.mSuspensionMinLength = sus_min
        cfg.mSuspensionMaxLength = sus_max
        cfg.mSuspensionSpring.mFrequency = sus_freq
        cfg.mRadius              = (r1 > r2) ? r1 : r2
        cfg.mWidth               = w
        /// WheelSettingsWV specific
        cfg.mMaxSteerAngle       = ang_steer
        cfg.mMaxBrakeTorque      = torque_break
        cfg.mMaxHandBrakeTorque  = torque_hbreak
        
        /// create GUI wheel
        let wheel =
            new THREE.Mesh(new THREE.CylinderGeometry(r1, r2, w, 20, 1), this.mati)
        wheel.sync = ()=>{                     /// * lambda to sync wheel/body
            let tx = this.cnst.GetWheelWorldTransform(id, this.right, this.up)
            wheel.position.copy(V3G(tx.GetTranslation()))
			wheel.quaternion.copy(Q4G(tx.GetRotation().GetQuaternion()))
        }
        this.wheels[id] = wheel                /// * keep ref
        this.core.scene.add(wheel)             /// * shown in GUI
        wheel.sync()                           /// * sync GUI wheel to GUI body
        
//        console.log(wheel)
    }
    setAntiRoll(id, left, right, stiff=1000) {
        let rb = this.config.mAntiRollBars.at(id)
        rb.mLeftWheel  = left
        rb.mRightWheel = right
        rb.mStiffness  = stiff
//        console.log(rb)
    }
    useMotorcycleDiff() {
        this.setDifferential(
            0, -1, 1,                          ///< diff id, left, right wheel id
            4.8, 1.0, 1.0                      ///< ratio diff, engine torque, left-right split
        )
    }
    useWheeledCarDiff(
        fb_torque_ratio,
        fb_limited_slip_ratio = 1.4,           ///< max/min between wheels speed
        lr_limited_slip_ratio = 1.4
    ) {
        const n = this.handle.GetDifferentials().size()
        switch (n) {
        case 1:
            if (fb_torque_ratio > 0.99) {
                this.setDifferential(          ///< Front differential
                    0, 0, 1, 1)
            }
            else if (fb_torque_ratio < 0.01) {
                this.setDifferential(          ///< Rear differential
                    0, 2, 3, 1)
            }
            else console.log("sum of fb_torque_ratio != 1.0")
            break
        case 2:
            this.setDifferential(              ///< Front differential
                0, 0, 1, fb_torque_ratio)
            this.setDifferential(              ///< Rear differential
                1, 2, 3, 1-fb_torque_ratio)    /// * check total torque sum = 1.0
            break
        }
        this.handle.mDifferentialLimitedSlipRatio = fb_limited_slip_ratio
    }
}
