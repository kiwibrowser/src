// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview.ticket_items', function() {
  'use strict';

  class Copies extends print_preview.ticket_items.TicketItem {
    /**
     * Copies ticket item whose value is a {@code string} that indicates how
     * many copies of the document should be printed. The ticket item is backed
     * by a string since the user can textually input the copies value.
     * @param {!print_preview.DestinationStore} destinationStore Destination
     *     store used to determine if a destination has the copies capability.
     */
    constructor(destinationStore) {
      super(null /*appState*/, null /*field*/, destinationStore);
    }

    /** @override */
    wouldValueBeValid(value) {
      return value != '';
    }

    /** @override */
    isCapabilityAvailable() {
      return !!this.getCopiesCapability_();
    }

    /** @return {number} The number of copies indicated by the ticket item. */
    getValueAsNumber() {
      const value = this.getValue();
      return value == '' ? 0 : parseInt(value, 10);
    }

    /** @override */
    getDefaultValueInternal() {
      const cap = this.getCopiesCapability_();
      return cap.hasOwnProperty('default') ? cap.default : '1';
    }

    /** @override */
    getCapabilityNotAvailableValueInternal() {
      return '1';
    }

    /**
     * @return {Object} Copies capability of the selected destination.
     * @private
     */
    getCopiesCapability_() {
      const dest = this.getSelectedDestInternal();
      return (dest && dest.capabilities && dest.capabilities.printer &&
              dest.capabilities.printer.copies) ||
          null;
    }
  }

  // Export
  return {Copies: Copies};
});
