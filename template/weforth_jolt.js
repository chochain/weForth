///
/// @file
/// @brief weForth - Jolt interface
///
'use strict'

function build_sandbox(n, sz, h) {    // nxn, sz=tileSize, h:max_height
    // Create regular grid of triangles
    const n2   = n * 0.5, ctr = n2 * sz
    const bump = (x, z) => h * ((x==z || (x+z)==n) ? 0.3 : 0)
    const hmap = (x, z) => {
        return (x==0 || x==n || z==0 || z==n)
            ? h * 8
            : (h * (1 - Math.cos((x-n2)/4) * Math.cos((z-n2)/3)) + bump(x, z))
    }
    const tlst = new Jolt.TriangleList
    tlst.resize(n * n * 2)         // allot the space

    for (let x = 0; x < n; x++) {
        for (let z = 0; z < n; z++) {
            const x1 = sz * x - ctr, x2 = x1 + sz
            const z1 = sz * z - ctr, z2 = z1 + sz
            {
                const t  = tlst.at((x * n + z) * 2)
                const v1 = t.get_mV(0), v2 = t.get_mV(1), v3 = t.get_mV(2)
                v1.x = x1, v1.z = z1, v1.y = hmap(x, z)
                v2.x = x1, v2.z = z2, v2.y = hmap(x, z + 1)
                v3.x = x2, v3.z = z2, v3.y = hmap(x + 1, z + 1)
            }
            {
                const t  = tlst.at((x * n + z) * 2 + 1)
                const v1 = t.get_mV(0), v2 = t.get_mV(1), v3 = t.get_mV(2)
                v1.x = x1, v1.z = z1, v1.y = hmap(x, z)
                v2.x = x2, v2.z = z2, v2.y = hmap(x + 1, z + 1)
                v3.x = x2, v3.z = z1, v3.y = hmap(x + 1, z)
            }
        }
    }
    const mati  = new Jolt.PhysicsMaterialList
    const config= new Jolt.MeshShapeSettings(tlst, mati)
    Jolt.destroy(tlst);
    Jolt.destroy(mati);

    return config
}
///
///> static shape 
///
function get_shape(t, v=null) {
    const rx = ()=>0.5 + Math.random()
    const x = v || (t==0 ? [ 30, 1, 0.8 ] : [ rx(), rx(), rx() ])
    let config = null
    switch (t) {
    case 0: {
        config = build_sandbox(x[0], x[1], x[2])
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
    const x  = v || (t==0 ? [ 30, 1, 0.8 ] : [ rx(), rx(), rx() ])
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
function get_q4(
    x = Math.random(),
    y = Math.random(),
    z = Math.random(),
    w = 2*Math.PI * Math.random()
) {
    const v3 = new Jolt.Vec3(0.001+x, y, z).Normalized()
    const q4 = (x==0 && y==0 && z==0)
        ? new Jolt.Quat(0, 0, 0, 1)
        : Jolt.Quat.prototype.sRotation(v3, w)
    Jolt.destroy(v3)
    return q4
}

let   req_q   = []
const CMD_LST = [
    'sandbox', 'body', 'drop',
    'bike', 'fwd', 'rwd', '4x4',
    'wheel', 'engine', 'gearbox',
    'start'
]
let   xkey    = {
	F: false, // forward
	B: false, // backward
	L: false, // left
	R: false, // right
	X: false, // break
    callback: null
}
function xkey_down(event) {
    switch(event.key) {             // keyCode
	case 'w': xkey.F = true; break
	case 's': xkey.B = true; break
	case 'a': xkey.L = true; break
	case 'd': xkey.R = true; break
    }
}
function xkey_up(event) {
    switch(event.key) {             // keyCode
	case 'w': xkey.F = false; break
	case 's': xkey.B = false; break
	case 'a': xkey.L = false; break
	case 'd': xkey.R = false; break
    }
}

let this_veh = null                          ///< lastly defined vehicle
let veh_cb  = []                             ///< list of vehicle to update

function veh_update_req(veh) {
    veh_cb.push(veh)                         /// * add to callback list
    veh.core.tick(veh.id)                    /// * activate body physics
}
function veh_update() {
    veh_cb.forEach(veh=>{
        veh.update()
        // v.follow()
    })
}
function jolt_req(req) {                    ///> jolt job queue
    if (!req ||
        CMD_LST.indexOf(req[1])<0) return 0 /// * not Jolt command
    req.push(Date.now() - req[0])           /// * encode timediff
    req_q.push(req)
    return 1
}
function jolt_update(core) {
    if (xkey.callback) xkey.callback(core)
    veh_update()
    
    const v = req_q.shift()                 ///> pop from command request queue
    if (!v) return                          /// * queue empty, bail

    v.push(Date.now() - v[0])               /// * encode timediff
    console.log(v)                          /// * debug trace
    
    const cmd  = v[1]                       ///> Jolt command
    const n    = v[2]|0                     ///> object_id (integer)
    const color= v[2]|0                     ///> color in "0xaabbcc" format
    const x    = v[3]                       ///> geometry parameters
    const ds   = v[4]                       ///> shape dynaset

    let id, pos, rot
    if (ds) {
        id   = ds[0]|0
        pos  = new Jolt.RVec3(ds[2], ds[3], ds[4])  // ds.slice(2,5) doesn't work?
        rot  = get_q4(ds[5], ds[6], ds[7], ds[8])
    }
    switch (cmd) {
    case 'sandbox':
        const sb = get_shape(0, x)
        return core.addShape(id, sb, pos, rot, color, 0, false)
    case 'body':
        const shape = get_shape(ds[1]|0)
        const nobj  = core.addShape(id, shape, pos, rot, color)
        const lv    = new Jolt.Vec3(ds[9], ds[10], ds[11])
        const av    = new Jolt.Vec3(ds[12], ds[13], ds[14])
        core.setVelocity(id, lv, av)
        return nobj
    case 'bike':
        this_veh = new Vehicle(
            core, id, '2',             ///< Jolt core, bike id, '2 wheel model'
            x[0], x[1], x[2],          ///< dim[width, height, length]
            pos, rot, color,           ///< position, rotation, color
            2, 1, 0                    ///< number of wheels, diff, anti-roll bars
        )
        this_veh.useMotorcycleDiff()
        return this_veh
    case 'fwd':
        this_veh = new Vehicle(
            core, id, '4',             ///< Jolt core, car id, '4 wheel model'
            x[0], x[1], x[2],          ///< width, height, length
            pos, rot, color,
            4, 1, 1                    ///< number of wheels, diff, anti-roll bars
        )
        this_veh.useWheeledCarDiff(1)  ///< fb_torque_ratio = 1.0
        this_veh.setAntiRoll(0, 0, 1)
        return this_veh
    case 'rwd':
        this_veh = new Vehicle(
            core, id, '4',             ///< Jolt core, car id, '4 wheel model'
            x[0], x[1], x[2],          ///< width, height, length
            pos, rot, color,
            4, 1, 1                    ///< number of wheels, diff, anti-roll bars
        )
        this_veh.useWheeledCarDiff(0)  ///< fb_torque_ratio = 0.0
        this_veh.setAntiRoll(0, 2, 3)
        return this_veh
    case '4x4':
        this_veh = new Vehicle(
            core, id, '4',             ///< Jolt core, car id, '4 wheel model'
            x[0], x[1], x[2],          ///< width, height, length
            pos, rot, color,
            4, 2, 1                    ///< number of diff, wheels, anti-roll bars
        )
        this_veh.useWheeledCarDiff(0.5)
        return this_veh
    case 'wheel':
        this_veh.setWheel(             ///> create wheel
            id, pos,                   ///< wheel id, pos[x,y,z]
            x[0], x[1], x[2],          ///< dim[r1, r2, w]
            ds[9],  ds[10], ds[11],    ///< suspension freq, min, max
            ds[12], ds[13], ds[14]     ///< steering, caster, break strength
        )
        return this_veh
    case 'engine':
        this_veh.setEngine(
            x[0], x[1], x[2])          ///< torque, max RPM, min RPM
        return this_veh
    case 'gearbox':
        this_veh.setTransmission(
            x[0], x[1], x[2])          ///< clutch strength, up RPM, down RPM
        return this_veh
    case 'start':
        veh_update_req(this_veh)
        return this_veh
    case 'drop': return core.remove(n)
    default: console.log('unknown cmd='+cmd); break
    }
    return 0
}
