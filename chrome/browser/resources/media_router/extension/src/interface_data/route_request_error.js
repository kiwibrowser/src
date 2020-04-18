// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Extension of Error for route request errors.
 */

goog.provide('mr.RouteRequestError');
goog.provide('mr.RouteRequestResultCode');


/**
 * Keep in sync with:
 *   - RouteRequestResultCode in media_router.mojom
 *   - RouteRequestResult::ResultCode in route_request_result.h
 *   - MediaRouteProviderResult enum in tools/metrics/histograms.xml
 * @enum {number}
 */
mr.RouteRequestResultCode = {
  UNKNOWN_ERROR: 0,
  OK: 1,
  TIMED_OUT: 2,
  ROUTE_NOT_FOUND: 3,
  SINK_NOT_FOUND: 4,
  INVALID_ORIGIN: 5,
  OFF_THE_RECORD_MISMATCH: 6,
  NO_SUPPORTED_PROVIDER: 7,
  CANCELLED: 8,
};

mr.RouteRequestError = class extends Error {
  /**
   * @param {!mr.RouteRequestResultCode} errorCode
   * @param {string=} opt_message
   * @param {string=} opt_stack
   */
  constructor(errorCode, opt_message, opt_stack) {
    super();

    this.name = 'RouteRequestError';
    this.message = opt_message || '';
    if (opt_stack) {
      this.stack = opt_stack;
    } else {
      // Attempt to ensure there is a stack trace.
      if (Error.captureStackTrace) {
        Error.captureStackTrace(this, mr.RouteRequestError);
      } else {
        const stack = new Error().stack;
        if (stack) {
          this.stack = stack;
        }
      }
    }

    /** @type {!mr.RouteRequestResultCode} */
    this.errorCode = errorCode;
  }

  /**
   * If the given error is a mr.RouteRequestError, returns it. Otherwise, a
   * returns a mr.RouteRequestError with the error's message and UNKNOWN error
   * code.
   * @param {*} error
   * @return {!mr.RouteRequestError}
   */
  static wrap(error) {
    if (error instanceof mr.RouteRequestError) {
      return error;
    } else if (error instanceof Error) {
      return new mr.RouteRequestError(
          mr.RouteRequestResultCode.UNKNOWN_ERROR, error.message, error.stack);
    } else {
      return new mr.RouteRequestError(mr.RouteRequestResultCode.UNKNOWN_ERROR);
    }
  }

  /**
   * @param {*} error Possibly an instance of mr.RouteRequestError.
   * @return {boolean} True if the argument represents a timeout.
   */
  static isTimeout(error) {
    if (error instanceof mr.RouteRequestError) {
      return error.errorCode == mr.RouteRequestResultCode.TIMED_OUT;
    }
    return false;
  }
};
