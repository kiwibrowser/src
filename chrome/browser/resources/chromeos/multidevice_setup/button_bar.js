// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * DOM Element containing (page-dependent) navigation buttons for the
 * MultiDevice Setup WebUI.
 */
Polymer({
  is: 'button-bar',

  properties: {
    /**
     *  Translated text to display on the forward-naviation button.
     *
     *  Undefined if the visible page has no forward-navigation button.
     *
     *  @type {string|undefined}
     */
    forwardButtonText: String,

    /**
     *  Translated text to display on the backward-naviation button.
     *
     *  Undefined if the visible page has no backward-navigation button.
     *
     *  @type {string|undefined}
     */
    backwardButtonText: String,
  },

  /** @private */
  onForwardButtonClicked_: function() {
    this.fire('forward-navigation-requested');
  },

  /** @private */
  onBackwardButtonClicked_: function() {
    this.fire('backward-navigation-requested');
  },
});
