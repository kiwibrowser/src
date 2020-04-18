if (self.importScripts) {
    importScripts('common.js');
}

function runCloneECTests(format, algorithmName, namedCurve, keyType, extractable, keyUsages, keyData)
{
    var importData = hexStringToUint8Array(keyData);
    var importAlgorithm = { name: algorithmName, namedCurve: namedCurve };

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
        shouldEvaluateAs("importedKey.algorithm.namedCurve", namedCurve);
        shouldEvaluateAs("importedKey.usages.join(',')", keyUsages.join(","));

        shouldNotBe("importedKey", "clonedKey");

        shouldBeUndefined("clonedKey.extraProperty");
        shouldEvaluateAs("clonedKey.type", keyType);
        shouldEvaluateAs("clonedKey.extractable", extractable);
        shouldEvaluateAs("clonedKey.algorithm.name", algorithmName);
        shouldEvaluateAs("clonedKey.algorithm.namedCurve", namedCurve);
        shouldEvaluateAs("clonedKey.usages.join(',')", keyUsages.join(","));

        logSerializedKey(importedKey);

        if (extractable)
            bytesShouldMatchHexString("Cloned key exported data", keyData, clonedKeyData);

        debug("");
    });
}

function testCloneEcKeys(format, algorithmName, possibleKeyUsages, keyType, keyDataForCurve)
{
    var lastPromise = Promise.resolve(null);
    var kPossibleExtractable = [true, false];
    var kPossibleNamedCurves = ['P-256', 'P-384', 'P-521'];

    kPossibleNamedCurves.forEach(function(namedCurve) {
        kPossibleExtractable.forEach(function(extractable) {
            possibleKeyUsages.forEach(function(keyUsages) {
                lastPromise = lastPromise.then(runCloneECTests.bind(null, format, algorithmName, namedCurve, keyType, extractable, keyUsages, keyDataForCurve[namedCurve]));
            });
        });
    });

    return lastPromise;
}

function testCloneEcPublicKeys(algorithmName, possibleKeyUsages)
{
    var kKeyDataForCurve = {
        "P-256": "3059301306072A8648CE3D020106082A8648CE3D030107034200049CB0CF6930" +
                 "3DAFC761D4E4687B4ECF039E6D34AB964AF80810D8D558A4A8D6F72D51233A17" +
                 "88920A86EE08A1962C79EFA317FB7879E297DAD2146DB995FA1C78",
        "P-384": "3076301006072A8648CE3D020106052B81040022036200040874A2E0B8FF448F" +
                 "0E54321E27F4F1E64D064CDEB7D26F458C32E930120F4E57DC85C2693F977EED" +
                 "4A8ECC8DB981B4D91F69446DF4F4C6F5DE19003F45F891D0EBCD2FFFDB5C81C0" +
                 "40E8D6994C43C7FEEDB98A4A31EDFB35E89A30013C3B9267",
        "P-521": "30819B301006072A8648CE3D020106052B81040023038186000400F50A087032" +
                 "50C15F043C8C46E99783435245CF98F4F2694B0E2F8D029A514DD6F0B086D4ED" +
                 "892000CD5590107AAE69C4C0A7A95F7CF74E5770A07D5DB55BCE4AB400F2C770" +
                 "BAB8B9BE4CDB6ECD3DC26C698DA0D2599CEBF3D904F7F9CA3A55E64731810D73" +
                 "CD317264E50BABA4BC2860857E16D6CBB79501BC9E3A32BD172EA8A71DEE"
    };
    return testCloneEcKeys("spki", algorithmName, possibleKeyUsages, "public", kKeyDataForCurve);
}

function testCloneEcPrivateKeys(algorithmName, possibleKeyUsages)
{
    var kKeyDataForCurve = {
        "P-256": "308187020100301306072A8648CE3D020106082A8648CE3D030107046D306B02" +
                 "010104201FE33950C5F461124AE992C2BDFDF1C73B1615F571BD567E60D19AA1" +
                 "F48CDF42A144034200047C110C66DCFDA807F6E69E45DDB3C74F69A1484D203E" +
                 "8DC5ADA8E9A9DD7CB3C70DF448986E51BDE5D1576F99901F9C2C6A806A47FD90" +
                 "7643A72B835597EFC8C6",
        "P-384": "3081B6020100301006072A8648CE3D020106052B8104002204819E30819B0201" +
                 "010430A492CE8FA90084C227E1A32F7974D39E9FF67A7E8705EC3419B35FB607" +
                 "582BEBD461E0B1520AC76EC2DD4E9B63EBAE71A16403620004E55FEE6C49D8D5" +
                 "23F5CE7BF9C0425CE4FF650708B7DE5CFB095901523979A7F042602DB3085473" +
                 "5369813B5C3F5EF86828F59CC5DC509892A988D38A8E2519DE3D0C4FD0FBDB09" +
                 "93E38F18506C17606C5E24249246F1CE94983A5361C5BE983E",
        "P-521": "3081EE020100301006072A8648CE3D020106052B810400230481D63081D30201" +
                 "01044201BD56BD106118EDA246155BD43B42B8E13F0A6E25DD3BB376026FAB4D" +
                 "C92B6157BC6DFEC2D15DD3D0CF2A39AA68494042AF48BA9601118DA82C6F2108" +
                 "A3A203AD74A181890381860004012FBCAEFFA6A51F3EE4D3D2B51C5DEC6D7C72" +
                 "6CA353FC014EA2BF7CFBB9B910D32CBFA6A00FE39B6CDB8946F22775398B2E23" +
                 "3C0CF144D78C8A7742B5C7A3BB5D23009CDEF823DD7BF9A79E8CCEACD2E4527C" +
                 "231D0AE5967AF0958E931D7DDCCF2805A3E618DC3039FEC9FEBBD33052FE4C0F" +
                 "EE98F033106064982D88F4E03549D4A64D"
    };
    return testCloneEcKeys("pkcs8", algorithmName, possibleKeyUsages, "private", kKeyDataForCurve);
}
