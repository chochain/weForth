///
/// @file
/// @brief weForth - worker proxy to ceforth.js (called by ceforth.html)
///
Module = {
    print: (e)=>postMessage([ 'cmd', e ])
}
var vm_dict_len = 0
var vm_mem_addr = 0

importScripts('weforth_helper.js')             /// * vocabulary handler

function send_ss() {
    let ex  = Module.asm
    let base= ex.vm_base()
    let len = ex.vm_ss_idx()
    let ss  = new Int32Array(Module.asm.memory.buffer, ex.vm_ss(), len)
    let top = new Int32Array(Module.asm.memory.buffer, ex.top, 1)
    let div = []
    ss.forEach(v=>div.push(v.toString(base)))
    div.push(top[0].toString(base))
    postMessage([ 'ss', '[ ' + div.join(' ') + ' ]' ])
}
function send_dict() {
    let len = Module.asm.vm_dict_idx()
    if (vm_dict_len == len) return             /// * dict no change, skip
    
    vm_dict_len = len
    let dict = Module.cwrap('vm_dict', 'string', ['number'])
    let nlst = []                              ///< built-in words list
    let clst = null                            ///< colon word list
    for (let i = 0; i < len; ++i) {
        let nm = dict(i)
        if (clst) clst.push(nm)
        else {
            if (nm[0]!='_') nlst.push(nm)
            if (nm == 'boot') clst = []
        }
    }
    postMessage([ 'dc', voc_tree(nlst) ])
    postMessage([ 'us', colon_words(clst) ])
}
function send_mem(off, len) {
    let adr = Module.asm.vm_mem() + off
    let mem = new Uint8Array(Module.asm.memory.buffer, adr, len)
    postMessage([ 'mm', mem ])
}
self.onmessage = function(e) {                    /// * link worker input port
    let forth = Module.cwrap('forth', null, ['number', 'string'])
    let k = e.data[0], v = e.data[1]
    switch (k) {
    case 'cmd': forth(0, v);          break       /// * call Forth in C/C++
    case 'ss' : send_ss();            break
    case 'dc' : send_dict();          break
    case 'mm' : send_mem(v[0], v[1]); break
    default: postMessage('unknown type');
    }
}
importScripts('weforth.js')                       /// * load js emscripten created
