// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Provider events.

 */

goog.provide('mr.InternalMessageEvent');
goog.provide('mr.ProviderEventType');


/**
 * @enum {string}
 */
mr.ProviderEventType = {
  INTERNAL_MESSAGE: 'internal_message'
};


/**
 * @template T
 */
mr.InternalMessageEvent = class {
  /**
   * @param {string} routeId
   * @param {T} message
   */
  constructor(routeId, message) {
    /**
     * @const
     */
    this.type = mr.ProviderEventType.INTERNAL_MESSAGE;

    /**
     * @type {string}
     */
    this.routeId = routeId;

    /**
     * @type {T}
     */
    this.message = message;
  }
};
