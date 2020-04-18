// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Utilities for handling extension API errors.
 */

goog.module('mr.RunTimeErrorUtils');
goog.module.declareLegacyNamespace();

const Logger = goog.require('mr.Logger');


/**
 * Converts chrome.runtime.lastError into an Error object.  Should only be
 * called from an extension function callback that returns null or undefined.
 * By accessing chrome.runtime.lastError, this method has a side effect of
 * "handling" the error by preventing it from turning into an unchecked error.

 *
 * @param {string} functionName The name of the extension API function.
 * @param {!Logger=} logger If there is an error, logs a FINE error message
 *     with the logger, if provided.
 * @return {?Error} An Error object with chrome.runtime.lastError.message, if
 *     any.
 */
exports.getError = function(functionName, logger = undefined) {
  if (!chrome.runtime.lastError) {
    return null;
  }
  const message = functionName + ' failed, chrome.runtime.lastError: ' +
      (chrome.runtime.lastError.message || 'Unknown error');
  if (logger) {
    logger.fine(message);
  }
  return new Error(message);
};
