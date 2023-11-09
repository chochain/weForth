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
importScripts('weforth.js')                    /// * load js emscripten created

function send_ss() {
    const ex  = Module.asm
    const base= ex.vm_base()
    const len = ex.vm_ss_idx()
    const ss  = new Int32Array(Module.asm.memory.buffer, ex.vm_ss(), len)
    const top = new Int32Array(Module.asm.memory.buffer, ex.top, 1)
    let div = []
    ss.forEach(v=>div.push(v.toString(base)))
    div.push(top[0].toString(base))
    postMessage([ 'ss', '[ ' + div.join(' ') + ' ]' ])
}
function send_dict() {
    const len = Module.asm.vm_dict_idx()
    if (vm_dict_len == len) return             /// * dict no change, skip
    
    vm_dict_len = len
    const dict = Module.cwrap('vm_dict', 'string', ['number'])
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
    const hx = '0123456789ABCDEF'
    const h2 = (v)=>hx[(v>>4)&0xf]+hx[v&0xf]
    const h4 = (v)=>h2(v>>8)+h2(v)
    const adr = Module.asm.vm_mem() + off
    const mem = new Uint8Array(Module.asm.memory.buffer, adr, len)
    const n   = (off + len + 0x10) & ~0xf
    let div = '', bt = '', tx = ''
    for (let j = off&~0xf; j < n; j+=0x10) {
        for (let i = 0; i < 0x10; i++) {
            let ch = mem[i+j] || 0
            bt += `${hx[ch>>4]}${hx[ch&0xf]}`
            if ((i & 0x3)==3) bt += ' '
            tx += (ch < 0x20) ? '_' : String.fromCharCode(ch)
        }
        div += h4(j) + ': ' + bt + tx + '\n'
        bt = '', tx = ''
    }
    postMessage([ 'mm', div ])
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
