// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'history-router',

  properties: {
    selectedPage: {
      type: String,
      notify: true,
      observer: 'selectedPageChanged_',
    },

    /** @type {QueryState} */
    queryState: Object,

    path_: String,

    queryParams_: Object,
  },

  /** @private {boolean} */
  parsing_: false,

  observers: [
    'onUrlChanged_(path_, queryParams_)',
  ],

  /** @override */
  attached: function() {
    // Redirect legacy search URLs to URLs compatible with material history.
    if (window.location.hash) {
      window.location.href = window.location.href.split('#')[0] + '?' +
          window.location.hash.substr(1);
    }
  },

  /**
   * Write all relevant page state to the URL.
   */
  serializeUrl: function() {
    let path = this.selectedPage;

    if (path == 'history')
      path = '';

    // Make all modifications at the end of the method so observers can't change
    // the outcome.
    this.path_ = '/' + path;
    this.set('queryParams_.q', this.queryState.searchTerm || null);
  },

  /** @private */
  selectedPageChanged_: function() {
    // Update the URL if the page was changed externally, but ignore the update
    // if it came from parseUrl_().
    if (!this.parsing_)
      this.serializeUrl();
  },

  /** @private */
  parseUrl_: function() {
    this.parsing_ = true;
    const changes = {};
    const sections = this.path_.substr(1).split('/');
    const page = sections[0] || 'history';

    changes.search = this.queryParams_.q || '';

    // Must change selectedPage before `change-query`, otherwise the
    // query-manager will call serializeUrl() with the old page.
    this.selectedPage = page;
    this.fire('change-query', changes);
    this.serializeUrl();

    this.parsing_ = false;
  },

  /** @private */
  onUrlChanged_: function() {
    // Changing the url and query parameters at the same time will cause two
    // calls to onUrlChanged_. Debounce the actual work so that these two
    // changes get processed together.
    this.debounce('parseUrl', this.parseUrl_.bind(this));
  },
});
