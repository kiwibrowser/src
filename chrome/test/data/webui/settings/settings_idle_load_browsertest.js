// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Tests for settings-idle-load. */

/** @const {string} Path to root from chrome/test/data/webui/settings/. */
const ROOT_PATH = '../../../../../';

/**
 * @constructor
 * @extends testing.Test
 */
function SettingsIdleLoadBrowserTest() {}

SettingsIdleLoadBrowserTest.prototype = {
  __proto__: testing.Test.prototype,

  /** @override */
  browsePreload: 'chrome://settings/controls/setting_idle_load.html',

  /** @override */
  extraLibraries: [
    ROOT_PATH + 'third_party/mocha/mocha.js',
    '../mocha_adapter.js',
  ],

  /** @override */
  isAsync: true,

  /** @override */
  runAccessibilityChecks: false,
};

TEST_F('SettingsIdleLoadBrowserTest', 'All', function() {
  // Register mocha tests.
  suite('Settings idle load tests', function() {
    setup(function() {
      const template = '<template is="settings-idle-load" id="idleTemplate" ' +
          '    url="chrome://resources/html/polymer.html">' +
          '  <div></div>' +
          '</template>';
      document.body.innerHTML = template;
      // The div should not be initially accesible.
      assertFalse(!!document.body.querySelector('div'));
    });

    test('stamps after get()', function() {
      // Calling get() will force stamping without waiting for idle time.
      return document.getElementById('idleTemplate')
          .get()
          .then(function(inner) {
            assertEquals('DIV', inner.nodeName);
            assertEquals(inner, document.body.querySelector('div'));
          });
    });

    test('stamps after idle', function(done) {
      requestIdleCallback(function() {
        // After JS calls idle-callbacks, this should be accesible.
        assertTrue(!!document.body.querySelector('div'));
        done();
      });
    });
  });

  // Run all registered tests.
  mocha.run();
});
