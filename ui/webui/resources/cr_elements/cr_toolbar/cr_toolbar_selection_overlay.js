// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Element which displays the number of selected items with
 * Cancel/Delete buttons, designed to be used as an overlay on top of
 * <cr-toolbar>. See <history-toolbar> for an example usage.
 */

Polymer({
  is: 'cr-toolbar-selection-overlay',

  properties: {
    deleteLabel: String,

    cancelLabel: String,

    selectionLabel: String,

    deleteDisabled: Boolean,
  },

  /** @return {PaperButtonElement} */
  get deleteButton() {
    return this.$.delete;
  },

  /** @private */
  onClearSelectionTap_: function() {
    this.fire('clear-selected-items');
  },

  /** @private */
  onDeleteTap_: function() {
    this.fire('delete-selected-items');
  },
});
