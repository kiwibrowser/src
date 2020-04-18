// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * A tracker to substitute for analytics.Tracker in tests.
 * @construtor
 * @implements {analytics.Tracker}
 */
TestTracker = function() {};

/**
 * @param {!analytics.HitType|!analytics.EventBuilder} hitType
 * @param {(!analytics.ParameterMap|
 *     !Object<!analytics.Value>)=} opt_extraParams
 * @return {!goog.async.Deferred}
 */
TestTracker.prototype.send = function(hitType, opt_extraParams) {
};

/**
 * @param {string} description
 * @return {!goog.async.Deferred}
 */
TestTracker.prototype.sendAppView = function() {};

/**
 * @param {string} category
 * @param {string} action
 * @param {string=} opt_label
 * @param {number=} opt_value
 * @return {!goog.async.Deferred}
 */
TestTracker.prototype.sendEvent = function() {};

/**
 * @param {string} network Specifies the social network, for example Facebook
 *     or Google Plus.
 * @param {string} action Specifies the social interaction action.
 *     For example on Google Plus when a user clicks the +1 button,
 *     the social action is 'plus'.
 * @param {string} target Specifies the target of a social interaction.
 *     This value is typically a URL but can be any text.
 * @return {!goog.async.Deferred}
 */
TestTracker.prototype.sendSocial = function() {};

/**
 * @param {string=} opt_description Specifies the description of an exception.
 * @param {boolean=} opt_fatal Was the exception fatal.
 * @return {!goog.async.Deferred}
 */
TestTracker.prototype.sendException = function() {};

/**
 * @param {string} category Specifies the category of the timing.
 * @param {string} variable Specifies the variable name of the timing.
 * @param {number} value Specifies the value of the timing.
 * @param {string=} opt_label Specifies the optional label of the timing.
 * @param {number=} opt_sampleRate
 * @return {!goog.async.Deferred}
 */
TestTracker.prototype.sendTiming = function() {};

TestTracker.prototype.forceSessionStart = function() {};

/**
 * @param {string} category
 * @param {string} variable
 * @param {string=} opt_label
 * @param {number=} opt_sampleRate
 * @return {!TestTracker.Timing}
 */
TestTracker.prototype.startTiming = function() {
  return /** @type {!TestTracker.Timing} */ ({
    send: function() {}
  });
};
