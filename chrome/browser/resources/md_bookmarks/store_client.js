// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Defines StoreClient, a Polymer behavior to tie a front-end
 * element to back-end data from the store.
 */

cr.define('bookmarks', function() {
  /**
   * @polymerBehavior
   * @implements {StoreObserver}
   */
  const StoreClient = {
    created: function() {
      /**
       * @type {!Array<{
       *   localProperty: string,
       *   valueGetter: function(!BookmarksPageState)
       * }>}
       */
      this.watches_ = [];
    },

    attached: function() {
      bookmarks.Store.getInstance().addObserver(this);
    },

    detached: function() {
      bookmarks.Store.getInstance().removeObserver(this);
    },

    /**
     * Watches a particular part of the state tree, updating |localProperty|
     * to the return value of |valueGetter| whenever the state changes. Eg, to
     * keep |this.item| updated with the value of a node:
     *   watch('item', (state) => state.nodes[this.itemId]);
     *
     * Note that object identity is used to determine if the value has changed
     * before updating the UI, rather than Polymer-style deep equality. If the
     * getter function returns |undefined|, no changes will propagate to the UI.
     *
     * Typechecking is supressed because this conflicts with
     * Object.prototype.watch, which is a Gecko-only method that is recognized
     * by Closure.
     * @suppress {checkTypes}
     * @param {string} localProperty
     * @param {function(!BookmarksPageState)} valueGetter
     */
    watch: function(localProperty, valueGetter) {
      if (!this.getPropertyInfo(localProperty).defined) {
        console.error(
            'No property ' + localProperty + ' defined on ' + this.is);
      }
      this.watches_.push({
        localProperty: localProperty,
        valueGetter: valueGetter,
      });
    },

    /**
     * Helper to dispatch an action to the store, which will update the store
     * data and then (possibly) flow through to the UI.
     * @param {?Action} action
     */
    dispatch: function(action) {
      bookmarks.Store.getInstance().dispatch(action);
    },

    /**
     * Helper to dispatch a DeferredAction to the store, which will
     * asynchronously perform updates to the store data and UI.
     * @param {DeferredAction} action
     */
    dispatchAsync: function(action) {
      bookmarks.Store.getInstance().dispatchAsync(action);
    },

    /** @param {string} newState */
    onStateChanged: function(newState) {
      this.watches_.forEach((watch) => {
        const oldValue = this[watch.localProperty];
        const newValue = watch.valueGetter(newState);

        // Avoid poking Polymer unless something has actually changed. Reducers
        // must return new objects rather than mutating existing objects, so
        // any real changes will pass through correctly.
        if (oldValue === newValue || newValue === undefined)
          return;

        this[watch.localProperty] = newValue;
      });
    },

    updateFromStore: function() {
      if (bookmarks.Store.getInstance().isInitialized())
        this.onStateChanged(bookmarks.Store.getInstance().data);
    },

    /** @return {!BookmarksPageState} */
    getState: function() {
      return bookmarks.Store.getInstance().data;
    },
  };

  return {
    StoreClient: StoreClient,
  };
});
