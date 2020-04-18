// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Internal messages between parts of the extension, e.g., event
 * page, background page, e2e test page.
 *

 */

goog.provide('mr.InternalMessage');
goog.provide('mr.InternalMessageType');


/**
 * @enum {string}
 */
mr.InternalMessageType = {
  // Feedback ==> event page
  SUBSCRIBE_LOG_DATA: 'subscribe_log_data',
  RETRIEVE_LOG_DATA: 'retrieve_log_data',
  // Control Messages
  // App ==> Cloud MRP
  START: 'start',
  STOP: 'stop',
  // Responses
  // Cloud MRP ==> App
  ROUTE: 'route',
  STOPPED: 'stopped',
  ERROR: 'error',
  // App ==> Log Subscribers
  SUBSCRIBED: 'subscribed',
  LOG_MESSAGE: 'log_message',
};

mr.InternalMessage = class {
  /**
   * @param {string} source ID of source of message.
   * @param {mr.InternalMessageType} type The message type.
   * @param {*=} opt_message
   */
  constructor(source, type, opt_message) {
    /**
     * @type {string}
     * @export
     */
    this.source = source;

    /**
     * @type {mr.InternalMessageType}
     * @export
     */
    this.type = type;

    /**
     * @type {*}
     * @export
     */
    this.message = opt_message;
  }
};
