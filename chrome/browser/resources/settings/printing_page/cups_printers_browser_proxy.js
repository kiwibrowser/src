// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview A helper object used from the "CUPS printing" section to
 * interact with the browser. Used only on Chrome OS.
 */

/**
 * @typedef {{
 *   ppdManufacturer: string,
 *   ppdModel: string,
 *   printerAddress: string,
 *   printerAutoconf: boolean,
 *   printerDescription: string,
 *   printerId: string,
 *   printerManufacturer: string,
 *   printerModel: string,
 *   printerMakeAndModel: string,
 *   printerName: string,
 *   printerPPDPath: string,
 *   printerProtocol: string,
 *   printerQueue: string,
 *   printerStatus: string,
 * }}
 */
let CupsPrinterInfo;

/**
 * @typedef {{
 *   printerList: !Array<!CupsPrinterInfo>,
 * }}
 */
let CupsPrintersList;

/**
 * @typedef {{
 *   success: boolean,
 *   manufacturers: Array<string>
 * }}
 */
let ManufacturersInfo;

/**
 * @typedef {{
 *   success: boolean,
 *   models: Array<string>
 * }}
 */
let ModelsInfo;

/**
 * @typedef {{
 *   manufacturer: string,
 *   model: string,
 *   makeAndModel: string,
 *   autoconf: boolean
 * }}
 */
let PrinterMakeModel;

/**
 * @typedef {{
 *   ppdManufacturer: string,
 *   ppdModel: string
 * }}
 */
let PrinterPpdMakeModel;

/**
 *  @enum {number}
 *  These values must be kept in sync with the PrinterSetupResult enum in
 *  chrome/browser/chromeos/printing/printer_configurer.h.
 */
const PrinterSetupResult = {
  FATAL_ERROR: 0,
  SUCCESS: 1,
  PRINTER_UNREACHABLE: 2,
  DBUS_ERROR: 3,
  NATIVE_PRINTERS_NOT_ALLOWED: 4,
  INVALID_PRINTER_UPDATE: 5,
  PPD_TOO_LARGE: 10,
  INVALID_PPD: 11,
  PPD_NOT_FOUND: 12,
  PPD_UNRETRIEVABLE: 13,
};

/**
 * @typedef {{
 *   message: string
 * }}
 */
let QueryFailure;

cr.define('settings', function() {
  /** @interface */
  class CupsPrintersBrowserProxy {
    /**
     * @return {!Promise<!CupsPrintersList>}
     */
    getCupsPrintersList() {}

    /**
     * @param {string} printerId
     * @param {string} printerName
     */
    updateCupsPrinter(printerId, printerName) {}

    /**
     * @param {string} printerId
     * @param {string} printerName
     */
    removeCupsPrinter(printerId, printerName) {}

    /**
     * @return {!Promise<string>} The full path of the printer PPD file.
     */
    getCupsPrinterPPDPath() {}

    /**
     * @param {!CupsPrinterInfo} newPrinter
     */
    addCupsPrinter(newPrinter) {}

    startDiscoveringPrinters() {}
    stopDiscoveringPrinters() {}

    /**
     * @return {!Promise<!ManufacturersInfo>}
     */
    getCupsPrinterManufacturersList() {}

    /**
     * @param {string} manufacturer
     * @return {!Promise<!ModelsInfo>}
     */
    getCupsPrinterModelsList(manufacturer) {}

    /**
     * @param {!CupsPrinterInfo} newPrinter
     * @return {!Promise<!PrinterMakeModel>}
     */
    getPrinterInfo(newPrinter) {}

    /**
     * @param {string} printerId
     * @return {!Promise<!PrinterPpdMakeModel>}
     */
    getPrinterPpdManufacturerAndModel(printerId) {}

    /**
     * @param{string} printerId
     */
    addDiscoveredPrinter(printerId) {}

    /**
     * Report to the handler that setup was cancelled.
     * @param {!CupsPrinterInfo} newPrinter
     */
    cancelPrinterSetUp(newPrinter) {}
  }

  /**
   * @implements {settings.CupsPrintersBrowserProxy}
   */
  class CupsPrintersBrowserProxyImpl {
    /** @override */
    getCupsPrintersList() {
      return cr.sendWithPromise('getCupsPrintersList');
    }

    /** @override */
    updateCupsPrinter(printerId, printerName) {
      chrome.send('updateCupsPrinter', [printerId, printerName]);
    }

    /** @override */
    removeCupsPrinter(printerId, printerName) {
      chrome.send('removeCupsPrinter', [printerId, printerName]);
    }

    /** @override */
    addCupsPrinter(newPrinter) {
      chrome.send('addCupsPrinter', [newPrinter]);
    }

    /** @override */
    getCupsPrinterPPDPath() {
      return cr.sendWithPromise('selectPPDFile');
    }

    /** @override */
    startDiscoveringPrinters() {
      chrome.send('startDiscoveringPrinters');
    }

    /** @override */
    stopDiscoveringPrinters() {
      chrome.send('stopDiscoveringPrinters');
    }

    /** @override */
    getCupsPrinterManufacturersList() {
      return cr.sendWithPromise('getCupsPrinterManufacturersList');
    }

    /** @override */
    getCupsPrinterModelsList(manufacturer) {
      return cr.sendWithPromise('getCupsPrinterModelsList', manufacturer);
    }

    /** @override */
    getPrinterInfo(newPrinter) {
      return cr.sendWithPromise('getPrinterInfo', newPrinter);
    }

    /** @override */
    getPrinterPpdManufacturerAndModel(printerId) {
      return cr.sendWithPromise('getPrinterPpdManufacturerAndModel', printerId);
    }

    /** @override */
    addDiscoveredPrinter(printerId) {
      chrome.send('addDiscoveredPrinter', [printerId]);
    }

    /** @override */
    cancelPrinterSetUp(newPrinter) {
      chrome.send('cancelPrinterSetUp', [newPrinter]);
    }
  }

  cr.addSingletonGetter(CupsPrintersBrowserProxyImpl);

  return {
    CupsPrintersBrowserProxy: CupsPrintersBrowserProxy,
    CupsPrintersBrowserProxyImpl: CupsPrintersBrowserProxyImpl,
  };
});
