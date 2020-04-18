// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Naming controller for directory tree.
 * @param {!DirectoryModel} directoryModel
 * @param {!DirectoryTree} directoryTree
 * @param {!cr.ui.dialogs.AlertDialog} alertDialog
 * @constructor
 * @struct
 */
function DirectoryTreeNamingController(
    directoryModel, directoryTree, alertDialog) {
  /**
   * @private {!DirectoryModel}
   * @const
   */
  this.directoryModel_ = directoryModel;

  /**
   * @private {!DirectoryTree}
   * @const
   */
  this.directoryTree_ = directoryTree;

  /**
   * @private {!cr.ui.dialogs.AlertDialog}
   * @const
   */
  this.alertDialog_ = alertDialog;

  /**
   * @private {DirectoryItem}
   */
  this.currentDirectoryItem_ = null;

  /**
   * @private {boolean}
   */
  this.editting_ = false;

  /**
   * @private {boolean}
   */
  this.isRemovableRoot_ = false;

  /**
   * @private {VolumeInfo}
   */
  this.volumeInfo_ = null;

  /**
   * @private {!HTMLInputElement}
   * @const
   */
  this.inputElement_ = /** @type {!HTMLInputElement} */
      (document.createElement('input'));
  this.inputElement_.type = 'text';
  this.inputElement_.spellcheck = false;
  this.inputElement_.addEventListener('keydown', this.onKeyDown_.bind(this));
  this.inputElement_.addEventListener('blur', this.commitRename_.bind(this));
  this.inputElement_.addEventListener('click', function(event) {
    // Stop propagation of click event to prevent it being captured by directory
    // item and current directory is changed to editing item.
    event.stopPropagation();
  });
}

/**
 * Returns input element.
 * @return {!HTMLInputElement}
 */
DirectoryTreeNamingController.prototype.getInputElement = function() {
  return this.inputElement_;
};

/**
 * Attaches naming controller to specified directory item and start rename.
 * @param {!DirectoryItem} directoryItem An html element of a node of the
 *     target.
 * @param {boolean} isRemovableRoot Indicates whether the target is removable
 *     node or not.
 * @param {VolumeInfo} volumeInfo A volume information about the target node.
 *     |volumeInfo| can be null if method is invoked on a folder that is in the
 *     tree view and is not root of an external drive.
 */
DirectoryTreeNamingController.prototype.attachAndStart = function(
    directoryItem, isRemovableRoot, volumeInfo) {

  this.isRemovableRoot_ = isRemovableRoot;
  this.volumeInfo_ = this.isRemovableRoot_ ? assert(volumeInfo) : null;

  if (!!this.currentDirectoryItem_)
    return;

  this.currentDirectoryItem_ = directoryItem;
  this.currentDirectoryItem_.setAttribute('renaming', true);

  var renameInputElementPlaceholder =
      this.currentDirectoryItem_.firstElementChild.getElementsByClassName(
          'rename-placeholder');

  if (this.isRemovableRoot_ && renameInputElementPlaceholder.length === 1) {
    renameInputElementPlaceholder[0].appendChild(this.inputElement_);
  } else {
    this.currentDirectoryItem_.firstElementChild.appendChild(
        this.inputElement_);
  }

  this.inputElement_.value = this.currentDirectoryItem_.label;
  this.inputElement_.select();
  this.inputElement_.focus();

  this.editting_ = true;
};

/**
 * Commits rename.
 * @private
 */
DirectoryTreeNamingController.prototype.commitRename_ = function() {
  if (!this.editting_)
    return;
  this.editting_ = false;

  var entry = this.currentDirectoryItem_.entry;
  var newName = this.inputElement_.value;

  // If new name is the same as current name or empty (only for removable
  // devices), do nothing.
  if (newName === this.currentDirectoryItem_.label ||
      (newName.length == 0 && this.isRemovableRoot_)) {
    this.detach_();
    return;
  }

  if (this.isRemovableRoot_) {
    // Validate new name.
    util.validateExternalDriveName(newName, assert(this.volumeInfo_))
        .then(
            this.performExternalDriveRename_.bind(this, entry, newName),
            function(errorMessage) {
              this.alertDialog_.show(errorMessage, this.detach_.bind(this));
            }.bind(this));
  } else {
    // Validate new name.
    new Promise(entry.getParent.bind(entry))
        .then(function(parentEntry) {
          return util.validateFileName(
              parentEntry, newName,
              this.directoryModel_.getFileFilter().isFilterHiddenOn());
        }.bind(this))
        .then(
            this.performRename_.bind(this, entry, newName),
            function(errorMessage) {
              this.alertDialog_.show(errorMessage, this.detach_.bind(this));
            }.bind(this));
  }
};

/**
 * Performs rename operation.
 * @param {!DirectoryEntry} entry
 * @param {string} newName Validated name.
 * @private
 */
DirectoryTreeNamingController.prototype.performRename_ = function(
    entry, newName) {
  var renamingCurrentDirectory = util.isSameEntry(entry,
      this.directoryModel_.getCurrentDirEntry());
  if (renamingCurrentDirectory)
    this.directoryModel_.setIgnoringCurrentDirectoryDeletion(true /* ignore */);

  // TODO(yawano): Rename might take time on some volumes. Optimistically show
  // new name in the UI before actual rename is completed.
  new Promise(util.rename.bind(null, entry, newName))
      .then(
          function(newEntry) {
            // Show new name before detaching input element to prevent showing
            // old name.
            var label =
                this.currentDirectoryItem_.firstElementChild.querySelector(
                    '.label');
            label.textContent = newName;

            this.currentDirectoryItem_.entry = newEntry;

            this.detach_();

            // If renamed directory was current directory, change it to new one.
            if (renamingCurrentDirectory) {
              this.directoryModel_.changeDirectoryEntry(
                  newEntry,
                  this.directoryModel_.setIgnoringCurrentDirectoryDeletion.bind(
                      this.directoryModel_, false /* not ignore */));
            }
          }.bind(this),
          function(error) {
            this.directoryModel_.setIgnoringCurrentDirectoryDeletion(
                false /* not ignore*/);
            this.detach_();

            this.alertDialog_.show(
                util.getRenameErrorMessage(error, entry, newName));
          }.bind(this));
};

/**
 * Performs external drive rename operation.
 * @param {!DirectoryEntry} entry
 * @param {string} newName Validated name.
 * @private
 */
DirectoryTreeNamingController.prototype.performExternalDriveRename_ = function(
    entry, newName) {
  // Invoke external drive rename
  chrome.fileManagerPrivate.renameVolume(this.volumeInfo_.volumeId, newName);
  // Show new name before detaching input element to prevent showing old
  // name.
  var label =
      this.currentDirectoryItem_.firstElementChild.querySelector('.label');
  label.textContent = newName;

  this.detach_();
};

/**
 * Cancels rename.
 * @private
 */
DirectoryTreeNamingController.prototype.cancelRename_ = function() {
  if (!this.editting_)
    return;
  this.editting_ = false;

  this.detach_();
};

/**
 * Detaches controller from current directory item.
 * @private
 */
DirectoryTreeNamingController.prototype.detach_ = function() {
  assert(!!this.currentDirectoryItem_);

  var renameInputElementPlaceholder =
      this.currentDirectoryItem_.firstElementChild.getElementsByClassName(
          'rename-placeholder');

  if (this.isRemovableRoot_ && renameInputElementPlaceholder.length === 1) {
    renameInputElementPlaceholder[0].removeChild(this.inputElement_);
  } else {
    this.currentDirectoryItem_.firstElementChild.removeChild(
        this.inputElement_);
  }

  this.currentDirectoryItem_.removeAttribute('renaming');
  this.currentDirectoryItem_ = null;

  // Restore focus to directory tree.
  this.directoryTree_.focus();
};

/**
 * Handles keydown event.
 * @param {!Event} event
 * @private
 */
DirectoryTreeNamingController.prototype.onKeyDown_ = function(event) {
  // Ignore key events if event.keyCode is VK_PROCESSKEY(229).
  // TODO(fukino): Remove this workaround once crbug.com/644140 is fixed.
  if (event.keyCode === 229)
    return;

  event.stopPropagation();

  switch (util.getKeyModifiers(event) + event.key) {
    case 'Escape':
      this.cancelRename_();
      event.preventDefault();
      break;

    case 'Enter':
      this.commitRename_();
      event.preventDefault();
      break;
  }
};
