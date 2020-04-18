// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview DIAL provider callbacks used for other DIAL services to inform
 *  activity or sink updates.
 */

goog.provide('mr.dial.ActivityCallbacks');
goog.provide('mr.dial.ProviderCallbacks');
goog.provide('mr.dial.SinkDiscoveryCallbacks');



/**
 * @record
 */
mr.dial.ActivityCallbacks = class {
  /**
   * @param {!mr.dial.Activity } activity
   */
  onActivityAdded(activity) {}

  /**
   * @param {!mr.dial.Activity } activity
   */
  onActivityRemoved(activity) {}

  /**
   * @param {!mr.dial.Activity } activity
   */
  onActivityUpdated(activity) {}
};



/**
 * @record
 */
mr.dial.SinkDiscoveryCallbacks = class {
  /**
   * @param {!mr.dial.Sink} sink Sink that has been added.
   */
  onSinkAdded(sink) {}

  /**
   * @param {!Array.<!mr.dial.Sink>} sinks Sinks that have been removed.
   */
  onSinksRemoved(sinks) {}

  /**
   * @param {!mr.dial.Sink} sink Sink that has been updated.
   */
  onSinkUpdated(sink) {}
};



/**
 * @record
 * @extends {mr.dial.ActivityCallbacks}
 * @extends {mr.dial.SinkDiscoveryCallbacks}
 */
mr.dial.ProviderCallbacks = class {};
