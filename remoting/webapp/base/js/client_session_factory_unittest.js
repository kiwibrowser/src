// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {

'use strict';

/** @type {remoting.MockConnection} */
var mockConnection;
/** @type {remoting.ClientSessionFactory} */
var factory;
/** @type {remoting.ClientSession.EventHandler} */
var listener;
/** @type {remoting.SessionLogger} */
var logger;

/**
 * @constructor
 * @implements {remoting.ClientSession.EventHandler}
 */
var SessionListener = function() {};
SessionListener.prototype.onConnectionFailed = function(error) {};
SessionListener.prototype.onConnected = function(connectionInfo) {};
SessionListener.prototype.onDisconnected = function(reason) {};
SessionListener.prototype.onError = function(error) {};

QUnit.module('ClientSessionFactory', {
  beforeEach: function() {
    chromeMocks.identity.mock$setToken('fake_token');

    mockConnection = new remoting.MockConnection();
    listener = new SessionListener();
    logger = new remoting.SessionLogger(remoting.ChromotingEvent.Role.CLIENT,
                                        base.doNothing);
    factory = new remoting.ClientSessionFactory(
        document.createElement('div'),
        [remoting.ClientSession.Capability.SEND_INITIAL_RESOLUTION]);
  },
  afterEach: function() {
    mockConnection.restore();
  }
});

QUnit.test('createSession() should return a remoting.ClientSession',
    function(assert) {
  return factory.createSession(listener, logger).then(
    function(/** remoting.ClientSession */ session){
      assert.ok(session instanceof remoting.ClientSession);
      assert.ok(
          mockConnection.plugin().hasCapability(
              remoting.ClientSession.Capability.SEND_INITIAL_RESOLUTION),
          'Capability is set correctly.');
  });
});

QUnit.test('createSession() should reject on signal strategy failure',
    function(assert) {
  var mockSignalStrategy = mockConnection.signalStrategy();
  mockSignalStrategy.connect = function() {
    Promise.resolve().then(function () {
      mockSignalStrategy.setStateForTesting(
          remoting.SignalStrategy.State.FAILED);
    });
  };

  var signalStrategyDispose = sinon.stub(mockSignalStrategy, 'dispose');

  return factory.createSession(listener, logger).then(
    assert.ok.bind(assert, false, 'Expect createSession() to fail.')
  ).catch(function(/** remoting.Error */ error) {
    assert.ok(
        signalStrategyDispose.called, 'SignalStrategy is disposed on failure.');
    assert.equal(error.getDetail(), 'setStateForTesting',
                 'Error message is set correctly.');
  });
});

QUnit.test('createSession() should reject on plugin initialization failure',
    function(assert) {
  var mockSignalStrategy = mockConnection.signalStrategy();
  function onPluginCreated(/** remoting.MockClientPlugin */ plugin) {
     plugin.mock$initializationResult = false;
  }
  mockConnection.pluginFactory().mock$setPluginCreated(onPluginCreated);

  var signalStrategyDispose = sinon.stub(mockSignalStrategy, 'dispose');

  return factory.createSession(listener, logger).then(function() {
    assert.ok(false, 'Expect createSession() to fail.');
  }).catch(function(/** remoting.Error */ error) {
    assert.ok(
        signalStrategyDispose.called, 'SignalStrategy is disposed on failure.');
    assert.ok(error.hasTag(remoting.Error.Tag.MISSING_PLUGIN),
        'Initialization failed with MISSING_PLUGIN.');
  });
});

})();
