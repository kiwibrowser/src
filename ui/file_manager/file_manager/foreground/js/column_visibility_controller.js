// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * A class that controls the visibility of the import status in the main table
 * UI.
 * @param {!FileManagerUI} ui
 * @param {!DirectoryModel} directoryModel
 * @param {!VolumeManagerCommon.VolumeInfoProvider} volumeManager
 * @constructor
 * @struct
 */
function ColumnVisibilityController(ui, directoryModel, volumeManager) {
  /** @private {!DirectoryModel} */
  this.directoryModel_ = directoryModel;

  /** @private {!VolumeManagerCommon.VolumeInfoProvider} */
  this.volumeManager_  = volumeManager;

  /** @private {!FileManagerUI} */
  this.ui_ = ui;

  // Register event listener.
  directoryModel.addEventListener(
      'directory-changed', this.onDirectoryChanged_.bind(this));
};

/**
 * @param {!Event} event
 * @private
 */
ColumnVisibilityController.prototype.onDirectoryChanged_ = function(event) {
  // Enable the status column in import-eligible locations.
  //
  // TODO(kenobi): Once import status is exposed as part of the metadata system,
  // remove this and have the underlying UI determine its own status using
  // metadata.
  var isImportEligible =
      importer.isBeneathMediaDir(event.newDirEntry, this.volumeManager_) &&
      !!this.volumeManager_.getCurrentProfileVolumeInfo(
          VolumeManagerCommon.VolumeType.DRIVE);
  this.ui_.listContainer.table.setImportStatusVisible(isImportEligible);
  this.ui_.listContainer.grid.setImportStatusVisible(isImportEligible);
};
