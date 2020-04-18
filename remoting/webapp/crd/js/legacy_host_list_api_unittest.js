// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * Unit tests for host_controller.js.
 */

(function() {

'use strict';

var FAKE_HOST_ID = '0bad0bad-0bad-0bad-0bad-0bad0bad0bad';
var FAKE_HOST_NAME = '<FAKE_HOST_NAME>';
var FAKE_PUBLIC_KEY = '<FAKE_PUBLIC_KEY>';
var FAKE_HOST_CLIENT_ID = '<FAKE_HOST_CLIENT_ID>';
var FAKE_AUTH_CODE = '<FAKE_AUTH_CODE>';

/** @type {sinon.TestStub} */
var generateUuidStub;

QUnit.module('host_list_api_impl', {
  beforeEach: function(/** QUnit.Assert */ assert) {
    remoting.settings = new remoting.Settings();
    remoting.MockXhr.activate();
    generateUuidStub = sinon.stub(base, 'generateUuid');
    generateUuidStub.returns(FAKE_HOST_ID);
  },
  afterEach: function(/** QUnit.Assert */ assert) {
    generateUuidStub.restore();
    remoting.MockXhr.restore();
    remoting.settings = null;
  }
});

/**
 * Install an HTTP response for requests to the registry.
 * @param {QUnit.Assert} assert
 */
function queueRegistryResponse(assert) {
  var responseJson = {
      data: {
        authorizationCode: FAKE_AUTH_CODE
      }
  };

  remoting.MockXhr.setResponseFor(
      'POST', 'DIRECTORY_API_BASE_URL/@me/hosts',
      function(/** remoting.MockXhr */ xhr) {
        assert.deepEqual(
            xhr.params.jsonContent,
            { data: {
              hostId: FAKE_HOST_ID,
              hostName: FAKE_HOST_NAME,
              publicKey: FAKE_PUBLIC_KEY
            } });
        xhr.setJsonResponse(200, responseJson);
      });
}

QUnit.test('register', function(assert) {
  var impl = new remoting.LegacyHostListApi();
  queueRegistryResponse(assert);
  return impl.register(
      FAKE_HOST_NAME,
      FAKE_PUBLIC_KEY,
      FAKE_HOST_CLIENT_ID
  ).then(function(regResult) {
    assert.equal(regResult.authCode, FAKE_AUTH_CODE);
    assert.equal(regResult.email, '');
  });
});

QUnit.test('register failure', function(assert) {
  var impl = new remoting.LegacyHostListApi();
  remoting.MockXhr.setEmptyResponseFor(
      'POST', 'DIRECTORY_API_BASE_URL/@me/hosts', 500);
  return impl.register(
      FAKE_HOST_NAME,
      FAKE_PUBLIC_KEY,
      FAKE_HOST_CLIENT_ID
  ).then(function(authCode) {
    throw 'test failed';
  }, function(/** remoting.Error */ e) {
    assert.equal(e.getTag(), remoting.Error.Tag.REGISTRATION_FAILED);
  });
});

})();
