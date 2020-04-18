function verifyArray(ta, length) {
    var i;
    for (i = 0; i < length; ++i) {
        if (ta[i] != i) {
            postMessage("FAIL: Transferred data is incorrect. Expected " +
                i + " got " + ta[i]);
            return;
        }
    }
    postMessage("PASS: Transferred data is OK.");
}

function verifyArrayType(ta, name) {
    var className = Object.prototype.toString.call(ta);
    if (className.indexOf(name) != -1)
        postMessage("PASS: Transferred array type is OK.");
    else
        postMessage("FAIL: Expected array type " + name + " got " + className);
}

self.addEventListener('message', function(e) {
    var ab;
    var sab;
    var sab2;
    var ta;

    switch (e.data.name) {
        case 'SharedArrayBuffer':
            sab = e.data.data;
            ta = new Uint8Array(sab);
            verifyArray(ta, e.data.length);
            break;

        case 'Int8Array':
        case 'Uint8Array':
        case 'Uint8ClampedArray':
        case 'Int16Array':
        case 'Uint16Array':
        case 'Int32Array':
        case 'Uint32Array':
        case 'Float32Array':
        case 'Float64Array':
            ta = e.data.data;
            verifyArrayType(ta, e.data.name);
            verifyArray(ta, e.data.length);
            break;

        case 'ArrayBufferAndSharedArrayBuffer':
            ab = e.data.ab;
            sab = e.data.sab;
            verifyArray(new Uint8Array(ab), e.data.abByteLength);
            verifyArray(new Uint8Array(sab), e.data.sabByteLength);
            break;

        case 'SharedArrayBufferTwice':
            sab = e.data.sab;
            sab2 = e.data.sab2;
            if (sab !== sab2) {
                postMessage('FAIL: Expected two SharedArrayBuffers to be equal.');
            }
            verifyArray(new Uint8Array(sab), e.data.sabByteLength);
            break;

        default:
            postMessage("ERROR: unknown command " + e.data.name);
            break;
    }
    postMessage("DONE");
});
