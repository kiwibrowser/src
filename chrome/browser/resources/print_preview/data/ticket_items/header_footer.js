// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview.ticket_items', function() {
  'use strict';

  class HeaderFooter extends print_preview.ticket_items.TicketItem {
    /**
     * Header-footer ticket item whose value is a {@code boolean} that indicates
     * whether the document should be printed with headers and footers.
     * @param {!print_preview.AppState} appState App state used to persist
     *     whether header-footer is enabled.
     * @param {!print_preview.DocumentInfo} documentInfo Information about the
     *     document to print.
     * @param {!print_preview.ticket_items.MarginsType} marginsType Ticket item
     *     that stores which predefined margins to print with.
     * @param {!print_preview.ticket_items.CustomMargins} customMargins Ticket
     *     item that stores custom margin values.
     * @param {!print_preview.ticket_items.MediaSize} mediaSize Ticket item that
     *     stores media size values.
     * @param {!print_preview.ticket_items.Landscape} landscape Ticket item that
     *     stores landscape values.
     */
    constructor(
        appState, documentInfo, marginsType, customMargins, mediaSize,
        landscape) {
      super(
          appState, print_preview.AppStateField.IS_HEADER_FOOTER_ENABLED,
          null /*destinationStore*/, documentInfo);

      /**
       * Ticket item that stores which predefined margins to print with.
       * @private {!print_preview.ticket_items.MarginsType}
       */
      this.marginsType_ = marginsType;

      /**
       * Ticket item that stores custom margin values.
       * @private {!print_preview.ticket_items.CustomMargins}
       */
      this.customMargins_ = customMargins;

      /**
       * Ticket item that stores media size values.
       * @private {!print_preview.ticket_items.MediaSize}
       */
      this.mediaSize_ = mediaSize;

      /**
       * Ticket item that stores landscape values.
       * @private {!print_preview.ticket_items.Landscape}
       */
      this.landscape_ = landscape;

      this.addEventListeners_();
    }

    /** @override */
    wouldValueBeValid(value) {
      return true;
    }

    /** @override */
    isCapabilityAvailable() {
      if (!this.getDocumentInfoInternal().isModifiable) {
        return false;
      }
      if (this.marginsType_.getValue() ==
          print_preview.ticket_items.MarginsTypeValue.NO_MARGINS) {
        return false;
      }
      const microns = this.landscape_.getValue() ?
          this.mediaSize_.getValue().width_microns :
          this.mediaSize_.getValue().height_microns;
      if (microns < HeaderFooter.MINIMUM_HEIGHT_MICRONS_) {
        // If this is a small paper size, there is not space for headers
        // and footers regardless of the margins.
        return false;
      }
      if (this.marginsType_.getValue() ==
          print_preview.ticket_items.MarginsTypeValue.MINIMUM) {
        return true;
      }
      let margins;
      if (this.marginsType_.getValue() ==
          print_preview.ticket_items.MarginsTypeValue.CUSTOM) {
        if (!this.customMargins_.isValid()) {
          return false;
        }
        margins = this.customMargins_.getValue();
      } else {
        margins = this.getDocumentInfoInternal().margins;
      }
      const orientEnum = print_preview.ticket_items.CustomMarginsOrientation;
      return margins == null || margins.get(orientEnum.TOP) > 0 ||
          margins.get(orientEnum.BOTTOM) > 0;
    }

    /** @override */
    getDefaultValueInternal() {
      return true;
    }

    /** @override */
    getCapabilityNotAvailableValueInternal() {
      return false;
    }

    /**
     * Adds CHANGE listeners to dependent ticket items.
     * @private
     */
    addEventListeners_() {
      this.getTrackerInternal().add(
          this.marginsType_,
          print_preview.ticket_items.TicketItem.EventType.CHANGE,
          this.dispatchChangeEventInternal.bind(this));
      this.getTrackerInternal().add(
          this.customMargins_,
          print_preview.ticket_items.TicketItem.EventType.CHANGE,
          this.dispatchChangeEventInternal.bind(this));
      this.getTrackerInternal().add(
          this.mediaSize_,
          print_preview.ticket_items.TicketItem.EventType.CHANGE,
          this.dispatchChangeEventInternal.bind(this));
      this.getTrackerInternal().add(
          this.landscape_,
          print_preview.ticket_items.TicketItem.EventType.CHANGE,
          this.dispatchChangeEventInternal.bind(this));
    }
  }

  /**
   * Minimum height of page in microns to allow headers and footers. Should
   * match the value for min_size_printer_units in printing/print_settings.cc
   * so that we do not request header/footer for margins that will be zero.
   * @private {number}
   * @const
   */
  HeaderFooter.MINIMUM_HEIGHT_MICRONS_ = 25400;


  // Export
  return {HeaderFooter: HeaderFooter};
});
