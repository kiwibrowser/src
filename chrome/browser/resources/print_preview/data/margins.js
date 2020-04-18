// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.exportPath('print_preview.ticket_items');
/**
 * Enumeration of the orientations of margins.
 * @enum {string}
 */
print_preview.ticket_items.CustomMarginsOrientation = {
  TOP: 'top',
  RIGHT: 'right',
  BOTTOM: 'bottom',
  LEFT: 'left'
};

/**
 * Must be kept in sync with the C++ MarginType enum in
 * printing/print_job_constants.h.
 * @enum {number}
 */
print_preview.ticket_items.MarginsTypeValue = {
  DEFAULT: 0,
  NO_MARGINS: 1,
  MINIMUM: 2,
  CUSTOM: 3
};

/**
 * Keep in sync with the C++ kSettingMargin... values in
 * printing/print_job_constants.h.
 * @typedef {{
 *   marginTop: number,
 *   marginRight: number,
 *   marginBottom: number,
 *   marginLeft: number,
 * }}
 */
print_preview.MarginsSetting;

cr.define('print_preview', function() {
  'use strict';

  class Margins {
    /**
     * Creates a Margins object that holds four margin values in points.
     * @param {number} top The top margin in pts.
     * @param {number} right The right margin in pts.
     * @param {number} bottom The bottom margin in pts.
     * @param {number} left The left margin in pts.
     */
    constructor(top, right, bottom, left) {
      /**
       * Backing store for the margin values in points.
       * @type {!Object<
       *     !print_preview.ticket_items.CustomMarginsOrientation, number>}
       * @private
       */
      this.value_ = {};
      this.value_[print_preview.ticket_items.CustomMarginsOrientation.TOP] =
          top;
      this.value_[print_preview.ticket_items.CustomMarginsOrientation.RIGHT] =
          right;
      this.value_[print_preview.ticket_items.CustomMarginsOrientation.BOTTOM] =
          bottom;
      this.value_[print_preview.ticket_items.CustomMarginsOrientation.LEFT] =
          left;
    }

    /**
     * Parses a margins object from the given serialized state.
     * @param {Object} state Serialized representation of the margins created by
     *     the {@code serialize} method.
     * @return {!print_preview.Margins} New margins instance.
     */
    static parse(state) {
      return new print_preview.Margins(
          state[print_preview.ticket_items.CustomMarginsOrientation.TOP] || 0,
          state[print_preview.ticket_items.CustomMarginsOrientation.RIGHT] || 0,
          state[print_preview.ticket_items.CustomMarginsOrientation.BOTTOM] ||
              0,
          state[print_preview.ticket_items.CustomMarginsOrientation.LEFT] || 0);
    }

    /**
     * @param {!print_preview.ticket_items.CustomMarginsOrientation}
     *     orientation Specifies the margin value to get.
     * @return {number} Value of the margin of the given orientation.
     */
    get(orientation) {
      return this.value_[orientation];
    }

    /**
     * @param {!print_preview.ticket_items.CustomMarginsOrientation}
     *     orientation Specifies the margin to set.
     * @param {number} value Updated value of the margin in points to modify.
     * @return {!print_preview.Margins} A new copy of |this| with the
     *     modification made to the specified margin.
     */
    set(orientation, value) {
      const newValue = this.clone_();
      newValue[orientation] = value;
      return new Margins(
          newValue[print_preview.ticket_items.CustomMarginsOrientation.TOP],
          newValue[print_preview.ticket_items.CustomMarginsOrientation.RIGHT],
          newValue[print_preview.ticket_items.CustomMarginsOrientation.BOTTOM],
          newValue[print_preview.ticket_items.CustomMarginsOrientation.LEFT]);
    }

    /**
     * @param {print_preview.Margins} other The other margins object to compare
     *     against.
     * @return {boolean} Whether this margins object is equal to another.
     */
    equals(other) {
      if (other == null) {
        return false;
      }
      for (const orientation in this.value_) {
        if (this.value_[orientation] != other.value_[orientation]) {
          return false;
        }
      }
      return true;
    }

    /** @return {Object} A serialized representation of the margins. */
    serialize() {
      return this.clone_();
    }

    /**
     * @return {Object} Cloned state of the margins.
     * @private
     */
    clone_() {
      const clone = {};
      for (const o in this.value_) {
        clone[o] = this.value_[o];
      }
      return clone;
    }
  }

  // Export
  return {Margins: Margins};
});
