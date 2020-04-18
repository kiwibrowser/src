// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview', function() {
  'use strict';

  /**
   * Base class for the printer option element visualizing the generic selection
   * based option.
   * @param {(!print_preview.ticket_items.Dpi |
   *          !print_preview.ticket_items.MediaSize)} ticketItem
   *     Ticket item visualized by this component. Must have a defined
   *     capability() getter.
   * @constructor
   * @extends {print_preview.SettingsSection}
   */
  function SettingsSectionSelect(ticketItem) {
    print_preview.SettingsSection.call(this);

    /**
     * {(!print_preview.ticket_items.Dpi |
     *   !print_preview.ticket_items.MediaSize)}
     * @private
     */
    this.ticketItem_ = ticketItem;
  }

  SettingsSectionSelect.prototype = {
    __proto__: print_preview.SettingsSection.prototype,

    /** @override */
    isAvailable: function() {
      return this.ticketItem_.isCapabilityAvailable();
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
          assert(this.select_), 'change', this.onSelectChange_.bind(this));
      this.tracker.add(
          this.ticketItem_,
          print_preview.ticket_items.TicketItem.EventType.CHANGE,
          this.onTicketItemChange_.bind(this));
    },

    /**
     * @return {HTMLSelectElement} Select element containing option items.
     * @private
     */
    get select_() {
      return this.getElement().querySelector('.settings-select');
    },

    /**
     * Makes sure the content of the select element matches the capabilities of
     * the destination.
     * @private
     */
    updateSelect_: function() {
      const select = this.select_;
      if (!this.isAvailable()) {
        select.innerHTML = '';
        return;
      }
      // Should the select content be updated?
      const sameContent =
          this.ticketItem_.capability().option.length == select.length &&
          this.ticketItem_.capability().option.every(function(option, index) {
            return select.options[index].value == JSON.stringify(option);
          });
      let indexToSelect = select.selectedIndex;
      if (!sameContent) {
        select.innerHTML = '';
        this.ticketItem_.capability().option.forEach(function(option, index) {
          const selectOption = document.createElement('option');
          selectOption.text = this.getCustomDisplayName_(option) ||
              this.getDefaultDisplayName_(option);
          selectOption.value = JSON.stringify(option);
          select.appendChild(selectOption);
          if (option.is_default)
            indexToSelect = index;
        }, this);
      }
      // Try to select current ticket item.
      const valueToSelect = JSON.stringify(this.ticketItem_.getValue());
      for (let i = 0, option; (option = select.options[i]); i++) {
        if (option.value == valueToSelect) {
          indexToSelect = i;
          break;
        }
      }
      select.selectedIndex = indexToSelect;
      this.onSelectChange_();
    },

    /**
     * @param {!Object} option Option to get the custom display name for.
     * @return {string} Custom display name for the option.
     * @private
     */
    getCustomDisplayName_: function(option) {
      let displayName = option.custom_display_name;
      if (!displayName && option.custom_display_name_localized) {
        displayName =
            getStringForCurrentLocale(option.custom_display_name_localized);
      }
      return displayName;
    },

    /**
     * @param {!Object} option Option to get the default display name for.
     * @return {string} Default option display name.
     * @private
     */
    getDefaultDisplayName_: function(option) {
      throw Error('Abstract method not overridden');
    },

    /**
     * Called when the select element is changed. Updates the print ticket.
     * @private
     */
    onSelectChange_: function() {
      const select = this.select_;
      this.ticketItem_.updateValue(
          JSON.parse(select.options[select.selectedIndex].value));
    },

    /**
     * Called when the print ticket store changes. Selects the corresponding
     * select option.
     * @private
     */
    onTicketItemChange_: function() {
      this.updateSelect_();
      this.updateUiStateInternal();
    }
  };

  // Export
  return {SettingsSectionSelect: SettingsSectionSelect};
});
