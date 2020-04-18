// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Controller to handle naming.
 *
 * @param {!ListContainer} listContainer
 * @param {!cr.ui.dialogs.AlertDialog} alertDialog
 * @param {!cr.ui.dialogs.ConfirmDialog} confirmDialog
 * @param {!DirectoryModel} directoryModel
 * @param {!FileFilter} fileFilter
 * @param {!FileSelectionHandler} selectionHandler
 * @constructor
 * @struct
 */
function NamingController(
    listContainer, alertDialog, confirmDialog, directoryModel, fileFilter,
    selectionHandler) {
  /**
   * @type {!ListContainer}
   * @const
   * @private
   */
  this.listContainer_ = listContainer;

  /**
   * @type {!cr.ui.dialogs.AlertDialog}
   * @const
   * @private
   */
  this.alertDialog_ = alertDialog;

 /**
   * @type {!cr.ui.dialogs.ConfirmDialog}
   * @const
   * @private
   */
  this.confirmDialog_ = confirmDialog;

  /**
   * @type {!DirectoryModel}
   * @const
   * @private
   */
  this.directoryModel_ = directoryModel;

  /**
   * @type {!FileFilter}
   * @const
   * @private
   */
  this.fileFilter_ = fileFilter;

  /**
   * @type {!FileSelectionHandler}
   * @const
   * @private
   */
  this.selectionHandler_ = selectionHandler;

  // Register events.
  this.listContainer_.renameInput.addEventListener(
      'keydown', this.onRenameInputKeyDown_.bind(this));
  this.listContainer_.renameInput.addEventListener(
      'blur', this.onRenameInputBlur_.bind(this));
}

/**
 * Verifies the user entered name for file or folder to be created or
 * renamed to. See also util.validateFileName.
 *
 * @param {!DirectoryEntry} parentEntry The URL of the parent directory entry.
 * @param {string} name New file or folder name.
 * @param {function(boolean)} onDone Function to invoke when user closes the
 *    warning box or immediatelly if file name is correct. If the name was
 *    valid it is passed true, and false otherwise.
 */
NamingController.prototype.validateFileName = function(
    parentEntry, name, onDone) {
  var fileNameErrorPromise = util.validateFileName(
      parentEntry,
      name,
      this.fileFilter_.isFilterHiddenOn());
  fileNameErrorPromise.then(onDone.bind(null, true), function(message) {
    this.alertDialog_.show(message, onDone.bind(null, false));
  }.bind(this)).catch(function(error) {
    console.error(error.stack || error);
  });
};

/**
 * @param {string} filename
 * @return {Promise<string>}
 */
NamingController.prototype.validateFileNameForSaving = function(filename) {
  var directory = /** @type {DirectoryEntry} */ (
      this.directoryModel_.getCurrentDirEntry());
  var currentDirUrl = directory.toURL().replace(/\/?$/, '/');
  var fileUrl = currentDirUrl + encodeURIComponent(filename);

  return new Promise(this.validateFileName.bind(this, directory, filename)).
      then(function(isValid) {
        if (!isValid)
          return Promise.reject('Invalid filename.');

        if (directory && util.isFakeEntry(directory)) {
          // Can't save a file into a fake directory.
          return Promise.reject('Cannot save into fake entry.');
        }

        return new Promise(
            directory.getFile.bind(directory, filename, {create: false}));
      }).then(function() {
        // An existing file is found. Show confirmation dialog to
        // overwrite it. If the user select "OK" on the dialog, save it.
        return new Promise(function(fulfill, reject) {
          this.confirmDialog_.show(
              strf('CONFIRM_OVERWRITE_FILE', filename),
              fulfill.bind(null, fileUrl),
              reject.bind(null, 'Cancelled'),
              function() {});
        }.bind(this));
      }.bind(this), function(error) {
        if (error.name == util.FileError.NOT_FOUND_ERR) {
          // The file does not exist, so it should be ok to create a
          // new file.
          return fileUrl;
        }

        if (error.name == util.FileError.TYPE_MISMATCH_ERR) {
          // An directory is found.
          // Do not allow to overwrite directory.
          this.alertDialog_.show(strf('DIRECTORY_ALREADY_EXISTS', filename));
          return Promise.reject(error);
        }

        // Unexpected error.
        console.error('File save failed: ' + error.code);
        return Promise.reject(error);
      }.bind(this));
};

/**
 * @return {boolean}
 */
NamingController.prototype.isRenamingInProgress = function() {
  return !!this.listContainer_.renameInput.currentEntry;
};

NamingController.prototype.initiateRename = function() {
  var item = this.listContainer_.currentList.ensureLeadItemExists();
  if (!item)
    return;
  var label = item.querySelector('.filename-label');
  var input = this.listContainer_.renameInput;
  var currentEntry =
      this.listContainer_.currentList.dataModel.item(item.listIndex);

  input.value = label.textContent;
  item.setAttribute('renaming', '');
  label.parentNode.appendChild(input);
  input.focus();

  var selectionEnd = input.value.lastIndexOf('.');
  if (currentEntry.isFile && selectionEnd !== -1) {
    input.selectionStart = 0;
    input.selectionEnd = selectionEnd;
  } else {
    input.select();
  }

  // This has to be set late in the process so we don't handle spurious
  // blur events.
  input.currentEntry = currentEntry;
  this.listContainer_.startBatchUpdates();
};

/**
 * Restores the item which is being renamed while refreshing the file list. Do
 * nothing if no item is being renamed or such an item disappeared.
 *
 * While refreshing file list it gets repopulated with new file entries.
 * There is not a big difference whether DOM items stay the same or not.
 * Except for the item that the user is renaming.
 */
NamingController.prototype.restoreItemBeingRenamed = function() {
  if (!this.isRenamingInProgress())
    return;

  var dm = this.directoryModel_;
  var leadIndex = dm.getFileListSelection().leadIndex;
  if (leadIndex < 0)
    return;

  var leadEntry = /** @type {Entry} */ (dm.getFileList().item(leadIndex));
  if (!util.isSameEntry(
          this.listContainer_.renameInput.currentEntry, leadEntry)) {
    return;
  }

  var leadListItem = this.listContainer_.findListItemForNode(
      this.listContainer_.renameInput);
  if (this.listContainer_.currentListType == ListContainer.ListType.DETAIL) {
    this.listContainer_.table.updateFileMetadata(leadListItem, leadEntry);
  }
  this.listContainer_.currentList.restoreLeadItem(leadListItem);
};

/**
 * @param {Event} event Key event.
 * @private
 */
NamingController.prototype.onRenameInputKeyDown_ = function(event) {
  // Ignore key events if event.keyCode is VK_PROCESSKEY(229).
  // TODO(fukino): Remove this workaround once crbug.com/644140 is fixed.
  if (event.keyCode === 229)
    return;

  if (!this.isRenamingInProgress())
    return;

  // Do not move selection or lead item in list during rename.
  if (event.key === 'ArrowUp' || event.key === 'ArrowDown') {
    event.stopPropagation();
  }

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

/**
 * @param {Event} event Blur event.
 * @private
 */
NamingController.prototype.onRenameInputBlur_ = function(event) {
  if (this.isRenamingInProgress() &&
      !this.listContainer_.renameInput.validation_) {
    this.commitRename_();
  }
};

/**
 * @private
 */
NamingController.prototype.commitRename_ = function() {
  var input = this.listContainer_.renameInput;
  var entry = input.currentEntry;
  var newName = input.value;

  if (!newName || newName == entry.name) {
    this.cancelRename_();
    return;
  }

  var renamedItemElement = this.listContainer_.findListItemForNode(
      this.listContainer_.renameInput);
  var nameNode = renamedItemElement.querySelector('.filename-label');

  input.validation_ = true;
  var validationDone = function(valid) {
    input.validation_ = false;

    if (!valid) {
      // Cancel rename if it fails to restore focus from alert dialog.
      // Otherwise, just cancel the commitment and continue to rename.
      if (document.activeElement != input)
        this.cancelRename_();
      return;
    }

    // Validation succeeded. Do renaming.
    this.listContainer_.renameInput.currentEntry = null;
    if (this.listContainer_.renameInput.parentNode) {
      this.listContainer_.renameInput.parentNode.removeChild(
          this.listContainer_.renameInput);
    }
    renamedItemElement.setAttribute('renaming', 'provisional');

    // Optimistically apply new name immediately to avoid flickering in
    // case of success.
    nameNode.textContent = newName;

    util.rename(
        entry, newName,
        function(newEntry) {
          this.directoryModel_.onRenameEntry(entry, newEntry, function() {
            // Select new entry.
            this.listContainer_.currentList.selectionModel.selectedIndex =
                this.directoryModel_.getFileList().indexOf(newEntry);
            // Force to update selection immediately.
            this.selectionHandler_.onFileSelectionChanged();

            renamedItemElement.removeAttribute('renaming');
            this.listContainer_.endBatchUpdates();

            // Focus may go out of the list. Back it to the list.
            this.listContainer_.currentList.focus();
          }.bind(this));
        }.bind(this),
        function(error) {
          // Write back to the old name.
          nameNode.textContent = entry.name;
          renamedItemElement.removeAttribute('renaming');
          this.listContainer_.endBatchUpdates();

          // Show error dialog.
          var message = util.getRenameErrorMessage(error, entry, newName);
          this.alertDialog_.show(message);
        }.bind(this));
  }.bind(this);

  // TODO(mtomasz): this.getCurrentDirectoryEntry() might not return the actual
  // parent if the directory content is a search result. Fix it to do proper
  // validation.
  this.validateFileName(
      /** @type {!DirectoryEntry} */ (this.directoryModel_.getCurrentDirEntry()),
      newName,
      validationDone.bind(this));
};

/**
 * @private
 */
NamingController.prototype.cancelRename_ = function() {
  this.listContainer_.renameInput.currentEntry = null;

  var item = this.listContainer_.findListItemForNode(
      this.listContainer_.renameInput);
  if (item)
    item.removeAttribute('renaming');

  var parent = this.listContainer_.renameInput.parentNode;
  if (parent)
    parent.removeChild(this.listContainer_.renameInput);

  this.listContainer_.endBatchUpdates();

  // Focus may go out of the list. Back it to the list.
  this.listContainer_.currentList.focus();
};
