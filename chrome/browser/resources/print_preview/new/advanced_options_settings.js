// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'print-preview-advanced-options-settings',

  behaviors: [SettingsBehavior],

  properties: {
    disabled: Boolean,

    /** @type {!print_preview.Destination} */
    destination: Object,
  },

  /** @private */
  onButtonClick_: function() {
    const dialog = this.$.advancedDialog.get();
    // This async() call is a workaround to prevent a DCHECK - see
    // https://crbug.com/804047.
    this.async(() => {
      dialog.show();
    }, 1);
  },
});
