// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview_test', function() {
  /**
   * Index of the "Save as PDF" printer.
   * @type {number}
   * @const
   */
  const PDF_INDEX = 0;

  /**
   * Index of the Foo printer.
   * @type {number}
   * @const
   */
  const FOO_INDEX = 1;

  /**
   * Index of the Bar printer.
   * @type {number}
   * @const
   */
  const BAR_INDEX = 2;

  let printPreview = null;
  let nativeLayer = null;
  let initialSettings = null;
  let localDestinationInfos = null;
  let previewArea = null;

  /**
   * Initialize print preview with the initial settings currently stored in
   * |initialSettings|.
   */
  function setInitialSettings() {
    nativeLayer.setInitialSettings(initialSettings);
    printPreview.initialize();
  }

  /**
   * Sets settings and destinations and local destination that is the system
   * default.
   * @param {print_preview.PrinterCapabilitiesResponse=} opt_device The
   *     response to use for printer capabilities when the printer represented
   *     by |opt_device| is loaded. To avoid crashing when initialize() is
   *     called, |opt_device| should represent the printer that will be selected
   *     when print preview is first opened, i.e. the system default
   *     destination, or the most recently used destination or destination
   *     selected by the rules string if these parameters are defined in
   *     initialSettings.serializedAppStateStr_.
   *     If |opt_device| is not provided, a default device with ID 'FooDevice'
   *     will be used.
   * @return {!Promise<print_preview.PrinterCapabilitiesResponse>} a
   *     promise that will resolve when getPrinterCapabilities has been
   *     called for the device (either default or provided).
   */
  function setupSettingsAndDestinationsWithCapabilities(opt_device) {
    nativeLayer.setInitialSettings(initialSettings);
    nativeLayer.setLocalDestinations(localDestinationInfos);
    opt_device = opt_device ||
        print_preview_test_utils.getCddTemplate('FooDevice', 'FooName');
    nativeLayer.setLocalDestinationCapabilities(opt_device);

    printPreview.initialize();
    return nativeLayer.whenCalled('getInitialSettings').then(function() {
      printPreview.destinationStore_.startLoadDestinations(
          print_preview.PrinterType.LOCAL_PRINTER);
      return Promise.all([
        nativeLayer.whenCalled('getPrinters'),
        nativeLayer.whenCalled('getPrinterCapabilities')
      ]);
    });
  }

  /**
   * Wait for a new printer to have capabilities set and to reset the preview.
   */
  function waitForPrinterToUpdatePreview() {
    return Promise.all([
      nativeLayer.whenCalled('getPrinterCapabilities'),
      nativeLayer.whenCalled('getPreview'),
    ]);
  }

  /**
   * Verify that |section| visibility matches |visible|.
   * @param {HTMLDivElement} section The section to check.
   * @param {boolean} visible The expected state of visibility.
   */
  function checkSectionVisible(section, visible) {
    assertNotEquals(null, section);
    expectEquals(
        visible, section.classList.contains('visible'),
        'section=' + section.id);
  }

  /**
   * @param {?HTMLElement} el
   * @param {boolean} isDisplayed
   */
  function checkElementDisplayed(el, isDisplayed) {
    assertNotEquals(null, el);
    expectEquals(
        isDisplayed, !el.hidden,
        'element="' + el.id + '" of class "' + el.classList + '"');
  }

  /**
   * @return {!print_preview.PrinterCapabilitiesResponse} The capabilities of
   *     the Save as PDF destination.
   */
  function getPdfPrinter() {
    return {
      printer: {
        deviceName: 'Save as PDF',
      },
      capabilities: {
        version: '1.0',
        printer: {
          page_orientation: {
            option: [
              {type: 'AUTO', is_default: true}, {type: 'PORTRAIT'},
              {type: 'LANDSCAPE'}
            ]
          },
          color: {option: [{type: 'STANDARD_COLOR', is_default: true}]},
          media_size: {
            option: [{
              name: 'NA_LETTER',
              width_microns: 0,
              height_microns: 0,
              is_default: true
            }]
          }
        }
      }
    };
  }

  /**
   * Gets a serialized app state string with some non-default values.
   * @return {string}
   */
  function getAppStateString() {
    const origin = cr.isChromeOS ? 'chrome_os' : 'local';
    const cdd =
        print_preview_test_utils.getCddTemplate('ID1', 'One').capabilities;
    return JSON.stringify({
      version: 2,
      recentDestinations: [
        {
          id: 'ID1',
          origin: origin,
          account: '',
          capabilities: cdd,
          displayName: 'One',
          extensionId: '',
          extensionName: '',
        },
        {
          id: 'ID2',
          origin: origin,
          account: '',
          capabilities: cdd,
          displayName: 'Two',
          extensionId: '',
          extensionName: '',
        },
        {
          id: 'ID3',
          origin: origin,
          account: '',
          capabilities: cdd,
          displayName: 'Three',
          extensionId: '',
          extensionName: '',
        },
      ],
      dpi: {horizontal_dpi: 100, vertical_dpi: 100},
      mediaSize: {
        name: 'CUSTOM_SQUARE',
        width_microns: 215900,
        height_microns: 215900,
        custom_display_name: 'CUSTOM_SQUARE'
      },
      customMargins: {top: 74, right: 74, bottom: 74, left: 74},
      vendorOptions: {},
      marginsType: print_preview.ticket_items.MarginsTypeValue.CUSTOM,
      scaling: '90',
      isHeaderFooterEnabled: true,
      isCssBackgroundEnabled: true,
      isFitToPageEnabled: true,
      isCollateEnabled: true,
      isDuplexEnabled: true,
      isLandscapeEnabled: true,
      isColorEnabled: true,
    });
  }

  /**
   * Even though animation duration and delay is set to zero, it is necessary to
   * wait until the animation has finished.
   * @return {!Promise} A promise firing when the animation is done.
   */
  function whenAnimationDone(elementId) {
    return new Promise(function(resolve) {
      // Add a listener for the animation end event.
      const element = $(elementId);
      element.addEventListener('animationend', function f(e) {
        element.removeEventListener('animationend', f);
        resolve();
      });
    });
  }

  /**
   * @param {!HTMLElement} headerFooter The header/footer settings section.
   * @param {boolean} displayed Whether the header footer section should be
   *     displayed when the preview is done loading.
   */
  function checkHeaderFooterOnLoad(headerFooter, displayed) {
    return nativeLayer.whenCalled('getPreview').then(function() {
      checkElementDisplayed(headerFooter, displayed);
      return whenAnimationDone('more-settings');
    });
  }

  /**
   * Expand the 'More Settings' div to expose all options.
   */
  function expandMoreSettings() {
    const moreSettings = $('more-settings');
    checkSectionVisible(moreSettings, true);
    moreSettings.click();
  }

  // Simulates a click of the advanced options settings button to bring up the
  // advanced settings overlay.
  function openAdvancedSettings() {
    // Check for button and click to view advanced settings section.
    const advancedOptionsSettingsButton =
        $('advanced-options-settings')
            .querySelector('.advanced-options-settings-button');
    checkElementDisplayed(advancedOptionsSettingsButton, true);
    // Button is disabled during testing due to test version of
    // testPluginCompatibility() being set to always return false. Enable button
    // to send click event.
    advancedOptionsSettingsButton.disabled = false;
    advancedOptionsSettingsButton.click();
  }

  /**
   * Repeated setup steps for the advanced settings tests.
   * Sets capabilities, and verifies advanced options section is visible
   * after expanding more settings. Then opens the advanced settings overlay
   * and verifies it is displayed.
   */
  function startAdvancedSettingsTest(device) {
    expandMoreSettings();

    // Check that the advanced options settings section is visible.
    checkSectionVisible($('advanced-options-settings'), true);

    // Open the advanced settings overlay.
    openAdvancedSettings();

    // Check advanced settings overlay is visible by checking that the close
    // button is displayed.
    const advancedSettingsCloseButton =
        $('advanced-settings').querySelector('.close-button');
    checkElementDisplayed(advancedSettingsCloseButton, true);
  }

  /**
   * Performs some setup for invalid certificate tests using 2 destinations
   * in |printers|. printers[0] will be set as the most recent destination,
   * and printers[1] will be the second most recent destination. Sets up
   * cloud print interface, user info, and runs initialize().
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

    nativeLayer.setInitialSettings(initialSettings);
    localDestinationInfos = [];
    nativeLayer.setLocalDestinations(localDestinationInfos);
    printPreview.userInfo_.setUsers('foo@chromium.org', ['foo@chromium.org']);
    printPreview.initialize();
    cr.webUIListenerCallback('use-cloud-print', 'cloudprint url', false);
    printers.forEach(printer => {
      printPreview.cloudPrintInterface_.setPrinter(printer.id, printer);
    });
  }

  /** @return {boolean} */
  function isPrintAsImageEnabled() {
    // Should be enabled by default on non Windows/Mac.
    return !cr.isWindows && !cr.isMac;
  }

  const suiteName = 'PrintPreview';

  suite(suiteName, function() {
    suiteSetup(function() {
      cloudprint.CloudPrintInterface = print_preview.CloudPrintInterfaceStub;
      print_preview.PreviewArea.prototype.checkPluginCompatibility_ =
          function() {
        return true;
      };
    });

    setup(function() {
      initialSettings = print_preview_test_utils.getDefaultInitialSettings();
      localDestinationInfos = [
        {printerName: 'FooName', deviceName: 'FooDevice'},
        {printerName: 'BarName', deviceName: 'BarDevice'},
      ];

      nativeLayer = new print_preview.NativeLayerStub();
      print_preview.NativeLayer.setInstance(nativeLayer);
      printPreview = new print_preview.PrintPreview();
      previewArea = printPreview.getPreviewArea();
      previewArea.plugin_ = new print_preview.PDFPluginStub(
          previewArea.onPluginLoad_.bind(previewArea));
    });

    // Test some basic assumptions about the print preview WebUI.
    test('PrinterList', function() {
      return setupSettingsAndDestinationsWithCapabilities().then(function() {
        const recentList =
            $('destination-search').querySelector('.recent-list ul');
        const printList =
            $('destination-search').querySelector('.print-list ul');
        assertNotEquals(null, recentList);
        assertEquals(1, recentList.childNodes.length);
        assertEquals(
            'FooName',
            recentList.childNodes.item(0)
                .querySelector('.destination-list-item-name')
                .textContent);
        assertNotEquals(null, printList);
        assertEquals(3, printList.childNodes.length);
        assertEquals(
            'Save as PDF',
            printList.childNodes.item(PDF_INDEX)
                .querySelector('.destination-list-item-name')
                .textContent);
        assertEquals(
            'FooName',
            printList.childNodes.item(FOO_INDEX)
                .querySelector('.destination-list-item-name')
                .textContent);
        assertEquals(
            'BarName',
            printList.childNodes.item(BAR_INDEX)
                .querySelector('.destination-list-item-name')
                .textContent);
      });
    });

    // Test restore settings with one destination.
    test('RestoreLocalDestination', function() {
      initialSettings.serializedAppStateStr = JSON.stringify({
        version: 2,
        recentDestinations: [
          {
            id: 'ID',
            origin: cr.isChromeOS ? 'chrome_os' : 'local',
            account: '',
            capabilities: 0,
            displayName: '',
            extensionId: '',
            extensionName: '',
          },
        ],
      });
      nativeLayer.setLocalDestinationCapabilities(
          print_preview_test_utils.getCddTemplate('ID'));
      setInitialSettings();
      return nativeLayer.whenCalled('getInitialSettings');
    });

    test('RestoreMultipleDestinations', function() {
      initialSettings.serializedAppStateStr = getAppStateString();
      // Set all three of these destinations in the local destination infos
      // (represents currently available printers), plus an extra destination.
      localDestinationInfos = [
        {printerName: 'One', deviceName: 'ID1'},
        {printerName: 'Two', deviceName: 'ID2'},
        {printerName: 'Three', deviceName: 'ID3'},
        {printerName: 'Four', deviceName: 'ID4'}
      ];

      // Set up capabilities for ID1. This should be the device that should hav
      // its capabilities fetched, since it is the most recent. If another
      // device is selected the native layer will reject the callback.
      const device = print_preview_test_utils.getCddTemplate('ID1', 'One');

      return setupSettingsAndDestinationsWithCapabilities(device).then(
          function() {
            // The most recently used destination should be the currently
            // selected one. This is ID1.
            assertEquals(
                'ID1', printPreview.destinationStore_.selectedDestination.id);

            // Look through the destinations. ID1, ID2, and ID3 should all be
            // recent.
            const destinations = printPreview.destinationStore_.destinations_;
            const idsFound = [];

            for (let i = 0; i < destinations.length; i++) {
              if (!destinations[i])
                continue;
              if (destinations[i].isRecent)
                idsFound.push(destinations[i].id);
            }

            // Make sure there were 3 recent destinations and that they are the
            // correct IDs.
            assertEquals(3, idsFound.length);
            assertNotEquals(-1, idsFound.indexOf('ID1'));
            assertNotEquals(-1, idsFound.indexOf('ID2'));
            assertNotEquals(-1, idsFound.indexOf('ID3'));
          });
    });

    // Tests that the app state can be saved correctly.
    test('SaveAppState', function() {
      // Set all three of these destinations in the local destination infos
      // (represents currently available printers), plus an extra destination.
      localDestinationInfos = [
        {printerName: 'One', deviceName: 'ID1'},
        {printerName: 'Two', deviceName: 'ID2'},
        {printerName: 'Three', deviceName: 'ID3'},
        {printerName: 'Four', deviceName: 'ID4'}
      ];

      initialSettings.printerName = 'ID3';
      const device = print_preview_test_utils.getCddTemplate('ID3', 'Three');

      nativeLayer.setLocalDestinationCapabilities(
          print_preview_test_utils.getCddTemplate('ID1', 'One'));
      nativeLayer.setLocalDestinationCapabilities(
          print_preview_test_utils.getCddTemplate('ID2', 'Two'));
      return setupSettingsAndDestinationsWithCapabilities(device)
          .then(function() {
            nativeLayer.reset();
            // Select ID2 then ID1 so that recent destinations will be 1, 2, 3
            const destination2 =
                printPreview.destinationStore_.destinations().find(
                    d => d.id == 'ID2');
            printPreview.destinationStore_.selectDestination(destination2);
            return waitForPrinterToUpdatePreview();
          })
          .then(function() {
            nativeLayer.reset();
            const destination1 =
                printPreview.destinationStore_.destinations().find(
                    d => d.id == 'ID1');
            printPreview.destinationStore_.selectDestination(destination1);
            return waitForPrinterToUpdatePreview();
          })
          .then(function() {
            // Update the persisted ticket items.
            printPreview.printTicketStore_.mediaSize.updateValue({
              name: 'CUSTOM_SQUARE',
              width_microns: 215900,
              height_microns: 215900,
              custom_display_name: 'CUSTOM_SQUARE'
            });
            printPreview.printTicketStore_.scaling.updateValue('90');
            printPreview.printTicketStore_.dpi.updateValue(
                {horizontal_dpi: 100, vertical_dpi: 100});
            printPreview.printTicketStore_.headerFooter.updateValue(true);
            printPreview.printTicketStore_.cssBackground.updateValue(true);
            printPreview.printTicketStore_.fitToPage.updateValue(true);
            printPreview.printTicketStore_.collate.updateValue(true);
            printPreview.printTicketStore_.duplex.updateValue(true);
            printPreview.printTicketStore_.landscape.updateValue(true);
            printPreview.printTicketStore_.color.updateValue(true);
            printPreview.printTicketStore_.marginsType.updateValue(
                print_preview.ticket_items.MarginsTypeValue.CUSTOM);
            nativeLayer.resetResolver('saveAppState');
            printPreview.printTicketStore_.customMargins.updateValue(
                new print_preview.Margins(74, 74, 74, 74));
            return nativeLayer.whenCalled('saveAppState');
          })
          .then(function(state) {  // validate state serialized correctly.
            expectEquals(getAppStateString(), state);
          });
    });

    test('RestoreAppState', function() {
      initialSettings.serializedAppStateStr = getAppStateString();
      // Set up destinations from app state.
      localDestinationInfos = [
        {printerName: 'One', deviceName: 'ID1'},
        {printerName: 'Two', deviceName: 'ID2'},
        {printerName: 'Three', deviceName: 'ID3'},
      ];
      const device = print_preview_test_utils.getCddTemplate('ID1', 'One');

      return Promise
          .all([
            setupSettingsAndDestinationsWithCapabilities(device),
            nativeLayer.whenCalled('getPreview')
          ])
          .then(function(args) {
            // Check printer 1 was selected.
            assertEquals(
                'ID1', printPreview.destinationStore_.selectedDestination.id);

            // Validate the parameters for getPreview match the app state.
            const ticket = JSON.parse(args[1].printTicket);
            expectEquals('CUSTOM_SQUARE', ticket.mediaSize.name);
            expectEquals(90, ticket.scaleFactor);
            expectEquals(100, ticket.dpiHorizontal);
            expectTrue(ticket.headerFooterEnabled);
            expectTrue(ticket.shouldPrintBackgrounds);
            expectTrue(ticket.fitToPageEnabled);
            expectTrue(ticket.collate);
            expectEquals(
                ticket.duplex,
                print_preview.PreviewGenerator.DuplexMode.LONG_EDGE);
            expectTrue(ticket.landscape);
            expectEquals(ticket.color, print_preview.ColorMode.COLOR);
            expectEquals(
                print_preview.ticket_items.MarginsTypeValue.CUSTOM,
                ticket.marginsType);
            expectEquals(74, ticket.marginsCustom.marginTop);

            // Change scaling (a persisted ticket item value)
            expandMoreSettings();
            const scalingSettings = $('scaling-settings');
            checkSectionVisible(scalingSettings, true);
            const scalingInput = scalingSettings.querySelector('.user-value');
            scalingInput.stepUp(5);
            const enterEvent = document.createEvent('Event');
            enterEvent.initEvent('keydown');
            enterEvent.keyCode = 'Enter';
            scalingInput.dispatchEvent(enterEvent);

            // Change back to old value.
            nativeLayer.resetResolver('saveAppState');
            scalingInput.stepDown(5);
            scalingInput.dispatchEvent(enterEvent);

            return nativeLayer.whenCalled('saveAppState').then(function(state) {
              // Validate that the re-serialized app state string matches.
              expectEquals(getAppStateString(), state);
            });
          });
    });

    test('DefaultDestinationSelectionRules', function() {
      // It also makes sure these rules do override system default destination.
      initialSettings.serializedDefaultDestinationSelectionRulesStr =
          JSON.stringify({namePattern: '.*Bar.*'});
      return setupSettingsAndDestinationsWithCapabilities(
                 print_preview_test_utils.getCddTemplate(
                     'BarDevice', 'BarName'))
          .then(function() {
            assertEquals(
                'BarDevice',
                printPreview.destinationStore_.selectedDestination.id);
          });
    });

    test('SystemDialogLinkIsHiddenInAppKioskMode', function() {
      if (!cr.isChromeOS)
        initialSettings.isInAppKioskMode = true;
      nativeLayer.setLocalDestinationCapabilities(
          print_preview_test_utils.getCddTemplate('FooDevice'));
      setInitialSettings();
      return nativeLayer.whenCalled('getInitialSettings').then(function() {
        if (cr.isChromeOS)
          assertEquals(null, $('system-dialog-link'));
        else
          checkElementDisplayed($('system-dialog-link'), false);
      });
    });

    test('SectionsDisabled', function() {
      checkSectionVisible($('layout-settings'), false);
      checkSectionVisible($('color-settings'), false);
      checkSectionVisible($('copies-settings'), false);
      const device = print_preview_test_utils.getCddTemplate('FooDevice');
      device.capabilities.printer.color = {
        option: [{is_default: true, type: 'STANDARD_COLOR'}]
      };
      delete device.capabilities.printer.copies;

      return setupSettingsAndDestinationsWithCapabilities(device).then(
          function() {
            checkSectionVisible($('layout-settings'), true);
            checkSectionVisible($('color-settings'), false);
            checkSectionVisible($('copies-settings'), false);

            return whenAnimationDone('other-options-collapsible');
          });
    });

    // When the source is 'PDF' and 'Save as PDF' option is selected, we hide
    // the fit to page option.
    test('PrintToPDFSelectedCapabilities', function() {
      // Setup initial settings
      initialSettings.previewModifiable = false;
      initialSettings.printerName = 'Save as PDF';

      // Set PDF printer
      nativeLayer.setLocalDestinationCapabilities(getPdfPrinter());

      setInitialSettings();
      return nativeLayer.whenCalled('getInitialSettings')
          .then(function() {
            return nativeLayer.whenCalled('getPrinterCapabilities');
          })
          .then(function() {
            const otherOptions = $('other-options-settings');
            const scalingSettings = $('scaling-settings');
            // If rasterization is an option, other options should be visible.
            // If not, there should be no available other options.
            checkSectionVisible(otherOptions, isPrintAsImageEnabled());
            if (isPrintAsImageEnabled()) {
              checkElementDisplayed(
                  otherOptions.querySelector('#rasterize-container'), true);
            }
            checkSectionVisible($('media-size-settings'), false);
            checkSectionVisible(scalingSettings, false);
            checkElementDisplayed(
                scalingSettings.querySelector('#fit-to-page-container'), false);
          });
    });

    // When the source is 'HTML', we always hide the fit to page option and show
    // media size option.
    test('SourceIsHTMLCapabilities', function() {
      return setupSettingsAndDestinationsWithCapabilities().then(function() {
        const otherOptions = $('other-options-settings');
        let rasterize;
        if (isPrintAsImageEnabled())
          rasterize = otherOptions.querySelector('#rasterize-container');
        const mediaSize = $('media-size-settings');
        const scalingSettings = $('scaling-settings');
        const fitToPage =
            scalingSettings.querySelector('#fit-to-page-container');

        // Check that options are collapsed (section is visible, because
        // duplex is available).
        checkSectionVisible(otherOptions, true);
        checkElementDisplayed(fitToPage, false);
        if (isPrintAsImageEnabled())
          checkElementDisplayed(rasterize, false);
        checkSectionVisible(mediaSize, false);
        checkSectionVisible(scalingSettings, false);

        expandMoreSettings();

        checkElementDisplayed(fitToPage, false);
        if (isPrintAsImageEnabled())
          checkElementDisplayed(rasterize, false);
        checkSectionVisible(mediaSize, true);
        checkSectionVisible(scalingSettings, true);

        return whenAnimationDone('more-settings');
      });
    });

    // When the source is 'PDF', depending on the selected destination printer,
    // we show/hide the fit to page option and hide media size selection.
    test('SourceIsPDFCapabilities', function() {
      initialSettings.previewModifiable = false;
      return setupSettingsAndDestinationsWithCapabilities().then(function() {
        const otherOptions = $('other-options-settings');
        const scalingSettings = $('scaling-settings');
        const fitToPageContainer =
            scalingSettings.querySelector('#fit-to-page-container');
        let rasterizeContainer;
        if (isPrintAsImageEnabled()) {
          rasterizeContainer =
              otherOptions.querySelector('#rasterize-container');
        }

        checkSectionVisible(otherOptions, true);
        checkElementDisplayed(fitToPageContainer, true);
        if (isPrintAsImageEnabled())
          checkElementDisplayed(rasterizeContainer, false);
        expectTrue(fitToPageContainer.querySelector('.checkbox').checked);
        expandMoreSettings();
        if (isPrintAsImageEnabled()) {
          checkElementDisplayed(rasterizeContainer, true);
          expectFalse(rasterizeContainer.querySelector('.checkbox').checked);
        }
        checkSectionVisible($('media-size-settings'), true);
        checkSectionVisible(scalingSettings, true);

        return whenAnimationDone('other-options-collapsible');
      });
    });

    // When the source is 'PDF', depending on the selected destination printer,
    // we show/hide the fit to page option and hide media size selection.
    test('ScalingUnchecksFitToPage', function() {
      initialSettings.previewModifiable = false;
      // Wait for preview to load.
      return Promise
          .all([
            setupSettingsAndDestinationsWithCapabilities(),
            nativeLayer.whenCalled('getPreview')
          ])
          .then(function(args) {
            const scalingSettings = $('scaling-settings');
            checkSectionVisible(scalingSettings, true);
            const fitToPageContainer =
                scalingSettings.querySelector('#fit-to-page-container');
            checkElementDisplayed(fitToPageContainer, true);
            const ticket = JSON.parse(args[1].printTicket);
            expectTrue(ticket.fitToPageEnabled);
            expectEquals(100, ticket.scaleFactor);
            expectTrue(fitToPageContainer.querySelector('.checkbox').checked);
            expandMoreSettings();
            checkSectionVisible($('media-size-settings'), true);
            checkSectionVisible(scalingSettings, true);
            nativeLayer.resetResolver('getPreview');

            // Change scaling input
            const scalingInput = scalingSettings.querySelector('.user-value');
            expectEquals('100', scalingInput.value);
            scalingInput.stepUp(5);
            expectEquals('105', scalingInput.value);

            // Trigger the event
            const enterEvent = document.createEvent('Event');
            enterEvent.initEvent('keydown');
            enterEvent.keyCode = 'Enter';
            scalingInput.dispatchEvent(enterEvent);

            // Wait for the preview to refresh and verify print ticket and
            // display. There will be 2 preview requests. Since we only catch
            // the first one, only verify fit to page in print ticket.
            return nativeLayer.whenCalled('getPreview').then(function(args) {
              const updatedTicket = JSON.parse(args.printTicket);
              expectFalse(updatedTicket.fitToPageEnabled);
              expectFalse(
                  fitToPageContainer.querySelector('.checkbox').checked);
              return whenAnimationDone('more-settings');
            });
          });
    });

    // When the number of copies print preset is set for source 'PDF', we update
    // the copies value if capability is supported by printer.
    test('CheckNumCopiesPrintPreset', function() {
      initialSettings.previewModifiable = false;
      return setupSettingsAndDestinationsWithCapabilities().then(function() {
        // Indicate that the number of copies print preset is set for source
        // PDF.
        const copies = 2;
        cr.webUIListenerCallback('print-preset-options', true, copies);
        checkSectionVisible($('copies-settings'), true);
        expectEquals(
            copies,
            parseInt($('copies-settings').querySelector('.user-value').value));

        return whenAnimationDone('other-options-collapsible');
      });
    });

    // When the duplex print preset is set for source 'PDF', we update the
    // duplex setting if capability is supported by printer.
    test('CheckDuplexPrintPreset', function() {
      initialSettings.previewModifiable = false;
      return setupSettingsAndDestinationsWithCapabilities().then(function() {
        // Indicate that the duplex print preset is set to 'long edge' for
        // source PDF.
        cr.webUIListenerCallback('print-preset-options', false, 1, 1);
        const otherOptions = $('other-options-settings');
        checkSectionVisible(otherOptions, true);
        const duplexContainer = otherOptions.querySelector('#duplex-container');
        checkElementDisplayed(duplexContainer, true);
        expectTrue(duplexContainer.querySelector('.checkbox').checked);

        return whenAnimationDone('other-options-collapsible');
      });
    });

    // Make sure that custom margins controls are properly set up.
    test('CustomMarginsControlsCheck', function() {
      return setupSettingsAndDestinationsWithCapabilities().then(function() {
        nativeLayer.resetResolver('getPreview');
        printPreview.printTicketStore_.marginsType.updateValue(
            print_preview.ticket_items.MarginsTypeValue.CUSTOM);
        return nativeLayer.whenCalled('getPreview').then(function() {
          ['left', 'top', 'right', 'bottom'].forEach(function(margin) {
            const control =
                $('preview-area').querySelector('.margin-control-' + margin);
            assertNotEquals(null, control);
            const input = control.querySelector('.margin-control-textbox');
            assertTrue(input.hasAttribute('aria-label'));
            assertNotEquals('undefined', input.getAttribute('aria-label'));
          });
        });
      });
    });

    // Page layout has zero margins. Hide header and footer option.
    test('PageLayoutHasNoMarginsHideHeaderFooter', function() {
      return setupSettingsAndDestinationsWithCapabilities().then(function() {
        const otherOptions = $('other-options-settings');
        const headerFooter =
            otherOptions.querySelector('#header-footer-container');

        // Check that options are collapsed (section is visible, because
        // duplex is available).
        checkSectionVisible(otherOptions, true);
        checkElementDisplayed(headerFooter, false);

        expandMoreSettings();

        checkElementDisplayed(headerFooter, true);

        nativeLayer.resetResolver('getPreview');
        printPreview.printTicketStore_.marginsType.updateValue(
            print_preview.ticket_items.MarginsTypeValue.CUSTOM);
        printPreview.printTicketStore_.customMargins.updateValue(
            new print_preview.Margins(0, 0, 0, 0));

        return checkHeaderFooterOnLoad(headerFooter, false);
      });
    });

    // Page layout has half-inch margins. Show header and footer option.
    test('PageLayoutHasMarginsShowHeaderFooter', function() {
      return setupSettingsAndDestinationsWithCapabilities().then(function() {
        const otherOptions = $('other-options-settings');
        const headerFooter =
            otherOptions.querySelector('#header-footer-container');

        // Check that options are collapsed (section is visible, because
        // duplex is available).
        checkSectionVisible(otherOptions, true);
        checkElementDisplayed(headerFooter, false);

        expandMoreSettings();

        checkElementDisplayed(headerFooter, true);

        nativeLayer.resetResolver('getPreview');
        printPreview.printTicketStore_.marginsType.updateValue(
            print_preview.ticket_items.MarginsTypeValue.CUSTOM);
        printPreview.printTicketStore_.customMargins.updateValue(
            new print_preview.Margins(36, 36, 36, 36));
        return checkHeaderFooterOnLoad(headerFooter, true);
      });
    });

    // Page layout has zero top and bottom margins. Hide header and footer
    // option.
    test('ZeroTopAndBottomMarginsHideHeaderFooter', function() {
      return setupSettingsAndDestinationsWithCapabilities().then(function() {
        const otherOptions = $('other-options-settings');
        const headerFooter =
            otherOptions.querySelector('#header-footer-container');

        // Check that options are collapsed (section is visible, because
        // duplex is available).
        checkSectionVisible(otherOptions, true);
        checkElementDisplayed(headerFooter, false);

        expandMoreSettings();

        checkElementDisplayed(headerFooter, true);

        nativeLayer.resetResolver('getPreview');
        printPreview.printTicketStore_.marginsType.updateValue(
            print_preview.ticket_items.MarginsTypeValue.CUSTOM);
        printPreview.printTicketStore_.customMargins.updateValue(
            new print_preview.Margins(0, 36, 0, 36));

        return checkHeaderFooterOnLoad(headerFooter, false);
      });
    });

    // Page layout has zero top and half-inch bottom margin. Show header and
    // footer option.
    test('ZeroTopAndNonZeroBottomMarginShowHeaderFooter', function() {
      return setupSettingsAndDestinationsWithCapabilities().then(function() {
        const otherOptions = $('other-options-settings');
        const headerFooter =
            otherOptions.querySelector('#header-footer-container');

        // Check that options are collapsed (section is visible, because
        // duplex is available).
        checkSectionVisible(otherOptions, true);
        checkElementDisplayed(headerFooter, false);

        expandMoreSettings();

        checkElementDisplayed(headerFooter, true);

        nativeLayer.resetResolver('getPreview');
        printPreview.printTicketStore_.marginsType.updateValue(
            print_preview.ticket_items.MarginsTypeValue.CUSTOM);
        printPreview.printTicketStore_.customMargins.updateValue(
            new print_preview.Margins(0, 36, 36, 36));

        return checkHeaderFooterOnLoad(headerFooter, true);
      });
    });

    // Check header footer availability with small (label) page size.
    test('SmallPaperSizeHeaderFooter', function() {
      const device = print_preview_test_utils.getCddTemplate('FooDevice');
      device.capabilities.printer.media_size = {
        'option': [
          {
            'name': 'SmallLabel',
            'width_microns': 38100,
            'height_microns': 12700,
            'is_default': false
          },
          {
            'name': 'BigLabel',
            'width_microns': 50800,
            'height_microns': 76200,
            'is_default': true
          }
        ]
      };
      return setupSettingsAndDestinationsWithCapabilities(device).then(
          function() {
            const otherOptions = $('other-options-settings');
            const headerFooter =
                otherOptions.querySelector('#header-footer-container');

            // Check that options are collapsed (section is visible, because
            // duplex is available).
            checkSectionVisible(otherOptions, true);
            checkElementDisplayed(headerFooter, false);

            expandMoreSettings();

            // Big label should have header/footer
            checkElementDisplayed(headerFooter, true);

            // Small label should not
            nativeLayer.resetResolver('getPreview');
            printPreview.printTicketStore_.mediaSize.updateValue(
                device.capabilities.printer.media_size.option[0]);
            return nativeLayer.whenCalled('getPreview').then(function() {
              checkElementDisplayed(headerFooter, false);

              // Oriented in landscape, there should be enough space for
              // header/footer.
              nativeLayer.resetResolver('getPreview');
              printPreview.printTicketStore_.landscape.updateValue(true);
              return checkHeaderFooterOnLoad(headerFooter, true);
            });
          });
    });

    // Test that the color settings, one option, standard monochrome.
    test('ColorSettingsMonochrome', function() {
      // Only one option, standard monochrome.
      const device = print_preview_test_utils.getCddTemplate('FooDevice');
      device.capabilities.printer.color = {
        'option': [{'is_default': true, 'type': 'STANDARD_MONOCHROME'}]
      };

      return setupSettingsAndDestinationsWithCapabilities(device).then(
          function() {
            checkSectionVisible($('color-settings'), false);

            return whenAnimationDone('more-settings');
          });
    });

    // Test that the color settings, one option, custom monochrome.
    test('ColorSettingsCustomMonochrome', function() {
      // Only one option, standard monochrome.
      const device = print_preview_test_utils.getCddTemplate('FooDevice');
      device.capabilities.printer.color = {
        'option': [{
          'is_default': true,
          'type': 'CUSTOM_MONOCHROME',
          'vendor_id': '42'
        }]
      };

      return setupSettingsAndDestinationsWithCapabilities(device).then(
          function() {
            checkSectionVisible($('color-settings'), false);

            return whenAnimationDone('more-settings');
          });
    });

    // Test that the color settings, one option, standard color.
    test('ColorSettingsColor', function() {
      const device = print_preview_test_utils.getCddTemplate('FooDevice');
      device.capabilities.printer.color = {
        'option': [{'is_default': true, 'type': 'STANDARD_COLOR'}]
      };

      return setupSettingsAndDestinationsWithCapabilities(device).then(
          function() {
            checkSectionVisible($('color-settings'), false);

            return whenAnimationDone('more-settings');
          });
    });

    // Test that the color settings, one option, custom color.
    test('ColorSettingsCustomColor', function() {
      const device = print_preview_test_utils.getCddTemplate('FooDevice');
      device.capabilities.printer.color = {
        'option':
            [{'is_default': true, 'type': 'CUSTOM_COLOR', 'vendor_id': '42'}]
      };
      return setupSettingsAndDestinationsWithCapabilities(device).then(
          function() {
            checkSectionVisible($('color-settings'), false);

            return whenAnimationDone('more-settings');
          });
    });

    // Test that the color settings, two options, both standard, defaults to
    // color.
    test('ColorSettingsBothStandardDefaultColor', function() {
      const device = print_preview_test_utils.getCddTemplate('FooDevice');
      device.capabilities.printer.color = {
        'option': [
          {'type': 'STANDARD_MONOCHROME'},
          {'is_default': true, 'type': 'STANDARD_COLOR'}
        ]
      };
      return setupSettingsAndDestinationsWithCapabilities(device).then(
          function() {
            checkSectionVisible($('color-settings'), true);
            expectEquals(
                'color',
                $('color-settings')
                    .querySelector('.color-settings-select')
                    .value);

            return whenAnimationDone('more-settings');
          });
    });

    // Test that the color settings, two options, both standard, defaults to
    // monochrome.
    test('ColorSettingsBothStandardDefaultMonochrome', function() {
      const device = print_preview_test_utils.getCddTemplate('FooDevice');
      device.capabilities.printer.color = {
        'option': [
          {'is_default': true, 'type': 'STANDARD_MONOCHROME'},
          {'type': 'STANDARD_COLOR'}
        ]
      };
      return setupSettingsAndDestinationsWithCapabilities(device).then(
          function() {
            checkSectionVisible($('color-settings'), true);
            expectEquals(
                'bw',
                $('color-settings')
                    .querySelector('.color-settings-select')
                    .value);

            return whenAnimationDone('more-settings');
          });
    });

    // Test that the color settings, two options, both custom, defaults to
    // color.
    test('ColorSettingsBothCustomDefaultColor', function() {
      const device = print_preview_test_utils.getCddTemplate('FooDevice');
      device.capabilities.printer.color = {
        'option': [
          {'type': 'CUSTOM_MONOCHROME', 'vendor_id': '42'},
          {'is_default': true, 'type': 'CUSTOM_COLOR', 'vendor_id': '43'}
        ]
      };
      return setupSettingsAndDestinationsWithCapabilities(device).then(
          function() {
            checkSectionVisible($('color-settings'), true);
            expectEquals(
                'color',
                $('color-settings')
                    .querySelector('.color-settings-select')
                    .value);

            return whenAnimationDone('more-settings');
          });
    });

    // Test to verify that duplex settings are set according to the printer
    // capabilities.
    test('DuplexSettingsTrue', function() {
      return setupSettingsAndDestinationsWithCapabilities().then(function() {
        const otherOptions = $('other-options-settings');
        checkSectionVisible(otherOptions, true);
        duplexContainer = otherOptions.querySelector('#duplex-container');
        expectFalse(duplexContainer.hidden);
        expectFalse(duplexContainer.querySelector('.checkbox').checked);

        return whenAnimationDone('more-settings');
      });
    });

    // Test to verify that duplex settings are set according to the printer
    // capabilities.
    test('DuplexSettingsFalse', function() {
      const device = print_preview_test_utils.getCddTemplate('FooDevice');
      delete device.capabilities.printer.duplex;
      return setupSettingsAndDestinationsWithCapabilities(device).then(
          function() {
            // Check that it is collapsed.
            const otherOptions = $('other-options-settings');
            checkSectionVisible(otherOptions, false);

            expandMoreSettings();

            // Now it should be visible.
            checkSectionVisible(otherOptions, true);
            expectTrue(otherOptions.querySelector('#duplex-container').hidden);

            return whenAnimationDone('more-settings');
          });
    });

    // Test that changing the selected printer updates the preview.
    test('PrinterChangeUpdatesPreview', function() {
      return Promise
          .all([
            setupSettingsAndDestinationsWithCapabilities(),
            nativeLayer.whenCalled('getPreview'),
          ])
          .then(function(args) {
            const ticket = JSON.parse(args[1].printTicket);
            expectEquals(0, ticket.requestID);
            expectEquals('FooDevice', ticket.deviceName);
            nativeLayer.reset();

            // Setup capabilities for BarDevice.
            const device = print_preview_test_utils.getCddTemplate('BarDevice');
            device.capabilities.printer.color = {
              'option': [{'is_default': true, 'type': 'STANDARD_MONOCHROME'}]
            };
            nativeLayer.setLocalDestinationCapabilities(device);
            // Select BarDevice
            const barDestination =
                printPreview.destinationStore_.destinations().find(
                    d => d.id == 'BarDevice');
            printPreview.destinationStore_.selectDestination(barDestination);
            return waitForPrinterToUpdatePreview();
          })
          .then(function(args) {
            const ticket = JSON.parse(args[1].printTicket);
            expectEquals(1, ticket.requestID);
            expectEquals('BarDevice', ticket.deviceName);
          });
    });

    // Test that error message is displayed when plugin doesn't exist.
    test('NoPDFPluginErrorMessage', function() {
      previewArea.checkPluginCompatibility_ = function() {
        return false;
      };
      nativeLayer.setLocalDestinationCapabilities(
          print_preview_test_utils.getCddTemplate('FooDevice'));
      setInitialSettings();
      return nativeLayer.whenCalled('getInitialSettings').then(function() {
        const previewAreaEl = $('preview-area');

        const loadingMessageEl = previewAreaEl.getElementsByClassName(
            'preview-area-loading-message')[0];
        expectTrue(loadingMessageEl.hidden);

        const previewFailedMessageEl = previewAreaEl.getElementsByClassName(
            'preview-area-preview-failed-message')[0];
        expectTrue(previewFailedMessageEl.hidden);

        const printFailedMessageEl = previewAreaEl.getElementsByClassName(
            'preview-area-print-failed')[0];
        expectTrue(printFailedMessageEl.hidden);

        const customMessageEl = previewAreaEl.getElementsByClassName(
            'preview-area-custom-message')[0];
        expectFalse(customMessageEl.hidden);
      });
    });

    // Test custom localized paper names.
    test('CustomPaperNames', function() {
      const customLocalizedMediaName = 'Vendor defined localized media name';
      const customMediaName = 'Vendor defined media name';

      const device = print_preview_test_utils.getCddTemplate('FooDevice');
      device.capabilities.printer.media_size = {
        option: [
          {
            name: 'CUSTOM',
            width_microns: 15900,
            height_microns: 79400,
            is_default: true,
            custom_display_name_localized: [
              {locale: navigator.language, value: customLocalizedMediaName}
            ]
          },
          {
            name: 'CUSTOM',
            width_microns: 15900,
            height_microns: 79400,
            custom_display_name: customMediaName
          }
        ]
      };

      return setupSettingsAndDestinationsWithCapabilities(device).then(
          function() {
            expandMoreSettings();

            checkSectionVisible($('media-size-settings'), true);
            const mediaSelect =
                $('media-size-settings').querySelector('.settings-select');
            // Check the default media item.
            expectEquals(
                customLocalizedMediaName,
                mediaSelect.options[mediaSelect.selectedIndex].text);
            // Check the other media item.
            expectEquals(
                customMediaName,
                mediaSelect.options[mediaSelect.selectedIndex == 0 ? 1 : 0]
                    .text);

            return whenAnimationDone('more-settings');
          });
    });

    // Test advanced settings with 1 capability (should not display settings
    // search box).
    test('AdvancedSettings1Option', function() {
      const device =
          print_preview_test_utils.getCddTemplateWithAdvancedSettings(
              1, 'FooDevice');
      return setupSettingsAndDestinationsWithCapabilities(device).then(
          function() {
            startAdvancedSettingsTest(device);
            checkElementDisplayed(
                $('advanced-settings').querySelector('.search-box-area'),
                false);

            return whenAnimationDone('more-settings');
          });
    });

    // Test advanced settings with 2 capabilities (should have settings search
    // box).
    test('AdvancedSettings2Options', function() {
      const device =
          print_preview_test_utils.getCddTemplateWithAdvancedSettings(
              2, 'FooDevice');
      return setupSettingsAndDestinationsWithCapabilities(device).then(
          function() {
            startAdvancedSettingsTest(device);

            checkElementDisplayed(
                $('advanced-settings').querySelector('.search-box-area'), true);

            return whenAnimationDone('more-settings');
          });
    });

    // Test that initialization with saved destination only issues one call
    // to startPreview.
    test('InitIssuesOneRequest', function() {
      // Load in a bunch of recent destinations with non null capabilities.
      const origin = cr.isChromeOS ? 'chrome_os' : 'local';
      initialSettings.serializedAppStateStr = JSON.stringify({
        version: 2,
        recentDestinations: [1, 2, 3].map(function(i) {
          return {
            id: 'ID' + i,
            origin: origin,
            account: '',
            capabilities: print_preview_test_utils.getCddTemplate('ID' + i),
            displayName: '',
            extensionId: '',
            extensionName: ''
          };
        }),
      });

      // Ensure all capabilities are available for fetch.
      nativeLayer.setLocalDestinationCapabilities(
          print_preview_test_utils.getCddTemplate('ID1'));
      nativeLayer.setLocalDestinationCapabilities(
          print_preview_test_utils.getCddTemplate('ID2'));
      nativeLayer.setLocalDestinationCapabilities(
          print_preview_test_utils.getCddTemplate('ID3'));

      // For crbug.com/666595. If multiple destinations are fetched there may
      // be multiple preview requests. This verifies the first fetch is for
      // ID1, which ensures no other destinations are fetched earlier. The last
      // destination retrieved before timeout will end up in the preview
      // request. Ensure this is also ID1.
      setInitialSettings();
      const initialSettingsSet = nativeLayer.whenCalled('getInitialSettings');
      return initialSettingsSet
          .then(function() {
            return nativeLayer.whenCalled('getPrinterCapabilities');
          })
          .then(function(args) {
            expectEquals('ID1', args.destinationId);
            expectEquals(print_preview.PrinterType.LOCAL, args.type);
            return nativeLayer.whenCalled('getPreview');
          })
          .then(function(previewArgs) {
            const ticket = JSON.parse(previewArgs.printTicket);
            expectEquals(0, ticket.requestID);
            expectEquals('ID1', ticket.deviceName);
          });
    });

    // Test that invalid settings errors disable the print preview and display
    // an error and that the preview dialog can be recovered by selecting a
    // new destination.
    test('InvalidSettingsError', function() {
      const barDevice = print_preview_test_utils.getCddTemplate('BarDevice');
      nativeLayer.setLocalDestinationCapabilities(barDevice);

      // FooDevice is the default printer, so will be selected for the initial
      // preview request.
      nativeLayer.setInvalidPrinterId('FooDevice');
      return Promise
          .all([
            setupSettingsAndDestinationsWithCapabilities(),
            nativeLayer.whenCalled('getPreview'),
          ])
          .then(function() {
            // Print preview should have failed with invalid settings, since
            // FooDevice was set as an invalid printer.
            const previewAreaEl = $('preview-area');
            const customMessageEl = previewAreaEl.getElementsByClassName(
                'preview-area-custom-message')[0];
            expectFalse(customMessageEl.hidden);
            const expectedMessageStart =
                'The selected printer is not available or ' +
                'not installed correctly.';
            expectTrue(
                customMessageEl.textContent.includes(expectedMessageStart));

            // Verify that the print button is disabled
            const printButton = $('print-header').querySelector('button.print');
            checkElementDisplayed(printButton, true);
            expectTrue(printButton.disabled);

            // Reset
            nativeLayer.reset();

            // Select a new destination
            const barDestination =
                printPreview.destinationStore_.destinations().find(
                    d => d.id == 'BarDevice');
            printPreview.destinationStore_.selectDestination(barDestination);
            return waitForPrinterToUpdatePreview();
          })
          .then(function() {
            // Has active print button and successfully 'prints', indicating
            // recovery from error state.
            const printButton = $('print-header').querySelector('button.print');
            expectFalse(printButton.disabled);
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
                expectEquals(barDevice.printer.deviceName, ticket.deviceName);
                expectEquals(
                    print_preview_test_utils.getDefaultOrientation(barDevice) ==
                        'LANDSCAPE',
                    ticket.landscape);
                expectEquals(1, ticket.copies);
                const mediaDefault =
                    print_preview_test_utils.getDefaultMediaSize(barDevice);
                expectEquals(
                    mediaDefault.width_microns, ticket.mediaSize.width_microns);
                expectEquals(
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
    test('InvalidCertificateError', function() {
      const invalidPrinter =
          print_preview_test_utils.createDestinationWithCertificateStatus(
              'FooDevice', 'FooName', true);
      const validPrinter =
          print_preview_test_utils.createDestinationWithCertificateStatus(
              'BarDevice', 'BarName', false);
      setupInvalidCertificateTest([invalidPrinter, validPrinter]);

      // Get references to a few elements for testing.
      const printButton = $('print-header').querySelector('button.print');
      const previewAreaEl = $('preview-area');
      const overlayEl =
          previewAreaEl.getElementsByClassName('preview-area-overlay-layer')[0];
      const cloudPrintMessageEl = previewAreaEl.getElementsByClassName(
          'preview-area-unsupported-cloud-printer')[0];

      return nativeLayer.whenCalled('getInitialSettings')
          .then(function() {
            printPreview.destinationStore_.startLoadCloudDestinations();

            // FooDevice will be selected since it is the most recently used
            // printer, so the invalid certificate error should be shown.
            // The overlay must be visible for the message to be seen.
            expectFalse(overlayEl.classList.contains('invisible'));

            // Verify that the correct message is shown.
            expectFalse(cloudPrintMessageEl.hidden);
            const expectedMessageStart =
                'The selected Google Cloud Print device ' +
                'is no longer supported. Try setting up the printer in your ' +
                'computer\'s system settings.';
            expectTrue(
                cloudPrintMessageEl.textContent.includes(expectedMessageStart));

            // Verify that the print button is disabled
            checkElementDisplayed(printButton, true);
            expectTrue(printButton.disabled);

            // Reset
            nativeLayer.reset();

            // Select a new, valid cloud destination.
            printPreview.destinationStore_.selectDestination(validPrinter);
            return nativeLayer.whenCalled('getPreview');
          })
          .then(function() {
            // Has active print button, indicating recovery from error state.
            expectFalse(printButton.disabled);

            // Note: because in the test it is generally true that the preview
            // request is resolved before the 200ms timeout to show the loading
            // message expires, the message element may not be hidden. It will
            // be hidden the next time a different message, e.g. 'Loading...',
            // is shown in the overlay. However, if this is the case, the
            // overlay should not be visible, so that the message is no longer
            // visible to the user.
            expectTrue(
                cloudPrintMessageEl.hidden ||
                overlayEl.classList.contains('invisible'));
          });
    });

    // Test that GCP invalid certificate printers disable the print preview when
    // selected and display an error and that the preview dialog can be
    // recovered by selecting a new destination. Tests that even if destination
    // was previously selected, the error is cleared.
    test('InvalidCertificateErrorReselectDestination', function() {
      const invalidPrinter =
          print_preview_test_utils.createDestinationWithCertificateStatus(
              'FooDevice', 'FooName', true);
      const validPrinter =
          print_preview_test_utils.createDestinationWithCertificateStatus(
              'BarDevice', 'BarName', false);
      setupInvalidCertificateTest([validPrinter, invalidPrinter]);

      // Get references to a few elements for testing.
      const printButton = $('print-header').querySelector('button.print');
      const previewAreaEl = $('preview-area');
      const overlayEl =
          previewAreaEl.getElementsByClassName('preview-area-overlay-layer')[0];
      const cloudPrintMessageEl = previewAreaEl.getElementsByClassName(
          'preview-area-unsupported-cloud-printer')[0];

      return nativeLayer.whenCalled('getInitialSettings')
          .then(function() {
            // Start loading cloud destinations so that the printer capabilities
            // arrive.
            printPreview.destinationStore_.startLoadCloudDestinations();
            return nativeLayer.whenCalled('getPreview');
          })
          .then(function() {
            // Has active print button.
            expectFalse(printButton.disabled);
            // No error message.
            expectTrue(
                cloudPrintMessageEl.hidden ||
                overlayEl.classList.contains('invisible'));

            // Select the invalid destination and wait for the event.
            const whenInvalid = test_util.eventToPromise(
                print_preview.DestinationStore.EventType
                    .SELECTED_DESTINATION_UNSUPPORTED,
                printPreview.destinationStore_);
            printPreview.destinationStore_.selectDestination(invalidPrinter);
            return whenInvalid;
          })
          .then(function() {
            // Should have error message.
            expectFalse(overlayEl.classList.contains('invisible'));
            expectFalse(cloudPrintMessageEl.hidden);

            // Reset
            nativeLayer.reset();

            // Reselect the valid cloud destination.
            const whenSelected = test_util.eventToPromise(
                print_preview.DestinationStore.EventType.DESTINATION_SELECT,
                printPreview.destinationStore_);
            printPreview.destinationStore_.selectDestination(validPrinter);
            return whenSelected;
          })
          .then(function() {
            // Has active print button.
            expectFalse(printButton.disabled);
            // No error message.
            expectTrue(
                cloudPrintMessageEl.hidden ||
                overlayEl.classList.contains('invisible'));
          });
    });

    // Test that GCP invalid certificate printers disable the print preview when
    // selected and display an error. Verifies that the error prevents the
    // preview from regenerating when settings are toggled.
    test('InvalidCertificateErrorNoPreview', function() {
      const invalidPrinter =
          print_preview_test_utils.createDestinationWithCertificateStatus(
              'FooDevice', 'FooName', true);
      const validPrinter =
          print_preview_test_utils.createDestinationWithCertificateStatus(
              'BarDevice', 'BarName', false);

      // Set the valid printer first. If the invalid printer is the first
      // printer loaded the bug does not occur since the print ticket store is
      // never initialized.
      setupInvalidCertificateTest([validPrinter, invalidPrinter]);

      // Get references to a few elements for testing.
      const printButton = $('print-header').querySelector('button.print');
      const previewAreaEl = $('preview-area');
      const overlayEl =
          previewAreaEl.getElementsByClassName('preview-area-overlay-layer')[0];
      const cloudPrintMessageEl = previewAreaEl.getElementsByClassName(
          'preview-area-unsupported-cloud-printer')[0];

      return nativeLayer.whenCalled('getInitialSettings')
          .then(function() {
            printPreview.destinationStore_.startLoadCloudDestinations();
            return nativeLayer.whenCalled('getPreview');
          })
          .then(function() {
            // Select the invalid destination and wait for the event.
            const whenInvalid = test_util.eventToPromise(
                print_preview.DestinationStore.EventType
                    .SELECTED_DESTINATION_UNSUPPORTED,
                printPreview.destinationStore_);
            printPreview.destinationStore_.selectDestination(invalidPrinter);
            return whenInvalid;
          })
          .then(function() {
            // FooDevice will be selected since it is the most recently used
            // printer, so the invalid certificate error should be shown.
            // The overlay must be visible for the message to be seen.
            expectFalse(overlayEl.classList.contains('invisible'));
            expectFalse(cloudPrintMessageEl.hidden);

            // Verify that the print button is disabled
            checkElementDisplayed(printButton, true);
            expectTrue(printButton.disabled);

            // Reset
            nativeLayer.resetResolver('getPreview');

            // Update the print ticket by changing portrait to landscape and
            // wait for the event to fire.
            const whenTicketChanged = test_util.eventToPromise(
                print_preview.ticket_items.TicketItem.EventType.CHANGE,
                printPreview.printTicketStore_.landscape);
            const promise = nativeLayer.whenCalled('getPreview');
            promise.then(function() {
              assertTrue(false);
            });
            assertFalse(printPreview.printTicketStore_.landscape.getValue());
            printPreview.printTicketStore_.landscape.updateValue(true);
            // Wait for update. It should not result in a call to getPreview().
            return whenTicketChanged;
          })
          .then(function() {
            // Still disabled.
            expectTrue(printButton.disabled);
            // Overlay still visible.
            expectFalse(overlayEl.classList.contains('invisible'));
            expectFalse(cloudPrintMessageEl.hidden);
          });
    });

    // Test that the policy to use the system default printer by default
    // instead of the most recently used destination works.
    test('SystemDefaultPrinterPolicy', function() {
      // Add recent destination.
      initialSettings.serializedAppStateStr = JSON.stringify({
        version: 2,
        recentDestinations: [
          {
            id: 'ID1',
            origin: 'local',
            account: '',
            capabilities: 0,
            displayName: 'One',
            extensionId: '',
            extensionName: '',
          },
        ],
      });

      // Setup local destinations with the system default + recent.
      localDestinationInfos = [
        {printerName: 'One', deviceName: 'ID1'},
        {printerName: 'FooName', deviceName: 'FooDevice'}
      ];
      nativeLayer.setLocalDestinationCapabilities(
          print_preview_test_utils.getCddTemplate('ID1', 'One'));

      return setupSettingsAndDestinationsWithCapabilities().then(function() {
        // The system default destination should be used instead of the
        // most recent destination.
        assertEquals(
            'FooDevice', printPreview.destinationStore_.selectedDestination.id);
      });
    });

    if (cr.isMac) {
      // Test that Mac "Open PDF in Preview" link is treated correctly as a
      // local printer. See crbug.com/741341 and crbug.com/741528
      test('MacOpenPDFInPreview', function() {
        const device = getPdfPrinter();
        initialSettings.printerName = device.printer.deviceName;
        return setupSettingsAndDestinationsWithCapabilities(device)
            .then(function() {
              assertEquals(
                  device.printer.deviceName,
                  printPreview.destinationStore_.selectedDestination.id);
              return nativeLayer.whenCalled('getPreview');
            })
            .then(function() {
              const openPdfPreviewLink = $('open-pdf-in-preview-link');
              checkElementDisplayed(openPdfPreviewLink, true);
              openPdfPreviewLink.click();
              // Should result in a print call and dialog should close.
              return nativeLayer.whenCalled('print');
            })
            .then(
                /**
                 * @param {string} printTicket The print ticket print() was
                 *     called for.
                 */
                function(printTicket) {
                  expectTrue(JSON.parse(printTicket).OpenPDFInPreview);
                  return nativeLayer.whenCalled('dialogClose');
                });
      });

      // Test that the OpenPDFInPreview link is correctly disabled when the
      // print ticket is invalid.
      test('MacOpenPDFInPreviewBadPrintTicket', function() {
        const device = getPdfPrinter();
        initialSettings.printerName = device.printer.deviceName;
        const openPdfPreviewLink = $('open-pdf-in-preview-link');
        return Promise
            .all([
              setupSettingsAndDestinationsWithCapabilities(device),
              nativeLayer.whenCalled('getPreview')
            ])
            .then(function() {
              checkElementDisplayed(openPdfPreviewLink, true);
              expectFalse(openPdfPreviewLink.disabled);
              const pageSettings = $('page-settings');
              checkSectionVisible(pageSettings, true);
              nativeLayer.resetResolver('getPreview');

              // Wait for ticket change.
              const whenTicketChange = test_util.eventToPromise(
                  print_preview.ticket_items.TicketItem.EventType.CHANGE,
                  printPreview.printTicketStore_.pageRange);

              // Set page settings to a bad value
              pageSettings.querySelector('.page-settings-custom-input').value =
                  'abc';
              pageSettings.querySelector('.page-settings-custom-radio').click();

              // No new preview
              nativeLayer.whenCalled('getPreview').then(function() {
                assertTrue(false);
              });

              return whenTicketChange;
            })
            .then(function() {
              // Expect disabled print button and Pdf in preview link
              const printButton =
                  $('print-header').querySelector('button.print');
              checkElementDisplayed(printButton, true);
              expectTrue(printButton.disabled);
              checkElementDisplayed(openPdfPreviewLink, true);
              expectTrue(openPdfPreviewLink.disabled);
            });
      });
    }  // cr.isMac

    // Test that the system dialog link works correctly on Windows
    if (cr.isWindows) {
      test('WinSystemDialogLink', function() {
        return setupSettingsAndDestinationsWithCapabilities()
            .then(function() {
              assertEquals(
                  'FooDevice',
                  printPreview.destinationStore_.selectedDestination.id);
              return nativeLayer.whenCalled('getPreview');
            })
            .then(function() {
              const systemDialogLink = $('system-dialog-link');
              checkElementDisplayed(systemDialogLink, true);
              systemDialogLink.click();
              // Should result in a print call and dialog should close.
              return nativeLayer.whenCalled('print');
            })
            .then(
                /**
                 * @param {string} printTicket The print ticket print() was
                 *     called for.
                 */
                function(printTicket) {
                  expectTrue(JSON.parse(printTicket).showSystemDialog);
                  return nativeLayer.whenCalled('dialogClose');
                });
      });

      // Test that the System Dialog link is correctly disabled when the
      // print ticket is invalid.
      test('WinSystemDialogLinkBadPrintTicket', function() {
        const systemDialogLink = $('system-dialog-link');
        return Promise
            .all([
              setupSettingsAndDestinationsWithCapabilities(),
              nativeLayer.whenCalled('getPreview')
            ])
            .then(function() {
              checkElementDisplayed(systemDialogLink, true);
              expectFalse(systemDialogLink.disabled);

              const pageSettings = $('page-settings');
              checkSectionVisible(pageSettings, true);
              nativeLayer.resetResolver('getPreview');

              // Wait for ticket change.
              const whenTicketChange = test_util.eventToPromise(
                  print_preview.ticket_items.TicketItem.EventType.CHANGE,
                  printPreview.printTicketStore_.pageRange);

              // Set page settings to a bad value
              pageSettings.querySelector('.page-settings-custom-input').value =
                  'abc';
              pageSettings.querySelector('.page-settings-custom-radio').click();

              // No new preview
              nativeLayer.whenCalled('getPreview').then(function() {
                assertTrue(false);
              });
              return whenTicketChange;
            })
            .then(function() {
              // Expect disabled print button and Pdf in preview link
              const printButton =
                  $('print-header').querySelector('button.print');
              checkElementDisplayed(printButton, true);
              expectTrue(printButton.disabled);
              checkElementDisplayed(systemDialogLink, true);
              expectTrue(systemDialogLink.disabled);
            });
      });
    }  // cr.isWindows
  });

  return {
    suiteName: suiteName,
  };
});
