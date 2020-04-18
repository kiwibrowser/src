// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * TODO(garykac): Create interface for SignalStrategy.
 * @suppress {checkTypes|checkVars|reportUnknownTypes|visibility}
 */

(function() {

'use strict';

/** @type {(sinon.Spy|function(remoting.SignalStrategy.State))} */
var onStateChange = null;

/** @type {(sinon.Spy|function(Element):void)} */
var onIncomingStanzaCallback = null;

/** @type {remoting.DnsBlackholeChecker} */
var checker = null;

/** @type {remoting.MockSignalStrategy} */
var signalStrategy = null;

/** @type {sinon.FakeXhr} */
var fakeXhr = null;

QUnit.module('dns_blackhole_checker', {
  beforeEach: function(assert) {
    sinon.useFakeXMLHttpRequest().onCreate = function(xhr) {
      QUnit.equal(fakeXhr, null, 'exactly one XHR is issued');
      fakeXhr = xhr;
    };

    remoting.settings = new remoting.Settings();
    onStateChange = sinon.spy();
    onIncomingStanzaCallback = sinon.spy();
    signalStrategy = new remoting.MockSignalStrategy();
    sinon.stub(signalStrategy, 'connect', base.doNothing);
    checker = new remoting.DnsBlackholeChecker(signalStrategy);

    checker.setStateChangedCallback(onStateChange);
    checker.setIncomingStanzaCallback(onIncomingStanzaCallback);

    sinon.assert.notCalled(onStateChange);
    sinon.assert.notCalled(signalStrategy.connect);
    checker.connect('server', 'username', 'authToken');
    sinon.assert.calledWith(signalStrategy.connect, 'server', 'username',
                            'authToken');

    assert.equal(
        fakeXhr.url, checker.url_,
        'the correct URL is requested');
  },
  afterEach: function() {
    base.dispose(checker);
    sinon.assert.calledWith(onStateChange,
                            remoting.SignalStrategy.State.CLOSED);
    remoting.settings = null;
    onStateChange = null;
    onIncomingStanzaCallback = null;
    checker = null;
    fakeXhr = null;
  }
});

QUnit.test('success',
  function(assert) {
    function checkState(state) {
      signalStrategy.setStateForTesting(state);
      sinon.assert.calledWith(onStateChange, state);
      assert.equal(checker.getState(), state);
    }

    return Promise.resolve().then(function() {
      fakeXhr.respond(200);
    }).then(function() {
      sinon.assert.notCalled(onStateChange);
      checkState(remoting.SignalStrategy.State.CONNECTING);
      checkState(remoting.SignalStrategy.State.HANDSHAKE);
      checkState(remoting.SignalStrategy.State.CONNECTED);
    });
  });

QUnit.test('http response after connected',
  function(assert) {
    function checkState(state) {
      signalStrategy.setStateForTesting(state);
      sinon.assert.calledWith(onStateChange, state);
      assert.equal(checker.getState(), state);
    }

    checkState(remoting.SignalStrategy.State.CONNECTING);
    checkState(remoting.SignalStrategy.State.HANDSHAKE);
    onStateChange.reset();

    // Verify that DnsBlackholeChecker stays in HANDSHAKE state even if the
    // signal strategy has connected.
    return Promise.resolve().then(function() {
      signalStrategy.setStateForTesting(
          remoting.SignalStrategy.State.CONNECTED);
    }).then(function() {
      sinon.assert.notCalled(onStateChange);
    assert.equal(checker.getState(), remoting.SignalStrategy.State.HANDSHAKE);

      // Verify that DnsBlackholeChecker goes to CONNECTED state after the
      // the HTTP request has succeeded.
      return Promise.resolve().then(function() {
        fakeXhr.respond(200);
      });
    }).then(function() {
      sinon.assert.calledWith(onStateChange,
                              remoting.SignalStrategy.State.CONNECTED);
    });
  });

QUnit.test('connect failed',
  function(assert) {
    function checkState(state) {
      signalStrategy.setStateForTesting(state);
      sinon.assert.calledWith(onStateChange, state);
    };

    return Promise.resolve().then(function() {
      fakeXhr.respond(200);
    }).then(function() {
      sinon.assert.notCalled(onStateChange);
      checkState(remoting.SignalStrategy.State.CONNECTING);
      checkState(remoting.SignalStrategy.State.FAILED);
    });
  });

QUnit.test('blocked',
  function(assert) {
    function checkState(state) {
    assert.equal(checker.getError().getTag(),
                 remoting.Error.Tag.NOT_AUTHORIZED);
      onStateChange.reset();
      signalStrategy.setStateForTesting(state);
      sinon.assert.notCalled(onStateChange);
      assert.equal(checker.getState(),
          checker.getState(),
          remoting.SignalStrategy.State.FAILED,
          'checker state is still FAILED');
    };

    return Promise.resolve().then(function() {
      fakeXhr.respond(400);
    }).then(function() {
      sinon.assert.calledWith(
          onStateChange, remoting.SignalStrategy.State.FAILED);
      assert.equal(
          checker.getError().getTag(),
          remoting.Error.Tag.NOT_AUTHORIZED,
          'checker error is NOT_AUTHORIZED');
      checkState(remoting.SignalStrategy.State.CONNECTING);
      checkState(remoting.SignalStrategy.State.HANDSHAKE);
      checkState(remoting.SignalStrategy.State.FAILED);
    });
  });

QUnit.test('blocked after connected',
  function(assert) {
    function checkState(state) {
      signalStrategy.setStateForTesting(state);
      sinon.assert.calledWith(onStateChange, state);
      assert.equal(checker.getState(), state);
    };

    checkState(remoting.SignalStrategy.State.CONNECTING);
    checkState(remoting.SignalStrategy.State.HANDSHAKE);
    onStateChange.reset();

    // Verify that DnsBlackholeChecker stays in HANDSHAKE state even
    // if the signal strategy has connected.
    return Promise.resolve().then(function() {
      signalStrategy.setStateForTesting(
          remoting.SignalStrategy.State.CONNECTED);
    }).then(function() {
      sinon.assert.notCalled(onStateChange);
    assert.equal(checker.getState(), remoting.SignalStrategy.State.HANDSHAKE);

      // Verify that DnsBlackholeChecker goes to FAILED state after it
      // gets the blocked HTTP response.
      return Promise.resolve().then(function() {
        fakeXhr.respond(400);
      });
    }).then(function() {
      sinon.assert.calledWith(onStateChange,
                              remoting.SignalStrategy.State.FAILED);
    assert.ok(checker.getError().hasTag(remoting.Error.Tag.NOT_AUTHORIZED));
    });
  }
);

})();
