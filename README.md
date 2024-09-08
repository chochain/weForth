# weForth - Web eForth with WASM

WebAssembly enpowered eForth on web browsers. Is it faster? Is it more portable? Yes, and Yes.

<img src="https://chochain.github.io/weForth/img/weforth_jolt2.png" style="width:800px"></img>

Well, on my aged laptop, the impression is pretty exciting! It's at least 5x faster than pure Javascript implementation on a browser and at 60% speed of C/C++ natively compiled code on CPU. It was at 25% of native a year ago but as Javascript JIT improves, it now runs faster as well. Not bad at all! On the portability end, though not exactly plug-and-play but some simple alteration turned my C code web-enabled. Of course, WASM has yet to integrate with the front-end well enough, so updating DOM is a different feat. If we want to venture beyond being a terminal app some UI glue is still required.

Regardless, it brought me warm smiles seeing eForth run in a browser. Better yet, it's straight from C/C++ source code. Other popular scripting languages such as Python, Ruby are trending toward WASM/WASI implementation as well. However, unlike FORTH, they depend mostly on JIT without a built-in compiler, the interpreter-in-an-interpreter design will likely cap the top-end performance (i.e. stuck at 10~15% of native speed, so far).

With WASM, the interoperability between different languages become a thing of the near future. If words can be compiled directly into WASM opcodes, OS and peripherals can be accessed through WASI, adding an interactive graphical front-end, weForth can become a worthy scripting alternative for Web.

### Features
   * supports 32-bit float
   * call interface from FORTH into Javascript functions
   * integrated with Web graphics (SDL2, WebGL) and Physics Engine (Jolt)
   * built-in editor, IDE-style interactive front-end
   * access to ss, dict, and VM memory (via WebAssembly.Memory) from Javascript
   * browser can fetch from self-host web server (i.g. python3 -m http.server)
   * can run parallel in Web Worker threads

<img src="https://chochain.github.io/weForth/img/weforth_logo_snip2.png" style="width:800px"></img>

### Build - (make sure python3 and Emscripten are installed)
#### Templates
Serving as the core of demos, the templates under ~/template directory are used in different built shown below. They are organized by the Makefile
   * eforth.html  - vanilla weForth, one single-threaded HTML. A good place to start.
   * ceforth.html - weForth, now with integrated editor, single-threaded.
   * weforth.html - weForth runs in a worker thread, can also do fancy GUI stuffs now
       Javascript module/files are included in the HTML for added functionality
       <pre>
       + weforth_helper.js - vocabulary lookup table
       + weforth_worker.js - worker thread proxy object
       + weforth_sleep.js  - sleep/delay support for async environment
       + weforth_logo.js   - Turtle Graphic implementation
       + weforth_jolt.js   - Jolt Physics Engine integration
       + file_io.js        - file IO support
       </pre>

       The following Forth scripts under ~/tests/forth are also included for GUI integration demo
       <pre>
       + forth/logo.fs - Turtle Graphics
       + forth/jolt.fs - Jolt Physics Engine
       </pre>

#### Bare-bone eForth on Web

    make zero
    Note: -O2 works OK, -O3 emscripten spits wrong code

try eforth.html [here](https://chochain.github.io/weForth/ref/eforth.html)

#### Single WASM file

    make one
    
try ceforth.html [here](https://chochain.github.io/weForth/ref/ceforth.html)

#### Extra Web Worker thread

    make two
    
try weforth.html [here](https://chochain.github.io/weForth/ref/weforth.html)

### Run on your own box
Server-side

    python3 -m http.server
    
Client-side Browser

    http://localhost:8000/tests/eforth.html, ceforth.html or weforth.html

### Javascript interface
To communicate between Forth and Javascript engine, weForth implemented a word 'JS' and a function call_js(). They utilize Emscripten EM_JS macro that *postMessage* from C++ backend-side to *onmessage* on browser-side. Depends on your need, handler can be very simple to complicated.

#### in eforth.html and ceforth.html - a simple (and dangerous) handler with the single-threaded demo

    this.onmessage = e=>{
        if (e.data[0]=='js') this.eval(e.data[1])
    }

    > 54321 s" alert('%d ... hello world!')" JS⏎
    
<img src="https://chochain.github.io/weForth/img/weforth_js.png" width=604px></img>

#### weforth.html - a more complex message dispatcher with the multi-threaded web worker demo

    const ex = (ops)=>Function(       ///< scope limiting eval
        `"use strict"; this.update('${ops}')`
    ).bind(logo)()
    vm.onmessage = e=>{               /// * wait for worker's response
        let k = e.data[0], v = e.data[1]
        switch (k) {
        case 'cmd': to_txt(v);         break
        case 'dc' : show_dict(v);      break
        case 'us' : usr.innerHTML = v; break
        case 'ss' : ss.innerHTML  = v; break
        case 'mm' : mm.innerHTML  = v; ok=1; break
        case 'js' : ex(v);             break
        default: console.log('onmessage.error=>'+e)
        }
    }

    > s" forth/logo.fs" included⏎
    > : seg FD 30 RT ;⏎
    > : color 2* PC ;⏎
    > : daz 100 0 do i color i seg loop ;⏎
    > daz⏎
    
<img src="https://chochain.github.io/weForth/img/weforth_logo.png" width=604px></img>

### DEBUG the WASM file (dump all functions, check with wasm-objdump in WABT kit)

    make debug
    read tests/ceforth.wasm.txt (really long)

### Benchmark (on my aged IBM X230 w Intel i5-3470@3.2GHz)
Simple 10M tests
  
    : xx 9999 FOR 34 DROP NEXT ;⏎
    : yy 999 FOR xx NEXT ;⏎
    : zz MS NEGATE yy MS + ;⏎
    zz⏎

<img src="https://chochain.github.io/weForth/img/weforth_perf.png" width=800px></img>

Note:
* eForth.js uses JS straight, can do floating-points
* uEforth v7 uses Asm.js, build Forth up with JS "assembly".
* weForth v1 uses token indirected threaded
* weForth+switch(op), is 2x slower than just function pointers.
* weForth v1.2 without yield in nest(), speeds up 3x.
* WASM -O3 => err functions (wa.*) not found
* FireFox v122, is 2x faster than v120
* Chrome is about 10% slower than FireFox
* weForth 4.2 w float32-enabled, runs faster than int32, but why?
* weForth 4.2 w float32/real-time is slower due to message passing
* weForth 4.2 w float32/real-time is even slower without WebGL, why?

### TODO
* Physics Engine
  + vehicle sim
  + CSG i.e. compound shape, [OpenCSG](https://opencsg.org/), [GTS](https://gts.sourceforge.net/)
  + review Jolt::RefTarget class (for ref counter, can apply to tensorForth)
* inter-VM communication
* add network system (SD_net)
* review WebSerial
* review wasmtime (CLI), perf+hotspot (profiling)
- review DragonRuby/mRuby (SDL)          => 3D preferred
- review R3, Forth CPU visualizer (SDL)  => 3D preferred
* review GraFORTH spec.
  + File system (FS/IndexedDB)
  - Editor                               => CodeMirrow chosen
  + 2D graphic (SDL_gfx, SDL_image)
  + Character graphic (SDL_ttf or HTML5)
  - 3D graphic (GL)                      => Three.js WebGL
  + Audio (SDL_media)
* use WASM stack as ss (brk, sbrk)
* macro-assembler

### References
* SDL2
  + [Read first](https://lyceum-allotments.github.io/2016/06/emscripten-and-sdl-2-tutorial-part-1/)
  + [LazyFoo for SDL2](https://lazyfoo.net/tutorials/SDL/)
  + [raylib vs SDL2](https://gist.github.com/raysan5/17392498d40e2cb281f5d09c0a4bf798)
* WebGL
  + [Three.js](https://threejs.org/manual/#en/fundamentals)
* Physics Engine
  + [Jolt Physics](https://jrouwe.github.io/JoltPhysics/)

