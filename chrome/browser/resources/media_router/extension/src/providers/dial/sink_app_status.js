// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview The availability of an app on a sink.
 */

goog.provide('mr.dial.SinkAppStatus');


/**
 * Tracks the availability of an app on a sink.  Apps start out in an
 * UNKNOWN status and are changed to AVAILABLE or UNAVAILABLE once the status is
 * known, i.e. after we query the sink for the app.
 *
 * @enum {string}
 */
mr.dial.SinkAppStatus = {
  AVAILABLE: 'available',
  UNAVAILABLE: 'unavailable',
  UNKNOWN: 'unknown'
};
