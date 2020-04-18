// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Site settings page tests. */

GEN_INCLUDE(['settings_page_browsertest.js']);
/**
 * @constructor
 * @extends {SettingsPageBrowserTest}
 */
function SettingsSiteSettingsPageBrowserTest() {}

SettingsSiteSettingsPageBrowserTest.prototype = {
  __proto__: SettingsPageBrowserTest.prototype,
};

// Failing on ChromiumOS dbg. https://crbug.com/709442
GEN('#if (defined(OS_WIN) || defined(OS_CHROMEOS))  && !defined(NDEBUG)');
GEN('#define MAYBE_labels DISABLED_labels');
GEN('#else');
GEN('#define MAYBE_labels labels');
GEN('#endif');
TEST_F('SettingsSiteSettingsPageBrowserTest', 'MAYBE_labels', function() {
  suite('Site settings page', function() {
    let ui;

    suiteSetup(function() {
      ui = assert(document.createElement('settings-site-settings-page'));
    });

    test('defaultSettingLabel_ tests', function() {
      assertEquals(
          'a',
          ui.defaultSettingLabel_(settings.ContentSetting.ALLOW, 'a', 'b'));
      assertEquals(
          'b',
          ui.defaultSettingLabel_(settings.ContentSetting.BLOCK, 'a', 'b'));
      assertEquals(
          'a',
          ui.defaultSettingLabel_(
              settings.ContentSetting.ALLOW, 'a', 'b', 'c'));
      assertEquals(
          'b',
          ui.defaultSettingLabel_(
              settings.ContentSetting.BLOCK, 'a', 'b', 'c'));
      assertEquals(
          'c',
          ui.defaultSettingLabel_(
              settings.ContentSetting.SESSION_ONLY, 'a', 'b', 'c'));
      assertEquals(
          'c',
          ui.defaultSettingLabel_(
              settings.ContentSetting.DEFAULT, 'a', 'b', 'c'));
      assertEquals(
          'c',
          ui.defaultSettingLabel_(settings.ContentSetting.ASK, 'a', 'b', 'c'));
      assertEquals(
          'c',
          ui.defaultSettingLabel_(
              settings.ContentSetting.DETECT_IMPORTANT_CONTENT, 'a', 'b', 'c'));
    });
  });

  mocha.run();
});
