// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Polymer element for displaying material design Recommend Apps
 * screen.
 */

Polymer({
  is: 'recommend-apps',

  behaviors: [I18nBehavior],

  properties: {
    apps: {
      type: Array,
      value: []  // TODO(rsgingerrs): add the function to fetch the list of apps
    }
  },

  focus: function() {
    this.$.recommendAppsOverviewDialog.focus();
  },

  onSkip_: function() {
    chrome.send('login.RecommendAppsScreen.userActed', ['recommendAppsSkip']);
  },

  onInstall_: function() {
    // TODO(rsgingerrs): Actions if the user selects some apps to install
  },
});
