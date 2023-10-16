# weForth - Web eForth with WASM

WebAssembly enpowered eForth on web browsers. Is it faster? Is it more portable?

Well, on my aged laptop, the impression is pretty exciting! It's about 3x faster than pure Javascript implementation on a browser and at 1/5 speed of C/C++ natively compiled code on CPU. On the portability end, though not exactly plug-and-play but some simple alteration turned my C code web-eabled. Of course, WASM has not integrated with the front-end well enough, updating DOM is a different feat.

Regardless, it brought me warm smiles seeing eForth run in a browser. Better yet, it's straight from C/C++ source code. Other popular scripting languages such as Python, Ruby are trending toward WASM/WASI implementation as well. However, depending solely on JIT without built-in compiler as Forth does, the interpreter-in-an-interpreter design will likely cap the top-end performance (i.e. stuck at 5%~10% of native, so far).

With WASM, the interoperability between different languages become a thing of the near future. If Forth can compile word directly into WASM opcodes, engage WASI to access OS and peripherals, hookup the graphic front-end (i.g. SDL or WebGL), weForth can become a worthy scripting alternative for Web.

### Features
* Javascript access to ss, dict, and VM memory (via WebAssembly.Memory)
* Forth in Web Worker threads (multi-VMs possible)
* IDE-style interactive front-end (cloud possible, i.g. JupyterLab)

### To Compile (make sure emscripten is installed)
* em++ -o tests/ceforth.html src/ceforth.cpp --shell-file src/forth_template.html -sEXPORTED_FUNCTIONS=_main,_forth,_vm_base,_vm_ss,_vm_ss_idx,_vm_dict_idx,_vm_dict,_vm_mem,_top -sEXPORTED_RUNTIME_METHODS=ccall,cwrap
  > Note: -O2 works OK, -O3 emscripten spits wrong code
  
* em++ -o tests/ceForth_403.html src/ceForth_403.cpp --shell-file src/forth_template.html -sEXPORTED_FUNCTIONS=_main,_forth -sEXPORTED_RUNTIME_METHODS=ccall,cwrap

### To Compile to Web Worker (run almost at the same speed as main thread)
* cp src/weforth_static.html tests/weforth.html; cp src/weforth.css src/file_io.js src/weforth_helper.js src/weforth_worker.js tests
* em++ -o tests/weforth.js src/ceforth.cpp -sEXPORTED_FUNCTIONS=_main,_forth,_vm_base,_vm_ss,_vm_ss_idx,_vm_dict_idx,_vm_dict,_vm_mem,_top -sEXPORTED_RUNTIME_METHODS=ccall,cwrap

### To Debug (dump all functions)
* em++ -o tests/ceforth.html src/ceforth.cpp --shell-file src/forth_template.html -sEXPORT_ALL=1 -sLINKABLE=1 -sEXPORTED_RUNTIME_METHODS=ccall,cwrap

### To Run
* Server-side
  > python3 tests/serv.py
* Client-side Browser
  > http://localhost:8000/tests/weforth.html

### Benchmark (on IBM X230)
|implementation|source code|optimization|Platform|1K*10K cycles (in ms)|code size (KB)|
|---|---|---|---|---|---|
|ceforth v8|C|--|CPU|214|91|
|ceforth v8|C|-O2|CPU|104|70|
|ceforth v8|C|-O3|CPU|105|74|
|eforth.js v6|JavaScript||FireFox v107|1550|20|
|uEforth v7|asm.js / C|--|FireFox v107|959|?|
|||||||
|weForth v1|WASM / C|--|FireFox v107|7433|237|
|weForth v1|WASM / C|-O2|FireFox v107|1901|157|
|weForth v1|WASM / C|-O3|FireFox v107|failed(unknown function)|174|
|||||||
|weForth v1|WASM / C|--|FireFox v107 1-Worker|7496|237|
|weForth v1|WASM / C|-O2|FireFox v107 1-Worker|1922|157|
|weForth v1|WASM / C|-O3|FireFox v107 1-Worker|1847|174|
|||||||
|weForth+switch|WASM / C|--|FireFox v107 1-Worker|7676|256|
|weForth+switch|WASM / C|-O2|FireFox v107 1-Worker|3780|168|
|weForth+switch|WASM / C|-O3|FireFox v107 1-Worker|3755|185|
|||||||
|weForth v1.2|WASM / C, no yield|--|FireFox v107 1-Worker|988|232|
|weForth v1.2|WASM / C, no yield|-O2|FireFox v107 1-Worker|528|156|
|weForth v1.2|WASM / C, no yield|-O3|FireFox v107 1-Worker|553|173|

* Note1: uEforth v7 uses switch(op), instead of 'computed goto' (asm.js/WASM has no goto)
* Note2: weForth v1 uses subroutine indirected-threaded
* Note3: weForth+switch(op), is 2x slower than just function pointers. Why?
* Note4: v1.2 Web Worker without yield in nest() speed up 3x
       
### TODO
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
