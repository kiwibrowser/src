// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'oobe-a11y-option',

  properties: {
    /**
     * If cr-toggle is checked.
     */
    checked: Boolean,

    /**
     * Chrome message handling this option.
     */
    chromeMessage: String,

    /**
     * ARIA-label for the button.
     *
     * Note that we are not using "aria-label" property here, because
     * we want to pass the label value but not actually declare it as an
     * ARIA property anywhere but the actual target element.
     */
    labelForAria: String,
  },

  focus: function() {
    this.$.button.focus();
  },
});
