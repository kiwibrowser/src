// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @type {!MockFileSystem} Simulate the drive file system. */
var drive;

/** @type {!MockFileSystem} Simulate a removable volume. */
var hoge;

// Set up string assets.
loadTimeData.data = {
  DRIVE_DIRECTORY_LABEL: 'My Drive',
  DOWNLOADS_DIRECTORY_LABEL: 'Downloads',
  LINUX_FILES_ROOT_LABEL: 'Linux Files',
};

function setUp() {
  new MockCommandLinePrivate();
  // Override VolumeInfo.prototype.resolveDisplayRoot.
  VolumeInfoImpl.prototype.resolveDisplayRoot = function() {};

  // TODO(crbug.com/834103): Add integration test for Crostini.
  drive = new MockFileSystem('drive');
  hoge = new MockFileSystem('removable:hoge');
}

function testModel() {
  var volumeManager = new MockVolumeManagerWrapper();
  var shortcutListModel = new MockFolderShortcutDataModel(
      [new MockFileEntry(drive, '/root/shortcut')]);
  var recentItem = new NavigationModelFakeItem(
      'recent-label', NavigationModelItemType.RECENT,
      {toURL: () => 'fake-entry://recent'});
  var addNewServicesItem = new NavigationModelMenuItem(
      'menu-button-label', '#add-new-services', 'menu-button-icon');
  var model = new NavigationListModel(
      volumeManager, shortcutListModel, recentItem, addNewServicesItem);
  model.linuxFilesItem = new NavigationModelFakeItem(
      'linux-files-label', NavigationModelItemType.CROSTINI,
      {toURL: () => 'fake-entry://linux-files'});

  assertEquals(6, model.length);
  assertEquals('drive', model.item(0).volumeInfo.volumeId);
  assertEquals('downloads', model.item(1).volumeInfo.volumeId);
  assertEquals('fake-entry://recent', model.item(2).entry.toURL());
  assertEquals('fake-entry://linux-files', model.item(3).entry.toURL());
  assertEquals('/root/shortcut', model.item(4).entry.fullPath);
  assertEquals('menu-button-label', model.item(5).label);
  assertEquals('#add-new-services', model.item(5).menu);
  assertEquals('menu-button-icon', model.item(5).icon);
}

function testNoRecentOrLinuxFiles() {
  var volumeManager = new MockVolumeManagerWrapper();
  var shortcutListModel = new MockFolderShortcutDataModel(
      [new MockFileEntry(drive, '/root/shortcut')]);
  var recentItem = null;
  var addNewServicesItem = new NavigationModelMenuItem(
      'menu-button-label', '#add-new-services', 'menu-button-icon');
  var model = new NavigationListModel(
      volumeManager, shortcutListModel, recentItem, addNewServicesItem);

  assertEquals(4, model.length);
  assertEquals('drive', model.item(0).volumeInfo.volumeId);
  assertEquals('downloads', model.item(1).volumeInfo.volumeId);
  assertEquals('/root/shortcut', model.item(2).entry.fullPath);
  assertEquals('menu-button-label', model.item(3).label);
  assertEquals('#add-new-services', model.item(3).menu);
  assertEquals('menu-button-icon', model.item(3).icon);
}

function testAddAndRemoveShortcuts() {
  var volumeManager = new MockVolumeManagerWrapper();
  var shortcutListModel = new MockFolderShortcutDataModel(
      [new MockFileEntry(drive, '/root/shortcut')]);
  var recentItem = null;
  var addNewServicesItem = null;
  var model = new NavigationListModel(
      volumeManager, shortcutListModel, recentItem, addNewServicesItem);

  assertEquals(3, model.length);

  // Add a shortcut at the tail.
  shortcutListModel.splice(1, 0, new MockFileEntry(drive, '/root/shortcut2'));
  assertEquals(4, model.length);
  assertEquals('/root/shortcut2', model.item(3).entry.fullPath);

  // Add a shortcut at the head.
  shortcutListModel.splice(0, 0, new MockFileEntry(drive, '/root/hoge'));
  assertEquals(5, model.length);
  assertEquals('/root/hoge', model.item(2).entry.fullPath);
  assertEquals('/root/shortcut', model.item(3).entry.fullPath);
  assertEquals('/root/shortcut2', model.item(4).entry.fullPath);

  // Remove the last shortcut.
  shortcutListModel.splice(2, 1);
  assertEquals(4, model.length);
  assertEquals('/root/hoge', model.item(2).entry.fullPath);
  assertEquals('/root/shortcut', model.item(3).entry.fullPath);

  // Remove the first shortcut.
  shortcutListModel.splice(0, 1);
  assertEquals(3, model.length);
  assertEquals('/root/shortcut', model.item(2).entry.fullPath);
}

function testAddAndRemoveVolumes() {
  var volumeManager = new MockVolumeManagerWrapper();
  var shortcutListModel = new MockFolderShortcutDataModel(
      [new MockFileEntry(drive, '/root/shortcut')]);
  var recentItem = null;
  var addNewServicesItem = null;
  var model = new NavigationListModel(
      volumeManager, shortcutListModel, recentItem, addNewServicesItem);

  assertEquals(3, model.length);

  // Removable volume 'hoge' is mounted.
  volumeManager.volumeInfoList.push(
      MockVolumeManagerWrapper.createMockVolumeInfo(
          VolumeManagerCommon.VolumeType.REMOVABLE, 'removable:hoge'));
  assertEquals(4, model.length);
  assertEquals('drive', model.item(0).volumeInfo.volumeId);
  assertEquals('downloads', model.item(1).volumeInfo.volumeId);
  assertEquals('removable:hoge', model.item(2).volumeInfo.volumeId);
  assertEquals('/root/shortcut', model.item(3).entry.fullPath);

  // Removable volume 'fuga' is mounted.
  volumeManager.volumeInfoList.push(
      MockVolumeManagerWrapper.createMockVolumeInfo(
          VolumeManagerCommon.VolumeType.REMOVABLE, 'removable:fuga'));
  assertEquals(5, model.length);
  assertEquals('drive', model.item(0).volumeInfo.volumeId);
  assertEquals('downloads', model.item(1).volumeInfo.volumeId);
  assertEquals('removable:hoge', model.item(2).volumeInfo.volumeId);
  assertEquals('removable:fuga', model.item(3).volumeInfo.volumeId);
  assertEquals('/root/shortcut', model.item(4).entry.fullPath);

  // A shortcut is created on the 'hoge' volume.
  shortcutListModel.splice(
      1, 0, new MockFileEntry(hoge, '/shortcut2'));
  assertEquals(6, model.length);
  assertEquals('drive', model.item(0).volumeInfo.volumeId);
  assertEquals('downloads', model.item(1).volumeInfo.volumeId);
  assertEquals('removable:hoge', model.item(2).volumeInfo.volumeId);
  assertEquals('removable:fuga', model.item(3).volumeInfo.volumeId);
  assertEquals('/root/shortcut', model.item(4).entry.fullPath);
  assertEquals('/shortcut2', model.item(5).entry.fullPath);
}
