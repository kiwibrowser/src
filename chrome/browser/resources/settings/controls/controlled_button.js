// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'controlled-button',

  behaviors: [
    CrPolicyPrefBehavior,
    PrefControlBehavior,
  ],

  properties: {
    endJustified: {
      type: Boolean,
      value: false,
      reflectToAttribute: true,
    },

    label: String,

    /** @private */
    enforced_: {
      type: Boolean,
      computed: 'isPrefEnforced(pref.*)',
      reflectToAttribute: true,
    },
  },

  /**
   * @param {!Event} e
   * @private
   */
  onIndicatorTap_: function(e) {
    // Disallow <controlled-button on-click="..."> when controlled.
    e.preventDefault();
    e.stopPropagation();
  },
});
