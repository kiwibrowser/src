// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Runs the Polymer Accessibility Settings tests. */

/** @const {string} Path to root from chrome/test/data/webui/settings/a11y. */
const ROOT_PATH = '../../../../../../';

// Polymer BrowserTest fixture and aXe-core accessibility audit.
GEN_INCLUDE([
  ROOT_PATH + 'chrome/test/data/webui/a11y/accessibility_test.js',
  ROOT_PATH + 'chrome/test/data/webui/polymer_browser_test_base.js',
]);

/**
 * Test fixture for Accessibility of Chrome Settings.
 * @constructor
 * @extends {PolymerTest}
 */
function SettingsAccessibilityTest() {}

// Default accessibility audit options. Specify in test definition to use.
SettingsAccessibilityTest.axeOptions = {
  'rules': {
    // Disable 'skip-link' check since there are few tab stops before the main
    // content.
    'skip-link': {enabled: false},
    // TODO(crbug.com/761461): enable after addressing flaky tests.
    'color-contrast': {enabled: false},
  }
};

// Default accessibility audit options. Specify in test definition to use.
SettingsAccessibilityTest.violationFilter = {
  // Polymer components use aria-active-attribute.
  'aria-valid-attr': function(nodeResult) {
    return nodeResult.element.hasAttribute('aria-active-attribute');
  },
  'button-name': function(nodeResult) {
    if (nodeResult.element.classList.contains('icon-expand-more'))
      return true;

    // Ignore the <button> residing within cr-toggle, which has tabindex -1
    // anyway.
    const parentNode = nodeResult.element.parentNode;
    return parentNode && parentNode.host &&
        parentNode.host.tagName == 'CR-TOGGLE';
  },
};

SettingsAccessibilityTest.prototype = {
  __proto__: PolymerTest.prototype,

  /** @override */
  browsePreload: 'chrome://settings/',

  // Include files that define the mocha tests.
  extraLibraries: PolymerTest.getLibraries(ROOT_PATH).concat([
    '../ensure_lazy_loaded.js',
  ]),

  // TODO(hcarmona): Remove once ADT is not longer in the testing infrastructure
  runAccessibilityChecks: false,

  setUp: function() {
    PolymerTest.prototype.setUp.call(this);
    settings.ensureLazyLoaded();
  },
};
