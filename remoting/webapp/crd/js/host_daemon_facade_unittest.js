// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * Unit test for host_daemon_facade.js.
 */

/** @type {chromeMocks.runtime.Port} */
var nativePortMock;

(function() {
'use strict';

/** @type {sinon.TestStub} */
var postMessageStub;

/** @type {Array<Object>} */
var mockHostResponses;

/** @type {remoting.HostDaemonFacade} */
var it;

QUnit.module('host_daemon_facade', {
  beforeEach: function(/** QUnit.Assert */ assert) {
    chromeMocks.identity.mock$setToken('my_token');
    nativePortMock =
        chromeMocks.runtime.connectNative('com.google.chrome.remote_desktop');
    mockHostResponses = [];
    postMessageStub = sinon.stub(
        nativePortMock, 'postMessage', sendMockHostResponse);
  },
  afterEach: function(/** QUnit.Assert */ assert) {
    if (mockHostResponses.length) {
      throw new Error('responses not all used');
    }
    mockHostResponses = null;
    postMessageStub.restore();
    it = null;
  }
});

function sendMockHostResponse() {
  if (mockHostResponses.length == 0) {
    throw new Error('don\'t know how to responsd');
  }
  var toSend = mockHostResponses.pop();
  Promise.resolve().then(function() {
    nativePortMock.onMessage.mock$fire(toSend);
  });
}

QUnit.test('initialize/hasFeature true', function(assert) {
  mockHostResponses.push({
    id: 0,
    type: 'helloResponse',
    version: '',
    supportedFeatures: [
      remoting.HostController.Feature.OAUTH_CLIENT,
      remoting.HostController.Feature.PAIRING_REGISTRY
    ]
  });
  it = new remoting.HostDaemonFacade();
  assert.deepEqual(postMessageStub.args[0][0], {
    id: 0,
    type: 'hello'
  });
  return it.hasFeature(remoting.HostController.Feature.PAIRING_REGISTRY).
      then(function(arg) {
        assert.equal(arg, true);
      });
});

QUnit.test('initialize/hasFeature false', function(assert) {
  mockHostResponses.push({
    id: 0,
    type: 'helloResponse',
    version: '',
    supportedFeatures: []
  });
  it = new remoting.HostDaemonFacade();
  assert.deepEqual(postMessageStub.args[0][0], {
    id: 0,
    type: 'hello'
  });
  return it.hasFeature(remoting.HostController.Feature.PAIRING_REGISTRY).
      then(function(arg) {
        assert.equal(arg, false);
      });
});

QUnit.test('initialize/getDaemonVersion', function(assert) {
  mockHostResponses.push({
    id: 0,
    type: 'helloResponse',
    version: '<daemonVersion>',
    supportedFeatures: []
  });
  it = new remoting.HostDaemonFacade();
  assert.deepEqual(postMessageStub.args[0][0], {
    id: 0,
    type: 'hello'
  });
  return it.getDaemonVersion().
      then(function onDone(arg) {
        assert.equal(arg, '<daemonVersion>');
      });
});

/**
 * @param {string} description
 * @param {function(!QUnit.Assert):*} callback
 */
function postInitTest(description, callback) {
  QUnit.test(description, function(assert) {
    mockHostResponses.push({
      id: 0,
      type: 'helloResponse',
      version: ''
    });
    console.assert(it == null, 'Daemon facade already exists.');
    it = new remoting.HostDaemonFacade();
    assert.deepEqual(postMessageStub.args[0][0], {
      id: 0,
      type: 'hello'
    });
    return it.getDaemonVersion().then(function() {
      return callback(assert);
    });
  });
}

postInitTest('getHostName', function(assert) {
  mockHostResponses.push({
    id: 1,
    type: 'getHostNameResponse',
    hostname: '<fakeHostName>'
  });
  return it.getHostName().then(function(hostName) {
    assert.deepEqual(postMessageStub.args[1][0], {
      id: 1,
      type: 'getHostName'
    });
    assert.equal(hostName, '<fakeHostName>');
  });
});

postInitTest('getPinHash', function(assert) {
  mockHostResponses.push({
    id: 1,
    type: 'getPinHashResponse',
    hash: '<fakePinHash>'
  });
  return it.getPinHash('<hostId>', '<pin>').then(function(hostName) {
    assert.deepEqual(postMessageStub.args[1][0], {
      id: 1,
      type: 'getPinHash',
      hostId: '<hostId>',
      pin: '<pin>'
    });
    assert.equal(hostName, '<fakePinHash>');
  });
});

postInitTest('generateKeyPair', function(assert) {
  mockHostResponses.push({
    id: 1,
    type: 'generateKeyPairResponse',
    privateKey: '<fakePrivateKey>',
    publicKey: '<fakePublicKey>'
  });
  return it.generateKeyPair().then(function(pair) {
    assert.deepEqual(postMessageStub.args[1][0], {
      id: 1,
      type: 'generateKeyPair'
    });
    assert.deepEqual(pair, {
      privateKey: '<fakePrivateKey>',
      publicKey: '<fakePublicKey>'
    });
  });
});

postInitTest('updateDaemonConfig', function(assert) {
  mockHostResponses.push({
    id: 1,
    type: 'updateDaemonConfigResponse',
    result: 'OK'
  });
  return it.updateDaemonConfig({
    fakeDaemonConfig: true
  }).then(function(result) {
    assert.deepEqual(postMessageStub.args[1][0], {
      id: 1,
      type: 'updateDaemonConfig',
      config: { fakeDaemonConfig: true }
    });
    assert.equal(result, remoting.HostController.AsyncResult.OK);
  });
});

postInitTest('getDaemonConfig', function(assert) {
  mockHostResponses.push({
    id: 1,
    type: 'getDaemonConfigResponse',
    config: { fakeDaemonConfig: true }
  });
  return it.getDaemonConfig().then(function(result) {
    assert.deepEqual(postMessageStub.args[1][0], {
      id: 1,
      type: 'getDaemonConfig'
    });
    assert.deepEqual(result, { fakeDaemonConfig: true });
  });
});

[0,1,2,3,4,5,6,7].forEach(function(/** number */ flags) {
  postInitTest('getUsageStatsConsent, flags=' + flags, function(assert) {
    var supported = Boolean(flags & 1);
    var allowed = Boolean(flags & 2);
    var setByPolicy = Boolean(flags & 4);
    mockHostResponses.push({
      id: 1,
      type: 'getUsageStatsConsentResponse',
      supported: supported,
      allowed: allowed,
      setByPolicy: setByPolicy
    });
    return it.getUsageStatsConsent().then(function(result) {
      assert.deepEqual(postMessageStub.args[1][0], {
        id: 1,
        type: 'getUsageStatsConsent'
      });
      assert.deepEqual(result, {
        supported: supported,
        allowed: allowed,
        setByPolicy: setByPolicy
      });
    });
  });
});

[false, true].forEach(function(/** boolean */ consent) {
  postInitTest('startDaemon, consent=' + consent, function(assert) {
    mockHostResponses.push({
      id: 1,
      type: 'startDaemonResponse',
      result: 'FAILED'
    });
    return it.startDaemon({
      fakeConfig: true
    }, consent).then(function(result) {
      assert.deepEqual(postMessageStub.args[1][0], {
        id: 1,
        type: 'startDaemon',
        config: { fakeConfig: true },
        consent: consent
      });
      assert.equal(result, remoting.HostController.AsyncResult.FAILED);
    });
  });
});

postInitTest('stopDaemon', function(assert) {
  mockHostResponses.push({
    id: 1,
    type: 'stopDaemonResponse',
    result: 'CANCELLED'
  });
  return it.stopDaemon().then(function(result) {
    assert.deepEqual(postMessageStub.args[1][0], {
      id: 1,
      type: 'stopDaemon'
    });
    assert.equal(result, remoting.HostController.AsyncResult.CANCELLED);
  });
});

postInitTest('getPairedClients', function(assert) {
  /**
   * @param {number} n
   * @return {remoting.PairedClient}
   */
  function makeClient(n) {
    return /** @type {remoting.PairedClient} */ ({
      clientId: '<fakeClientId' + n + '>',
      clientName: '<fakeClientName' + n + '>',
      createdTime: n * 316571  // random prime number
    });
  };

  var client0 = makeClient(0);
  var client1 = makeClient(1);
  mockHostResponses.push({
    id: 1,
    type: 'getPairedClientsResponse',
    pairedClients: [client0, client1]
  });
  return it.getPairedClients().then(function(result) {
    assert.deepEqual(postMessageStub.args[1][0], {
      id: 1,
      type: 'getPairedClients'
    });
    // Our facade is not really a facade!  It adds extra fields.
    // TODO(jrw): Move non-facade logic to host_controller.js.
    assert.equal(result.length, 2);
    assert.equal(result[0].clientId, '<fakeClientId0>');
    assert.equal(result[0].clientName, '<fakeClientName0>');
    assert.equal(result[0].createdTime, client0.createdTime);
    assert.equal(typeof result[0].createDom, 'function');
    assert.equal(result[1].clientId, '<fakeClientId1>');
    assert.equal(result[1].clientName, '<fakeClientName1>');
    assert.equal(result[1].createdTime, client1.createdTime);
    assert.equal(typeof result[1].createDom, 'function');
  });
});

[false, true].map(function(/** boolean */ deleted) {
  postInitTest('clearPairedClients, deleted=' + deleted, function(assert) {
    mockHostResponses.push({
      id: 1,
      type: 'clearPairedClientsResponse',
      result: deleted
    });
    return it.clearPairedClients().then(function(result) {
      assert.deepEqual(postMessageStub.args[1][0], {
        id: 1,
        type: 'clearPairedClients'
      });
      assert.equal(result, deleted);
    });
  });
});

[false, true].map(function(/** boolean */ deleted) {
  postInitTest('deletePairedClient, deleted=' + deleted, function(assert) {
    mockHostResponses.push({
      id: 1,
      type: 'deletePairedClientResponse',
      result: deleted
    });
    return it.deletePairedClient('<fakeClientId>').then(function(result) {
      assert.deepEqual(postMessageStub.args[1][0], {
        id: 1,
        type: 'deletePairedClient',
        clientId: '<fakeClientId>'
      });
      assert.equal(result, deleted);
    });
  });
});

postInitTest('getHostClientId', function(assert) {
  mockHostResponses.push({
    id: 1,
    type: 'getHostClientIdResponse',
    clientId: '<fakeClientId>'
  });
  return it.getHostClientId().then(function(result) {
    assert.deepEqual(postMessageStub.args[1][0], {
      id: 1,
      type: 'getHostClientId'
    });
    assert.equal(result, '<fakeClientId>');
  });
});

postInitTest('getCredentialsFromAuthCode', function(assert) {
  mockHostResponses.push({
    id: 1,
    type: 'getCredentialsFromAuthCodeResponse',
    userEmail: '<fakeUserEmail>',
    refreshToken: '<fakeRefreshToken>'
  });
  return it.getCredentialsFromAuthCode('<fakeAuthCode>').then(function(result) {
    assert.deepEqual(postMessageStub.args[1][0], {
      id: 1,
      type: 'getCredentialsFromAuthCode',
      authorizationCode: '<fakeAuthCode>'
    });
    assert.deepEqual(result, {
      userEmail: '<fakeUserEmail>',
      refreshToken: '<fakeRefreshToken>'
    });
  });
});

})();
