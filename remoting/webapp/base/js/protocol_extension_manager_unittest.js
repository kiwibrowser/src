// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 */

(function() {

'use strict';

/** @type {remoting.ProtocolExtensionManager} */
var extensionManager;

/** @type {(sinon.Spy|function(string, string))} */
var sendClientMessage;

/**
 * @constructor
 * @param {Array<string>} types
 * @implements {remoting.ProtocolExtension}
 */
var DummyExtension = function(types) {
  /** @private {?function(string, string)} */
  this.sendMessageToHost_ = null;
  /** @private */
  this.types_ = types;
};

DummyExtension.prototype.getExtensionTypes = function() {
  return this.types_.slice(0);
}

/**
 * @param {function(string,string)} sendMessageToHost Callback to send a message
 *     to the host.
 */
DummyExtension.prototype.startExtension = function(sendMessageToHost) {
  this.sendMessageToHost_ = sendMessageToHost;
};

/**
 * Called when an extension message of a matching type is received.
 *
 * @param {string} type The message type.
 * @param {Object} message The parsed extension message data.
 * @return {boolean} True if the extension message was handled.
 */
DummyExtension.prototype.onExtensionMessage = function(type, message){
  return this.types_.indexOf(type) !== 1;
};


QUnit.module('ProtocolExtensionManager', {
  beforeEach: function() {
    sendClientMessage = /** @type {function(string, string)} */ (sinon.spy());
    extensionManager = new remoting.ProtocolExtensionManager(sendClientMessage);
  },
  afterEach: function() {
  }
});

QUnit.test('should route message to extension by type', function(assert) {
  var extension = new DummyExtension(['type1', 'type2']);
  var onExtensionMessage = /** @type {(sinon.Spy|function(string, string))} */ (
      sinon.spy(extension, 'onExtensionMessage'));
  assert.ok(extensionManager.register(extension));
  extensionManager.start();

  extensionManager.onProtocolExtensionMessage('type1', '{"message": "hello"}');
  assert.ok(onExtensionMessage.called);
  onExtensionMessage.reset();

  extensionManager.onProtocolExtensionMessage('type2', '{"message": "hello"}');
  assert.ok(onExtensionMessage.called);
  onExtensionMessage.reset();

  extensionManager.onProtocolExtensionMessage('type3', '{"message": "hello"}');
  assert.ok(!onExtensionMessage.called);
  onExtensionMessage.reset();
});

QUnit.test('startExtension() should only be called once', function(assert) {
  var extension = new DummyExtension(['type1', 'type2']);
  var startExtension = /** @type {(sinon.Spy|Function)} */ (
      sinon.spy(extension, 'startExtension'));

  assert.ok(extensionManager.register(extension));
  extensionManager.start();
  assert.ok(startExtension.calledOnce);
});


QUnit.test('should not register extensions of the same type', function(assert) {
  var extension1 = new DummyExtension(['type1']);
  var extension2 = new DummyExtension(['type1']);

  var onExtensionMessage1 = /** @type {(sinon.Spy|function(string, string))} */(
      sinon.spy(extension1, 'onExtensionMessage'));
  var onExtensionMessage2 = /** @type {(sinon.Spy|function(string, string))} */(
      sinon.spy(extension2, 'onExtensionMessage'));

  assert.ok(extensionManager.register(extension1));
  assert.ok(!extensionManager.register(extension2));
  extensionManager.start();

  extensionManager.onProtocolExtensionMessage('type1', '{"message": "hello"}');
  assert.ok(onExtensionMessage1.called);
  assert.ok(!onExtensionMessage2.called);
});

QUnit.test('should handle extensions registration after it is started',
    function(assert) {
  var extension = new DummyExtension(['type']);

  var onExtensionMessage = /** @type {(sinon.Spy|function(string, string))} */(
      sinon.spy(extension, 'onExtensionMessage'));

  extensionManager.start();
  assert.ok(extensionManager.register(extension));

  extensionManager.onProtocolExtensionMessage('type', '{"message": "hello"}');
  assert.ok(onExtensionMessage.called);
});

})();
