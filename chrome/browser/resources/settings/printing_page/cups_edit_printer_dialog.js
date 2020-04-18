// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview 'settings-cups-edit-printer-dialog' is a dialog to edit the
 * existing printer's information and re-configure it.
 */

Polymer({
  is: 'settings-cups-edit-printer-dialog',

  behaviors: [
    SetManufacturerModelBehavior,
  ],

  properties: {
    /**
     * If the printer needs to be re-configured.
     * @private {boolean}
     */
    needsReconfigured_: Boolean,

    /**
     * The current PPD in use by the printer.
     * @private
     */
    existingUserPPDMessage_: String,

    networkProtocolActive_: {
      type: Boolean,
      computed: 'isNetworkProtocol_(activePrinter.printerProtocol)',
    },
  },

  observers: [
    'printerInfoChanged_(activePrinter.*)',
  ],

  /** @override */
  ready: function() {
    settings.CupsPrintersBrowserProxyImpl.getInstance()
        .getPrinterPpdManufacturerAndModel(this.activePrinter.printerId)
        .then(
            this.onGetPrinterPpdManufacturerAndModel_.bind(this),
            this.onGetPrinterPpdManufacturerAndModelFailed_.bind(this));
    let basename = this.getBaseName(this.activePrinter.printerPPDPath);
    if (basename) {
      this.existingUserPPDMessage_ =
          loadTimeData.getStringF('currentPpdMessage', basename);
    }
  },

  /**
   * @param {!{path: string, value: string}} change
   * @private
   */
  printerInfoChanged_: function(change) {
    if (change.path != 'activePrinter.printerName')
      this.needsReconfigured_ = true;
  },

  /**
   * @param {!Event} event
   * @private
   */
  onProtocolChange_: function(event) {
    this.set('activePrinter.printerProtocol', event.target.value);
  },

  /** @private */
  onCancelTap_: function() {
    this.$$('add-printer-dialog').close();
  },

  /** @private */
  onSaveTap_: function() {
    if (this.needsReconfigured_) {
      settings.CupsPrintersBrowserProxyImpl.getInstance().addCupsPrinter(
          this.activePrinter);
    } else {
      settings.CupsPrintersBrowserProxyImpl.getInstance().updateCupsPrinter(
          this.activePrinter.printerId, this.activePrinter.printerName);
    }
    this.$$('add-printer-dialog').close();
  },

  /**
   * @param {!CupsPrinterInfo} printer
   * @return {string} The printer's URI that displays in the UI
   * @private
   */
  getPrinterURI_: function(printer) {
    if (!printer) {
      return '';
    } else if (
        printer.printerProtocol && printer.printerAddress &&
        printer.printerQueue) {
      return printer.printerProtocol + '://' + printer.printerAddress + '/' +
          printer.printerQueue;
    } else if (printer.printerProtocol && printer.printerAddress) {
      return printer.printerProtocol + '://' + printer.printerAddress;
    } else {
      return '';
    }
  },

  /**
   * Handler for getPrinterPpdManufacturerAndModel() success case.
   * @param {!PrinterPpdMakeModel} info
   * @private
   */
  onGetPrinterPpdManufacturerAndModel_: function(info) {
    this.set('activePrinter.ppdManufacturer', info.ppdManufacturer);
    this.set('activePrinter.ppdModel', info.ppdModel);

    // |needsReconfigured_| needs to reset to false after |ppdManufacturer| and
    // |ppdModel| are initialized to their correct values.
    this.needsReconfigured_ = false;
  },

  /**
   * Handler for getPrinterPpdManufacturerAndModel() failure case.
   * @private
   */
  onGetPrinterPpdManufacturerAndModelFailed_: function() {
    this.needsReconfigured_ = false;
  },

  /**
   * @param {string} protocol
   * @return {boolean} Whether |protocol| is a network protocol
   * @private
   */
  isNetworkProtocol_: function(protocol) {
    return ['ipp', 'ipps', 'http', 'https', 'socket', 'lpd'].includes(protocol);
  },
});
