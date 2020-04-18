// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Helper functions for implementing an incremental search field. See
 * <settings-subpage-search> for a simple implementation.
 * @polymerBehavior
 */
var CrSearchFieldBehavior = {
  properties: {
    label: {
      type: String,
      value: '',
    },

    clearLabel: {
      type: String,
      value: '',
    },

    hasSearchText: {
      type: Boolean,
      reflectToAttribute: true,
      value: false,
    },

    /** @private */
    lastValue_: {
      type: String,
      value: '',
    },
  },

  /**
   * @return {!HTMLInputElement} The input field element the behavior should
   *     use.
   */
  getSearchInput: function() {},

  /**
   * @return {string} The value of the search field.
   */
  getValue: function() {
    return this.getSearchInput().value;
  },

  /**
   * Sets the value of the search field.
   * @param {string} value
   * @param {boolean=} opt_noEvent Whether to prevent a 'search-changed' event
   *     firing for this change.
   */
  setValue: function(value, opt_noEvent) {
    var searchInput = this.getSearchInput();
    searchInput.value = value;

    this.onSearchTermInput();
    this.onValueChanged_(value, !!opt_noEvent);
  },

  onSearchTermSearch: function() {
    this.onValueChanged_(this.getValue(), false);
  },

  /**
   * Update the state of the search field whenever the underlying input value
   * changes. Unlike onsearch or onkeypress, this is reliably called immediately
   * after any change, whether the result of user input or JS modification.
   */
  onSearchTermInput: function() {
    this.hasSearchText = this.$.searchInput.value != '';
  },

  /**
   * Updates the internal state of the search field based on a change that has
   * already happened.
   * @param {string} newValue
   * @param {boolean} noEvent Whether to prevent a 'search-changed' event firing
   *     for this change.
   * @private
   */
  onValueChanged_: function(newValue, noEvent) {
    if (newValue == this.lastValue_)
      return;

    this.lastValue_ = newValue;

    if (!noEvent)
      this.fire('search-changed', newValue);
  },
};
