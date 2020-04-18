// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.



goog.provide('mr.MessagePortServiceImpl');

goog.require('mr.EventTarget');
goog.require('mr.MessagePort');
goog.require('mr.MessagePortService');
goog.require('mr.ProviderEventType');


/**
 * @template T
 * @implements {mr.MessagePort}
 * @private
 */
mr.MessagePortImpl_ = class {
  /**
   * @param {string} routeId
   * @param {function(string, (!Object|string), Object=): !Promise}
   *    sendMessage
   * @param {!mr.EventTarget} messageEventTarget
   */
  constructor(routeId, sendMessage, messageEventTarget) {
    /** @private {string} */
    this.routeId_ = routeId;

    /** @private {function(string, (!Object|string), Object=): !Promise} */
    this.sendMessage_ = sendMessage;

    /** @private {!mr.EventTarget} */
    this.messageEventTarget_ = messageEventTarget;

    /** @type {function((!Object|string))} */
    this.onMessage = () => {};

    messageEventTarget.listen(
        mr.ProviderEventType.INTERNAL_MESSAGE, this.onRouteMessage_, this);
  }

  /**
   * @override
   */
  sendMessage(message, opt_extraInfo) {
    return this.sendMessage_(this.routeId_, message, opt_extraInfo);
  }

  /**
   * Called when a message is received.
   *
   * @param {mr.InternalMessageEvent.<!Object|string>} event
   * @private
   */
  onRouteMessage_(event) {
    if (event.routeId != this.routeId_) {
      return;
    }
    this.onMessage(event.message);
  }

  /**
   * @override
   */
  dispose() {

    this.onMessage = () => {};
    this.messageEventTarget_.unlisten(
        mr.ProviderEventType.INTERNAL_MESSAGE, this.onRouteMessage_, this);
  }
};


/** @private @const {!mr.MessagePort} */
mr.MessagePortImpl_.NULL_INSTANCE_ =
    new mr.MessagePortImpl_('', () => Promise.resolve(), new mr.EventTarget());


/**
 * @implements {mr.MessagePortService}
 */
mr.MessagePortServiceImpl = class {
  /**
   * @param {!mr.ProviderManagerCallbacks} providerManagerCallbacks
   */
  constructor(providerManagerCallbacks) {
    /**
     * @private {!mr.ProviderManagerCallbacks}
     */
    this.providerManagerCallbacks_ = providerManagerCallbacks;
  }

  /**
   * @override
   */
  getInternalMessenger(routeId) {
    const provider =
        this.providerManagerCallbacks_.getProviderFromRouteId(routeId);
    return !provider ?
        mr.MessagePortImpl_.NULL_INSTANCE_ :
        new mr.MessagePortImpl_(
            routeId, provider.sendRouteMessage.bind(provider),
            this.providerManagerCallbacks_.getRouteMessageEventTarget());
  }
};
