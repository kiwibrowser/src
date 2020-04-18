// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'change-password-page' is the settings page containing change password
 * settings.
 */
Polymer({
  is: 'settings-change-password-page',

  /** @private */
  changePassword_: function() {
    listenOnce(this, 'transitionend', () => {
      settings.ChangePasswordBrowserProxyImpl.getInstance().changePassword();
    });
  },
});
