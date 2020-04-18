// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Demo mode setup screen implementation.
 */

login.createScreen('DemoSetupScreen', 'demo-setup', function() {
  return {
    UI_STATE: {ERROR: -1, DEFAULT: 0, PROCESSING: 1},
    EXTERNAL_API: ['setState'],

    get defaultControl() {
      return $('demo-setup-content');
    },

    /** @override */
    onBeforeShow: function(data) {
      this.setState(this.UI_STATE.DEFAULT);
    },

    /**
     * Sets state of the UI.
     * @param {number} state.
     */
    setState: function(state) {
      this.state_ = state;
    },
  };
});
