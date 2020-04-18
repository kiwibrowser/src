// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {

'use strict';

var testUsername = 'testUsername@gmail.com';
var testToken = 'testToken';
var socketId = 3;

var onStanzaStr = null;

/** @type {remoting.XmppConnection} */
var connection = null;

/** @type {remoting.TcpSocket} */
var socket = null;

var stateChangeHandler = function(/** remoting.SignalStrategy.State */ state) {}

function onStateChange(/** remoting.SignalStrategy.State */ state) {
  stateChangeHandler(state)
};

/**
 * @param {QUnit.Assert} assert
 * @param {remoting.SignalStrategy.State} expectedState
 * @returns {Promise}
 */
function expectNextState(assert, expectedState) {
  return new Promise(function(resolve, reject) {
    stateChangeHandler = function(/** remoting.SignalStrategy.State */ state) {
      assert.equal(state, expectedState);
      assert.equal(connection.getState(), expectedState);
      resolve(0);
    }
  });
}

QUnit.module('XmppConnection', {
  beforeEach: function() {
    remoting.settings = new remoting.Settings();

    onStanzaStr = sinon.spy();
    /** @param {Element} stanza */
    function onStanza(stanza) {
      onStanzaStr(new XMLSerializer().serializeToString(stanza));
    }

    socket = /** @type{remoting.TcpSocket} */
        (sinon.createStubInstance(remoting.TcpSocket));

    connection = new remoting.XmppConnection();
    connection.setSocketForTests(socket);
    connection.setStateChangedCallback(onStateChange);
    connection.setIncomingStanzaCallback(onStanza);
  },
  afterEach: function() {
    remoting.settings = null;
  }
});

QUnit.test('should go to FAILED state when failed to connect',
    function(assert) {
  var done = assert.async();
  $testStub(socket.connect).withArgs("xmpp.example.com", 123)
      .returns(new Promise(function(resolve, reject) { reject(-1); }));

  var deferredSend = new base.Deferred();
  $testStub(socket.send).onFirstCall().returns(deferredSend.promise());

  expectNextState(assert, remoting.SignalStrategy.State.CONNECTING)
      .then(onConnecting);
  connection.connect('xmpp.example.com:123', 'testUsername@gmail.com',
                     'testToken');

  function onConnecting() {
    expectNextState(assert, remoting.SignalStrategy.State.FAILED)
        .then(onFailed);
  }

  function onFailed() {
    sinon.assert.calledWith(socket.dispose);
    assert.ok(connection.getError().hasTag(remoting.Error.Tag.NETWORK_FAILURE));
    done();
  }
});

QUnit.test('should use XmppLoginHandler for handshake', function(assert) {

  $testStub(socket.connect).withArgs("xmpp.example.com", 123)
      .returns(new Promise(function(resolve, reject) { resolve(0) }));

  var deferredSend = new base.Deferred();
  $testStub(socket.send).onFirstCall().returns(deferredSend.promise());

  var parser = new remoting.XmppStreamParser();
  var parserMock = sinon.mock(parser);
  var setCallbacksCalled = parserMock.expects('setCallbacks').once();
  var State = remoting.SignalStrategy.State;

  var promise = expectNextState(assert, State.CONNECTING).then(function() {
    return expectNextState(assert, State.HANDSHAKE);
  }).then(function() {
    var handshakeDoneCallback =
        connection.loginHandler_.getHandshakeDoneCallbackForTesting();
    var onConnected = expectNextState(assert, State.CONNECTED);
    handshakeDoneCallback('test@example.com/123123', parser);
    return onConnected;
  }).then(function() {
    setCallbacksCalled.verify();

    // Simulate read() callback with |data|. It should be passed to
    // the parser.
    var data = base.encodeUtf8('<iq id="1">hello</iq>');
    sinon.assert.calledWith(socket.startReceiving);
    var appendDataCalled =
        parserMock.expects('appendData').once().withArgs(data);
    $testStub(socket.startReceiving).getCall(0).args[0](data);
    appendDataCalled.verify();
  });

  connection.connect(
      'xmpp.example.com:123', 'testUsername@gmail.com', 'testToken');
  return promise;
});

})();
