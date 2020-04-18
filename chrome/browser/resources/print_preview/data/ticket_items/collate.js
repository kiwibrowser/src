// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview.ticket_items', function() {
  'use strict';

  class Collate extends print_preview.ticket_items.TicketItem {
    /**
     * Collate ticket item whose value is a {@code boolean} that indicates
     * whether collation is enabled.
     * @param {!print_preview.AppState} appState App state used to persist
     *     collate selection.
     * @param {!print_preview.DestinationStore} destinationStore Destination
     *     store used determine if a destination has the collate capability.
     */
    constructor(appState, destinationStore) {
      super(
          appState, print_preview.AppStateField.IS_COLLATE_ENABLED,
          destinationStore);
    }

    /** @override */
    wouldValueBeValid(value) {
      return true;
    }

    /** @override */
    isCapabilityAvailable() {
      return !!this.getCollateCapability_();
    }

    /** @override */
    getDefaultValueInternal() {
      const capability = this.getCollateCapability_();
      return capability.hasOwnProperty('default') ? capability.default : true;
    }

    /** @override */
    getCapabilityNotAvailableValueInternal() {
      return true;
    }

    /**
     * @return {Object} Collate capability of the selected destination.
     * @private
     */
    getCollateCapability_() {
      const dest = this.getSelectedDestInternal();
      return (dest && dest.capabilities && dest.capabilities.printer &&
              dest.capabilities.printer.collate) ||
          null;
    }
  }

  // Export
  return {Collate: Collate};
});
