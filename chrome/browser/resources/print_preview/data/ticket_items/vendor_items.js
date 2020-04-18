// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview.ticket_items', function() {
  'use strict';

  class VendorItems extends cr.EventTarget {
    /**
     * An object that represents a user modifiable item in a print ticket. Each
     * ticket item has a value which can be set by the user. Ticket items can
     * also be unavailable for modifying if the print destination doesn't
     * support it or if other ticket item constraints are not met.
     * @param {print_preview.AppState} appState Application state model to
     *     update when ticket items update.
     * @param {print_preview.DestinationStore} destinationStore Used listen for
     *     changes in the currently selected destination's capabilities. Since
     *     this is a common dependency of ticket items, it's handled in the
     *     base class.
     */
    constructor(appState, destinationStore) {
      super();

      /**
       * Application state model to update when ticket items update.
       * @private {print_preview.AppState}
       */
      this.appState_ = appState || null;

      /**
       * Used listen for changes in the currently selected destination's
       * capabilities.
       * @private {print_preview.DestinationStore}
       */
      this.destinationStore_ = destinationStore || null;

      /**
       * Vendor ticket items store, maps item id to the item value.
       * @private {!Object<string>}
       */
      this.items_ = {};
    }

    /** @return {boolean} Whether vendor capabilities are available. */
    isCapabilityAvailable() {
      return !!this.capability;
    }

    /** @return {boolean} Whether the ticket item was modified by the user. */
    isUserEdited() {
      // If there's at least one ticket item stored in values, it was edited.
      for (const key in this.items_) {
        if (this.items_.hasOwnProperty(key))
          return true;
      }
      return false;
    }

    /** @return {Object} Vendor capabilities of the selected destination. */
    get capability() {
      const destination = this.destinationStore_ ?
          this.destinationStore_.selectedDestination :
          null;
      if (!destination)
        return null;
      if (destination.type == print_preview.DestinationType.MOBILE) {
        return null;
      }
      return (destination.capabilities && destination.capabilities.printer &&
              destination.capabilities.printer.vendor_capability) ||
          null;
    }

    /**
     * Vendor ticket items store, maps item id to the item value.
     * @return {!Object<string>}
     */
    get ticketItems() {
      return this.items_;
    }

    /**
     * @param {!Object<string>} values Values to set as the values of vendor
     *     ticket items. Maps vendor item id to the value.
     */
    updateValue(values) {
      this.items_ = {};
      if (typeof values == 'object') {
        for (const key in values) {
          if (values.hasOwnProperty(key) && typeof values[key] == 'string') {
            // Let's empirically limit each value at 2K.
            this.items_[key] = values[key].substring(0, 2048);
          }
        }
      }

      if (this.appState_) {
        this.appState_.persistField(
            print_preview.AppStateField.VENDOR_OPTIONS, this.items_);
      }
    }
  }

  // Export
  return {VendorItems: VendorItems};
});
