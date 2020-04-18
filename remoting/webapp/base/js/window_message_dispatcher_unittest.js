// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 */

(function() {

'use strict';

/** @type {base.WindowMessageDispatcher} */
var windowMessageDispatcher = null;

QUnit.module('WindowMessageDispatcher', {
  beforeEach: function() {
    windowMessageDispatcher = new base.WindowMessageDispatcher();
  },
  afterEach: function() {
    base.dispose(windowMessageDispatcher);
    windowMessageDispatcher = null;
  }
});

// Window messages are asynchronous. The message we post may still be in the
// pipeline by the time we return from the main test routine. So we return a
// promise to wait for the delivery of the message.
QUnit.test('handler should be invoked after registration', function(assert) {
  var deferred = new base.Deferred();

  /** @param {Event} event */
  var handler = function(event) {
    assert.equal(event.data['source'], 'testSource');
    assert.equal(event.data['command'], 'testCommand');

    // Resolve the promise to signal the completion of the test.
    deferred.resolve();
  };

  windowMessageDispatcher.registerMessageHandler('testSource', handler);

  var message = {'source': 'testSource', 'command': 'testCommand'};
  window.postMessage(message, '*');

  return deferred.promise();
});

// In this test we test message dispatching to 'testSource1' before and after
// the registration and unregistration of the message handler. The messages
// for 'testSource2' are used as a synchronization mechanism.
// Here is the workflow of this test:
// 1. Register message handlers for 'testSource1' and 'testSource2'.
// 2. Post a message to testSource1.
// 3. When the message is delivered, unregister message handler for
//    'testSource1' and send a message to 'testSource2'.
// 4. When the message to 'testSource2' is delivered. Send a message
//    each to 'testSource1' and 'testSource2'.
// 5. When the second message to 'testSource2' is delivered. Make sure
//    that the second message to 'testSource1' has not been delivered.
QUnit.test('handler should not be invoked after unregistration',
           function(assert) {
  var deferred = new base.Deferred();
  var callCount1 = 0;
  var callCount2 = 0;

  /** @param {Event} event */
  var handler1 = function(event) {
    if (callCount1 > 0) {
      deferred.reject('handler1 should not be called more than once.');
    }

    assert.equal(event.data['source'], 'testSource1');
    assert.equal(event.data['command'], 'testCommand1');

    ++callCount1;

    // 3. When the message is delivered, unregister message handler for
    //    'testSource1' and send a message to 'testSource2'.
    windowMessageDispatcher.unregisterMessageHandler('testSource1');
    window.postMessage(
        {'source': 'testSource2', 'command': 'testCommand2'}, '*');
  };

  /** @param {Event} event */
  var handler2 = function(event) {
    assert.equal(event.data['source'], 'testSource2');
    assert.equal(event.data['command'], 'testCommand2');

    ++callCount2;

    switch (callCount2) {
      case 1:
        // 4. When the message to 'testSource2' is delivered. Send a message
        //    each to 'testSource1' and 'testSource2'.
        assert.equal(callCount1, 1);
        window.postMessage(
            {'source': 'testSource1', 'command': 'testCommand1'}, '*');
        window.postMessage(
            {'source': 'testSource2', 'command': 'testCommand2'}, '*');
        break;
      case 2:
        // 5. When the second message to 'testSource2' is delivered. Make sure
        //    that the second message to 'testSource1' has not been delivered.
        assert.equal(callCount1, 1);
        deferred.resolve();
        break;
      default:
        deferred.reject('handler1 was never called.');
    };
  };

  // 1. Register message handlers for 'testSource1' and 'testSource2'.
  windowMessageDispatcher.registerMessageHandler('testSource1', handler1);
  windowMessageDispatcher.registerMessageHandler('testSource2', handler2);

  // 2. Post a message to testSource1.
  window.postMessage(
      {'source': 'testSource1', 'command': 'testCommand1'}, '*');

  return deferred.promise();
});

})();
