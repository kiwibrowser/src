// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Quick view model that doesn't fit into properties of quick view element.
 *
 * @constructor
 * @struct
 * @extends {cr.EventTarget}
 */
function QuickViewModel() {
  /**
   * Current selected file entry.
   * @type {FileEntry}
   * @private
   */
  this.selectedEntry_ = null;
}

/**
 * QuickViewModel extends cr.EventTarget.
 */
QuickViewModel.prototype.__proto__ = cr.EventTarget.prototype;

/** @return {FileEntry} */
QuickViewModel.prototype.getSelectedEntry = function() {
  return this.selectedEntry_;
};

/**
 * @param {!FileEntry} entry
 */
QuickViewModel.prototype.setSelectedEntry = function(entry) {
  this.selectedEntry_ = entry;
  cr.dispatchSimpleEvent(this, 'selected-entry-changed');
};
