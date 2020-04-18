// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Defines UMA analytics specific to the DIAL provider.
 */

goog.provide('mr.DialAnalytics');

goog.require('mr.Analytics');


/**
 * Contains all analytics logic for the DIAL provider.
 * @const {*}
 */
mr.DialAnalytics = {};


/** @enum {string} */
mr.DialAnalytics.Metric = {
  DEVICE_DESCRIPTION_FAILURE: 'MediaRouter.Dial.Device.Description.Failure',
  DEVICE_DESCRIPTION_FROM_CACHE: 'MediaRouter.Dial.Device.Description.Cached',
  DIAL_CREATE_ROUTE: 'MediaRouter.Dial.Create.Route',
  NON_CAST_DISCOVERY: 'MediaRouter.Dial.Sink.Discovered.NonCast'
};


/**
 * Possible values for the route creation analytics.
 * @enum {number}
 */
mr.DialAnalytics.DialRouteCreation = {
  FAILED_NO_SINK: 0,
  ROUTE_CREATED: 1,
  NO_APP_INFO: 2,
  FAILED_LAUNCH_APP: 3
};


/**
 * Possible values for device description failures.
 * @enum {number}
 */
mr.DialAnalytics.DeviceDescriptionFailures = {
  ERROR: 0,
  PARSE: 1,
  EMPTY: 2
};


/**
 * Records analytics around route creation.
 * @param {mr.DialAnalytics.DialRouteCreation} value
 */
mr.DialAnalytics.recordCreateRoute = function(value) {
  mr.Analytics.recordEnum(
      mr.DialAnalytics.Metric.DIAL_CREATE_ROUTE, value,
      mr.DialAnalytics.DialRouteCreation);
};


/**
 * Records a failure with the device description.
 * @param {!mr.DialAnalytics.DeviceDescriptionFailures} value The failure
 * reason.
 */
mr.DialAnalytics.recordDeviceDescriptionFailure = function(value) {
  mr.Analytics.recordEnum(
      mr.DialAnalytics.Metric.DEVICE_DESCRIPTION_FAILURE, value,
      mr.DialAnalytics.DeviceDescriptionFailures);
};


/**
 * Records that device description was retreived from the cache.
 */
mr.DialAnalytics.recordDeviceDescriptionFromCache = function() {
  mr.Analytics.recordEvent(
      mr.DialAnalytics.Metric.DEVICE_DESCRIPTION_FROM_CACHE);
};


/**
 * Records that a device was discovered by DIAL that didn't support the cast
 * protocol.
 */
mr.DialAnalytics.recordNonCastDiscovery = function() {
  mr.Analytics.recordEvent(mr.DialAnalytics.Metric.NON_CAST_DISCOVERY);
};


/**
 * Histogram name for available DIAL devices count.
 * @private @const {string}
 */
mr.DialAnalytics.AVAILABLE_DEVICES_COUNT_ =
    'MediaRouter.Dial.AvailableDevicesCount';


/**
 * Histogram name for known DIAL devices count.
 * @private @const {string}
 */
mr.DialAnalytics.KNOWN_DEVICES_COUNT_ = 'MediaRouter.Dial.KnownDevicesCount';


/**
 * Records device counts.
 * @param {!mr.DeviceCounts} deviceCounts
 */
mr.DialAnalytics.recordDeviceCounts = function(deviceCounts) {
  mr.Analytics.recordSmallCount(
      mr.DialAnalytics.AVAILABLE_DEVICES_COUNT_,
      deviceCounts.availableDeviceCount);
  mr.Analytics.recordSmallCount(
      mr.DialAnalytics.KNOWN_DEVICES_COUNT_, deviceCounts.knownDeviceCount);
};
