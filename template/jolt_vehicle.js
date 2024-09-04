export default class {
    constructor(jolt) {
        this.jolt    = jolt
        this.right   = new Jolt.Vec3(0, 1, 0)
        this.up      = new Jolt.Vec3(1, 0, 0)
        
		const motorcycle  = dynamicObjects[dynamicObjects.length - 1]
		const modelWheels = [];
		for (let i = 0; i < vehicle.mWheels.size(); i++) {
			modelWheels.push(createThreeWheel(constraint, i, motorcycle));
		}
    }
    diff(left, right, ratio) {
		const diff = new Jolt.VehicleDifferentialSettings()
		diff.mLeftWheel         = left  //-1
		diff.mRightWheel        = right //1
		diff.mDifferentialRatio = ratio //1.93 * 40.0 / 16.0
        return diff
    }
    motorCycle(
        w, h, l,                        // body width, height, length
        torque,                         // Engine max torque
        rpm_max, rpm_min,               // Engine max, min RPM
        rpm_up, rpm_down                // Transmission shift RPM
    ) {
		// Create motorcycle body
		const shape  = new Jolt.OffsetCenterOfMassShapeSettings(
            new Jolt.Vec3(0, -h/2, 0),
			new Jolt.BoxShapeSettings(new Jolt.Vec3(w/2, h/2, l/2))
        ).Create().Get()
        
		const ctrl    = new Jolt.MotorcycleControllerSettings()
		ctrl.mEngine.mMaxTorque            = torque     //150
		ctrl.mEngine.mMinRPM               = rpm_min    //1000
		ctrl.mEngine.mMaxRPM               = rpm_max    //10000
        
		ctrl.mTransmission.mShiftDownRPM   = rpm_down   //2000
		ctrl.mTransmission.mShiftUpRPM     = rpm_up     //8000
		ctrl.mTransmission.mClutchStrength = 2
        
		ctrl.mDifferentials.clear()
		ctrl.mDifferentials.push_back(diff)
        
        const vehicle = new Jolt.VehicleConstraintSettings()
		vehicle.mWheels.clear()
		vehicle.mMaxPitchRollAngle = maxPitchRollAngle
		vehicle.mController        = ctrl

        jolt.addVehicle(shape, ds, color, vehicle, Jolt.MotorcycleController)
    }
    _wheel(
        r, w, h, pos,                   // wheel radius, width, height, and position
        steer_ang, caster_ang,          // steeringAngle, casterAngle
        t_break,                        // breakTorque
        sus_max, sus_min, sus_freq      // suspension max, min lengths, and frequency
    ) {
	    const w = new Jolt.WheelSettingsWV();
	    w.mRadius              = r
	    w.mWidth               = w
	    w.mPosition            = new Jolt.Vec3(0.0, -0.9 * halfVehicleHeight, frontWheelPosZ)
	    w.mSteeringAxis        = new Jolt.Vec3(0, 1, -Math.tan(caster_ang)).Normalized()
	    w.mMaxSteerAngle       = steer_ang
	    w.mMaxBrakeTorque      = t_break
	    w.mSuspensionDirection = new Jolt.Vec3(0, -1, Math.tan(caster_ang)).Normalized()
	    w.mSuspensionMaxLength         = sus_max
	    w.mSuspensionMinLength         = sus_min
	    w.mSuspensionSpring.mFrequency = sus_freq
    }
}
