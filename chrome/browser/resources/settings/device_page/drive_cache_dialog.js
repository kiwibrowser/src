// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'settings-drive-cache-dialog' is the dialog to delete Google Drive temporary
 * offline files.
 */
Polymer({
  is: 'settings-drive-cache-dialog',

  behaviors: [
    I18nBehavior,
  ],

  open: function() {
    this.$.dialog.showModal();
  },

  /** @private */
  onCancelTap_: function() {
    this.$.dialog.cancel();
  },

  /** @private */
  onDeleteTap_: function() {
    chrome.send('clearDriveCache');
    this.$.dialog.close();
  },
});
