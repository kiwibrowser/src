// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var goog = {};
goog.async = {};

/**
 * @constructor
 * @template VALUE
 */
goog.async.Deferred;

/**
 * @param {!function(this:T,VALUE):?} cb
 * @param {T=} opt_scope
 * @return {!goog.async.Deferred}
 * @template T
 */
goog.async.Deferred.prototype.addCallback;

/** @param {VALUE=} opt_result */
goog.async.Deferred.prototype.callback;

/**
 * @param {?(function(this:THIS, VALUE):
 *             (RESULT|IThenable<RESULT>|Thenable))=} opt_onFulfilled
 * @param {?(function(this:THIS, *): *)=} opt_onRejected
 * @param {THIS=} opt_context
 * @return {!Promise<RESULT>}
 * @template RESULT,THIS
 */
goog.async.Deferred.prototype.then;

var analytics = {};

/** @typedef {string} */
analytics.HitType;

/** @enum {analytics.HitType} */
analytics.HitTypes = {
  APPVIEW: 'appview',
  EVENT: 'event',
  SOCIAL: 'social',
  TRANSACTION: 'transaction',
  ITEM: 'item',
  TIMING: 'timing',
  EXCEPTION: 'exception'
};

/**
 * @typedef {{
 *   id: string,
 *   name: string,
 *   valueType: analytics.ValueType,
 *   maxLength: (number|undefined),
 *   defaultValue: (string|undefined)
 * }}
 */
analytics.Parameter;

/** @typedef {string|number|boolean} */
analytics.Value;

/** @typedef {string} */
analytics.ValueType;


/**
 * @param {string} appName
 * @param {string=} opt_appVersion
 * @return {!analytics.GoogleAnalytics}
 */
analytics.getService;


/** @interface */
analytics.GoogleAnalytics;

/**
 * @param {string} trackingId
 * @return {!analytics.Tracker}
 */
analytics.GoogleAnalytics.prototype.getTracker;

/** @return {!goog.async.Deferred.<!analytics.Config>} */
analytics.GoogleAnalytics.prototype.getConfig;


/** @interface */
analytics.Tracker;

/** @typedef {function(!analytics.Tracker.Hit)} */
analytics.Tracker.Filter;

/**
 * @param {!analytics.HitType|!analytics.EventBuilder} hitType
 * @param {(!analytics.ParameterMap|
 *     !Object.<string, !analytics.Value>)=} opt_extraParams
 * @return {!goog.async.Deferred}
 */
analytics.Tracker.prototype.send;

/**
 * @param {string} description
 * @return {!goog.async.Deferred}
 */
analytics.Tracker.prototype.sendAppView;

/**
 * @param {string} category
 * @param {string} action
 * @param {string=} opt_label
 * @param {number=} opt_value
 * @return {!goog.async.Deferred}
 */
analytics.Tracker.prototype.sendEvent;

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
analytics.Tracker.prototype.sendSocial;

/**
 * @param {string=} opt_description Specifies the description of an exception.
 * @param {boolean=} opt_fatal Was the exception fatal.
 * @return {!goog.async.Deferred}
 */
analytics.Tracker.prototype.sendException;

/**
 * @param {string} category Specifies the category of the timing.
 * @param {string} variable Specifies the variable name of the timing.
 * @param {number} value Specifies the value of the timing.
 * @param {string=} opt_label Specifies the optional label of the timing.
 * @param {number=} opt_sampleRate
 * @return {!goog.async.Deferred}
 */
analytics.Tracker.prototype.sendTiming;

analytics.Tracker.prototype.forceSessionStart;

/**
 * @param {string} category
 * @param {string} variable
 * @param {string=} opt_label
 * @param {number=} opt_sampleRate
 * @return {!analytics.Tracker.Timing}
 */
analytics.Tracker.prototype.startTiming;

/** @interface */
analytics.Tracker.Timing;

/** @return {!goog.async.Deferred} */
analytics.Tracker.Timing.prototype.send;

/** @param {!analytics.Tracker.Filter} filter */
analytics.Tracker.prototype.addFilter;

/** @interface */
analytics.Tracker.Hit;

/** @return {!analytics.HitType} */
analytics.Tracker.Hit.prototype.getHitType;

/** @return {!analytics.ParameterMap} */
analytics.Tracker.Hit.prototype.getParameters;

analytics.Tracker.Hit.prototype.cancel;


/** @interface */
analytics.Config = function() {};

/** @param {boolean} permitted */
analytics.Config.prototype.setTrackingPermitted;

/** @return {boolean} */
analytics.Config.prototype.isTrackingPermitted;

/** @param {number} sampleRate */
analytics.Config.prototype.setSampleRate;

/** @return {!goog.async.Deferred} Settles once the id has been reset. */
analytics.Config.prototype.resetUserId;


/** @interface */
analytics.ParameterMap;

/**
 * @typedef {{
 *   key: !analytics.Parameter,
 *   value: !analytics.Value
 * }}
 */
analytics.ParameterMap.Entry;

/**
 * @param {!analytics.Parameter} param
 * @param {!analytics.Value} value
 */
analytics.ParameterMap.prototype.set;

/**
 * @param {!analytics.Parameter} param
 * @return {?analytics.Value}
 */
analytics.ParameterMap.prototype.get;

/** @param {!analytics.Parameter} param */
analytics.ParameterMap.prototype.remove;

/** @return {!Object.<string, analytics.Value>} */
analytics.ParameterMap.prototype.toObject;


/** @interface */
analytics.EventBuilder;

/** @typedef {{ index: number, value: string }} */
analytics.EventBuilder.Dimension;

/** @typedef {{ index: number, value: number }} */
analytics.EventBuilder.Metric;

/** @return {!analytics.EventBuilder} */
analytics.EventBuilder.builder;

/**
 * @param {string} category
 * @return {!analytics.EventBuilder}
 */
analytics.EventBuilder.prototype.category;

/**
 * @param {string} action
 * @return {!analytics.EventBuilder}
 */
analytics.EventBuilder.prototype.action;

/**
 * @param {string} label
 * @return {!analytics.EventBuilder}
 */
analytics.EventBuilder.prototype.label;

/**
 * @param {number} value
 * @return {!analytics.EventBuilder}
 */
analytics.EventBuilder.prototype.value;

/**
 * @param {!analytics.EventBuilder.Dimension} dimension
 * @return {!analytics.EventBuilder}
 */
analytics.EventBuilder.prototype.dimension;

/**
 * @param {!analytics.EventBuilder.Metric} metric
 * @return {!analytics.EventBuilder}
 */
analytics.EventBuilder.prototype.metric;

/**
 * @param {!analytics.Tracker} tracker
 * @return {!goog.async.Deferred}
 */
analytics.EventBuilder.prototype.send;

/** @param {!analytics.ParameterMap} parameters */
analytics.EventBuilder.prototype.collect;
