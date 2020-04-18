// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview', function() {
  'use strict';

  class DocumentInfo extends cr.EventTarget {
    /**
     * Data model which contains information related to the document to print.
     */
    constructor() {
      super();

      /**
       * Whether the document is styled by CSS media styles.
       * @private {boolean}
       */
      this.hasCssMediaStyles_ = false;

      /**
       * Whether the document has selected content.
       * @private {boolean}
       */
      this.hasSelection_ = false;

      /**
       * Whether the document to print is modifiable (i.e. can be reflowed).
       * @private {boolean}
       */
      this.isModifiable_ = true;

      /**
       * Whether scaling of the document is prohibited.
       * @private {boolean}
       */
      this.isScalingDisabled_ = false;

      /**
       * Scaling required to fit to page.
       * @private {number}
       */
      this.fitToPageScaling_ = 100;

      /**
       * Margins of the document in points.
       * @private {print_preview.Margins}
       */
      this.margins_ = null;

      /**
       * Number of pages in the document to print.
       * @private {number}
       */
      this.pageCount_ = 0;

      /**
       * Size of the pages of the document in points. Actual page-related
       * information won't be set until preview generation occurs, so use
       * a default value until then. This way, the print ticket store will be
       * valid even if no preview can be generated.
       * @private {!print_preview.Size}
       */
      this.pageSize_ = new print_preview.Size(612, 792);  // 8.5"x11"

      /**
       * Printable area of the document in points.
       * @private {!print_preview.PrintableArea}
       */
      this.printableArea_ = new print_preview.PrintableArea(
          new print_preview.Coordinate2d(0, 0), this.pageSize_);

      /**
       * Title of document.
       * @private {string}
       */
      this.title_ = '';

      /**
       * Whether this data model has been initialized.
       * @private {boolean}
       */
      this.isInitialized_ = false;
    }

    /** @return {boolean} Whether the document is styled by CSS media styles. */
    get hasCssMediaStyles() {
      return this.hasCssMediaStyles_;
    }

    /** @return {boolean} Whether the document has selected content. */
    get hasSelection() {
      return this.hasSelection_;
    }

    /**
     * @return {boolean} Whether the document to print is modifiable (i.e. can
     *     be reflowed).
     */
    get isModifiable() {
      return this.isModifiable_;
    }

    /** @return {boolean} Whether scaling of the document is prohibited. */
    get isScalingDisabled() {
      return this.isScalingDisabled_;
    }

    /** @return {number} Scaling required to fit to page. */
    get fitToPageScaling() {
      return this.fitToPageScaling_;
    }

    /** @return {print_preview.Margins} Margins of the document in points. */
    get margins() {
      return this.margins_;
    }

    /** @return {number} Number of pages in the document to print. */
    get pageCount() {
      return this.pageCount_;
    }

    /**
     * @return {!print_preview.Size} Size of the pages of the document in
     *     points.
     */
    get pageSize() {
      return this.pageSize_;
    }

    /**
     * @return {!print_preview.PrintableArea} Printable area of the document in
     *     points.
     */
    get printableArea() {
      return this.printableArea_;
    }

    /** @return {string} Title of document. */
    get title() {
      return this.title_;
    }

    /**
     * Initializes the state of the data model and dispatches a CHANGE event.
     * @param {boolean} isModifiable Whether the document is modifiable.
     * @param {string} title Title of the document.
     * @param {boolean} hasSelection Whether the document has user-selected
     *     content.
     */
    init(isModifiable, title, hasSelection) {
      this.isModifiable_ = isModifiable;
      this.title_ = title;
      this.hasSelection_ = hasSelection;
      this.isInitialized_ = true;
      cr.dispatchSimpleEvent(this, DocumentInfo.EventType.CHANGE);
    }

    /**
     * Updates whether scaling is disabled for the document and dispatches a
     * CHANGE event.
     * @param {boolean} isScalingDisabled Whether scaling of the document is
     *     prohibited.
     */
    updateIsScalingDisabled(isScalingDisabled) {
      if (this.isInitialized_ && this.isScalingDisabled_ != isScalingDisabled) {
        this.isScalingDisabled_ = isScalingDisabled;
        cr.dispatchSimpleEvent(this, DocumentInfo.EventType.CHANGE);
      }
    }

    /**
     * Updates the total number of pages in the document and dispatches a CHANGE
     * event.
     * @param {number} pageCount Number of pages in the document.
     */
    updatePageCount(pageCount) {
      if (this.isInitialized_ && this.pageCount_ != pageCount) {
        this.pageCount_ = pageCount;
        cr.dispatchSimpleEvent(this, DocumentInfo.EventType.CHANGE);
      }
    }

    /**
     * Updates the fit to page scaling. Does not dispatch a CHANGE event, since
     * this is only called in tests and in the new print preview UI, which uses
     * data bindings to notify UI elements of the change.
     * @param {number} scaleFactor The fit to page scale factor.
     */
    updateFitToPageScaling(scaleFactor) {
      this.fitToPageScaling_ = scaleFactor;
    }

    /**
     * Updates information about each page and dispatches a CHANGE event.
     * @param {!print_preview.PrintableArea} printableArea Printable area of the
     *     document in points.
     * @param {!print_preview.Size} pageSize Size of the pages of the document
     *     in points.
     * @param {boolean} hasCssMediaStyles Whether the document is styled by CSS
     *     media styles.
     * @param {print_preview.Margins} margins Margins of the document in points.
     */
    updatePageInfo(printableArea, pageSize, hasCssMediaStyles, margins) {
      if (this.isInitialized_ &&
          (!this.printableArea_.equals(printableArea) ||
           !this.pageSize_.equals(pageSize) ||
           this.hasCssMediaStyles_ != hasCssMediaStyles ||
           this.margins_ == null || !this.margins_.equals(margins))) {
        this.printableArea_ = printableArea;
        this.pageSize_ = pageSize;
        this.hasCssMediaStyles_ = hasCssMediaStyles;
        this.margins_ = margins;
        cr.dispatchSimpleEvent(this, DocumentInfo.EventType.CHANGE);
      }
    }
  }

  /**
   * Event types dispatched by this data model.
   * @enum {string}
   */
  DocumentInfo.EventType = {CHANGE: 'print_preview.DocumentInfo.CHANGE'};

  // Export
  return {DocumentInfo: DocumentInfo};
});
