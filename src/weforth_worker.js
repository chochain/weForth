///
/// @file
/// @brief weForth - worker proxy to ceforth.js (called by ceforth.html)
///
Module = {
    print: (e)=>postMessage([ 'cmd', e ])
}
var vm_dict_len = 0

importScripts('weforth_helper.js')             /// * vocabulary handler

function show_ss() {
    let ex  = Module.asm
    let base= ex.vm_base()
    let len = ex.vm_ss_idx()
    let ss  = new Int32Array(Module.asm.memory.buffer, ex.vm_ss(), len)
    let top = new Int32Array(Module.asm.memory.buffer, ex.top, 1)
    let div = []
    ss.forEach(v=>div.push(v.toString(base)))
    div.push(top[0].toString(base))
    postMessage([ 'ss', div ])
}
function show_dict() {
    let ex  = Module.asm          
    let len = ex.vm_dict_idx()
    if (vm_dict_len == len) return
    
    vm_dict_len = len
    let dict = Module.cwrap('vm_dict', 'string', ['number'])
    let nlst = []
    for (let i = 0; i < len; ++i) {
        let nm = dict(i)
        if (nm[0]!='_') nlst.push(nm)
    }
    postMessage([ 'dc', _voc_tree(nlst) ])
}
self.onmessage = function(e) {                    /// * link worker input port
    let forth = Module.cwrap('forth', null, ['number', 'string'])
    switch (e.data[0]) {
    case 'cmd': forth(0, e.data[1]); break        /// * call Forth in C/C++
    case 'ss' : show_ss();           break
    case 'dc' : show_dict();         break
    default: postMessage('unknown type');
    }
}
importScripts('weforth.js')                       /// * load js emscripten created
