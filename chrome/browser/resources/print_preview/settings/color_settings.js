// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview', function() {
  'use strict';

  /**
   * Creates a ColorSettings object. This object encapsulates all settings and
   * logic related to color selection (color/bw).
   * @param {!print_preview.ticket_items.Color} colorTicketItem Used for writing
   *     and reading color value.
   * @constructor
   * @extends {print_preview.SettingsSection}
   */
  function ColorSettings(colorTicketItem) {
    print_preview.SettingsSection.call(this);

    /**
     * Used for reading/writing the color value.
     * @type {!print_preview.ticket_items.Color}
     * @private
     */
    this.colorTicketItem_ = colorTicketItem;
  }

  ColorSettings.prototype = {
    __proto__: print_preview.SettingsSection.prototype,

    /** @override */
    isAvailable: function() {
      return this.colorTicketItem_.isCapabilityAvailable();
    },

    /** @override */
    hasCollapsibleContent: function() {
      return false;
    },

    /** @override */
    set isEnabled(isEnabled) {
      this.select_.disabled = !isEnabled;
    },

    /** @override */
    enterDocument: function() {
      print_preview.SettingsSection.prototype.enterDocument.call(this);
      this.tracker.add(this.select_, 'change', this.onSelectChange_.bind(this));
      this.tracker.add(
          this.colorTicketItem_,
          print_preview.ticket_items.TicketItem.EventType.CHANGE,
          this.updateState_.bind(this));
    },

    /**
     * Called when the select element is changed. Updates the print ticket
     * color selection.
     * @private
     */
    onSelectChange_: function() {
      const select = this.select_;
      const isColor = select.options[select.selectedIndex].value == 'color';
      this.colorTicketItem_.updateValue(isColor);
    },

    /**
     * @return {!HTMLSelectElement} Select element containing the color options.
     * @private
     */
    get select_() {
      return /** @type {!HTMLSelectElement} */ (
          this.getChildElement('.color-settings-select'));
    },

    /**
     * Updates state of the widget.
     * @private
     */
    updateState_: function() {
      if (this.isAvailable()) {
        const select = this.select_;
        const valueToSelect = this.colorTicketItem_.getValue() ? 'color' : 'bw';
        for (let i = 0; i < select.options.length; i++) {
          if (select.options[i].value == valueToSelect) {
            select.selectedIndex = i;
            break;
          }
        }
      }
      this.updateUiStateInternal();
    }
  };

  // Export
  return {ColorSettings: ColorSettings};
});
