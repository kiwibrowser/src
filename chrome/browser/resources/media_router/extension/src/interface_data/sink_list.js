// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Data structure returned by mr.Provider.getAvailableSinks.
 */

goog.provide('mr.SinkList');

mr.SinkList = class {
  /**
   * @param {!Array<mr.Sink>} sinks
   * @param {Array<string>=} opt_origins List of origins that have access to the
   * sink list. If not provided, the sink list is accessible from all origins.
   */
  constructor(sinks, opt_origins) {
    /**
     * @type {!Array<!mr.Sink>}
     * @export
     */
    this.sinks = sinks;

    /**
     * @type {?Array<string>}
     * @export
     */
    this.origins = opt_origins || null;
  }
};


/** @const {!mr.SinkList} */
mr.SinkList.EMPTY = new mr.SinkList([]);
