// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Handles internal messages that arrive in the
 * chrome.runtime.onMessage event. It is assumed that all incoming messages have
 * type mr.InternalMessage.
 */

goog.provide('mr.InternalMessageListener');

goog.require('mr.EventAnalytics');
goog.require('mr.EventListener');
goog.require('mr.InternalMessage');
goog.require('mr.InternalMessageType');


/**
 * @extends {mr.EventListener<ChromeEvent>}
 */
mr.InternalMessageListener = class extends mr.EventListener {
  constructor() {
    super(
        mr.EventAnalytics.Event.RUNTIME_ON_MESSAGE, 'InternalMessageListener',
        mr.ModuleId.PROVIDER_MANAGER, chrome.runtime.onMessage);
  }

  /**
   * @override
   */
  validateEvent(message, sender, sendResponse) {
    const internalMessage = /** @type {mr.InternalMessage} */ (message);
    return internalMessage.type == mr.InternalMessageType.RETRIEVE_LOG_DATA &&
        sender.id == chrome.runtime.id &&
        sender.url == `chrome-extension://${sender.id}/feedback.html`;
  }

  /**
   * @override
   */
  deferredReturnValue() {
    // Indicates the messaging channel should be kept open until
    // sendResponse() is called.
    return true;
  }

  /**
   * @return {!mr.InternalMessageListener}
   */
  static get() {
    if (!mr.InternalMessageListener.listener_) {
      mr.InternalMessageListener.listener_ = new mr.InternalMessageListener();
    }
    return mr.InternalMessageListener.listener_;
  }
};


/** @private {?mr.InternalMessageListener} */
mr.InternalMessageListener.listener_ = null;
