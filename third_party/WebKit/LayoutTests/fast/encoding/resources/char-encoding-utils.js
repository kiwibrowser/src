let uniqueId = 0;
function encodeText(charsetName, unicode) {
  return new Promise((resolve, reject) => {
    const frame_id = `subframe${++uniqueId}`;

    const iframe = document.createElement('iframe');
    iframe.style.display = 'none';
    // |iframe.name| must be assigned before adding frame to the body or
    // |form.target| will not find it.
    iframe.name = frame_id;
    document.body.appendChild(iframe);

    const form = document.body.appendChild(document.createElement('form'));
    form.style.display = 'none';
    form.method = 'GET';
    form.action = 'resources/dummy.html';
    form.acceptCharset = charsetName;
    form.target = frame_id;

    const input = form.appendChild(document.createElement('input'));
    input.type = 'text';
    input.name = 'text';
    input.value = String.fromCharCode(unicode.replace('U+', '0x'));

    iframe.onload = () => {
      const url = iframe.contentWindow.location.href;
      const result = url.substr(url.indexOf('=') + 1);

      iframe.remove();
      form.remove();

      resolve(result);
    };
    form.submit();
  });
}

function testEncode(charsetName, unicode, characterSequence) {
  promise_test(t => {
    return encodeText(charsetName, unicode).then(result => {
      assert_equals(result, characterSequence);
    });
  }, `Encode ${charsetName}: ${unicode} -> ${characterSequence}`);
}
