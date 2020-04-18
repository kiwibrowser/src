// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 */

(function() {

'use strict';

/** @type {sinon.TestStub} */
var mockIsAppsV2 = null;
var mockChromeStorage = {};

/**
 * @param {string} v1UserName
 * @param {string} v1UserEmail
 * @param {boolean} v1HasHosts
 */
function setMigrationData_(v1UserName, v1UserEmail, v1HasHosts) {
  /** @return {!Promise} */
  remoting.identity.getUserInfo = function() {
    if (base.isAppsV2()) {
      return Promise.resolve(
          {email: 'v2user@gmail.com', name: 'v2userName'});
    } else {
      return Promise.resolve(
          {email: v1UserEmail, name: v1UserName});
    }
  };
  /** @return {!Promise} */
  remoting.identity.getEmail = function() {
    return remoting.identity.getUserInfo().then(
        /** @param {{email:string, name:string}} info */
        function(info) {
          return info.email;
        });
  };

  mockIsAppsV2.returns(false);
  if (v1HasHosts) {
    remoting.AppsV2Migration.saveUserInfo();
  }
}

QUnit.module('AppsV2Migration', {
  beforeEach: function() {
    mockIsAppsV2 = sinon.stub(base, 'isAppsV2');
    remoting.identity = new remoting.Identity();
  },
  afterEach: function() {
    mockIsAppsV2.restore();
    remoting.identity = null;
  }
});

QUnit.test(
  'hasHostsInV1App() should reject the promise if v1 user has same identity',
  function(assert) {
    assert.expect(0);
    setMigrationData_('v1userName', 'v2user@gmail.com', true);
    mockIsAppsV2.returns(true);
    return base.Promise.negate(remoting.AppsV2Migration.hasHostsInV1App());
});

QUnit.test(
  'hasHostsInV1App() should reject the promise if v1 user has no hosts',
  function(assert) {
    assert.expect(0);
    setMigrationData_('v1userName', 'v1user@gmail.com', false);
    mockIsAppsV2.returns(true);
    return base.Promise.negate(remoting.AppsV2Migration.hasHostsInV1App());
});

QUnit.test('hasHostsInV1App() should reject the promise in v1',
    function(assert) {
  assert.expect(0);
  setMigrationData_('v1userName', 'v1user@gmail.com', true);
  mockIsAppsV2.returns(false);
  return base.Promise.negate(remoting.AppsV2Migration.hasHostsInV1App());
});

QUnit.test(
  'saveUserInfo() should clear the preferences on v2',
  function(assert) {
    assert.expect(0);
    setMigrationData_('v1userName', 'v1user@gmail.com', true);
    mockIsAppsV2.returns(true);
    remoting.AppsV2Migration.saveUserInfo();
    return base.Promise.negate(remoting.AppsV2Migration.hasHostsInV1App());
});

})();
