// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'category-setting-exceptions' is the polymer element for showing a certain
 * category of exceptions under Site Settings.
 */
Polymer({
  is: 'category-setting-exceptions',

  properties: {
    /**
     * Some content types (like Location) do not allow the user to manually
     * edit the exception list from within Settings.
     * @private
     */
    readOnlyList: {
      type: Boolean,
      value: false,
    },

    /**
     * The heading text for the blocked exception list.
     */
    blockHeader: String,
  },

  /** @override */
  ready: function() {
    this.ContentSetting = settings.ContentSetting;
  },
});
