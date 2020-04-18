// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'android-settings-element' is the element to open Android settings.
 */

Polymer({
  is: 'settings-android-settings-element',

  /** @private {?settings.AndroidAppsBrowserProxy} */
  browserProxy_: null,

  /** @override */
  created: function() {
    this.browserProxy_ = settings.AndroidAppsBrowserProxyImpl.getInstance();
  },

  /**
   * @param {!Event} event
   * @private
   */
  onManageAndroidAppsKeydown_: function(event) {
    if (event.key != 'Enter' && event.key != ' ')
      return;
    this.browserProxy_.showAndroidAppsSettings(true /** keyboardAction */);
    event.stopPropagation();
  },

  /** @private */
  onManageAndroidAppsTap_: function(event) {
    this.browserProxy_.showAndroidAppsSettings(false /** keyboardAction */);
  },
});