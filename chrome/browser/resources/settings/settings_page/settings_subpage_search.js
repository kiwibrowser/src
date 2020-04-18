// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'settings-subpage-search' shows a search field in the subpage's header.
 */

Polymer({
  is: 'settings-subpage-search',

  behaviors: [CrSearchFieldBehavior],

  properties: {
    autofocus: Boolean,
  },

  /** @return {!HTMLInputElement} */
  getSearchInput: function() {
    return this.$.searchInput;
  },

  /** @private */
  onTapClear_: function() {
    this.setValue('');
    this.$.searchInput.focus();
  },
});
