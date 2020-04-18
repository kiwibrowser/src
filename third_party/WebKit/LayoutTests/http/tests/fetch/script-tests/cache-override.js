if (self.importScripts) {
  importScripts('../resources/fetch-test-helpers.js');
}

promise_test(function() {
    var lastModified = '';
    var eTag = '';
    var url = '../resources/doctype.html';
    var expectedText = '<!DOCTYPE html>\n';
    return fetch(url)
      .then(function(res) {
          lastModified = res.headers.get('last-modified');
          eTag = res.headers.get('etag');
          assert_not_equals(lastModified, '', 'last-modified must be set.');
          assert_not_equals(eTag, '', 'eTag must be set.');

          return fetch(url);
        })
      .then(function(res) {
          assert_equals(res.status, 200,
                        'Automatically cached response status must be 200.');
          return res.text();
        })
      .then(function(text) {
          assert_equals(
            text, expectedText,
            'Automatically cached response body must be correct.');

          return fetch(url,
                       { headers: [['If-Modified-Since', lastModified]] });
        })
      .then(function(res) {
          assert_equals(
            res.status, 304,
            'When If-Modified-Since is overridden, the response status must ' +
            'be 304.');
          return res.text();
        })
      .then(function(text) {
          assert_equals(
            text, '',
            'When If-Modified-Since is overridden, the response body must be' +
            ' empty.');

          return fetch(url,
                       { headers: [['If-Modified-Since',
                                    'Tue, 01 Jan 1980 01:00:00 GMT']] });
        })
      .then(function(res) {
          assert_equals(
            res.status, 200,
            'When If-Modified-Since is overridden, the modified response ' +
            'status must be 200.');
          return res.text();
        })
      .then(function(text) {
          assert_equals(
            text, expectedText,
            'When If-Modified-Since is overridden, the modified response body' +
            ' must be correct.');

          return fetch(url,
                       { headers: [['If-Unmodified-Since', lastModified]] });
        })
      .then(function(res) {
          assert_equals(
            res.status, 200,
            'When If-Unmodified-Since is overridden, the modified response ' +
            'status must be 200.');
          return res.text();
        })
      .then(function(text) {
          assert_equals(
            text, expectedText,
            'When If-Unmodified-Since is overridden, the modified response ' +
            'body must be correct.');

          return fetch(url,
                       { headers: [['If-Unmodified-Since',
                                    'Tue, 01 Jan 1980 01:00:00 GMT']] });
        })
      .then(function(res) {
          assert_equals(
            res.status, 412,
            'When If-Unmodified is overridden, the modified response status ' +
            'must be 412.');
          return res.text();
        })
      .then(function(text) {
          assert_equals(
            text, '',
            'When If-Unmodified is overridden, the modified response body ' +
            'must be empty.');

          return fetch(url,
                       { headers: [['If-Match', eTag]] });
        })
      .then(function(res) {
          assert_equals(
            res.status, 200,
            'When If-Match is overridden, the response status must be 200.');
          return res.text();
        })
      .then(function(text) {
          assert_equals(
            text, expectedText,
            'When If-Match is overridden, the response body must be correct.');

          // FIXME: We used to have a test of If-Match overridden with an
          // invalid etag, but removed due to broken If-Match handling of
          // Apache 2.4. See crbug.com/423070

          return fetch(url,
                       { headers: [['If-None-Match', eTag]] });
        })
      .then(function(res) {
          assert_equals(
            res.status, 304,
            'When If-None-Match is overridden, the response status must be ' +
            '304.');
          return res.text();
        })
      .then(function(text) {
          assert_equals(
            text, '',
            'When If-None-Match is overridden, the response body must be ' +
            'empty.');

          return fetch(url,
                       { headers: [['If-None-Match', 'xyzzy']] });
        })
      .then(function(res) {
          assert_equals(
            res.status, 200,
            'When If-None-Match is overridden to the invalid tag, the ' +
            'response status must be 200.');
          return res.text();
        })
      .then(function(text) {
          assert_equals(
            text, expectedText,
            'When If-None-Match is overridden to the invalid tag, the ' +
            'response body must be correct.');

          return fetch(url,
                       { headers: [['If-Range', eTag],
                                   ['Range', 'bytes=10-30']] });
        })
      .then(function(res) {
          assert_equals(
            res.status, 206,
            'When If-Range is overridden, the response status must be 206.');
          return res.text();
        })
      .then(function(text) {
          assert_equals(
            text, expectedText.substring(10, 31),
            'When If-Range is overridden, the response body must be correct.');

          return fetch(url,
                       { headers: [['If-Range', 'xyzzy'],
                                   ['Range', 'bytes=10-30']] });
        })
      .then(function(res) {
          assert_equals(
            res.status, 200,
            'When If-Range is overridden to the invalid tag, the response ' +
            'status must be 200.');
          return res.text();
        })
      .then(function(text) {
          assert_equals(
            text, expectedText,
            'When If-Range is overridden to the invalid tag, the response ' +
            'body must be correct.');

          return fetch('../resources/fetch-status.php?status=304');
        })
      .then(function(res) {
          assert_equals(
            res.status, 304 ,
            'When the server returns 304 and there\'s a cache miss, the ' +
            'response status must be 304.');
        });
  }, '304 handling for fetch().');

done();
