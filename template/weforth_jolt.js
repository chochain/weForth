///
/// @file
/// @brief weForth - Jolt interface
///
'use strict'

const MAX_TYPE = 6
const COLOR_LST= [0xc0f0c0, 0xf04040, 0xa0a0f0, 0x80f080, 0xf0d080, 0xf0a0f0 ]

function rnd(n) { return n * (Math.random() - 0.5) }
function rnd_q4() {
    let v3 = new Jolt.Vec3(0.001 + Math.random(), Math.random(), Math.random())
    let q4 = Jolt.Quat.prototype.sRotation(v3.Normalized(), 2 * Math.PI * Math.random())
    Jolt.destroy(v3)
    return q4
}
function build_mesh(n, sz, h) {    // nxn, sz=tileSize, h:max_height
    // Create regular grid of triangles
    let hmap = (x, y)=> h * Math.sin(x / 2) * Math.cos(y / 3)
    let tlst = new Jolt.TriangleList;
    tlst.resize(n * n * 2);
    for (let x = 0; x < n; ++x) {
        for (let z = 0; z < n; ++z) {
            let ctr= n * sz / 2
            let x1 = sz * x - ctr
            let z1 = sz * z - ctr
            let x2 = x1 + sz
            let z2 = z1 + sz
            {
                let t  = tlst.at((x * n + z) * 2)
                let v1 = t.get_mV(0), v2 = t.get_mV(1), v3 = t.get_mV(2)
                v1.x = x1, v1.z = z1, v1.y = hmap(x, z)
                v2.x = x1, v2.z = z2, v2.y = hmap(x, z + 1)
                v3.x = x2, v3.z = z2, v3.y = hmap(x + 1, z + 1)
            }
            {
                let t  = tlst.at((x * n + z) * 2 + 1)
                let v1 = t.get_mV(0), v2 = t.get_mV(1), v3 = t.get_mV(2);
                v1.x = x1, v1.z = z1, v1.y = hmap(x, z)
                v2.x = x2, v2.z = z2, v2.y = hmap(x + 1, z + 1)
                v3.x = x2, v3.z = z1, v3.y = hmap(x + 1, z)
            }
        }
    }
    let mati  = new Jolt.PhysicsMaterialList;
    let shape = new Jolt.MeshShapeSettings(tlst, mati).Create().Get();
    Jolt.destroy(tlst);
    Jolt.destroy(mati);

    return shape
}
function rnd_shape(t) {
    if (t > MAX_TYPE) t = 1
    let shape  = null

    switch (t) {
    case 0: {
        shape = build_mesh(30, 1, 0.8)
        break
    }
    case 1: {                     // Sphere
        let r = 0.5 + Math.random()
        shape = new Jolt.SphereShape(r, null)
        break
    }
    case 2: {                    // Box
        let sx = 1 + Math.random()
        let sy = 1 + Math.random()
        let sz = 1 + Math.random()
        shape = new Jolt.BoxShape(new Jolt.Vec3(sx * 0.5, sy * 0.5, sz * 0.5), 0.05, null)
        break
    }
    case 3: {                    // Cylinder
        let r = 0.5 + Math.random()
        let h2= 0.5 + 0.5 * Math.random()
        shape = new Jolt.CylinderShape(h2, r, 0.05, null)
        break
    }
    case 4: {                     // Capsule
        let l = 1.0 + Math.random()
        let r1= 0.5 + 0.5 * Math.random() 
        shape = new Jolt.CapsuleShape(l, r1, 0.05, null)
        break
    }
    case 5: {                    // Static compound shape
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
    
    let idx   = Math.ceil(Math.random() * MAX_TYPE)
    let shape = rnd_shape(idx)
    let pos   = new Jolt.RVec3(rnd(20), 20, rnd(20))
    let rot   = rnd_q4()
    
//    jolt.add(shape, pos, rot, COLOR_LST[idx])
    jolt.add(shape, pos, rot, HSV(idx * 16))
    next = t + PERIOD
}

function get_shape(t, x) {
    let shape = null
    switch (t) {
    case 'mesh':
        shape = build_mesh(30, 1, 0.8)
        break
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

function jolt_enq(cmd) {
    const req = cmd.split(/\s+/)            ///> split command into request (array)
    if (req.length < 2) return 0            /// * skip LOGO command
    
    req_q.push(req)
    return 1                                /// completed
}

function jolt_update(jolt) {
    const v = req_q.shift()                 ///> pop first ops from queue
    if (!v) return                          /// * empty queue
    
    console.log(v)
    
    const t  = v[0],   color = v[1]|0       ///> type, color
    const px = v[2]|0, ps    = v[3]|0       ///> geometry, shape
    const wa = wasmExports
    const mem= wa.vm_mem()
//    const x  = new Float32Array(wa.memory.buffer, mem+px, 3)  ///> dimensions
//    const s  = new Float32Array(wa.memory.buffer, mem+ps, 8)  ///> id, pos, rot
    const x  = new Uint8Array(wa.memory.buffer, mem+px, 12)  ///> dimensions
    const s  = new Uint8Array(wa.memory.buffer, mem+ps, 32)  ///> id, pos, rot
    console.log(mem)
    console.log(px.toString(16))
    console.log(x)
    console.log(ps.toString(16))
    console.log(s)
//    const id    = s[0]
//    const pos   = new Jolt.RVec3(s[1], s[2], s[3])
    //    const rot   = new Jolt.Quat(s[4], s[5], s[6], s[7])
    const msh   = t=='mesh'
    const pos   = msh ? new Jolt.RVec3(0, -5, 0)  : new Jolt.RVec3(rnd(20), 20, rnd(20))
    const rot   = msh ? new Jolt.Quat(0, 0, 0, 1) : rnd_q4()
    const idx   = msh ? 0 : Math.ceil(Math.random() * MAX_TYPE)
    const shape = rnd_shape(idx)            /// get_shape(t, x)

    jolt.add(shape, pos, rot, COLOR_LST[idx], msh)
}
