// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {
'use strict';

Polymer({
  is: 'print-preview-destination-list',

  behaviors: [I18nBehavior],

  properties: {
    /** @type {Array<!print_preview.Destination>} */
    destinations: {
      type: Array,
      observer: 'destinationsChanged_',
    },

    /** @type {boolean} */
    hasActionLink: {
      type: Boolean,
      value: false,
    },

    /** @type {boolean} */
    loadingDestinations: {
      type: Boolean,
      value: false,
    },

    /** @type {?RegExp} */
    searchQuery: {
      type: Object,
      observer: 'update_',
    },

    /** @type {boolean} */
    title: String,

    /** @private {number} */
    matchingDestinationsCount_: {
      type: Number,
      value: 0,
    },

    /** @private {boolean} */
    hasDestinations_: {
      type: Boolean,
      computed: 'computeHasDestinations_(matchingDestinationsCount_)',
    },

    /** @private {boolean} */
    showDestinationsTotal_: {
      type: Boolean,
      computed: 'computeShowDestinationsTotal_(matchingDestinationsCount_)',
    },
  },

  /** @private {boolean} */
  newDestinations_: false,

  /**
   * @param {!Array<!print_preview.Destination>} current
   * @param {?Array<!print_preview.Destination>} previous
   * @private
   */
  destinationsChanged_: function(current, previous) {
    if (previous == undefined) {
      this.matchingDestinationsCount_ = this.destinations.length;
    } else {
      this.newDestinations_ = true;
    }
  },

  /** @private */
  updateIfNeeded_: function() {
    if (!this.newDestinations_)
      return;
    this.newDestinations_ = false;
    this.update_();
  },

  /** @private */
  update_: function() {
    if (!this.destinations)
      return;

    const listItems =
        this.shadowRoot.querySelectorAll('print-preview-destination-list-item');

    let matchCount = 0;
    listItems.forEach(item => {
      item.hidden =
          !!this.searchQuery && !item.destination.matches(this.searchQuery);
      if (!item.hidden) {
        matchCount++;
        item.update();
      }
    });

    this.matchingDestinationsCount_ =
        !this.searchQuery ? listItems.length : matchCount;
  },

  /**
   * @return {boolean}
   * @private
   */
  computeHasDestinations_: function() {
    return !this.destinations || this.matchingDestinationsCount_ > 0;
  },

  /**
   * @return {boolean}
   * @private
   */
  computeShowDestinationsTotal_: function() {
    return this.matchingDestinationsCount_ > 4;
  },

  /** @private */
  onActionLinkClick_: function() {
    print_preview.NativeLayer.getInstance().managePrinters();
  },

  /**
   * @param {!Event} e Event containing the destination that was selected.
   * @private
   */
  onDestinationSelected_: function(e) {
    this.fire('destination-selected', e.target);
  },
});
})();
