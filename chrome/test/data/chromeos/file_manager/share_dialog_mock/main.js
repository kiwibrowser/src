// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Namespace for the share dialog mock.
var shareDialog = {};

/**
 * Origin of the Files app.
 * @type {string}
 * @const
 */
shareDialog.EMBEDDER_ORIGIN =
    'chrome-extension://hhaomjibdihmijegdhdafkllkbggdgoj';

/**
 * Target width of the sharing window in pixels.
 * @type {number}
 * @const
 */
shareDialog.TARGET_WIDTH = 350;

/**
 * Target height of the sharing window in pixels.
 * @type {number}
 * @const
 */
shareDialog.TARGET_HEIGHT = 250;

/**
 * Target window of the Files app. Used to communicate over messages. Filled
 * out once the first message from the embedder arrives.
 * @type {Window}
 */
shareDialog.embedderTarget = null;

/**
 * List of pending messages enqueued to be sent before establishing the target.
 * @type {Array<Object>}
 */
shareDialog.pendingMessages = [];

/**
 * Sends a message to the embedder. If the embedder target is not available,
 * then enqueues them. Such enqueued messages will be sent as soon as the target
 * is available.
 *
 * @param {string} type Message identifier
 * @param {Object=} opt_args Arguments for the message.
 * @private
 */
shareDialog.sendMessage_ = function(type, opt_args) {
  if (!shareDialog.embedderTarget) {
    shareDialog.pendingMessages.push({type: type, args: opt_args});
    return;
  }

  var data = {};
  data.type = type;
  if (opt_args)
    data.args = opt_args;

  shareDialog.embedderTarget.postMessage(JSON.stringify(data),
                                         shareDialog.EMBEDDER_ORIGIN);
};

/**
 * Handles a request from the embedder to make the body visible.
 * @private
 */
shareDialog.onMakeBodyVisible_ = function() {
  document.body.style.display = '';
};

/**
 * Handles an event from the embedder than preparation to show the contents
 * is done.
 * @private
 */
shareDialog.onPrepareComplete_ = function() {
  shareDialog.resize();
};

/**
 * Handles an event from the embedder than preparation resize the window is
 * done.
 * @private
 */
shareDialog.onResizeComplete_ = function() {
  var container = document.querySelector('#container');
  container.style.width = shareDialog.TARGET_WIDTH + 'px';
  container.style.height = shareDialog.TARGET_HEIGHT + 'px';
};

/**
 * Changes the visibility of the dialog.
 * @param {boolean} visible True to set the dialog visible, false to set it
 *     invisible.
 */
shareDialog.setVisible = function(visible) {
  shareDialog.sendMessage_('setVisible', {visible: visible});
};

/**
 * Prepares the embedder to make the contents visible.
 */
shareDialog.prepareForVisible = function() {
  shareDialog.sendMessage_('prepareForVisible');
};

/**
 * Resizes the embedder to the content window dimensions.
 */
shareDialog.resize = function() {
  shareDialog.sendMessage_('prepareForResize');
};

/**
 * Handles messages sent by the embedder. If it is the first message, then
 * the target is established and all enqueued messages to be sent to the
 * embedder are sent before handling the message from the embedder.
 *
 * @param {Event} message Message event.
 * @private
 */
shareDialog.onMessage_ = function(message) {
  if (message.origin != shareDialog.EMBEDDER_ORIGIN)
    return;

  if (!shareDialog.embedderTarget) {
    shareDialog.embedderTarget = message.source;
    for (var i = 0; i < shareDialog.pendingMessages.length; i++) {
      shareDialog.sendMessage_(shareDialog.pendingMessages[i].type,
                               shareDialog.pendingMessages[i].args);
    }
    shareDialog.pendingMessages = [];
  }

  var packet = JSON.parse(message.data)
  var type = packet.type;
  var args = packet.args;

  switch (type) {
    case 'makeBodyVisible':
      shareDialog.onMakeBodyVisible_(args);
      break;
    case 'prepareComplete':
      shareDialog.onPrepareComplete_(args);
      break;
    case 'resizeComplete':
      shareDialog.onResizeComplete_(args);
      break;
  }
};

/**
 * Initializes the mocked share dialog.
 */
shareDialog.initialize = function() {
  window.addEventListener('message', shareDialog.onMessage_);
  shareDialog.prepareForVisible();
};

window.addEventListener('load', shareDialog.initialize);
