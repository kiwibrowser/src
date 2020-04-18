// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview', function() {
  'use strict';

  class PageNumberSet {
    /**
     * An immutable ordered set of page numbers.
     * @param {!Array<number>} pageNumberList A list of page numbers to include
     *     in the set.
     */
    constructor(pageNumberList) {
      /**
       * Internal data store for the page number set.
       * @type {!Array<number>}
       * @private
       */
      this.pageNumberSet_ = pageListToPageSet(pageNumberList);
    }

    /** @return {number} The number of page numbers in the set. */
    get size() {
      return this.pageNumberSet_.length;
    }

    /**
     * @param {number} index 0-based index of the page number to get.
     * @return {number} Page number at the given index.
     */
    getPageNumberAt(index) {
      return this.pageNumberSet_[index];
    }

    /**
     * @param {number} pageNumber 1-based page number to check for.
     * @return {boolean} Whether the given page number is in the page range.
     */
    hasPageNumber(pageNumber) {
      return arrayContains(this.pageNumberSet_, pageNumber);
    }

    /**
     * @param {number} pageNumber 1-based number of the page to get index of.
     * @return {number} 0-based index of the given page number with respect to
     *     all of the pages in the page range.
     */
    getPageNumberIndex(pageNumber) {
      return this.pageNumberSet_.indexOf(pageNumber);
    }

    /** @return {!Array<number>} Array representation of the set. */
    asArray() {
      return this.pageNumberSet_.slice(0);
    }
  }

  // Export
  return {PageNumberSet: PageNumberSet};
});
