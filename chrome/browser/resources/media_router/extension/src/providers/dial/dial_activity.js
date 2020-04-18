// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview DIAL activity (locally launched or discovered).
 */

goog.provide('mr.dial.Activity');

mr.dial.Activity = class {
  /**
   * @param {!mr.Route} route
   * @param {!string} appName
   */
  constructor(route, appName) {
    /** @type {!mr.Route} */
    this.route = route;
    /** @type {string} */
    this.appName = appName;
  }
};
