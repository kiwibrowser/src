// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/** @suppress {duplicate} */
var remoting = remoting || {};

/**
 * This class manages all the message windows (remoting.MessageWindow).
 * @param {base.WindowMessageDispatcher} windowMessageDispatcher
 * @constructor
 * @implements {base.Disposable}
 */
remoting.MessageWindowManager = function(windowMessageDispatcher) {
  /**
   * @type {!Object<number, remoting.MessageWindow>}
   * @private
   */
  this.messageWindows_ = {};

  /**
   * The next window id to auto-assign.
   * @private {number}
   */
  this.nextId_ = 1;

  /** @private {base.WindowMessageDispatcher} */
  this.windowMessageDispatcher_ = windowMessageDispatcher;

  this.windowMessageDispatcher_.registerMessageHandler(
      'message-window', this.onMessage_.bind(this));
};

remoting.MessageWindowManager.prototype.dispose = function() {
  this.windowMessageDispatcher_.unregisterMessageHandler('message-window');
};

/**
 * @param {remoting.MessageWindow} window The window to associate
 *     with the window id.
 * @return {number} The window id.
 */
remoting.MessageWindowManager.prototype.addMessageWindow = function(window) {
  var id = ++this.nextId_;
  this.messageWindows_[id] = window;
  return id;
};

/**
 * @param {number} id The window id.
 * @return {remoting.MessageWindow}
 */
remoting.MessageWindowManager.prototype.getMessageWindow = function(id) {
  return this.messageWindows_[id];
};

/**
 * @param {number} id The window id to delete.
 */
remoting.MessageWindowManager.prototype.deleteMessageWindow = function(id) {
  delete this.messageWindows_[id];
};

/**
 * Close all of the registered MessageWindows
 */
remoting.MessageWindowManager.prototype.closeAllMessageWindows = function() {
  /** @type {Array<remoting.MessageWindow>} */
  var windows = [];
  // Make a list of the windows to close.
  // We don't delete the window directly in this loop because close() can
  // call deleteMessageWindow which will update messageWindows_.
  for (var win_id in this.messageWindows_) {
    /** @type {remoting.MessageWindow} */
    var win = this.getMessageWindow(parseInt(win_id, 10));
    console.assert(win != null, 'Unknown window id ' + win_id + '.');
    windows.push(win);
  }
  for (var i = 0; i < windows.length; i++) {
    /** @type {remoting.MessageWindow} */(windows[i]).close();
  }
};

/**
 * Dispatch a message box result to the appropriate callback.
 *
 * @param {Event} event
 * @private
 */
remoting.MessageWindowManager.prototype.onMessage_ = function(event) {
  console.assert(typeof event.data === 'object',
                 'Unexpected data. Expected object, got ' + event.data + '.');
  console.assert(event.data['source'] == 'message-window',
                'Bad event source: ' +
                /** @type {string} */ (event.data['source']) + '.');

  if (event.data['command'] == 'messageWindowResult') {
    var id = /** @type {number} */ (event.data['id']);
    var result = /** @type {number} */ (event.data['result']);

    if (typeof(id) != 'number' || typeof(result) != 'number') {
      console.log('Poorly formatted id or result');
      return;
    }

    var messageWindow = this.getMessageWindow(id);
    if (!messageWindow) {
      console.log('Ignoring unknown message window id:', id);
      return;
    }

    messageWindow.handleResult(result);
    messageWindow.close();
  }
};

/** @type {remoting.MessageWindowManager} */
remoting.messageWindowManager = null;
