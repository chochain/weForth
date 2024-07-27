///
/// @file
/// @brief weForth - worker proxy to weforth.js (called by weforth.html)
///
const res = (k,v)=>postMessage([ k, v ])  ///> worker response to front-end

Module = { print: e=>res('txt', e) }      ///> WASM print interface => output queue

var vm_dict_len = 0
var vm_mem_addr = 0
var vm_boot_idx = 0

importScripts('weforth_helper.js')        /// * vocabulary handler
importScripts('weforth.js')               /// * load js emscripten created

function get_ss() {
    const wa  = wasmExports
    const base= wa.vm_base()
    const toa = (p, n)=> wa.vm_dflt()
        ? new Float32Array(wa.memory.buffer, p, n)
        : (base==10
           ? new Int32Array(wa.memory.buffer, p, n)
           : new Uint32Array(wa.memory.buffer, p, n))
    const len = wa.vm_ss_idx()>0 ? wa.vm_ss_idx() : 0
    const ss  = toa(wa.vm_ss(), len)
    const top = toa(wa.top, 1)
    const tos = v => Number.isInteger(v)
          ? v.toString(base)
          : Math.round(v*100000)/100000   /// * better than toFixed()
    let   div = []
    ss.forEach(v=>div.push(tos(v)))
    div.push(tos(top[0]))
    
    return div.join(' ')
}
function get_dict(usr=false) {
    const wa  = wasmExports
    const len = wa.vm_dict_idx()
    if (vm_dict_len == len) return ''     /// * dict no change, use cached
    
    vm_dict_len = len
    const dict = Module.cwrap('vm_dict', 'string', ['number'])
    let lst = []                          ///< built-in words list
    for (let i = usr ? vm_boot_idx : 0; i < len; ++i) {
        let nm = dict(i)
        if (nm=='boot') vm_boot_idx = i+1 ///< capture the start of colon words
        if (nm[0] != '_') lst.push(nm)    ///< collect words
    }
    return usr ? colon_words(lst) : voc_tree(lst)
}
function get_mem(idx=-1, len=0x320) {
    const wa  = wasmExports
    const hx  = '0123456789ABCDEF'
    const h2  = v=>hx[(v>>4)&0xf]+hx[v&0xf]
    const h4  = v=>h2(v>>8)+h2(v)
    const off = idx < 0                 ///> idx < 0 => use HERE
          ? (wa.vm_mem_idx() > len ? wa.vm_mem_idx() - len : 0)
          : idx
    const adr = wa.vm_mem() + off
    const n   = (off + len + 0x10) & ~0xf
    const mem = new Uint8Array(wa.memory.buffer, adr, len)
    
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
    return div
}
///
/// worker message pipeline to main thread
///
self.onmessage = function(e) {         ///> worker input message queue
    let k = e.data[0], v = e.data[1]
    switch (k) {
    case 'cmd':
        let forth =
            Module.cwrap('forth', null, ['number', 'string'])
        forth(0, v)                    /// * calls Module.print
        break
    case 'dc' : res('dc',  get_dict());            break
    case 'usr': res('usr', get_dict(true));        break
    case 'ss' : res('ss',  '[ '+ get_ss() + ' ]'); break
    case 'mm' : res('mm',  !v ? get_mem() : get_mem(v[0], v[1])); break
    case 'ui' : res('ui',  get_ui());              break
    default   : res('unknown type');
    }
}
