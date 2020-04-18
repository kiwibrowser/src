// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Tests for shared Polymer components. */

/** @const {string} Path to source root. */
var ROOT_PATH = '../../../../../';

// Polymer BrowserTest fixture.
GEN_INCLUDE(
    [ROOT_PATH + 'chrome/test/data/webui/polymer_browser_test_base.js']);

/**
 * Test fixture for shared Polymer components.
 * @constructor
 * @extends {PolymerTest}
 */
function CrComponentsBrowserTest() {}

CrComponentsBrowserTest.prototype = {
  __proto__: PolymerTest.prototype,

  /** @override */
  extraLibraries: PolymerTest.getLibraries(ROOT_PATH),

  /** @override */
  get browsePreload() {
    throw 'subclasses should override to load a WebUI page that includes it.';
  },

  /** @override */
  runAccessibilityChecks: true,

  /** @override */
  setUp: function() {
    PolymerTest.prototype.setUp.call(this);
    // We aren't loading the main document.
    this.accessibilityAuditConfig.ignoreSelectors('humanLangMissing', 'html');
  },
};

GEN('#if defined(OS_CHROMEOS)');

/**
 * @constructor
 * @extends {CrComponentsBrowserTest}
 */
function CrComponentsNetworkConfigTest() {}

CrComponentsNetworkConfigTest.prototype = {
  __proto__: CrComponentsBrowserTest.prototype,

  /** @override */
  browsePreload: 'chrome://internet-config-dialog',

  /** @override */
  extraLibraries: CrComponentsBrowserTest.prototype.extraLibraries.concat([
    ROOT_PATH + 'ui/webui/resources/js/assert.js',
    ROOT_PATH + 'ui/webui/resources/js/promise_resolver.js',
    '../fake_chrome_event.js',
    '../chromeos/networking_private_constants.js',
    '../chromeos/fake_networking_private.js',
    '../chromeos/cr_onc_strings.js',
    'network_config_test.js',
  ]),
};

TEST_F('CrComponentsNetworkConfigTest', 'All', function() {
  mocha.run();
});

GEN('#endif');
