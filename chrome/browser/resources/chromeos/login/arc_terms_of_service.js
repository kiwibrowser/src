// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Polymer element for displaying material design for ARC Terms Of
 * Service screen.
 */

Polymer({
  is: 'arc-tos-root',

  properties: {
    /**
     * Accept, Skip and Retry buttons are disabled until content is loaded.
     */
    arcTosButtonsDisabled: {
      type: Boolean,
      value: true,
    },

    /**
     * Reference to OOBE screen object.
     * @type {!OobeTypes.Screen}
     */
    screen: {
      type: Object,
    },
  },

  /**
   * Returns element by its id.
   */
  getElement: function(id) {
    return this.$[id];
  },

  /**
   * Returns focused element inside this element.
   */
  getActiveElement: function(id) {
    return this.shadowRoot.activeElement;
  },

  /**
   * On-tap event handler for Accept button.
   *
   * @private
   */
  onAccept_: function() {
    this.screen.onAccept();
  },

  /**
   * On-tap event handler for Next button.
   *
   * @private
   */
  onNext_: function() {
    this.screen.onNext();
  },

  /**
   * On-tap event handler for Retry button.
   *
   * @private
   */
  onRetry_: function() {
    this.screen.reloadPlayStoreToS();
  },

  /**
   * On-tap event handler for Skip button.
   *
   * @private
   */
  onSkip_: function() {
    this.screen.onSkip();
  }
});
