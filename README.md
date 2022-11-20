# WeForth - Web eForth with WASM

WebAssembly enpowered eForth on web browsers

KJV 1900: Why came we forth out of Egpyt?

### run Web Server
python3 tests/serv.py

### To compile
> em++ -o tests/ceforth.html src/ceforth.cpp --shell-file src/forth_template.html -sEXPORTED_FUNCTIONS=_main,_forth -sEXPORTED_RUNTIME_METHODS=ccall,cwrap
> em++ -o tests/ceForth_403.html src/ceForth_403.cpp --shell-file src/forth_template.html -sEXPORTED_FUNCTIONS=_main,_forth -sEXPORTED_RUNTIME_METHODS=ccall,cwrap

### Benchmark
|||Platform|10K*10K cycles (in ms)|
|---|---|---|---|
|eforth.js v8|JS|FireFox v107.0(64-bit)|15500|
|ceforth v8|C on CPU|raw|2101|
|ceforth v8|WASM C |FireFox v107.0(64-bit)|7814|
