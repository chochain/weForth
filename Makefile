CC = em++

FLST = \
	template/weforth.html \
	template/weforth.css \
	template/file_io.js \
	template/weforth_helper.js \
	template/weforth_worker.js

one: src/ceforth.cpp template/ceforth.html
	echo "WASM: eForth single-threaded"
	$(CC) -o tests/ceforth.html src/ceforth.cpp --shell-file template/ceforth.html -sEXPORTED_FUNCTIONS=_main,_forth,_vm_base,_vm_ss,_vm_ss_idx,_vm_dict_idx,_vm_dict,_vm_mem,_top -sEXPORTED_RUNTIME_METHODS=ccall,cwrap -O2

two: src/ceforth.cpp $(FLST)
	echo "WASM: eForth + one worker thread"
	cp $(FLST) ./tests
	$(CC) -o tests/weforth.js src/ceforth.cpp -sEXPORTED_FUNCTIONS=_main,_forth,_vm_base,_vm_ss,_vm_ss_idx,_vm_dict_idx,_vm_dict,_vm_mem,_top -sEXPORTED_RUNTIME_METHODS=ccall,cwrap -O2

debug: src/ceforth.cpp $(FLST)
	echo "WASM: create WASM objdump file"
	$(CC) -o tests/ceforth.html src/ceforth.cpp --shell-file template/ceforth.html -sEXPORT_ALL=1 -sLINKABLE=1 -sEXPORTED_RUNTIME_METHODS=ccall,cwrap
	wasm-objdump -x tests/ceforth.wasm > tests/ceforth.wasm.txt

all: one two
	echo "cmd: python3 tests/serv.py to start local web server"
	echo "cmd: enter http://localhost:8000/tests/ceforth.html or weforth.html to test"

sdl: tests/sdl2.cpp
	$(CC) -o tests/sdl2.js tests/sdl2.cpp -s WASM=1 -s USE_SDL=2 -s USE_SDL_TTF=2 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS='["png"]' --preload-file tests/assets

exe: tests/sdl2.cpp
	g++ -o tests/sdl2 tests/sdl2.cpp `sdl2-config --cflags --libs` -lSDL2_image -lSDL2_mixer -lSDL2_ttf
