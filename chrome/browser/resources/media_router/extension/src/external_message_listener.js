// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Listener for messages received from external apps/extensions or
 * web pages.
 *

 */

goog.provide('mr.ExternalMessageListener');

goog.require('mr.EventAnalytics');
goog.require('mr.EventListener');
goog.require('mr.InternalMessageType');
goog.require('mr.ModuleId');


mr.ExternalMessageListener = class extends mr.EventListener {
  constructor() {
    super(
        mr.EventAnalytics.Event.RUNTIME_ON_MESSAGE_EXTERNAL,
        'ExternalMessageListener', mr.ModuleId.PROVIDER_MANAGER,
        chrome.runtime.onMessageExternal);
  }

  /**
   * @override
   */
  validateEvent(message, sender, sendResponse) {
    // Make sure all messages have a sender |id| and that the ID is whitelisted.
    // If messages have a |tab| they are from a web page (most likely the Cast
    // setup page).
    if (!sender.id ||
        mr.ExternalMessageListener.WHITELIST_.indexOf(sender.id) == -1) {
      return false;
    }

    // Check if message type is valid.
    return message.type == mr.InternalMessageType.START ||
        message.type == mr.InternalMessageType.STOP ||
        message.type == mr.InternalMessageType.SUBSCRIBE_LOG_DATA;
  }

  /**
   * @override
   */
  deferredReturnValue() {
    // Indicates the messaging channel should be kept open until
    // sendResponse() is called.
    return true;
  }

  /** @return {!mr.ExternalMessageListener} */
  static get() {
    if (!mr.ExternalMessageListener.listener_) {
      mr.ExternalMessageListener.listener_ = new mr.ExternalMessageListener();
    }
    return mr.ExternalMessageListener.listener_;
  }
};


/** @private {?mr.ExternalMessageListener} */
mr.ExternalMessageListener.listener_ = null;


/**
 * List of app ids which are allowed to use the command messages.
 * These must also be included in the manifest 'externally_connectable' list.
 *
 * @private @const {!Array<string>}
 */
mr.ExternalMessageListener.WHITELIST_ = [
  // ghire kiosk app
  'idmofbkcelhplfjnmmdolenpigiiiecc',  // prod
  'ggedfkijiiammpnbdadhllnehapomdge',  // staging
  'njjegkblellcjnakomndbaloifhcoccg'   // dev
];
