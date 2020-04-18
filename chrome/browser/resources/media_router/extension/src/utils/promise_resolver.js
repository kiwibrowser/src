// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**

 */
goog.module('mr.PromiseResolver');
goog.module.declareLegacyNamespace();


/**
 * Wrapper around a Promise that allows the Promise to be resolved by
 * calling a method.
 *
 * @template T
 */
exports = class {
  constructor() {
    /**
     * @private {function(T)}
     */
    this.resolveFunc_;

    /**
     * @private {function(*)}
     */
    this.rejectFunc_;

    /**
     * @const {!Promise<T>}
     */
    this.promise = new Promise((resolve, reject) => {
      this.resolveFunc_ = resolve;
      this.rejectFunc_ = reject;
    });
  }

  /**
   * Resolves the promise.
   * @param {T} value
   */
  resolve(value) {
    this.resolveFunc_(value);
  }

  /**
   * Rejects the promise.
   * @param {*} reason
   */
  reject(reason) {
    this.rejectFunc_(reason);
  }
};
