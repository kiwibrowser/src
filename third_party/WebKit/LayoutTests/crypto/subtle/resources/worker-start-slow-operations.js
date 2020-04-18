function generateRsaKey()
{
    var extractable = false;
    var usages = ['sign', 'verify'];
    var algorithm = {name: "RSASSA-PKCS1-v1_5", modulusLength: 512, publicExponent: new Uint8Array([1, 0, 1]), hash: {name: 'sha-1'}};

    return crypto.subtle.generateKey(algorithm, extractable, usages).then(undefined, function(error) {
        postMessage(error);
    });
}

// Start many concurrent operations but don't do anything with the result, or
// even wait for them to complete.
for (var i = 0; i < 1000; ++i)
    generateRsaKey();

// Inform the outer script that the worker started.
postMessage("Worker started");

close();
