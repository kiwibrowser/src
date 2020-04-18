// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.module('mr.DeviceCounts');
goog.module.declareLegacyNamespace();


/**
 * A struct to hold a snapshot of device counts for a sink discovery service.
 * @typedef {{
 *   availableDeviceCount: number,
 *   knownDeviceCount: number
 * }}
 */
let DeviceCounts;

exports = DeviceCounts;
