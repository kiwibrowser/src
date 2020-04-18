// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Controler for handling behaviors of the Files app opened as a file/folder
 * selection dialog.
 *
 * @param {DialogType} dialogType Dialog type.
 * @param {!DialogFooter} dialogFooter Dialog footer.
 * @param {!DirectoryModel} directoryModel Directory model.
 * @param {!MetadataModel} metadataModel Metadata cache.
 * @param {!VolumeManagerWrapper} volumeManager Volume manager.
 * @param {!FileFilter} fileFilter File filter model.
 * @param {!NamingController} namingController Naming controller.
 * @param {!FileSelectionHandler} fileSelectionHandler Initial file selection.
 * @param {!LaunchParam} launchParam Whether the dialog should return local
 *     path or not.
 * @constructor
 * @struct
 */
function DialogActionController(
    dialogType,
    dialogFooter,
    directoryModel,
    metadataModel,
    volumeManager,
    fileFilter,
    namingController,
    fileSelectionHandler,
    launchParam) {
  /**
   * @type {!DialogType}
   * @const
   * @private
   */
  this.dialogType_ = dialogType;

  /**
   * @type {!DialogFooter}
   * @const
   * @private
   */
  this.dialogFooter_ = dialogFooter;

  /**
   * @type {!DirectoryModel}
   * @const
   * @private
   */
  this.directoryModel_ = directoryModel;

  /**
   * @type {!MetadataModel}
   * @const
   * @private
   */
  this.metadataModel_ = metadataModel;

  /**
   * @type {!VolumeManagerWrapper}
   * @const
   * @private
   */
  this.volumeManager_ = volumeManager;

  /**
   * @type {!FileFilter}
   * @const
   * @private
   */
  this.fileFilter_ = fileFilter;

  /**
   * @type {!NamingController}
   * @const
   * @private
   */
  this.namingController_ = namingController;

  /**
   * @type {!FileSelectionHandler}
   * @private
   * @const
   */
  this.fileSelectionHandler_ = fileSelectionHandler;

  /**
   * List of acceptable file types for open dialog.
   * @type {!Array<Object>}
   * @const
   * @private
   */
  this.fileTypes_ = launchParam.typeList || [];

  /**
   * @type {!AllowedPaths}
   * @const
   * @private
   */
  this.allowedPaths_ = launchParam.allowedPaths;

  /**
   * Bound function for onCancel_.
   * @type {!function(this:DialogActionController, Event)}
   * @private
   */
  this.onCancelBound_ = this.processCancelAction_.bind(this);

  dialogFooter.okButton.addEventListener(
      'click', this.processOKAction_.bind(this));
  dialogFooter.cancelButton.addEventListener(
      'click', this.onCancelBound_);
  dialogFooter.newFolderButton.addEventListener(
      'click', this.processNewFolderAction_.bind(this));
  dialogFooter.fileTypeSelector.addEventListener(
      'change', this.onFileTypeFilterChanged_.bind(this));
  dialogFooter.filenameInput.addEventListener(
      'input', this.updateOkButton_.bind(this));
  fileSelectionHandler.addEventListener(
      FileSelectionHandler.EventType.CHANGE_THROTTLED,
      this.onFileSelectionChanged_.bind(this));

  dialogFooter.initFileTypeFilter(
      this.fileTypes_, launchParam.includeAllFiles);
  this.onFileTypeFilterChanged_();

  this.newFolderCommand_ =
      /** @type {cr.ui.Command} */ (document.getElementById('new-folder'));
  this.newFolderCommand_.addEventListener(
      'disabledChange', this.updateNewFolderButton_.bind(this));
}

/**
 * @private
 */
DialogActionController.prototype.processOKActionForSaveDialog_ = function() {
  var selection = this.fileSelectionHandler_.selection;

  // If OK action is clicked when a directory is selected, open the directory.
  if (selection.directoryCount === 1 && selection.fileCount === 0) {
    this.directoryModel_.changeDirectoryEntry(
        /** @type {!DirectoryEntry} */ (selection.entries[0]));
    return;
  }

  // Save-as doesn't require a valid selection from the list, since
  // we're going to take the filename from the text input.
  var filename = this.dialogFooter_.filenameInput.value;
  if (!filename)
    throw new Error('Missing filename!');

  this.namingController_.validateFileNameForSaving(filename).then(
      function(url) {
        // TODO(mtomasz): Clean this up by avoiding constructing a URL
        //                via string concatenation.
        this.selectFilesAndClose_({
          urls: [url],
          multiple: false,
          filterIndex: this.dialogFooter_.selectedFilterIndex
        });
      }.bind(this)).catch(function(error) {
        if (error instanceof Error)
          console.error(error.stack && error);
      });
};

/**
 * Handle a click of the ok button.
 *
 * The ok button has different UI labels depending on the type of dialog, but
 * in code it's always referred to as 'ok'.
 *
 * @private
 */
DialogActionController.prototype.processOKAction_ = function() {
  if (this.dialogType_ === DialogType.SELECT_SAVEAS_FILE) {
    this.processOKActionForSaveDialog_();
    return;
  }

  var files = [];
  var selectedIndexes =
      this.directoryModel_.getFileListSelection().selectedIndexes;

  if (DialogType.isFolderDialog(this.dialogType_) &&
      selectedIndexes.length === 0) {
    var url = this.directoryModel_.getCurrentDirEntry().toURL();
    var singleSelection = {
      urls: [url],
      multiple: false,
      filterIndex: this.dialogFooter_.selectedFilterIndex
    };
    this.selectFilesAndClose_(singleSelection);
    return;
  }

  // All other dialog types require at least one selected list item.
  // The logic to control whether or not the ok button is enabled should
  // prevent us from ever getting here, but we sanity check to be sure.
  if (!selectedIndexes.length)
    throw new Error('Nothing selected!');

  var dm = this.directoryModel_.getFileList();
  for (var i = 0; i < selectedIndexes.length; i++) {
    var entry = dm.item(selectedIndexes[i]);
    if (!entry) {
      console.error('Error locating selected file at index: ' + i);
      continue;
    }

    files.push(entry.toURL());
  }

  // Multi-file selection has no other restrictions.
  if (this.dialogType_ === DialogType.SELECT_OPEN_MULTI_FILE) {
    var multipleSelection = {
      urls: files,
      multiple: true
    };
    this.selectFilesAndClose_(multipleSelection);
    return;
  }

  // Everything else must have exactly one.
  if (files.length > 1)
    throw new Error('Too many files selected!');

  var selectedEntry = dm.item(selectedIndexes[0]);

  if (DialogType.isFolderDialog(this.dialogType_)) {
    if (!selectedEntry.isDirectory)
      throw new Error('Selected entry is not a folder!');
  } else if (this.dialogType_ === DialogType.SELECT_OPEN_FILE) {
    if (!selectedEntry.isFile)
      throw new Error('Selected entry is not a file!');
  }

  var singleSelection = {
    urls: [files[0]],
    multiple: false,
    filterIndex: this.dialogFooter_.selectedFilterIndex
  };
  this.selectFilesAndClose_(singleSelection);
};

/**
 * Cancels file selection and closes the file selection dialog.
 * @private
 */
DialogActionController.prototype.processCancelAction_ = function() {
  chrome.fileManagerPrivate.cancelDialog();
  window.close();
};

/**
 * Creates a new folder using new-folder command.
 * @private
 */
DialogActionController.prototype.processNewFolderAction_ = function() {
  this.newFolderCommand_.canExecuteChange(this.dialogFooter_.newFolderButton);
  this.newFolderCommand_.execute(this.dialogFooter_.newFolderButton);
};

/**
 * Handles disabledChange event to update the new-folder button's avaliability.
 * @private
 */
DialogActionController.prototype.updateNewFolderButton_ = function() {
  this.dialogFooter_.newFolderButton.disabled = this.newFolderCommand_.disabled;
};

/**
 * Tries to close this modal dialog with some files selected.
 * Performs preprocessing if needed (e.g. for Drive).
 * @param {Object} selection Contains urls, filterIndex and multiple fields.
 * @private
 */
DialogActionController.prototype.selectFilesAndClose_ = function(selection) {
  var currentRootType = this.directoryModel_.getCurrentRootType();
  var callSelectFilesApiAndClose = function(callback) {
    var onFileSelected = function() {
      callback();
      if (!chrome.runtime.lastError) {
        // Call next method on a timeout, as it's unsafe to
        // close a window from a callback.
        setTimeout(window.close.bind(window), 0);
      }
    };
    // Record the root types of chosen files in OPEN dialog.
    if (this.dialogType_ == DialogType.SELECT_OPEN_FILE ||
        this.dialogType_ == DialogType.SELECT_OPEN_MULTI_FILE) {
      metrics.recordEnum(
          'OpenFiles.RootType', currentRootType,
          VolumeManagerCommon.RootTypesForUMA);
    }
    if (selection.multiple) {
      chrome.fileManagerPrivate.selectFiles(
          selection.urls,
          this.allowedPaths_ === AllowedPaths.NATIVE_PATH,
          onFileSelected);
    } else {
      chrome.fileManagerPrivate.selectFile(
          selection.urls[0],
          selection.filterIndex,
          this.dialogType_ !== DialogType.SELECT_SAVEAS_FILE /* for opening */,
          this.allowedPaths_ === AllowedPaths.NATIVE_PATH,
          onFileSelected);
    }
  }.bind(this);

  if (currentRootType !== VolumeManagerCommon.VolumeType.DRIVE ||
      this.dialogType_ === DialogType.SELECT_SAVEAS_FILE) {
    callSelectFilesApiAndClose(function() {});
    return;
  }

  var shade = document.createElement('div');
  shade.className = 'shade';
  var footer = this.dialogFooter_.element;
  var progress = footer.querySelector('.progress-track');
  progress.style.width = '0%';
  var cancelled = false;

  var progressMap = {};
  var filesStarted = 0;
  var filesTotal = selection.urls.length;
  for (var index = 0; index < selection.urls.length; index++) {
    progressMap[selection.urls[index]] = -1;
  }
  var lastPercent = 0;
  var bytesTotal = 0;
  var bytesDone = 0;

  var onFileTransfersUpdated = function(status) {
    if (!(status.fileUrl in progressMap))
      return;
    if (status.total === -1)
      return;

    var old = progressMap[status.fileUrl];
    if (old === -1) {
      // -1 means we don't know file size yet.
      bytesTotal += status.total;
      filesStarted++;
      old = 0;
    }
    bytesDone += status.processed - old;
    progressMap[status.fileUrl] = status.processed;

    var percent = bytesTotal === 0 ? 0 : bytesDone / bytesTotal;
    // For files we don't have information about, assume the progress is zero.
    percent = percent * filesStarted / filesTotal * 100;
    // Do not decrease the progress. This may happen, if first downloaded
    // file is small, and the second one is large.
    lastPercent = Math.max(lastPercent, percent);
    progress.style.width = lastPercent + '%';
  }.bind(this);

  var setup = function() {
    document.querySelector('.dialog-container').appendChild(shade);
    setTimeout(function() { shade.setAttribute('fadein', 'fadein'); }, 100);
    footer.setAttribute('progress', 'progress');
    this.dialogFooter_.cancelButton.removeEventListener(
        'click', this.onCancelBound_);
    this.dialogFooter_.cancelButton.addEventListener('click', onCancel);
    chrome.fileManagerPrivate.onFileTransfersUpdated.addListener(
        onFileTransfersUpdated);
  }.bind(this);

  var cleanup = function() {
    shade.parentNode.removeChild(shade);
    footer.removeAttribute('progress');
    this.dialogFooter_.cancelButton.removeEventListener('click', onCancel);
    this.dialogFooter_.cancelButton.addEventListener(
        'click', this.onCancelBound_);
    chrome.fileManagerPrivate.onFileTransfersUpdated.removeListener(
        onFileTransfersUpdated);
  }.bind(this);

  var onCancel = function() {
    // According to API cancel may fail, but there is no proper UI to reflect
    // this. So, we just silently assume that everything is cancelled.
    util.URLsToEntries(selection.urls).then(function(entries) {
      chrome.fileManagerPrivate.cancelFileTransfers(
          entries, util.checkAPIError);
    });
  }.bind(this);

  var onProperties = function(properties) {
    for (var i = 0; i < properties.length; i++) {
      if (properties[i].present) {
        // For files already in GCache, we don't get any transfer updates.
        filesTotal--;
      }
    }
    callSelectFilesApiAndClose(cleanup);
  }.bind(this);

  setup();

  // TODO(mtomasz): Use Entry instead of URLs, if possible.
  util.URLsToEntries(selection.urls, function(entries) {
    this.metadataModel_.get(entries, ['present']).then(onProperties);
  }.bind(this));
};

/**
 * Filters file according to the selected file type.
 * @private
 */
DialogActionController.prototype.onFileTypeFilterChanged_ = function() {
  this.fileFilter_.removeFilter('fileType');
  var selectedIndex = this.dialogFooter_.selectedFilterIndex;
  if (selectedIndex > 0) { // Specific filter selected.
    var regexp = new RegExp('\\.(' +
        this.fileTypes_[selectedIndex - 1].extensions.join('|') + ')$', 'i');
    var filter = function(entry) {
      return entry.isDirectory || regexp.test(entry.name);
    };
    this.fileFilter_.addFilter('fileType', filter);

    // In save dialog, update the destination name extension.
    if (this.dialogType_ === DialogType.SELECT_SAVEAS_FILE) {
      var current = this.dialogFooter_.filenameInput.value;
      var newExt = this.fileTypes_[selectedIndex - 1].extensions[0];
      if (newExt && !regexp.test(current)) {
        var i = current.lastIndexOf('.');
        if (i >= 0) {
          this.dialogFooter_.filenameInput.value =
              current.substr(0, i) + '.' + newExt;
          this.dialogFooter_.selectTargetNameInFilenameInput();
        }
      }
    }
  }
};

/**
 * Handles selection change.
 *
 * @private
 */
DialogActionController.prototype.onFileSelectionChanged_ = function() {
  // If this is a save-as dialog, copy the selected file into the filename
  // input text box.
  var selection = this.fileSelectionHandler_.selection;
  if (this.dialogType_ === DialogType.SELECT_SAVEAS_FILE &&
      selection.totalCount === 1 &&
      selection.entries[0].isFile &&
      this.dialogFooter_.filenameInput.value !== selection.entries[0].name) {
    this.dialogFooter_.filenameInput.value = selection.entries[0].name;
  }

  this.updateOkButton_();
  if (!this.dialogFooter_.okButton.disabled)
    util.testSendMessage('dialog-ready');
};

/**
 * Updates the Ok button enabled state.
 * @private
 */
DialogActionController.prototype.updateOkButton_ = function() {
  var selection = this.fileSelectionHandler_.selection;

  if (this.dialogType_ === DialogType.FULL_PAGE) {
    // No "select" buttons on the full page UI.
    this.dialogFooter_.okButton.disabled = false;
    return;
  }

  if (DialogType.isFolderDialog(this.dialogType_)) {
    // In SELECT_FOLDER mode, we allow to select current directory
    // when nothing is selected.
    this.dialogFooter_.okButton.disabled =
        selection.directoryCount > 1 || selection.fileCount !== 0;
    return;
  }

  if (this.dialogType_ === DialogType.SELECT_SAVEAS_FILE) {
    if (selection.directoryCount === 1 && selection.fileCount === 0) {
      this.dialogFooter_.okButtonLabel.textContent = str('OPEN_LABEL');
      this.dialogFooter_.okButton.disabled = false;
    } else {
      this.dialogFooter_.okButtonLabel.textContent = str('SAVE_LABEL');
      this.dialogFooter_.okButton.disabled =
          this.directoryModel_.isReadOnly() ||
          !this.dialogFooter_.filenameInput.value;
    }
    return;
  }

  if (this.dialogType_ === DialogType.SELECT_OPEN_FILE) {
    this.dialogFooter_.okButton.disabled =
        selection.directoryCount !== 0 ||
        selection.fileCount !== 1 ||
        !this.fileSelectionHandler_.isAvailable();
    return;
  }

  if (this.dialogType_ === DialogType.SELECT_OPEN_MULTI_FILE) {
    this.dialogFooter_.okButton.disabled =
        selection.directoryCount !== 0 ||
        selection.fileCount === 0 ||
        !this.fileSelectionHandler_.isAvailable();
    return;
  }

  assertNotReached('Unknown dialog type.');
};
