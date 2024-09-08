export default class {
    constructor(
        jolt,
        id, vtype,                      // vehicle id, type
        w, h, l, mass,                  // vehicle width, height, length, mass
        pos, rot, color,                // dynaset and body color
        ang_pitch_roll                  // max pitch/roll angle
    ) {
        this.jolt    = jolt
        this.right   = new Jolt.Vec3(0, 1, 0)
        this.up      = new Jolt.Vec3(1, 0, 0)
		this.shape   = new Jolt.OffsetCenterOfMassShapeSettings(
            new Jolt.Vec3(0, -h/2, 0),   // below the body for stability
			new Jolt.BoxShapeSettings(new Jolt.Vec3(w/2, h/2, l/2))
        )
        this.body    = this.shape.Create().Get()
        this.vehicle = new Jolt.VehicleConstraintSettings()
		this.vehicle.mAntiRollBars.clear()
		this.vehicle.mWheels.clear()
		this.vehicle.mMaxPitchRollAngle = ang_pitch_roll
        
        jolt.addShape(id, this.shape, pos, rot, color, mass)

		// Set collision tester that checks the wheels for collision with the floor
		this.cnst  = new Jolt.VehicleConstraint(this.body, this.vehicle)
		this.tstr  = new Jolt.VehicleCollisionTesterCastCylinder(L_MOVING, 1);
		this.cnst.SetVehicleCollisionTester(this.tstr)
        
		jolt.phyx.AddConstraint(this.cnst)
        jolt.phyx.AddStepListener(new Jolt.VehicleConstraintStepListener(this.cnst))

        let ctype;
        switch (vtype) {
        case '2':
            this.ctrl = new Jolt.MotorcycleControllerSettings()
            ctype     = Jolt.MotorcycleController
            break
        case '4':
            this.ctrl = new Jolt.WheeledVehicleControllerSettings()
            ctype     = Jolt.WheeledVehicleController
            break
        case 'T':
            this.ctrl = new Jolt.TrackedVehicleControllerSettings()
			ctype     = Jolt.TrackedVehicleController
        }
		this.handle = Jolt.castObject(this.cnst.GetController(), ctype)
		this.ctrl.mDifferentials.clear()
        this.vehicle.mController = this.ctrl
        
        this.wheels = []
    }
    xkey_update(k) {
	    let f1 = 0.0, r1 = 0.0, x1 = 0.0, handBrake = 0.0
        
	    f1 = k.F ? 1.0 : (k.B ? -1.0 : 0.0)

	    if (f0 * f1 < 0.0) {
		    const rot = wrapQuat(this.body.GetRotation().Conjugated())
		    const lv  = wrapVec3(this.body.GetLinearVelocity())
		    const v1  = lv.applyQuaternion(rot).z
		    if ((f1 > 0.0 && v1 < -0.1) || (f1 < 0.0 && v1 > 0.1)) {
			    f1 = 0.0; x1 = 1.0    // Brake while we've not stopped yet
		    }
		    else f0 = f1  		      // When we've come to a stop, accept the new direction
	    }
        
	    r1 = k.R ? 1.0 : (k.L ? -1.0 : 0.0)
	    if (r1 > r0)      r1 = Math.min(r0 + steerSpeed * deltaTime, r1)
	    else if (r1 < r0) r1 = Math.max(r0 - steerSpeed * deltaTime, r1)
	    r0 = r1
        
	    if (k.X) { f1 = 0.0; r1 = 0.0; x1 = 1.0; }
        
        /// CC: move the following to jolt_core
	    this.handle.SetDriverInput(f1, r1, x1, handBrake);
	    if (r1 != 0.0 || f1 != 0.0 || x1 != 0.0 || handBrake != 0.0) {
		    this.jolt.bintf.ActivateBody(this.body.GetID())
        }
    }    
    pre_physics_update(dt) {
	    let oldPos = wrapVec3(this.body.GetPosition())
	    this.jolt.orb.target = motorcycle.position
	    onExampleUpdate = (time, deltaTime) => {
		    this.prePhysicsUpdate(deltaTime)
		    const pos = wrapVec3(this.body.GetPosition())
		    jolt.cam.position.add(pos.clone().sub(oldPos))
		    oldPos = pos
            for (let i = 0; i < this.wheels.size(); i++) {
			    this.wheels[i].updateLocalTransform()
            }
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
			track.mDrivenWheel = this.vehicle.mWheels.size() + (wheelPos.length - 1)
			wheelPos.forEach((loc, i)=>{
				const w = new Jolt.WheelSettingsTV();
				w.mPosition = new Jolt.Vec3(t == 0 ? w/2 : -w/2, loc[0], loc[1]);
				w.mRadius = r
				w.mWidth  = w
				w.mSuspensionMinLength         = sus_min
				w.mSuspensionMaxLength         = i==0 || i==wheelPos.length - 1 ? sus_min : sus_max
				w.mSuspensionSpring.mFrequency = sus_freq

				track.mWheels.push_back(vehicle.mWheels.size())
				this.vehicle.mWheels.push_back(w)
			})
		}
		const turret = new THREE.Mesh(new THREE.BoxGeometry(tw, th, tl, 1, 1, 1), tankMat1)
		turret.position.set(0, h/2 + th, 0)
        this.jolt._addToScene(turret)

		const barrel = new THREE.Mesh(new THREE.CylinderGeometry(br, br, bl, 20, 1), tankMat2)
		barrel.quaternion.setFromAxisAngle(new THREE.Vector3(1, 0, 0), Math.PI / 2);
		barrel.position.set(0, h/2 + th, tl + bl/2)
        this.jolt._addToScene(barrel)
    }
    addWheel(
        r, w, h, z_pos,                 // wheel radius, width, height, and position in Z
        ang_steer, ang_caster,          // steering angle, caster angle
        sus_freq,                       // suspension frequency, and max/min lengths
        sus_min  = 0.3, sus_max = 0.5,
        torque_break  = 1500,           // break torque, hand break torque
        torque_hbreak = 4000
    ) {
        /// create physical wheel
	    const cfg = new Jolt.WheelSettingsWV();
	    cfg.mPosition                    = new Jolt.Vec3(0.0, -0.9 * h/2, z_pos)
	    cfg.mSuspensionDirection         = new Jolt.Vec3(0, -1, Math.tan(ang_caster)).Normalized()
	    cfg.mSteeringAxis                = new Jolt.Vec3(0, 1, -Math.tan(ang_caster)).Normalized()
        cfg.mWheelUp                     = new Jolt.Vec3(0, 1, 0)
        cfg.mWheelForward                = new Jolt.Vec3(0, 0, 1)
	    cfg.mSuspensionMinLength         = sus_min
	    cfg.mSuspensionMaxLength         = sus_max
	    cfg.mSuspensionSpring.mFrequency = sus_freq
	    cfg.mRadius                      = r
	    cfg.mWidth                       = w
        // WV specific
	    cfg.mMaxSteerAngle               = ang_steer
	    cfg.mMaxBrakeTorque              = torque_break
        cfg.mMaxHandBrakeTorque          = torque_hbreak

        const idx   = this.vehicle.mWheels.push_back(cfg) - 1
        /// create GUI wheel (CC: move this to jolt_core)
		const wheel =
              new THREE.Mesh(new THREE.CylinderGeometry(r, r, w, 20, 1), wheelMaterial)
		wheel.updateLocalTransform = ()=>{
			let tsfm = this.cnst.GetWheelLocalTransform(idx, this.right, this.up)
			wheel.position.copy(wrapVec3(tsfm.GetTranslation()))
			wheel.quaternion.copy(wrapQuat(tsfm.GetRotation().GetQuaternion()))
		}
		wheel.updateLocalTransform()
        
        this.jolt._addToScene(wheel)
    }
    addAntiRoll(left, right, stiff=1000) {
		const rb = new Jolt.VehicleAntiRollBar()
		rb.mLeftWheel  = left
		rb.mRightWheel = right
        rb.mStiffness  = stiff
        
		this.vehicle.mAntiRollBars.push_back(rb)
    }
    addDifferential(
        left, right,                        // left, right wheel index
        torque_ratio,                       // torque apply to this differential 1.0
        lr_limited_slip_ratio,              // max/min between wheel speed 1.4
        diff_ratio = 3.42,                  // rotation speed between gearbox and wheel 3.42
        lr_split   = 0.5                    // engine torque between l/r wheel 0.5
    ) {
		const d = new Jolt.VehicleDifferentialSettings()
		d.mLeftWheel         = left                      // -1=no wheel
		d.mRightWheel        = right                     // 1  
		d.mDifferentialRatio = diff_ratio                // 1.93 * 40.0 / 16.0 (gear/tire)
        d.mLeftRightSplit    = lr_split                  // 0=left, 0.5=center, 1.0=right
		d.mLimitedSlipRatio  = lr_limited_slip_ratio     // max/min between two wheels
		d.mEngineTorqueRatio = torque_ratio              // 0.5
        
        this.ctrl.mDifferentials.push_back(d)
    }
    addCallback(cnst, type) {
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
    setEngine(
        torque,                                 // max engine torque
        rpm_max, rpm_min,                       // Engine max torque, max RPM, min RPM
        iner = 0.5, damp = 0.2                  // momoent of inertial and angular damping
    ) {
        let eng = this.ctrl.mEngine
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
        let tran = this.ctrl.mTransmission
        // tran.mMode              = ETransmissionMode::Auto
        // tran.mGearRatios        = [ 2.66, 1.78, 1.3, 1.0, 0.74 ]
        // tran.mReverseGearRatios = [ -2.90 ]
        // tran.mSwitchTime        = 0.5
        // tran.mClutchReleaseTime = 0.3
        // tran.mSwitchLatency     = 0.5
		tran.mShiftUpRPM     = rpm_up     //8000
		tran.mShiftDownRPM   = rpm_down   //2000
		tran.mClutchStrength = clutch     //2
    }
    setMotorCycleDiff() {
        this.addDifferential(-1, 1, 1.0, 1.4, 1.93 * 40.0 / 16.0)
    }
    setWheeledCarDiff(
        fb_torque_ratio,
        fb_limited_slip_ratio = 1.4,           // max/min between wheels speed
        lr_limited_slip_ratio = 1.4
    ) {
		this.ctrl.mDifferentialLimitedSlipRatio = fb_limited_slip_ratio
        this.ctrl.mDifferentials.clear()
		this.addDifferential(                  ///< Front differential
            FL_WHEEL, FR_WHEEL,
            fb_torque_ratio,
            lr_limited_slip_ratio)
        
        if (fb_torque_ratio==1.0) return       /// * 4x4?
        
        this.addDifferential(                  ///< Rear differential
			BL_WHEEL, BR_WHEEL,
            1.0 - fb_torque_ratio,             /// * check total torque sum = 1.0
            lr_limited_slip_ratio)
}