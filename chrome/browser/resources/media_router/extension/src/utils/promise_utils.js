// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Utility functions for dealing with promises.
 */
goog.module('mr.PromiseUtils');
goog.module.declareLegacyNamespace();


/**
 * Given a list of promises, waits for all promises to settle, and produces a
 * list indicating whether each input promise was fulfilled (i.e. resolved) or
 * rejected.
 *
 * Each object in the output contains two fields: the |fulfilled| field is true
 * if the corresponding input promise was resolved, or false if it was rejected.
 * When |fulfilled| is true, the |value| field contains the value with which the
 * promise was resolved.  Otherwise, the |reason| field contains the reason with
 * which the promise was rejected.
 *
 * @param {!Array<!Promise<T>>} promises
 * @return {!Promise<!Array<{
 *     fulfilled: boolean,
 *     value: (T | undefined),
 *     reason: *
 * }>>}
 * @template T
 */
exports.allSettled = function(promises) {
  return Promise.all(promises.map(
      promise => promise.then(
          value => ({fulfilled: true, value: value}),
          reason => ({fulfilled: false, reason: reason}))));
};
