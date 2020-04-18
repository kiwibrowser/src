// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview', function() {
  'use strict';

  /**
   * Print options section to control printer advanced options.
   * @param {!print_preview.ticket_items.VendorItems} ticketItem Ticket item to
   *     check settings availability.
   * @param {!print_preview.DestinationStore} destinationStore Used to determine
   *     the selected destination.
   * @constructor
   * @extends {print_preview.SettingsSection}
   */
  function AdvancedOptionsSettings(ticketItem, destinationStore) {
    print_preview.SettingsSection.call(this);

    /**
     * Ticket item to check settings availability.
     * @private {!print_preview.ticket_items.VendorItems}
     */
    this.ticketItem_ = ticketItem;

    /**
     * Used to determine the selected destination.
     * @private {!print_preview.DestinationStore}
     */
    this.destinationStore_ = destinationStore;
  }

  /**
   * Event types dispatched by the component.
   * @enum {string}
   */
  AdvancedOptionsSettings.EventType = {
    BUTTON_ACTIVATED: 'print_preview.AdvancedOptionsSettings.BUTTON_ACTIVATED'
  };

  AdvancedOptionsSettings.prototype = {
    __proto__: print_preview.SettingsSection.prototype,

    /** @override */
    isAvailable: function() {
      return this.ticketItem_.isCapabilityAvailable();
    },

    /** @override */
    hasCollapsibleContent: function() {
      return this.isAvailable();
    },

    /** @param {boolean} isEnabled Whether the component is enabled. */
    set isEnabled(isEnabled) {
      this.getButton_().disabled = !isEnabled;
    },

    /** @override */
    enterDocument: function() {
      print_preview.SettingsSection.prototype.enterDocument.call(this);

      this.tracker.add(this.getButton_(), 'click', () => {
        cr.dispatchSimpleEvent(
            this, AdvancedOptionsSettings.EventType.BUTTON_ACTIVATED);
      });
      this.tracker.add(
          this.destinationStore_,
          print_preview.DestinationStore.EventType.DESTINATION_SELECT,
          this.onDestinationChanged_.bind(this));
      this.tracker.add(
          this.destinationStore_,
          print_preview.DestinationStore.EventType
              .SELECTED_DESTINATION_CAPABILITIES_READY,
          this.onDestinationChanged_.bind(this));
    },

    /**
     * @return {!HTMLElement}
     * @private
     */
    getButton_: function() {
      return this.getChildElement('.advanced-options-settings-button');
    },

    /**
     * Called when the destination selection has changed. Updates UI elements.
     * @private
     */
    onDestinationChanged_: function() {
      this.updateUiStateInternal();
    }
  };

  // Export
  return {AdvancedOptionsSettings: AdvancedOptionsSettings};
});
