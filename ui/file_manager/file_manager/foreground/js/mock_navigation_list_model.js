// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Mock class for NavigationListModel.
 * Current implementation of mock class cannot handle shortcut list.
 *
 * @param {VolumeManager} volumeManager A volume manager.
 * @constructor
 * @extends {cr.EventTarget}
 */
function MockNavigationListModel(volumeManager) {
  this.volumeManager_ = volumeManager;
}

/**
 * MockNavigationListModel inherits cr.EventTarget.
 */
MockNavigationListModel.prototype = {
  __proto__: cr.EventTarget.prototype,

  get length() { return this.length_(); }
};

/**
 * Returns the item at the given index.
 * @param {number} index The index of the entry to get.
 * @return {NavigationModelItem} The item at the given index.
 */
MockNavigationListModel.prototype.item = function(index) {
  var volumeInfo = this.volumeManager_.volumeInfoList.item(index);
  return new NavigationModelVolumeItem(volumeInfo.label, volumeInfo);
};

/**
 * Returns the number of items in the model.
 * @return {number} The length of the model.
 * @private
 */
MockNavigationListModel.prototype.length_ = function() {
  return this.volumeManager_.volumeInfoList.length;
};
