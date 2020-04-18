// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview.ticket_items', function() {
  'use strict';

  class PageRange extends print_preview.ticket_items.TicketItem {
    /**
     * Page range ticket item whose value is a {@code string} that represents
     * which pages in the document should be printed.
     * @param {!print_preview.DocumentInfo} documentInfo Information about the
     *     document to print.
     */
    constructor(documentInfo) {
      super(
          null /*appState*/, null /*field*/, null /*destinationStore*/,
          documentInfo);
    }

    /** @override */
    wouldValueBeValid(value) {
      const result = pageRangeTextToPageRanges(
          value, this.getDocumentInfoInternal().pageCount);
      return Array.isArray(result);
    }

    /**
     * @return {!print_preview.PageNumberSet} Set of page numbers defined by the
     *     page range string.
     */
    getPageNumberSet() {
      const pageNumberList = pageRangeTextToPageList(
          this.getValueAsString_(), this.getDocumentInfoInternal().pageCount);
      return new print_preview.PageNumberSet(pageNumberList);
    }

    /** @override */
    isCapabilityAvailable() {
      return true;
    }

    /** @override */
    getDefaultValueInternal() {
      return '';
    }

    /** @override */
    getCapabilityNotAvailableValueInternal() {
      return '';
    }

    /**
     * @return {string} The value of the ticket item as a string.
     * @private
     */
    getValueAsString_() {
      return /** @type {string} */ (this.getValue());
    }

    /**
     * @return {!Array<Object<{from: number, to: number}>>} A list of page
     *     ranges.
     */
    getPageRanges() {
      const pageRanges = pageRangeTextToPageRanges(this.getValueAsString_());
      return Array.isArray(pageRanges) ? pageRanges : [];
    }

    /**
     * @return {!Array<Object<{from: number, to: number}>>} A list of page
     *     ranges suitable for use in the native layer.
     * TODO(vitalybuka): this should be removed when native layer switched to
     *     page ranges.
     */
    getDocumentPageRanges() {
      const pageRanges = pageRangeTextToPageRanges(
          this.getValueAsString_(), this.getDocumentInfoInternal().pageCount);
      return Array.isArray(pageRanges) ? pageRanges : [];
    }

    /**
     * @return {!number} Number of pages reported by the document.
     */
    getDocumentNumPages() {
      return this.getDocumentInfoInternal().pageCount;
    }

    /**
     * @return {!PageRangeStatus}
     */
    checkValidity() {
      const pageRanges = pageRangeTextToPageRanges(
          this.getValueAsString_(), this.getDocumentInfoInternal().pageCount);
      return Array.isArray(pageRanges) ? PageRangeStatus.NO_ERROR : pageRanges;
    }
  }

  /**
   * Impossibly large page number.
   * @type {number}
   * @const
   * @private
   */
  PageRange.MAX_PAGE_NUMBER_ = 1000000000;


  // Export
  return {PageRange: PageRange};
});
