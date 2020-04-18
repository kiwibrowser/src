// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview 'settings-add-smb-share-dialog' is a component for adding
 * an SMB Share.
 */
Polymer({
  is: 'settings-add-smb-share-dialog',

  properties: {
    /** @private {string} */
    mountUrl_: String,

    /** @private {string} */
    username_: String,

    /** @private {string} */
    password_: String,
  },

  /** @private {?settings.SmbBrowserProxy} */
  browserProxy_: null,

  /** @override */
  created: function() {
    this.browserProxy_ = settings.SmbBrowserProxyImpl.getInstance();
  },

  /** @override */
  attached: function() {
    this.$.dialog.showModal();
  },

  /** @private */
  cancel_: function() {
    this.$.dialog.cancel();
  },

  /** @private */
  onAddButtonTap_: function() {
    this.browserProxy_.smbMount(this.mountUrl_, this.username_, this.password_);
    this.$.dialog.close();
  },

  /**
   * @return {boolean}
   * @private
   */
  canAddShare_: function() {
    return !!this.mountUrl_;
  },
});
