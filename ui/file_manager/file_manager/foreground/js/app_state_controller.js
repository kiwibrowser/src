// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @param {DialogType} dialogType
 * @constructor
 * @struct
 */
function AppStateController(dialogType) {
  /**
   * @const {string}
   * @private
   */
  this.viewOptionStorageKey_ = 'file-manager-' + dialogType;

  /** @private {DirectoryModel} */
  this.directoryModel_ = null;

  /**
   * @type {FileManagerUI}
   * @private
   */
  this.ui_ = null;

  /**
   * @type {*}
   * @private
   */
  this.viewOptions_ = null;

  /**
   * Preferred sort field of file list. This will be ignored in the Recent
   * folder, since it always uses descendant order of date-mofidied.
   * @private {string}
   */
  this.fileListSortField_ = AppStateController.DEFAULT_SORT_FIELD;

  /**
   * Preferred sort direction of file list. This will be ignored in the Recent
   * folder, since it always uses descendant order of date-mofidied.
   * @private {string}
   */
  this.fileListSortDirection_ = AppStateController.DEFAULT_SORT_DIRECTION;
};

/**
 * Default sort field of the file list.
 * @const {string}
 */
AppStateController.DEFAULT_SORT_FIELD = 'modificationTime';

/**
 * Default sort direction of the file list.
 * @const {string}
 */
AppStateController.DEFAULT_SORT_DIRECTION = 'desc';

/**
 * @return {Promise}
 */
AppStateController.prototype.loadInitialViewOptions = function() {
  // Load initial view option.
  return new Promise(function(fulfill, reject) {
    chrome.storage.local.get(this.viewOptionStorageKey_, function(values) {
      if (chrome.runtime.lastError) {
        reject('Failed to load view options: ' +
            chrome.runtime.lastError.message);
      } else {
        fulfill(values);
      }
    });
  }.bind(this)).then(function(values) {
    this.viewOptions_ = {};
    var value = values[this.viewOptionStorageKey_];
    if (!value)
      return;

    // Load the global default options.
    try {
      this.viewOptions_ = JSON.parse(value);
    } catch (ignore) {}

    // Override with window-specific options.
    if (window.appState && window.appState.viewOptions) {
      for (var key in window.appState.viewOptions) {
        if (window.appState.viewOptions.hasOwnProperty(key))
          this.viewOptions_[key] = window.appState.viewOptions[key];
      }
    }
  }.bind(this)).catch(function(error) {
    this.viewOptions_ = {};
    console.error(error);
  }.bind(this));
};

/**
 * @param {!FileManagerUI} ui
 * @param {!DirectoryModel} directoryModel
 */
AppStateController.prototype.initialize = function(ui, directoryModel) {
  assert(this.viewOptions_);

  this.ui_ = ui;
  this.directoryModel_ = directoryModel;

  // Register event listeners.
  ui.listContainer.table.addEventListener(
      'column-resize-end', this.saveViewOptions.bind(this));
  directoryModel.getFileList().addEventListener(
      'sorted', this.onFileListSorted_.bind(this));
  directoryModel.addEventListener(
      'directory-changed', this.onDirectoryChanged_.bind(this));

  // Restore preferences.
  this.ui_.setCurrentListType(
      this.viewOptions_.listType || ListContainer.ListType.DETAIL);
  if (this.viewOptions_.sortField)
    this.fileListSortField_ = this.viewOptions_.sortField;
  if (this.viewOptions_.sortDirection)
    this.fileListSortDirection_ = this.viewOptions_.sortDirection;
  this.directoryModel_.getFileList().sort(
      this.fileListSortField_, this.fileListSortDirection_);
  if (this.viewOptions_.columnConfig) {
    this.ui_.listContainer.table.columnModel.restoreColumnConfig(
        this.viewOptions_.columnConfig);
  }
};

/**
 * Saves current view option.
 */
AppStateController.prototype.saveViewOptions = function() {
  var prefs = {
    sortField: this.fileListSortField_,
    sortDirection: this.fileListSortDirection_,
    columnConfig: {},
    listType: this.ui_.listContainer.currentListType,
  };
  var cm = this.ui_.listContainer.table.columnModel;
  prefs.columnConfig = cm.exportColumnConfig();
  // Save the global default.
  var items = {};
  items[this.viewOptionStorageKey_] = JSON.stringify(prefs);
  chrome.storage.local.set(items, function() {
    if (chrome.runtime.lastError)
      console.error('Failed to save view options: ' +
          chrome.runtime.lastError.message);
  });

  // Save the window-specific preference.
  if (window.appState) {
    window.appState.viewOptions = prefs;
    util.saveAppState();
  }
};

AppStateController.prototype.onFileListSorted_ = function() {
  var currentDirectory = this.directoryModel_.getCurrentDirEntry();
  if (!currentDirectory)
    return;

  // Update preferred sort field and direction only when the current directory
  // is not Recent folder.
  if (!util.isRecentRoot(currentDirectory)) {
    var currentSortStatus = this.directoryModel_.getFileList().sortStatus;
    this.fileListSortField_ = currentSortStatus.field;
    this.fileListSortDirection_ = currentSortStatus.direction;
  }
  this.saveViewOptions();
};

/**
 * @param {Event} event
 * @private
 */
AppStateController.prototype.onDirectoryChanged_ = function(event) {
  if (!event.newDirEntry)
    return;

  // Sort the file list by:
  // 1) 'date-mofidied' and 'desc' order on Recent folder.
  // 2) preferred field and direction on other folders.
  var isOnRecent = util.isRecentRoot(event.newDirEntry);
  var isOnRecentBefore =
      event.previousDirEntry && util.isRecentRoot(event.previousDirEntry);
  if (isOnRecent != isOnRecentBefore) {
    if (isOnRecent) {
      this.directoryModel_.getFileList().sort(
          AppStateController.DEFAULT_SORT_FIELD,
          AppStateController.DEFAULT_SORT_DIRECTION);
    } else {
      this.directoryModel_.getFileList().sort(
          this.fileListSortField_, this.fileListSortDirection_);
    }
  }

  // TODO(mtomasz): Consider remembering the selection.
  util.updateAppState(
      this.directoryModel_.getCurrentDirEntry() ?
          this.directoryModel_.getCurrentDirEntry().toURL() : '',
      '' /* selectionURL */,
      '' /* opt_param */);
};
