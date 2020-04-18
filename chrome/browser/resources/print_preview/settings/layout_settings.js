// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview', function() {
  'use strict';

  /**
   * Creates a LayoutSettings object. This object encapsulates all settings and
   * logic related to layout mode (portrait/landscape).
   * @param {!print_preview.ticket_items.Landscape} landscapeTicketItem Used to
   *     get the layout written to the print ticket.
   * @constructor
   * @extends {print_preview.SettingsSection}
   */
  function LayoutSettings(landscapeTicketItem) {
    print_preview.SettingsSection.call(this);

    /**
     * Used to get the layout written to the print ticket.
     * @type {!print_preview.ticket_items.Landscape}
     * @private
     */
    this.landscapeTicketItem_ = landscapeTicketItem;
  }

  LayoutSettings.prototype = {
    __proto__: print_preview.SettingsSection.prototype,

    /** @override */
    isAvailable: function() {
      return this.landscapeTicketItem_.isCapabilityAvailable();
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
      this.tracker.add(this.select_, 'change', this.onSelectChange.bind(this));
      this.tracker.add(
          this.landscapeTicketItem_,
          print_preview.ticket_items.TicketItem.EventType.CHANGE,
          this.onLandscapeTicketItemChange_.bind(this));
    },

    /**
     * Called when the select element is changed. Updates the print ticket
     * layout selection.
     */
    onSelectChange: function() {
      const select = this.select_;
      const isLandscape =
          select.options[select.selectedIndex].value == 'landscape';
      this.landscapeTicketItem_.updateValue(isLandscape);
    },

    /**
     * @return {!HTMLSelectElement} Select element containing the layout
     *     options.
     * @private
     */
    get select_() {
      return /** @type {!HTMLSelectElement} */ (
          this.getChildElement('.layout-settings-select'));
    },

    /**
     * Called when the print ticket store changes state. Updates the state of
     * the radio buttons and hides the setting if necessary.
     * @private
     */
    onLandscapeTicketItemChange_: function() {
      if (this.isAvailable()) {
        const select = this.select_;
        const valueToSelect =
            this.landscapeTicketItem_.getValue() ? 'landscape' : 'portrait';
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
  return {LayoutSettings: LayoutSettings};
});
