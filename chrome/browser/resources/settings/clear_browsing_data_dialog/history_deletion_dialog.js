// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview 'settings-history-deletion-dialog' is a dialog that is
 * optionally shown inside settings-clear-browsing-data-dialog after deleting
 * browsing history. It informs the user about the existence of other forms
 * of browsing history in their account.
 */
Polymer({
  is: 'settings-history-deletion-dialog',

  /** @override */
  attached: function() {
    this.$.dialog.showModal();
  },

  /**
   * Tap handler for the "OK" button.
   * @private
   */
  onOkTap_: function() {
    this.$.dialog.close();
  },
});
