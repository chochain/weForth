///
/// @file
/// @brief weForth - worker proxy to weforth.js (called by weforth.html)
///
Module = {
    print: e=>postMessage([ 'cmd', e ])
}
var vm_dict_len = 0
var vm_mem_addr = 0

importScripts('weforth_helper.js')  /// * vocabulary handler
importScripts('weforth.js')         /// * load js emscripten created

function send_ss() {
    const wa  = wasmExports
    const base= wa.vm_base()
    const len = wa.vm_ss_idx()>0 ? wa.vm_ss_idx() : 0
    const ss  = base==10
          ? new Int32Array(wa.memory.buffer, wa.vm_ss(), len)
          : new Uint32Array(wa.memory.buffer, wa.vm_ss(), len)
    const top = base==10
          ? new Int32Array(wa.memory.buffer, wa.top, 1)
          : new Uint32Array(wa.memory.buffer, wa.top, 1)
    let div = []
    ss.forEach(v=>div.push(v.toString(base)))
    div.push(top[0].toString(base))
    postMessage([ 'ss', '[ ' + div.join(' ') + ' ]' ])
}
function send_dict() {
    const wa  = wasmExports
    const len = wa.vm_dict_idx()
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
    const wa = wasmExports
    const hx = '0123456789ABCDEF'
    const h2 = v=>hx[(v>>4)&0xf]+hx[v&0xf]
    const h4 = v=>h2(v>>8)+h2(v)
    const adr = wa.vm_mem() + off
    const mem = new Uint8Array(wa.memory.buffer, adr, len)
    const n   = (off + len + 0x10) & ~0xf
    let div = '', bt = '', tx = ''
    for (let j = off&~0xf; j < n; j+=0x10) {
        for (let i = 0; i < 0x10; i++) {
            let ch = mem[i+j] || 0
            bt += `${hx[ch>>4]}${hx[ch&0xf]}`
            bt += ((i & 0x3)==3) ? '  ' : ' '
            tx += (ch < 0x20) ? '_' : String.fromCharCode(ch)
        }
        div += h4(j) + ': ' + bt + tx + '\n'
        bt = '', tx = ''
    }
    postMessage([ 'mm', div ])
}
///
/// worker message pipeline to main thread
///
self.onmessage = function(e) {                    /// * link worker input port
    let forth = Module.cwrap('forth', null, ['number', 'string'])
    let k = e.data[0], v = e.data[1]
    switch (k) {
    case 'cmd': forth(0, v);          break       /// * call Forth in C/C++
    case 'ss' : send_ss();            break
    case 'dc' : send_dict();          break
    case 'mm' : send_mem(v[0], v[1]); break
    case 'ui' : send_ui();            break
    default: postMessage('unknown type');
    }
}
