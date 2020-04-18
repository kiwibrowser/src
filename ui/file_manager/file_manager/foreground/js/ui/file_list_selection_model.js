// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @param {number=} opt_length The number items in the selection.
 * @constructor
 * @extends {cr.ui.ListSelectionModel}
 * @struct
 */
function FileListSelectionModel(opt_length) {
  cr.ui.ListSelectionModel.call(this, opt_length);

  /** @private {boolean} */
  this.isCheckSelectMode_ = false;

  this.addEventListener('change', this.onChangeEvent_.bind(this));
}

FileListSelectionModel.prototype = /** @struct */ {
  __proto__: cr.ui.ListSelectionModel.prototype
};

/**
 * Updates the check-select mode.
 * @param {boolean} enabled True if check-select mode should be enabled.
 */
FileListSelectionModel.prototype.setCheckSelectMode = function(enabled) {
  this.isCheckSelectMode_ = enabled;
};

/**
 * Gets the check-select mode.
 * @return {boolean} True if check-select mode is enabled.
 */
FileListSelectionModel.prototype.getCheckSelectMode = function() {
  return this.isCheckSelectMode_;
};

/**
 * @override
 * Changes to single-select mode if all selected files get deleted.
 */
FileListSelectionModel.prototype.adjustToReordering = function(permutation) {
  // Look at the old state.
  var oldSelectedItemsCount = this.selectedIndexes.length;
  var oldLeadIndex = this.leadIndex;
  var newSelectedItemsCount =
      this.selectedIndexes.filter(i => permutation[i] != -1).length;
  // Call the superclass function.
  cr.ui.ListSelectionModel.prototype.adjustToReordering.call(this, permutation);
  // Leave check-select mode if all items have been deleted.
  if (oldSelectedItemsCount && !newSelectedItemsCount && this.length_ &&
      oldLeadIndex != -1) {
    this.isCheckSelectMode_ = false;
  }
};

/**
 * Handles change event to update isCheckSelectMode_ BEFORE the change event
 * is dispatched to other listeners.
 * @param {!Event} event Event object of 'change' event.
 * @private
 */
FileListSelectionModel.prototype.onChangeEvent_ = function(event) {
  // When the number of selected item is not one, update che check-select mode.
  // When the number of selected item is one, the mode depends on the last
  // keyboard/mouse operation. In this case, the mode is controlled from
  // outside. See filelist.handlePointerDownUp and filelist.handleKeyDown.
  var selectedIndexes = this.selectedIndexes;
  if (selectedIndexes.length === 0) {
    this.isCheckSelectMode_ = false;
  } else if (selectedIndexes.length >= 2) {
    this.isCheckSelectMode_ = true;
  }
};

/**
 * @param {number=} opt_length The number items in the selection.
 * @constructor
 * @extends {cr.ui.ListSingleSelectionModel}
 * @struct
 */
function FileListSingleSelectionModel(opt_length) {
  cr.ui.ListSingleSelectionModel.call(this, opt_length);
}

FileListSingleSelectionModel.prototype = /** @struct */ {
  __proto__: cr.ui.ListSingleSelectionModel.prototype
};

/**
 * Updates the check-select mode.
 * @param {boolean} enabled True if check-select mode should be enabled.
 */
FileListSingleSelectionModel.prototype.setCheckSelectMode = function(enabled) {
  // Do nothing, as check-select mode is invalid in single selection model.
};

/**
 * Gets the check-select mode.
 * @return {boolean} True if check-select mode is enabled.
 */
FileListSingleSelectionModel.prototype.getCheckSelectMode = function() {
  return false;
};
