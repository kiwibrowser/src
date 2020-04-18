onmessage = function (evt) {
    var jwkKey = {
        kty: "oct",
        k: "jnOw99oOZFLIEPMrgJB55WL46tJSLGt7jnOw99oOZFI"
    };
    Promise.resolve(null).then(function (result) {
        return crypto.subtle.importKey("jwk", jwkKey,
                                       {name: "AES-CBC"},
                                       true,
                                       ['encrypt', 'decrypt',
                                        'wrapKey', 'unwrapKey']);
    }).then(function (result) {
        postMessage({ success: true});
    }, function (result) {
        postMessage({ message: result.message });
    });
};
