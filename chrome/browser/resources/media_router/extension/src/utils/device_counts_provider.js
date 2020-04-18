// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.module('mr.DeviceCountsProvider');
goog.module.declareLegacyNamespace();

const DeviceCounts = goog.require('mr.DeviceCounts');

/**
 * Implemented by services that are capable of providing counts of devices that
 * they manage.
 * @record
 */
const DeviceCountsProvider = class {
  /**
   * Returns the device counts currently known to the service.
   * @return {!DeviceCounts}
   */
  getDeviceCounts() {}
};

exports = DeviceCountsProvider;
