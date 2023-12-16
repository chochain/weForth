# weForth - Web eForth with WASM

WebAssembly enpowered eForth on web browsers. Is it faster? Is it more portable?

Well, on my aged laptop, the impression is pretty exciting! It's about 2x faster than pure Javascript implementation on a browser and at 1/4 speed of C/C++ natively compiled code on CPU. On the portability end, though not exactly plug-and-play but some simple alteration turned my C code web-eabled. Of course, WASM has not integrated with the front-end well enough, updating DOM is a different feat.

Regardless, it brought me warm smiles seeing eForth run in a browser. Better yet, it's straight from C/C++ source code. Other popular scripting languages such as Python, Ruby are trending toward WASM/WASI implementation as well. However, depending solely on JIT without built-in compiler as Forth does, the interpreter-in-an-interpreter design will likely cap the top-end performance (i.e. stuck at 5%~10% of native, so far).

With WASM, the interoperability between different languages become a thing of the near future. If Forth can compile word directly into WASM opcodes, engage WASI to access OS and peripherals, hookup the graphic front-end (i.g. SDL or WebGL), weForth can become a worthy scripting alternative for Web.

### Features
* Javascript access to ss, dict, and VM memory (via WebAssembly.Memory)
* Forth in Web Worker threads (multi-VMs possible)
* IDE-style interactive front-end (cloud possible, i.g. JupyterLab)

### To Compile and Run (make sure python3 and Emscripten is installed)
* em++ -o tests/ceforth.html src/ceforth.cpp --shell-file template/ceforth.html -sEXPORTED_FUNCTIONS=_main,_forth,_vm_base,_vm_ss,_vm_ss_idx,_vm_dict_idx,_vm_dict,_vm_mem,_top -sEXPORTED_RUNTIME_METHODS=ccall,cwrap
  > Note: -O2 works OK, -O3 emscripten spits wrong code
* Server-side
  > python3 tests/serv.py
* Client-side Browser
  > http://localhost:8000/tests/ceforth.html

* em++ -o tests/ceForth_403.html src/ceForth_403.cpp --shell-file template/ceforth.html -sEXPORTED_FUNCTIONS=_main,_forth -sEXPORTED_RUNTIME_METHODS=ccall,cwrap
* Server-side
  > python3 tests/serv.py
* Client-side Browser
  > http://localhost:8000/tests/ceForth_403.html

### To Compile to Web Worker (run almost at the same speed as main thread)
* cp template/weforth.html template/weforth.css template/file_io.js template/weforth_helper.js template/weforth_worker.js tests
* em++ -o tests/weforth.js src/ceforth.cpp -sEXPORTED_FUNCTIONS=_main,_forth,_vm_base,_vm_ss,_vm_ss_idx,_vm_dict_idx,_vm_dict,_vm_mem,_top -sEXPORTED_RUNTIME_METHODS=ccall,cwrap -O2
* Server-side
  > python3 tests/serv.py
* Client-side Browser
  > http://localhost:8000/tests/weforth.html

### To Debug (dump all functions)
* em++ -o tests/ceforth.html src/ceforth.cpp --shell-file src/forth_template.html -sEXPORT_ALL=1 -sLINKABLE=1 -sEXPORTED_RUNTIME_METHODS=ccall,cwrap

### Benchmark (on my aged IBM X230)
|implementation|source code|optimization|Platform|1K*10K cycles (in ms)|code size (KB)|
|---|---|---|---|---|---|
|ceforth v8|C|-O0|CPU|266|111|
|ceforth v8|C|-O1|CPU|133|86|
|ceforth v8|C|-O2|CPU|106|83|
|ceforth v8|C|-O3|CPU|108|91|
|eForth.js v6|JavaScript||FireFox v120|756|20|
|uEforth v7.0|asm.js / C|?|FireFox v120|814|?|
|||||||
|weForth v1|WASM / C|-O0|FireFox v120 1-Worker|7496|237|
|weForth v1|WASM / C|-O2|FireFox v120 1-Worker|1922|157|
|weForth v1|WASM / C|-O3|FireFox v120 1-Worker|1847|174|
|||||||
|weForth v1.2|WASM / C|-O0|FireFox v120 1-Worker|943|254|
|weForth v1.2|WASM / C|-O1|FireFox v120 1-Worker|450|196|
|weForth v1.2|WASM / C|-O2|FireFox v120 1-Worker|410|165|
|weForth v1.2|WASM / C|-O3|FireFox v120 1-Worker|err - fn NA|182|

* Note1: uEforth v7 uses switch(op), instead of 'computed goto' (asm.js/WASM has no goto)
* Note2: weForth v1 uses token indirected threaded
* Note3: weForth+switch(op), is 2x slower than just function pointers. Why?
* Note4: weForth v1.2 without yield in nest() speeds up 3x. Why?
       
### TODO
* review wasmtime (CLI), perf+hotspot (profiling)
* Sync UI with eForth.js
* GraFORTH spec.
  * File system (FS/IndexedDB)
  * Editor
  * 2D graphic (SDL)
  * Character graphic (HTML5)
  * 3D graphic (WebGL)
  * Music (SDL)
* add network system (wget/WebSocket)
* inter-VM communication
* use WASM stack as ss
* macro-assembler
