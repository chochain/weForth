# Tests suite and examples
### C code
<pre>
#include <stdio.h>

int main() {
    printf("Hello World!\n");
    return 0;
}

int my_func(int n, char *str) {
    printf("my_func(%d, '%s') called\n", n, str);
    return 0;
}
</pre>

### JS glue
<pre>
    {{{ SCRIPT }}}
    <button id='my-button'>Click me</button>
    <script type='text/javascript'>
      document.getElementById('my-button').addEventListener('click', () => {
          var my_func = Module.cwrap('my_func', 'number', ['number', 'string'])
          my_func(123, 'my string')
          my_func(456, 'string2')
      })
    </script>
</pre>

### run Web Server
python3 tests/serv.py

### To compile
> emcc -o hello.html hello.c --shell-file hello_template.html -sEXPORTED_FUNCTIONS=_main,_my_func -sEXPORTED_RUNTIME_METHODS=ccall,cwrap

> emcc -o test_mem.js test_mem.c -sEXPORTED_FUNCTIONS=_main,_myArray -sEXPORTED_RUNTIME_METHODS=ccall,cwrap

> em++ -o test_fptr.html test_fptr.cpp --shell-file fptr_template.html -sEXPORTED_FUNCTIONS=_main,_forth -sEXPORTED_RUNTIME_METHODS=ccall,cwrap

