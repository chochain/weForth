EM = em++ -Wall -O2 # -O3 does not work
CC = g++ -Wall -O2

SRC = ./src/ceforth.cpp

EXP = _main,_forth,_vm_base,_vm_dflt,_vm_ss,_vm_ss_idx,_vm_dict_idx,_vm_dict,_vm_mem,_top

HTML = \
	template/weforth.html \
	template/weforth.css \
	template/file_io.js \
	template/weforth_logo.js \
	template/weforth_helper.js \
	template/weforth_worker.js

all: zero one two
	echo "cmd: python3 -m http.server to start local web server"
	echo "cmd: enter http://localhost:8000/tests/ceforth.html or weforth.html to test"

zero: $(SRC)
	echo "WASM: eForth simple demo"
	$(EM) -o tests/eforth.html $^ \
		--shell-file template/eforth.html \
		-sEXPORTED_FUNCTIONS=$(EXP) \
		-sEXPORTED_RUNTIME_METHODS=ccall,cwrap

one: $(SRC)
	echo "WASM: eForth single-threaded"
	$(EM) -o tests/ceforth.html $^ \
		--shell-file template/ceforth.html \
		-sEXPORTED_FUNCTIONS=$(EXP) \
		-sEXPORTED_RUNTIME_METHODS=ccall,cwrap

two: $(SRC)
	echo "WASM: eForth + one worker thread"
	cp $(HTML) ./tests
	$(EM) -o tests/weforth.js $^ \
		-sEXPORTED_FUNCTIONS=$(EXP) \
		-sEXPORTED_RUNTIME_METHODS=ccall,cwrap

debug: $(SRC)
	echo "WASM: create WASM objdump file"
	cp $(HTML) ./tests
	$(CC) -o tests/eforth $^
	$(EM) -o tests/ceforth.html $^ --shell-file template/ceforth.html -sEXPORT_ALL=1 -sLINKABLE=1 -sEXPORTED_RUNTIME_METHODS=ccall,cwrap
	wasm-objdump -x tests/ceforth.wasm > tests/ceforth.wasm.txt

sdl: tests/sdl2.cpp
	$(EM) -o tests/sdl2.js $< -sSINGLE_FILE -sUSE_SDL=2 -sUSE_SDL_IMAGE=2 -sSDL2_IMAGE_FORMATS='["png"]' -sUSE_SDL_TTF=2 -sUSE_SDL_GFX=2 --preload-file tests/assets
	$(CC) -o tests/sdl2 $< `sdl2-config --cflags --libs` -lSDL2_image -lSDL2_ttf -lSDL2_gfx

gfx: tests/gfx.c
	$(EM) -o tests/gfx.js $< -sSINGLE_FILE -sUSE_SDL=2 -sUSE_SDL_IMAGE=2 -sSDL2_IMAGE_FORMATS='["png"]' -sUSE_SDL_TTF=2 -sUSE_SDL_GFX=2 --preload-file tests/assets
	$(CC) -o tests/gfx $< `sdl2-config --cflags --libs` -lSDL2_gfx

gl: tests/gl.c
	$(CC) -o tests/gl $< `sdl2-config --cflags --libs` -lGL
