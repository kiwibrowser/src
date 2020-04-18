// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  /**
   * This element is a one way bound interface that routes the page URL to
   * the searchTerm and selectedId. Clients must initialize themselves by
   * reading the router's fields after attach.
   */
  is: 'bookmarks-router',

  behaviors: [
    bookmarks.StoreClient,
  ],

  properties: {
    /**
     * Parameter q is routed to the searchTerm.
     * Parameter id is routed to the selectedId.
     * @private
     */
    queryParams_: Object,

    /** @private */
    searchTerm_: {
      type: String,
      value: '',
    },

    /** @private {?string} */
    selectedId_: String,
  },

  observers: [
    'onQueryParamsChanged_(queryParams_)',
    'onStateChanged_(searchTerm_, selectedId_)',
  ],

  attached: function() {
    this.watch('selectedId_', function(state) {
      return state.selectedFolder;
    });
    this.watch('searchTerm_', function(state) {
      return state.search.term;
    });
    this.updateFromStore();
  },

  /** @private */
  onQueryParamsChanged_: function() {
    const searchTerm = this.queryParams_.q || '';
    let selectedId = this.queryParams_.id;
    if (!selectedId && !searchTerm)
      selectedId = BOOKMARKS_BAR_ID;

    if (searchTerm != this.searchTerm_) {
      this.searchTerm_ = searchTerm;
      this.dispatch(bookmarks.actions.setSearchTerm(searchTerm));
    }

    if (selectedId && selectedId != this.selectedId_) {
      this.selectedId_ = selectedId;
      // Need to dispatch a deferred action so that during page load
      // `this.getState()` will only evaluate after the Store is initialized.
      this.dispatchAsync((dispatch) => {
        dispatch(
            bookmarks.actions.selectFolder(selectedId, this.getState().nodes));
      });
    }
  },

  /** @private */
  onStateChanged_: function() {
    this.debounce('updateQueryParams', this.updateQueryParams_.bind(this));
  },

  /** @private */
  updateQueryParams_: function() {
    if (this.searchTerm_)
      this.queryParams_ = {q: this.searchTerm_};
    else if (this.selectedId_ != BOOKMARKS_BAR_ID)
      this.queryParams_ = {id: this.selectedId_};
    else
      this.queryParams_ = {};
  },
});
