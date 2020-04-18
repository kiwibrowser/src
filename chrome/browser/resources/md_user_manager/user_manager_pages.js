// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview 'user-manager-pages' is the element that controls paging in the
 * user manager screen.
 */
Polymer({
  is: 'user-manager-pages',

  properties: {
    /**
     * ID of the currently selected page.
     * @private {string}
     */
    selectedPage_: {type: String, value: 'user-pods-page'},

    /**
     * Data passed to the currently selected page.
     * @private {?Object}
     */
    pageData_: {type: Object, value: null}
  },

  listeners: {'change-page': 'onChangePage_'},

  /**
   * Handler for the change-page event.
   * @param {Event} e The event containing ID of the page that is to be selected
   *     and the optional data to be passed to the page.
   * @private
   */
  onChangePage_: function(e) {
    this.setSelectedPage(e.detail.page, e.detail.data);
  },

  /**
   * Sets the selected page.
   * @param {string} pageId ID of the page that is to be selected.
   * @param {Object=} opt_pageData Optional data to be passed to the page.
   */
  setSelectedPage: function(pageId, opt_pageData) {
    this.pageData_ = opt_pageData || null;
    this.selectedPage_ = pageId;
  },

  /**
   * This is to prevent events from propagating to the document element, which
   * erroneously triggers user-pod selections.
   *
   * TODO(scottchen): re-examine if its necessary for user_pod_row.js to bind
   * listeners on the entire document element.
   *
   * @param {!Event} e
   * @private
   */
  stopPropagation_: function(e) {
    e.stopPropagation();
  },

  /**
   * Returns True if the first argument is present in the given set of values.
   * @param {string} selectedPage ID of the currently selected page.
   * @param {...string} var_args Pages IDs to check the first argument against.
   * @return {boolean}
   */
  isPresentIn_: function(selectedPage, var_args) {
    const pages = Array.prototype.slice.call(arguments, 1);
    return pages.indexOf(selectedPage) !== -1;
  }
});
