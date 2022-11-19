# weForth - Web eForth with WASM

portable eForth running on web browsers with WASM

KJV 1900: Why came we forth out of Egpyt?

### run Web Server
python3 tests/serv.py

### To compile
> em++ -o tests/ceforth.html src/ceforth.cpp --shell-file src/forth_template.html -sEXPORTED_FUNCTIONS=_main,_forth -sEXPORTED_RUNTIME_METHODS=ccall,cwrap

> em++ -o tests/ceForth_403.html src/ceForth_403.cpp --shell-file src/forth_template.html -sEXPORTED_FUNCTIONS=_main,_forth -sEXPORTED_RUNTIME_METHODS=ccall,cwrap
