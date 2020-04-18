// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview 'settings-cups-printers' is a component for showing CUPS
 * Printer settings subpage (chrome://settings/cupsPrinters). It is used to
 * set up legacy & non-CloudPrint printers on ChromeOS by leveraging CUPS (the
 * unix printing system) and the many open source drivers built for CUPS.
 */
// TODO(xdai): Rename it to 'settings-cups-printers-page'.
Polymer({
  is: 'settings-cups-printers',

  behaviors: [WebUIListenerBehavior],

  properties: {
    /** @type {!Array<!CupsPrinterInfo>} */
    printers: {
      type: Array,
      notify: true,
    },

    prefs: Object,

    /** @type {?CupsPrinterInfo} */
    activePrinter: {
      type: Object,
      notify: true,
    },

    searchTerm: {
      type: String,
    },

    /** @private */
    canAddPrinter_: Boolean,

    /** @private */
    showCupsEditPrinterDialog_: Boolean,

    /**@private */
    addPrinterResultText_: String,
  },

  listeners: {
    'edit-cups-printer-details': 'onShowCupsEditPrinterDialog_',
  },

  /**
   * @type {function()}
   * @private
   */
  networksChangedListener_: function() {},

  /** @override */
  ready: function() {
    this.updateCupsPrintersList_();
    this.refreshNetworks_();
  },

  /** @override */
  attached: function() {
    this.addWebUIListener('on-add-cups-printer', this.onAddPrinter_.bind(this));
    this.addWebUIListener(
        'on-printers-changed', this.printersChanged_.bind(this));
    this.networksChangedListener_ = this.refreshNetworks_.bind(this);
    chrome.networkingPrivate.onNetworksChanged.addListener(
        this.networksChangedListener_);
  },

  /** @override */
  detached: function() {
    chrome.networkingPrivate.onNetworksChanged.removeListener(
        this.networksChangedListener_);
  },

  /**
   * Callback function when networks change.
   * @private
   */
  refreshNetworks_: function() {
    chrome.networkingPrivate.getNetworks(
        {
          'networkType': chrome.networkingPrivate.NetworkType.ALL,
          'configured': true
        },
        this.onNetworksReceived_.bind(this));
  },

  /**
   * Callback function when configured networks are received.
   * @param {!Array<!chrome.networkingPrivate.NetworkStateProperties>} states
   *     A list of network state information for each network.
   * @private
   */
  onNetworksReceived_: function(states) {
    this.canAddPrinter_ = states.some(function(entry) {
      return entry.hasOwnProperty('ConnectionState') &&
          entry.ConnectionState == 'Connected';
    });
  },

  /**
   * @param {PrinterSetupResult} result_code
   * @param {string} printerName
   * @private
   */
  onAddPrinter_: function(result_code, printerName) {
    if (result_code == PrinterSetupResult.SUCCESS) {
      this.updateCupsPrintersList_();
      this.addPrinterResultText_ =
          loadTimeData.getStringF('printerAddedSuccessfulMessage', printerName);
    } else {
      switch (result_code) {
        case PrinterSetupResult.FATAL_ERROR:
          this.addPrinterResultText_ =
              loadTimeData.getString('printerAddedFatalErrorMessage');
          break;
        case PrinterSetupResult.PRINTER_UNREACHABLE:
          this.addPrinterResultText_ =
              loadTimeData.getString('printerAddedUnreachableMessage');
          break;
        case PrinterSetupResult.DBUS_ERROR:
          // Simply display a generic error message as this error should only
          // occur when a call to Dbus fails which isn't meaningful to the user.
          this.addPrinterResultText_ =
              loadTimeData.getString('printerAddedFailedmMessage');
          break;
        case PrinterSetupResult.NATIVE_PRINTERS_NOT_ALLOWED:
          this.addPrinterResultText_ = loadTimeData.getString(
              'printerAddedNativePrintersNotAllowedMessage');
          break;
        case PrinterSetupResult.INVALID_PRINTER_UPDATE:
          this.addPrinterResultText_ =
              loadTimeData.getString('editPrinterInvalidPrinterUpdate');
          break;
        case PrinterSetupResult.PPD_TOO_LARGE:
          this.addPrinterResultText_ =
              loadTimeData.getString('printerAddedPpdTooLargeMessage');
          break;
        case PrinterSetupResult.INVALID_PPD:
          this.addPrinterResultText_ =
              loadTimeData.getString('printerAddedInvalidPpdMessage');
          break;
        case PrinterSetupResult.PPD_NOT_FOUND:
          this.addPrinterResultText_ =
              loadTimeData.getString('printerAddedPpdNotFoundMessage');
          break;
        case PrinterSetupResult.PPD_UNRETRIEVABLE:
          this.addPrinterResultText_ =
              loadTimeData.getString('printerAddedPpdUnretrievableMessage');
          break;
      }
    }

    this.$.errorToast.show();
  },

  /** @private */
  updateCupsPrintersList_: function() {
    settings.CupsPrintersBrowserProxyImpl.getInstance()
        .getCupsPrintersList()
        .then(this.printersChanged_.bind(this));
  },

  /**
   * @param {!CupsPrintersList} cupsPrintersList
   * @private
   */
  printersChanged_: function(cupsPrintersList) {
    this.printers = cupsPrintersList.printerList;
  },

  /** @private */
  onAddPrinterTap_: function() {
    this.$.addPrinterDialog.open();
    this.$.addPrinterErrorMessage.hidden = true;
  },

  /** @private */
  onAddPrinterDialogClose_: function() {
    cr.ui.focusWithoutInk(assert(this.$$('#addPrinter')));
  },

  /** @private */
  onShowCupsEditPrinterDialog_: function() {
    this.showCupsEditPrinterDialog_ = true;
    this.async(function() {
      const dialog = this.$$('settings-cups-edit-printer-dialog');
      dialog.addEventListener('close', function() {
        this.showCupsEditPrinterDialog_ = false;
      }.bind(this));
    });
  },

  /**
   * @param {string} searchTerm
   * @return {boolean} If the 'no-search-results-found' string should be shown.
   * @private
   */
  showNoSearchResultsMessage_: function(searchTerm) {
    if (!searchTerm || !this.printers.length)
      return false;
    searchTerm = searchTerm.toLowerCase();
    return !this.printers.some(printer => {
      return printer.printerName.toLowerCase().includes(searchTerm);
    });
  },

  /**
   * @param {boolean} connectedToNetwork Whether the device is connected to
         a network.
   * @param {boolean} userNativePrintersAllowed Whether users are allowed to
         configure their own native printers.
   * @return {boolean} Whether the 'Add Printer' button is active.
   * @private
   */
  addPrinterButtonActive_: function(
      connectedToNetwork, userNativePrintersAllowed) {
    return connectedToNetwork && userNativePrintersAllowed;
  }
});
