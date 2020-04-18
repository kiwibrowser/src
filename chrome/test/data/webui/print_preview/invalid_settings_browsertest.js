// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('invalid_settings_browsertest', function() {
  /** @enum {string} */
  const TestNames = {
    NoPDFPluginError: 'no pdf plugin error',
    InvalidSettingsError: 'invalid settings error',
    InvalidCertificateError: 'invalid certificate error',
    InvalidCertificateErrorReselectDestination: 'invalid certificate reselect',
  };

  const suiteName = 'InvalidSettingsBrowserTest';
  suite(suiteName, function() {
    /** @type {?PrintPreviewAppElement} */
    let page = null;

    /** @type {?print_preview.NativeLayer} */
    let nativeLayer = null;

    /** @type {!print_preview.NativeInitialSettings} */
    const initialSettings = {
      isInKioskAutoPrintMode: false,
      isInAppKioskMode: false,
      thousandsDelimeter: ',',
      decimalDelimeter: '.',
      unitType: 1,
      previewModifiable: true,
      documentTitle: 'title',
      documentHasSelection: true,
      shouldPrintSelectionOnly: false,
      printerName: 'FooDevice',
      serializedAppStateStr: null,
      serializedDefaultDestinationSelectionRulesStr: null
    };

    /** @type {!Array<!print_preview.LocalDestinationInfo>} */
    let localDestinationInfos = [
      {printerName: 'FooName', deviceName: 'FooDevice'},
      {printerName: 'BarName', deviceName: 'BarDevice'},
    ];

    /** @override */
    setup(function() {
      cloudprint.CloudPrintInterface = print_preview.CloudPrintInterfaceStub;
      nativeLayer = new print_preview.NativeLayerStub();
      print_preview.NativeLayer.setInstance(nativeLayer);
      PolymerTest.clearBody();
    });

    /**
     * Initializes the page with initial settings and local destinations
     * given by |initialSettings| and |localDestinationInfos|. Also creates
     * the fake plugin. Moved out of setup so tests can set those parameters
     * differently.
     */
    function createPage() {
      nativeLayer.setInitialSettings(initialSettings);
      nativeLayer.setLocalDestinations(localDestinationInfos);
      if (initialSettings.printerName) {
        nativeLayer.setLocalDestinationCapabilities(
            print_preview_test_utils.getCddTemplate(
                initialSettings.printerName));
      }

      page = document.createElement('print-preview-app');
      const previewArea = page.$$('print-preview-preview-area');
      previewArea.plugin_ = new print_preview.PDFPluginStub(
          previewArea.onPluginLoad_.bind(previewArea));
    }

    /**
     * Performs some setup for invalid certificate tests using 2 destinations
     * in |printers|. printers[0] will be set as the most recent destination,
     * and printers[1] will be the second most recent destination. Sets up
     * cloud print interface, user info, and runs createPage().
     * @param {!Array<!print_preview.Destination>} printers
     */
    function setupInvalidCertificateTest(printers) {
      initialSettings.printerName = '';
      initialSettings.serializedAppStateStr = JSON.stringify({
        version: 2,
        recentDestinations: [
          print_preview.makeRecentDestination(printers[0]),
          print_preview.makeRecentDestination(printers[1]),
        ],
      });
      localDestinationInfos = [];

      createPage();

      document.body.appendChild(page);
      page.userInfo_.setUsers('foo@chromium.org', ['foo@chromium.org']);
      cr.webUIListenerCallback('use-cloud-print', 'cloudprint url', false);
      printers.forEach(printer => {
        page.cloudPrintInterface_.setPrinter(printer.id, printer);
      });
    }

    // Test that error message is displayed when plugin doesn't exist.
    test(assert(TestNames.NoPDFPluginError), function() {
      createPage();
      const previewArea = page.$.previewArea;
      previewArea.checkPluginCompatibility = function() {
        return false;
      };
      document.body.appendChild(page);

      return nativeLayer.whenCalled('getInitialSettings').then(function() {
        const overlayEl = previewArea.$$('.preview-area-overlay-layer');
        const messageEl = previewArea.$$('.preview-area-message');

        // Make sure the overlay is visible.
        assertFalse(overlayEl.classList.contains('invisible'));

        // Make sure the correct text is shown.
        const expectedMessageChromium = 'Chromium does not include the PDF ' +
            'viewer which is required for Print Preview to function.';
        const expectedMessageChrome = 'Google Chrome cannot show the print ' +
            'preview when the built-in PDF viewer is missing.';
        assertTrue(
            messageEl.textContent.includes(expectedMessageChromium) ||
            messageEl.textContent.includes(expectedMessageChrome));
      });
    });

    // Tests that when a printer cannot be communicated with correctly the
    // preview area displays an invalid printer error message and printing
    // is disabled. Verifies that the user can recover from this error by
    // selecting a different, valid printer.
    test(assert(TestNames.InvalidSettingsError), function() {
      createPage();
      const barDevice = print_preview_test_utils.getCddTemplate('BarDevice');
      nativeLayer.setLocalDestinationCapabilities(barDevice);

      // FooDevice is the default printer, so will be selected for the initial
      // preview request.
      nativeLayer.setInvalidPrinterId('FooDevice');

      // Expected message
      const expectedMessage = 'The selected printer is not available or not ' +
          'installed correctly.  Check your printer or try selecting another ' +
          'printer.';

      // Get references to relevant elements.
      const previewAreaEl = page.$.previewArea;
      const overlay = previewAreaEl.$$('.preview-area-overlay-layer');
      const messageEl = previewAreaEl.$$('.preview-area-message');
      const header = page.$$('print-preview-header');
      const printButton = header.$$('.print');

      document.body.appendChild(page);
      return nativeLayer.whenCalled('getInitialSettings')
          .then(function() {
            page.destinationStore_.startLoadDestinations(
                print_preview.PrinterType.LOCAL_PRINTER);
            // Wait for the preview request.
            return Promise.all([
              nativeLayer.whenCalled('getPrinterCapabilities'),
              nativeLayer.whenCalled('getPreview')
            ]);
          })
          .then(function() {
            // Print preview should have failed with invalid settings, since
            // FooDevice was set as an invalid printer.
            assertFalse(overlay.classList.contains('invisible'));
            assertTrue(messageEl.textContent.includes(expectedMessage));

            // Verify that the print button is disabled
            assertTrue(printButton.disabled);

            // Reset
            nativeLayer.reset();

            // Select a new destination
            const barDestination = page.destinationStore_.destinations().find(
                d => d.id == 'BarDevice');
            page.destinationStore_.selectDestination(barDestination);

            // Wait for the preview to be updated.
            return Promise.all([
              nativeLayer.whenCalled('getPrinterCapabilities'),
              nativeLayer.whenCalled('getPreview')
            ]);
          })
          .then(function() {
            // Message should be gone.
            assertTrue(overlay.classList.contains('invisible'));
            assertFalse(messageEl.textContent.includes(expectedMessage));

            // Has active print button and successfully 'prints', indicating
            assertFalse(printButton.disabled);
            printButton.click();
            // This should result in a call to print.
            return nativeLayer.whenCalled('print');
          })
          .then(
              /**
               * @param {string} printTicket The print ticket print() was called
               *     for.
               */
              function(printTicket) {
                // Sanity check some printing argument values.
                const ticket = JSON.parse(printTicket);
                assertEquals(barDevice.printer.deviceName, ticket.deviceName);
                assertEquals(
                    print_preview_test_utils.getDefaultOrientation(barDevice) ==
                        'LANDSCAPE',
                    ticket.landscape);
                assertEquals(1, ticket.copies);
                const mediaDefault =
                    print_preview_test_utils.getDefaultMediaSize(barDevice);
                assertEquals(
                    mediaDefault.width_microns, ticket.mediaSize.width_microns);
                assertEquals(
                    mediaDefault.height_microns,
                    ticket.mediaSize.height_microns);
                return nativeLayer.whenCalled('dialogClose');
              });
    });

    // Test that GCP invalid certificate printers disable the print preview when
    // selected and display an error and that the preview dialog can be
    // recovered by selecting a new destination. Verifies this works when the
    // invalid printer is the most recent destination and is selected by
    // default.
    test(assert(TestNames.InvalidCertificateError), function() {
      const invalidPrinter =
          print_preview_test_utils.createDestinationWithCertificateStatus(
              'FooDevice', 'FooName', true);
      const validPrinter =
          print_preview_test_utils.createDestinationWithCertificateStatus(
              'BarDevice', 'BarName', false);
      setupInvalidCertificateTest([invalidPrinter, validPrinter]);

      // Expected message
      const expectedMessage = 'The selected Google Cloud Print device is no ' +
          'longer supported. Try setting up the printer in your computer\'s ' +
          'system settings.';

      // Get references to relevant elements.
      const previewAreaEl = page.$.previewArea;
      const overlayEl = previewAreaEl.$$('.preview-area-overlay-layer');
      const messageEl = previewAreaEl.$$('.preview-area-message');
      const header = page.$$('print-preview-header');
      const printButton = header.$$('.print');
      const destinationSettings = page.$$('print-preview-destination-settings');
      const scalingSettings = page.$$('print-preview-scaling-settings');
      const layoutSettings = page.$$('print-preview-layout-settings');

      return nativeLayer.whenCalled('getInitialSettings')
          .then(function() {
            page.destinationStore_.startLoadCloudDestinations();

            // FooDevice will be selected since it is the most recently used
            // printer, so the invalid certificate error should be shown.
            // The overlay must be visible for the message to be seen.
            assertFalse(overlayEl.classList.contains('invisible'));

            // Verify that the correct message is shown.
            assertTrue(messageEl.textContent.includes(expectedMessage));

            // Verify that the print button is disabled
            assertTrue(printButton.disabled);

            // Verify the state is invalid and that some settings sections are
            // also disabled, so there is no way to regenerate the preview.
            assertEquals(print_preview_new.State.INVALID_PRINTER, page.state);
            assertTrue(layoutSettings.$$('select').disabled);
            assertTrue(scalingSettings.$$('input').disabled);

            // The destination settings button should be enabled, so that the
            // user can select a new printer.
            assertFalse(destinationSettings.$$('button').disabled);

            // Reset
            nativeLayer.reset();

            // Select a new, valid cloud destination.
            page.destinationStore_.selectDestination(validPrinter);
            return nativeLayer.whenCalled('getPreview');
          })
          .then(function() {
            // Has active print button, indicating recovery from error state.
            assertFalse(printButton.disabled);

            // Settings sections are now active.
            assertFalse(layoutSettings.$$('select').disabled);
            assertFalse(scalingSettings.$$('input').disabled);

            // The destination settings button should still be enabled.
            assertFalse(destinationSettings.$$('button').disabled);

            // Message text should have changed and overlay should be invisible.
            assertFalse(messageEl.textContent.includes(expectedMessage));
            assertTrue(overlayEl.classList.contains('invisible'));
          });
    });

    // Test that GCP invalid certificate printers disable the print preview when
    // selected and display an error and that the preview dialog can be
    // recovered by selecting a new destination. Tests that even if destination
    // was previously selected, the error is cleared.
    test(
        assert(TestNames.InvalidCertificateErrorReselectDestination),
        function() {
          const invalidPrinter =
              print_preview_test_utils.createDestinationWithCertificateStatus(
                  'FooDevice', 'FooName', true);
          const validPrinter =
              print_preview_test_utils.createDestinationWithCertificateStatus(
                  'BarDevice', 'BarName', false);
          setupInvalidCertificateTest([validPrinter, invalidPrinter]);

          // Get references to relevant elements.
          const previewAreaEl = page.$.previewArea;
          const overlayEl = previewAreaEl.$$('.preview-area-overlay-layer');
          const messageEl = previewAreaEl.$$('.preview-area-message');
          const header = page.$$('print-preview-header');
          const printButton = header.$$('.print');

          return nativeLayer.whenCalled('getInitialSettings')
              .then(function() {
                // Start loading cloud destinations so that the printer
                // capabilities arrive.
                page.destinationStore_.startLoadCloudDestinations();
                return nativeLayer.whenCalled('getPreview');
              })
              .then(function() {
                // Has active print button.
                assertFalse(printButton.disabled);
                // No error message.
                assertTrue(overlayEl.classList.contains('invisible'));

                // Select the invalid destination and wait for the event.
                const whenInvalid = test_util.eventToPromise(
                    print_preview.DestinationStore.EventType
                        .SELECTED_DESTINATION_UNSUPPORTED,
                    page.destinationStore_);
                page.destinationStore_.selectDestination(invalidPrinter);
                return whenInvalid;
              })
              .then(function() {
                // Should have error message.
                assertFalse(overlayEl.classList.contains('invisible'));

                // Reset
                nativeLayer.reset();

                // Reselect the valid cloud destination.
                const whenSelected = test_util.eventToPromise(
                    print_preview.DestinationStore.EventType.DESTINATION_SELECT,
                    page.destinationStore_);
                page.destinationStore_.selectDestination(validPrinter);
                return whenSelected;
              })
              .then(function() {
                // Has active print button and no error message.
                assertFalse(printButton.disabled);
                assertTrue(overlayEl.classList.contains('invisible'));
              });
        });
  });

  return {
    suiteName: suiteName,
    TestNames: TestNames,
  };
});
