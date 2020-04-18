// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview', function() {
  'use strict';

  class PreviewGenerator extends cr.EventTarget {
    /**
     * Interface to the Chromium print preview generator.
     * @param {!print_preview.DestinationStore} destinationStore Used to get the
     *     currently selected destination.
     * @param {!print_preview.PrintTicketStore} printTicketStore Used to read
     *     the state of the ticket and write document information.
     * @param {!print_preview.NativeLayer} nativeLayer Used to communicate to
     *     Chromium's preview rendering system.
     * @param {!print_preview.DocumentInfo} documentInfo Document data model.
     * @param {!WebUIListenerTracker} listenerTracker Tracker for the WebUI
     *     listeners added in the PreviewGenerator constructor.
     */
    constructor(
        destinationStore, printTicketStore, nativeLayer, documentInfo,
        listenerTracker) {
      super();

      /**
       * Used to get the currently selected destination.
       * @private {!print_preview.DestinationStore}
       */
      this.destinationStore_ = destinationStore;

      /**
       * Used to read the state of the ticket and write document information.
       * @private {!print_preview.PrintTicketStore}
       */
      this.printTicketStore_ = printTicketStore;

      /**
       * Interface to the Chromium native layer.
       * @private {!print_preview.NativeLayer}
       */
      this.nativeLayer_ = nativeLayer;

      /**
       * Document data model.
       * @private {!print_preview.DocumentInfo}
       */
      this.documentInfo_ = documentInfo;

      /**
       * ID of current in-flight request. Requests that do not share this ID
       * will be ignored.
       * @private {number}
       */
      this.inFlightRequestId_ = -1;

      /**
       * Media size to generate preview with. {@code null} indicates default
       * size.
       * @private {print_preview.ValueType}
       */
      this.mediaSize_ = null;

      /**
       * Whether the previews are being generated in landscape mode.
       * @private {boolean}
       */
      this.isLandscapeEnabled_ = false;

      /**
       * Whether the previews are being generated with a header and footer.
       * @private {boolean}
       */
      this.isHeaderFooterEnabled_ = false;

      /**
       * Whether the previews are being generated in color.
       * @private {boolean}
       */
      this.colorValue_ = false;

      /**
       * Whether the document should be fitted to the page.
       * @private {boolean}
       */
      this.isFitToPageEnabled_ = false;

      /**
       * The scaling factor (in percent) for the document. Ignored if fit to
       * page is true.
       * @private {number}
       */
      this.scalingValue_ = 100;

      /**
       * Whether the PDF should be rasterized for printing.
       * @private {boolean}
       */
      this.rasterize_ = false;

      /**
       * Page ranges setting used used to generate the last preview.
       * @private {Array<{from: number, to: number}>}
       */
      this.pageRanges_ = null;

      /**
       * Margins type used to generate the last preview.
       * @private {!print_preview.ticket_items.MarginsTypeValue}
       */
      this.marginsType_ = print_preview.ticket_items.MarginsTypeValue.DEFAULT;

      /**
       * Whether the document should have element CSS backgrounds printed.
       * @private {boolean}
       */
      this.isCssBackgroundEnabled_ = false;

      /**
       * Whether the document should have only the selected area printed.
       * @private {boolean}
       */
      this.isSelectionOnlyEnabled_ = false;

      /**
       * Destination that was selected for the last preview.
       * @private {print_preview.Destination}
       */
      this.selectedDestination_ = null;

      this.addWebUIEventListeners_(listenerTracker);
    }

    /**
     * Starts listening for relevant WebUI events and adds the listeners to
     * |listenerTracker|. |listenerTracker| is responsible for removing the
     * listeners when necessary.
     * @param {!WebUIListenerTracker} listenerTracker
     * @private
     */
    addWebUIEventListeners_(listenerTracker) {
      listenerTracker.add(
          'page-count-ready', this.onPageCountReady_.bind(this));
      listenerTracker.add(
          'page-layout-ready', this.onPageLayoutReady_.bind(this));
      listenerTracker.add(
          'page-preview-ready', this.onPagePreviewReady_.bind(this));
    }

    /**
     * Request that new preview be generated. A preview request will not be
     * generated if the print ticket has not changed sufficiently.
     * @return {{id: number,
     *           request: Promise}} The preview request id, or -1 if no preview
     *     was requested, and a promise that will resolve when the preview is
     *     complete (null if no preview was actually requested).
     */
    requestPreview() {
      if (!this.printTicketStore_.isTicketValidForPreview() ||
          !this.printTicketStore_.isInitialized ||
          !this.destinationStore_.selectedDestination) {
        return {id: -1, request: null};
      }
      const previewChanged = this.hasPreviewChanged_();
      if (!previewChanged && !this.hasPreviewPageRangeChanged_()) {
        // Changes to these ticket items might not trigger a new preview, but
        // they still need to be recorded.
        this.marginsType_ = this.printTicketStore_.marginsType.getValue();
        return {id: -1, request: null};
      }
      this.mediaSize_ = this.printTicketStore_.mediaSize.getValue();
      this.isLandscapeEnabled_ = this.printTicketStore_.landscape.getValue();
      this.isHeaderFooterEnabled_ =
          this.printTicketStore_.headerFooter.getValue();
      this.colorValue_ = this.printTicketStore_.color.getValue();
      this.isFitToPageEnabled_ = this.printTicketStore_.fitToPage.getValue();
      this.rasterize_ = this.printTicketStore_.rasterize.getValue();
      this.scalingValue_ = this.printTicketStore_.scaling.getValueAsNumber();
      this.pageRanges_ = this.printTicketStore_.pageRange.getPageRanges();
      this.marginsType_ = this.printTicketStore_.marginsType.getValue();
      this.isCssBackgroundEnabled_ =
          this.printTicketStore_.cssBackground.getValue();
      this.isSelectionOnlyEnabled_ =
          this.printTicketStore_.selectionOnly.getValue();
      this.selectedDestination_ = this.destinationStore_.selectedDestination;

      this.inFlightRequestId_++;
      return {
        id: this.inFlightRequestId_,
        request: this.getPreview_(),
      };
    }

    /**
     * @return {!Promise} Promise that resolves when the preview has been
     *     generated.
     * @private
     */
    getPreview_() {
      const printTicketStore = this.printTicketStore_;
      const destination = assert(this.destinationStore_.selectedDestination);

      assert(
          printTicketStore.isTicketValidForPreview(),
          'Trying to generate preview when ticket is not valid');

      const ticket = {
        pageRange: printTicketStore.pageRange.getDocumentPageRanges(),
        mediaSize: printTicketStore.mediaSize.getValue(),
        landscape: printTicketStore.landscape.getValue(),
        color:
            destination.getNativeColorModel(printTicketStore.color.getValue()),
        headerFooterEnabled: printTicketStore.headerFooter.getValue(),
        marginsType: printTicketStore.marginsType.getValue(),
        isFirstRequest: this.inFlightRequestId_ == 0,
        requestID: this.inFlightRequestId_,
        previewModifiable: this.documentInfo_.isModifiable,
        fitToPageEnabled: printTicketStore.fitToPage.getValue(),
        scaleFactor: printTicketStore.scaling.getValueAsNumber(),
        // NOTE: Even though the following fields dont directly relate to the
        // preview, they still need to be included.
        // e.g. printing::PrintSettingsFromJobSettings() still checks for them.
        collate: true,
        copies: 1,
        deviceName: destination.id,
        dpiHorizontal: 'horizontal_dpi' in printTicketStore.dpi.getValue() ?
            printTicketStore.dpi.getValue().horizontal_dpi :
            0,
        dpiVertical: 'vertical_dpi' in printTicketStore.dpi.getValue() ?
            printTicketStore.dpi.getValue().vertical_dpi :
            0,
        duplex: printTicketStore.duplex.getValue() ?
            PreviewGenerator.DuplexMode.LONG_EDGE :
            PreviewGenerator.DuplexMode.SIMPLEX,
        printToPDF: destination.id ==
            print_preview.Destination.GooglePromotedId.SAVE_AS_PDF,
        printWithCloudPrint: !destination.isLocal,
        printWithPrivet: destination.isPrivet,
        printWithExtension: destination.isExtension,
        rasterizePDF: printTicketStore.rasterize.getValue(),
        shouldPrintBackgrounds: printTicketStore.cssBackground.getValue(),
        shouldPrintSelectionOnly: printTicketStore.selectionOnly.getValue()
      };

      // Set 'cloudPrintID' only if the destination is not local.
      if (destination && !destination.isLocal) {
        ticket.cloudPrintID = destination.id;
      }

      if (printTicketStore.marginsType.isCapabilityAvailable() &&
          printTicketStore.marginsType.getValue() ==
              print_preview.ticket_items.MarginsTypeValue.CUSTOM) {
        const customMargins = printTicketStore.customMargins.getValue();
        const orientationEnum =
            print_preview.ticket_items.CustomMarginsOrientation;
        ticket.marginsCustom = {
          marginTop: customMargins.get(orientationEnum.TOP),
          marginRight: customMargins.get(orientationEnum.RIGHT),
          marginBottom: customMargins.get(orientationEnum.BOTTOM),
          marginLeft: customMargins.get(orientationEnum.LEFT)
        };
      }

      return this.nativeLayer_.getPreview(JSON.stringify(ticket));
    }

    /**
     * Dispatches a PAGE_READY event to signal that a page preview is ready.
     * @param {number} previewIndex Index of the page with respect to the pages
     *     shown in the preview. E.g an index of 0 is the first displayed page,
     *     but not necessarily the first original document page.
     * @param {number} pageNumber Number of the page with respect to the
     *     document. A value of 3 means it's the third page of the original
     *     document.
     * @param {number} previewUid Unique identifier of the preview.
     * @private
     */
    dispatchPageReadyEvent_(previewIndex, pageNumber, previewUid) {
      const pageGenEvent = new Event(PreviewGenerator.EventType.PAGE_READY);
      pageGenEvent.previewIndex = previewIndex;
      pageGenEvent.previewUrl = 'chrome://print/' + previewUid.toString() +
          '/' + (pageNumber - 1) + '/print.pdf';
      this.dispatchEvent(pageGenEvent);
    }

    /**
     * Dispatches a PREVIEW_START event. Signals that the preview should be
     * reloaded.
     * @param {number} previewUid Unique identifier of the preview.
     * @param {number} index Index of the first page of the preview.
     * @private
     */
    dispatchPreviewStartEvent_(previewUid, index) {
      const previewStartEvent =
          new Event(PreviewGenerator.EventType.PREVIEW_START);
      if (!this.documentInfo_.isModifiable) {
        index = -1;
      }
      previewStartEvent.previewUrl = 'chrome://print/' + previewUid.toString() +
          '/' + index + '/print.pdf';
      this.dispatchEvent(previewStartEvent);
    }

    /**
     * @return {boolean} Whether the print ticket, excluding the page range, has
     *     changed sufficiently to determine whether a new preview request
     *     should be issued.
     * @private
     */
    hasPreviewChanged_() {
      const ticketStore = this.printTicketStore_;
      return this.inFlightRequestId_ == -1 ||
          !ticketStore.mediaSize.isValueEqual(this.mediaSize_) ||
          !ticketStore.landscape.isValueEqual(this.isLandscapeEnabled_) ||
          !ticketStore.headerFooter.isValueEqual(this.isHeaderFooterEnabled_) ||
          !ticketStore.color.isValueEqual(this.colorValue_) ||
          !ticketStore.scaling.isValueEqual(this.scalingValue_) ||
          !ticketStore.fitToPage.isValueEqual(this.isFitToPageEnabled_) ||
          !ticketStore.rasterize.isValueEqual(this.rasterize_) ||
          (!ticketStore.marginsType.isValueEqual(this.marginsType_) &&
           !ticketStore.marginsType.isValueEqual(
               print_preview.ticket_items.MarginsTypeValue.CUSTOM)) ||
          (ticketStore.marginsType.isValueEqual(
               print_preview.ticket_items.MarginsTypeValue.CUSTOM) &&
           !ticketStore.customMargins.isValueEqual(
               this.documentInfo_.margins)) ||
          !ticketStore.cssBackground.isValueEqual(
              this.isCssBackgroundEnabled_) ||
          !ticketStore.selectionOnly.isValueEqual(
              this.isSelectionOnlyEnabled_) ||
          (this.selectedDestination_ !=
           this.destinationStore_.selectedDestination);
    }

    /**
     * @return {boolean} Whether the page range in the print ticket has changed.
     * @private
     */
    hasPreviewPageRangeChanged_() {
      return this.pageRanges_ == null ||
          !areRangesEqual(
                 this.printTicketStore_.pageRange.getPageRanges(),
                 this.pageRanges_);
    }

    /**
     * Called when the page layout of the document is ready. Always occurs
     * as a result of a preview request.
     * @param {{marginTop: number,
     *          marginLeft: number,
     *          marginBottom: number,
     *          marginRight: number,
     *          contentWidth: number,
     *          contentHeight: number,
     *          printableAreaX: number,
     *          printableAreaY: number,
     *          printableAreaWidth: number,
     *          printableAreaHeight: number,
     *        }} pageLayout Layout information about the document.
     * @param {boolean} hasCustomPageSizeStyle Whether this document has a
     *     custom page size or style to use.
     * @private
     */
    onPageLayoutReady_(pageLayout, hasCustomPageSizeStyle) {
      // NOTE: A request ID is not specified, so assuming its for the current
      // in-flight request.

      const origin = new print_preview.Coordinate2d(
          pageLayout.printableAreaX, pageLayout.printableAreaY);
      const size = new print_preview.Size(
          pageLayout.printableAreaWidth, pageLayout.printableAreaHeight);

      const margins = new print_preview.Margins(
          Math.round(pageLayout.marginTop), Math.round(pageLayout.marginRight),
          Math.round(pageLayout.marginBottom),
          Math.round(pageLayout.marginLeft));

      const o = print_preview.ticket_items.CustomMarginsOrientation;
      const pageSize = new print_preview.Size(
          pageLayout.contentWidth + margins.get(o.LEFT) + margins.get(o.RIGHT),
          pageLayout.contentHeight + margins.get(o.TOP) +
              margins.get(o.BOTTOM));

      this.documentInfo_.updatePageInfo(
          new print_preview.PrintableArea(origin, size), pageSize,
          hasCustomPageSizeStyle, margins);
    }

    /**
     * Called when the document page count is received from the native layer.
     * Always occurs as a result of a preview request.
     * @param {number} pageCount The document's page count.
     * @param {number} previewResponseId The request ID that corresponds to this
     *     page count.
     * @param {number} fitToPageScaling The scaling required to fit the document
     *     to page (unused).
     * @private
     */
    onPageCountReady_(pageCount, previewResponseId, fitToPageScaling) {
      if (this.inFlightRequestId_ != previewResponseId) {
        return;  // Ignore old response.
      }
      this.documentInfo_.updatePageCount(pageCount);
      this.pageRanges_ = this.printTicketStore_.pageRange.getPageRanges();
    }

    /**
     * Called when a page's preview has been generated. Dispatches a
     * PAGE_READY event.
     * @param {number} pageIndex The index of the page whose preview is ready.
     * @param {number} previewUid The unique ID of the print preview UI.
     * @param {number} previewResponseId The preview request ID that this page
     *     preview is a response to.
     * @private
     */
    onPagePreviewReady_(pageIndex, previewUid, previewResponseId) {
      if (this.inFlightRequestId_ != previewResponseId) {
        return;  // Ignore old response.
      }
      const pageNumber = pageIndex + 1;
      const pageNumberSet = this.printTicketStore_.pageRange.getPageNumberSet();
      if (pageNumberSet.hasPageNumber(pageNumber)) {
        const previewIndex = pageNumberSet.getPageNumberIndex(pageNumber);
        if (previewIndex == 0) {
          this.dispatchPreviewStartEvent_(previewUid, pageIndex);
        }
        this.dispatchPageReadyEvent_(previewIndex, pageNumber, previewUid);
      }
    }

    /**
     * Called when the preview generation is complete. Dispatches a
     * DOCUMENT_READY event.
     * @param {number} previewResponseId
     * @param {number} previewUid
     */
    onPreviewGenerationDone(previewResponseId, previewUid) {
      if (this.inFlightRequestId_ != previewResponseId) {
        return;  // Ignore old response.
      }
      if (!this.documentInfo_.isModifiable) {
        // Dispatch a PREVIEW_START event since not generating a PDF, which
        // includes print preview for non-modifiable documents, does not
        // trigger PAGE_READY events.
        this.dispatchPreviewStartEvent_(previewUid, 0);
      }
      cr.dispatchSimpleEvent(this, PreviewGenerator.EventType.DOCUMENT_READY);
    }

    /**
     * Called when the preview generation fails.
     * @private
     */
    onPreviewGenerationFail_() {
      // NOTE: No request ID is returned from Chromium so its assumed its the
      // current one.
      cr.dispatchSimpleEvent(this, PreviewGenerator.EventType.FAIL);
    }
  }

  /**
   * Event types dispatched by the preview generator.
   * @enum {string}
   */
  PreviewGenerator.EventType = {
    // Dispatched when the document can be printed.
    DOCUMENT_READY: 'print_preview.PreviewGenerator.DOCUMENT_READY',

    // Dispatched when a page preview is ready. The previewIndex field of the
    // event is the index of the page in the modified document, not the
    // original. So page 4 of the original document might be previewIndex = 0 of
    // the modified document.
    PAGE_READY: 'print_preview.PreviewGenerator.PAGE_READY',

    // Dispatched when the document preview starts to be generated.
    PREVIEW_START: 'print_preview.PreviewGenerator.PREVIEW_START',

    // Dispatched when the current print preview request fails.
    FAIL: 'print_preview.PreviewGenerator.FAIL'
  };

  /**
   * Constant values matching printing::DuplexMode enum.
   * @enum {number}
   */
  PreviewGenerator
      .DuplexMode = {SIMPLEX: 0, LONG_EDGE: 1, UNKNOWN_DUPLEX_MODE: -1};

  // Export
  return {PreviewGenerator: PreviewGenerator};
});
