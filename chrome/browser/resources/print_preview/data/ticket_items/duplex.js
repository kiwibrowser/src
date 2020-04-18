// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview.ticket_items', function() {
  'use strict';

  class Duplex extends print_preview.ticket_items.TicketItem {
    /**
     * Duplex ticket item whose value is a {@code boolean} that indicates
     * whether the document should be duplex printed.
     * @param {!print_preview.AppState} appState App state used to persist
     *     collate selection.
     * @param {!print_preview.DestinationStore} destinationStore Destination
     *     store used determine if a destination has the collate capability.
     */
    constructor(appState, destinationStore) {
      super(
          appState, print_preview.AppStateField.IS_DUPLEX_ENABLED,
          destinationStore);
    }

    /** @override */
    wouldValueBeValid(value) {
      return true;
    }

    /** @override */
    isCapabilityAvailable() {
      const cap = this.getDuplexCapability_();
      if (!cap) {
        return false;
      }
      let hasLongEdgeOption = false;
      let hasSimplexOption = false;
      cap.option.forEach(function(option) {
        hasLongEdgeOption = hasLongEdgeOption || option.type == 'LONG_EDGE';
        hasSimplexOption = hasSimplexOption || option.type == 'NO_DUPLEX';
      });
      return hasLongEdgeOption && hasSimplexOption;
    }

    /** @override */
    getDefaultValueInternal() {
      const cap = this.getDuplexCapability_();
      const defaultOptions = cap.option.filter(function(option) {
        return option.is_default;
      });
      return defaultOptions.length == 0 ? false :
                                          defaultOptions[0].type == 'LONG_EDGE';
    }

    /** @override */
    getCapabilityNotAvailableValueInternal() {
      return false;
    }

    /**
     * @return {Object} Duplex capability of the selected destination.
     * @private
     */
    getDuplexCapability_() {
      const dest = this.getSelectedDestInternal();
      return (dest && dest.capabilities && dest.capabilities.printer &&
              dest.capabilities.printer.duplex) ||
          null;
    }
  }

  // Export
  return {Duplex: Duplex};
});
