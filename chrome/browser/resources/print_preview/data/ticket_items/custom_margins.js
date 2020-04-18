// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview.ticket_items', function() {
  'use strict';

  const CustomMarginsOrientation =
      print_preview.ticket_items.CustomMarginsOrientation;

  class CustomMargins extends print_preview.ticket_items.TicketItem {
    /**
     * Custom page margins ticket item whose value is a
     * {@code print_preview.Margins}.
     * @param {!print_preview.AppState} appState App state used to persist
     *     custom margins.
     * @param {!print_preview.DocumentInfo} documentInfo Information about the
     *     document to print.
     */
    constructor(appState, documentInfo) {
      super(
          appState, print_preview.AppStateField.CUSTOM_MARGINS,
          null /*destinationStore*/, documentInfo);
    }

    /** @override */
    wouldValueBeValid(value) {
      const margins = /** @type {!print_preview.Margins} */ (value);
      for (const key in CustomMarginsOrientation) {
        const o = CustomMarginsOrientation[key];
        const max = this.getMarginMax_(
            o, margins.get(CustomMargins.OppositeOrientation_[o]));
        if (margins.get(o) > max || margins.get(o) < 0) {
          return false;
        }
      }
      return true;
    }

    /** @override */
    isCapabilityAvailable() {
      return this.getDocumentInfoInternal().isModifiable;
    }

    /** @override */
    isValueEqual(value) {
      return this.getValue().equals(value);
    }

    /**
     * @param {!print_preview.ticket_items.CustomMarginsOrientation}
     *     orientation Specifies the margin to get the maximum value for.
     * @return {number} Maximum value in points of the specified margin.
     */
    getMarginMax(orientation) {
      const oppositeOrient = CustomMargins.OppositeOrientation_[orientation];
      const margins = /** @type {!print_preview.Margins} */ (this.getValue());
      return this.getMarginMax_(orientation, margins.get(oppositeOrient));
    }

    /** @override */
    updateValue(value) {
      let margins = /** @type {!print_preview.Margins} */ (value);
      if (margins != null) {
        margins = new print_preview.Margins(
            Math.round(margins.get(CustomMarginsOrientation.TOP)),
            Math.round(margins.get(CustomMarginsOrientation.RIGHT)),
            Math.round(margins.get(CustomMarginsOrientation.BOTTOM)),
            Math.round(margins.get(CustomMarginsOrientation.LEFT)));
      }
      print_preview.ticket_items.TicketItem.prototype.updateValue.call(
          this, margins);
    }

    /**
     * Updates the specified margin in points while keeping the value within
     * a maximum and minimum.
     * @param {!print_preview.ticket_items.CustomMarginsOrientation}
     *     orientation Specifies the margin to update.
     * @param {number} value Updated margin value in points.
     */
    updateMargin(orientation, value) {
      const margins = /** @type {!print_preview.Margins} */ (this.getValue());
      const oppositeOrientation =
          CustomMargins.OppositeOrientation_[orientation];
      const max =
          this.getMarginMax_(orientation, margins.get(oppositeOrientation));
      value = Math.max(0, Math.min(max, value));
      this.updateValue(margins.set(orientation, value));
    }

    /** @override */
    getDefaultValueInternal() {
      return this.getDocumentInfoInternal().margins ||
          new print_preview.Margins(72, 72, 72, 72);
    }

    /** @override */
    getCapabilityNotAvailableValueInternal() {
      return this.getDocumentInfoInternal().margins ||
          new print_preview.Margins(72, 72, 72, 72);
    }

    /**
     * @param {!print_preview.ticket_items.CustomMarginsOrientation}
     *     orientation Specifies which margin to get the maximum value of.
     * @param {number} oppositeMargin Value of the margin in points
     *     opposite the specified margin.
     * @return {number} Maximum value in points of the specified margin.
     * @private
     */
    getMarginMax_(orientation, oppositeMargin) {
      const dimensionLength = (orientation == CustomMarginsOrientation.TOP ||
                               orientation == CustomMarginsOrientation.BOTTOM) ?
          this.getDocumentInfoInternal().pageSize.height :
          this.getDocumentInfoInternal().pageSize.width;

      const totalMargin =
          dimensionLength - CustomMargins.MINIMUM_MARGINS_DISTANCE_;
      return Math.round(totalMargin > 0 ? totalMargin - oppositeMargin : 0);
    }
  }

  /**
   * Mapping of a margin orientation to its opposite.
   * @type {!Object<!print_preview.ticket_items.CustomMarginsOrientation,
   *                 !print_preview.ticket_items.CustomMarginsOrientation>}
   * @private
   */
  CustomMargins.OppositeOrientation_ = {};
  CustomMargins.OppositeOrientation_[CustomMarginsOrientation.TOP] =
      CustomMarginsOrientation.BOTTOM;
  CustomMargins.OppositeOrientation_[CustomMarginsOrientation.RIGHT] =
      CustomMarginsOrientation.LEFT;
  CustomMargins.OppositeOrientation_[CustomMarginsOrientation.BOTTOM] =
      CustomMarginsOrientation.TOP;
  CustomMargins.OppositeOrientation_[CustomMarginsOrientation.LEFT] =
      CustomMarginsOrientation.RIGHT;

  /**
   * Minimum distance in points that two margins can be separated by.
   * @type {number}
   * @const
   * @private
   */
  CustomMargins.MINIMUM_MARGINS_DISTANCE_ = 72;  // 1 inch.


  // Export
  return {CustomMargins: CustomMargins};
});
