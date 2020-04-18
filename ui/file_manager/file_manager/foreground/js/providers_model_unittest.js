// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Set up string assets.
loadTimeData.data = {
  DRIVE_DIRECTORY_LABEL: 'My Drive',
  DOWNLOADS_DIRECTORY_LABEL: 'Downloads'
};

// A providing extension which has mounted a file system, and doesn't support
// multiple mounts.
var MOUNTED_SINGLE_PROVIDING_EXTENSION = {
  providerId: 'mounted-single-provider-id',
  iconSet: {
    icon16x16Url: 'chrome://mounted-single-extension-id-16.jpg',
    icon32x32Url: 'chrome://mounted-single-extension-id-32.jpg'
  },
  name: 'mounted-single-extension-name',
  configurable: false,
  watchable: true,
  multipleMounts: false,
  source: 'network'
};

// A providing extension which has not mounted a file system, and doesn't
// support  multiple mounts.
var NOT_MOUNTED_SINGLE_PROVIDING_EXTENSION = {
  providerId: 'not-mounted-single-provider-id',
  iconSet: {
    icon16x16Url: 'chrome://not-mounted-single-extension-id-16.jpg',
    icon32x32Url: 'chrome://not-mounted-single-extension-id-32.jpg'
  },
  name: 'not-mounted-single-extension-name',
  configurable: false,
  watchable: true,
  multipleMounts: false,
  source: 'network'
};
// A providing extension which has not mounted a file system, and doesn't
// support  multiple mounts.
var NOT_MOUNTED_SINGLE_PROVIDING_EXTENSION = {
  providerId: 'not-mounted-single-provider-id',
  iconSet: {
    icon16x16Url: 'chrome://not-mounted-single-extension-id-16.jpg',
    icon32x32Url: 'chrome://not-mounted-single-extension-id-32.jpg'
  },
  name: 'not-mounted-single-extension-name',
  configurable: false,
  watchable: true,
  multipleMounts: false,
  source: 'network'
};

// A providing extension which has mounted a file system and supports mounting
// more.
var MOUNTED_MULTIPLE_PROVIDING_EXTENSION = {
  providerId: 'mounted-multiple-provider-id',
  iconSet: {
    icon16x16Url: 'chrome://mounted-multiple-extension-id-16.jpg',
    icon32x32Url: 'chrome://mounted-multiple-extension-id-32.jpg'
  },
  name: 'mounted-multiple-extension-name',
  configurable: true,
  watchable: false,
  multipleMounts: true,
  source: 'network'
};

// A providing extension which has not mounted a file system but it's of "file"
// source. Such providers do mounting via file handlers.
var NOT_MOUNTED_FILE_PROVIDING_EXTENSION = {
  providerId: 'file-provider-id',
  iconSet: {
    icon16x16Url: 'chrome://file-extension-id-16.jpg',
    icon32x32Url: 'chrome://file-extension-id-32.jpg'
  },
  name: 'file-extension-name',
  configurable: false,
  watchable: true,
  multipleMounts: true,
  source: 'file'
};

// A providing extension which has not mounted a file system but it's of
// "device" source. Such providers are not mounted by user, but automatically
// when the device is attached.
var NOT_MOUNTED_DEVICE_PROVIDING_EXTENSION = {
  providerId: 'device-provider-id',
  iconSet: {
    icon16x16Url: 'chrome://device-extension-id-16.jpg',
    icon32x32Url: 'chrome://device-extension-id-32.jpg'
  },
  name: 'device-extension-name',
  configurable: false,
  watchable: true,
  multipleMounts: true,
  source: 'device'
};

var volumeManager = null;

function addProvidedVolume(volumeManager, providerId, volumeId) {
  var fileSystem = new MockFileSystem(volumeId, 'filesystem:' + volumeId);
  fileSystem.entries['/'] = new MockDirectoryEntry(fileSystem, '');

  var volumeInfo = new VolumeInfoImpl(
      VolumeManagerCommon.VolumeType.PROVIDED, volumeId, fileSystem,
      '',                                         // error
      '',                                         // deviceType
      '',                                         // devicePath
      false,                                      // isReadonly
      false,                                      // isReadonlyRemovableDevice
      {isCurrentProfile: true, displayName: ''},  // profile
      '',                                         // label
      providerId,                                 // providerId
      false,                                      // hasMedia
      false,                                      // configurable
      false,                                      // watchable
      'network',                                  // source
      '',                                         // diskFileSystemType
      {});                                        // iconSet

  volumeManager.volumeInfoList.push(volumeInfo);
}

function setUp() {
  // Create a dummy API for fetching a list of providers.
  // TODO(mtomasz): Add some native (non-extension) providers.
  chrome.fileManagerPrivate = {
    getProviders: function(callback) {
      callback([MOUNTED_SINGLE_PROVIDING_EXTENSION,
                NOT_MOUNTED_SINGLE_PROVIDING_EXTENSION,
                MOUNTED_MULTIPLE_PROVIDING_EXTENSION,
                NOT_MOUNTED_FILE_PROVIDING_EXTENSION,
                NOT_MOUNTED_DEVICE_PROVIDING_EXTENSION]);
    }
  };
  new MockCommandLinePrivate();
  chrome.runtime = {};

  // Create a dummy volume manager.
  volumeManager = new MockVolumeManagerWrapper();
  addProvidedVolume(
      volumeManager, MOUNTED_SINGLE_PROVIDING_EXTENSION.providerId, 'volume-1');
  addProvidedVolume(
      volumeManager, MOUNTED_MULTIPLE_PROVIDING_EXTENSION.providerId,
      'volume-2');
}

function testGetInstalledProviders(callback) {
  var model = new ProvidersModel(volumeManager);
  reportPromise(
      model.getInstalledProviders().then(function(providers) {
        assertEquals(5, providers.length);
        assertEquals(
            MOUNTED_SINGLE_PROVIDING_EXTENSION.providerId,
            providers[0].providerId);
        assertEquals(
            MOUNTED_SINGLE_PROVIDING_EXTENSION.iconSet, providers[0].iconSet);
        assertEquals(
            MOUNTED_SINGLE_PROVIDING_EXTENSION.name, providers[0].name);
        assertEquals(
            MOUNTED_SINGLE_PROVIDING_EXTENSION.configurable,
            providers[0].configurable);
        assertEquals(
            MOUNTED_SINGLE_PROVIDING_EXTENSION.watchable,
            providers[0].watchable);
        assertEquals(
            MOUNTED_SINGLE_PROVIDING_EXTENSION.multipleMounts,
            providers[0].multipleMounts);
        assertEquals(
            MOUNTED_SINGLE_PROVIDING_EXTENSION.source, providers[0].source);

        assertEquals(
            NOT_MOUNTED_SINGLE_PROVIDING_EXTENSION.providerId,
            providers[1].providerId);
        assertEquals(
            MOUNTED_MULTIPLE_PROVIDING_EXTENSION.providerId,
            providers[2].providerId);
        assertEquals(
            NOT_MOUNTED_FILE_PROVIDING_EXTENSION.providerId,
            providers[3].providerId);
        assertEquals(
            NOT_MOUNTED_DEVICE_PROVIDING_EXTENSION.providerId,
            providers[4].providerId);

        assertEquals(
            NOT_MOUNTED_SINGLE_PROVIDING_EXTENSION.iconSet,
            providers[1].iconSet);
        assertEquals(
            MOUNTED_MULTIPLE_PROVIDING_EXTENSION.iconSet, providers[2].iconSet);
        assertEquals(
            NOT_MOUNTED_FILE_PROVIDING_EXTENSION.iconSet, providers[3].iconSet);
        assertEquals(
            NOT_MOUNTED_DEVICE_PROVIDING_EXTENSION.iconSet,
            providers[4].iconSet);
      }),
      callback);
}

function testGetMountableProviders(callback) {
  var model = new ProvidersModel(volumeManager);
  reportPromise(
      model.getMountableProviders().then(function(providers) {
        assertEquals(2, providers.length);
        assertEquals(
            NOT_MOUNTED_SINGLE_PROVIDING_EXTENSION.providerId,
            providers[0].providerId);
        assertEquals(
            MOUNTED_MULTIPLE_PROVIDING_EXTENSION.providerId,
            providers[1].providerId);
      }),
      callback);
}
