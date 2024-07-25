///
/// @file
/// @brief weForth - Jolt interface
///
'use strict'

const MAX_TYPE = 4
const COLOR_LST= [0xff0000, 0xd9b1a3, 0x4d4139, 0xccad33 ]

function rnd(n) { return n * (Math.random() - 0.5) }
function rnd_q4() {
    let v3 = new Jolt.Vec3(0.001 + Math.random(), Math.random(), Math.random())
    let q4 = Jolt.Quat.prototype.sRotation(v3.Normalized(), 2 * Math.PI * Math.random())
    Jolt.destroy(v3)
    return q4
}
function rnd_shape(t) {
    if (t >= MAX_TYPE) t = 0
    let shape  = null

    switch (t) {
    case 0: {                     // Sphere
        let r = 0.5 + Math.random()
        shape = new Jolt.SphereShape(r, null)
        break
    }
    case 1: {                    // Box
        let sx = 1 + Math.random()
        let sy = 1 + Math.random()
        let sz = 1 + Math.random()
        shape = new Jolt.BoxShape(new Jolt.Vec3(sx * 0.5, sy * 0.5, sz * 0.5), 0.05, null)
        break
    }
    case 2: {                    // Cylinder
        let r = 0.5 + Math.random()
        let h2= 0.5 + 0.5 * Math.random()
        shape = new Jolt.CylinderShape(h2, r, 0.05, null)
        break
    }
    case 3: {                    // Static compound shape
        let config = new Jolt.StaticCompoundShapeSettings()
        let l = 1.0 + Math.random()
        let r2 = 0.5 + 0.5 * Math.random()
        let r1 = 0.5 * r2
        config.AddShape(new Jolt.Vec3(-l, 0, 0), Jolt.Quat.prototype.sIdentity(), new Jolt.SphereShapeSettings(r2))
        config.AddShape(new Jolt.Vec3( l, 0, 0), Jolt.Quat.prototype.sIdentity(), new Jolt.SphereShapeSettings(r2))
        config.AddShape(new Jolt.Vec3( 0, 0, 0), Jolt.Quat.prototype.sRotation(new Jolt.Vec3(0, 0, 1), 0.5 * Math.PI), new Jolt.CapsuleShapeSettings(l, r1))
        shape = config.Create().Get()
        Jolt.destroy(config)
        break
    }
    }
    return shape
}

export default function(jolt) {
    let idx   = Math.floor(Math.random() * MAX_TYPE)
    let shape = rnd_shape(idx)
    let pos   = new Jolt.RVec3(rnd(20), 20, rnd(20))
    let rot   = rnd_q4()
    jolt.add(shape, pos, rot, COLOR_LST[idx])
}

/*
import JoltCore from './jolt_core.js'
import initJolt from './js/jolt-physics.wasm-compat.js'

(function() {
    const vu = document.getElementById('dsp')
    let jolt = null
    initJolt().then(Jolt=>{
        window.Jolt = Jolt
        jolt = new JoltCore(
            vu.offsetWidth, vu.offsetHeight,
            window.devicePixelRatio, onUpdate)
        window.addEventListener('resize',
            ()=>jolt.resize(vu.offsetWidth, vu.offsetHeight), false)

        let next = PERIOD
        function onUpdate(t) {
            if (jolt.length < MAX_OBJ && t > next) {
                let idx   = Math.floor(Math.random() * MAX_TYPE)
                let shape = randomShape(idx)
                let pos   = new Jolt.RVec3(rrnd(25), 20, rnd(25))
                let rot   = randomQuat()
                jolt.add(shape, pos, rot, COLOR_LST[idx])
                next = t + PERIOD
            }
        }
        ///> start Jolt engine
        jolt.addMeshFloor(30, 1, 4, 0, 5, 0)
        jolt.render()
    })
})();
*/
