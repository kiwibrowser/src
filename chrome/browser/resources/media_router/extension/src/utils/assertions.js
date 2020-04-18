// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Various assert-like functions.
 */
goog.module('mr.Assertions');
goog.module.declareLegacyNamespace();

const Config = goog.require('mr.Config');


/**
 * Given an unknown value, return it if it is an Error, or return a new Error
 * otherwise.  Note that unlike other methods in the module, it does not throw
 * exceptions.
 *
 * @param {*} err The purported error
 * @param {string=} opt_message The message used to construct an Error if |err|
 *     is not an error.
 * @return {!Error}
 */
exports.toError = function(err, opt_message) {
  if (err instanceof Error) {
    return err;
  } else {
    return Error(opt_message || `Expected an Error value, got ${err}`);
  }
};


/**
 * Represents an assertion failure.
 */
const AssertionError = class extends Error {
  /**
   * @param {string=} message Error message.
   */
  constructor(message = '') {
    super();
    this.name = 'AssertionError';
    this.message = message;
    if (Error.captureStackTrace) {
      Error.captureStackTrace(this, AssertionError);
    } else {
      this.stack = new Error().stack;
    }
  }
};


/**
 * Checks if the condition evaluates to true if mr.Config.isDebugChannel is
 * true.
 * @template T
 * @param {T} condition The condition to check.
 * @param {string=} message Error message if condition evaluates to false.
 * @throws {AssertionError} When the condition evaluates to false.
 * @return {T} The condition.
 */
exports.assert = function(condition, message = undefined) {
  if (Config.isDebugChannel && !condition) {
    throw new AssertionError(message);
  }
  return condition;
};


/**
 * Checks that a value is a string if mr.Config.isDebugChannel is true.
 * @param {*} value The value to check
 * @param {string=} message The message
 * @return {string} The value
 * @throws {AssertionError} if the value is not a string
 */
exports.assertString = function(value, message = undefined) {
  if (Config.isDebugChannel && typeof value !== 'string') {
    throw new AssertionError();
  }
  return /** @type {string} */ (value);
};


/**
 * Returns a Promise that rejects with 'Not implemented' as the error
 * message.
 * @template T
 * @return {!Promise<T>}
 */
exports.rejectNotImplemented = function() {
  return Promise.reject(new Error('Not implemented'));
};

exports.AssertionError = AssertionError;
