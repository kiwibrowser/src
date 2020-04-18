// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * Provides WebviewEventManager which can register and keep track of listeners
 * on EventTargets and WebRequests, and unregister all listeners later.
 */
'use strict';

/**
 * Creates a new WebviewEventManager.
 */
function WebviewEventManager() {
  this.unbindWebviewCleanupFunctions_ = [];
}

WebviewEventManager.prototype = {
  /**
   * Adds a EventListener to |eventTarget| and adds a clean-up function so we
   * can remove the listener in unbindFromWebview.
   * @param {Object} webview the object to add the listener to
   * @param {string} type the event type
   * @param {Function} listener the event listener
   * @private
   */
  addEventListener: function(eventTarget, type, listener) {
    eventTarget.addEventListener(type, listener);
    this.unbindWebviewCleanupFunctions_.push(
        eventTarget.removeEventListener.bind(eventTarget, type, listener));
  },

  /**
   * Adds a listener to |webRequestEvent| and adds a clean-up function so we can
   * remove the listener in unbindFromWebview.
   * @param {Object} webRequestEvent the object to add the listener to
   * @param {string} type the event type
   * @param {Function} listener the event listener
   * @private
   */
  addWebRequestEventListener: function(
      webRequestEvent, listener, filter, extraInfoSpec) {
    webRequestEvent.addListener(listener, filter, extraInfoSpec);
    this.unbindWebviewCleanupFunctions_.push(
        webRequestEvent.removeListener.bind(webRequestEvent, listener));
  },

  /**
   * Unbinds this Authenticator from the currently bound webview.
   * @private
   */
  removeAllListeners: function() {
    for (var i = 0; i < this.unbindWebviewCleanupFunctions_.length; i++)
      this.unbindWebviewCleanupFunctions_[i]();
    this.unbindWebviewCleanupFunctions_ = [];
  }
};

/**
 * Class factory.
 * @return {WebviewEventManager}
 */
WebviewEventManager.create = function() {
  return new WebviewEventManager();
};
