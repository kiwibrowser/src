// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('advanced_dialog_test', function() {
  /** @enum {string} */
  const TestNames = {
    AdvancedSettings1Option: 'advanced settings 1 option',
    AdvancedSettings2Options: 'advanced settings 2 options',
  };

  const suiteName = 'AdvancedDialogTest';
  suite(suiteName, function() {
    /** @type {?PrintPreviewAdvancedSettingsDialogElement} */
    let dialog = null;

    /** @type {?print_preview.Destination} */
    let destination = null;

    /** @type {string} */
    let printerId = 'FooDevice';

    /** @type {string} */
    let printerName = 'FooName';

    /** @override */
    setup(function() {
      // Create destination
      destination = new print_preview.Destination(
          printerId, print_preview.DestinationType.GOOGLE,
          print_preview.DestinationOrigin.COOKIES, printerName,
          true /* isRecent */,
          print_preview.DestinationConnectionStatus.ONLINE);
      PolymerTest.clearBody();
      dialog = document.createElement('print-preview-advanced-dialog');
    });

    /**
     * Verifies that the search box is shown or hidden based on the number
     * of capabilities and that the correct number of items are created.
     * @param {number} count The number of vendor capabilities the printer
     *     should have.
     */
    function testListWithItemCount(count) {
      const template =
          print_preview_test_utils.getCddTemplateWithAdvancedSettings(
              count, printerId, printerName);
      destination.capabilities = template.capabilities;
      dialog.destination = destination;

      document.body.appendChild(dialog);
      dialog.show();
      Polymer.dom.flush();

      // Search box should be hidden if there is only 1 item.
      const searchBox = dialog.$.searchBox;
      assertEquals(count == 1, searchBox.hidden);

      // Verify item is displayed.
      const vendorItems = template.capabilities.printer.vendor_capability;
      const items = dialog.shadowRoot.querySelectorAll(
          'print-preview-advanced-settings-item');
      assertEquals(count, items.length);
    }

    // Tests that the search box does not appear when there is only one option,
    // and that the vendor item is correctly displayed.
    test(assert(TestNames.AdvancedSettings1Option), function() {
      testListWithItemCount(1);
    });

    // Tests that the search box appears when there are two options, and that
    // the items are correctly displayed.
    test(assert(TestNames.AdvancedSettings2Options), function() {
      testListWithItemCount(2);
    });
  });

  return {
    suiteName: suiteName,
    TestNames: TestNames,
  };
});
