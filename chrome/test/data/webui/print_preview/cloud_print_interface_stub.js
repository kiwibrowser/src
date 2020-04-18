// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview', function() {
  /**
   * Test version of the cloud print interface.
   */
  class CloudPrintInterfaceStub extends cr.EventTarget {
    constructor() {
      super();

      /** @private {!Map<string, !print_preview.Destination>} */
      this.cloudPrintersMap_ = new Map();
    }

    /**
     * @param {string} id The ID of the printer.
     * @param {!print_preview.Destination} printer The destination to return
     *     when the printer is requested.
     */
    setPrinter(id, printer) {
      this.cloudPrintersMap_.set(id, printer);
    }

    /**
     * Dispatches a CloudPrintInterfaceEventType.SEARCH_DONE event with the
     * printers that have been set so far using setPrinter().
     */
    search() {
      const searchDoneEvent =
          new Event(cloudprint.CloudPrintInterfaceEventType.SEARCH_DONE);
      searchDoneEvent.origin = print_preview.DestinationOrigin.COOKIES;
      searchDoneEvent.printers = [];
      this.cloudPrintersMap_.forEach((value) => {
        searchDoneEvent.printers.push(value);
      });
      searchDoneEvent.isRecent = true;
      searchDoneEvent.user = 'foo@chromium.org';
      searchDoneEvent.searchDone = true;
      this.dispatchEvent(searchDoneEvent);
    }

    /** @param {string} account Account the request is sent for. */
    invites(account) {}

    /**
     * Dispatches a CloudPrintInterfaceEventType.PRINTER_DONE event with the
     * printer details if the printer has been added by calling setPrinter().
     * @param {string} printerId ID of the printer to lookup.
     * @param {!print_preview.DestinationOrigin} origin Origin of the printer.
     * @param {string=} account Account this printer is registered for.
     */
    printer(printerId, origin, account) {
      const printer = this.cloudPrintersMap_.get(printerId);
      if (!!printer) {
        const printerDoneEvent =
            new Event(cloudprint.CloudPrintInterfaceEventType.PRINTER_DONE);
        printerDoneEvent.printer = printer;
        printerDoneEvent.printer.capabilities =
            print_preview_test_utils.getCddTemplate(printerId);
        this.dispatchEvent(printerDoneEvent);
      }
    }
  }

  return {
    CloudPrintInterfaceStub: CloudPrintInterfaceStub,
  };
});
