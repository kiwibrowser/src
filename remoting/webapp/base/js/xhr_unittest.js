// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 */

(function() {

'use strict';

/** @type {sinon.FakeXhrCtrl} */
var fakeXhrCtrl;

/** @type {sinon.FakeXhr} */
var fakeXhr;

QUnit.module('xhr', {
  beforeEach: function() {
    fakeXhr = null;
    fakeXhrCtrl = sinon.useFakeXMLHttpRequest();
    fakeXhrCtrl.onCreate =
        function(/** sinon.FakeXhr */ xhr) {
          fakeXhr = xhr;
        };
    remoting.identity = new remoting.Identity();
    chromeMocks.identity.mock$setToken('my_token');
  },
  afterEach: function() {
    remoting.identity = null;
    fakeXhrCtrl.restore();
  }
});

QUnit.test('urlencodeParamHash', function(assert) {
  assert.equal(
      remoting.Xhr.urlencodeParamHash({}),
      '');
  assert.equal(
      remoting.Xhr.urlencodeParamHash({'key': 'value'}),
      'key=value');
  assert.equal(
      remoting.Xhr.urlencodeParamHash({'key /?=&': 'value /?=&'}),
      'key%20%2F%3F%3D%26=value%20%2F%3F%3D%26');
  assert.equal(
      remoting.Xhr.urlencodeParamHash({'k1': 'v1', 'k2': 'v2'}),
      'k1=v1&k2=v2');
});

//
// Test for what happens when the parameters are specified
// incorrectly.
//

QUnit.test('invalid *content parameters', function(assert) {
  assert.throws(function() {
    new remoting.Xhr({
      method: 'POST',
      url: 'http://foo.com',
      jsonContent: {},
      formContent: {}
    });
  });

  assert.throws(function() {
    new remoting.Xhr({
      method: 'POST',
      url: 'http://foo.com',
      textContent: '',
      formContent: {}
    });
  });

  assert.throws(function() {
    new remoting.Xhr({
      method: 'POST',
      url: 'http://foo.com',
      textContent: '',
      jsonContent: {}
    });
  });
});


QUnit.test('invalid URL parameters', function(assert) {
  assert.throws(function() {
    new remoting.Xhr({
      method: 'POST',
      url: 'http://foo.com?',
      urlParams: {}
    });
  });

  assert.throws(function() {
    new remoting.Xhr({
      method: 'POST',
      url: 'http://foo.com#',
      urlParams: {}
    });
  });
});

QUnit.test('invalid auth parameters', function(assert) {
  assert.throws(function() {
    new remoting.Xhr({
      method: 'POST',
      url: 'http://foo.com',
      useIdentity: false,
      oauthToken: '',
      headers: {
        'Authorization': ''
      }
    });
  });

  assert.throws(function() {
    new remoting.Xhr({
      method: 'POST',
      url: 'http://foo.com',
      useIdentity: true,
      headers: {
        'Authorization': ''
      }
    });
  });

  assert.throws(function() {
    new remoting.Xhr({
      method: 'POST',
      url: 'http://foo.com',
      useIdentity: true,
      oauthToken: '',
      headers: {}
    });
  });
});

QUnit.test('invalid auth parameters', function(assert) {
  assert.throws(function() {
    new remoting.Xhr({
      method: 'POST',
      url: 'http://foo.com',
      useIdentity: false,
      oauthToken: '',
      headers: {
        'Authorization': ''
      }
    });
  });

  assert.throws(function() {
    new remoting.Xhr({
      method: 'POST',
      url: 'http://foo.com',
      useIdentity: true,
      headers: {
        'Authorization': ''
      }
    });
  });

  assert.throws(function() {
    new remoting.Xhr({
      method: 'POST',
      url: 'http://foo.com',
      useIdentity: true,
      oauthToken: '',
      headers: {}
    });
  });
});


QUnit.test('unexpected parameters', function(assert) {
  assert.throws(function() {
    new remoting.Xhr({
      method: 'POST',
      url: 'http://foo.com',
      xyzzy: 'not a real parameter'
    });
  });
});

//
// The typical case.
//

QUnit.test('successful GET', function(assert) {
  var promise = new remoting.Xhr({
    method: 'GET',
    url: 'http://foo.com'
  }).start().then(function(response) {
    assert.ok(!response.isError());
    assert.equal(response.status, 200);
    assert.equal(response.getText(), 'body');
  });
  assert.equal(fakeXhr.method, 'GET');
  assert.equal(fakeXhr.url, 'http://foo.com');
  assert.equal(fakeXhr.withCredentials, false);
  assert.equal(fakeXhr.requestBody, null);
  assert.ok(!('Content-type' in fakeXhr.requestHeaders));
  fakeXhr.respond(200, {}, 'body');
  return promise;
});

//
// Tests for the effect of acceptJson.
//

QUnit.test('acceptJson required', function(assert) {
  var promise = new remoting.Xhr({
    method: 'GET',
    url: 'http://foo.com'
  }).start().then(function(response) {
    assert.throws(response.getJson);
  });
  fakeXhr.respond(200, {}, '{}');
  return promise;
});

QUnit.test('JSON response', function(assert) {
  var responseJson = {
      'myJsonData': [true]
  };
  var responseText = JSON.stringify(responseJson);
  var promise = new remoting.Xhr({
    method: 'GET',
    url: 'http://foo.com',
    acceptJson: true
  }).start().then(function(response) {
    // Calling getText is still OK even when a JSON response is
    // requested.
    assert.equal(
        response.getText(),
        responseText);
    // Check that getJson works as advertised.
    assert.deepEqual(
        response.getJson(),
        responseJson);
    // Calling getJson multiple times doesn't re-parse the response.
    assert.strictEqual(
        response.getJson(),
        response.getJson());
  });
  fakeXhr.respond(200, {}, responseText);
  return promise;
});

//
// Tests for various parameters that modify the HTTP request.
//

QUnit.test('GET with param string', function(assert) {
  new remoting.Xhr({
    method: 'GET',
    url: 'http://foo.com',
    urlParams: 'the_param_string'
  }).start();
  assert.equal(fakeXhr.url, 'http://foo.com?the_param_string');
});

QUnit.test('GET with param object', function(assert) {
  new remoting.Xhr({
    method: 'GET',
    url: 'http://foo.com',
    urlParams: {'a': 'b', 'c': 'd'}
  }).start();
  assert.equal(fakeXhr.url, 'http://foo.com?a=b&c=d');
});

QUnit.test('GET with headers', function(assert) {
  new remoting.Xhr({
    method: 'GET',
    url: 'http://foo.com',
    headers: {'Header1': 'headerValue1', 'Header2': 'headerValue2'}
  }).start();
  assert.equal(
      fakeXhr.requestHeaders['Header1'],
      'headerValue1');
  assert.equal(
      fakeXhr.requestHeaders['Header2'],
      'headerValue2');
});


QUnit.test('GET with credentials', function(assert) {
  new remoting.Xhr({
    method: 'GET',
    url: 'http://foo.com',
    withCredentials: true
  }).start();
  assert.equal(fakeXhr.withCredentials, true);
});

//
// Checking that typical POST requests work.
//

QUnit.test('POST with text content', function(assert) {
  var done = assert.async();

  var promise = new remoting.Xhr({
    method: 'POST',
    url: 'http://foo.com',
    textContent: 'the_content_string'
  }).start().then(function(response) {
    assert.equal(response.status, 200);
    assert.equal(response.getText(), 'body');
    done();
  });
  assert.equal(fakeXhr.method, 'POST');
  assert.equal(fakeXhr.url, 'http://foo.com');
  assert.equal(fakeXhr.withCredentials, false);
  assert.equal(fakeXhr.requestBody, 'the_content_string');
  assert.equal(fakeXhr.requestHeaders['Content-type'],
               'text/plain; charset=UTF-8');
  fakeXhr.respond(200, {}, 'body');
  return promise;
});

QUnit.test('POST with form content', function(assert) {
  new remoting.Xhr({
    method: 'POST',
    url: 'http://foo.com',
    formContent: {'a': 'b', 'c': 'd'}
  }).start();
  assert.equal(fakeXhr.requestBody, 'a=b&c=d');
  assert.equal(
      fakeXhr.requestHeaders['Content-type'],
      'application/x-www-form-urlencoded; charset=UTF-8');
});

//
// Tests for authentication-related options.
//

QUnit.test('GET with auth token', function(assert) {
  new remoting.Xhr({
    method: 'GET',
    url: 'http://foo.com',
    oauthToken: 'my_token'
  }).start();
  assert.equal(fakeXhr.requestHeaders['Authorization'],
              'Bearer my_token');
});

QUnit.test('GET with useIdentity', function(assert) {
  var xhr = new remoting.Xhr({
    method: 'GET',
    url: 'http://foo.com',
    useIdentity: true
  });

  xhr.start();
  var done = assert.async();
  fakeXhr.addEventListener('loadstart', function() {
    assert.equal(fakeXhr.requestHeaders['Authorization'],
                 'Bearer my_token');
    done();
  });
});

//
// Error responses.
//
QUnit.test('GET with error response', function(assert) {
  var promise = new remoting.Xhr({
    method: 'GET',
    url: 'http://foo.com'
  }).start().then(function(response) {
    assert.ok(response.isError());
    assert.equal(response.status, 500);
    assert.equal(response.getText(), 'body');
  });
  fakeXhr.respond(500, {}, 'body');
  return promise;
});

QUnit.test('204 is not an error', function(assert) {
  var promise = new remoting.Xhr({
    method: 'GET',
    url: 'http://foo.com'
  }).start().then(function(response) {
    assert.ok(!response.isError());
    assert.equal(response.status, 204);
    assert.equal(response.getText(), '');
  });
  fakeXhr.respond(204, {}, null);
  return promise;
});

QUnit.test('GET with non-HTTP response', function(assert) {
  var promise = new remoting.Xhr({
    method: 'GET',
    url: 'http://foo.com'
  }).start().then(function(response) {
    assert.ok(response.isError());
  });
  fakeXhr.respond(0, {}, null);
  return promise;
});

QUnit.module('AutoRetryXhr', {
  beforeEach: function() {
    fakeXhr = null;
    fakeXhrCtrl = sinon.useFakeXMLHttpRequest();
    fakeXhrCtrl.onCreate = function(/** sinon.FakeXhr */ xhr) {
      fakeXhr = xhr;
    };
  },
  afterEach: function() {
    fakeXhrCtrl.restore();
  }
});

/**
 * A class allows you to specify a sequence of canned responses, to be returned
 * in order in response to XHRs.
 *
 * @param {Array<Array>} cannedResponses
 * @constructor
 */
var CannedXhrResponder = function(cannedResponses) {
  /** @private */
  this.mockXhr_ = sinon.useFakeXMLHttpRequest();
  this.cannedResponses_ = cannedResponses;
  this.mockXhr_.onCreate = this.onCreate_.bind(this);
};

/** @private */
CannedXhrResponder.prototype.onCreate_ = function(/** sinon.FakeXhr */ xhr) {
  var that = this;
  var response = that.cannedResponses_.shift();
  Promise.resolve().then(function() {
    xhr.respond.apply(xhr, response);
  });
};

CannedXhrResponder.prototype.dispose = function() {
  this.mockXhr_.restore();
};

QUnit.test('retries on status code equals 0',
  /**
   * 'this' is not defined for jscompile, so it can't figure out the type of
   * this.clock.
   * @suppress {reportUnknownTypes|checkVars|checkTypes}
   */
  function(assert) {
    this.clock.restore();
    fakeXhrCtrl.restore();
    var responder = new CannedXhrResponder([
      [0, {}, null],
      [0, {}, null],
      [200, {}, 'body']
    ]);
    var promise = new remoting.AutoRetryXhr({
      method: 'GET',
      url: 'http://foo.com'
    }).start().then(function(response) {
      assert.ok(!response.isError());
    }).catch(function(){
      throw new Error('Expect retry to succeed.');
    });
    return promise;
});

QUnit.test('respects opt_maxRetryAttempts',
  /**
   * 'this' is not defined for jscompile, so it can't figure out the type of
   * this.clock.
   * @suppress {reportUnknownTypes|checkVars|checkTypes}
   */
  function(assert) {
    this.clock.restore();
    fakeXhrCtrl.restore();
    var responder = new CannedXhrResponder([
      [0, {}, null],
      [0, {}, null],
      [0, {}, null],
      [200, {}, 'body']
    ]);
    var promise = new remoting.AutoRetryXhr({
      method: 'GET',
      url: 'http://foo.com'
    }, 2).start().then(function(response) {
      throw new Error('Expect retry to fail.');
    }).catch(function(){
      assert.ok(true);
    });
    return promise;
});

QUnit.test('does not retry when offline', function(assert) {
  var isOnlineStub = sinon.stub(base, 'isOnline');
  isOnlineStub.returns(false);

  var promise = new remoting.AutoRetryXhr({
    method: 'GET',
    url: 'http://foo.com'
  }).start().then(function(response) {
    assert.ok(false, 'Expect failure');
  }).catch(function(/** remoting.Error */ error) {
    assert.equal(error.getTag(), remoting.Error.Tag.NETWORK_FAILURE);
  });
  isOnlineStub.restore();
  return promise;
});

QUnit.test('resolves with successful responses', function(assert) {
  var promise = new remoting.AutoRetryXhr({
    method: 'GET',
    url: 'http://foo.com'
  }).start().then(function(response) {
    assert.ok(!response.isError());
    assert.equal(response.getText(), 'body');
  });
  fakeXhr.respond(200, {}, 'body');
  return promise;
});

QUnit.test('rejects with failed responses', function(assert) {
  var promise = new remoting.AutoRetryXhr({
    method: 'GET',
    url: 'http://foo.com'
  }).start().then(function(response) {
    assert.ok(response.isError());
    assert.equal(response.getText(), 'failure');
  });
  fakeXhr.respond(500, {}, 'failure');
  return promise;
});

})();
