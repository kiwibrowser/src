// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

window.metrics = {
  recordEnum: function() {}
};

var mockTaskHistory = {
  getLastExecutedTime: function(id) {
    return 0;
  }
};

loadTimeData.data = {
  NO_TASK_FOR_EXECUTABLE: 'NO_TASK_FOR_EXECUTABLE',
  NO_TASK_FOR_FILE_URL: 'NO_TASK_FOR_FILE_URL',
  NO_TASK_FOR_FILE: 'NO_TASK_FOR_FILE',
  NO_TASK_FOR_DMG: 'NO_TASK_FOR_DMG',
  NO_TASK_FOR_CRX: 'NO_TASK_FOR_CRX',
  NO_TASK_FOR_CRX_TITLE: 'NO_TASK_FOR_CRX_TITLE',
  OPEN_WITH_BUTTON_LABEL: 'OPEN_WITH_BUTTON_LABEL',
  TASK_OPEN: 'TASK_OPEN',
  MORE_ACTIONS_BUTTON_LABEL: 'MORE_ACTIONS_BUTTON_LABEL'
};

function setUp() {
  window.chrome = {
    commandLinePrivate: {
      hasSwitch: function(name, callback) {
        callback(false);
      }
    },
    fileManagerPrivate: {
      getFileTasks: function(entries, callback) {
        setTimeout(
            callback.bind(null, [{
                            taskId: 'handler-extension-id|app|any',
                            isDefault: false,
                            isGenericFileHandler: true
                          }]),
            0);
      },
      executeTask: function(taskId, entries, onViewFiles) {
        onViewFiles('failed');
      }
    },
    runtime: {id: 'test-extension-id'}
  };
}

/**
 * Returns a mock file manager.
 * @return {!FileManager}
 */
function getMockFileManager() {
  return {
    volumeManager: {
      getDriveConnectionState: function() {
        return VolumeManagerCommon.DriveConnectionType.ONLINE;
      },
      getVolumeInfo: function() {
        return {
          volumeType: VolumeManagerCommon.VolumeType.DRIVE
        };
      }
    },
    ui: {
      alertDialog: {
        showHtml: function(title, text, onOk, onCancel, onShow) {}
      }
    },
    metadataModel: {},
    directoryModel: {
      getCurrentRootType: function() {
        return null;
      }
    }
  };
}

/**
 * Returns a promise which is resolved when showHtml of alert dialog is called
 * with expected title and text.
 *
 * @param {!Array<!Entry>} entries Entries.
 * @param {string} expectedTitle An expected title.
 * @param {string} expectedText An expected text.
 * @return {!Promise}
 */
function showHtmlOfAlertDialogIsCalled(entries, expectedTitle, expectedText) {
  return new Promise(function(resolve, reject) {
    var fileManager = getMockFileManager();
    fileManager.ui.alertDialog.showHtml =
        function(title, text, onOk, onCancel, onShow) {
          assertEquals(expectedTitle, title);
          assertEquals(expectedText, text);
          resolve();
        };

    FileTasks
        .create(
            fileManager.volumeManager, fileManager.metadataModel,
            fileManager.directoryModel, fileManager.ui, entries, [null],
            mockTaskHistory)
        .then(function(tasks) {
          tasks.executeDefault();
        });
  });
}

/**
 * Returns a promise which is resolved when openSuggestAppsDialog is called.
 *
 * @param {!Array<!Entry>} entries Entries.
 * @param {!Array<?string>} mimeTypes Mime types.
 * @return {!Promise}
 */
function openSuggestAppsDialogIsCalled(entries, mimeTypes) {
  return new Promise(function(resolve, reject) {
    var fileManager = getMockFileManager();
    fileManager.ui.suggestAppsDialog = {
      showByExtensionAndMime: function(extension, mimeType, onDialogClosed) {
        resolve();
      }
    };

    FileTasks
        .create(
            fileManager.volumeManager, fileManager.metadataModel,
            fileManager.directoryModel, fileManager.ui, entries, mimeTypes,
            mockTaskHistory)
        .then(function(tasks) {
          tasks.executeDefault();
        });
  });
}

/**
 * Returns a promise which is resolved when task picker is shown.
 *
 * @param {!Array<!Entry>} entries Entries.
 * @param {!Array<?string>} mimeTypes Mime types.
 * @return {!Promise}
 */
function showDefaultTaskDialogCalled(entries, mimeTypes) {
  return new Promise(function(resolve, reject) {
    var fileManager = getMockFileManager();
    fileManager.ui.defaultTaskPicker = {
      showDefaultTaskDialog: function(
          title, message, items, defaultIdx, onSuccess) {
        resolve();
      }
    };

    FileTasks
        .create(
            fileManager.volumeManager, fileManager.metadataModel,
            fileManager.directoryModel, fileManager.ui, entries, mimeTypes,
            mockTaskHistory)
        .then(function(tasks) {
          tasks.executeDefault();
        });
  });
}

function testToOpenExeFile(callback) {
  var mockFileSystem = new MockFileSystem('volumeId');
  var mockEntry = new MockFileEntry(mockFileSystem, '/test.exe');

  reportPromise(showHtmlOfAlertDialogIsCalled(
      [mockEntry], 'test.exe', 'NO_TASK_FOR_EXECUTABLE'), callback);
}

function testToOpenDmgFile(callback) {
  var mockFileSystem = new MockFileSystem('volumeId');
  var mockEntry = new MockFileEntry(mockFileSystem, '/test.dmg');

  reportPromise(showHtmlOfAlertDialogIsCalled(
      [mockEntry], 'test.dmg', 'NO_TASK_FOR_DMG'), callback);
}

function testToOpenCrxFile(callback) {
  var mockFileSystem = new MockFileSystem('volumeId');
  var mockEntry = new MockFileEntry(mockFileSystem, '/test.crx');

  reportPromise(showHtmlOfAlertDialogIsCalled(
      [mockEntry], 'NO_TASK_FOR_CRX_TITLE', 'NO_TASK_FOR_CRX'), callback);
}

function testToOpenRtfFile(callback) {
  var mockFileSystem = new MockFileSystem('volumeId');
  var mockEntry = new MockFileEntry(mockFileSystem, '/test.rtf');

  reportPromise(openSuggestAppsDialogIsCalled(
      [mockEntry], ['application/rtf']), callback);
}

/**
 * Test case for openSuggestAppsDialog with an entry which has external type of
 * metadata.
 */
function testOpenSuggestAppsDialogWithMetadata(callback) {
  var showByExtensionAndMimeIsCalled = new Promise(function(resolve, reject) {
    var fileSystem = new MockFileSystem('volumeId');
    var entry = new MockFileEntry(fileSystem, '/test.rtf');

    FileTasks
        .create(
            {
              getDriveConnectionState: function() {
                return VolumeManagerCommon.DriveConnectionType.ONLINE;
              }
            },
            {}, {}, {
              taskMenuButton: document.createElement('button'),
              fileContextMenu:
                  {defaultActionMenuItem: document.createElement('div')},
              suggestAppsDialog: {
                showByExtensionAndMime: function(
                    extension, mimeType, onDialogClosed) {
                  assertEquals('.rtf', extension);
                  assertEquals('application/rtf', mimeType);
                  resolve();
                }
              }
            },
            [entry], ['application/rtf'], mockTaskHistory)
        .then(function(tasks) {
          tasks.openSuggestAppsDialog(
              function() {}, function() {}, function() {});
        });
  });

  reportPromise(showByExtensionAndMimeIsCalled, callback);
}

/**
 * Test case for openSuggestAppsDialog with an entry which doesn't have
 * extension. Since both extension and MIME type are required for
 * openSuggestAppsDialogopen, onFalure should be called for this test case.
 */
function testOpenSuggestAppsDialogFailure(callback) {
  var onFailureIsCalled = new Promise(function(resolve, reject) {
    var fileSystem = new MockFileSystem('volumeId');
    var entry = new MockFileEntry(fileSystem, '/test');

    FileTasks
        .create(
            {
              getDriveConnectionState: function() {
                return VolumeManagerCommon.DriveConnectionType.ONLINE;
              }
            },
            {}, {}, {
              taskMenuButton: document.createElement('button'),
              fileContextMenu:
                  {defaultActionMenuItem: document.createElement('div')}
            },
            [entry], [null], mockTaskHistory)
        .then(function(tasks) {
          tasks.openSuggestAppsDialog(function() {}, function() {}, resolve);
        });
  });

  reportPromise(onFailureIsCalled, callback);
}

/**
 * Test case for opening task picker with an entry which doesn't have default
 * app but multiple apps that can open it.
 */
function testOpenTaskPicker(callback) {
  window.chrome.fileManagerPrivate.getFileTasks = function(entries, callback) {
    setTimeout(
        callback.bind(
            null,
            [
              {
                taskId: 'handler-extension-id1|app|any',
                isDefault: false,
                isGenericFileHandler: false,
                title: 'app 1',
              },
              {
                taskId: 'handler-extension-id2|app|any',
                isDefault: false,
                isGenericFileHandler: false,
                title: 'app 2',
              }
            ]),
        0);
  };

  var mockFileSystem = new MockFileSystem('volumeId');
  var mockEntry = new MockFileEntry(mockFileSystem, '/test.tiff');

  reportPromise(
      showDefaultTaskDialogCalled([mockEntry], ['image/tiff']), callback);
}

function testOpenWithMostRecentlyExecuted(callback) {
  const latestTaskId = 'handler-extension-most-recently-executed|app|any';
  const oldTaskId = 'handler-extension-executed-before|app|any';
  window.chrome.fileManagerPrivate.getFileTasks = function(entries, callback) {
    setTimeout(
        callback.bind(
            null,
            // File tasks is sorted by last executed time, latest first.
            [
              {
                taskId: latestTaskId,
                isDefault: false,
                isGenericFileHandler: false,
                title: 'app 1',
              },
              {
                taskId: oldTaskId,
                isDefault: false,
                isGenericFileHandler: false,
                title: 'app 2',
              },
              {
                taskId: 'handler-extension-never-executed|app|any',
                isDefault: false,
                isGenericFileHandler: false,
                title: 'app 3',
              },
            ]),
        0);
  };
  var taskHistory = {
    getLastExecutedTime: function(id) {
      if (id == oldTaskId)
        return 10000;
      else if (id == latestTaskId)
        return 20000;
      return 0;
    },
    recordTaskExecuted: function(taskId) {}
  };
  var executedTask = null;
  window.chrome.fileManagerPrivate.executeTask = function(
      taskId, entries, onViewFiles) {
    executedTask = taskId;
    onViewFiles('success');
  };

  var mockFileSystem = new MockFileSystem('volumeId');
  var mockEntry = new MockFileEntry(mockFileSystem, '/test.tiff');
  var entries = [mockEntry];
  var promise = new Promise(function(resolve, reject) {
    var fileManager = getMockFileManager();
    fileManager.ui.defaultTaskPicker = {
      showDefaultTaskDialog: function(
          title, message, items, defaultIdx, onSuccess) {
        failWithMessage('should not show task picker');
      }
    };

    FileTasks
        .create(
            fileManager.volumeManager, fileManager.metadataModel,
            fileManager.directoryModel, fileManager.ui, [mockEntry], [null],
            taskHistory)
        .then(function(tasks) {
          tasks.executeDefault();
          assertEquals(latestTaskId, executedTask);
          resolve();
        });
  });
  reportPromise(promise, callback);
}

function testChooseZipArchiverOverZipUnpacker(callback) {
  var zipUnpackerTaskId = 'oedeeodfidgoollimchfdnbmhcpnklnd|app|zip';
  var zipArchiverTaskId = 'dmboannefpncccogfdikhmhpmdnddgoe|app|open';

  chrome.commandLinePrivate.hasSwitch = function(name, callback) {
    callback(name == 'enable-zip-archiver-unpacker');
  };

  window.chrome.fileManagerPrivate.getFileTasks = function(entries, callback) {
    setTimeout(
        callback.bind(
            null,
            [
              {
                taskId: zipArchiverTaskId,
                isDefault: false,
                isGenericFileHandler: false,
                title: 'Zip Archiver',
              },
              // Zip unpacker. Will be hidden because Zip Archiver is enabled.
              {
                taskId: zipUnpackerTaskId,
                isDefault: false,
                isGenericFileHandler: false,
                title: 'ZIP unpacker',
              },
            ]),
        0);
  };
  // None of the tasks has ever been executed.
  var taskHistory = {
    getLastExecutedTime: function(id) {
      return 0;
    },
    recordTaskExecuted: function(taskId) {}
  };
  var executedTask = null;
  window.chrome.fileManagerPrivate.executeTask = function(
      taskId, entries, onViewFiles) {
    executedTask = taskId;
    onViewFiles('success');
  };

  var mockFileSystem = new MockFileSystem('volumeId');
  var mockEntry = new MockFileEntry(mockFileSystem, '/test.zip');
  var promise = new Promise(function(resolve, reject) {
    var fileManager = getMockFileManager();
    fileManager.ui.defaultTaskPicker = {
      showDefaultTaskDialog: function(
          title, message, items, defaultIdx, onSuccess) {
        failWithMessage('run zip archiver', 'default task picker was shown');
      }
    };

    FileTasks
        .create(
            fileManager.volumeManager, fileManager.metadataModel,
            fileManager.directoryModel, fileManager.ui, [mockEntry], [null],
            taskHistory)
        .then(function(tasks) {
          tasks.executeDefault();
          assertEquals(zipArchiverTaskId, executedTask);
          resolve();
        });
  });
  reportPromise(promise, callback);
}
