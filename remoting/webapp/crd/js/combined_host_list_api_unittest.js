// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * Unit tests for combined_host_list_api.js.
 */

(function() {

'use strict';

/** @type {!remoting.MockHostListApi} */
var mockGcdApi;

/** @type {!remoting.MockHostListApi} */
var mockLegacyApi;

/** @type {!remoting.CombinedHostListApi} */
var combinedApi;

/** @type {sinon.TestStub} */
var registerWithHostIdStub;

/** @type {!remoting.Host} */
var commonHostGcd;

/** @type {!remoting.Host} */
var commonHostLegacy;

QUnit.module('CombinedHostListApi', {
  beforeEach: function(/** QUnit.Assert */ assert) {
    remoting.settings = new remoting.Settings();
    remoting.settings['USE_GCD'] = true;
    remoting.mockIdentity.setAccessToken(
        remoting.MockIdentity.AccessToken.VALID);
    mockGcdApi = new remoting.MockHostListApi();
    mockGcdApi.addMockHost('gcd-host');
    commonHostGcd = mockGcdApi.addMockHost('common-host');
    commonHostGcd.hostName = 'common-host-gcd';
    mockLegacyApi = new remoting.MockHostListApi();
    mockLegacyApi.addMockHost('legacy-host');
    commonHostLegacy = mockLegacyApi.addMockHost('common-host');
    commonHostLegacy.hostName = 'common-host-legacy';
    combinedApi = new remoting.CombinedHostListApi(mockLegacyApi, mockGcdApi);
    registerWithHostIdStub =
        sinon.stub(remoting.LegacyHostListApi, 'registerWithHostId');
  },
  afterEach: function(/** QUnit.Assert */ assert) {
    remoting.settings = null;
    registerWithHostIdStub.restore();
  }
});

QUnit.test('register', function(/** QUnit.Assert */ assert) {
  registerWithHostIdStub.returns(Promise.resolve());

  mockGcdApi.authCodeFromRegister = '<fake_auth_code>';
  mockGcdApi.emailFromRegister = '<fake_email>';
  mockGcdApi.hostIdFromRegister = '<fake_host_id>';
  mockLegacyApi.authCodeFromRegister = '<wrong_fake_auth_code>';
  mockLegacyApi.emailFromRegister = '<wrong_fake_email>';
  mockLegacyApi.hostIdFromRegister = '<wrong_fake_host_id>';
  return combinedApi.register('', '', '').then(function(regResult) {
    assert.equal(regResult.authCode, '<fake_auth_code>');
    assert.equal(regResult.email, '<fake_email>');
    assert.equal(regResult.hostId, '<fake_host_id>');
  });
});

QUnit.test('get', function(/** QUnit.Assert */ assert) {
  return combinedApi.get().then(function(hosts) {
    assert.equal(hosts.length, 3);
    var hostIds = new Set();
    hosts.forEach(function(host) {
      hostIds.add(host.hostId);
      if (host.hostId == 'common-host') {
        assert.equal(host.hostName, 'common-host-gcd');
      };
    });
    assert.ok(hostIds.has('gcd-host'));
    assert.ok(hostIds.has('legacy-host'));
    assert.ok(hostIds.has('common-host'));
  });
});

QUnit.test('get w/ GCD newer', function(/** QUnit.Assert */ assert) {
  commonHostGcd.updatedTime = '1970-01-02';
  commonHostLegacy.updatedTime = '1970-01-01';
  return combinedApi.get().then(function(hosts) {
    hosts.forEach(function(host) {
      if (host.hostId == 'common-host') {
        assert.equal(host.hostName, 'common-host-gcd');
      };
    });
  });
});

QUnit.test('get w/ legacy newer', function(/** QUnit.Assert */ assert) {
  commonHostGcd.updatedTime = '1970-01-01';
  commonHostLegacy.updatedTime = '1970-01-02';
  return combinedApi.get().then(function(hosts) {
    hosts.forEach(function(host) {
      if (host.hostId == 'common-host') {
        assert.equal(host.hostName, 'common-host-legacy');
      };
    });
  });
});

QUnit.test('put to legacy', function(/** QUnit.Assert */ assert) {
  return combinedApi.get().then(function() {
    return combinedApi.put('legacy-host', 'new host name', '').then(
        function() {
          assert.equal(mockLegacyApi.hosts[0].hostName,
                       'new host name');
        });
  });
});

QUnit.test('put to GCD', function(/** QUnit.Assert */ assert) {
  return combinedApi.get().then(function() {
    return combinedApi.put('gcd-host', 'new host name', '').then(
        function() {
          assert.equal(mockGcdApi.hosts[0].hostName,
                       'new host name');
        });
  });
});


QUnit.test('put to both', function(/** QUnit.Assert */ assert) {
  return combinedApi.get().then(function() {
    return combinedApi.put('common-host', 'new host name', '').then(
        function() {
          assert.equal(mockGcdApi.hosts[1].hostName,
                       'new host name');
          assert.equal(mockLegacyApi.hosts[1].hostName,
                       'new host name');
        });
  });
});

QUnit.test('remove from legacy', function(/** QUnit.Assert */ assert) {
  return combinedApi.get().then(function() {
    return combinedApi.remove('legacy-host').then(function() {
      assert.equal(mockGcdApi.hosts.length, 2);
      assert.equal(mockLegacyApi.hosts.length, 1);
      assert.notEqual(mockLegacyApi.hosts[0].hostId, 'legacy-host');
    });
  });
});

QUnit.test('remove from gcd', function(/** QUnit.Assert */ assert) {
  return combinedApi.get().then(function() {
    return combinedApi.remove('gcd-host').then(function() {
      assert.equal(mockLegacyApi.hosts.length, 2);
      assert.equal(mockGcdApi.hosts.length, 1);
      assert.notEqual(mockGcdApi.hosts[0].hostId, 'gcd-host');
    });
  });
});

QUnit.test('remove from both', function(/** QUnit.Assert */ assert) {
  return combinedApi.get().then(function() {
    return combinedApi.remove('common-host').then(function() {
      assert.equal(mockGcdApi.hosts.length, 1);
      assert.equal(mockLegacyApi.hosts.length, 1);
      assert.notEqual(mockGcdApi.hosts[0].hostId, 'common-host');
      assert.notEqual(mockLegacyApi.hosts[0].hostId, 'common-host');
    });
  });
});

})();