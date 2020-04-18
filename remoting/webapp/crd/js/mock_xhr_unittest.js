// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {
'use strict';

QUnit.module('mock_xhr', {
  beforeEach: function() {
    remoting.MockXhr.activate();
  },
  afterEach: function() {
    remoting.MockXhr.restore();
  }
});

/**
 * @param {string=} opt_url The URL to request.
 * @return {!Promise<!remoting.Xhr.Response>}
 */
function sendRequest(opt_url) {
  return new remoting.Xhr({
    method: 'GET',
    url: opt_url || 'http://foo.com'
  }).start();
};

/**
 * @return {!Promise<!remoting.Xhr.Response>}
 */
function sendRequestForJson() {
  return new remoting.Xhr({
    method: 'GET',
    url: 'http://foo.com',
    acceptJson: true
  }).start();
}

QUnit.test('unhandled requests fail', function(assert) {
  assert.throws(sendRequest);
});

QUnit.test('setEmptyResponseFor', function(assert) {
  remoting.MockXhr.setEmptyResponseFor('GET', 'http://foo.com');
  var promise = sendRequest();
  assert.throws(sendRequest);
  return promise.then(function(/** remoting.Xhr.Response */ result) {
    assert.equal(result.status, 204);
    assert.equal(result.getText(), '');
    assert.throws(result.getJson.bind(result));
  });
});

QUnit.test('setEmptyResponseFor with repeat', function(assert) {
  remoting.MockXhr.setEmptyResponseFor('GET', 'http://foo.com', 404, true);
  var promise1 = sendRequest();
  var promise2 = sendRequest();
  return promise1.then(function(/** remoting.Xhr.Response */ result) {
    assert.equal(result.status, 404);
    assert.equal(result.getText(), '');
    assert.throws(result.getJson.bind(result));
    return promise2;
  }).then(function(/** remoting.Xhr.Response */ result) {
    assert.equal(result.status, 404);
    assert.equal(result.getText(), '');
    assert.throws(result.getJson.bind(result));
  });
});

QUnit.test('setEmptyResponseFor with RegExp', function(assert) {
  remoting.MockXhr.setEmptyResponseFor('GET', /foo/);
  var promise = sendRequest();
  assert.throws(sendRequest);
  return promise.then(function(/** remoting.Xhr.Response */ result) {
    assert.equal(result.status, 204);
    assert.equal(result.getText(), '');
    assert.throws(result.getJson.bind(result));
  });
});

QUnit.test('setTextResponseFor', function(assert) {
  remoting.MockXhr.setTextResponseFor('GET', /foo/, 'first');
  remoting.MockXhr.setTextResponseFor('GET', /foo/, 'second');
  var promise1 = sendRequest();
  var promise2 = sendRequest();
  return promise1.then(function(/** remoting.Xhr.Response */ result) {
    assert.equal(result.status, 200);
    assert.equal(result.getText(), 'first');
    assert.throws(result.getJson.bind(result));
    return promise2;
  }).then(function(/** remoting.Xhr.Response */ result) {
    assert.equal(result.status, 200);
    assert.equal(result.getText(), 'second');
    assert.throws(result.getJson.bind(result));
  });
});

QUnit.test('setTextResponseFor with different URLs', function(assert) {
  remoting.MockXhr.setTextResponseFor('GET', /foo/, 'first');
  remoting.MockXhr.setTextResponseFor('GET', /bar/, 'second');
  var promise1 = sendRequest('http://bar.com');
  var promise2 = sendRequest();
  return promise1.then(function(/** remoting.Xhr.Response */ result) {
    assert.equal(result.status, 200);
    assert.equal(result.getText(), 'second');
    assert.throws(result.getJson.bind(result));
    return promise2;
  }).then(function(/** remoting.Xhr.Response */ result) {
    assert.equal(result.status, 200);
    assert.equal(result.getText(), 'first');
    assert.throws(result.getJson.bind(result));
  });
});


QUnit.test('setTextResponseFor with default', function(assert) {
  remoting.MockXhr.setTextResponseFor('GET', /foo/, 'specific');
  remoting.MockXhr.setTextResponseFor('GET', null, 'default', true);
  var promise1 = sendRequest('http://bar.com');
  var promise2 = sendRequest();
  var promise3 = sendRequest();
  return promise1.then(function(/** remoting.Xhr.Response */ result) {
    assert.equal(result.status, 200);
    assert.equal(result.getText(), 'default');
    assert.throws(result.getJson.bind(result));
    return promise2;
  }).then(function(/** remoting.Xhr.Response */ result) {
    assert.equal(result.status, 200);
    assert.equal(result.getText(), 'specific');
    assert.throws(result.getJson.bind(result));
    return promise3;
  }).then(function(/** remoting.Xhr.Response */ result) {
    assert.equal(result.status, 200);
    assert.equal(result.getText(), 'default');
    assert.throws(result.getJson.bind(result));
  });
});


QUnit.test('setJsonResponseFor', function(assert) {
  remoting.MockXhr.setJsonResponseFor('GET', 'http://foo.com', 'foo');
  var promise = sendRequestForJson();
  assert.throws(sendRequestForJson);
  return promise.then(function(/** remoting.Xhr.Response */ result) {
    assert.equal(result.status, 200);
    assert.equal(JSON.parse(result.getText()), 'foo');
    assert.equal(result.getJson(), 'foo');
  });
});

})();
