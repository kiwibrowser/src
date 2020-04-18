// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Utility methods for Objects.
 */

goog.provide('mr.ObjectUtils');

mr.ObjectUtils = class {
  /**
   * Gets the value in an Object with the given list of keys by traversing the
   * objects with them.
   * @param {!Object} obj
   * @param {...string} path
   * @return {*} The value in the object with the given path, or undefined if
   *     it does not exist due to missing either one of the intermediate keys
   *     or the final key.
   */
  static getPath(obj, ...path) {
    let value = obj;
    for (const key of path) {
      if (value == undefined || typeof value != 'object') {
        return undefined;
      }
      value = value[key];
    }
    return value;
  }
};
