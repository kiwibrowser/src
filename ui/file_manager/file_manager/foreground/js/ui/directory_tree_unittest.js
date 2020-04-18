// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var chrome;
var mockCommandLinePrivate;

/**
 * Set string data.
 * @type {Object}
 */
loadTimeData.data = {
  DOWNLOADS_DIRECTORY_LABEL: 'Downloads',
  DRIVE_DIRECTORY_LABEL: 'Google Drive',
  DRIVE_MY_DRIVE_LABEL: 'My Drive',
  DRIVE_TEAM_DRIVES_LABEL: 'Team Drives',
  DRIVE_OFFLINE_COLLECTION_LABEL: 'Offline',
  DRIVE_SHARED_WITH_ME_COLLECTION_LABEL: 'Shared with me',
  REMOVABLE_DIRECTORY_LABEL: 'External Storage',
  ARCHIVE_DIRECTORY_LABEL: 'Archives'
};

function setUp() {
  chrome = {
    fileManagerPrivate: {
      onDirectoryChanged: {
        addListener: function(listener) { /* Do nothing. */ }
      }
    }
  };
  mockCommandLinePrivate = new MockCommandLinePrivate();

  window.webkitResolveLocalFileSystemURLEntries = {};
  window.webkitResolveLocalFileSystemURL = function(url, callback) {
    callback(webkitResolveLocalFileSystemURLEntries[url]);
  };
}

/**
 * Returns item labels of a directory tree as a list.
 *
 * @param {DirectoryTree} directoryTree A directory tree.
 * @return {Array<string>} List of labels.
 */
function getDirectoryTreeItemLabelsAsAList(directoryTree) {
  var result = [];
  for (var i = 0; i < directoryTree.items.length; i++) {
    var item = directoryTree.items[i];
    result.push(item.label);
  }
  return result;
}

/**
 * Test case for typical creation of directory tree.
 * This test expect that the following tree is built.
 *
 * Google Drive
 * - My Drive
 * - Shared with me
 * - Offline
 * Downloads
 *
 * @param {!function(boolean)} callback A callback function which is called with
 *     test result.
 */
function testCreateDirectoryTree(callback) {
  // Create elements.
  var parentElement = document.createElement('div');
  var directoryTree = document.createElement('div');
  parentElement.appendChild(directoryTree);

  // Create mocks.
  var directoryModel = new MockDirectoryModel();
  var volumeManager = new MockVolumeManagerWrapper();
  var fileOperationManager = {
    addEventListener: function(name, callback) {}
  };

  // Set entry which is returned by
  // window.webkitResolveLocalFileSystemURLResults.
  var driveFileSystem = volumeManager.volumeInfoList.item(0).fileSystem;
  window.webkitResolveLocalFileSystemURLEntries['filesystem:drive/root'] =
      new MockDirectoryEntry(driveFileSystem, '/root');

  DirectoryTree.decorate(directoryTree, directoryModel, volumeManager,
      null, fileOperationManager, true);
  directoryTree.dataModel = new MockNavigationListModel(volumeManager);
  directoryTree.redraw(true);

  // At top level, Drive and downloads should be listed.
  assertEquals(2, directoryTree.items.length);
  assertEquals(str('DRIVE_DIRECTORY_LABEL'), directoryTree.items[0].label);
  assertEquals(str('DOWNLOADS_DIRECTORY_LABEL'), directoryTree.items[1].label);

  var driveItem = directoryTree.items[0];

  reportPromise(
      waitUntil(function() {
        // Under the drive item, there exist 3 entries.
        return driveItem.items.length == 3;
      }).then(function() {
        // There exist 1 my drive entry and 3 fake entries under the drive item.
        assertEquals(str('DRIVE_MY_DRIVE_LABEL'), driveItem.items[0].label);
        assertEquals(
            str('DRIVE_SHARED_WITH_ME_COLLECTION_LABEL'),
            driveItem.items[1].label);
        assertEquals(
            str('DRIVE_OFFLINE_COLLECTION_LABEL'), driveItem.items[2].label);
      }),
      callback);
}

/**
 * Test case for creating tree with Team Drives.
 * This test expect that the following tree is built.
 *
 * Google Drive
 * - My Drive
 * - Team Drives
 * - Shared with me
 * - Offline
 * Downloads
 *
 * @param {!function(boolean)} callback A callback function which is called with
 *     test result.
 */
function testCreateDirectoryTreeWithTeamDrive(callback) {
  mockCommandLinePrivate.addSwitch('team-drives');

  // Create elements.
  var parentElement = document.createElement('div');
  var directoryTree = document.createElement('div');
  directoryTree.metadataModel = {
    notifyEntriesChanged: function() {},
    get: function(entries, labels) {
      // Mock a non-shared directory
      return Promise.resolve([{shared: false}]);
    }
  };
  parentElement.appendChild(directoryTree);

  // Create mocks.
  var directoryModel = new MockDirectoryModel();
  var volumeManager = new MockVolumeManagerWrapper();
  var fileOperationManager = {addEventListener: function(name, callback) {}};

  // Set entry which is returned by
  // window.webkitResolveLocalFileSystemURLResults.
  var driveFileSystem = volumeManager.volumeInfoList.item(0).fileSystem;
  window.webkitResolveLocalFileSystemURLEntries['filesystem:drive/root'] =
      new MockDirectoryEntry(driveFileSystem, '/root');
  window
      .webkitResolveLocalFileSystemURLEntries['filesystem:drive/team_drives'] =
      new MockDirectoryEntry(driveFileSystem, '/team_drives');
  window.webkitResolveLocalFileSystemURLEntries
      ['filesystem:drive/team_drives/a'] =
      new MockDirectoryEntry(driveFileSystem, '/team_drives/a');

  DirectoryTree.decorate(
      directoryTree, directoryModel, volumeManager, null, fileOperationManager,
      true);
  directoryTree.dataModel = new MockNavigationListModel(volumeManager);
  directoryTree.redraw(true);

  // At top level, Drive and downloads should be listed.
  assertEquals(2, directoryTree.items.length);
  assertEquals(str('DRIVE_DIRECTORY_LABEL'), directoryTree.items[0].label);
  assertEquals(str('DOWNLOADS_DIRECTORY_LABEL'), directoryTree.items[1].label);

  var driveItem = directoryTree.items[0];

  reportPromise(
      waitUntil(function() {
        // Under the drive item, there exist 4 entries.
        return driveItem.items.length == 4;
      }).then(function() {
        // There exist 1 my drive entry and 3 fake entries under the drive item.
        assertEquals(str('DRIVE_MY_DRIVE_LABEL'), driveItem.items[0].label);
        assertEquals(str('DRIVE_TEAM_DRIVES_LABEL'), driveItem.items[1].label);
        assertEquals(
            str('DRIVE_SHARED_WITH_ME_COLLECTION_LABEL'),
            driveItem.items[2].label);
        assertEquals(
            str('DRIVE_OFFLINE_COLLECTION_LABEL'), driveItem.items[3].label);
      }),
      callback);
}

/**
 * Test case for creating tree with empty Team Drives.
 * Team Drives subtree should be hidden when the user don't have access to any
 * Team Drive.
 *
 * @param {!function(boolean)} callback A callback function which is called with
 *     test result.
 */
function testCreateDirectoryTreeWithEmptyTeamDrive(callback) {
  mockCommandLinePrivate.addSwitch('team-drives');

  // Create elements.
  var parentElement = document.createElement('div');
  var directoryTree = document.createElement('div');
  directoryTree.metadataModel = {
    notifyEntriesChanged: function() {},
    get: function(entries, labels) {
      // Mock a non-shared directory
      return Promise.resolve([{shared: false}]);
    }
  };
  parentElement.appendChild(directoryTree);

  // Create mocks.
  var directoryModel = new MockDirectoryModel();
  var volumeManager = new MockVolumeManagerWrapper();
  var fileOperationManager = {addEventListener: function(name, callback) {}};

  // Set entry which is returned by
  // window.webkitResolveLocalFileSystemURLResults.
  var driveFileSystem = volumeManager.volumeInfoList.item(0).fileSystem;
  window.webkitResolveLocalFileSystemURLEntries['filesystem:drive/root'] =
      new MockDirectoryEntry(driveFileSystem, '/root');
  window
      .webkitResolveLocalFileSystemURLEntries['filesystem:drive/team_drives'] =
      new MockDirectoryEntry(driveFileSystem, '/team_drives');
  // No directories exist under Team Drives

  DirectoryTree.decorate(
      directoryTree, directoryModel, volumeManager, null, fileOperationManager,
      true);
  directoryTree.dataModel = new MockNavigationListModel(volumeManager);
  directoryTree.redraw(true);

  var driveItem = directoryTree.items[0];

  reportPromise(
      waitUntil(function() {
        // Root entries under Drive volume is generated except Team Drives.
        // See testCreateDirectoryTreeWithTeamDrive for detail.
        return driveItem.items.length == 3;
      }).then(function() {
        for (var i = 0; i < driveItem.items.length; i++) {
          assertFalse(
              driveItem.items[i].label == str('DRIVE_TEAM_DRIVES_LABEL'),
              'Team Drives node should not be shown');
        }
      }),
      callback);
}

/**
 * Test case for updateSubElementsFromList.
 *
 * Mounts/unmounts removable and archive volumes, and checks these volumes come
 * up to/disappear from the list correctly.
 */
function testUpdateSubElementsFromList() {
  // Creates elements.
  var parentElement = document.createElement('div');
  var directoryTree = document.createElement('div');
  parentElement.appendChild(directoryTree);

  // Creates mocks.
  var directoryModel = new MockDirectoryModel();
  var volumeManager = new MockVolumeManagerWrapper();
  var fileOperationManager = {
    addEventListener: function(name, callback) {}
  };

  // Sets entry which is returned by
  // window.webkitResolveLocalFileSystemURLResults.
  var driveFileSystem = volumeManager.volumeInfoList.item(0).fileSystem;
  window.webkitResolveLocalFileSystemURLEntries['filesystem:drive/root'] =
      new MockDirectoryEntry(driveFileSystem, '/root');

  DirectoryTree.decorate(directoryTree, directoryModel, volumeManager,
      null, fileOperationManager, true);
  directoryTree.dataModel = new MockNavigationListModel(volumeManager);
  directoryTree.updateSubElementsFromList(true);

  // There are 2 volumes, Drive and Downloads, at first.
  assertArrayEquals([
    str('DRIVE_DIRECTORY_LABEL'),
    str('DOWNLOADS_DIRECTORY_LABEL')
  ], getDirectoryTreeItemLabelsAsAList(directoryTree));

  // Mounts a removable volume.
  var removableVolume = MockVolumeManagerWrapper.createMockVolumeInfo(
      VolumeManagerCommon.VolumeType.REMOVABLE,
      'removable',
      str('REMOVABLE_DIRECTORY_LABEL'));
  volumeManager.volumeInfoList.push(removableVolume);

  // Asserts that the directoryTree is not updated before the update.
  assertArrayEquals([
    str('DRIVE_DIRECTORY_LABEL'),
    str('DOWNLOADS_DIRECTORY_LABEL')
  ], getDirectoryTreeItemLabelsAsAList(directoryTree));

  // Asserts that a removable directory is added after the update.
  directoryTree.updateSubElementsFromList(false);
  assertArrayEquals([
    str('DRIVE_DIRECTORY_LABEL'),
    str('DOWNLOADS_DIRECTORY_LABEL'),
    str('REMOVABLE_DIRECTORY_LABEL')
  ], getDirectoryTreeItemLabelsAsAList(directoryTree));

  // Mounts an archive volume before the removable directory.
  var archiveVolume = MockVolumeManagerWrapper.createMockVolumeInfo(
      VolumeManagerCommon.VolumeType.ARCHIVE,
      'archive',
      str('ARCHIVE_DIRECTORY_LABEL'));
  volumeManager.volumeInfoList.splice(2, 0, archiveVolume);

  // Asserts that the directoryTree is not updated before the update.
  assertArrayEquals([
    str('DRIVE_DIRECTORY_LABEL'),
    str('DOWNLOADS_DIRECTORY_LABEL'),
    str('REMOVABLE_DIRECTORY_LABEL')
  ], getDirectoryTreeItemLabelsAsAList(directoryTree));

  // Asserts that an archive directory is added before the removable directory.
  directoryTree.updateSubElementsFromList(false);
  assertArrayEquals([
    str('DRIVE_DIRECTORY_LABEL'),
    str('DOWNLOADS_DIRECTORY_LABEL'),
    str('ARCHIVE_DIRECTORY_LABEL'),
    str('REMOVABLE_DIRECTORY_LABEL')
  ], getDirectoryTreeItemLabelsAsAList(directoryTree));

  // Deletes an archive directory.
  volumeManager.volumeInfoList.splice(2, 1);

  // Asserts that the directoryTree is not updated before the update.
  assertArrayEquals([
    str('DRIVE_DIRECTORY_LABEL'),
    str('DOWNLOADS_DIRECTORY_LABEL'),
    str('ARCHIVE_DIRECTORY_LABEL'),
    str('REMOVABLE_DIRECTORY_LABEL')
  ], getDirectoryTreeItemLabelsAsAList(directoryTree));

  // Asserts that an archive directory is deleted.
  directoryTree.updateSubElementsFromList(false);
  assertArrayEquals([
    str('DRIVE_DIRECTORY_LABEL'),
    str('DOWNLOADS_DIRECTORY_LABEL'),
    str('REMOVABLE_DIRECTORY_LABEL')
  ], getDirectoryTreeItemLabelsAsAList(directoryTree));
}
