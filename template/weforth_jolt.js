///
/// @file
/// @brief weForth - Jolt interface
///
'use strict'

const MAX_TYPE = 5
const COLOR_LST= [0xf04040, 0xa0a0f0, 0x80f080, 0xf0d080, 0xf0a0f0 ]

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
    case 3: {                     // Capsule
        let l = 1.0 + Math.random()
        let r1= 0.5 + 0.5 * Math.random() 
        shape = new Jolt.CapsuleShape(l, r1, 0.05, null)
        break
    }
    case 4: {                    // Static compound shape
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
const MAX_OBJ = 100
const PERIOD  = 0.25
let   next    = 0

function rnd_update(jolt) {
    let t = jolt.time
    if (jolt.length >= MAX_OBJ || t < next) return
    
    let idx   = Math.floor(Math.random() * MAX_TYPE)
    let shape = rnd_shape(idx)
    let pos   = new Jolt.RVec3(rnd(20), 20, rnd(20))
    let rot   = rnd_q4()
    
    jolt.add(shape, pos, rot, COLOR_LST[idx])
    next = t + PERIOD
}

function get_shape(t, x) {
    let shape = null
    switch (t) {
    case 'box':
        shape = new Jolt.BoxShape(new Jolt.Vec3(x[1], x[2], x[3]), 0.05, null)
        break
    case 'ball':
        shape = new Jolt.SphereShape(x[1], null)
        break
    case 'pipe':
        shape = new Jolt.CylinderShape(x[1], x[2], 0.05, null)
        break
    case 'pill':
        shape = new Jolt.CapsuleShape(x[1], x[2], 0.05, null)
        break
    default: console.log('?op:'+op); break
    }
    return shape
}

let req_q = []

function jolt_enq(ops) {
    req_q.push(ops)
}

function jolt_update(jolt) {
    const req= req_q.shift()                ///> pop first ops from queue
    if (!req) return                        /// * empty queue
    
    console.log(req)
    const wa = wasmExports
    const av = req.split(' ')
    const t  = av[0], p0 = av[2], p1 = av[3]              ///> type, geoms, shape
    const x  = new Float32Array(wa.memory.buffer, p0, 3)  ///> sizing
    const v  = new Float32Array(wa.memory.buffer, p1, 8)  ///> id, pos, rot
    console.log(x)
    console.log(v)
//    const id    = v[0]
//    const pos   = new Jolt.RVec3(v[1], v[2], v[3])
//    const rot   = new Jolt.Quat(v[4], v[5], v[6], v[7])
//    const color = av[1] | 0
    const pos   = new Jolt.RVec3(rnd(20), 20, rnd(20))
    const rot   = rnd_q4()
    const idx   = Math.floor(Math.random() * MAX_TYPE)
    const color = COLOR_LST[idx]
    const shape = rnd_shape(idx)            /// get_shape(t, x)

    console.log(shape)
    console.log(color)
        
    jolt.add(shape, pos, rot, color)
}
