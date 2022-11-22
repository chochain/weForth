///
/// @file
/// @brief weForth - worker proxy to ceforth.js (called by ceforth.html)
///
Module = {
    print: postMessage
}
self.onmessage = function(e) {                    /// * link worker input port
    let forth = Module.cwrap('forth', null, ['number', 'string'])
    forth(0, e.data[0])                           /// * call Forth in C/C++
}
importScripts('ceforth.js')                       /// * load js emscripten created
