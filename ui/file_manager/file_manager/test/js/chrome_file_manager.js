// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Test implementation of chrome.file[ManagerPrivate|System] apis.
// These APIs are provided natively to a chrome app, but since we are
// running as a regular web page, we must provide test implementations.

mockVolumeManager = new MockVolumeManager();

// Create drive /root/ immediately.
mockVolumeManager
    .getCurrentProfileVolumeInfo(VolumeManagerCommon.VolumeType.DRIVE)
    .fileSystem.populate(['/root/']);

chrome.fileManagerPrivate = {
  currentId_: 'test@example.com',
  dispatchEvent_: function(listenerType, event) {
    setTimeout(() => {
      this[listenerType].listeners_.forEach(l => l.call(null, event));
    }, 0);
  },
  displayedId_: 'test@example.com',
  preferences_: {
    allowRedeemOffers: true,
    cellularDisabled: true,
    driveEnabled: true,
    hostedFilesDisabled: true,
    searchSuggestEnabled: true,
    timezone: 'Australia/Sydney',
    use24hourClock: false,
  },
  profiles_: [{
    displayName: 'Test User',
    isCurrentProfile: true,
    profileId: 'test@example.com'
  }],
  token_: 'token',
  SourceRestriction: {
    ANY_SOURCE: 'any_source',
    NATIVE_OR_DRIVE_SOURCE: 'native_or_drive_source',
    NATIVE_SOURCE: 'native_source',
  },
  addFileWatch: (entry, callback) => {
    // Returns success.
    setTimeout(callback, 0, true);
  },
  enableExternalFileScheme: () => {},
  executeTask: (taskId, entries, callback) => {
    // Returns opened|message_sent|failed|empty.
    setTimeout(callback, 0, 'failed');
  },
  getDriveConnectionState: (callback) => {
    setTimeout(callback, 0, mockVolumeManager.getDriveConnectionState());
  },
  getEntryProperties: (entries, names, callback) => {
    // Returns EntryProperties[].
    var results = [];
    entries.forEach(entry => {
      var props = {};
      names.forEach(name => {
        props[name] = entry.metadata[name];
      });
      results.push(props);
    });
    setTimeout(callback, 0, results);
  },
  getFileTasks: (entries, callback) => {
    // Returns FileTask[].
    var results = [];
    // Support for view-in-browser on single text file used by QuickView.
    if (entries.length == 1 &&
        entries[0].metadata.contentMimeType == 'text/plain') {
      results.push({
        taskId: '|file|view-in-browser',
        title: '__MSG_OPEN_ACTION__',
        isDefault: true,
      });
    }
    setTimeout(callback, 0, results);
  },
  getPreferences: (callback) => {
    setTimeout(callback, 0, chrome.fileManagerPrivate.preferences_);
  },
  getProfiles: (callback) => {
    // Returns profiles, currentId, displayedId
    setTimeout(
        callback, 0, chrome.fileManagerPrivate.profiles_,
        chrome.fileManagerPrivate.currentId_,
        chrome.fileManagerPrivate.displayedId_);
  },
  getProviders: (callback) => {
    // Returns Provider[].
    setTimeout(callback, 0, []);
  },
  getRecentFiles: (restriction, callback) => {
    // Returns Entry[].
    setTimeout(callback, 0, []);
  },
  getSizeStats: (volumeId, callback) => {
    // MountPointSizeStats { totalSize: double,  remainingSize: double }
    setTimeout(callback, 0, {totalSize: 16e9, remainingSize: 8e9});
  },
  getStrings: (callback) => {
    // Returns map of strings.
    setTimeout(callback, 0, loadTimeData.data_);
  },
  getVolumeMetadataList: (callback) => {
    var list = [];
    for (var i = 0; i < mockVolumeManager.volumeInfoList.length; i++) {
      list.push(mockVolumeManager.volumeInfoList.item(i));
    }
    setTimeout(callback, 0, list);
  },
  grantAccess: (entryUrls, callback) => {
    setTimeout(callback, 0);
  },
  crostiniEnabled_: true,
  isCrostiniEnabled: function(callback) {
    setTimeout(callback, 0, this.crostiniEnabled_);
  },
  isUMAEnabled: (callback) => {
    setTimeout(callback, 0, false);
  },
  mountCrostiniContainer: (callback) => {
    // Simulate startup of vm and container by taking 3s.
    setTimeout(callback, 3000);
  },
  onAppsUpdated: {
    addListener: () => {},
  },
  onCopyProgress: {
    listeners_: [],
    addListener: function(l) {
      this.listeners_.push(l);
    },
    removeListener: function(l) {
      this.listeners_ = this.listeners_.filter(e => e !== l);
    },
  },
  onDeviceChanged: {
    addListener: () => {},
  },
  onDirectoryChanged: {
    listeners_: [],
    addListener: function(l) {
      this.listeners_.push(l);
    },
    removeListener: function(l) {
      this.listeners_.splice(this.listeners_.indexOf(l), 1);
    },
  },
  onDriveConnectionStatusChanged: {
    addListener: () => {},
  },
  onDriveSyncError: {
    addListener: () => {},
  },
  onFileTransfersUpdated: {
    addListener: () => {},
  },
  onMountCompleted: {
    listeners_: [],
    addListener: function(l) {
      this.listeners_.push(l);
    },
  },
  onPreferencesChanged: {
    addListener: () => {},
  },
  openInspector: (type) => {},
  openSettingsSubpage: (sub_page) => {},
  removeFileWatch: (entry, callback) => {
    setTimeout(callback, 0, true);
  },
  removeMount(volumeId) {
    chrome.fileManagerPrivate.dispatchEvent_('onMountCompleted', {
      status: 'success',
      eventType: 'unmount',
      volumeMetadata: {
        volumeId: volumeId,
      },
    });
  },
  requestWebStoreAccessToken: (callback) => {
    setTimeout(callback, 0, chrome.fileManagerPrivate.token_);
  },
  resolveIsolatedEntries: (entries, callback) => {
    setTimeout(callback, 0, entries);
  },
  searchDriveMetadata: (searchParams, callback) => {
    // Returns SearchResult[].
    // SearchResult { entry: Entry, highlightedBaseName: string }
    setTimeout(callback, 0, []);
  },
  nextCopyId_: 0,
  startCopy: (entry, parentEntry, newName, callback) => {
    // Returns copyId immediately.
    var copyId = chrome.fileManagerPrivate.nextCopyId_++;
    callback(copyId);
    chrome.fileManagerPrivate.onCopyProgress.listeners_.forEach(l => {
      l(copyId, {type: 'begin_copy_entry', sourceUrl: entry.toURL()});
    });
    entry.copyTo(
        parentEntry, newName,
        // Success.
        (copied) => {
          chrome.fileManagerPrivate.onCopyProgress.listeners_.forEach(l => {
            l(copyId, {
              type: 'end_copy_entry',
              sourceUrl: entry.toURL(),
              destinationUrl: copied.toURL()
            });
          });
          chrome.fileManagerPrivate.onCopyProgress.listeners_.forEach(l => {
            l(copyId, {
              type: 'success',
              sourceUrl: entry.toURL(),
              destinationUrl: copied.toURL()
            });
          });
        },
        // Error.
        (error) => {
          chrome.fileManagerPrivate.onCopyProgress.listeners_.forEach(l => {
            l(copyId, {type: 'error', error: error});
          });
        });
  },
  validatePathNameLength: (parentEntry, name, callback) => {
    setTimeout(callback, 0, true);
  },
};

chrome.mediaGalleries = {
  getMetadata: (mediaFile, options, callback) => {
    // Returns metdata {mimeType: ..., ...}.
    setTimeout(() => {
      webkitResolveLocalFileSystemURL(mediaFile.name, entry => {
        callback({mimeType: entry.metadata.contentMimeType});
      }, 0);
    });
  },
};

chrome.fileSystem = {
  requestFileSystem: (options, callback) => {
    var volume =
        mockVolumeManager.volumeInfoList.findByVolumeId(options.volumeId);
    setTimeout(callback, 0, volume ? volume.fileSystem : null);
  },
};

/**
 * Override webkitResolveLocalFileSystemURL for testing.
 * @param {string} url URL to resolve.
 * @param {function(!MockEntry)} successCallback Success callback.
 * @param {function(!DOMException)} errorCallback Error callback.
 */
webkitResolveLocalFileSystemURL = (url, successCallback, errorCallback) => {
  var match = url.match(/^filesystem:(\w+)(\/.*)/);
  if (match) {
    var volumeType = match[1];
    var path = match[2];
    var volume = mockVolumeManager.getCurrentProfileVolumeInfo(volumeType);
    if (volume) {
      // Decode URI in file paths.
      path = path.split('/').map(decodeURIComponent).join('/');
      var entry = volume.fileSystem.entries[path];
      if (entry) {
        setTimeout(successCallback, 0, entry);
        return;
      }
    }
  }
  var error = new DOMException(
      'webkitResolveLocalFileSystemURL not found: [' + url + ']');
  if (errorCallback) {
    setTimeout(errorCallback, 0, error);
  } else {
    throw error;
  }
};
