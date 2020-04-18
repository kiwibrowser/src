// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview API for posting messages to a route and getting messages from
 *  the route.
 */

goog.provide('mr.MessagePort');
goog.provide('mr.MessagePortService');



/**
 * @record
 */
mr.MessagePort = class {
  /**
   * Called to post a message to the sink via a media route.
   * @param {!Object|string} message The message to post. Object will be
   *  serialized to a JSON string.
   * @param {Object=} opt_extraInfo Extra info about how to send a message.
   * @return {!Promise} Fulfilled when the message is posted, or rejected
   *   if there an error.
   */
  sendMessage(message, opt_extraInfo) {}

  /**
   * Releases any resources used by this object.
   */
  dispose() {}
};


/**
 * The message handler to invoke when messages associated
 *  with the route arrives.
 * @type {function((!Object|string))}
 */
mr.MessagePort.prototype.onMessage;



/**
 * @record
 */
mr.MessagePortService = class {
  /**
   * Returns the MessagePort for internal communication with the provider.
   * @param {string} routeId
   * @return {!mr.MessagePort}
   */
  getInternalMessenger(routeId) {}
  /**
   * @return {!mr.MessagePortService}
   */
  static getService() {
    return mr.MessagePortService.service_;
  }

  /**
   * @param {!mr.MessagePortService} service
   */
  static setService(service) {
    if (!mr.MessagePortService.service_) {
      mr.MessagePortService.service_ = service;
    }
  }
};


/** @private {!mr.MessagePortService} */
mr.MessagePortService.service_;
