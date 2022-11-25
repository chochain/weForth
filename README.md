# weForth - Web eForth with WASM

WebAssembly enpowered eForth on web browsers, is it faster? is portable?

Well, the result is pretty exciting! It's about 3x faster than pure Javascript implementation and 5x slower than running on CPU natively. On the portability end, though not exactly plug-and-play but with some minor alteration can make it web-eabled. Of course, updating DOM is a different feat.

It brought me warm smiles to see eForth run in a browser. Better yet, straight from C/C++ code. Other popular scripting languages such as Python, Ruby are trending toward WASM/WASI implementation as well. However, without built-in compiler as Forth does, they will not likely to speed up much (i.e. stuck at 10~20x slower so far).

With WASM, the interoperability between different languages become a thing of the near future. Hopefully, a little bit more effort to compile word directly into WASM opcodes, to engage with WASI, to hookup the graphic front-end (i.g. SDL or WebGL), weForth can become a worthy scripting alternative for Web.

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
* compare features to uEforth
  + Forth-83
  <pre>
  Nucleus layer 
    !  *  */  */MOD  +  +!  -  /  /MOD  0<  0=  0>  1+  1-  2+ 2-  2/
    <  =  >  >R  ?DUP  @  ABS  AND  C!  C@  CMOVE CMOVE>  COUNT
    D+  D<  DEPTH  DNEGATE  DROP  DUP  EXECUTE EXIT  FILL
    I  J  MAX  MIN  MOD  NEGATE  NOT  OR  OVER  PICK
    R>  R@  ROLL  ROT  SWAP  U<  UM*  UM/MOD  XOR
  Device layer 
    BLOCK  BUFFER  CR  EMIT  EXPECT  FLUSH  KEY  SAVE-BUFFERS SPACE  SPACES  TYPE  UPDATE
  Interpreter layer 
    #  #>  #S  #TIB  '  (  -TRAILING  .  .(  <#  >BODY  >IN
    ABORT  BASE  BLK  CONVERT  DECIMAL  DEFINITIONS  FIND
    FORGET  FORTH  FORTH-83  HERE  HOLD  LOAD  PAD  QUIT  SIGN
    SPAN  TIB  U.  WORD
  Compiler layer 
    +LOOP  ,  ."  :  ;  ABORT"  ALLOT  BEGIN  COMPILE  CONSTANT
    CREATE  DO  DOES>  ELSE  IF  IMMEDIATE  LEAVE  LITERAL  LOOP
    REPEAT  STATE  THEN  UNTIL  VARIABLE  VOCABULARY  WHILE   
    [']  [COMPILE]  ]
  </pre>
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
