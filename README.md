# weForth - Web eForth with WASM

WebAssembly enpowered eForth on web browsers. Is it faster? Is it more portable? Yes, and Yes.

Well, on my aged laptop, the impression is pretty exciting! It's at least 5x faster than pure Javascript implementation on a browser and at 40% speed of C/C++ natively compiled code on CPU. It was at 25% of native a year ago but as Javascript JIT improves, it now runs faster as well. Not bad at all! On the portability end, though not exactly plug-and-play but some simple alteration turned my C code web-enabled. Of course, WASM has yet to integrate with the front-end well enough, so updating DOM is a different feat. If we want to venture beyond being a terminal app some UI glue is still required.

Regardless, it brought me warm smiles seeing eForth run in a browser. Better yet, it's straight from C/C++ source code. Other popular scripting languages such as Python, Ruby are trending toward WASM/WASI implementation as well. However, unlike FORTH, they depend mostly on JIT without a built-in compiler, the interpreter-in-an-interpreter design will likely cap the top-end performance (i.e. stuck at 5~10% of native speed, so far).

With WASM, the interoperability between different languages become a thing of the near future. If words can be compiled directly into WASM opcodes, OS and peripherals can be accessed through WASI, adding a graphic front-end (i.g. SDL or WebGL), weForth can become a worthy scripting alternative for Web.

### Features
   * supports 32-bit float
   * utilizes Web Worker threads (multi-VMs possible)
   * have access to ss, dict, and VM memory (via WebAssembly.Memory) from Javascript
   * call interface from FORTH into Javascript functions
   * IDE-style interactive front-end (cloud possible, i.g. JupyterLab)

> <img src="https://chochain.github.io/weForth/img/weforth_logo_snip2.png" style="width:800px">sample</img>

### Build - (make sure python3 and Emscripten are installed)
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

#### ceforth.html - a very simple (and dangerous) handler with the single-threaded demo

    this.onmessage = e=>{
        if (e.data[0]=='js') this.eval(e.data[1])
    }

    > 54321 s" alert('%d ... hello world!')" JS⏎
    
> <img src="https://chochain.github.io/weForth/img/weforth_js.png" width=604px>JS call</img>

#### weforth.html - a more complex handler with the multi-threaded web worker demo

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
    
> <img src="https://chochain.github.io/weForth/img/weforth_logo.png" width=604px>Logo demo</img>

### DEBUG the WASM file (dump all functions, check with wasm-objdump in WABT kit)

    make debug
    read tests/ceforth.wasm.txt (really long)

### Benchmark (on my aged IBM X230 w Intel i5-3470@3.2GHz)
Simple 10M tests
  
    : xx 9999 FOR 34 DROP NEXT ;⏎
    : yy 999 FOR xx NEXT ;⏎
    : zz MS NEGATE yy MS + ;⏎
    zz⏎

|implementation|version|source code|optimization|platform|run time(ms)|code size(KB)|
|--|--|--|--|--|--|--|
|eforth   |4.2  |g++  / C  |-O0|CPU  |1830|186|
|         |     |          |-O2|CPU  |85  |106|
|         |     |          |-O3|CPU  |87  |114|
|eforth   |4.2  |EM.b / C  |-O0|FF.b |4250|273|
|         |     |          |-O2|FF.b |215 |160|
|         |     |          |-O3|FF.b |222 |176|
|eForth.js|pre6 |JavaScript|   |FF.a |756 |20 |
|         |pre6 |          |   |FF.b |1059|20 |
|         |4.2  |          |   |FF.b |1459|20 |
|uEforth  |7.0.2|Asm.js    |   |FF.a |814 |29 |
|         |7.0.7|          |   |FF.b |387 |29 |
|weForth  |1.2  |EM.a / C  |-O0|FF.a1|943 |254|
|         |     |          |-O2|FF.a1|410 |165|
|weForth  |1.4  |EM.b / C  |-O0|FF.b |451 |267|
|         |     |          |-O2|FF.b |181 |170|
|weForth  |4.1  |EM.b / C  |-O0|FF.b |348 |300|
|         |     |          |-O2|FF.b |154 |164|
|         |     |          |-O2|FF.b1|160 |164|
|weForth<br/>float32|4.2  |EM.b / C  |-O2|FF.b |160 |153|

    FF.a = FireFox v120, FF.a1 = FF.a + 1 worker
    FF.b = FireFox v122, FF.b1 = FF.b + 1 worker
    EM.a = Emscripten v3.1.51
    EM.b = Emscripten v3.1.53

Note:
* eForth.js uses JS straight, can do floating-points
* uEforth v7 uses Asm.js, build Forth up with JS "assembly".
* weForth v1 uses token indirected threaded
* weForth+switch(op), is 2x slower than just function pointers.
* weForth v1.2 without yield in nest(), speeds up 3x.
* WASM -O3 => err functions (wa.*) not found
* FireFox v122, is 2x faster than v120
* Chrome is about 10% slower than FireFox
* weForth 4.2 w float32-enabled, runs equally fast as int32, but why?

### TODO
* Physics Engine
  + review Three.js + (Ammo.js/Bullet, or JOLT)
    + CSG Object (with optional motion trail) [OpenCSG](https://opencsg.org/), [GTS](https://gts.sourceforge.net/)
    + Collision (with directional distance sensing)
  + review raylib
* review wasmtime (CLI), perf+hotspot (profiling)
* review DragonRuby/mRuby (SDL)
* review R3, Forth CPU visualizer (SDL)
* GraFORTH spec.
  + File system (FS/IndexedDB)
  + Editor
  + 2D graphic (SDL_gfx, SDL_image)
  + Character graphic (SDL_ttf or HTML5)
  + 3D graphic (GL)
  + Music (SDL_media)
* add network system (SD_net)
* inter-VM communication
* use WASM stack as ss
* macro-assembler

### References
* SDL2
  + Read first https://lyceum-allotments.github.io/2016/06/emscripten-and-sdl-2-tutorial-part-1/
  + LazyFoo for SDL2 https://lazyfoo.net/tutorials/SDL/
  + raylib vs SDL2 https://gist.github.com/raysan5/17392498d40e2cb281f5d09c0a4bf798
