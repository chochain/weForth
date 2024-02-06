# weForth - Web eForth with WASM

WebAssembly enpowered eForth on web browsers. Is it faster? Is it more portable? Yes, and Yes.

Well, on my aged laptop, the impression is pretty exciting! It's about 2x faster than pure Javascript implementation on a browser and at 1/4 speed of C/C++ natively compiled code on CPU. On the portability end, though not exactly plug-and-play but some simple alteration turned my C code web-eabled. Of course, WASM has not integrated with the front-end well enough, updating DOM is a different feat.

Regardless, it brought me warm smiles seeing eForth run in a browser. Better yet, it's straight from C/C++ source code. Other popular scripting languages such as Python, Ruby are trending toward WASM/WASI implementation as well. However, depending solely on JIT without built-in compiler as Forth does, the interpreter-in-an-interpreter design will likely cap the top-end performance (i.e. stuck at 5%~10% of native, so far).

With WASM, the interoperability between different languages become a thing of the near future. If Forth can compile word directly into WASM opcodes, engage WASI to access OS and peripherals, hookup the graphic front-end (i.g. SDL or WebGL), weForth can become a worthy scripting alternative for Web.

### Features
* Javascript access to ss, dict, and VM memory (via WebAssembly.Memory)
* Forth in Web Worker threads (multi-VMs possible)
* IDE-style interactive front-end (cloud possible, i.g. JupyterLab)

### To Compile with single file (make sure python3 and Emscripten are installed)
  > make one
  > Note: -O2 works OK, -O3 emscripten spits wrong code

### To Compile with one Web Worker thread (multi-threaded)
  > make two

### To Compile with one Web Worker thread (multi-threaded)
* Server-side
  > python3 -m http.server
* Client-side Browser
  > http://localhost:8000/tests/ceforth.html or weforth.html

### To Debug the WASM file (dump all functions, check with wasm-objdump in WABT kit)
  > make debug
  > read tests/ceforth.wasm.txt (really long)

### Benchmark (on my aged IBM X230)
> Simple 1K*10K tests
>> : xx 9999 FOR 34 DROP NEXT ;<br/>
>> : yy 999 FOR xx NEXT ;<br/>
>> : zz MS NEGATE yy MS + ;<br/>
>> zz

* CPU = Intel i5-3470 @ 3.2GHz
* FF.a = FireFox v120, FF.a1 = FF.a + 1 worker
* FF.b = FireFox v122, FF.b1 = FF.b + 1 worker
* EM.a = Emscripten v3.1.51
* EM.b = Emscripten v3.1.53

|implementation|version|source code|optimization|platform|run time(ms)|code size(KB)|
|--|--|--|--|--|--|--|
|ceforth  |8.0  |C         |-O0|CPU  |266 |111|
|         |     |          |-O2|CPU  |106 |83 |
|eForth.js|6.0  |JavaScript|   |FF.a |756 |20 |
|         |     |          |   |FF.b |1059|20 |
|uEforth  |7.0.2|Asm.js    |   |FF.a |814 |29 |
|         |7.0.7|          |   |FF.b |302 |29 |
|weForth  |1.2  |EM.a / C  |-O0|FF.a1|943 |254|
|         |     |          |-O2|FF.a1|410 |165|
|weForth  |1.4  |EM.b / C  |-O0|FF.b |515 |259|
|         |     |          |-O2|FF.b |161 |168|
|weForth  |1.4  |EM.b / C  |-O0|FF.b1|516 |259|
|         |     |          |-O2|FF.b1|163 |168|

* Note1: eForth.js uses JS straight, can do floating-points
* Note2: uEforth v7 uses Asm.js, build Forth up with JS "assembly".
* Note3: weForth v1 uses token indirected threaded
* Note4: weForth+switch(op), is 2x slower than just function pointers.
* Note5: weForth v1.2 without yield in nest() speeds up 3x.
* Note6: WASM -O3 => err functions (wa.*) not found
* Note7: FireFox v122 is vastly faster than v120
* Note8: Chrome is about 10% slower than FireFox

### SDL2
* learn SDL2
> [Read first](https://lyceum-allotments.github.io/2016/06/emscripten-and-sdl-2-tutorial-part-1/)
> [LazyFoo for SDL2](https://lazyfoo.net/tutorials/SDL/)
* install sdl2, image, sound, and fonts
> sudo apt install libsdl2-dev libsdl2-2.0-0 -y;
> sudo apt install libjpeg-dev libwebp-dev libtiff5-dev libsdl2-image-dev libsdl2-image-2.0-0 -y;
> sudo apt install libmikmod-dev libfishsound1-dev libsmpeg-dev liboggz2-dev libflac-dev libfluidsynth-dev libsdl2-mixer-dev libsdl2-mixer-2.0-0 -y;
> sudo apt install libfreetype6-dev libsdl2-ttf-dev libsdl2-ttf-2.0-0 -y;
* compile to host exe
> make exe
> ./tests/sdl2
* compile to WASM
> make sdl
> http://localhost:8000/tests/sdl2.html

### TODO
* review wasmtime (CLI), perf+hotspot (profiling)
* Forth CPU visualizer (with SDL)
* GraFORTH spec.
  + File system (FS/IndexedDB)
  + Editor
  + 2D graphic (SDL_gfx, SDL_image)
  + Character graphic (SDL_ttf or HTML5)
  + 3D graphic (GL)
  + Music (SDL_media)
* Robotic Simulation Engine
  + CSG Object (with optional motion trail) [OpenCSG](https://opencsg.org/), [GTS](https://gts.sourceforge.net/)
  + Collision (with directional distance sensing)
* add network system (SD_net)
* inter-VM communication
* use WASM stack as ss
* macro-assembler
