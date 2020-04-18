// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Define accessibility tests for the MANAGE_PROFILE route.
 */

// The MANAGE_PROFILE route is non-Chrome OS only.
GEN('#if !defined(OS_CHROMEOS)');

// SettingsAccessibilityTest fixture.
GEN_INCLUDE([
  'settings_accessibility_test.js',
]);

AccessibilityTest.define('SettingsAccessibilityTest', {
  /** @override */
  name: 'MANAGE_PROFILE',
  /** @override */
  axeOptions: SettingsAccessibilityTest.axeOptions,
  /** @override */
  setup: function() {
    settings.navigateTo(settings.routes.MANAGE_PROFILE);
    Polymer.dom.flush();
  },
  /** @override */
  tests: {'Accessible with No Changes': function() {}},
  /** @override */
  violationFilter:
      Object.assign({}, SettingsAccessibilityTest.violationFilter, {
        // Excuse Polymer paper-input elements.
        'aria-valid-attr-value': function(nodeResult) {
          const describerId =
              nodeResult.element.getAttribute('aria-describedby');
          return describerId === '' && nodeResult.element.tagName == 'INPUT';
        },
        'tabindex': function(nodeResult) {
          // TODO(crbug.com/808276): remove this exception when bug is fixed.
          return nodeResult.element.getAttribute('tabindex') == '0';
        },
      }),
});

GEN('#endif  // !defined(OS_CHROMEOS)');
