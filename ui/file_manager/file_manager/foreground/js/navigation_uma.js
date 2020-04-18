// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * UMA exporter for navigation in the Files app.
 *
 * @param {!VolumeManagerWrapper} volumeManager
 *
 * @constructor
 */
function NavigationUma(volumeManager) {
  /**
   * @type {!VolumeManagerWrapper}
   * @private
   */
  this.volumeManager_ = volumeManager;
}

/**
 * Exports file type metric with the given |name|.
 *
 * @param {!FileEntry} entry
 * @param {string} name The histogram name.
 *
 * @private
 */
NavigationUma.prototype.exportRootType_ = function(entry, name) {
  var locationInfo = this.volumeManager_.getLocationInfo(entry);
  if (locationInfo)
    metrics.recordEnum(
        name, locationInfo.rootType, VolumeManagerCommon.RootTypesForUMA);
};

/**
 * Exports UMA based on the entry that has became new current directory.
 *
 * @param {!FileEntry} entry the new directory
 */
NavigationUma.prototype.onDirectoryChanged = function(entry) {
  this.exportRootType_(entry, 'ChangeDirectory.RootType');
};
