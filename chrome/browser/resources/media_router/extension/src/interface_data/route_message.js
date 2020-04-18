// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview The route message object shared between component extension
 *  and Chrome media router.
 */

goog.provide('mr.RouteMessage');

mr.RouteMessage = class {
  /**
   * @param {string} routeId
   * @param {string|!Uint8Array} message
   */
  constructor(routeId, message) {
    /**
     * @type {string}
     * @export
     */
    this.routeId = routeId;

    /**
     * @type {string|!Uint8Array}
     * @export
     */
    this.message = message;
  }

  /**
   * @param {!mr.RouteMessage} routeMessage
   * @return {boolean} true if the message is in binary format.
   */
  static isBinary(routeMessage) {
    return typeof routeMessage.message != 'string';
  }

  /**
   * @param {!mr.RouteMessage} routeMessage
   * Returns the length of the message if it is a string, otherwise 0.
   * @return {number}
   */
  static stringLength(routeMessage) {
    return mr.RouteMessage.isBinary(routeMessage) ? 0 :
                                                    routeMessage.message.length;
  }
};
