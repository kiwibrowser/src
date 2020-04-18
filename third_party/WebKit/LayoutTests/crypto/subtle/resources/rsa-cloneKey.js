if (self.importScripts) {
    importScripts('common.js');
}

function runCloneRSATests(format, algorithmName, hashName, keyType, extractable, keyUsages, keyData)
{
    var importData = hexStringToUint8Array(keyData[format]);
    var importAlgorithm = { name: algorithmName, hash: { name: hashName } };

    var results = {};

    return crypto.subtle.importKey(format, importData, importAlgorithm, extractable, keyUsages).then(function(importedKey) {
        results.importedKey = importedKey;
        importedKey.extraProperty = 'hi';
        return cloneKey(importedKey);
    }).then(function(clonedKey) {
        results.clonedKey = clonedKey;
        if (extractable)
            return crypto.subtle.exportKey(format, clonedKey);
        return null;
    }).then(function(clonedKeyData) {
        importedKey = results.importedKey;
        clonedKey = results.clonedKey;

        shouldEvaluateAs("importedKey.extraProperty", "hi");
        shouldEvaluateAs("importedKey.type", keyType);
        shouldEvaluateAs("importedKey.extractable", extractable);
        shouldEvaluateAs("importedKey.algorithm.name", algorithmName);
        shouldEvaluateAs("importedKey.algorithm.modulusLength", keyData.modulusLengthBits);
        bytesShouldMatchHexString("importedKey.algorithm.publicExponent", keyData.publicExponent, importedKey.algorithm.publicExponent);
        shouldEvaluateAs("importedKey.algorithm.hash.name", hashName);
        shouldEvaluateAs("importedKey.usages.join(',')", keyUsages.join(","));

        shouldNotBe("importedKey", "clonedKey");

        shouldBeUndefined("clonedKey.extraProperty");
        shouldEvaluateAs("clonedKey.type", keyType);
        shouldEvaluateAs("clonedKey.extractable", extractable);
        shouldEvaluateAs("clonedKey.algorithm.name", algorithmName);
        shouldEvaluateAs("clonedKey.algorithm.modulusLength", keyData.modulusLengthBits);
        bytesShouldMatchHexString("clonedKey.algorithm.publicExponent", keyData.publicExponent, clonedKey.algorithm.publicExponent);
        shouldEvaluateAs("clonedKey.algorithm.hash.name", hashName);
        shouldEvaluateAs("clonedKey.usages.join(',')", keyUsages.join(","));

        logSerializedKey(importedKey);

        if (extractable)
            bytesShouldMatchHexString("Cloned key exported data", keyData[format], clonedKeyData);

        debug("");
    });
}

function testCloneRSAKeys(format, algorithmName, possibleKeyUsages, keyType, possibleKeyData)
{
    var lastPromise = Promise.resolve(null);
    var kPossibleExtractable = [true, false];
    var kPossibleHashAlgorithms = ['SHA-1', 'SHA-256', 'SHA-512'];

    kPossibleHashAlgorithms.forEach(function(hashName) {
        kPossibleExtractable.forEach(function(extractable) {
            possibleKeyUsages.forEach(function(keyUsages) {
                possibleKeyData.forEach(function(keyData) {
                    lastPromise = lastPromise.then(runCloneRSATests.bind(null, format, algorithmName, hashName, keyType, extractable, keyUsages, keyData));
                });
            });
        });
    });

    return lastPromise;
}

function testCloneRSAPublicKeys(algorithmName, possibleKeyUsages, keyData)
{
    return testCloneRSAKeys("spki", algorithmName, possibleKeyUsages, "public", keyData);
}

function testCloneRSAPrivateKeys(algorithmName, possibleKeyUsages, keyData)
{
    return testCloneRSAKeys("pkcs8", algorithmName, possibleKeyUsages, "private", keyData);
}
