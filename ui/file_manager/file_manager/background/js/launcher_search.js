// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Provides drive search results to chrome launcher.
 * @constructor
 * @struct
 */
function LauncherSearch() {
  // Launcher search provider is restricted to dev channel at now.
  if (!chrome.launcherSearchProvider)
    return;

  /**
   * Active query id. This value is set null when there is no active query.
   * @private {?number}
   */
  this.queryId_ = null;

  /**
   * True if this feature is enabled.
   * @private {boolean}
   */
  this.enabled_ = false;

  /**
   * @private {function(number, string, number)}
   */
  this.onQueryStartedBound_ = this.onQueryStarted_.bind(this);

  /**
   * @private {function(number)}
   */
  this.onQueryEndedBound_ = this.onQueryEnded_.bind(this);

  /**
   * @private {function(string)}
   */
  this.onOpenResultBound_ = this.onOpenResult_.bind(this);

  // This feature is disabled when drive is disabled.
  chrome.fileManagerPrivate.onPreferencesChanged.addListener(
      this.onPreferencesChanged_.bind(this));
  this.onPreferencesChanged_();
}

/**
 * Handles onPreferencesChanged event.
 */
LauncherSearch.prototype.onPreferencesChanged_ = function() {
  chrome.fileManagerPrivate.getPreferences(function(preferences) {
    this.initializeEventListeners_(
        preferences.driveEnabled, preferences.searchSuggestEnabled);
  }.bind(this));
};

/**
 * Initialize event listeners of chrome.launcherSearchProvider.
 *
 * When drive and search suggest are enabled, listen events of
 * chrome.launcherSearchProvider and provide seach resutls. When one of them is
 * disabled, remove these event listeners and stop providing search results.
 *
 * @param {boolean} isDriveEnabled True if drive is enabled.
 * @param {boolean} isSearchSuggestEnabled True if search suggest is enabled.
 */
LauncherSearch.prototype.initializeEventListeners_ = function(
    isDriveEnabled, isSearchSuggestEnabled) {
  var launcherSearchEnabled = isDriveEnabled && isSearchSuggestEnabled;

  // If this.enabled_ === launcherSearchEnabled, we don't need to change
  // anything here.
  if (this.enabled_ === launcherSearchEnabled)
    return;

  // Remove event listeners if it's already enabled.
  if (this.enabled_) {
    chrome.launcherSearchProvider.onQueryStarted.removeListener(
        this.onQueryStartedBound_);
    chrome.launcherSearchProvider.onQueryEnded.removeListener(
        this.onQueryEndedBound_);
    chrome.launcherSearchProvider.onOpenResult.removeListener(
        this.onOpenResultBound_);
  }

  // Set queryId_ to null to prevent that on-going search returns search
  // results.
  this.queryId_ = null;

  // Add event listeners when launcher search of Drive is enabled.
  if (launcherSearchEnabled) {
    this.enabled_ = true;
    chrome.launcherSearchProvider.onQueryStarted.addListener(
        this.onQueryStartedBound_);
    chrome.launcherSearchProvider.onQueryEnded.addListener(
        this.onQueryEndedBound_);
    chrome.launcherSearchProvider.onOpenResult.addListener(
        this.onOpenResultBound_);
  } else {
    this.enabled_ = false;
  }
};

/**
 * Handles onQueryStarted event.
 * @param {number} queryId
 * @param {string} query
 * @param {number} limit
 */
LauncherSearch.prototype.onQueryStarted_ = function(queryId, query, limit) {
  this.queryId_ = queryId;

  // Request an instance of volume manager to ensure that all volumes are
  // initialized. When user searches while background page of the Files app is
  // not running, it happens that this method is executed before all volumes are
  // initialized. In this method, chrome.fileManagerPrivate.searchDriveMetadata
  // resolves url internally, and it fails if filesystem of the url is not
  // initialized.
  volumeManagerFactory.getInstance()
      .then(() => {
        return Promise.all([
          this.queryDriveEntries_(queryId, query, limit),
          this.queryLocalEntries_(queryId, query)
        ]);
      })
      .then((results) => {
        const entries = results[0].concat(results[1]);
        if (queryId !== this.queryId_ || entries.length === 0)
          return;

        const resultEntries = this.chooseEntries_(entries, query, limit);
        const searchResults = resultEntries.map(this.createSearchResult_);
        chrome.launcherSearchProvider.setSearchResults(queryId, searchResults);
      });
};

/**
 * Handles onQueryEnded event.
 * @param {number} queryId
 */
LauncherSearch.prototype.onQueryEnded_ = function(queryId) {
  this.queryId_ = null;
};

/**
 * Handles onOpenResult event.
 * @param {string} itemId
 */
LauncherSearch.prototype.onOpenResult_ = function(itemId) {
  // Request an instance of volume manager to ensure that all volumes are
  // initialized. webkitResolveLocalFileSystemURL in util.urlToEntry fails if
  // filesystem of the url is not initialized.
  volumeManagerFactory.getInstance().then(function() {
    util.urlToEntry(itemId).then(function(entry) {
      if (entry.isDirectory) {
        // If it's directory, open the directory with file manager.
        launcher.launchFileManager(
            { currentDirectoryURL: entry.toURL() },
            undefined, /* App ID */
            LaunchType.FOCUS_SAME_OR_CREATE);
      } else {
        // If the file is not directory, try to execute default task.
        chrome.fileManagerPrivate.getFileTasks([entry], function(tasks) {
          // Select default task.
          var defaultTask = null;
          for (var i = 0; i < tasks.length; i++) {
            var task = tasks[i];
            if (task.isDefault) {
              defaultTask = task;
              break;
            }
          }

          // If we haven't picked a default task yet, then just pick the first
          // one which is not genric file hanlder as default task.
          // TODO(yawano) Share task execution logic with file_tasks.js.
          if (!defaultTask) {
            for (var i = 0; i < tasks.length; i++) {
              var task = tasks[i];
              if (!task.isGenericFileHandler) {
                defaultTask = task;
                break;
              }
            }
          }

          if (defaultTask) {
            // Execute default task.
            chrome.fileManagerPrivate.executeTask(
                defaultTask.taskId,
                [entry],
                function(result) {
                  if (result === 'opened' || result === 'message_sent')
                    return;
                  this.openFileManagerWithSelectionURL_(entry.toURL());
                }.bind(this));
          } else {
            // If there is no default task for the url, open a file manager with
            // selecting it.
            // TODO(yawano): Add fallback to view-in-browser as file_tasks.js do
            this.openFileManagerWithSelectionURL_(entry.toURL());
          }
        }.bind(this));
      }
    }.bind(this));
  }.bind(this));
};

/**
 * Opens file manager with selecting a specified url.
 * @param {string} selectionURL A url to be selected.
 * @private
 */
LauncherSearch.prototype.openFileManagerWithSelectionURL_ = function(
    selectionURL) {
  launcher.launchFileManager(
      {selectionURL: selectionURL},
      undefined, /* App ID */
      LaunchType.FOCUS_SAME_OR_CREATE);
};

/**
 * Queries entries which match the given query in Google Drive.
 * @param {number} queryId
 * @param {string} query
 * @param {number} limit
 * @return {!Promise<!Array<!Entry>>}
 * @private
 */
LauncherSearch.prototype.queryDriveEntries_ = function(queryId, query, limit) {
  var param = {query: query, types: 'ALL', maxResults: limit};
  return new Promise((resolve, reject) => {
    chrome.fileManagerPrivate.searchDriveMetadata(param, function(results) {
      resolve(results.map(result => result.entry));
    });
  });
};

/**
 * Queries entries which match the given query in Downloads.
 * @param {number} queryId
 * @param {string} query
 * @return {!Promise<!Array<!Entry>>}
 * @private
 */
LauncherSearch.prototype.queryLocalEntries_ = function(queryId, query) {
  if (!query)
    return Promise.resolve([]);

  return this.getDownloadsEntry_()
      .then((downloadsEntry) => {
        return this.queryEntriesRecursively_(
            downloadsEntry, queryId, query.toLowerCase());
      })
      .catch((error) => {
        if (error.name != 'AbortError')
          console.error('Query local entries failed.', error);
        return [];
      });
};

/**
 * Returns root entry of Downloads volume.
 * @return {!Promise<!DirectoryEntry>}
 * @private
 */
LauncherSearch.prototype.getDownloadsEntry_ = function() {
  return volumeManagerFactory.getInstance().then((volumeManager) => {
    var downloadsVolumeInfo = volumeManager.getCurrentProfileVolumeInfo(
        VolumeManagerCommon.VolumeType.DOWNLOADS);
    return downloadsVolumeInfo.resolveDisplayRoot();
  });
};

/**
 * Queries entries which match the given query inside rootEntry recursively.
 * @param {!DirectoryEntry} rootEntry
 * @param {number} queryId
 * @param {string} query
 * @return {!Promise<!Array<!Entry>>}
 * @private
 */
LauncherSearch.prototype.queryEntriesRecursively_ = function(
    rootEntry, queryId, query) {
  return new Promise((resolve, reject) => {
    let foundEntries = [];
    util.readEntriesRecursively(
        rootEntry,
        (entries) => {
          const matchEntries = entries.filter(
              entry => entry.name.toLowerCase().indexOf(query) >= 0);
          if (matchEntries.length > 0)
            foundEntries = foundEntries.concat(matchEntries);
        },
        () => {
          resolve(foundEntries);
        },
        reject,
        () => {
          return this.queryId_ != queryId;
        });
  });
};

/**
 * Chooses entries to show among the given entries.
 * @param {!Array<!Entry>} entries
 * @param {string} query
 * @param {number} limit
 * @return {!Array<!Entry>}
 * @private
 */
LauncherSearch.prototype.chooseEntries_ = function(entries, query, limit) {
  query = query.toLowerCase();
  const scoreEntry = (entry) => {
    // Prefer entry which has the query string as a prefix.
    if (entry.name.toLowerCase().indexOf(query) === 0)
      return 1;
    return 0;
  };
  const sortedEntries = entries.sort((a, b) => {
    return scoreEntry(b) - scoreEntry(a);
  });
  return sortedEntries.slice(0, limit);
};

/**
 * Creates a search result from entry to pass to launcherSearchProvider API.
 * @param {!Entry} entry
 * @return {!Object}
 * @private
 */
LauncherSearch.prototype.createSearchResult_ = function(entry) {
  // TODO(yawano): Use filetype_folder_shared.png for a shared
  //     folder.
  // TODO(yawano): Add archive launcher filetype icon.
  var icon = FileType.getIcon(entry);
  if (icon === 'UNKNOWN' || icon === 'archive')
    icon = 'generic';

  var useHighDpiIcon = window.devicePixelRatio > 1.0;
  var iconUrl = chrome.runtime.getURL(
      'foreground/images/launcher_filetypes/' + (useHighDpiIcon ? '2x/' : '') +
      'launcher_filetype_' + icon + '.png');

  // Hide extensions for hosted files.
  var title = FileType.isHosted(entry) ?
      entry.name.substr(
          0, entry.name.length - FileType.getExtension(entry).length) :
      entry.name;

  return {
    itemId: entry.toURL(),
    title: title,
    iconUrl: iconUrl,
    // Relevance is set as 2 for all results as a temporary
    // implementation. 2 is the middle value.
    // TODO(yawano): Implement practical relevance calculation.
    relevance: 2
  };
};
