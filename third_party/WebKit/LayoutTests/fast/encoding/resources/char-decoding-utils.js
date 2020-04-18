function decodeText(charsetName, characterSequence) {
  return new Promise((resolve, reject) => {
    const req = new XMLHttpRequest;
    req.open('GET', `data:text/plain,${characterSequence}`);
    req.overrideMimeType(`text/plain; charset="${charsetName}"`);
    req.send('');
    req.onload = () => resolve(req.responseText);
    req.onerror = () => reject(new Error(req.statusText));
  });
}

function decode(charsetName, characterSequence) {
  return decodeText(charsetName, characterSequence).then(decodedText => {
    return decodedText.split('')
      .map(char => char.charCodeAt(0))
      .map(code => 'U+' + ('0000' + code.toString(16).toUpperCase()).slice(-4))
      .join('/');
  });
}

function testDecode(charsetName, characterSequence, unicode) {
  promise_test(t => {
    return decode(charsetName, characterSequence).then(result => {
      assert_equals(result, unicode);
    });
  }, `Decode ${charsetName}: ${characterSequence} => ${unicode}`);
}

function batchTestDecode(inputData) {
  for (let i in inputData.encodings) {
    for (let j in inputData.encoded) {
      testDecode(inputData.encodings[i],
                 inputData.encoded[j],
                 inputData.unicode[j]);
    }
  }
}
