// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
'use strict';

/**
 * Dummy private APIs.
 */
var chrome;

/**
 * Callbacks registered by setTimeout.
 * @type {Array<function>}
 */
var timeoutCallbacks;

/** @type {!MockVolumeManager} */
var volumeManager;

/** @type {DeviceHandler} */
var handler;

// Set up string assets.
loadTimeData.data = {
  REMOVABLE_DEVICE_DETECTION_TITLE: 'Device detected',
  REMOVABLE_DEVICE_NAVIGATION_MESSAGE: 'DEVICE_NAVIGATION',
  REMOVABLE_DEVICE_NAVIGATION_BUTTON_LABEL: '',
  REMOVABLE_DEVICE_IMPORT_MESSAGE: 'DEVICE_IMPORT',
  REMOVABLE_DEVICE_IMPORT_BUTTON_LABEL: '',
  DEVICE_UNKNOWN_BUTTON_LABEL: 'DEVICE_UNKNOWN_BUTTON_LABEL',
  DEVICE_UNKNOWN_MESSAGE: 'DEVICE_UNKNOWN: $1',
  DEVICE_UNKNOWN_DEFAULT_MESSAGE: 'DEVICE_UNKNOWN_DEFAULT_MESSAGE',
  DEVICE_UNSUPPORTED_MESSAGE: 'DEVICE_UNSUPPORTED: $1',
  DEVICE_HARD_UNPLUGGED_TITLE: 'DEVICE_HARD_UNPLUGGED_TITLE',
  DEVICE_HARD_UNPLUGGED_MESSAGE: 'DEVICE_HARD_UNPLUGGED_MESSAGE',
  DOWNLOADS_DIRECTORY_LABEL: 'DOWNLOADS_DIRECTORY_LABEL',
  DRIVE_DIRECTORY_LABEL: 'DRIVE_DIRECTORY_LABEL',
  MULTIPART_DEVICE_UNSUPPORTED_MESSAGE: 'MULTIPART_DEVICE_UNSUPPORTED: $1',
  EXTERNAL_STORAGE_DISABLED_MESSAGE: 'EXTERNAL_STORAGE_DISABLED',
  FORMATTING_OF_DEVICE_PENDING_TITLE: 'FORMATTING_OF_DEVICE_PENDING_TITLE',
  FORMATTING_OF_DEVICE_PENDING_MESSAGE: 'FORMATTING_OF_DEVICE_PENDING',
  FORMATTING_OF_DEVICE_FINISHED_TITLE: 'FORMATTING_OF_DEVICE_FINISHED_TITLE',
  FORMATTING_FINISHED_SUCCESS_MESSAGE: 'FORMATTING_FINISHED_SUCCESS',
  FORMATTING_OF_DEVICE_FAILED_TITLE: 'FORMATTING_OF_DEVICE_FAILED_TITLE',
  FORMATTING_FINISHED_FAILURE_MESSAGE: 'FORMATTING_FINISHED_FAILURE',
  RENAMING_OF_DEVICE_FAILED_TITLE: 'RENAMING_OF_DEVICE_FAILED_TITLE',
  RENAMING_OF_DEVICE_FINISHED_FAILURE_MESSAGE:
      'RENAMING_OF_DEVICE_FINISHED_FAILURE',
};

// Set up the test components.
function setUp() {
  new MockCommandLinePrivate();
  volumeManager = new MockVolumeManager();
  MockVolumeManager.installMockSingleton(volumeManager);

  setupChromeApis();

  handler = new DeviceHandler();
}

function setUpInIncognitoContext() {
  chrome.extension.inIncognitoContext = true;
}

function testGoodDevice(callback) {
  chrome.fileManagerPrivate.onMountCompleted.dispatch({
    eventType: 'mount',
    status: 'success',
    volumeMetadata: {
      isParentDevice: true,
      deviceType: 'usb',
      devicePath: '/device/path',
      deviceLabel: 'label'
    },
    shouldNotify: true
  });

  reportPromise(
      chrome.notifications.resolver.promise.then(
          function(notifications) {
            assertEquals(1, Object.keys(notifications).length);
            var options = notifications['deviceNavigation:/device/path'];
            assertEquals('DEVICE_NAVIGATION', options.message);
            assertTrue(options.isClickable);
          }),
      callback);
}

function testRemovableMediaDeviceWithImportEnabled(callback) {
  var storage = new MockChromeStorageAPI();

  setupFileSystem(
      VolumeManagerCommon.VolumeType.REMOVABLE,
      'blabbity',
      [
        '/DCIM/',
        '/DCIM/grandma.jpg'
      ]);

  var resolver = new importer.Resolver();

  // Handle media device navigation requests.
  handler.addEventListener(
      DeviceHandler.VOLUME_NAVIGATION_REQUESTED,
      function(event) {
        resolver.resolve(event);
      });

  chrome.fileManagerPrivate.onMountCompleted.dispatch({
    eventType: 'mount',
    status: 'success',
    volumeMetadata: {
      volumeId: 'blabbity',
      deviceType: 'usb'
    },
    shouldNotify: true
  });

  reportPromise(
      resolver.promise.then(
          function(event) {
            assertEquals('blabbity', event.volumeId);
          }),
      callback);
}

function testMtpMediaDeviceWithImportEnabled(callback) {
  var storage = new MockChromeStorageAPI();

  setupFileSystem(
      VolumeManagerCommon.VolumeType.MTP,
      'blabbity',
      [
        '/dcim/',
        '/dcim/grandpa.jpg'
      ]);

  var resolver = new importer.Resolver();

  // Handle media device navigation requests.
  handler.addEventListener(
      DeviceHandler.VOLUME_NAVIGATION_REQUESTED,
      function(event) {
        resolver.resolve(event);
      });

  chrome.fileManagerPrivate.onMountCompleted.dispatch({
    eventType: 'mount',
    status: 'success',
    volumeMetadata: {
      volumeId: 'blabbity',
      deviceType: 'mtp'
    },
    shouldNotify: true
  });

  reportPromise(
      resolver.promise.then(
          function(event) {
            assertEquals('blabbity', event.volumeId);
          }),
      callback);
}

function testMediaDeviceWithImportDisabled(callback) {
  chrome.commandLinePrivate.cloudImportDisabled = true;

  chrome.fileManagerPrivate.onMountCompleted.dispatch({
    eventType: 'mount',
    status: 'success',
    volumeMetadata: {
      isParentDevice: true,
      deviceType: 'usb',
      devicePath: '/device/path',
      deviceLabel: 'label',
      hasMedia: true
    },
    shouldNotify: true
  });

  reportPromise(
      chrome.notifications.resolver.promise.then(
          function(notifications) {
            assertEquals(1, Object.keys(notifications).length);
            assertEquals(
                'DEVICE_NAVIGATION',
                notifications[
                    'deviceNavigation:/device/path'].message,
                'Device notification did not have the right message.');
          }),
      callback);
}

function testGoodDeviceNotNavigated() {
  chrome.fileManagerPrivate.onMountCompleted.dispatch({
    eventType: 'mount',
    status: 'success',
    volumeMetadata: {
      isParentDevice: true,
      deviceType: 'usb',
      devicePath: '/device/path',
      deviceLabel: 'label'
    },
    shouldNotify: false
  });

  assertEquals(0, Object.keys(chrome.notifications.items).length);
  assertFalse(chrome.notifications.resolver.settled);
}

function testGoodDeviceWithBadParent(callback) {
  chrome.fileManagerPrivate.onMountCompleted.dispatch({
    eventType: 'mount',
    status: 'error_internal',
    volumeMetadata: {
      isParentDevice: true,
      deviceType: 'usb',
      devicePath: '/device/path',
      deviceLabel: 'label'
    },
    shouldNotify: true
  });

  reportPromise(
      chrome.notifications.resolver.promise.then(
          function(notifications) {
            assertFalse(!!notifications['device:/device/path']);
            assertEquals(
                'DEVICE_UNKNOWN: label',
                notifications['deviceFail:/device/path'].message);
          }),
      callback);
}

function testGoodDeviceWithBadParent_DuplicateMount(callback) {
  chrome.fileManagerPrivate.onMountCompleted.dispatch({
    eventType: 'mount',
    status: 'success',
    volumeMetadata: {
      isParentDevice: false,
      deviceType: 'usb',
      devicePath: '/device/path',
      deviceLabel: 'label'
    },
    shouldNotify: true
  });

  // Mounting the same device repeatedly should produce only
  // a single notification.
  chrome.fileManagerPrivate.onMountCompleted.dispatch({
    eventType: 'mount',
    status: 'success',
    volumeMetadata: {
      isParentDevice: false,
      deviceType: 'usb',
      devicePath: '/device/path',
      deviceLabel: 'label'
    },
    shouldNotify: true
  });

  reportPromise(
      chrome.notifications.resolver.promise.then(
          function(notifications) {
            assertEquals(1, Object.keys(notifications).length);
            assertEquals(
                'DEVICE_NAVIGATION',
                notifications['deviceNavigation:/device/path'].message);
          }),
      callback);
}

function testUnsupportedDevice(callback) {
  chrome.fileManagerPrivate.onMountCompleted.dispatch({
    eventType: 'mount',
    status: 'error_unsupported_filesystem',
    volumeMetadata: {
      isParentDevice: false,
      deviceType: 'usb',
      devicePath: '/device/path',
      deviceLabel: 'label'
    },
    shouldNotify: true
  });

  reportPromise(
      chrome.notifications.resolver.promise.then(
          function(notifications) {
            assertFalse(!!chrome.notifications.items['device:/device/path']);
            assertEquals(
                'DEVICE_UNSUPPORTED: label',
                chrome.notifications.items['deviceFail:/device/path'].message);
          }),
      callback);
}

function testUnknownDevice(callback) {
  // Emulate adding a device which has unknown filesystem.
  chrome.fileManagerPrivate.onMountCompleted.dispatch({
    eventType: 'mount',
    status: 'error_unknown_filesystem',
    volumeMetadata: {
      isParentDevice: false,
      isReadOnly: false,
      deviceType: 'usb',
      devicePath: '/device/path',
    },
    shouldNotify: true
  });

  reportPromise(
      chrome.notifications.resolver.promise.then(
          function(notifications) {
            assertFalse(!!chrome.notifications.items['device:/device/path']);
            var item = chrome.notifications.items['deviceFail:/device/path'];
            assertEquals('DEVICE_UNKNOWN_DEFAULT_MESSAGE', item.message);
            // "Format device" button should appear.
            assertEquals('DEVICE_UNKNOWN_BUTTON_LABEL', item.buttons[0].title);
          }),
      callback);
}

function testUnknownReadonlyDevice(callback) {
  // Emulate adding a device which has unknown filesystem but is read-only.
  chrome.fileManagerPrivate.onMountCompleted.dispatch({
    eventType: 'mount',
    status: 'error_unknown_filesystem',
    volumeMetadata: {
      isParentDevice: true,
      isReadOnly: true,
      deviceType: 'sd',
      devicePath: '/device/path',
    },
    shouldNotify: true
  });

  reportPromise(
      chrome.notifications.resolver.promise.then(
          function(notifications) {
            assertFalse(!!chrome.notifications.items['device:/device/path']);
            var item = chrome.notifications.items['deviceFail:/device/path'];
            assertEquals('DEVICE_UNKNOWN_DEFAULT_MESSAGE', item.message);
            // "Format device" button should not appear.
            assertFalse(!!item.buttons);
          }),
      callback);
}

function testUnsupportedWithUnknownParentReplacesNotification() {
  chrome.fileManagerPrivate.onMountCompleted.dispatch({
    eventType: 'mount',
    status: 'error_internal',
    volumeMetadata: {
      isParentDevice: true,
      deviceType: 'usb',
      devicePath: '/device/path',
      deviceLabel: 'label'
    },
    shouldNotify: true
  });

  assertEquals(
      'DEVICE_UNKNOWN: label',
      chrome.notifications.items['deviceFail:/device/path'].message);

  chrome.fileManagerPrivate.onMountCompleted.dispatch({
    eventType: 'mount',
    status: 'error_unsupported_filesystem',
    volumeMetadata: {
      isParentDevice: false,
      deviceType: 'usb',
      devicePath: '/device/path',
      deviceLabel: 'label'
    },
    shouldNotify: true
  });

  assertEquals(1, Object.keys(chrome.notifications.items).length);
  assertEquals(
      'DEVICE_UNSUPPORTED: label',
      chrome.notifications.items['deviceFail:/device/path'].message);
}

function testMountPartialSuccess(callback) {
  chrome.fileManagerPrivate.onMountCompleted.dispatch({
    eventType: 'mount',
    status: 'success',
    volumeMetadata: {
      isParentDevice: false,
      deviceType: 'usb',
      devicePath: '/device/path',
      deviceLabel: 'label'
    },
    shouldNotify: true
  });

  reportPromise(
      chrome.notifications.resolver.promise.then(
          function(notifications) {
            assertEquals(1, Object.keys(notifications).length);
            assertEquals(
                'DEVICE_NAVIGATION',
                notifications['deviceNavigation:/device/path'].message);
          }).then(function() {
            chrome.fileManagerPrivate.onMountCompleted.dispatch({
              eventType: 'mount',
              status: 'error_unsupported_filesystem',
              volumeMetadata: {
                isParentDevice: false,
                deviceType: 'usb',
                devicePath: '/device/path',
                deviceLabel: 'label'
              },
              shouldNotify: true
            });
          }).then(function() {
            var notifications = chrome.notifications.items;
            assertEquals(
                2, Object.keys(notifications).length);
            assertEquals(
                'MULTIPART_DEVICE_UNSUPPORTED: label',
                notifications['deviceFail:/device/path'].message);
          }),
      callback);
}

function testUnknown(callback) {
  chrome.fileManagerPrivate.onMountCompleted.dispatch({
    eventType: 'mount',
    status: 'error_unknown',
    volumeMetadata: {
      isParentDevice: false,
      deviceType: 'usb',
      devicePath: '/device/path',
      deviceLabel: 'label'
    },
    shouldNotify: true
  });

  reportPromise(
      chrome.notifications.resolver.promise.then(
          function(notifications) {
            assertEquals(1, Object.keys(notifications).length);
            assertEquals(
                'DEVICE_UNKNOWN: label',
                notifications['deviceFail:/device/path'].message);
          }),
      callback);
}

function testNonASCIILabel(callback) {
  chrome.fileManagerPrivate.onMountCompleted.dispatch({
    eventType: 'mount',
    status: 'error_internal',
    volumeMetadata: {
      isParentDevice: false,
      deviceType: 'usb',
      devicePath: '/device/path',
      // "RA (U+30E9) BE (U+30D9) RU (U+30EB)" in Katakana letters.
      deviceLabel: '\u30E9\u30D9\u30EB'
    },
    shouldNotify: true
  });

  reportPromise(
      chrome.notifications.resolver.promise.then(
          function(notifications) {
            assertEquals(1, Object.keys(notifications).length);
            assertEquals(
                'DEVICE_UNKNOWN: \u30E9\u30D9\u30EB',
                notifications['deviceFail:/device/path'].message);
          }),
      callback);
}

function testMulitpleFail() {
  // The first parent error.
  chrome.fileManagerPrivate.onMountCompleted.dispatch({
    eventType: 'mount',
    status: 'error_internal',
    volumeMetadata: {
      isParentDevice: true,
      deviceType: 'usb',
      devicePath: '/device/path',
      deviceLabel: 'label'
    },
    shouldNotify: true
  });
  assertEquals(1, Object.keys(chrome.notifications.items).length);
  assertEquals(
      'DEVICE_UNKNOWN: label',
      chrome.notifications.items['deviceFail:/device/path'].message);

  // The first child error that replaces the parent error.
  chrome.fileManagerPrivate.onMountCompleted.dispatch({
    eventType: 'mount',
    status: 'error_internal',
    volumeMetadata: {
      isParentDevice: false,
      deviceType: 'usb',
      devicePath: '/device/path',
      deviceLabel: 'label'
    },
    shouldNotify: true
  });
  assertEquals(1, Object.keys(chrome.notifications.items).length);
  assertEquals(
      'DEVICE_UNKNOWN: label',
      chrome.notifications.items['deviceFail:/device/path'].message);

  // The second child error that turns to a multi-partition error.
  chrome.fileManagerPrivate.onMountCompleted.dispatch({
    eventType: 'mount',
    status: 'error_internal',
    volumeMetadata: {
      isParentDevice: false,
      deviceType: 'usb',
      devicePath: '/device/path',
      deviceLabel: 'label'
    },
    shouldNotify: true
  });
  assertEquals(1, Object.keys(chrome.notifications.items).length);
  assertEquals(
      'MULTIPART_DEVICE_UNSUPPORTED: label',
      chrome.notifications.items['deviceFail:/device/path'].message);

  // The third child error that should be ignored because the error message does
  // not changed.
  chrome.fileManagerPrivate.onMountCompleted.dispatch({
    eventType: 'mount',
    status: 'error_internal',
    volumeMetadata: {
      isParentDevice: false,
      deviceType: 'usb',
      devicePath: '/device/path',
      deviceLabel: 'label'
    },
    shouldNotify: true
  });
  assertEquals(1, Object.keys(chrome.notifications.items).length);
  assertEquals(
      'MULTIPART_DEVICE_UNSUPPORTED: label',
      chrome.notifications.items['deviceFail:/device/path'].message);
}

function testDisabledDevice() {
  chrome.fileManagerPrivate.onDeviceChanged.dispatch({
    type: 'disabled',
    devicePath: '/device/path'
  });
  assertEquals(1, Object.keys(chrome.notifications.items).length);
  assertEquals('EXTERNAL_STORAGE_DISABLED',
               chrome.notifications.items['deviceFail:/device/path'].message);

  chrome.fileManagerPrivate.onDeviceChanged.dispatch({
    type: 'removed',
    devicePath: '/device/path'
  });
  assertEquals(0, Object.keys(chrome.notifications.items).length);
}

function testFormatSucceeded() {
  chrome.fileManagerPrivate.onDeviceChanged.dispatch({
    type: 'format_start',
    devicePath: '/device/path'
  });
  assertEquals(1, Object.keys(chrome.notifications.items).length);
  assertEquals('FORMATTING_OF_DEVICE_PENDING',
               chrome.notifications.items['formatStart:/device/path'].message);

  chrome.fileManagerPrivate.onDeviceChanged.dispatch({
    type: 'format_success',
    devicePath: '/device/path'
  });
  assertEquals(1, Object.keys(chrome.notifications.items).length);
  assertEquals('FORMATTING_FINISHED_SUCCESS',
               chrome.notifications.items[
                   'formatSuccess:/device/path'].message);
}

function testFormatFailed() {
  chrome.fileManagerPrivate.onDeviceChanged.dispatch({
    type: 'format_start',
    devicePath: '/device/path'
  });
  assertEquals(1, Object.keys(chrome.notifications.items).length);
  assertEquals('FORMATTING_OF_DEVICE_PENDING',
               chrome.notifications.items['formatStart:/device/path'].message);

  chrome.fileManagerPrivate.onDeviceChanged.dispatch({
    type: 'format_fail',
    devicePath: '/device/path'
  });
  assertEquals(1, Object.keys(chrome.notifications.items).length);
  assertEquals('FORMATTING_FINISHED_FAILURE',
               chrome.notifications.items['formatFail:/device/path'].message);
}

function testRenameSucceeded() {
  chrome.fileManagerPrivate.onDeviceChanged.dispatch(
      {type: 'rename_start', devicePath: '/device/path'});
  assertEquals(0, Object.keys(chrome.notifications.items).length);

  chrome.fileManagerPrivate.onDeviceChanged.dispatch(
      {type: 'rename_success', devicePath: '/device/path'});
  assertEquals(0, Object.keys(chrome.notifications.items).length);
}

function testRenameFailed() {
  chrome.fileManagerPrivate.onDeviceChanged.dispatch(
      {type: 'rename_start', devicePath: '/device/path'});
  assertEquals(0, Object.keys(chrome.notifications.items).length);

  chrome.fileManagerPrivate.onDeviceChanged.dispatch(
      {type: 'rename_fail', devicePath: '/device/path'});
  assertEquals(1, Object.keys(chrome.notifications.items).length);
  assertEquals(
      'RENAMING_OF_DEVICE_FINISHED_FAILURE',
      chrome.notifications.items['renameFail:/device/path'].message);
}

function testDeviceHardUnplugged() {
  chrome.fileManagerPrivate.onDeviceChanged.dispatch({
    type: 'hard_unplugged',
    devicePath: '/device/path'
  });
  assertEquals(1, Object.keys(chrome.notifications.items).length);
  assertEquals('DEVICE_HARD_UNPLUGGED_MESSAGE',
               chrome.notifications.items[
                   'hardUnplugged:/device/path'].message);
}

function testNotificationClicked(callback) {
  var devicePath = '/device/path';
  var notificationId = 'deviceNavigation:' + devicePath;

  // Add a listener for navigation-requested events.
  var resolver = new importer.Resolver();
  handler.addEventListener(
      DeviceHandler.VOLUME_NAVIGATION_REQUESTED,
      function(event) {
        resolver.resolve(event);
      });

  // Call the notification-body-clicked handler and check that the
  // navigation-requested event is dispatched.
  chrome.notifications.onClicked.dispatch(notificationId);
  reportPromise(
      resolver.promise.then(
          function(event) {
            assertEquals(null, event.volumeId);
            assertEquals(devicePath, event.devicePath);
            assertEquals(null, event.filePath);
          }),
      callback);
}

function testMiscMessagesInIncognito() {
  setUpInIncognitoContext();
  chrome.fileManagerPrivate.onDeviceChanged.dispatch(
      {type: 'format_start', devicePath: '/device/path'});
  // No notification sent by this instance in incognito context.
  assertEquals(0, Object.keys(chrome.notifications.items).length);
  assertFalse(chrome.notifications.resolver.settled);
}

function testMountCompleteInIncognito() {
  setUpInIncognitoContext();
  chrome.fileManagerPrivate.onMountCompleted.dispatch({
    eventType: 'mount',
    status: 'success',
    volumeMetadata: {
      isParentDevice: false,
      deviceType: 'usb',
      devicePath: '/device/path',
      deviceLabel: 'label'
    },
    shouldNotify: true
  });

  assertEquals(0, Object.keys(chrome.notifications.items).length);
  // TODO(yamaguchi): I think this test is incomplete.
  // This looks as if notification is not generated yet because the promise
  // is not settled yet. Same for testGoodDeviceNotNavigated.
  assertFalse(chrome.notifications.resolver.settled);
}

/**
 * @param {!VolumeManagerCommon.VolumeType} volumeType
 * @param {string} volumeId
 * @param {!Array<string>} fileNames
 * @return {!VolumeInfo}
 */
function setupFileSystem(volumeType, volumeId, fileNames) {
  var volumeInfo = volumeManager.createVolumeInfo(
      volumeType, volumeId, 'A volume known as ' + volumeId);
  assertTrue(!!volumeInfo);
  volumeInfo.fileSystem.populate(fileNames);
  return volumeInfo;
}

function setupChromeApis() {
  // Make dummy APIs.
  chrome = {
    commandLinePrivate: {
      hasSwitch: function(switchName, callback) {
        if (switchName === 'disable-cloud-import') {
          callback(chrome.commandLinePrivate.cloudImportDisabled);
        }
      },
      cloudImportDisabled: false
    },
    extension: {inIncognitoContext: false},
    fileManagerPrivate: {
      onDeviceChanged: {
        addListener: function(listener) {
          this.dispatch = listener;
        }
      },
      onMountCompleted: {
        addListener: function(listener) {
          this.dispatch = listener;
        }
      },
      getProfiles: function(callback) {
        callback([{profileId: 'userid@xyz.domain.org'}]);
      }
    },
    i18n: {
      getUILanguage: function() {
        return 'en-US';
      }
    },
    notifications: {
      resolver: new importer.Resolver(),
      create: function(id, params, callback) {
        this.promise = this.resolver.promise;
        this.items[id] = params;
        if (!this.resolver.settled) {
          this.resolver.resolve(this.items);
        }
        callback();
      },
      clear: function(id, callback) {
        delete this.items[id];
        callback();
      },
      items: {},
      onButtonClicked: {
        addListener: function(listener) {
          this.dispatch = listener;
        }
      },
      onClicked: {
        addListener: function(listener) {
          this.dispatch = listener;
        }
      },
      getAll: function(callback) {
        callback([]);
      }
    },
    runtime: {
      getURL: function(path) {
        return path;
      },
      onStartup: {addListener: function() {}}
    }
  };
}
