self.onmessage = ({ data: [ id, buffer ] }) => {
    let name = 'worker['+id+']'
    console.log(name + ' received on: ' + Date.now());
    console.log(name + ' buffer=', buffer);
    buffer.foo = 456;
    const view = new Uint8Array(buffer);
    view[id]   = id + 123;
    console.log(name + ' sent on:' + Date.now());
    postMessage(name + ' updated');  // notify main
};
