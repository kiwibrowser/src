// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Define accessibility tests for the MANAGE_TTS_SETTINGS route.
 */

// This is only for Chrome OS.
GEN('#if defined(OS_CHROMEOS)');

// SettingsAccessibilityTest fixture.
GEN_INCLUDE([
  'settings_accessibility_test.js',
]);

TtsAccessibilityTest = class extends SettingsAccessibilityTest {
  /** @override */
  get commandLineSwitches() {
    return ['enable-experimental-a11y-features'];
  }
};

AccessibilityTest.define('TtsAccessibilityTest', {
  /** @override */
  name: 'MANAGE_TTS_SETTINGS',
  /** @override */
  axeOptions: SettingsAccessibilityTest.axeOptions,
  /** @override */
  setup: function() {
    settings.router.navigateTo(settings.routes.MANAGE_TTS_SETTINGS);
    Polymer.dom.flush();
  },
  /** @override */
  tests: {'Accessible with No Changes': function() {}},
  /** @override */
  violationFilter: SettingsAccessibilityTest.violationFilter,
});

GEN('#endif  // defined(OS_CHROMEOS)');
