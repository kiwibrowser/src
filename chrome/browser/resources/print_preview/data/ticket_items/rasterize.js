// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview.ticket_items', function() {
  'use strict';

  class Rasterize extends print_preview.ticket_items.TicketItem {
    /**
     * Rasterize ticket item whose value is a {@code boolean} that indicates
     * whether the PDF document should be rendered as images.
     * @param {!print_preview.DocumentInfo} documentInfo Information about the
     *     document to print, used to determine if document is a PDF.
     */
    constructor(destinationStore, documentInfo) {
      super(
          null /* appState */, null /* field */, null /* destinationStore */,
          documentInfo);
    }

    /** @override */
    wouldValueBeValid(value) {
      return true;
    }

    /** @override */
    isCapabilityAvailable() {
      return !this.getDocumentInfoInternal().isModifiable;
    }

    /** @override */
    getDefaultValueInternal() {
      return false;
    }

    /** @override */
    getCapabilityNotAvailableValueInternal() {
      return this.getDefaultValueInternal();
    }
  }

  // Export
  return {Rasterize: Rasterize};
});
