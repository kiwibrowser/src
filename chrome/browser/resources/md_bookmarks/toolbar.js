// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'bookmarks-toolbar',

  behaviors: [
    bookmarks.StoreClient,
  ],

  properties: {
    sidebarWidth: {
      type: String,
      observer: 'onSidebarWidthChanged_',
    },

    showSelectionOverlay: {
      type: Boolean,
      computed: 'shouldShowSelectionOverlay_(selectedItems_, globalCanEdit_)',
      readOnly: true,
      reflectToAttribute: true,
    },

    /** @private */
    narrow_: {
      type: Boolean,
      reflectToAttribute: true,
    },

    /** @private */
    searchTerm_: {
      type: String,
      observer: 'onSearchTermChanged_',
    },

    /** @private {!Set<string>} */
    selectedItems_: Object,

    /** @private */
    globalCanEdit_: Boolean,
  },

  attached: function() {
    this.watch('searchTerm_', function(state) {
      return state.search.term;
    });
    this.watch('selectedItems_', function(state) {
      return state.selection.items;
    });
    this.watch('globalCanEdit_', function(state) {
      return state.prefs.canEdit;
    });
    this.updateFromStore();
  },

  /** @return {CrToolbarSearchFieldElement} */
  get searchField() {
    return /** @type {CrToolbarElement} */ (this.$$('cr-toolbar'))
        .getSearchField();
  },

  /**
   * @param {Event} e
   * @private
   */
  onMenuButtonOpenTap_: function(e) {
    this.fire('open-command-menu', {
      targetElement: e.target,
      source: MenuSource.TOOLBAR,
    });
  },

  /** @private */
  onDeleteSelectionTap_: function() {
    const selection = this.selectedItems_;
    const commandManager = bookmarks.CommandManager.getInstance();
    assert(commandManager.canExecute(Command.DELETE, selection));
    commandManager.handle(Command.DELETE, selection);
  },

  /** @private */
  onClearSelectionTap_: function() {
    this.dispatch(bookmarks.actions.deselectItems());
  },

  /**
   * @param {Event} e
   * @private
   */
  onSearchChanged_: function(e) {
    const searchTerm = /** @type {string} */ (e.detail);
    if (searchTerm != this.searchTerm_)
      this.dispatch(bookmarks.actions.setSearchTerm(searchTerm));
  },

  /** @private */
  onSidebarWidthChanged_: function() {
    this.style.setProperty('--sidebar-width', this.sidebarWidth);
  },

  /** @private */
  onSearchTermChanged_: function() {
    this.searchField.setValue(this.searchTerm_ || '');
  },

  /**
   * @return {boolean}
   * @private
   */
  shouldShowSelectionOverlay_: function() {
    return this.selectedItems_.size > 1 && this.globalCanEdit_;
  },

  canDeleteSelection_: function() {
    return bookmarks.CommandManager.getInstance().canExecute(
        Command.DELETE, this.selectedItems_);
  },

  /**
   * @return {string}
   * @private
   */
  getItemsSelectedString_: function() {
    return loadTimeData.getStringF('itemsSelected', this.selectedItems_.size);
  },
});
