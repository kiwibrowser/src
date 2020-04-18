// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Controller for searching.
 * @param {!SearchBox} searchBox Search box UI element.
 * @param {!LocationLine} locationLine Location line UI element.
 * @param {!DirectoryModel} directoryModel Directory model.
 * @param {!TaskController} taskController Task controller to execute the
 *     selected item.
 * @constructor
 */
function SearchController(
    searchBox, locationLine, directoryModel, volumeManager, taskController) {
  /**
   * @type {SearchBox}
   * @private
   */
  this.searchBox_ = searchBox;

  /**
   * @type {LocationLine}
   * @private
   */
  this.locationLine_ = locationLine;

  /**
   * @type {DirectoryModel}
   * @private
   */
  this.directoryModel_ = directoryModel;

  /**
   * @type {VolumeManager}
   * @private
   */
  this.volumeManager_ = volumeManager;

  /**
   * @type {!TaskController}
   * @private
   */
  this.taskController_ = taskController;

  searchBox.addEventListener(
      SearchBox.EventType.TEXT_CHANGE, this.onTextChange_.bind(this));
  searchBox.addEventListener(
      SearchBox.EventType.ITEM_SELECT, this.onItemSelect_.bind(this));
  directoryModel.addEventListener('directory-changed', this.clear.bind(this));
}

SearchController.prototype = {
  /**
   * Obtains current directory's locaiton info.
   * @type {EntryLocation}
   * @private
   */
  get currentLocationInfo_() {
    var entry = this.directoryModel_.getCurrentDirEntry();
    return entry && this.volumeManager_.getLocationInfo(entry);
  },

  /**
   * Whether the current directory is on drive or not.
   * @private
   */
  get isOnDrive_() {
    var currentLocationInfo = this.currentLocationInfo_;
    return currentLocationInfo && currentLocationInfo.isDriveBased;
  }
};

/**
 * Clears the search state.
 */
SearchController.prototype.clear = function() {
  this.directoryModel_.clearLastSearchQuery();
  this.searchBox_.clear();
};

/**
 * Handles text change event.
 * @private
 */
SearchController.prototype.onTextChange_ = function() {
  var searchString = this.searchBox_.inputElement.value.trimLeft();

  // On drive, incremental search is not invoked since we have an auto-
  // complete suggestion instead.
  if (!this.isOnDrive_) {
    this.search_(searchString);
    return;
  }

  // When the search text is changed, finishes the search and showes back
  // the last directory by passing an empty string to
  // {@code DirectoryModel.search()}.
  if (this.directoryModel_.isSearching() &&
      this.directoryModel_.getLastSearchQuery() != searchString) {
    this.directoryModel_.search('', function() {}, function() {});
  }

  this.requestAutocompleteSuggestions_();
};

/**
 * Updates autocompletion items.
 * @private
 */
SearchController.prototype.requestAutocompleteSuggestions_ = function() {
  // Remember the most recent query. If there is an other request in progress,
  // then it's result will be discarded and it will call a new request for
  // this query.
  var searchString = this.searchBox_.inputElement.value.trimLeft();
  this.lastAutocompleteQuery_ = searchString;
  if (this.autocompleteSuggestionsBusy_)
    return;

  // Clear search if the query empty.
  if (!searchString) {
    this.searchBox_.autocompleteList.suggestions = [];
    return;
  }

  // Add header item.
  var headerItem = /** @type {SearchItem} */ (
      {isHeaderItem: true, searchQuery: searchString});
  if (!this.searchBox_.autocompleteList.dataModel ||
      this.searchBox_.autocompleteList.dataModel.length == 0) {
    this.searchBox_.autocompleteList.suggestions = [headerItem];
  } else {
    // Updates only the head item to prevent a flickering on typing.
    this.searchBox_.autocompleteList.dataModel.splice(0, 1, headerItem);
  }

  // The autocomplete list should be resized and repositioned here as the
  // search box is resized when it's focused.
  this.searchBox_.autocompleteList.syncWidthAndPositionToInput();
  this.autocompleteSuggestionsBusy_ = true;

  chrome.fileManagerPrivate.searchDriveMetadata(
      {
        query: searchString,
        types: 'ALL',
        maxResults: 4
      },
      function(suggestions) {
        this.autocompleteSuggestionsBusy_ = false;

        // Discard results for previous requests and fire a new search
        // for the most recent query.
        if (searchString != this.lastAutocompleteQuery_) {
          this.requestAutocompleteSuggestions_();
          return;
        }

        // Keeps the items in the suggestion list.
        this.searchBox_.autocompleteList.suggestions =
            [headerItem].concat(suggestions);
      }.bind(this));
};

/**
 * Opens the currently selected suggestion item.
 * @private
 */
SearchController.prototype.onItemSelect_ = function() {
  var selectedItem = this.searchBox_.autocompleteList.selectedItem;

  // Clear the current auto complete list.
  this.lastAutocompleteQuery_ = '';
  this.searchBox_.autocompleteList.suggestions = [];

  // If the entry is the search item or no entry is selected, just change to
  // the search result.
  if (!selectedItem || selectedItem.isHeaderItem) {
    var query = selectedItem ?
        selectedItem.searchQuery : this.searchBox_.inputElement.value;
    this.search_(query);
    return;
  }

  // Clear the search box if an item except for the search item is
  // selected. Eventually the following directory change clears the search box,
  // but if the selected item is located just under /drive/other, the current
  // directory will not changed. For handling the case, and for improving
  // response time, clear the text manually here.
  this.clear();

  // If the entry is a directory, just change the directory.
  var entry = selectedItem.entry;
  if (entry.isDirectory) {
    this.directoryModel_.changeDirectoryEntry(entry);
    return;
  }

  // Change the current directory to the directory that contains the
  // selected file. Note that this is necessary for an image or a video,
  // which should be opened in the gallery mode, as the gallery mode
  // requires the entry to be in the current directory model. For
  // consistency, the current directory is always changed regardless of
  // the file type.
  entry.getParent(function(parentEntry) {
    // Check if the parent entry points /drive/other or not.
    // If so it just opens the file.
    var locationInfo = this.volumeManager_.getLocationInfo(parentEntry);
    if (!locationInfo ||
        (locationInfo.isRootEntry &&
         locationInfo.rootType === VolumeManagerCommon.RootType.DRIVE_OTHER)) {
      this.taskController_.executeEntryTask(entry);
      return;
    }
    // If the parent entry can be /drive/other.
    this.directoryModel_.changeDirectoryEntry(
        parentEntry,
        function() {
          this.directoryModel_.selectEntry(entry);
          this.taskController_.executeEntryTask(entry);
        }.bind(this));
  }.bind(this));
};

/**
 * Search files and update the list with the search result.
 * @param {string} searchString String to be searched with.
 * @private
 */
SearchController.prototype.search_ = function(searchString) {

  var onSearchRescan = function() {
    // If the current location is somewhere in Drive, all files in Drive can
    // be listed as search results regardless of current location.
    // In this case, showing current location is confusing, so use the Drive
    // root "My Drive" as the current location.
    if (this.isOnDrive_) {
      var locationInfo = this.currentLocationInfo_;
      var rootEntry = locationInfo.volumeInfo.displayRoot;
      if (rootEntry)
        this.locationLine_.show(rootEntry);
    }
  };

  var onClearSearch = function() {
    this.locationLine_.show(
        this.directoryModel_.getCurrentDirEntry());
  };

  this.directoryModel_.search(
      searchString,
      onSearchRescan.bind(this),
      onClearSearch.bind(this));
};
