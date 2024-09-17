const V3G = v=> new THREE.Vector3(v.GetX(), v.GetY(), v.GetZ())              // => GUI vec3
const Q4G = q=> new THREE.Quaternion(q.GetX(), q.GetY(), q.GetZ(), q.GetW()) // => GUI quaternion
const RAD = d=> d*Math.PI/180.0

function createVehicleTrack(core) {
	const track = [
		[[[38, 64, -14], [38, 64, -16], [38, -64, -16], [38, -64, -14], [64, -64, -16], [64, -64, -14], [64, 64, -16], [64, 64, -14]], [[-16, 64, -14], [-16, 64, -16], [-16, -64, -16], [-16, -64, -14], [10, -64, -16], [10, -64, -14], [10, 64, -16], [10, 64, -14]], [[10, -48, -14], [10, -48, -16], [10, -64, -16], [10, -64, -14], [38, -64, -16], [38, -64, -14], [38, -48, -16], [38, -48, -14]], [[10, 64, -14], [10, 64, -16], [10, 48, -16], [10, 48, -14], [38, 48, -16], [38, 48, -14], [38, 64, -16], [38, 64, -14]]],
		[[[38, 48, -10], [38, 48, -14], [38, -48, -14], [38, -48, -10], [40, -48, -14], [40, -48, -10], [40, 48, -14], [40, 48, -10]], [[62, 62, -10], [62, 62, -14], [62, -64, -14], [62, -64, -10], [64, -64, -14], [64, -64, -10], [64, 62, -14], [64, 62, -10]], [[8, 48, -10], [8, 48, -14], [8, -48, -14], [8, -48, -10], [10, -48, -14], [10, -48, -10], [10, 48, -14], [10, 48, -10]], [[-16, 62, -10], [-16, 62, -14], [-16, -64, -14], [-16, -64, -10], [-14, -64, -14], [-14, -64, -10], [-14, 62, -14], [-14, 62, -10]], [[-14, -62, -10], [-14, -62, -14], [-14, -64, -14], [-14, -64, -10], [62, -64, -14], [62, -64, -10], [62, -62, -14], [62, -62, -10]], [[8, -48, -10], [8, -48, -14], [8, -50, -14], [8, -50, -10], [40, -50, -14], [40, -50, -10], [40, -48, -14], [40, -48, -10]], [[8, 50, -10], [8, 50, -14], [8, 48, -14], [8, 48, -10], [40, 48, -14], [40, 48, -10], [40, 50, -14], [40, 50, -10]], [[-16, 64, -10], [-16, 64, -14], [-16, 62, -14], [-16, 62, -10], [64, 62, -14], [64, 62, -10], [64, 64, -14], [64, 64, -10]]],
		[[[-4, 22, -14], [-4, -14, -14], [-4, -14, -10], [4, -14, -14], [4, -14, -10], [4, 22, -14]], [[-4, -27, -14], [-4, -48, -14], [-4, -48, -11], [4, -48, -14], [4, -48, -11], [4, -27, -14]], [[-4, 50, -14], [-4, 30, -14], [-4, 30, -12], [4, 30, -14], [4, 30, -12], [4, 50, -14]], [[46, 50, -14], [46, 31, -14], [46, 50, -12], [54, 31, -14], [54, 50, -12], [54, 50, -14]], [[46, 16, -14], [46, -19, -14], [46, 16, -10], [54, -19, -14], [54, 16, -10], [54, 16, -14]], [[46, -28, -14], [46, -48, -14], [46, -28, -11], [54, -48, -14], [54, -28, -11], [54, -28, -14]]]
	];

	const mapColors = [0x666666, 0x006600, 0x000066];

	let tempVec = new Jolt.Vec3(0, 1, 0);
	const mapRot = Jolt.Quat.prototype.sRotation(tempVec, 0.5 * Math.PI);
	let tempRVec = new Jolt.RVec3(0, 0, 0);
	track.forEach((type, tIdx) => {
		type.forEach(block => {
			const hull = new Jolt.ConvexHullShapeSettings;
			block.forEach(v => {
				tempVec.Set(-v[1], v[2], v[0]);
				hull.mPoints.push_back(tempVec);
			});
			const shape = hull.Create().Get();
			tempRVec.Set(0, 10, 0);
			const creationSettings = new Jolt.BodyCreationSettings(shape, tempRVec, mapRot, Jolt.EMotionType_Static, LAYER_NON_MOVING);
			Jolt.destroy(hull);
			const body = core.intf.CreateBody(creationSettings);
			Jolt.destroy(creationSettings);
			body.SetFriction(1.0);
			addToScene(body, mapColors[tIdx]);
		});
	});
	Jolt.destroy(tempVec);
	Jolt.destroy(tempRVec);
}

export default class {
    constructor(
        core,                         ///< jolt_core instance
        id, vtype,                    ///< vehicle id, and type
        w, h, l,                      ///< vehicle dimensions [width, height, length]
        pos, rot, color,              ///< dynaset and body color
        nwheel, ndiff=1, narbar=0,    ///< number of wheels, diffs, and anti-roll bars
        mass=1500,                    ///< body mass
        ang_pitch_roll=RAD(60)        ///< max pitch/roll angle
    ) {
        this.core    = core
        this.right   = new Jolt.Vec3(0, 1, 0)
        this.up      = new Jolt.Vec3(1, 0, 0)
        this.mati    = new THREE.MeshPhongMaterial({ color: 0xcccccc })
        this.mati.map= core.tex         // set checker texture
        ///
        /// create physical body
        ///
        this.id      = id
        this.shape   = new Jolt.OffsetCenterOfMassShapeSettings(
            new Jolt.Vec3(0, -h, 0),    // below the body for stability
            new Jolt.BoxShapeSettings(new Jolt.Vec3(w/2, h/2, l/2))
        ).Create().Get()
        core.addShape(id, this.shape, pos, rot, color, mass)
        core.tick(id, false)            // deactivate body update for now
        ///
        /// Vehicle configurations
        ///
        let cfg = new Jolt.VehicleConstraintSettings()
        let ctl, ctype
        cfg.mAntiRollBars.clear()
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
        /// initialize differentials, wheels, and anti-roll bars
        ///
        const init_part = (a, n, p)=>{
            a.clear(); for (let i=0; i<n; i++) a.push_back(new p())
        }
        init_part(cfg.mWheels,        nwheel, Jolt.WheelSettingsWV)
        init_part(ctl.mDifferentials, ndiff,  Jolt.VehicleDifferentialSettings)
        init_part(cfg.mAntiRollBars,  narbar, Jolt.VehicleAntiRollBar)

        this.ctrl   = cfg.mController = ctl          ///< short ref to controller
        this.config = cfg                            ///< vehicle configuration
        this.cnst   = core.setConstraint(id, cfg)    ///< set collision testsr
        this.handle = Jolt.castObject(this.cnst.GetController(), ctype)
        
        this.wheels = Array(nwheel)                  /// ref to GUI wheels
        this.xkey   = { F: 0.8, R: 0 }
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
    update() {                         ///> update GUI from physics
        for (let i=0; i < this.wheels.length; i++) this._syncWheel(i)
        const lv = this.cnst.GetVehicleBody().GetLinearVelocity()
        if (lv.LengthSq() < 0.1) {
            this.xkey.F *= -1
            this.xkey.R  = 0.3
        }
        else this.xkey.R = 0
        this.handle.SetDriverInput(this.xkey.F, this.xkey.R, 0, 0)
    }
    follow() {
        const pos = wrapVec3(this.body.GetPosition())
        jolt.orb.target = pos
        jolt.cam.position.add(pos.clone().sub(oldPos))
    }
    xkey_update(k, dv) {               // dv = steerSpeed * deltaTime
        let f0 = this.key.F, r0 = this.key.R
        let f1 = 0.0, r1 = 0.0, x1 = 0.0, handBrake = 0.0
        
        f1 = k.F ? 1.0 : (k.B ? -1.0 : 0.0)

        if (f0 * f1 < 0.0) {
            const rot = wrapQuat(this.body.GetRotation().Conjugated())
            const lv  = wrapVec3(this.body.GetLinearVelocity())
            const v1  = lv.applyQuaternion(rot).z
            if ((f1 > 0.0 && v1 < -0.1) || (f1 < 0.0 && v1 > 0.1)) {
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
        console.log(this.handle.GetTransmission())
    }
    setDifferential(
        id, left, right,                ///< diff, wheel index[left, right]
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
        console.log(id)
        console.log(this.handle.GetDifferentials().at(id))
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
        let cfg = this.cnst.GetWheel(id).GetSettings()
        cfg.mPosition            = pos
        cfg.mSuspensionDirection = new Jolt.Vec3(0, -1, Math.tan(ang_caster)).Normalized()   // bike point opposites to mSteeringAxis
        cfg.mSteeringAxis        = new Jolt.Vec3(0, 1, -Math.tan(ang_caster)).Normalized()
        cfg.mSuspensionMinLength = sus_min
        cfg.mSuspensionMaxLength = sus_max
        cfg.mSuspensionSpring.mFrequency = sus_freq
        cfg.mRadius              = (r1 > r2) ? r1 : r2
        cfg.mWidth               = w
        /// Wheeled Vehicle specific
        cfg.mMaxSteerAngle       = ang_steer
        cfg.mMaxBrakeTorque      = torque_break
        cfg.mMaxHandBrakeTorque  = torque_hbreak

        /// create GUI wheel
        let wheel =
             new THREE.Mesh(new THREE.CylinderGeometry(r1, r2, w, 20, 1), this.mati)
        this.wheels[id] = wheel         /// * keep ref
        this.core.scene.add(wheel)      /// * shown in GUI
        this._syncWheel(id)             /// * attach wheel to body
        console.log(cfg)
    }
    setAntiRoll(id, left, right, stiff=1000) {
        let rb = this.config.mAntiRollBars.at(id)
        rb.mLeftWheel  = left
        rb.mRightWheel = right
        rb.mStiffness  = stiff
        console.log(rb)
    }
    useMotorcycleDiff() {
        this.setDifferential(
            0, -1, 1,                          ///< body id, left, right wheel id
            4.8, 1.0, 1.0                      ///< ratio diff, engine torque, left-right split
        )
    }
    useWheeledCarDiff(
        fb_torque_ratio,
        fb_limited_slip_ratio = 1.4,           ///< max/min between wheels speed
        lr_limited_slip_ratio = 1.4
    ) {
        const n = this.ctrl.mDifferentials.size()
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
    _syncWheel(id) {               
        let wh = this.wheels[id]
        let tx = this.cnst.GetWheelWorldTransform(id, this.right, this.up)
        wh.position.copy(V3G(tx.GetTranslation()))
        wh.quaternion.copy(Q4G(tx.GetRotation().GetQuaternion()))
    }
}
