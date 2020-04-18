// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview', function() {
  'use strict';

  /**
   * Component that renders a search box for searching through destinations.
   * @param {string} searchBoxPlaceholderText Search box placeholder text.
   * @constructor
   * @extends {print_preview.Component}
   */
  function SearchBox(searchBoxPlaceholderText) {
    print_preview.Component.call(this);

    /**
     * Search box placeholder text.
     * @private {string}
     */
    this.searchBoxPlaceholderText_ = searchBoxPlaceholderText;

    /**
     * Timeout used to control incremental search.
     * @private {?number}
     */
    this.timeout_ = null;

    /**
     * Input box where the query is entered.
     * @private {HTMLInputElement}
     */
    this.input_ = null;
  }

  /**
   * Enumeration of event types dispatched from the search box.
   * @enum {string}
   */
  SearchBox.EventType = {SEARCH: 'print_preview.SearchBox.SEARCH'};

  /**
   * Delay in milliseconds before dispatching a SEARCH event.
   * @private {number}
   * @const
   */
  SearchBox.SEARCH_DELAY_ = 150;

  SearchBox.prototype = {
    __proto__: print_preview.Component.prototype,

    /** @param {?string} query New query to set the search box's query to. */
    setQuery: function(query) {
      query = query || '';
      this.input_.value = query.trim();
    },

    /** Sets the input element of the search box in focus. */
    focus: function() {
      this.input_.focus();
    },

    /** @override */
    createDom: function() {
      this.setElementInternal(
          this.cloneTemplateInternal('search-box-template'));
      this.input_ = assertInstanceof(
          this.getChildElement('.search-box-input'), HTMLInputElement);
      this.input_.setAttribute('placeholder', this.searchBoxPlaceholderText_);
    },

    /** @override */
    enterDocument: function() {
      print_preview.Component.prototype.enterDocument.call(this);
      this.tracker.add(
          assert(this.input_), 'input', this.onInputInput_.bind(this));
    },

    /** @override */
    exitDocument: function() {
      print_preview.Component.prototype.exitDocument.call(this);
      this.input_ = null;
    },

    /**
     * @return {string} The current query of the search box.
     * @private
     */
    getQuery_: function() {
      return this.input_.value.trim();
    },

    /**
     * Dispatches a SEARCH event.
     * @private
     */
    dispatchSearchEvent_: function() {
      this.timeout_ = null;
      const searchEvent = new Event(SearchBox.EventType.SEARCH);
      const query = this.getQuery_();
      searchEvent.query = query;
      if (query) {
        // Generate regexp-safe query by escaping metacharacters.
        const safeQuery = query.replace(/[-[\]{}()*+?.,\\^$|#\s]/g, '\\$&');
        searchEvent.queryRegExp = new RegExp('(' + safeQuery + ')', 'ig');
      } else {
        searchEvent.queryRegExp = null;
      }
      this.dispatchEvent(searchEvent);
    },

    /**
     * Called when the input element's value changes. Dispatches a search event.
     * @private
     */
    onInputInput_: function() {
      if (this.timeout_)
        clearTimeout(this.timeout_);
      this.timeout_ = setTimeout(
          this.dispatchSearchEvent_.bind(this), SearchBox.SEARCH_DELAY_);
    }
  };

  // Export
  return {SearchBox: SearchBox};
});
