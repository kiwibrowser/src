// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('system_dialog_browsertest', function() {
  /** @enum {string} */
  const TestNames = {
    LinkTriggersLocalPrint: 'link triggers local print',
    InvalidSettingsDisableLink: 'invalid settings disable link',
  };

  const suiteName = 'SystemDialogBrowserTest';
  suite(suiteName, function() {
    /** @type {?PrintPreviewAppElement} */
    let page = null;

    /** @type {?print_preview.NativeLayer} */
    let nativeLayer = null;

    /** @type {?PrintPreviewLinkContainerElement} */
    let linkContainer = null;

    /** @type {?HTMLElement} */
    let link = null;

    /** @type {string} */
    let printTicketKey = '';

    /** @override */
    setup(function() {
      nativeLayer = new print_preview.NativeLayerStub();
      print_preview.NativeLayer.setInstance(nativeLayer);
      PolymerTest.clearBody();

      const initialSettings =
          print_preview_test_utils.getDefaultInitialSettings();
      nativeLayer.setInitialSettings(initialSettings);
      nativeLayer.setLocalDestinationCapabilities(
          print_preview_test_utils.getCddTemplate(initialSettings.printerName));

      page = document.createElement('print-preview-app');
      linkContainer = page.$$('print-preview-link-container');
      const previewArea = page.$$('print-preview-preview-area');
      previewArea.plugin_ = new print_preview.PDFPluginStub(
          previewArea.onPluginLoad_.bind(previewArea));
      document.body.appendChild(page);
      return Promise
          .all([
            nativeLayer.whenCalled('getInitialSettings'),
            nativeLayer.whenCalled('getPrinterCapabilities'),
          ])
          .then(function() {
            return nativeLayer.whenCalled('getPreview');
          })
          .then(function() {
            assertEquals('FooDevice', page.destination_.id);
            link = cr.isWindows ? linkContainer.$.systemDialogLink :
                                  linkContainer.$.openPdfInPreviewLink;
            printTicketKey =
                cr.isWindows ? 'showSystemDialog' : 'OpenPDFInPreview';
          });
    });

    test(assert(TestNames.LinkTriggersLocalPrint), function() {
      assertFalse(linkContainer.disabled);
      assertFalse(link.hidden);
      link.click();
      // Should result in a print call and dialog should close.
      return nativeLayer.whenCalled('print').then(function(printTicket) {
        expectTrue(JSON.parse(printTicket)[printTicketKey]);
        return nativeLayer.whenCalled('dialogClose');
      });
    });

    test(assert(TestNames.InvalidSettingsDisableLink), function() {
      assertFalse(linkContainer.disabled);
      assertFalse(link.hidden);

      const pageSettings = page.$$('print-preview-pages-settings');
      assertFalse(pageSettings.hidden);
      nativeLayer.resetResolver('getPreview');

      // Set page settings to a bad value
      pageSettings.$$('#custom-radio-button').checked = true;
      pageSettings.$$('#all-radio-button')
          .dispatchEvent(new CustomEvent('change'));
      const pageSettingsInput = pageSettings.$$('.user-value');
      pageSettingsInput.value = 'abc';
      pageSettingsInput.dispatchEvent(new CustomEvent('input'));

      // No new preview
      nativeLayer.whenCalled('getPreview').then(function() {
        assertTrue(false);
      });

      return test_util.eventToPromise('input-change', pageSettings)
          .then(function() {
            // Expect disabled print button and Pdf in preview link
            const header = page.$$('print-preview-header');
            const printButton = header.$$('.print');
            assertTrue(printButton.disabled);
            assertTrue(linkContainer.disabled);
            assertFalse(link.hidden);
            assertTrue(link.disabled);
          });
    });
  });

  return {
    suiteName: suiteName,
    TestNames: TestNames,
  };
});
