// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Analytics for events. */

goog.provide('mr.EventAnalytics');
goog.provide('mr.EventAnalytics.Event');

goog.require('mr.Analytics');

/**
 * Possible event types that can wake the event page.  Keep names in sync with
 * extensions/browser/extension_event_histogram_value.h in Chromium and values
 * in sync with MediaRouterWakeEventType in
 * google3/analysis/uma/configs/chrome/histograms.xml.
 *
 * @enum {number}
 */
mr.EventAnalytics.Event = {
  // Special value meaning the event page was woken by the Media Router and not
  // a regular extension event.
  MEDIA_ROUTER: 0,
  CAST_CHANNEL_ON_ERROR: 1,
  CAST_CHANNEL_ON_MESSAGE: 2,
  DIAL_ON_DEVICE_LIST: 3,
  DIAL_ON_ERROR: 4,
  GCM_ON_MESSAGE: 5,
  IDENTITY_ON_SIGN_IN_CHANGED: 6,
  MDNS_ON_SERVICE_LIST: 7,
  NETWORKING_PRIVATE_ON_NETWORKS_CHANGED: 8,
  NETWORKING_PRIVATE_ON_NETWORK_LIST_CHANGED: 9,
  PROCESSES_ON_UPDATED: 10,
  RUNTIME_ON_MESSAGE: 11,
  RUNTIME_ON_MESSAGE_EXTERNAL: 12,
  SETTINGS_PRIVATE_ON_PREFS_CHANGED: 13,
  TABS_ON_UPDATED: 14,
};

/**
 * @private {mr.EventAnalytics.Event} The event that woke the event page.
 */
mr.EventAnalytics.firstEvent_;

/**
 * Records an event handler invocation in the event page.  If it is the first
 * event that woke the page, a histogram is recorded.  Subsequent events are a
 * no-op.
 *
 * @param {!mr.EventAnalytics.Event} eventType The event type.
 */
mr.EventAnalytics.recordEvent = function(eventType) {
  if (mr.EventAnalytics.firstEvent_ != undefined) return;
  mr.Analytics.recordEnum(
      'MediaRouter.Provider.WakeEvent', eventType, mr.EventAnalytics.Event);
  mr.EventAnalytics.firstEvent_ = eventType;
};
