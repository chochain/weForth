# weForth - Web eForth with WASM

WebAssembly enpowered eForth on web browsers

### To Compile (make sure emscripten is installed)
> em++ -o tests/ceforth.html src/ceforth.cpp --shell-file src/forth_template.html -sEXPORTED_FUNCTIONS=_main,_forth,_vm_ss,_vm_ss_idx,_vm_top,_vm_dict_idx,_vm_dict -sEXPORTED_RUNTIME_METHODS=ccall,cwrap
> em++ -o tests/ceForth_403.html src/ceForth_403.cpp --shell-file src/forth_template.html -sEXPORTED_FUNCTIONS=_main,_forth -sEXPORTED_RUNTIME_METHODS=ccall,cwrap

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

### Features
* WASM.Memory to rs, ss, dict, pmem
  > https://stackoverflow.com/questions/46748572/how-to-access-webassembly-linear-memory-from-c-c

### TODO
* WebWorker for multi-VM (see GreenArray)
* inner interpreter with indirect_call
* use WASM stack as ss
* macro-assembler
