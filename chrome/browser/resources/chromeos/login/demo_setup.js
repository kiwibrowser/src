// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Polymer element for displaying material design Demo Setup
 * screen.
 */

Polymer({
  is: 'demo-setup-md',

  /**
   * Online setup button click handler.
   * @private
   */
  onOnlineSetupClicked_: function(e) {
    chrome.send('login.DemoSetupScreen.userActed', ['online-setup']);
    e.stopPropagation();
  },

  /**
   * Offline setup button click handler.
   * @private
   */
  onOfflineSetupClicked_: function(e) {
    chrome.send('login.DemoSetupScreen.userActed', ['offline-setup']);
    e.stopPropagation();
  },

  /**
   * Close button click handler.
   * @private
   */
  onBackButtonClicked_: function(e) {
    chrome.send('login.DemoSetupScreen.userActed', ['close-setup']);
    e.stopPropagation();
  },
});
