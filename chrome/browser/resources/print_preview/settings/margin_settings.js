// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview', function() {
  'use strict';

  /**
   * Creates a MarginSettings object. This object encapsulates all settings and
   * logic related to the margins mode.
   * @param {!print_preview.ticket_items.MarginsType} marginsTypeTicketItem Used
   *     to read and write the margins type ticket item.
   * @constructor
   * @extends {print_preview.SettingsSection}
   */
  function MarginSettings(marginsTypeTicketItem) {
    print_preview.SettingsSection.call(this);

    /**
     * Used to read and write the margins type ticket item.
     * @type {!print_preview.ticket_items.MarginsType}
     * @private
     */
    this.marginsTypeTicketItem_ = marginsTypeTicketItem;
  }

  /**
   * CSS classes used by the margin settings component.
   * @enum {string}
   * @private
   */
  MarginSettings.Classes_ = {SELECT: 'margin-settings-select'};

  MarginSettings.prototype = {
    __proto__: print_preview.SettingsSection.prototype,

    /** @override */
    isAvailable: function() {
      return this.marginsTypeTicketItem_.isCapabilityAvailable();
    },

    /** @override */
    hasCollapsibleContent: function() {
      return this.isAvailable();
    },

    /** @override */
    set isEnabled(isEnabled) {
      this.select_.disabled = !isEnabled;
    },

    /** @override */
    enterDocument: function() {
      print_preview.SettingsSection.prototype.enterDocument.call(this);
      this.tracker.add(
          assert(this.select_), 'change', this.onSelectChange.bind(this));
      this.tracker.add(
          this.marginsTypeTicketItem_,
          print_preview.ticket_items.TicketItem.EventType.CHANGE,
          this.onMarginsTypeTicketItemChange_.bind(this));
    },

    /**
     * @return {HTMLSelectElement} Select element containing the margin options.
     */
    get select_() {
      return this.getElement().getElementsByClassName(
          MarginSettings.Classes_.SELECT)[0];
    },

    /**
     * Called when the select element is changed. Updates the print ticket
     * margin type.
     */
    onSelectChange: function() {
      const select = this.select_;
      const marginsType =
          /** @type {!print_preview.ticket_items.MarginsTypeValue} */ (
              select.selectedIndex);
      this.marginsTypeTicketItem_.updateValue(marginsType);
    },

    /**
     * Called when the print ticket store changes. Selects the corresponding
     * select option.
     * @private
     */
    onMarginsTypeTicketItemChange_: function() {
      if (this.isAvailable()) {
        const select = this.select_;
        const marginsType =
            /** @type {!print_preview.ticket_items.MarginsTypeValue} */ (
                this.marginsTypeTicketItem_.getValue());
        const selectedMarginsType =
            /** @type {!print_preview.ticket_items.MarginsTypeValue} */ (
                select.selectedIndex);
        if (marginsType != selectedMarginsType) {
          select.options[selectedMarginsType].selected = false;
          select.options[marginsType].selected = true;
        }
      }
      this.updateUiStateInternal();
    }
  };

  // Export
  return {MarginSettings: MarginSettings};
});
