///
/// @file
/// @brief weForth - worker proxy to weforth.js (called by weforth.html)
///
Module = { print: s=>postMessage(['txt', s]) } ///> WASM print interface => output queue

var vm_dict_len = 0
var vm_boot_idx = 0

importScripts('weforth_helper.js')             ///> vocabulary handler
importScripts('weforth.js')                    ///> load js emscripten created

function get_ss() {
    const FX = v=>Number.isInteger(v)          ///> precision control macro
          ? v.toString(base)
          : Math.round(v*100000)/100000        /// * better than toFixed()
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
    let   div = []
    ss.forEach(v=>div.push(FX(v)))
    div.push(FX(top[0]))
    
    return '[ '+div.join(' ')+' ]'
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
function get_mem(off, len) {
    const wa  = wasmExports
    const adr = wa.vm_mem() + off
    return new Uint8Array(wa.memory.buffer, adr, len)   /// CC: freed by caller?
}

let dump_mem0 = ''                                      /// memory cache
function dump(mem, off) {
    const hx  = '0123456789ABCDEF'
    const h2  = v=>hx[(v>>4)&0xf]+hx[v&0xf]
    const h4  = v=>h2(v>>8)+h2(v)
    if (dump_mem0.length != mem.length) {               /// check buffer size
        dump_mem0 = new Uint8Array(mem.length)          /// free and realloc
    }
    let div = ''
    for (let j = 0; j < mem.length; j+=0x10) {
        let bt = '', tx = '', en = 0
        for (let i = 0; i < 0x10; i++) {
            let c0 = dump_mem0[j + i] || 0
            let c  = dump_mem0[j + i] = mem[j + i] || 0 ///> also cache the char
            if (!en && c != c0) {
                bt += '<i>'; tx += '<i>'; en = 1        /// * enter i element
            }
            else if (en && c == c0) {
                bt += '</i>'; tx += '</i>'; en = 0      /// * exit <i> element
            }
            bt += `${hx[c>>4]}${hx[c&0xf]}`
            bt += ((i & 0x3)==3) ? '  ' : ' '
            tx += (c < 0x20) ? '_' : String.fromCharCode(c)
        }
        if (en) { bt += '</i>'; tx += '</i>' }
        div += h4(off+j) + ': ' + bt + tx + '\n'
    }
    return div
}
function get_px(v) {
    console.log(v)
    const op = v[0], fg = v[1]|0  ///> opcode, foreground-color
    const px = v[2]|0             ///> offsets to geometry buffer
    const ps = v[3]|0             ///> offset to shape buffer
    const wa = wasmExports
    const mem= wa.vm_mem()
    ///
    /// references to WASM ArrayBuffer without copying (fast, < 1ms)
    ///
    const x0 = new Float32Array(wa.memory.buffer, mem+px, 3)  ///> x1, x2, x3
    const s0 = new Float32Array(wa.memory.buffer, mem+ps, 8)  ///> id, pos, rot
    ///
    /// convert to transferable objects (F64 takes 2ms, F32 5ms)
    ///
    const x1 = new Float64Array(x0)
    const s1 = new Float64Array(s0)
    return [ op, fg, x1, s1, v[4], Date.now()-v[4] ]
}
///
/// worker message pipeline to main thread
///
/// @note: serialization, i.e. structuredClone(), is slow (24ms)
///        so prebuild a transferable object is much faster (~5ms)
///
self.onmessage = function(e) {                        ///> worker input message queue
    let k = e.data[0], v = e.data[1]
    const post = (v)=>postMessage([k, v])             ///> macro to response to front-end
    switch (k) {
    case 'cmd':
        let forth =
            Module.cwrap('forth', null, ['number', 'string'])
        forth(0, v)                                   /// * calls Module.print
        break
    case 'dc' : post(get_dict());            break
    case 'usr': post(get_dict(true));        break
    case 'ss' : post(get_ss());              break
    case 'dm' :                                       /// * dump memory
        const idx = v[0], n = v[1]
        const here= wasmExports.vm_mem_idx()
        const len = (n + 0x10) & ~0xf                 ///> 16-byte blocks
        const off = idx < 0                           ///> idx < 0 => from 'HERE'
            ? (here > len ? here - len : 0)
            : idx
        const ma  = get_mem(off & ~0xf, len)          ///> get memory ref
        post(dump(ma, off));                 break
    case 'mm' :
        const mm = get_mem(v[0], v[1])                ///> fetch memory block
        postMessage(                                  /// * to front-end, transfer
            [ k, mm ],
            [ mm.buffer ]);                  break
    case 'px' :
        const px = get_px(v)                          ///> fetch px values from Forth
        postMessage(                                  ///> to front-end, transfer
            [ k, px ],
            [ px[2].buffer, px[3].buffer ]); break
    default   : post('unknown type');
    }
}
