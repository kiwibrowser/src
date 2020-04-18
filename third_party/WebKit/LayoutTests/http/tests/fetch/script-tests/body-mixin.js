if (self.importScripts) {
  importScripts('../resources/fetch-test-helpers.js');
  importScripts('/streams/resources/rs-utils.js');
}

function isLocked(stream) {
  try {
    var reader = stream.getReader();
    reader.releaseLock();
    return false;
  } catch(e) {
    return true;
  }
}

promise_test(function(test) {
    return fetch('/fetch/resources/doctype.html')
      .then(function(response) {
          // Accessing the body property makes the stream start working.
          var stream = response.body;
          return response.text();
        })
      .then(function(text) {
          assert_equals(text, '<!DOCTYPE html>\n');
        })
    }, 'FetchTextAfterAccessingStreamTest');

promise_test(function(test) {
    var actual = '';
    return fetch('/fetch/resources/doctype.html')
      .then(function(response) {
          r = response;
          return readableStreamToArray(response.body);
        })
      .then(function(chunks) {
          var decoder = new TextDecoder();
          for (var chunk of chunks) {
            actual += decoder.decode(chunk, {stream: true});
          }
          // Put an empty buffer without the stream option to end decoding.
          actual += decoder.decode(new Uint8Array(0));
          assert_equals(actual, '<!DOCTYPE html>\n');
        })
    }, 'FetchStreamTest');

promise_test(function(test) {
    return fetch('/fetch/resources/progressive.php')
      .then(function(response) {
          var p1 = response.text();
          // Because progressive.php takes some time to load, we expect
          // response.text() is not yet completed here.
          var p2 = response.text().then(function() {
              return Promise.reject(new Error('resolved unexpectedly'));
            }, function(e) {
              return e;
            });
          return Promise.all([p1, p2]);
        })
      .then(function(results) {
          assert_equals(results[0].length, 190);
          assert_equals(results[1].name, 'TypeError');
        })
    }, 'FetchTwiceTest');

promise_test(function(test) {
    var response;
    return fetch('/fetch/resources/doctype.html')
      .then(function(res) {
          response = res;
          assert_false(response.bodyUsed);
          var p = response.arrayBuffer();
          assert_true(response.bodyUsed);
          assert_true(isLocked(response.body));
          return p;
        })
      .then(function(b) {
          assert_true(isLocked(response.body));
          assert_equals(b.byteLength, 16);
        })
    }, 'ArrayBufferTest');

promise_test(function(test) {
    var response;
    return fetch('/fetch/resources/doctype.html')
      .then(function(res) {
          response = res;
          assert_false(response.bodyUsed);
          var p = response.blob();
          assert_true(response.bodyUsed);
          assert_true(isLocked(response.body));
          return p;
        })
      .then(function(blob) {
          assert_true(isLocked(response.body));
          assert_equals(blob.size, 16);
          assert_equals(blob.type, 'text/html');
        })
    }, 'BlobTest');

promise_test(function(test) {
    var response;
    return fetch('/fetch/resources/doctype.html')
      .then(function(res) {
          response = res;
          assert_false(response.bodyUsed);
          var p = response.formData();
          assert_true(response.bodyUsed);
          assert_true(isLocked(response.body));
          return p;
        })
      .then(
        unreached_fulfillment(test),
        function(e) {
          assert_true(isLocked(response.body));
          assert_equals(e.name, 'TypeError', 'expected MIME type error');
        })
    }, 'FormDataFailedTest');

promise_test(function(test) {
    var response;
    return fetch('/fetch/resources/doctype.html')
      .then(function(res) {
          response = res;
          assert_false(response.bodyUsed);
          var p = response.json();
          assert_true(response.bodyUsed);
          assert_true(isLocked(response.body));
          return p;
        })
      .then(
        test.unreached_func('json() must fail'),
        function(e) {
          assert_true(isLocked(response.body));
          assert_equals(e.name, 'SyntaxError', 'expected JSON error');
        })
    }, 'JSONFailedTest');

promise_test(function(test) {
    var response;
    return fetch('/fetch/resources/simple.json')
      .then(function(res) {
          response = res;
          assert_false(response.bodyUsed);
          var p = response.json();
          assert_true(response.bodyUsed);
          assert_true(isLocked(response.body));
          return p;
        })
      .then(function(json) {
          assert_true(isLocked(response.body));
          assert_equals(json['a'], 1);
          assert_equals(json['b'], 2);
        })
    }, 'JSONTest');

promise_test(function(test) {
    var response;
    return fetch('/fetch/resources/doctype.html')
      .then(function(res) {
          response = res;
          assert_false(response.bodyUsed);
          var p = response.text();
          assert_true(response.bodyUsed);
          assert_true(isLocked(response.body));
          return p;
        })
      .then(function(text) {
          assert_true(isLocked(response.body));
          assert_equals(text, '<!DOCTYPE html>\n');
        })
    }, 'TextTest');

promise_test(function(test) {
    return fetch('/fetch/resources/non-ascii.txt')
      .then(function(response) {
          assert_false(response.bodyUsed);
          var p = response.text();
          assert_true(response.bodyUsed);
          return p;
        })
      .then(function(text) {
          assert_equals(text, '\u4e2d\u6587 Gem\u00fcse\n');
        })
    }, 'NonAsciiTextTest');

promise_test(function(test) {
    return fetch('/fetch/resources/bom-utf-8.php')
      .then(function(response) { return response.text(); })
      .then(function(text) {
          assert_equals(text, '\u4e09\u6751\u304b\u306a\u5b50',
                        'utf-8 string with BOM is decoded as utf-8 and ' +
                        'BOM is not included in the decoded result.');
        })
    }, 'BOMUTF8Test');

promise_test(function(test) {
    return fetch('/fetch/resources/bom-utf-16le.php')
      .then(function(response) { return response.text(); })
      .then(function(text) {
          assert_equals(text, '\ufffd\ufffd\tNQgK0j0P[',
                        'utf-16le string is decoded as if utf-8 ' +
                        'even if the data has utf-16le BOM.');
        })
    }, 'BOMUTF16LETest');

promise_test(function(test) {
    return fetch('/fetch/resources/bom-utf-16be.php')
      .then(function(response) { return response.text(); })
      .then(function(text) {
          assert_equals(text, '\ufffd\ufffdN\tgQ0K0j[P',
                        'utf-16be string is decoded as if utf-8 ' +
                        'even if the data has utf-16be BOM.');
        })
    }, 'BOMUTF16BETest');

test(t => {
    var req = new Request('/');
    assert_false(req.bodyUsed);
    req.text();
    assert_false(req.bodyUsed);
  }, 'BodyUsedShouldNotBeSetForNullBody');

test(t => {
    var req = new Request('/', {method: 'POST', body: ''});
    assert_false(req.bodyUsed);
    req.text();
    assert_true(req.bodyUsed);
  }, 'BodyUsedShouldBeSetForEmptyBody');

test(t => {
    var res = new Response('');
    assert_false(res.bodyUsed);
    var reader = res.body.getReader();
    assert_false(res.bodyUsed);
    reader.read();
    assert_true(res.bodyUsed);
  }, 'BodyUsedShouldBeSetWhenRead');

test(t => {
    var res = new Response('');
    assert_false(res.bodyUsed);
    var reader = res.body.getReader();
    assert_false(res.bodyUsed);
    reader.cancel();
    assert_true(res.bodyUsed);
  }, 'BodyUsedShouldBeSetWhenCancelled');

promise_test(t => {
    var res = new Response('');
    res.body.cancel();
    return res.arrayBuffer().then(unreached_fulfillment(t), e => {
        assert_equals(e.name, 'TypeError');
      });
  }, 'Used => arrayBuffer');

promise_test(t => {
    var res = new Response('');
    res.body.cancel();
    return res.blob().then(unreached_fulfillment(t), e => {
        assert_equals(e.name, 'TypeError');
      });
  }, 'Used => blob');

promise_test(t => {
    var res = new Response('');
    res.body.cancel();
    return res.formData().then(unreached_fulfillment(t), e => {
        assert_equals(e.name, 'TypeError');
      });
  }, 'Used => formData');

promise_test(t => {
    var res = new Response('');
    res.body.cancel();
    return res.json().then(unreached_fulfillment(t), e => {
        assert_equals(e.name, 'TypeError');
      });
  }, 'Used => json');

promise_test(t => {
    var res = new Response('');
    res.body.cancel();
    return res.text().then(unreached_fulfillment(t), e => {
        assert_equals(e.name, 'TypeError');
      });
  }, 'Used => text');

promise_test(t => {
    var res = new Response('');
    const reader = res.body.getReader();
    return res.arrayBuffer().then(unreached_fulfillment(t), e => {
        // TODO(yhirano): Use finally when it's available. Ditto below.
        reader.releaseLock();
        assert_equals(e.name, 'TypeError');
      });
  }, 'Locked => arrayBuffer');

promise_test(t => {
    var res = new Response('');
    const reader = res.body.getReader();
    return res.blob().then(unreached_fulfillment(t), e => {
        reader.releaseLock();
        assert_equals(e.name, 'TypeError');
      });
  }, 'Locked => blob');

promise_test(t => {
    var res = new Response('');
    const reader = res.body.getReader();
    return res.formData().then(unreached_fulfillment(t), e => {
        reader.releaseLock();
        assert_equals(e.name, 'TypeError');
      });
  }, 'Locked => formData');

promise_test(t => {
    var res = new Response('');
    const reader = res.body.getReader();
    return res.json().then(unreached_fulfillment(t), e => {
        reader.releaseLock();
        assert_equals(e.name, 'TypeError');
      });
  }, 'Locked => json');

promise_test(t => {
    var res = new Response('');
    const reader = res.body.getReader();
    return res.text().then(unreached_fulfillment(t), e => {
        reader.releaseLock();
        assert_equals(e.name, 'TypeError');
      });
  }, 'Locked => text');

promise_test(t => {
    return fetch('/fetch/resources/slow-failure.cgi').then(response => {
        return response.text().then(unreached_fulfillment(t), e => {
            assert_equals(e.name, 'TypeError');
          });
      });
  }, 'streaming error');

done();
