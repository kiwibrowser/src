// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview.ticket_items', function() {
  'use strict';

  class Scaling extends print_preview.ticket_items.TicketItem {
    /**
     * Scaling ticket item whose value is a {@code string} that indicates what
     * the scaling (in percent) of the document should be. The ticket item is
     * backed by a string since the user can textually input the scaling value.
     * @param {!print_preview.AppState} appState App state to persist item
     *     value.
     * @param {!print_preview.DocumentInfo} documentInfo Information about the
     *     document to print.
     * @param {!print_preview.DestinationStore} destinationStore Used to
     *     determine whether scaling should be available.
     */
    constructor(appState, destinationStore, documentInfo) {
      super(
          appState, print_preview.AppStateField.SCALING, destinationStore,
          documentInfo);
    }

    /** @override */
    wouldValueBeValid(value) {
      return value != '';
    }

    /** @override */
    isValueEqual(value) {
      return this.getValue() == value;
    }

    /** @override */
    isCapabilityAvailable() {
      // This is not a function of the printer, but should be disabled if we are
      // saving a PDF to a PDF.
      const knownSizeToSaveAsPdf =
          (!this.getDocumentInfoInternal().isModifiable ||
           this.getDocumentInfoInternal().hasCssMediaStyles) &&
          this.getSelectedDestInternal() &&
          this.getSelectedDestInternal().id ==
              print_preview.Destination.GooglePromotedId.SAVE_AS_PDF;
      return !knownSizeToSaveAsPdf;
    }

    /** @return {number} The scaling percentage indicated by the ticket item. */
    getValueAsNumber() {
      const value = this.getValue() == '' ? 0 : parseInt(this.getValue(), 10);
      assert(!isNaN(value));
      return value;
    }

    /** @override */
    getDefaultValueInternal() {
      return '100';
    }

    /** @override */
    getCapabilityNotAvailableValueInternal() {
      return '100';
    }
  }

  // Export
  return {Scaling: Scaling};
});
