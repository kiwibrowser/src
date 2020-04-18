// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview.ticket_items', function() {
  'use strict';

  class CssBackground extends print_preview.ticket_items.TicketItem {
    /**
     * Ticket item whose value is a {@code boolean} that represents whether to
     * print CSS backgrounds.
     * @param {!print_preview.AppState} appState App state to persist CSS
     *     background value.
     * @param {!print_preview.DocumentInfo} documentInfo Information about the
     *     document to print.
     */
    constructor(appState, documentInfo) {
      super(
          appState, print_preview.AppStateField.IS_CSS_BACKGROUND_ENABLED,
          null /*destinationStore*/, documentInfo);
    }

    /** @override */
    wouldValueBeValid(value) {
      return true;
    }

    /** @override */
    isCapabilityAvailable() {
      return this.getDocumentInfoInternal().isModifiable;
    }

    /** @override */
    getDefaultValueInternal() {
      return false;
    }

    /** @override */
    getCapabilityNotAvailableValueInternal() {
      return false;
    }
  }

  // Export
  return {CssBackground: CssBackground};
});
