// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview', function() {
  'use strict';

  /**
   * Creates a PrintHeader object. This object encapsulates all the elements
   * and logic related to the top part of the left pane in print_preview.html.
   * @param {!print_preview.PrintTicketStore} printTicketStore Used to read
   *     information about the document.
   * @param {!print_preview.DestinationStore} destinationStore Used to get the
   *     selected destination.
   * @constructor
   * @extends {print_preview.Component}
   */
  function PrintHeader(printTicketStore, destinationStore) {
    print_preview.Component.call(this);

    /**
     * Used to read information about the document.
     * @type {!print_preview.PrintTicketStore}
     * @private
     */
    this.printTicketStore_ = printTicketStore;

    /**
     * Used to get the selected destination.
     * @type {!print_preview.DestinationStore}
     * @private
     */
    this.destinationStore_ = destinationStore;

    /**
     * Whether the component is enabled.
     * @type {boolean}
     * @private
     */
    this.isEnabled_ = true;

    /**
     * Whether the print button is enabled.
     * @type {boolean}
     * @private
     */
    this.isPrintButtonEnabled_ = true;
  }

  /**
   * Event types dispatched by the print header.
   * @enum {string}
   */
  PrintHeader.EventType = {
    PRINT_BUTTON_CLICK: 'print_preview.PrintHeader.PRINT_BUTTON_CLICK',
    CANCEL_BUTTON_CLICK: 'print_preview.PrintHeader.CANCEL_BUTTON_CLICK'
  };

  PrintHeader.prototype = {
    __proto__: print_preview.Component.prototype,

    set isEnabled(isEnabled) {
      this.isEnabled_ = isEnabled;
      this.updatePrintButtonEnabledState_();
      this.isCancelButtonEnabled = isEnabled;
    },

    get isPrintButtonEnabled() {
      return !this.getChildElement('button.print').disabled;
    },

    set isPrintButtonEnabled(isEnabled) {
      this.isPrintButtonEnabled_ = isEnabled;
      this.updatePrintButtonEnabledState_();
    },

    set isCancelButtonEnabled(isEnabled) {
      this.getChildElement('button.cancel').disabled = !isEnabled;
    },

    /** @param {string} message Error message to display in the print header. */
    setErrorMessage: function(message) {
      const summaryEl = this.getChildElement('.summary');
      summaryEl.innerHTML = '';
      summaryEl.textContent = message;
      this.getChildElement('button.print').classList.toggle('loading', false);
      this.getChildElement('button.cancel').classList.toggle('loading', false);
    },

    /** @override */
    decorateInternal: function() {
      cr.ui.reverseButtonStrips(this.getElement());
    },

    /** @override */
    enterDocument: function() {
      print_preview.Component.prototype.enterDocument.call(this);

      // User events
      this.tracker.add(
          this.getChildElement('button.cancel'), 'click',
          this.onCancelButtonClick_.bind(this));
      this.tracker.add(
          this.getChildElement('button.print'), 'click',
          this.onPrintButtonClick_.bind(this));

      // Data events.
      this.tracker.add(
          this.printTicketStore_,
          print_preview.PrintTicketStore.EventType.INITIALIZE,
          this.onTicketChange.bind(this));
      this.tracker.add(
          this.printTicketStore_,
          print_preview.PrintTicketStore.EventType.DOCUMENT_CHANGE,
          this.onTicketChange.bind(this));
      this.tracker.add(
          this.destinationStore_,
          print_preview.DestinationStore.EventType.DESTINATION_SELECT,
          this.onDestinationSelect_.bind(this));
      this.tracker.add(
          this.printTicketStore_.duplex,
          print_preview.ticket_items.TicketItem.EventType.CHANGE,
          this.onTicketChange.bind(this));
    },

    /**
     * Updates Print Button state.
     * @private
     */
    updatePrintButtonEnabledState_: function() {
      this.getChildElement('button.print').disabled =
          this.destinationStore_.selectedDestination == null ||
          !this.isEnabled_ || !this.isPrintButtonEnabled_ ||
          !this.printTicketStore_.isTicketValid();
    },

    /**
     * Updates the summary element based on the currently selected user options.
     * @private
     */
    updateSummary_: function() {
      if (!this.printTicketStore_.isTicketValid()) {
        this.getChildElement('.summary').innerHTML = '';
        return;
      }

      const saveToPdfOrDrive = this.destinationStore_.selectedDestination &&
          (this.destinationStore_.selectedDestination.id ==
               print_preview.Destination.GooglePromotedId.SAVE_AS_PDF ||
           this.destinationStore_.selectedDestination.id ==
               print_preview.Destination.GooglePromotedId.DOCS);

      let numPages = this.printTicketStore_.pageRange.getPageNumberSet().size;
      let numSheets = numPages;
      if (!saveToPdfOrDrive && this.printTicketStore_.duplex.getValue()) {
        numSheets = Math.ceil(numPages / 2);
      }

      const copies = this.printTicketStore_.copies.getValueAsNumber();
      numSheets *= copies;
      numPages *= copies;

      const pagesLabel = loadTimeData.getString('printPreviewPageLabelPlural');
      let summaryLabel;
      if (numSheets > 1) {
        summaryLabel = saveToPdfOrDrive ?
            pagesLabel :
            loadTimeData.getString('printPreviewSheetsLabelPlural');
      } else {
        summaryLabel = loadTimeData.getString(
            saveToPdfOrDrive ? 'printPreviewPageLabelSingular' :
                               'printPreviewSheetsLabelSingular');
      }

      let html;
      let label;
      if (numPages != numSheets) {
        html = loadTimeData.getStringF(
            'printPreviewSummaryFormatLong',
            '<b>' + numSheets.toLocaleString() + '</b>',
            '<b>' + summaryLabel + '</b>', numPages.toLocaleString(),
            pagesLabel);
        label = loadTimeData.getStringF(
            'printPreviewSummaryFormatLong', numSheets.toLocaleString(),
            summaryLabel, numPages.toLocaleString(), pagesLabel);
      } else {
        html = loadTimeData.getStringF(
            'printPreviewSummaryFormatShort',
            '<b>' + numSheets.toLocaleString() + '</b>',
            '<b>' + summaryLabel + '</b>');
        label = loadTimeData.getStringF(
            'printPreviewSummaryFormatShort', numSheets.toLocaleString(),
            summaryLabel);
      }

      // Removing extra spaces from within the string.
      html = html.replace(/\s{2,}/g, ' ');

      const summary = this.getChildElement('.summary');
      summary.innerHTML = html;
      summary.setAttribute('aria-label', label);
    },

    /**
     * Called when the print button is clicked. Dispatches a PRINT_DOCUMENT
     * common event.
     * @private
     */
    onPrintButtonClick_: function() {
      if (this.destinationStore_.selectedDestination.id !=
          print_preview.Destination.GooglePromotedId.SAVE_AS_PDF) {
        this.getChildElement('button.print').classList.add('loading');
        this.getChildElement('button.cancel').classList.add('loading');
        const isSaveLabel =
            (this.destinationStore_.selectedDestination.id ==
             print_preview.Destination.GooglePromotedId.DOCS);
        this.getChildElement('.summary').innerHTML =
            loadTimeData.getString(isSaveLabel ? 'saving' : 'printing');
      }
      cr.dispatchSimpleEvent(this, PrintHeader.EventType.PRINT_BUTTON_CLICK);
    },

    /**
     * Called when the cancel button is clicked. Dispatches a
     * CLOSE_PRINT_PREVIEW event.
     * @private
     */
    onCancelButtonClick_: function() {
      cr.dispatchSimpleEvent(this, PrintHeader.EventType.CANCEL_BUTTON_CLICK);
    },

    /**
     * Called when a new destination is selected. Updates the text on the print
     * button.
     * @private
     */
    onDestinationSelect_: function() {
      const isSaveLabel = this.destinationStore_.selectedDestination &&
          (this.destinationStore_.selectedDestination.id ==
               print_preview.Destination.GooglePromotedId.SAVE_AS_PDF ||
           this.destinationStore_.selectedDestination.id ==
               print_preview.Destination.GooglePromotedId.DOCS);
      this.getChildElement('button.print').textContent =
          loadTimeData.getString(isSaveLabel ? 'saveButton' : 'printButton');
      if (this.destinationStore_.selectedDestination) {
        this.getChildElement('button.print').focus();
      }
    },

    /**
     * Called when the print ticket has changed. Disables the print button if
     * any of the settings are invalid.
     */
    onTicketChange: function() {
      this.updatePrintButtonEnabledState_();
      this.updateSummary_();
      if (document.activeElement == null ||
          document.activeElement == document.body) {
        this.getChildElement('button.print').focus();
      }
    }
  };

  // Export
  return {PrintHeader: PrintHeader};
});
