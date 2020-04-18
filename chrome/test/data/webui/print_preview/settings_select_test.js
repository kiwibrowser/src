// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('settings_select_test', function() {
  /** @enum {string} */
  const TestNames = {
    CustomMediaNames: 'custom media names',
  };

  const suiteName = 'SettingsSelectTest';
  suite(suiteName, function() {
    let settingsSelect = null;

    /** @override */
    setup(function() {
      PolymerTest.clearBody();
      settingsSelect = document.createElement('print-preview-settings-select');
      settingsSelect.disabled = false;
      document.body.appendChild(settingsSelect);
    });

    // Test that destinations are correctly displayed in the lists.
    test(assert(TestNames.CustomMediaNames), function() {
      // Set a capability with custom paper sizes.
      settingsSelect.settingName = 'mediaSize';
      settingsSelect.capability =
          print_preview_test_utils.getMediaSizeCapabilityWithCustomNames();
      const customLocalizedMediaName = settingsSelect.capability.option[0]
                                           .custom_display_name_localized[0]
                                           .value;
      const customMediaName =
          settingsSelect.capability.option[1].custom_display_name;
      Polymer.dom.flush();

      const select = settingsSelect.$$('select');
      // Verify that the selected option and names are as expected.
      assertEquals(0, select.selectedIndex);
      assertEquals(2, select.options.length);
      assertEquals(
          customLocalizedMediaName, select.options[0].textContent.trim());
      assertEquals(customMediaName, select.options[1].textContent.trim());
    });
  });

  return {
    suiteName: suiteName,
    TestNames: TestNames,
  };
});
