// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview The sink search criteria shared between component extension and
 * Chrome media router.
 */

goog.provide('mr.SinkSearchCriteria');

mr.SinkSearchCriteria = class {
  /**
   * @param {string} input
   * @param {?string} domain
   */
  constructor(input, domain) {
    /**
     * @type {string}
     * @export
     */
    this.input = input;

    /**
     * @type {?string}
     * @export
     */
    this.domain = domain;
  }
};
