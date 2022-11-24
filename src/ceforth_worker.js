///
/// @file
/// @brief weForth - worker proxy to ceforth.js (called by ceforth.html)
///
Module = {
    print: (e)=>postMessage([ 'cmd', e ])
}
function send_ss() {
    let ex  = Module.asm
    let len = ex.vm_ss_idx()
    let ss  = new Int32Array(Module.asm.memory.buffer, ex.vm_ss(), len)
    let top = new Int32Array(Module.asm.memory.buffer, ex.top, 1)
    let div = [ top[0] ]
    for (let i = len - 1; i >= 0; --i) {
        div.push(ss[i])
    }
    postMessage([ 'ss', div ])
}
function send_dict() {
    let ex  = Module.asm          
    let len = ex.vm_dict_idx()
    let dict= Module.cwrap('vm_dict', 'string', ['number'])
    let div = []
    for (let i = len - 1; i >= 0; --i) {
        div.push(dict(i))
    }
    postMessage([ 'dc', div ])
}
self.onmessage = function(e) {                    /// * link worker input port
    let forth = Module.cwrap('forth', null, ['number', 'string'])
    switch (e.data[0]) {
    case 'cmd': forth(0, e.data[1]); break        /// * call Forth in C/C++
    case 'ss' : send_ss();           break
    case 'dc' : send_dict();         break
    default: postMessage('unknown type');
    }
}
importScripts('ceforth.js')                       /// * load js emscripten created
