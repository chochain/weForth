# weForth - Web eForth with WASM

WebAssembly enpowered eForth on web browsers, is it faster? is portable?

Well, so far. It's not that fast. About 20% slower than pure Javascript implementation and 10~20x slower than running on CPU natively. On the portability end, it's not exactly plug-and-play either! Though mostly minor but some alteration and care are required to make it web-eabled.

Regardless, it's fun to see eForth run in a browser straight from C/C++ code. Hopefully, with stright WASM implementation, it can speed up a bit to become a worthy alternative.

### Features
* Access to ss, dict (via WebAssembly.Memory)
  > ref https://stackoverflow.com/questions/46748572/how-to-access-webassembly-linear-memory-from-c-c
* Forth in Web Worker thread

### To Compile (make sure emscripten is installed)
* em++ -o tests/ceforth.html src/ceforth.cpp --shell-file src/forth_template.html -sEXPORTED_FUNCTIONS=_main,_forth,_vm_ss,_vm_ss_idx,_vm_dict_idx,_vm_dict,_top -sEXPORTED_RUNTIME_METHODS=ccall,cwrap
  > Note: -O2 works OK, -O3 emscripten spits wrong code
  
* em++ -o tests/ceForth_403.html src/ceForth_403.cpp --shell-file src/forth_template.html -sEXPORTED_FUNCTIONS=_main,_forth -sEXPORTED_RUNTIME_METHODS=ccall,cwrap

### To Compile to Web Worker
* cp src/forth_static.html tests/ceforth.html
* em++ -o tests/ceforth.js src/ceforth.cpp -sEXPORTED_FUNCTIONS=_main,_forth,_vm_ss,_vm_ss_idx,_vm_dict_idx,_vm_dict,_top -sEXPORTED_RUNTIME_METHODS=ccall,cwrap
* add messaging import/export to tests/ceforth.js (after the first line, i.e. var Module = ...)
  <pre>
    Module['print'] = postMessage                   /// * link worker output port
    onmessage = function(e) {                       /// * link worker input port
      let forth = Module.cwrap('forth', null, ['number', 'string'])
      forth(0, e.data[0])                           /// * call Forth in C/C++
    }
  </pre>

### To Debug (dump all functions)
* em++ -o tests/ceforth.html src/ceforth.cpp --shell-file src/forth_template.html -sEXPORTED_ALL=1 -sLINKABLE=1 -sEXPORTED_RUNTIME_METHODS=ccall,cwrap

### To Run
* Server-side
  > python3 tests/serv.py
* Client-side Browser
  > http://localhost:8000/ceforth.html

### Benchmark (on IBM X230)
|||optimization|Platform|1K*10K cycles (in ms)|code size (KB)|
|---|---|---|---|---|---|
|ceforth v8|C|--|CPU|214|91|
|ceforth v8|C|-O2|CPU|104|70|
|ceforth v8|C|-O3|CPU|105|74|
|eforth.js v6|JavaScript||FireFox v107|1550|20|
|uEforth v7|asm.js / C|--|FireFox v107|959|?|
|weforth v1|WASM / C|--|FireFox v107|7433|237|
|weforth v1|WASM / C|-O2|FireFox v107|1901|157|
|weforth v1|WASM / C|-O3|FireFox v107|failed(unknown function)|174|
|weforth v1|WASM / C|--|FireFox v107 1-Worker|7496|237|
|weforth v1|WASM / C|-O2|FireFox v107 1-Worker|1922|157|
|weforth v1|WASM / C|-O3|FireFox v107 1-Worker|1847|174|

### TODO
* refactor
  > inner interpreter in WASM (with indirect_call, i.e. WebAssembly.Table)
  > outer interpreter and IO in C
* use WASM stack as ss
* macro-assembler
* multi-VM
