if (!crossOriginIsolated) {
    throw new Error('COOP: not set for SharedArrayBuffer');
}

const worker0 = new Worker('shared_worker.js');
const worker1 = new Worker('shared_worker.js');

worker0.onmessage = e=>{               ///< response from workers
    console.log('main received on:' + Date.now());
    console.log(e.data);
}
worker1.onmessage = e=>{               ///< response from workers
    console.log('main received on:' + Date.now());
    console.log(e.data);
}

const buffer = new SharedArrayBuffer(1024); ///< default all 0
const view   = new Uint8Array(buffer);      ///< view to U8 array

console.log('main sent to 0 on:' + Date.now())
worker0.postMessage([ 0, buffer ]);
console.log('main sent to 1 on:' + Date.now())
worker1.postMessage([ 1, buffer ]);
console.log('main buffer=', buffer);

setTimeout(() => {                     // check after 0.5 sec
    console.log('view[0]',  view[0]);  // should = 123
    console.log('view[1]',  view[1]);  // should = 124
}, 500);                               
