if (self.importScripts) {
    importScripts('common.js');
}

function runCloneSymmetricTests(algorithmName, extractable, keyUsages, keyData, hashName, keyHasLength)
{
    var importData = hexStringToUint8Array(keyData);
    var importAlgorithm = { name: algorithmName };
    if (hashName)
        importAlgorithm.hash = { name: hashName };
    var expectingHash = (algorithmName.toLowerCase() === "hmac");

    var results = {};

    return crypto.subtle.importKey('raw', importData, importAlgorithm, extractable, keyUsages).then(function(importedKey) {
        results.importedKey = importedKey;
        importedKey.extraProperty = 'hi';
        return cloneKey(importedKey);
    }).then(function(clonedKey) {
        results.clonedKey = clonedKey;
        if (extractable)
            return crypto.subtle.exportKey('raw', clonedKey);
        return null;
    }).then(function(clonedKeyData) {
        importedKey = results.importedKey;
        clonedKey = results.clonedKey;

        shouldEvaluateAs("importedKey.extraProperty", "hi");
        shouldEvaluateAs("importedKey.type", "secret");
        shouldEvaluateAs("importedKey.extractable", extractable);
        shouldEvaluateAs("importedKey.algorithm.name", algorithmName);
        if (keyHasLength)
            shouldEvaluateAs("importedKey.algorithm.length", importData.length * 8);
        if (expectingHash)
            shouldEvaluateAs("importedKey.algorithm.hash.name", hashName);
        shouldEvaluateAs("importedKey.usages.join(',')", keyUsages.join(","));

        shouldNotBe("importedKey", "clonedKey");

        shouldBeUndefined("clonedKey.extraProperty");
        shouldEvaluateAs("clonedKey.type", "secret");
        shouldEvaluateAs("clonedKey.extractable", extractable);
        shouldEvaluateAs("clonedKey.algorithm.name", algorithmName);
        if (keyHasLength)
            shouldEvaluateAs("clonedKey.algorithm.length", importData.length * 8);
        if (expectingHash)
            shouldEvaluateAs("clonedKey.algorithm.hash.name", hashName);
        shouldEvaluateAs("clonedKey.usages.join(',')", keyUsages.join(","));

        logSerializedKey(importedKey);

        if (extractable)
            bytesShouldMatchHexString("Cloned key exported data", keyData, clonedKeyData);

        debug("");
    });
}

function testCloneSymmetricKeys(algorithmName, possibleHashAlgorithms, possibleExtractable, possibleKeyUsages, possibleKeyData, keyHasLength)
{
    var lastPromise = Promise.resolve(null);

    possibleHashAlgorithms.forEach(function(hashName) {
        possibleExtractable.forEach(function(extractable) {
            possibleKeyUsages.forEach(function(keyUsages) {
                possibleKeyData.forEach(function(keyData) {
                    lastPromise = lastPromise.then(runCloneSymmetricTests.bind(null, algorithmName, extractable, keyUsages, keyData, hashName, keyHasLength));
                });
            });
        });
    });

    return lastPromise;
}
