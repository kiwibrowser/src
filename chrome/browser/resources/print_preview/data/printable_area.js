// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview', function() {
  'use strict';

  class PrintableArea {
    /**
     * Object describing the printable area of a page in the document.
     * @param {!print_preview.Coordinate2d} origin Top left corner of the
     *     printable area of the document.
     * @param {!print_preview.Size} size Size of the printable area of the
     *     document.
     */
    constructor(origin, size) {
      /**
       * Top left corner of the printable area of the document.
       * @type {!print_preview.Coordinate2d}
       * @private
       */
      this.origin_ = origin;

      /**
       * Size of the printable area of the document.
       * @type {!print_preview.Size}
       * @private
       */
      this.size_ = size;
    }

    /**
     * @return {!print_preview.Coordinate2d} Top left corner of the printable
     *     area of the document.
     */
    get origin() {
      return this.origin_;
    }

    /**
     * @return {!print_preview.Size} Size of the printable area of the document.
     */
    get size() {
      return this.size_;
    }

    /**
     * @param {print_preview.PrintableArea} other Other printable area to check
     *     for equality.
     * @return {boolean} Whether another printable area is equal to this one.
     */
    equals(other) {
      return other != null && this.origin_.equals(other.origin_) &&
          this.size_.equals(other.size_);
    }
  }

  // Export
  return {PrintableArea: PrintableArea};
});
