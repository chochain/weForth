export default class {
    constructor(jolt, mtype) {
        this.jolt    = jolt
        this.right   = new Jolt.Vec3(0, 1, 0)
        this.up      = new Jolt.Vec3(1, 0, 0)
        
        this.vehicle = new Jolt.VehicleConstraintSettings()

        let ctrl;
        switch (mtype) {
        case '2': ctrl = new Jolt.MotorcycleControllerSettings();     break
        case '4': ctrl = new Jolt.WheeledVehicleControllerSettings(); break
        case 'T': ctrl = new Jolt.TrackedVehicleControllerSettings(); break
        }
		ctrl.mDifferentials.clear()
        this.vehicle.mController = this.ctrl = ctrl
        
		this.vehicle.mAntiRollBars.clear()
		this.vehicle.mWheels.clear()
        this.wheels = []
        
		const motorcycle  = dynamicObjects[dynamicObjects.length - 1]
		const modelWheels = [];
		for (let i = 0; i < vehicle.mWheels.size(); i++) {
			modelWheels.push(createThreeWheel(constraint, i, motorcycle));
		}
    }
    createBody(
        w, h, l, mass,                  // vehicle width, height, length, mass
        ang_pitch_roll,                 // max pitch/roll angle
        ds, color                       // dynaset and body color
    ) {
		// Create motorcycle body
		const shape  = new Jolt.OffsetCenterOfMassShapeSettings(
            new Jolt.Vec3(0, -h/2, 0),
			new Jolt.BoxShapeSettings(new Jolt.Vec3(w/2, h/2, l/2))
        ).Create().Get()
        
		this.vehicle.mMaxPitchRollAngle = maxPitchRollAngle
        
//        jolt.addVehicle(shape, ds, color, this.vehicle, Jolt.MotorcycleController)
    }
    addWheel(
        r, w, h, z_pos,                 // wheel radius, width, height, and position in Z
        ang_steer, ang_caster,          // steering angle, caster angle
        sus_freq,                       // suspension frequency, and max/min lengths
        sus_min  = 0.3, sus_max = 0.5,
        torque_break  = 1500,           // break torque, hand break torque
        torque_hbreak = 4000
    ) {
	    const w = new Jolt.WheelSettingsWV();
	    w.mPosition                    = new Jolt.Vec3(0.0, -0.9 * h/2, z_pos)
	    w.mSuspensionDirection         = new Jolt.Vec3(0, -1, Math.tan(ang_caster)).Normalized()
	    w.mSteeringAxis                = new Jolt.Vec3(0, 1, -Math.tan(ang_caster)).Normalized()
        w.mWheelUp                     = new Jolt.Vec3(0, 1, 0)
        m.mWheelForward                = new Jolt.Vec3(0, 0, 1)
	    w.mSuspensionMinLength         = sus_min
	    w.mSuspensionMaxLength         = sus_max
	    w.mSuspensionSpring.mFrequency = sus_freq
	    w.mRadius                      = r
	    w.mWidth                       = w
        // WV specific
	    w.mMaxSteerAngle               = ang_steer
	    w.mMaxBrakeTorque              = torque_break
        w.mMaxHandBrakeTorque          = torque_hbreak

        this.vehicle.mWheels.push_back(w)
    }
    addAntiRoll(left, right, stiff=1000) {
		const rb = new Jolt.VehicleAntiRollBar()
		rb.mLeftWheel  = left
		rb.mRightWheel = right
        rb.mStiffness  = stiff
        
		this.vehicle.mAntiRollBars.push_back(rb)
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
    motorCycleDiff() {
        this.addDifferential(-1, 1, 1.0, 1.4, 1.93 * 40.0 / 16.0)
    }
    frontWheeledDriveDiff(
        fb_limited_slip_ratio = 1.4,           // max/min between wheels speed
        lr_limited_slip_ratio = 1.4
    ) {
		this.ctrl.mDifferentialLimitedSlipRatio = fb_limited_slip_ratio
        this.ctrl.mDifferentials.clear()
		this.addDifferential(                  ///< Front differential
            FL_WHEEL, FR_WHEEL,
            fb_torque_ratio,
            lr_limited_slip_ratio)
    }
    fourWheelDriveDiff(
        fb_torque_ratio,
        fb_limited_slip_ratio = 1.4,           // max/min between wheels speed
        lr_limited_slip_ratio = 1.4
    ) {
        this.frontWheelDriveDiff()
        this.addDifferential
		diff(                                  ///< Rear differential
			BL_WHEEL, BR_WHEEL,
            1.0 - fb_torque_ratio,             /// * check total torque sum = 1.0
            lr_limited_slip_ratio)
}
