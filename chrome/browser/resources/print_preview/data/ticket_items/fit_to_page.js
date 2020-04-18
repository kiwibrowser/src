// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview.ticket_items', function() {
  'use strict';

  class FitToPage extends print_preview.ticket_items.TicketItem {
    /**
     * Fit-to-page ticket item whose value is a {@code boolean} that indicates
     * whether to scale the document to fit the page.
     * @param {!print_preview.AppState} appState App state to persist item
     *     value.
     * @param {!print_preview.DocumentInfo} documentInfo Information about the
     *     document to print.
     * @param {!print_preview.DestinationStore} destinationStore Used to
     *     determine whether fit to page should be available.
     */
    constructor(appState, documentInfo, destinationStore) {
      super(
          appState, print_preview.AppStateField.IS_FIT_TO_PAGE_ENABLED,
          destinationStore, documentInfo);
    }

    /** @override */
    wouldValueBeValid(value) {
      return true;
    }

    /** @override */
    isCapabilityAvailable() {
      return !this.getDocumentInfoInternal().isModifiable &&
          (!this.getSelectedDestInternal() ||
           this.getSelectedDestInternal().id !=
               print_preview.Destination.GooglePromotedId.SAVE_AS_PDF);
    }

    /** @override */
    getDefaultValueInternal() {
      // It's on by default since it is not a document feature, it is rather
      // a property of the printer, hardware margins limitations. User can
      // always override it.
      return true;
    }

    /** @override */
    getCapabilityNotAvailableValueInternal() {
      return !this.getSelectedDestInternal() ||
          this.getSelectedDestInternal().id !=
          print_preview.Destination.GooglePromotedId.SAVE_AS_PDF;
    }
  }

  // Export
  return {FitToPage: FitToPage};
});
