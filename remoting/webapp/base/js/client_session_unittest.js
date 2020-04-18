// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {

'use strict';

/** @type {remoting.MockConnection} */
var mockConnection;
/** @type {remoting.ClientSession} */
var session;
/** @type {remoting.ClientSession.EventHandler} */
var listener;
/** @type {remoting.SessionLogger} */
var logger;
/** @type {sinon.TestStub} */
var logToServerStub;

/**
 * @constructor
 * @implements {remoting.ClientSession.EventHandler}
 */
var SessionListener = function() {};
SessionListener.prototype.onConnectionFailed = function(error) {};
SessionListener.prototype.onConnected = function(connectionInfo) {};
SessionListener.prototype.onDisconnected = function(error) {};

/**
 * @param {remoting.ClientSession.ConnectionError=} opt_error
 * @return {Promise}
 */
function connect(opt_error) {
  var deferred = new base.Deferred();
  var host = new remoting.Host('fake_hostId');
  host.jabberId = 'fake_jid';

  function onPluginCreated(/** remoting.MockClientPlugin */ plugin) {
    var State = remoting.ClientSession.State;
    plugin.mock$onConnect().then(function() {
      plugin.mock$setConnectionStatus(State.CONNECTING);
    }).then(function() {
      var status = (opt_error) ? State.FAILED : State.CONNECTED;
      plugin.mock$setConnectionStatus(status, opt_error);
    });
  }
  mockConnection.pluginFactory().mock$setPluginCreated(onPluginCreated);

  var sessionFactory = new remoting.ClientSessionFactory(
    document.createElement('div'), ['fake_capability']);

  sessionFactory.createSession(listener, logger).then(
      function(clientSession) {
    session = clientSession;
    clientSession.connect(host, new remoting.CredentialsProvider({
      pairingInfo: { clientId: 'fake_clientId', sharedSecret: 'fake_secret' }
    }));
  });

  listener.onConnected = function() {
    deferred.resolve();
  };

  listener.onConnectionFailed = function(/** remoting.Error */ error) {
    deferred.reject(error);
  };

  return deferred.promise();
}

QUnit.module('ClientSession', {
  beforeEach: function() {
    chromeMocks.identity.mock$setToken('fake_token');

    mockConnection = new remoting.MockConnection();
    listener = new SessionListener();
    logger = new remoting.SessionLogger(remoting.ChromotingEvent.Role.CLIENT,
                                        base.doNothing);
    logToServerStub = sinon.stub(logger, 'logSessionStateChange');
  },
  afterEach: function() {
    session.dispose();
    mockConnection.restore();
  }
});

QUnit.test('should raise CONNECTED event on connected', function(assert) {
  return connect().then(function(){
    assert.ok(true, 'Expect session to connect.');
  });
});

QUnit.test('onOutgoingIq() should send Iq to signalStrategy', function(assert) {
  var sendMessage = sinon.stub(mockConnection.signalStrategy(), 'sendMessage');
  return connect().then(function(){
    session.onOutgoingIq('sample message');
    assert.ok(sendMessage.calledWith('sample message'));
  });
});

QUnit.test('should foward Iq from signalStrategy to plugin', function(assert) {
  return connect().then(function() {
    var onIncomingIq = sinon.stub(mockConnection.plugin(), 'onIncomingIq');
    var stanza = new DOMParser()
                     .parseFromString('<iq>sample</iq>', 'text/xml')
                     .firstElementChild;
    mockConnection.signalStrategy().mock$onIncomingStanza(stanza);
    assert.ok(onIncomingIq.calledWith('<iq>sample</iq>'));
  });
});

QUnit.test('disconnect() should raise the CLOSED event', function(assert) {
  return connect().then(function() {
    var onDisconnected = sinon.stub(listener, 'onDisconnected');
    session.disconnect(remoting.Error.none());
    assert.equal(onDisconnected.callCount, 1);
  });
});

QUnit.test(
  'Connection error after CONNECTED should raise the CONNECTION_DROPPED event',
  function(assert) {

  var State = remoting.ChromotingEvent.SessionState;

  return connect().then(function() {
    var onDisconnected = sinon.stub(listener, 'onDisconnected');
    session.disconnect(new remoting.Error(remoting.Error.Tag.P2P_FAILURE));
    assert.equal(onDisconnected.callCount, 1);
    assert.equal(logToServerStub.args[4][0], State.CONNECTION_DROPPED);
  });
});

QUnit.test(
  'Connection error before CONNECTED should raise the CONNECTION_FAILED event',
  function(assert) {

  var PluginError = remoting.ClientSession.ConnectionError;
  var State = remoting.ChromotingEvent.SessionState;

  return connect(PluginError.SESSION_REJECTED).then(function() {
    assert.ok(false, 'Expect connection to fail');
  }).catch(function(/** remoting.Error */ error) {
    assert.ok(error.hasTag(remoting.Error.Tag.INVALID_ACCESS_CODE));
    assert.equal(logToServerStub.args[3][0], State.CONNECTION_FAILED);
    var errorLogged = /** @type {remoting.Error} */(logToServerStub.args[3][1]);
    assert.equal(errorLogged.getTag(), remoting.Error.Tag.INVALID_ACCESS_CODE);
  });
});

})();
