importScripts('common.js');

function continuouslyGenerateRsaKey()
{
    var extractable = false;
    var usages = ['sign', 'verify'];
    // Note that the modulus length is small.
    var algorithm = {name: "RSASSA-PKCS1-v1_5", modulusLength: 512, publicExponent: hexStringToUint8Array("010001"), hash: {name: 'sha-1'}};

    return crypto.subtle.generateKey(algorithm, extractable, usages).then(function(result) {
        // Infinite recursion intentional!
        return continuouslyGenerateRsaKey();
    });
}

// Starts a Promise which continually generates new RSA keys.
var unusedPromise = continuouslyGenerateRsaKey();

// Inform the outer script that the worker started.
postMessage("Worker started");
