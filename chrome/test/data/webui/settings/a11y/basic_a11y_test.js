// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Define accessibility tests for the BASIC route.
 */

// SettingsAccessibilityTest fixture.
GEN_INCLUDE([
  'settings_accessibility_test.js',
]);

AccessibilityTest.define('SettingsAccessibilityTest', {
  /** @override */
  name: 'BASIC',
  /** @override */
  axeOptions: SettingsAccessibilityTest.axeOptions,
  /** @override */
  tests: {'Accessible with No Changes': function() {}},
  /** @override */
  violationFilter: SettingsAccessibilityTest.violationFilter,
});
