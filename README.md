# weForth - Web eForth with WASM

WebAssembly enpowered eForth on web browsers, is it faster? is portable?

Well, so far. It's not that fast. About 20% slower than pure Javascript implementation and 10~20x slower than running on CPU natively. On the portability end, it's not exactly plug-and-play either! Though mostly minor but some alteration and care are required to make it web-eabled not to mention updating DOM.

Regardless, it's fun to see eForth run in a browser straight from C/C++ code. Other popular scripting languages such as Python, Ruby are trending toward WASM/WASI, as well. However, they will not likely to speed up much (i.e. stuck at 10~20x slower) since there's no built-in compiler utility as Forth does.

It's exciting to see that interops between different languages become a thing of the near future. So, hopefully, with stright WASM implementation plus the compiler, eForth can speed up a bit to become a worthy alternative.

### Features
* Javascript access to ss, dict (via WebAssembly.Memory)
* Forth in Web Worker thread (can support multi-VMs)

### To Compile (make sure emscripten is installed)
* em++ -o tests/ceforth.html src/ceforth.cpp --shell-file src/forth_template.html -sEXPORTED_FUNCTIONS=_main,_forth,_vm_ss,_vm_ss_idx,_vm_dict_idx,_vm_dict,_top -sEXPORTED_RUNTIME_METHODS=ccall,cwrap
  > Note: -O2 works OK, -O3 emscripten spits wrong code
  
* em++ -o tests/ceForth_403.html src/ceForth_403.cpp --shell-file src/forth_template.html -sEXPORTED_FUNCTIONS=_main,_forth -sEXPORTED_RUNTIME_METHODS=ccall,cwrap

### To Compile to Web Worker (run almost at the same speed as main thread)
* cp src/forth_static.html tests/ceforth.html; cp src/ceforth_worker.js tests
* em++ -o tests/ceforth.js src/ceforth.cpp -sEXPORTED_FUNCTIONS=_main,_forth,_vm_ss,_vm_ss_idx,_vm_dict_idx,_vm_dict,_top -sEXPORTED_RUNTIME_METHODS=ccall,cwrap

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
|weforth+switch|WASM / C|--|FireFox v107 1-Worker|7676|256|
|weforth+switch|WASM / C|-O2|FireFox v107 1-Worker|3780|168|
|weforth+switch|WASM / C|-O3|FireFox v107 1-Worker|3755|185|

* Note1: uEforth v7 uses switch(op), instead of 'computed goto' (asm.js/WASM has no goto)
* Note2: weforth v1 uses subroutine indirected-threaded, which is 2x slower than switch
* Note3: but, weforth+switch(op), is 2x slower than just function pointers?
       
### TODO
* refactor
  > inner interpreter in WASM (with indirect_call, i.e. WebAssembly.Table)<br/>
  > outer interpreter and IO in C
* use WASM stack as ss
* macro-assembler
* inter-VM communication
