// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Defines UMA analytics specific to Mirroring.
 */

goog.provide('mr.MirrorAnalytics');
goog.provide('mr.mirror.Error');

goog.require('mr.Analytics');


/**
 * Contains all common analytics logic for Mirroring.
 * @const {*}
 */
mr.MirrorAnalytics = {};


/**
 * Histogram name for mirroring start failure.
 * @private @const {string}
 */
mr.MirrorAnalytics.CAPTURING_FAILURE_HISTOGRAM_ =
    'MediaRouter.Mirror.Capturing.Failure';


/**
 * Possible values for the start failure analytics.
 * @enum {number}
 */
mr.MirrorAnalytics.CapturingFailure = {
  CAPTURE_TAB_FAIL_EMPTY_STREAM: 0,
  CAPTURE_DESKTOP_FAIL_ERROR_TIMEOUT: 1,
  CAPTURE_TAB_TIMEOUT: 2,
  CAPTURE_DESKTOP_FAIL_ERROR_USER_CANCEL: 3,
  ANSWER_NOT_RECEIVED: 4,
  CAPTURE_TAB_FAIL_ERROR_TIMEOUT: 5,
  ICE_CONNECTION_CLOSED: 6,
  TAB_FAIL: 7,
  DESKTOP_FAIL: 8,
  UNKNOWN: 9,
};


/**
 * Records a mirroring start failure.
 * @param {mr.MirrorAnalytics.CapturingFailure} reason The type of failure.
 * @param {string} name The name of the histogram.
 */
mr.MirrorAnalytics.recordCapturingFailureWithName = function(reason, name) {
  mr.Analytics.recordEnum(name, reason, mr.MirrorAnalytics.CapturingFailure);
};


/**
 * Error thrown when initiating a mirroring session.
 */
mr.mirror.Error = class extends Error {
  /**
   * @param {string} message The error message.
   * @param {mr.MirrorAnalytics.CapturingFailure=} reason The failure reason.
   */
  constructor(message, reason = undefined) {
    super(message);
    /** {!mr.MirrorAnalytics.CapturingFailure} The failure reason. */
    this.reason =
        (reason >= 0 && reason <= mr.MirrorAnalytics.CapturingFailure.UNKNOWN) ?
        reason :
        mr.MirrorAnalytics.CapturingFailure.UNKNOWN;
  }
};
