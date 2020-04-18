// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('header_test', function() {
  /** @enum {string} */
  const TestNames = {
    HeaderPrinterTypes: 'header printer types',
    HeaderWithDuplex: 'header with duplex',
    HeaderWithCopies: 'header with copies',
    HeaderWithNup: 'header with nup',
    HeaderChangesForState: 'header changes for state',
  };

  const suiteName = 'HeaderTest';
  suite(suiteName, function() {
    /** @type {?PrintPreviewHeaderElement} */
    let header = null;

    /** @override */
    setup(function() {
      // Only care about copies, duplex, pages, and pages per sheet.
      const settings = {
        copies: {
          value: '1',
          unavailableValue: '1',
          valid: true,
          available: true,
          key: '',
        },
        duplex: {
          value: false,
          unavailableValue: false,
          valid: true,
          available: true,
          key: 'isDuplexEnabled',
        },
        pages: {
          value: [1],
          unavailableValue: [],
          valid: true,
          available: true,
          key: '',
        },
        pagesPerSheet: {
          value: 1,
          unavailableValue: 1,
          valid: true,
          available: true,
          key: '',
        },
      };

      PolymerTest.clearBody();
      header = document.createElement('print-preview-header');
      header.settings = settings;
      header.destination = new print_preview.Destination(
          'FooDevice', print_preview.DestinationType.GOOGLE,
          print_preview.DestinationOrigin.COOKIES, 'FooName',
          true /* isRecent */,
          print_preview.DestinationConnectionStatus.ONLINE);
      header.errorMessage = '';
      header.state = print_preview_new.State.READY;
      document.body.appendChild(header);
    });

    function setPdfDestination() {
      header.set(
          'destination',
          new print_preview.Destination(
              print_preview.Destination.GooglePromotedId.SAVE_AS_PDF,
              print_preview.DestinationType.LOCAL,
              print_preview.DestinationOrigin.LOCAL,
              loadTimeData.getString('printToPDF'), false,
              print_preview.DestinationConnectionStatus.ONLINE));
    }

    // Tests that the 4 different messages (non-virtual printer singular and
    // plural, virtual printer singular and plural) all show up as expected.
    test(assert(TestNames.HeaderPrinterTypes), function() {
      const summary = header.$$('.summary');
      assertEquals('Total: 1 sheet of paper', summary.textContent);
      header.setSetting('pages', [1, 2, 3]);
      assertEquals('Total: 3 sheets of paper', summary.textContent);
      setPdfDestination();
      assertEquals('Total: 3 pages', summary.textContent);
      header.setSetting('pages', [1]);
      assertEquals('Total: 1 page', summary.textContent);
    });

    // Tests that the message is correctly adjusted with a duplex printer.
    test(assert(TestNames.HeaderWithDuplex), function() {
      const summary = header.$$('.summary');
      assertEquals('Total: 1 sheet of paper', summary.textContent);
      header.setSetting('pages', [1, 2, 3]);
      assertEquals('Total: 3 sheets of paper', summary.textContent);
      header.setSetting('duplex', true);
      assertEquals('Total: 2 sheets of paper', summary.textContent);
      header.setSetting('pages', [1, 2]);
      assertEquals('Total: 1 sheet of paper', summary.textContent);
    });

    // Tests that the message is correctly adjusted with multiple copies.
    test(assert(TestNames.HeaderWithCopies), function() {
      const summary = header.$$('.summary');
      assertEquals('Total: 1 sheet of paper', summary.textContent);
      header.setSetting('copies', 4);
      assertEquals('Total: 4 sheets of paper', summary.textContent);
      header.setSetting('duplex', true);
      assertEquals('Total: 4 sheets of paper', summary.textContent);
      header.setSetting('pages', [1, 2]);
      assertEquals('Total: 4 sheets of paper', summary.textContent);
      header.setSetting('duplex', false);
      assertEquals('Total: 8 sheets of paper', summary.textContent);
    });

    // Tests that the message is correctly adjusted for N-up.
    test(assert(TestNames.HeaderWithNup), function() {
      const summary = header.$$('.summary');
      assertEquals('Total: 1 sheet of paper', summary.textContent);
      header.setSetting('pagesPerSheet', 4);
      assertEquals('Total: 1 sheet of paper', summary.textContent);
      header.setSetting('pages', [1, 2, 3, 4, 5, 6]);
      assertEquals('Total: 2 sheets of paper', summary.textContent);
      header.setSetting('duplex', true);
      assertEquals('Total: 1 sheet of paper', summary.textContent);
      header.setSetting('pagesPerSheet', 2);
      assertEquals('Total: 2 sheets of paper', summary.textContent);
      header.setSetting('pagesPerSheet', 3);
      assertEquals('Total: 1 sheet of paper', summary.textContent);
      header.setSetting('copies', 2);
      assertEquals('Total: 2 sheets of paper', summary.textContent);

      // Check PDF destination
      header.setSetting('copies', 1);
      header.setSetting('duplex', false);
      setPdfDestination();
      assertEquals('Total: 2 pages', summary.textContent);
    });

    // Tests that the correct message is shown for non-READY states, and that
    // the print button is disabled appropriately.
    test(assert(TestNames.HeaderChangesForState), function() {
      const summary = header.$$('.summary');
      const printButton = header.$$('.print');
      assertEquals('Total: 1 sheet of paper', summary.textContent);
      assertFalse(printButton.disabled);

      header.set('state', print_preview_new.State.NOT_READY);
      assertEquals('', summary.textContent);
      assertTrue(printButton.disabled);

      header.set('state', print_preview_new.State.PRINTING);
      assertEquals(loadTimeData.getString('printing'), summary.textContent);
      assertTrue(printButton.disabled);
      setPdfDestination();
      assertEquals(loadTimeData.getString('saving'), summary.textContent);

      header.set('state', print_preview_new.State.INVALID_TICKET);
      assertEquals('', summary.textContent);
      assertTrue(printButton.disabled);

      header.set('state', print_preview_new.State.INVALID_PRINTER);
      assertEquals('', summary.textContent);
      assertTrue(printButton.disabled);

      const testError = 'Error printing to cloud print';
      header.set('errorMessage', testError);
      header.set('state', print_preview_new.State.FATAL_ERROR);
      assertEquals(testError, summary.textContent);
      assertTrue(printButton.disabled);
    });
  });

  return {
    suiteName: suiteName,
    TestNames: TestNames,
  };
});
