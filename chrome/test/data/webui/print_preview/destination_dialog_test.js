// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('destination_dialog_test', function() {
  /** @enum {string} */
  const TestNames = {
    PrinterList: 'PrinterList',
  };

  const suiteName = 'DestinationDialogTest';
  suite(suiteName, function() {
    /** @type {?PrintPreviewDestinationDialogElement} */
    let dialog = null;

    /** @type {?print_preview.DestinationStore} */
    let destinationStore = null;

    /** @type {?print_preview.UserInfo} */
    let userInfo = null;

    /** @type {?print_preview.NativeLayer} */
    let nativeLayer = null;

    /** @type {!Array<!print_preview.Destination>} */
    let destinations = [];

    /** @type {!Array<!print_preview.LocalDestinationInfo>} */
    let localDestinations = [];

    /** @type {!Array<!print_preview.RecentDestination>} */
    let recentDestinations = [];

    /** @override */
    setup(function() {
      // Create data classes
      nativeLayer = new print_preview.NativeLayerStub();
      print_preview.NativeLayer.setInstance(nativeLayer);
      userInfo = new print_preview.UserInfo();
      destinationStore = new print_preview.DestinationStore(
          userInfo, new WebUIListenerTracker());
      destinations = print_preview_test_utils.getDestinations(
          nativeLayer, localDestinations);
      recentDestinations =
          [print_preview.makeRecentDestination(destinations[4])];
      destinationStore.init(
          false /* isInAppKioskMode */, 'FooDevice' /* printerName */,
          '' /* serializedDefaultDestinationSelectionRulesStr */,
          recentDestinations /* recentDestinations */);
      nativeLayer.setLocalDestinations(localDestinations);

      // Set up dialog
      dialog = document.createElement('print-preview-destination-dialog');
      dialog.userInfo = userInfo;
      dialog.destinationStore = destinationStore;
      dialog.invitationStore = new print_preview.InvitationStore(userInfo);
      dialog.recentDestinations = recentDestinations;
      document.body.appendChild(dialog);
      return nativeLayer.whenCalled('getPrinterCapabilities')
          .then(function() {
            destinationStore.startLoadAllDestinations();
            dialog.show();
            return nativeLayer.whenCalled('getPrinters');
          })
          .then(function() {
            Polymer.dom.flush();
          });
    });

    // Test that destinations are correctly displayed in the lists.
    test(assert(TestNames.PrinterList), function() {
      const lists =
          dialog.shadowRoot.querySelectorAll('print-preview-destination-list');
      assertEquals(2, lists.length);

      const recentItems = lists[0].shadowRoot.querySelectorAll(
          'print-preview-destination-list-item');
      const printerItems = lists[1].shadowRoot.querySelectorAll(
          'print-preview-destination-list-item');

      assertEquals(1, recentItems.length);

      const getDisplayedName = item => item.$$('.name').textContent;

      assertEquals('FooName', getDisplayedName(recentItems[0]));
      // 5 printers + Save as PDF
      assertEquals(6, printerItems.length);
      // Save as PDF shows up first.
      assertEquals(
          print_preview.Destination.GooglePromotedId.SAVE_AS_PDF,
          getDisplayedName(printerItems[0]));
      // FooName will be second since it was updated by the capabilities fetch.
      assertEquals('FooName', getDisplayedName(printerItems[1]));
      Array.from(printerItems).slice(2).forEach((item, index) => {
        assertEquals(destinations[index].displayName, getDisplayedName(item));
      });

    });
  });

  return {
    suiteName: suiteName,
    TestNames: TestNames,
  };
});
