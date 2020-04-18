// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'history-toolbar',
  properties: {
    // Number of history items currently selected.
    // TODO(calamity): bind this to
    // listContainer.selectedItem.selectedPaths.length.
    count: {
      type: Number,
      value: 0,
      observer: 'changeToolbarView_',
    },

    // True if 1 or more history items are selected. When this value changes
    // the background colour changes.
    itemsSelected_: {
      type: Boolean,
      value: false,
      reflectToAttribute: true,
    },

    // The most recent term entered in the search field. Updated incrementally
    // as the user types.
    searchTerm: {
      type: String,
      observer: 'searchTermChanged_',
    },

    // True if the backend is processing and a spinner should be shown in the
    // toolbar.
    spinnerActive: {
      type: Boolean,
      value: false,
    },

    hasDrawer: {
      type: Boolean,
      reflectToAttribute: true,
    },

    hasMoreResults: Boolean,

    querying: Boolean,

    queryInfo: Object,

    // Whether to show the menu promo (a tooltip that points at the menu button
    // in narrow mode).
    showMenuPromo: Boolean,
  },

  /** @return {CrToolbarSearchFieldElement} */
  get searchField() {
    return /** @type {CrToolbarElement} */ (this.$['main-toolbar'])
        .getSearchField();
  },

  showSearchField: function() {
    this.searchField.showAndFocus();
  },

  deleteSelectedItems: function() {
    this.fire('delete-selected');
  },

  clearSelectedItems: function() {
    this.fire('unselect-all');
  },

  /**
   * Changes the toolbar background color depending on whether any history items
   * are currently selected.
   * @private
   */
  changeToolbarView_: function() {
    this.itemsSelected_ = this.count > 0;
  },

  /**
   * When changing the search term externally, update the search field to
   * reflect the new search term.
   * @private
   */
  searchTermChanged_: function() {
    if (this.searchField.getValue() != this.searchTerm) {
      this.searchField.showAndFocus();
      this.searchField.setValue(this.searchTerm);
    }
  },

  /**
   * @param {boolean} showMenuPromo
   * @return {boolean}
   * @private
   */
  canShowMenuPromo_: function(showMenuPromo) {
    return this.showMenuPromo && !loadTimeData.getBoolean('isGuestSession');
  },

  /**
   * @param {!CustomEvent} event
   * @private
   */
  onSearchChanged_: function(event) {
    this.fire('change-query', {search: event.detail});
  },

  /** @private */
  numberOfItemsSelected_: function(count) {
    return count > 0 ? loadTimeData.getStringF('itemsSelected', count) : '';
  },
});
