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
    let shape = new Jolt.MeshShapeSettings(tlst, mati).Create().Get();
    Jolt.destroy(tlst);
    Jolt.destroy(mati);

    return shape
}
function get_shape(t, v=null) {
    const rx = ()=>0.5 + Math.random()
    let x = v || (t==0 ? [ 30, 1, 0.8 ] : [ rx(), rx(), rx() ])
    let shape = null
    switch (t) {
    case 0: {
        shape = build_mesh(x[0], x[1], x[2])
        break
    }
    case 1: {                       // Sphere
        let r = x[0]
        shape = new Jolt.SphereShape(r, null)
        break
    }
    case 2: {                       // Box
        let sx = x[0], sy = x[1], sz = x[2]
        shape = new Jolt.BoxShape(new Jolt.Vec3(sx, sy, sz), 0.05, null)
        break
    }
    case 3: {                       // Cylinder
        let r = x[0], h2 = x[1]
        shape = new Jolt.CylinderShape(r, h2, 0.05, null)
        break
    }
    case 4: {                       // Capsule
        let l = x[0], r1 = x[1] * 0.5
        shape = new Jolt.CapsuleShape(l, r1, 0.05, null)
        break
    }
    case 5: {                       // tapered capsule
        let l = x[0], r1 = x[1] * 0.5, r2 = x[2] * 0.5
		let config = new Jolt.TaperedCapsuleShapeSettings(l, r1, r2, null)
        shape = config.Create().Get()
        Jolt.destroy(config)
        break
    }
    case 6: {                       // Static compound shape
        let config = new Jolt.StaticCompoundShapeSettings()
        let l = x[0], r2 = x[1] * 0.5, r1 = r2 * 0.5
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
        shape = config.Create().Get()
        Jolt.destroy(config)
        break
    }
    default: console.log('?rnd_shape t='+t); break
    }
    return shape
}

let   req_q   = []
const CMD_LST = [ 'mesh', 'body', 'drop' ]

function jolt_req(req) {                    ///> jolt job queue
    if (!req ||
        CMD_LST.indexOf(req[1])<0) return 0 /// * not Jolt command
    console.log(req)
    req.push(Date.now() - req[0])           /// * encode timediff
    req_q.push(req)
    return 1
}
function jolt_update(jolt) {
    const v = req_q.shift()                 ///> pop from job queue
    if (!v) return                          /// * queue empty, bail

    v.push(Date.now() - req[0])             /// * encode timediff
    console.log(v)                          /// * debug trace
    
    const cmd = v[1]                        ///> Jolt command
    const n   = v[2]|0                      ///> color or object id
    const x   = v[3]                        ///> geometry parameters
    const ds  = v[4]                        ///> shape dynaset

    switch (CMD_LST.indexOf(cmd)) {
    case 0: return jolt.add(get_shape(0, x),       ds, n, true)   // fixed=true
    case 1: return jolt.add(get_shape(ds[1]|0, x), ds, n)
    case 2: return jolt.drop(n)
    default: console.log('unknown cmd='+cmd); break
    }
    return 0
}
