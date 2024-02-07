EM = em++
CC = g++

FLST = \
	template/weforth.html \
	template/weforth.css \
	template/file_io.js \
	template/weforth_helper.js \
	template/weforth_worker.js

one: src/ceforth.cpp template/ceforth.html
	echo "WASM: eForth single-threaded"
	$(EM) -o tests/ceforth.html src/ceforth.cpp --shell-file template/ceforth.html -sEXPORTED_FUNCTIONS=_main,_forth,_vm_base,_vm_ss,_vm_ss_idx,_vm_dict_idx,_vm_dict,_vm_mem,_top -sEXPORTED_RUNTIME_METHODS=ccall,cwrap -O2

two: src/ceforth.cpp $(FLST)
	echo "WASM: eForth + one worker thread"
	cp $(FLST) ./html
	$(EM) -o tests/weforth.js src/ceforth.cpp -sEXPORTED_FUNCTIONS=_main,_forth,_vm_base,_vm_ss,_vm_ss_idx,_vm_dict_idx,_vm_dict,_vm_mem,_top -sEXPORTED_RUNTIME_METHODS=ccall,cwrap -O2

debug: src/ceforth.cpp $(FLST)
	echo "WASM: create WASM objdump file"
	$(EM) -o tests/ceforth.html src/ceforth.cpp --shell-file template/ceforth.html -sEXPORT_ALL=1 -sLINKABLE=1 -sEXPORTED_RUNTIME_METHODS=ccall,cwrap
	wasm-objdump -x tests/ceforth.wasm > tests/ceforth.wasm.txt

all: one two
	echo "cmd: python3 -m http.server to start local web server"
	echo "cmd: enter http://localhost:8000/tests/ceforth.html or weforth.html to test"

sdl: tests/sdl2.cpp
	$(EM) -o tests/sdl2.js tests/sdl2.cpp -sSINGLE_FILE -sUSE_SDL=2 -sUSE_SDL_IMAGE=2 -sSDL2_IMAGE_FORMATS='["png"]' -sUSE_SDL_TTF=2 -sUSE_SDL_GFX=2 --preload-file tests/assets
	$(CC) -o tests/sdl2 tests/sdl2.cpp `sdl2-config --cflags --libs` -lSDL2_image -lSDL2_ttf -lSDL2_gfx

gfx: tests/gfx.c
	$(EM) -o tests/gfx.js tests/gfx.c -sSINGLE_FILE -sUSE_SDL=2 -sUSE_SDL_IMAGE=2 -sSDL2_IMAGE_FORMATS='["png"]' -sUSE_SDL_TTF=2 -sUSE_SDL_GFX=2 --preload-file tests/assets
	$(CC) -o tests/gfx tests/gfx.c `sdl2-config --cflags --libs` -lSDL2_gfx

gl: tests/gl.c
	$(CC) -o tests/gl tests/gl.c `sdl2-config --cflags --libs` -lGL
