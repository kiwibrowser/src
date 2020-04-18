// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview', function() {
  'use strict';

  // TODO(rltoscano): Maybe clear print ticket when destination changes. Or
  // better yet, carry over any print ticket state that is possible. I.e. if
  // destination changes, the new destination might not support duplex anymore,
  // so we should clear the ticket's isDuplexEnabled state.

  class PrintTicketStore extends cr.EventTarget {
    /**
     * Storage of the print ticket and document statistics. Dispatches events
     * when the contents of the print ticket or document statistics change. Also
     * handles validation of the print ticket against destination capabilities
     * and against the document.
     * @param {!print_preview.DestinationStore} destinationStore Used to
     *     understand which printer is selected.
     * @param {!print_preview.AppState} appState Print preview application
     * state.
     * @param {!print_preview.DocumentInfo} documentInfo Document data model.
     */
    constructor(destinationStore, appState, documentInfo) {
      super();

      /**
       * Destination store used to understand which printer is selected.
       * @type {!print_preview.DestinationStore}
       * @private
       */
      this.destinationStore_ = destinationStore;

      /**
       * App state used to persist and load ticket values.
       * @type {!print_preview.AppState}
       * @private
       */
      this.appState_ = appState;

      /**
       * Information about the document to print.
       * @type {!print_preview.DocumentInfo}
       * @private
       */
      this.documentInfo_ = documentInfo;

      /**
       * The destination that capabilities were last received for.
       * @private {?print_preview.Destination}
       */
      this.destination_ = null;

      /**
       * Printing capabilities of Chromium and the currently selected
       * destination.
       * @type {!print_preview.CapabilitiesHolder}
       * @private
       */
      this.capabilitiesHolder_ = new print_preview.CapabilitiesHolder();

      /**
       * Current measurement system. Used to work with margin measurements.
       * @type {!print_preview.MeasurementSystem}
       * @private
       */
      this.measurementSystem_ = new print_preview.MeasurementSystem(
          ',', '.', print_preview.MeasurementSystemUnitType.IMPERIAL);

      /**
       * Collate ticket item.
       * @type {!print_preview.ticket_items.Collate}
       * @private
       */
      this.collate_ = new print_preview.ticket_items.Collate(
          this.appState_, this.destinationStore_);

      /**
       * Color ticket item.
       * @type {!print_preview.ticket_items.Color}
       * @private
       */
      this.color_ = new print_preview.ticket_items.Color(
          this.appState_, this.destinationStore_);

      /**
       * Copies ticket item.
       * @type {!print_preview.ticket_items.Copies}
       * @private
       */
      this.copies_ =
          new print_preview.ticket_items.Copies(this.destinationStore_);

      /**
       * DPI ticket item.
       * @type {!print_preview.ticket_items.Dpi}
       * @private
       */
      this.dpi_ = new print_preview.ticket_items.Dpi(
          this.appState_, this.destinationStore_);

      /**
       * Duplex ticket item.
       * @type {!print_preview.ticket_items.Duplex}
       * @private
       */
      this.duplex_ = new print_preview.ticket_items.Duplex(
          this.appState_, this.destinationStore_);

      /**
       * Page range ticket item.
       * @type {!print_preview.ticket_items.PageRange}
       * @private
       */
      this.pageRange_ =
          new print_preview.ticket_items.PageRange(this.documentInfo_);

      /**
       * Rasterize PDF ticket item.
       * @type {!print_preview.ticket_items.Rasterize}
       * @private
       */
      this.rasterize_ = new print_preview.ticket_items.Rasterize(
          this.destinationStore_, this.documentInfo_);

      /**
       * Scaling ticket item.
       * @type {!print_preview.ticket_items.Scaling}
       * @private
       */
      this.scaling_ = new print_preview.ticket_items.Scaling(
          this.appState_, this.destinationStore_, this.documentInfo_);

      /**
       * Custom margins ticket item.
       * @type {!print_preview.ticket_items.CustomMargins}
       * @private
       */
      this.customMargins_ = new print_preview.ticket_items.CustomMargins(
          this.appState_, this.documentInfo_);

      /**
       * Margins type ticket item.
       * @type {!print_preview.ticket_items.MarginsType}
       * @private
       */
      this.marginsType_ = new print_preview.ticket_items.MarginsType(
          this.appState_, this.documentInfo_, this.customMargins_);

      /**
       * Media size ticket item.
       * @type {!print_preview.ticket_items.MediaSize}
       * @private
       */
      this.mediaSize_ = new print_preview.ticket_items.MediaSize(
          this.appState_, this.destinationStore_, this.documentInfo_,
          this.marginsType_, this.customMargins_);

      /**
       * Landscape ticket item.
       * @type {!print_preview.ticket_items.Landscape}
       * @private
       */
      this.landscape_ = new print_preview.ticket_items.Landscape(
          this.appState_, this.destinationStore_, this.documentInfo_,
          this.marginsType_, this.customMargins_);

      /**
       * Header-footer ticket item.
       * @type {!print_preview.ticket_items.HeaderFooter}
       * @private
       */
      this.headerFooter_ = new print_preview.ticket_items.HeaderFooter(
          this.appState_, this.documentInfo_, this.marginsType_,
          this.customMargins_, this.mediaSize_, this.landscape_);

      /**
       * Fit-to-page ticket item.
       * @type {!print_preview.ticket_items.FitToPage}
       * @private
       */
      this.fitToPage_ = new print_preview.ticket_items.FitToPage(
          this.appState_, this.documentInfo_, this.destinationStore_);

      /**
       * Print CSS backgrounds ticket item.
       * @type {!print_preview.ticket_items.CssBackground}
       * @private
       */
      this.cssBackground_ = new print_preview.ticket_items.CssBackground(
          this.appState_, this.documentInfo_);

      /**
       * Print selection only ticket item.
       * @type {!print_preview.ticket_items.SelectionOnly}
       * @private
       */
      this.selectionOnly_ =
          new print_preview.ticket_items.SelectionOnly(this.documentInfo_);

      /**
       * Vendor ticket items.
       * @type {!print_preview.ticket_items.VendorItems}
       * @private
       */
      this.vendorItems_ = new print_preview.ticket_items.VendorItems(
          this.appState_, this.destinationStore_);

      /**
       * Keeps track of event listeners for the print ticket store.
       * @type {!EventTracker}
       * @private
       */
      this.tracker_ = new EventTracker();

      /**
       * Whether the print preview has been initialized.
       * @type {boolean}
       * @private
       */
      this.isInitialized_ = false;

      this.addEventListeners_();
    }

    get isInitialized() {
      return this.isInitialized_;
    }

    get collate() {
      return this.collate_;
    }

    get color() {
      return this.color_;
    }

    get copies() {
      return this.copies_;
    }

    get cssBackground() {
      return this.cssBackground_;
    }

    get customMargins() {
      return this.customMargins_;
    }

    get dpi() {
      return this.dpi_;
    }

    get duplex() {
      return this.duplex_;
    }

    get fitToPage() {
      return this.fitToPage_;
    }

    get headerFooter() {
      return this.headerFooter_;
    }

    get mediaSize() {
      return this.mediaSize_;
    }

    get landscape() {
      return this.landscape_;
    }

    get marginsType() {
      return this.marginsType_;
    }

    get pageRange() {
      return this.pageRange_;
    }

    get rasterize() {
      return this.rasterize_;
    }

    get scaling() {
      return this.scaling_;
    }

    get selectionOnly() {
      return this.selectionOnly_;
    }

    get vendorItems() {
      return this.vendorItems_;
    }

    get measurementSystem() {
      return this.measurementSystem_;
    }

    /**
     * Initializes the print ticket store. Dispatches an INITIALIZE event.
     * @param {string} thousandsDelimeter Delimeter of the thousands place.
     * @param {string} decimalDelimeter Delimeter of the decimal point.
     * @param {!print_preview.MeasurementSystemUnitType} unitType Type of unit
     *     of the local measurement system.
     * @param {boolean} selectionOnly Whether only selected content should be
     *     printed.
     */
    init(thousandsDelimeter, decimalDelimeter, unitType, selectionOnly) {
      this.measurementSystem_.setSystem(
          thousandsDelimeter, decimalDelimeter, unitType);
      this.selectionOnly_.updateValue(selectionOnly);

      // Initialize ticket with user's previous values.
      if (this.appState_.hasField(
              print_preview.AppStateField.IS_COLOR_ENABLED)) {
        this.color_.updateValue(
            /** @type {!Object} */ (this.appState_.getField(
                print_preview.AppStateField.IS_COLOR_ENABLED)));
      }
      if (this.appState_.hasField(print_preview.AppStateField.DPI)) {
        this.dpi_.updateValue(
            /** @type {!Object} */ (
                this.appState_.getField(print_preview.AppStateField.DPI)));
      }
      if (this.appState_.hasField(
              print_preview.AppStateField.IS_DUPLEX_ENABLED)) {
        this.duplex_.updateValue(
            /** @type {!Object} */ (this.appState_.getField(
                print_preview.AppStateField.IS_DUPLEX_ENABLED)));
      }
      if (this.appState_.hasField(print_preview.AppStateField.MEDIA_SIZE)) {
        this.mediaSize_.updateValue(
            /** @type {!Object} */ (this.appState_.getField(
                print_preview.AppStateField.MEDIA_SIZE)));
      }
      if (this.appState_.hasField(
              print_preview.AppStateField.IS_LANDSCAPE_ENABLED)) {
        this.landscape_.updateValue(
            /** @type {!Object} */ (this.appState_.getField(
                print_preview.AppStateField.IS_LANDSCAPE_ENABLED)));
      }
      // Initialize margins after landscape because landscape may reset margins.
      if (this.appState_.hasField(print_preview.AppStateField.MARGINS_TYPE)) {
        this.marginsType_.updateValue(
            /** @type {!Object} */ (this.appState_.getField(
                print_preview.AppStateField.MARGINS_TYPE)));
      }
      if (this.appState_.hasField(print_preview.AppStateField.CUSTOM_MARGINS)) {
        this.customMargins_.updateValue(
            /** @type {!Object} */ (this.appState_.getField(
                print_preview.AppStateField.CUSTOM_MARGINS)));
      }
      if (this.appState_.hasField(
              print_preview.AppStateField.IS_HEADER_FOOTER_ENABLED)) {
        this.headerFooter_.updateValue(
            /** @type {!Object} */ (this.appState_.getField(
                print_preview.AppStateField.IS_HEADER_FOOTER_ENABLED)));
      }
      if (this.appState_.hasField(
              print_preview.AppStateField.IS_COLLATE_ENABLED)) {
        this.collate_.updateValue(
            /** @type {!Object} */ (this.appState_.getField(
                print_preview.AppStateField.IS_COLLATE_ENABLED)));
      }
      if (this.appState_.hasField(
              print_preview.AppStateField.IS_FIT_TO_PAGE_ENABLED)) {
        this.fitToPage_.updateValue(
            /** @type {!Object} */ (this.appState_.getField(
                print_preview.AppStateField.IS_FIT_TO_PAGE_ENABLED)));
      }
      if (this.appState_.hasField(print_preview.AppStateField.SCALING)) {
        this.scaling_.updateValue(
            /** @type {!Object} */ (
                this.appState_.getField(print_preview.AppStateField.SCALING)));
      }
      if (this.appState_.hasField(
              print_preview.AppStateField.IS_CSS_BACKGROUND_ENABLED)) {
        this.cssBackground_.updateValue(
            /** @type {!Object} */ (this.appState_.getField(
                print_preview.AppStateField.IS_CSS_BACKGROUND_ENABLED)));
      }
      if (this.appState_.hasField(print_preview.AppStateField.VENDOR_OPTIONS)) {
        this.vendorItems_.updateValue(
            /** @type {!Object<string>} */ (this.appState_.getField(
                print_preview.AppStateField.VENDOR_OPTIONS)));
      }
    }

    /**
     * @return {boolean} {@code true} if the stored print ticket is valid,
     *     {@code false} otherwise.
     */
    isTicketValid() {
      return this.isTicketValidForPreview() &&
          (!this.copies_.isCapabilityAvailable() || this.copies_.isValid()) &&
          (!this.pageRange_.isCapabilityAvailable() ||
           this.pageRange_.isValid());
    }

    /** @return {boolean} Whether the ticket is valid for preview generation. */
    isTicketValidForPreview() {
      return (
          (!this.marginsType_.isCapabilityAvailable() ||
           !this.marginsType_.isValueEqual(
               print_preview.ticket_items.MarginsTypeValue.CUSTOM) ||
           this.customMargins_.isValid()) &&
          (!this.scaling_.isCapabilityAvailable() || this.scaling_.isValid()));
    }

    /**
     * Creates an object that represents a Google Cloud Print print ticket.
     * @param {!print_preview.Destination} destination Destination to print to.
     * @return {string} Google Cloud Print print ticket.
     */
    createPrintTicket(destination) {
      assert(
          !destination.isLocal || destination.isPrivet ||
              destination.isExtension,
          'Trying to create a Google Cloud Print print ticket for a local ' +
              ' non-privet and non-extension destination');

      assert(
          destination.capabilities,
          'Trying to create a Google Cloud Print print ticket for a ' +
              'destination with no print capabilities');
      const cjt = {version: '1.0', print: {}};
      if (this.collate.isCapabilityAvailable() && this.collate.isUserEdited()) {
        cjt.print.collate = {collate: this.collate.getValue()};
      }
      if (this.color.isCapabilityAvailable() && this.color.isUserEdited()) {
        const selectedOption =
            destination.getSelectedColorOption(this.color.getValue());
        if (!selectedOption) {
          console.error('Could not find correct color option');
        } else {
          cjt.print.color = {type: selectedOption.type};
          if (selectedOption.hasOwnProperty('vendor_id')) {
            cjt.print.color.vendor_id = selectedOption.vendor_id;
          }
        }
      } else {
        // Always try setting the color in the print ticket, otherwise a
        // reasonable reader of the ticket will have to do more work, or process
        // the ticket sub-optimally, in order to safely handle the lack of a
        // color ticket item.
        const defaultOption = destination.defaultColorOption;
        if (defaultOption) {
          cjt.print.color = {type: defaultOption.type};
          if (defaultOption.hasOwnProperty('vendor_id')) {
            cjt.print.color.vendor_id = defaultOption.vendor_id;
          }
        }
      }

      if (this.copies.isCapabilityAvailable() && this.copies.isUserEdited()) {
        cjt.print.copies = {copies: this.copies.getValueAsNumber()};
      }
      if (this.duplex.isCapabilityAvailable() && this.duplex.isUserEdited()) {
        cjt.print.duplex = {
          type: this.duplex.getValue() ? 'LONG_EDGE' : 'NO_DUPLEX'
        };
      }
      if (this.mediaSize.isCapabilityAvailable()) {
        const mediaValue = this.mediaSize.getValue();
        cjt.print.media_size = {
          width_microns: mediaValue.width_microns,
          height_microns: mediaValue.height_microns,
          is_continuous_feed: mediaValue.is_continuous_feed,
          vendor_id: mediaValue.vendor_id
        };
      }
      if (!this.landscape.isCapabilityAvailable()) {
        // In this case "orientation" option is hidden from user, so user can't
        // adjust it for page content, see Landscape.isCapabilityAvailable().
        // We can improve results if we set AUTO here.
        if (this.landscape.hasOption('AUTO'))
          cjt.print.page_orientation = {type: 'AUTO'};
      } else if (this.landscape.isUserEdited()) {
        cjt.print.page_orientation = {
          type: this.landscape.getValue() ? 'LANDSCAPE' : 'PORTRAIT'
        };
      }
      if (this.dpi.isCapabilityAvailable()) {
        const dpiValue = this.dpi.getValue();
        cjt.print.dpi = {
          horizontal_dpi: dpiValue.horizontal_dpi,
          vertical_dpi: dpiValue.vertical_dpi,
          vendor_id: dpiValue.vendor_id
        };
      }
      if (this.vendorItems.isCapabilityAvailable() &&
          this.vendorItems.isUserEdited()) {
        const items = this.vendorItems.ticketItems;
        cjt.print.vendor_ticket_item = [];
        for (const itemId in items) {
          if (items.hasOwnProperty(itemId)) {
            cjt.print.vendor_ticket_item.push(
                {id: itemId, value: items[itemId]});
          }
        }
      }
      return JSON.stringify(cjt);
    }

    /**
     * Adds event listeners for the print ticket store.
     * @private
     */
    addEventListeners_() {
      this.tracker_.add(
          this.destinationStore_,
          print_preview.DestinationStore.EventType
              .SELECTED_DESTINATION_CAPABILITIES_READY,
          this.onSelectedDestinationCapabilitiesReady_.bind(this));

      this.tracker_.add(
          this.destinationStore_,
          print_preview.DestinationStore.EventType
              .CACHED_SELECTED_DESTINATION_INFO_READY,
          this.onSelectedDestinationCapabilitiesReady_.bind(this));

      // TODO(rltoscano): Print ticket store shouldn't be re-dispatching these
      // events, the consumers of the print ticket store events should listen
      // for the events from document info instead. Will move this when
      // consumers are all migrated.
      this.tracker_.add(
          this.documentInfo_, print_preview.DocumentInfo.EventType.CHANGE,
          this.onDocumentInfoChange_.bind(this));
    }

    /**
     * Called when the capabilities of the selected destination are ready.
     * @private
     */
    onSelectedDestinationCapabilitiesReady_() {
      const selectedDestination = this.destinationStore_.selectedDestination;
      const isFirstUpdate = this.capabilitiesHolder_.get() == null;
      // Only clear the ticket items if the user selected a new destination
      // and this is not the first update.
      if (!isFirstUpdate && this.destination_ != selectedDestination) {
        this.customMargins_.updateValue(null);
        if (this.marginsType_.getValue() ==
            print_preview.ticket_items.MarginsTypeValue.CUSTOM) {
          this.marginsType_.updateValue(
              print_preview.ticket_items.MarginsTypeValue.DEFAULT);
        }
        this.vendorItems_.updateValue({});
      }
      const caps = assert(selectedDestination.capabilities);
      this.capabilitiesHolder_.set(caps);
      this.destination_ = selectedDestination;
      if (isFirstUpdate) {
        this.isInitialized_ = true;
        cr.dispatchSimpleEvent(this, PrintTicketStore.EventType.INITIALIZE);
      } else {
        cr.dispatchSimpleEvent(
            this, PrintTicketStore.EventType.CAPABILITIES_CHANGE);
      }
    }

    /**
     * Called when document data model has changed. Dispatches a print ticket
     * store event.
     * @private
     */
    onDocumentInfoChange_() {
      cr.dispatchSimpleEvent(this, PrintTicketStore.EventType.DOCUMENT_CHANGE);
    }
  }

  /**
   * Event types dispatched by the print ticket store.
   * @enum {string}
   */
  PrintTicketStore.EventType = {
    CAPABILITIES_CHANGE: 'print_preview.PrintTicketStore.CAPABILITIES_CHANGE',
    DOCUMENT_CHANGE: 'print_preview.PrintTicketStore.DOCUMENT_CHANGE',
    INITIALIZE: 'print_preview.PrintTicketStore.INITIALIZE',
  };

  // Export
  return {PrintTicketStore: PrintTicketStore};
});
