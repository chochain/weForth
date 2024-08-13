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

    let jolt = new Jolt.JoltInterface(config)            // create physics system
    Jolt.destroy(config)

    return jolt
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
///
///> THREE object factory
///
function objFactory(body, color) {
    let mati  = new THREE.MeshPhongMaterial({ color: color });
    let shape = body.GetShape();
    let obj;

    switch (shape.GetSubType()) {
    case Jolt.EShapeSubType_Box:
        let box = Jolt.castObject(shape, Jolt.BoxShape)
        let dim = V3G(box.GetHalfExtent()).multiplyScalar(2)
        obj = new THREE.Mesh(new THREE.BoxGeometry(dim.x, dim.y, dim.z, 1, 1, 1), mati)
        break
    case Jolt.EShapeSubType_Sphere:
        let ball= Jolt.castObject(shape, Jolt.SphereShape);
        obj = new THREE.Mesh(new THREE.SphereGeometry(ball.GetRadius(), 32, 32), mati)
        break
    case Jolt.EShapeSubType_Capsule:
        let cap = Jolt.castObject(shape, Jolt.CapsuleShape)
        obj = new THREE.Mesh(new THREE.CapsuleGeometry(cap.GetRadius(), 2 * cap.GetHalfHeightOfCylinder(), 20, 10), mati)
        break
    case Jolt.EShapeSubType_Cylinder:
        let cyl = Jolt.castObject(shape, Jolt.CylinderShape)
        obj = new THREE.Mesh(new THREE.CylinderGeometry(cyl.GetRadius(), cyl.GetRadius(), 2 * cyl.GetHalfHeight(), 20, 1), mati)
        break
    default:
        obj = (body.GetBodyType() == Jolt.EBodyType_SoftBody)
            ? getSoftBodyMesh(body, mati)
            : new THREE.Mesh(getMesh(shape), mati)
        break
    }
    obj.position.copy(V3G(body.GetPosition()))
    obj.quaternion.copy(Q4G(body.GetRotation()))
    obj.userData.body = body;                     // keep GUI->PhyX ref

    return obj
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
            this.update = callback

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
        ///> create object space
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
        if (this.update != null) this.update(this)             // callback to front-end
        /// update physics system
        for (let k in this.ospace) {
            let obj  = this.ospace[k]
            let body = obj.userData.body

            obj.position.copy(V3G(body.GetPosition()))
            obj.quaternion.copy(Q4G(body.GetRotation()))

            if (body.GetBodyType() == Jolt.EBodyType_SoftBody) {
                if (obj.userData.updateVertex) {
                    obj.userData.updateVertex()
                }
                else obj.geometry = getMesh(body.GetShape())
            }
        }
        /// update GUI
        let dt = Math.min(this.clock.getDelta(), 1.0 / 30.0) // stay above 30Hz
        this.time += dt
        this.jolt.Step(dt, dt > 1.0 / 55.0 ? 2 : 1)          // 2 steps if below 55 Hz
        this.ctrl.update(dt)
        this.rndr.render(this.scene, this.cam)
        this.stats.update()
        /// repaint screen, (fast/slow motion by adjusting framerate)
        requestAnimationFrame(()=>this.render())             // enqueue GUI event loop (default 60Hz)
/*        
        setTimeout(                                          // enqueue GUI event loop
            ()=>requestAnimationFrame(()=>this.render()),
            10                                               // 50=20Hz slow, 10=100Hz fast
        )
*/
    }
    add(shape, pos, rot, color, fixed=false) {
        let config = new Jolt.BodyCreationSettings(
            shape, pos, rot,
            fixed ? Jolt.EMotionType_Static : Jolt.EMotionType_Dynamic,
            fixed ? L_STATIC : L_MOVING)
        config.mRestitution = 0.5                            // bounciness
        let body   = this.intf.CreateBody(config)
        Jolt.destroy(config)
        
        return this._addToScene(body, color)
    }
    addBox(pos, rot, halfExt, mtype, layer, color = 0xffffff) {
        let shape = new Jolt.BoxShape(halfExt, 0.05, null)
        let config= new Jolt.BodyCreationSettings(shape, pos, rot, mtype, layer)

        let body  = this.intf.CreateBody(config)
        Jolt.destroy(config)

        return this._addToScene(body, color)
    }
    addSphere(pos, r, mtype, layer, color = 0xffffff) {
        let shape = new Jolt.SphereShape(r, null);
        let config= new Jolt.BodyCreationSettings(
            shape,
            pos,
            Jolt.Quat.prototype.sIdentity(),
            mtype, layer)

        let body = this.intf.CreateBody(config)
        Jolt.destroy(config)

        return this._addToScene(body, color)
    }
    addFloor(sz = 50, color=0xffffff) {
        var config = new Jolt.BodyCreationSettings(
            new Jolt.BoxShape(new Jolt.Vec3(sz, 0.5, sz), 0.05, null),
            new Jolt.RVec3(0, -0.5, 0),
            new Jolt.Quat(0, 0, 0, 1),
            Jolt.EMotionType_Static, L_STATIC)

        let body = this.intf.CreateBody(config)
        Jolt.destroy(config)

        return this._addToScene(body, color)
    }
    addLine(from, to, color) {
        const mati = new THREE.LineBasicMaterial({ color: color })
        const pts  = []
        pts.push(V3G(from))
        pts.push(V3G(to))

        const geom = new THREE.BufferGeometry().setFromPoints(pts)
        const line = new THREE.Line(geom, mati)

        this.scene.add(line)        // GUI only
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
        this.ctrl  = new THREE.OrbitControls(this.cam, this.arena)
        this.light = new THREE.DirectionalLight(0xffffff, 1)             // white light
        this.scene = new THREE.Scene()
        this.stats = new Stats();

        this.rndr.setClearColor(0xbfd1e5)
        this.rndr.setPixelRatio(px_ratio)
        this.rndr.setSize(w, h)
        this.cam.position.set(0, 15, 30)
        this.cam.lookAt(new THREE.Vector3(0, 0, 0))
        this.light.position.set(10, 10, 5)

        this.scene.add(this.light)
        this.stats.domElement.style.position = 'absolute'
        this.stats.domElement.style.top      = '0px'

        this.arena.appendChild(this.rndr.domElement)     // onto Web browser canvas
        this.arena.appendChild(this.stats.domElement)
    }
    _initPhysics() {
        this.jolt = setupJolt()                          // setup collision interface
        this.phyx = this.jolt.GetPhysicsSystem()         // physics system instance
        this.intf = this.phyx.GetBodyInterface()         // binding interface
    }
    _addToScene(body, color) {
        let id  = body.GetID()                           // JOLT assigned id
        let obj = objFactory(body, color)

        this.intf.AddBody(id, Jolt.EActivation_Activate) // add to physic system
        this.scene.add(obj)                              // add to GUI
        this.ospace[id.GetIndex()] = obj                 // keep obj in KV store for reference
        this.length += 1                                 // CC: need a lock?

        return body
    }
    _removeFromScene(obj) {
        let id = obj.userData.body.GetID()               // fetch JOLT id
        this.intf.RemoveBody(id)
        this.intf.DestroyBody(id)
        delete obj.userData.body                         // drop Jolt object first

        this.scene.remove(obj)                           // remove from GUI

        delete this.ospace[id.GetIndex()]                // remove from our KV store
        this.length -= 1                                 // CC: need a lock?
    }
}  // class JoltCore

