// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview 'password-edit-dialog' is the dialog that allows showing a
 *     saved password.
 */

(function() {
'use strict';

Polymer({
  is: 'password-edit-dialog',

  behaviors: [ShowPasswordBehavior],

  /** @override */
  attached: function() {
    this.$.dialog.showModal();
  },

  /** Closes the dialog. */
  close: function() {
    this.$.dialog.close();
  },

  /**
   * Handler for tapping the 'done' button. Should just dismiss the dialog.
   * @private
   */
  onActionButtonTap_: function() {
    this.close();
  },

  /**
   * @param {!Event} event
   * @private
   */
  onInputFocus_: function(event) {
    const inputElement =
        /** @type {!PaperInputElement} */ (Polymer.dom(event).localTarget)
            .inputElement;
    inputElement.setSelectionRange(0, 0);
    inputElement.focus();
    inputElement.select();
  }
});
})();
