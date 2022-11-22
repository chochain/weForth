# weForth - Web eForth with WASM

WebAssembly enpowered eForth on web browsers, is it faster? is portable?

### To Compile (make sure emscripten is installed)
> em++ -o tests/ceforth.html src/ceforth.cpp --shell-file src/forth_template.html -sEXPORTED_FUNCTIONS=_main,_forth,_vm_ss,_vm_ss_idx,_vm_dict_idx,_vm_dict,_top -sEXPORTED_RUNTIME_METHODS=ccall,cwrap
>> Note: -O2 works OK, -O3 emscripten spits wrong code
> em++ -o tests/ceForth_403.html src/ceForth_403.cpp --shell-file src/forth_template.html -sEXPORTED_FUNCTIONS=_main,_forth -sEXPORTED_RUNTIME_METHODS=ccall,cwrap
### Compile to Web Worker
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

### Debug (dump all functions)
> em++ -o tests/ceforth.html src/ceforth.cpp --shell-file src/forth_template.html -sEXPORTED_ALL=1 -sLINKABLE=1 -sEXPORTED_RUNTIME_METHODS=ccall,cwrap

### Start Web Server
python3 tests/serv.py

### From Browser
http://localhost:8000/ceforth.html

### Benchmark (on IBM X230)
|||optimization|Platform|10K*10K cycles (in ms)|code size (KB)|
|---|---|---|---|---|---|
|ceforth v8|C|--|CPU|2101|91|
|ceforth v8|C|-O3|CPU|960|74|
|eforth.js v6|JavaScript||FireFox v107.0(64-bit)|15500|20|
|weforth v1|WASM / C|--|FireFox v107.0(64-bit)|7814|237|
|weforth v1|WASM / C|-O3|FireFox v107.0(64-bit)|4211|173|
|weforth v1|WASM / C|-O3|FireFox v107.0(64-bit) 1-Worker|80760?|193|

### Features
* WASM.Memory to rs, ss, dict, pmem
  > https://stackoverflow.com/questions/46748572/how-to-access-webassembly-linear-memory-from-c-c
* use Web Worker

### TODO
* refactor
  > inner interpreter in WASM (with indirect_call, i.e. WebAssembly.Table)
  > outer interpreter and IO in C
* use WASM stack as ss
* macro-assembler
* multi-VM
