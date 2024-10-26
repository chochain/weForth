///
/// @file
/// @brief weForth - Jolt core implementation
///
'use strict'

///> transformation macros
const V3G = v=> new THREE.Vector3(v.GetX(), v.GetY(), v.GetZ())      // => GUI vec3
const Q4G = q=> new THREE.Quaternion(q.GetX(), q.GetY(), q.GetZ(), q.GetW()) // => GUI quaternion 
const V3W = v=> new Jolt.Vec3(v.x, v.y, v.z)                         // => physics vec3
const R3W = v=> new Jolt.RVec3(v.x, v.y, v.z)                        // => physics rvec3
const Q4W = q=> new Jolt.Quat(q.x, q.y, q.z, q.w)                    // => physics quaternion

///> physical object layers
const L_STATIC = 0
const L_MOVING = 1
const L_MAIN   = 2
///
///> initialize JOLT collision layers and filters
///
function setupJolt() {
    let pair = new Jolt.ObjectLayerPairFilterTable(L_MAIN)
    pair.EnableCollision(L_STATIC, L_MOVING)
    pair.EnableCollision(L_MOVING, L_MOVING)

    const bpStatic = new Jolt.BroadPhaseLayer(L_STATIC)
    const bpMoving = new Jolt.BroadPhaseLayer(L_MOVING)
    const bpMain   = L_MAIN

    let bpi = new Jolt.BroadPhaseLayerInterfaceTable(L_MAIN, bpMain)
    bpi.MapObjectToBroadPhaseLayer(L_STATIC, bpStatic)
    bpi.MapObjectToBroadPhaseLayer(L_MOVING, bpMoving)

    let config = new Jolt.JoltSettings()
    config.mObjectLayerPairFilter    = pair
    config.mBroadPhaseLayerInterface = bpi
    config.mObjectVsBroadPhaseLayerFilter =
        new Jolt.ObjectVsBroadPhaseLayerFilterTable(
            config.mBroadPhaseLayerInterface,
            bpMain,
            config.mObjectLayerPairFilter,
            L_MAIN
        )

    let jolt = new Jolt.JoltInterface(config)       ///< create physics system
    Jolt.destroy(config)

    return jolt
}
function setupCaster(core) {
    const caster = new Object()
    const ray    = caster.ray = new Jolt.RRayCast()
    const hit    = caster.hit = new Jolt.RayCastResult()
	caster.Cast  = ()=>{
        core.phyx.GetNarrowPhaseQuery().CastRay(
            ray,
            hit,
            new Jolt.DefaultBroadPhaseLayerFilter(core.jolt.GetObjectVsBroadPhaseLayerFilter(), L_MOVING),
            new Jolt.DefaultObjectLayerFilter(core.jolt.GetObjectLayerPairFilter(), L_MOVING)
        )
        console.log('hit')
        console.log(hit)
        const pt   = ray.GetPointOnRay(hit.mFraction)
        const bid  = hit.mBodyID
        console.log('pt')
        console.log(pt)
        console.log(bid)
        console.log(hit.mSubShapeID2.GetValue())
        if (!core.intf.IsAdded(bid)) {
            console.log('not added')
            return
        }
        const shape= core.intf.GetShape(bid)
        console.log('shape')
        console.log(shape)
        const norm = shape.GetSurfaceNormal(hit.mSubShapeID2, pt)
        console.log('norm')
        console.log(norm)

        core.arrow.position.copy(V3G(pt))
        core.arrow.setDirection(V3G(norm))
    }
    caster.Reset = ()=>caster.hit.Reset()
    return caster
}
///
///> THREE geomotry mesh factory
///
function getMesh(shape) {
    // Get triangle data
    let scale = new Jolt.Vec3(1, 1, 1)
    let tri   = new Jolt.ShapeGetTriangles(
        shape,
        Jolt.AABox.prototype.sBiggest(),
        shape.GetCenterOfMass(),
        Jolt.Quat.prototype.sIdentity(), scale)
    Jolt.destroy(scale)

    // Get a view on the triangle data (does not make a copy)
    let vtx   = new Float32Array(
        Jolt.HEAPF32.buffer,
        tri.GetVerticesData(),
        tri.GetVerticesSize() / Float32Array.BYTES_PER_ELEMENT)

    // Now move the triangle data to a buffer and clone it so that we can free the memory from the C++ heap (which could be limited in size)
    let buf = new THREE.BufferAttribute(vtx, 3).clone()
    Jolt.destroy(tri)

    // Create a three mesh
    let mesh = new THREE.BufferGeometry()
    mesh.setAttribute('position', buf)
    mesh.computeVertexNormals()

    return mesh
}
function checkerTexture() {
	const tldr = new THREE.TextureLoader()           
	const tex = tldr.load('data:image/gif;base64,R0lGODdhAgACAIABAAAAAP///ywAAAAAAgACAAACA0QCBQA7')
	tex.wrapS = tex.wrapT = THREE.RepeatWrapping
	tex.offset.set(0, 0)
	tex.repeat.set(1, 1)
	tex.magFilter = THREE.NearestFilter

    return tex
}
function syncToBody(msh) {
    const body = msh.userData.body
    
    msh.position.copy(V3G(body.GetPosition()))
    msh.quaternion.copy(Q4G(body.GetRotation()))
    
    if (body.GetBodyType() == Jolt.EBodyType_SoftBody) {
        if (msh.userData.updateVertex) {
            msh.userData.updateVertex()
        }
        else msh.geometry = getMesh(body.GetShape())
    }
}
///
///> THREE mesh factory
///
function meshFactory(body, color, tex=null) {
    let shape = body.GetShape()
    let mati  = new THREE.MeshPhongMaterial({
        color: color, shininess:80,
        // map: tex,                          // slow to 1/3
        // transparent:true, opacity:0.75
    })        
    let msh

    switch (shape.GetSubType()) {
    case Jolt.EShapeSubType_Box:
        let box = Jolt.castObject(shape, Jolt.BoxShape)
        let dim = V3G(box.GetHalfExtent()).multiplyScalar(2)
        msh = new THREE.Mesh(new THREE.BoxGeometry(dim.x, dim.y, dim.z, 1, 1, 1), mati)
        break
    case Jolt.EShapeSubType_Sphere:
        let ball= Jolt.castObject(shape, Jolt.SphereShape)
        msh = new THREE.Mesh(new THREE.SphereGeometry(ball.GetRadius(), 16, 16), mati)
        break
    case Jolt.EShapeSubType_Capsule:
        let cap = Jolt.castObject(shape, Jolt.CapsuleShape)
        msh = new THREE.Mesh(new THREE.CapsuleGeometry(cap.GetRadius(), 2 * cap.GetHalfHeightOfCylinder(), 3, 16), mati)
        break
    case Jolt.EShapeSubType_Cylinder:
        let cyl = Jolt.castObject(shape, Jolt.CylinderShape)
        msh = new THREE.Mesh(new THREE.CylinderGeometry(cyl.GetRadius(), cyl.GetRadius(), 2 * cyl.GetHalfHeight(), 16, 1), mati)
        break
    default:
        msh = (body.GetBodyType() == Jolt.EBodyType_SoftBody)
            ? getSoftBodyMesh(body, mati)
            : new THREE.Mesh(getMesh(shape), mati)
        break
    }
//    msh.receiveShadow        = true
//    msh.castShadow           = true
//    msh.material.flatShading = true
    msh.userData.body        = body;     /// * keep GUI->PhyX cross-ref
    syncToBody(msh)                      /// * synchronize THREE mesh to JOLT body

    return msh
}
///
///> arena  - Web canvas container
///> update - GUI update callback
///
export default class {
    constructor(vport, div, px_ratio, callback) {
        let v = this.vu    = document.getElementById(vport)
        let e = this.arena = document.getElementById(div)
        e.innerHTML = ''
        
        if (WebGL.isWebGLAvailable()) {
            let w = v.offsetWidth, h = v.offsetHeight
            this.update =                       /// * array of callback functions
                Array.isArray(callback) ? callback : [ callback ]

            this._initGraphics(w, h, px_ratio)  /// => rndr, cam, ctrl,light, scene, stats
            this._initPhysics()                 /// => jolt, phyx, intf
        }
        else {
            const warning = WebGL.getWebGLErrorMessage()
            e.appendChild(warning)
        }
        // The memory profiler doesn't have an ID so we can't mess with it in css, set an ID here
        let st = document.getElementById("memoryprofiler_canvas")
        if (st) st.parentElement.id = "memoryprofiler"

        ///> setup timer
        this.clock  = new THREE.Clock()
        this.time   = 0
        ///> create mesh object space
        this.ospace = {}
        this.length = 0
        
        new ResizeObserver(e=>this.resize()).observe(e)  /// * watch canvas resizing
    }
    resize() {
        let w = this.vu.offsetWidth, h = this.vu.offsetHeight
        this.cam.aspect = w / h
        this.cam.updateProjectionMatrix()
        this.rndr.setSize(w, h)
    }
    render() {
        this.update.forEach(fn=>fn(this))      /// * callback functions

        let dt = Math.min(this.clock.getDelta(), 1.0 / 30.0) /// * stay above 30Hz
        this.time += dt
        this.jolt.Step(dt, dt > 1.0 / 55.0 ? 2 : 1)          /// * 2 steps if below 55 Hz
        /// update GUI
        for (let id in this.ospace) {
            syncToBody(this.ospace[id])        /// * synchronize THREE to JOLT body
        }
        this.orb.update(dt)
        this.rndr.render(this.scene, this.cam)
        this.stats.update()
        /// repaint screen (with frame-rate control)
        requestAnimationFrame(()=>this.render())             /// * enqueue GUI event loop (default 60Hz)
/*        
        setTimeout(                                          // enqueue GUI event loop
            ()=>requestAnimationFrame(()=>this.render()),
            10                                               // 50=20Hz slow, 10=100Hz fast
        )
*/
    }
    addShape(
        id, shape, pos, rot,       // obj id, shape setting, pos3, rot4
        color,                     // #C0FFC0
        mass=0,                    // to calculate inertial, 0=default density
        move=true                  // moving object
    ) {
        const type  = move ? Jolt.EMotionType_Dynamic : Jolt.EMotionType_Static
        const layer = move ? L_MOVING : L_STATIC
        const body  = this._getBody(shape, pos, rot, type, layer, mass)
        
        return this._addToScene(id, body, color)
    }
    setConstraint(id, config) {
        const body = this.ospace[id].userData.body
		const cnst = new Jolt.VehicleConstraint(body, config)
		const tstr = new Jolt.VehicleCollisionTesterCastCylinder(L_MOVING, 1)
		cnst.SetVehicleCollisionTester(tstr)
        
		this.phyx.AddConstraint(cnst)
        this.phyx.AddStepListener(new Jolt.VehicleConstraintStepListener(cnst))
        
        return cnst
    }
    setVelocity(id, lv, av) {       // linear and angular velocities
        const body = this.ospace[id].userData.body
        const bid  = body.GetID()
        body.SetLinearVelocity(lv)
        body.SetAngularVelocity(av)
        
        this.intf.ActivateBody(bid)
    }
    tick(id, on=true) {                      // reactivate a body
        let bid = this.ospace[id].userData.body.GetID()
        if (on) this.intf.ActivateBody(bid)
        else    this.intf.DeactivateBody(bid)
    }
    remove(id) {
        return this._removeFromScene(id)
    }
    addLine(from, to, color) {
        const mati = new THREE.LineBasicMaterial({ color: color })
        const pts  = []
        pts.push(V3G(from))
        pts.push(V3G(to))

        const geom = new THREE.BufferGeometry().setFromPoints(pts)
        const line = new THREE.Line(geom, mati)

        this.scene.add(line)        // GUI only
        return line
    }
    addMarker(pos, sz, color) {
        const mati = new THREE.LineBasicMaterial({ color: color })
        const pts  = []
        const ctr  = V3G(pos)
        pts.push(ctr.clone().add(new THREE.Vector3(-sz,   0,   0)))
        pts.push(ctr.clone().add(new THREE.Vector3( sz,   0,   0)))
        pts.push(ctr.clone().add(new THREE.Vector3(  0, -sz,   0)))
        pts.push(ctr.clone().add(new THREE.Vector3(  0,  sz,   0)))
        pts.push(ctr.clone().add(new THREE.Vector3(  0,   0, -sz)))
        pts.push(ctr.clone().add(new THREE.Vector3(  0,   0,  sz)))

        const geom = new THREE.BufferGeometry().setFromPoints(pts)
        const mark = new THREE.LineSegments(geom, mati)

        this.scene.add(mark)         // GUI only
        return mark
    }
    _initGraphics(w, h, px_ratio) {
        ///> create global objects
        this.rndr  = new THREE.WebGLRenderer()
        this.cam   = new THREE.PerspectiveCamera(60, w / h, 0.2, 2000)
        this.orb   = new THREE.OrbitControls(this.cam, this.arena)
        this.light = new THREE.SpotLight(0xe0a060, 1)           // sun
//      this.light = new THREE.DirectionalLight(0xe0a060, 1)    // shadow expensive
        this.fused = new THREE.AmbientLight(0x404040)
        this.scene = new THREE.Scene()
        this.stats = new Stats()
        this.mouse = new THREE.Vector2()
        this.arrow = new THREE.ArrowHelper(
            new THREE.Vector3(0,1,0).normalize(), new THREE.Vector3(0,-5,0), 2, 0xff0000)
        this.tex   = checkerTexture()

        this.rndr.setClearColor(0xbfd1e5)
        this.rndr.setPixelRatio(px_ratio)
        this.rndr.setSize(w, h)
        this.cam.position.set(30, 15, 0)
        this.cam.lookAt(new THREE.Vector3(0, 0, 0))
        this.orb.enableDamping = false
        this.orb.enablePan     = true
        this.light.position.set(20, 30, 20)          // afternoon
//        this.light.castShadow            = true
//        this.light.shadow.bias           = -0.003
//        this.light.shadow.mapSize.width  = 2048
//        this.light.shadow.mapSize.height = 2048

        this.scene.add(this.light)
        this.scene.add(this.fused)
        this.scene.add(this.arrow)
        this.stats.domElement.style.position = 'absolute'
        this.stats.domElement.style.top      = '0px'

        this.arena.appendChild(this.rndr.domElement)     // onto Web browser canvas
        this.arena.appendChild(this.stats.domElement)
        
        this.rndr.domElement.addEventListener(           /// * mouse click handler
            'pointerdown', e=>this._click(e), false)
    }
    _initPhysics() {
        this.jolt   = setupJolt()                        // setup collision interface
        this.phyx   = this.jolt.GetPhysicsSystem()       // physics system instance
        this.intf   = this.phyx.GetBodyInterface()       // binding interface
        this.caster = setupCaster(this)
    }
    _getBody(shape, pos, rot, type, layer, mass) {       // create physical body
        let config = new Jolt.BodyCreationSettings(
            shape, pos, rot, type, layer
        )
        config.mRestitution = 0.5                        // bounciness
        if (mass > 0.0) {                                // override default
		    config.mOverrideMassProperties       = Jolt.EOverrideMassProperties_CalculateInertia
		    config.mMassPropertiesOverride.mMass = mass
        }
        if (layer==L_STATIC) config.mFriction    = 1.0
        
        let body = this.intf.CreateBody(config)
        Jolt.destroy(config)
        
        return body
    }
    _addToScene(id, body, color) {
        let bid = body.GetID()                           // JOLT assigned id
        let msh = meshFactory(body, color, this.tex)
        
        this.intf.AddBody(bid, Jolt.EActivation_Activate)// add to physic system
        this.scene.add(msh)                              // add to GUI

        this.ospace[id] = msh                            // keep mesh in KV store for reference
        return this.ospace.length                        // CC: need a lock?
    }
    _removeFromScene(id) {
        const msh = this.ospace[id]                      // get GUI mesh
        if (!msh) return 0

        let body  = msh.userData.body
        let bid   = body.GetID()                         // fetch JOLT BodyID
        if (body.IsStatic()) {                           // reactivate bodies if needed
            for (let id in this.ospace) this.tick(id)
        }
        this.intf.RemoveBody(bid)
        this.intf.DestroyBody(bid)

        body = null                                      // free ref
        delete msh.userData.body                         // drop Jolt body

        this.scene.remove(msh)                           // remove from GUI
        delete this.ospace[id]                           // remove from our KV storea

        return this.ospace.length                        // CC: need a lock?
    }
    _click(e) {
        this.mouse.set(
            (e.clientX / this.rndr.domElement.clientWidth)  * 2 - 1,
            -(e.clientY / this.rndr.domElement.clientHeight) * 2 + 1)

        const orig = new Jolt.Vec3(0, -4, 0)
        const dir  = new Jolt.Vec3(1, 0, 0)
        
        this.arrow.position.copy(V3G(orig))
        this.arrow.setDirection(V3G(dir))
        
        this.caster.ray.mOrigin.Set(orig)
        this.caster.ray.mDirection.Set(dir)
        this.caster.Cast()
    }
}  // class JoltCore

