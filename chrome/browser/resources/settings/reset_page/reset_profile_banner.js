// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'settings-reset-profile-banner' is the banner shown for prompting the user to
 * clear profile settings.
 */
Polymer({
  // TODO(dpapad): Rename to settings-reset-warning-dialog.
  is: 'settings-reset-profile-banner',

  listeners: {
    'cancel': 'onCancel_',
  },

  /** @override */
  attached: function() {
    this.$.dialog.showModal();
  },

  /** @private */
  onOkTap_: function() {
    this.$.dialog.cancel();
  },

  /** @private */
  onCancel_: function() {
    settings.ResetBrowserProxyImpl.getInstance().onHideResetProfileBanner();
  },

  /** @private */
  onResetTap_: function() {
    this.$.dialog.close();
    settings.navigateTo(settings.routes.RESET_DIALOG);
  },
});
