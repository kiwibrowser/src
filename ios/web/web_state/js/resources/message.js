// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Scripts for the message handler.

goog.provide('__crWeb.message');

goog.require('__crWeb.common');

/**
 * Namespace for this module.
 */
__gCrWeb.message = {};

// Store message namespace object in a global __gCrWeb object referenced by a
// string, so it does not get renamed by closure compiler during the
// minification.
__gCrWeb['message'] = __gCrWeb.message;

/* Beginning of anonymous object. */
(function() {
/**
 * Object to manage queue of messages waiting to be sent to the main
 * application for asynchronous processing.
 * @type {Object}
 * @private
 */
var messageQueue_ = {
  scheme: 'crwebinvoke',
  reset: function() {
    messageQueue_.queue = [];
    // Since the array will be JSON serialized, protect against non-standard
    // custom versions of Array.prototype.toJSON.
    delete messageQueue_.queue.toJSON;
  }
};
messageQueue_.reset();

/**
 * Invokes a command on the Objective-C side.
 * @param {Object} command The command in a JavaScript object.
 * @public
 */
__gCrWeb.message.invokeOnHost = function(command) {
  messageQueue_.queue.push(command);
  sendQueue_(messageQueue_);
};

/**
 * Returns the message queue as a string.
 * @return {string} The current message queue as a JSON string.
 */
__gCrWeb.message.getMessageQueue = function() {
  var messageQueueString = __gCrWeb.common.JSONStringify(messageQueue_.queue);
  messageQueue_.reset();
  return messageQueueString;
};

/**
 * Sends both queues if they contain messages.
 */
__gCrWeb.message.invokeQueues = function() {
  if (messageQueue_.queue.length > 0) sendQueue_(messageQueue_);
};

function sendQueue_(queueObject) {
  // Do nothing if windowId has not been set.
  if (typeof window.top.__gCrWeb.windowId != 'string') {
    return;
  }
  // Some pages/plugins implement Object.prototype.toJSON, which can result
  // in serializing messageQueue_ to an invalid format.
  var originalObjectToJSON = Object.prototype.toJSON;
  if (originalObjectToJSON) delete Object.prototype.toJSON;

  queueObject.queue.forEach(function(command) {
    __gCrWeb.common.sendWebKitMessage(queueObject.scheme, {
      'crwCommand': command,
      'crwWindowId': window.top.__gCrWeb['windowId']
    });
  });
  queueObject.reset();

  if (originalObjectToJSON) {
    // Restore Object.prototype.toJSON to prevent from breaking any
    // functionality on the page that depends on its custom implementation.
    Object.prototype.toJSON = originalObjectToJSON;
  }
}
}());
