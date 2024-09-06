///
/// @file
/// @brief weForth - Jolt interface
///
'use strict'

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
    let config= new Jolt.MeshShapeSettings(tlst, mati)
    Jolt.destroy(tlst);
    Jolt.destroy(mati);

    return config
}
///
///> static shape 
///
function get_shape(t, v=null) {
    const rx = ()=>0.5 + Math.random()
    let x = v || (t==0 ? [ 30, 1, 0.8 ] : [ rx(), rx(), rx() ])
    let config = null
    switch (t) {
    case 0: {
        config = build_mesh(x[0], x[1], x[2])
        break
    }
    case 1: {                       // Sphere
        let r = x[0]
        config = new Jolt.SphereShapeSettings(r, null)
        break
    }
    case 2: {                       // Box
        let sx = x[0], sy = x[1], sz = x[2]
        config = new Jolt.BoxShapeSettings(new Jolt.Vec3(sx, sy, sz), 0.05, null)
        break
    }
    case 3: {                       // Cylinder
        let r = x[0], h2 = x[1]
        config = new Jolt.CylinderShapeSettings(r, h2, 0.05, null)
        break
    }
    case 4: {                       // Capsule
        let l = x[0], r1 = x[1] * 0.5
        config = new Jolt.CapsuleShapeSettings(l, r1, 0.05, null)
        break
    }
    case 5: {                       // tapered capsule
        let l = x[0], r1 = x[1] * 0.5, r2 = x[2] * 0.5
		config = new Jolt.TaperedCapsuleShapeSettings(l, r1, r2, null)
        break
    }
    case 6: {                       // Static compound shape
        let l = x[0], r2 = x[1] * 0.5, r1 = r2 * 0.5
        config = new Jolt.StaticCompoundShapeSettings()
        config.AddShape(
            new Jolt.Vec3(-l, 0, 0),
            Jolt.Quat.prototype.sIdentity(),
            new Jolt.SphereShapeSettings(r2))
        config.AddShape(
            new Jolt.Vec3( l, 0, 0),
            Jolt.Quat.prototype.sIdentity(),
            new Jolt.SphereShapeSettings(r2))
        config.AddShape(
            new Jolt.Vec3( 0, 0, 0),
            Jolt.Quat.prototype.sRotation(new Jolt.Vec3(0, 0, 1), 0.5 * Math.PI),
            new Jolt.CapsuleShapeSettings(l, r1))
        break
    }
    default: console.log('?rnd_shape t='+t); break
    }
    let shape = null
    if (config) {
        shape = config.Create().Get()
        Jolt.destroy(config)
    }
    return shape
}
///
///> componded object implementation (2x slower)
///
function get_shape_compound(t, v=null) {
    const rx = ()=>0.5 + Math.random()
    let x = v || (t==0 ? [ 30, 1, 0.8 ] : [ rx(), rx(), rx() ])
    const config = new Jolt.StaticCompoundShapeSettings()
    const add    = (s, pos=new Jolt.Vec3(0,0,0), rot=Jolt.Quat.prototype.sIdentity())=>{
        config.AddShape(pos, rot, s)
    }
    switch (t) {
    case 0: {
        add(build_mesh(x[0], x[1], x[2]))
        break
    }
    case 1: {                       // Sphere
        let r = x[0]
        add(new Jolt.SphereShapeSettings(r, null))
        break
    }
    case 2: {                       // Box
        let sx = x[0], sy = x[1], sz = x[2]
        add(new Jolt.BoxShapeSettings(new Jolt.Vec3(sx, sy, sz), 0.05, null))
        break
    }
    case 3: {                       // Cylinder
        let r = x[0], h2 = x[1]
        add(new Jolt.CylinderShapeSettings(r, h2, 0.05, null))
        break
    }
    case 4: {                       // Capsule
        let l = x[0], r1 = x[1] * 0.5
        add(new Jolt.CapsuleShapeSettings(l, r1, 0.05, null))
        break
    }
    case 5: {                       // tapered capsule
        let l = x[0], r1 = x[1] * 0.5, r2 = x[2] * 0.5
		add(new Jolt.TaperedCapsuleShapeSettings(l, r1, r2, null))
        break
    }
    case 6: {                       // Static compound shape
        let l = x[0], r2 = x[1] * 0.5, r1 = r2 * 0.5
//        config = new Jolt.StaticCompoundShapeSettings()
        add(new Jolt.SphereShapeSettings(r2), new Jolt.Vec3(-l, 0, 0))
        add(new Jolt.SphereShapeSettings(r2), new Jolt.Vec3( l, 0, 0))
        add(new Jolt.CapsuleShapeSettings(l, r1),
            new Jolt.Vec3( 0, 0, 0),
            Jolt.Quat.prototype.sRotation(new Jolt.Vec3(0, 0, 1), 0.5 * Math.PI))
        break
    }
    default: console.log('?rnd_shape t='+t); break
    }
    const shape = config.Create().Get()
    Jolt.destroy(config)
    return shape
}

let   req_q   = []
const CMD_LST = [ 'mesh', 'body', 'drop' ]

function jolt_req(req) {                    ///> jolt job queue
    if (!req ||
        CMD_LST.indexOf(req[1])<0) return 0 /// * not Jolt command
    req.push(Date.now() - req[0])           /// * encode timediff
    req_q.push(req)
    return 1
}
function jolt_update(jolt) {
    const get_q4 = (
        x=Math.random(), y=Math.random(), z=Math.random(),
        w=2*Math.PI*Math.random())=>{
            let v3 = new Jolt.Vec3(0.001+x, y, z).Normalized()
            let q4 = (x==0 && y==0 && z==0)
                ? new Jolt.Quat(0, 0, 0, 1)
                : Jolt.Quat.prototype.sRotation(v3, w)
            Jolt.destroy(v3)
            return q4
        }
    const v = req_q.shift()                 ///> pop from job queue
    if (!v) return                          /// * queue empty, bail

    v.push(Date.now() - v[0])               /// * encode timediff
    console.log(v)                          /// * debug trace
    
    const cmd  = v[1]                       ///> Jolt command
    const n    = v[2]|0                     ///> object_id
    const color= v[2]|0                     ///> color (shared param with object_id)
    const x    = v[3]                       ///> geometry parameters
    const ds   = v[4]                       ///> shape dynaset
    const id   = ds[0]|0
    const pos  = new Jolt.RVec3(ds[2], ds[3], ds[4])     // ds.slice(2,5) doesn't work?
    const rot  = get_q4(ds[5], ds[6], ds[7], ds[8])

    switch (cmd) {
    case 'mesh':
        const mesh = get_shape(0, x)
        return jolt.addShape(id, mesh, pos, rot, color, 0, false)
    case 'body':
        const shape = get_shape(ds[1]|0)
        const nobj  = jolt.addShape(id, shape, pos, rot, color)
        const lv    = new Jolt.Vec3(ds[9], ds[10], ds[11])
        const av    = new Jolt.Vec3(ds[12], ds[13], ds[14])
        this.setVelocity(id, lv, av)
        return nobj
    case 'drop': return jolt.remove(n)
    default: console.log('unknown cmd='+cmd); break
    }
    return 0
}
