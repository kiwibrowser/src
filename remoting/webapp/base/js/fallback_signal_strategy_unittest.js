// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * TODO(garykac): Create interfaces for LogToServer and SignalStrategy.
 * @suppress {checkTypes|checkVars|reportUnknownTypes|visibility}
 */

(function() {

'use strict';

/** @constructor */
var MockLogToServer = function(/** QUnit.Assert */ assert) {
  /** @type {(sinon.Spy|Function)} */
  this.logSignalStrategyProgress = sinon.spy();
  this.assert_ = assert;
};

/** @type {function(...)} */
MockLogToServer.prototype.assertProgress = function() {
  this.assert_.equal(this.logSignalStrategyProgress.callCount * 2,
                     arguments.length);
  for (var i = 0; i < this.logSignalStrategyProgress.callCount; ++i) {
    this.assert_.equal(
        this.logSignalStrategyProgress.getCall(i).args[0], arguments[2 * i]);
    this.assert_.equal(this.logSignalStrategyProgress.getCall(i).args[1],
                arguments[2 * i + 1]);
  }
};

/** @type {(sinon.Spy|function(remoting.SignalStrategy.State))} */
var onStateChange = null;

/** @type {(sinon.Spy|function(Element):void)} */
var onIncomingStanzaCallback = null;

/** @type {remoting.FallbackSignalStrategy} */
var strategy = null;

/** @type {remoting.SignalStrategy} */
var primary = null;

/** @type {remoting.SignalStrategy} */
var secondary = null;

/** @type {MockLogToServer} */
var logToServer = null;

/**
 * @param {QUnit.Assert} assert
 * @param {remoting.MockSignalStrategy} baseSignalStrategy
 * @param {remoting.SignalStrategy.State} state
 * @param {boolean} expectCallback
 */
function setState(assert, baseSignalStrategy, state, expectCallback) {
  onStateChange.reset();
  baseSignalStrategy.setStateForTesting(state);

  if (expectCallback) {
    assert.equal(onStateChange.callCount, 1);
    assert.ok(onStateChange.calledWith(state));
    assert.equal(strategy.getState(), state);
  } else {
    assert.ok(!onStateChange.called);
  }
}

QUnit.module('fallback_signal_strategy', {
  beforeEach: function(/** QUnit.Assert */ assert) {
    onStateChange = sinon.spy();
    onIncomingStanzaCallback = sinon.spy();
    strategy = new remoting.FallbackSignalStrategy(
        new remoting.MockSignalStrategy('primary-jid',
                                        remoting.SignalStrategy.Type.XMPP),
        new remoting.MockSignalStrategy('secondary-jid',
                                        remoting.SignalStrategy.Type.WCS));
    strategy.setStateChangedCallback(onStateChange);
    strategy.setIncomingStanzaCallback(onIncomingStanzaCallback);
    primary = strategy.primary_;
    addSpies(primary);
    secondary = strategy.secondary_;
    addSpies(secondary);
    logToServer = new MockLogToServer(assert);
    strategy.logger_ = logToServer;
  },
  afterEach: function() {
    onStateChange = null;
    onIncomingStanzaCallback = null;
    strategy = null;
    primary = null;
    secondary = null;
    logToServer = null;
  },
});

/**
 * @param {remoting.SignalStrategy} strategy
 */
function addSpies(strategy) {
  sinon.spy(strategy, 'connect');
  sinon.spy(strategy, 'sendMessage');
  sinon.spy(strategy, 'dispose');
}

QUnit.test('primary succeeds; send & receive routed to it',
  function(assert) {
    assert.ok(!onStateChange.called);
    assert.ok(!primary.connect.called);
    strategy.connect('server', 'username', 'authToken');
    assert.ok(primary.connect.calledWith('server', 'username', 'authToken'));

    setState(assert, primary, remoting.SignalStrategy.State.NOT_CONNECTED,
             true);
    setState(assert, primary, remoting.SignalStrategy.State.CONNECTING, true);
    setState(assert, primary, remoting.SignalStrategy.State.HANDSHAKE, true);

    setState(assert, primary, remoting.SignalStrategy.State.CONNECTED, true);
    assert.equal(strategy.getJid(), 'primary-jid');

    logToServer.assertProgress(
        remoting.SignalStrategy.Type.XMPP,
        remoting.FallbackSignalStrategy.Progress.SUCCEEDED);

    assert.ok(!onIncomingStanzaCallback.called);
    primary.onIncomingStanzaCallback_('test-receive-primary');
    secondary.onIncomingStanzaCallback_('test-receive-secondary');
    assert.ok(onIncomingStanzaCallback.calledOnce);
    assert.ok(onIncomingStanzaCallback.calledWith('test-receive-primary'));

    assert.ok(!primary.sendMessage.called);
    strategy.sendMessage('test-send');
    assert.ok(primary.sendMessage.calledOnce);
    assert.ok(primary.sendMessage.calledWith('test-send'));

    assert.ok(!primary.dispose.called);
    assert.ok(!secondary.dispose.called);
    setState(assert, primary, remoting.SignalStrategy.State.CLOSED, true);
    strategy.dispose();
    assert.ok(primary.dispose.calledOnce);
    assert.ok(secondary.dispose.calledOnce);
  }
);

QUnit.test('primary fails; secondary succeeds; send & receive routed to it',
  function(assert) {
    assert.ok(!onStateChange.called);
    assert.ok(!primary.connect.called);
    strategy.connect('server', 'username', 'authToken');
    assert.ok(primary.connect.calledWith('server', 'username', 'authToken'));

    setState(assert, primary, remoting.SignalStrategy.State.NOT_CONNECTED,
             true);
    setState(assert, primary, remoting.SignalStrategy.State.CONNECTING, true);

    assert.ok(!secondary.connect.called);
    setState(assert, primary, remoting.SignalStrategy.State.FAILED, false);
    assert.ok(secondary.connect.calledWith('server', 'username', 'authToken'));

    setState(assert, secondary, remoting.SignalStrategy.State.NOT_CONNECTED,
             false);
    setState(assert, secondary, remoting.SignalStrategy.State.CONNECTING,
             false);
    setState(assert, secondary, remoting.SignalStrategy.State.HANDSHAKE, true);

    setState(assert, secondary, remoting.SignalStrategy.State.CONNECTED, true);
    assert.equal(strategy.getJid(), 'secondary-jid');

    logToServer.assertProgress(
        remoting.SignalStrategy.Type.XMPP,
        remoting.FallbackSignalStrategy.Progress.FAILED,
        remoting.SignalStrategy.Type.WCS,
        remoting.FallbackSignalStrategy.Progress.SUCCEEDED);

    assert.ok(!onIncomingStanzaCallback.called);
    primary.onIncomingStanzaCallback_('test-receive-primary');
    secondary.onIncomingStanzaCallback_('test-receive-secondary');
    assert.ok(onIncomingStanzaCallback.calledOnce);
    assert.ok(onIncomingStanzaCallback.calledWith('test-receive-secondary'));

    assert.ok(!secondary.sendMessage.called);
    strategy.sendMessage('test-send');
    assert.ok(!primary.sendMessage.called);
    assert.ok(secondary.sendMessage.calledOnce);
    assert.ok(secondary.sendMessage.calledWith('test-send'));
  }
);

QUnit.test('primary fails; secondary fails',
  function(assert) {
    assert.ok(!onStateChange.called);
    assert.ok(!primary.connect.called);
    strategy.connect('server', 'username', 'authToken');
    assert.ok(primary.connect.calledWith('server', 'username', 'authToken'));

    setState(assert, primary, remoting.SignalStrategy.State.NOT_CONNECTED,
             true);
    assert.ok(!secondary.connect.called);
    setState(assert, primary, remoting.SignalStrategy.State.CONNECTING, true);
    setState(assert, primary, remoting.SignalStrategy.State.FAILED, false);
    assert.ok(secondary.connect.calledWith('server', 'username', 'authToken'));
    setState(assert, secondary, remoting.SignalStrategy.State.NOT_CONNECTED,
             false);
    setState(assert, primary, remoting.SignalStrategy.State.CONNECTING, false);
    setState(assert, secondary, remoting.SignalStrategy.State.FAILED, true);
  }
);

QUnit.test('primary times out; secondary succeeds',
  function(assert) {
    assert.ok(!onStateChange.called);
    assert.ok(!primary.connect.called);
    strategy.connect('server', 'username', 'authToken');
    assert.ok(primary.connect.calledWith('server', 'username', 'authToken'));

    setState(assert, primary, remoting.SignalStrategy.State.NOT_CONNECTED,
                               true);
    setState(assert, primary, remoting.SignalStrategy.State.CONNECTING, true);
    this.clock.tick(strategy.PRIMARY_CONNECT_TIMEOUT_MS_ - 1);
    assert.ok(!secondary.connect.called);
    this.clock.tick(1);
    assert.ok(secondary.connect.calledWith('server', 'username', 'authToken'));
    setState(assert, secondary, remoting.SignalStrategy.State.NOT_CONNECTED,
             false);
    setState(assert, secondary, remoting.SignalStrategy.State.CONNECTING,
             false);
    setState(assert, secondary, remoting.SignalStrategy.State.HANDSHAKE, true);
    setState(assert, secondary, remoting.SignalStrategy.State.CONNECTED, true);

    setState(assert, secondary, remoting.SignalStrategy.State.CLOSED, true);
    setState(assert, primary, remoting.SignalStrategy.State.FAILED, false);

    logToServer.assertProgress(
        remoting.SignalStrategy.Type.XMPP,
        remoting.FallbackSignalStrategy.Progress.TIMED_OUT,
        remoting.SignalStrategy.Type.WCS,
        remoting.FallbackSignalStrategy.Progress.SUCCEEDED,
        remoting.SignalStrategy.Type.XMPP,
        remoting.FallbackSignalStrategy.Progress.FAILED_LATE);
  }
);

QUnit.test('primary times out; secondary fails',
  function(assert) {
    assert.ok(!onStateChange.called);
    assert.ok(!primary.connect.called);
    strategy.connect('server', 'username', 'authToken');
    assert.ok(primary.connect.calledWith('server', 'username', 'authToken'));

    setState(assert, primary, remoting.SignalStrategy.State.NOT_CONNECTED,
                               true);
    setState(assert, primary, remoting.SignalStrategy.State.CONNECTING, true);
    this.clock.tick(strategy.PRIMARY_CONNECT_TIMEOUT_MS_ - 1);
    assert.ok(!secondary.connect.called);
    this.clock.tick(1);
    assert.ok(secondary.connect.calledWith('server', 'username', 'authToken'));
    setState(assert, secondary, remoting.SignalStrategy.State.NOT_CONNECTED,
             false);
    setState(assert, secondary, remoting.SignalStrategy.State.CONNECTING,
             false);
    setState(assert, secondary, remoting.SignalStrategy.State.FAILED, true);
  }
);

QUnit.test('primary times out; secondary succeeds; primary succeeds late',
  function(assert) {
    assert.ok(!onStateChange.called);
    assert.ok(!primary.connect.called);
    strategy.connect('server', 'username', 'authToken');
    assert.ok(primary.connect.calledWith('server', 'username', 'authToken'));

    setState(assert, primary, remoting.SignalStrategy.State.NOT_CONNECTED,
                               true);
    setState(assert, primary, remoting.SignalStrategy.State.CONNECTING, true);
    this.clock.tick(strategy.PRIMARY_CONNECT_TIMEOUT_MS_);
    assert.ok(secondary.connect.calledWith('server', 'username', 'authToken'));
    setState(assert, secondary, remoting.SignalStrategy.State.NOT_CONNECTED,
             false);
    setState(assert, secondary, remoting.SignalStrategy.State.CONNECTING,
             false);
    setState(assert, secondary, remoting.SignalStrategy.State.HANDSHAKE, true);
    setState(assert, secondary, remoting.SignalStrategy.State.CONNECTED, true);

    setState(assert, primary, remoting.SignalStrategy.State.HANDSHAKE, false);
    setState(assert, primary, remoting.SignalStrategy.State.CONNECTED, false);

    logToServer.assertProgress(
        remoting.SignalStrategy.Type.XMPP,
        remoting.FallbackSignalStrategy.Progress.TIMED_OUT,
        remoting.SignalStrategy.Type.WCS,
        remoting.FallbackSignalStrategy.Progress.SUCCEEDED,
        remoting.SignalStrategy.Type.XMPP,
        remoting.FallbackSignalStrategy.Progress.SUCCEEDED_LATE);
  }
);

})();
