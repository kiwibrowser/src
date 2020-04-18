// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'history-side-bar',

  behaviors: [Polymer.IronA11yKeysBehavior],

  properties: {
    selectedPage: {
      type: String,
      notify: true,
    },

    /** @private */
    guestSession_: {
      type: Boolean,
      value: loadTimeData.getBoolean('isGuestSession'),
    },

    showFooter: Boolean,
  },

  keyBindings: {
    'space:keydown': 'onSpacePressed_',
  },

  /**
   * @param {CustomEvent} e
   * @private
   */
  onSpacePressed_: function(e) {
    e.detail.keyboardEvent.path[0].click();
  },

  /**
   * @private
   */
  onSelectorActivate_: function() {
    this.fire('history-close-drawer');
  },

  /**
   * Relocates the user to the clear browsing data section of the settings page.
   * @param {Event} e
   * @private
   */
  onClearBrowsingDataTap_: function(e) {
    const browserService = md_history.BrowserService.getInstance();
    browserService.recordAction('InitClearBrowsingData');
    browserService.openClearBrowsingData();
    /** @type {PaperRippleElement} */ (this.$['cbd-ripple']).upAction();
    e.preventDefault();
  },

  /**
   * @return {string}
   * @private
   */
  computeClearBrowsingDataTabIndex_: function() {
    return this.guestSession_ ? '-1' : '';
  },

  /**
   * Prevent clicks on sidebar items from navigating. These are only links for
   * accessibility purposes, taps are handled separately by <iron-selector>.
   * @private
   */
  onItemClick_: function(e) {
    e.preventDefault();
  },
});
