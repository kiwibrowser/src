// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

window.metrics = {
  recordEnum: function() {}
};

function MockMetadataModel(properties) {
  this.properties_ = properties;
}

MockMetadataModel.prototype.get = function() {
  return Promise.resolve([this.properties_]);
};

function setUp() {
  // Behavior of window.chrome depends on each test case. window.chrome should
  // be initialized properly inside each test function.
  window.chrome = {
    commandLinePrivate: {
      hasSwitch: function(name, callback) {
        callback(false);
      }
    },
    runtime: {id: 'test-extension-id', lastError: null},

    storage: {
      onChanged: {addListener: function(callback) {}},
      local: {
        get: function(key, callback) {
          callback({});
        },
        set: function(value) {}
      }
    }
  };

  cr.ui.decorate('command', cr.ui.Command);
}

function testExecuteEntryTask(callback) {
  window.chrome.fileManagerPrivate = {
    getFileTasks: function(entries, callback) {
      setTimeout(callback.bind(null, [
        {taskId:'handler-extension-id|file|open', isDefault: false},
        {taskId:'handler-extension-id|file|play', isDefault: true}
      ]), 0);
    },
    onAppsUpdated: {
      addListener: function() {},
    },
  };

  var fileSystem = new MockFileSystem('volumeId');
  fileSystem.entries['/test.png'] =
      new MockFileEntry(fileSystem, '/test.png', {});
  var controller = new TaskController(
      DialogType.FULL_PAGE, {
        getDriveConnectionState: function() {
          return VolumeManagerCommon.DriveConnectionType.ONLINE;
        },
        getVolumeInfo: function() {
          return {
            volumeType: VolumeManagerCommon.VolumeType.DRIVE
          };
        }
      },
      {
        taskMenuButton: document.createElement('button'),
        shareMenuButton: {menu: document.createElement('div')},
        fileContextMenu:
            {defaultActionMenuItem: document.createElement('div')}
      },
      new MockMetadataModel({}), {
        getCurrentRootType: function() {
          return null;
        }
      }, new cr.EventTarget(), null);

  controller.executeEntryTask(fileSystem.entries['/test.png']);
  reportPromise(new Promise(function(fulfill) {
    chrome.fileManagerPrivate.executeTask = fulfill;
  }).then(function(info) {
    assertEquals("handler-extension-id|file|play", info);
  }), callback);
}

function MockFileSelectionHandler() {
  this.computeAdditionalCallback = function() {};
  this.updateSelection([], []);
}

MockFileSelectionHandler.prototype.__proto__ = cr.EventTarget.prototype;

MockFileSelectionHandler.prototype.updateSelection = function(
    entries, mimeTypes) {
  this.selection = {
    entries: entries,
    mimeTypes: mimeTypes,
    computeAdditional: function(metadataModel) {
      this.computeAdditionalCallback();
      return new Promise(function(fulfill) {
        fulfill();
      });
    }.bind(this)
  };
};

function setupFileManagerPrivate() {
  window.chrome.fileManagerPrivate = {
    getFileTaskCalledCount_: 0,
    getFileTasks: function(entries, callback) {
      window.chrome.fileManagerPrivate.getFileTaskCalledCount_++;
      setTimeout(
          callback.bind(
              null,
              [
                {taskId: 'handler-extension-id|file|open', isDefault: false},
                {taskId: 'handler-extension-id|file|play', isDefault: true}
              ]),
          0);
    },
    onAppsUpdated: {
      addListener: function() {},
    },
  };
}

function createTaskController(selectionHandler) {
  return new TaskController(
      DialogType.FULL_PAGE, {}, {
        taskMenuButton: document.createElement('button'),
        shareMenuButton: {menu: document.createElement('div')},
        fileContextMenu:
            {defaultActionMenuItem: document.createElement('div')}
      },
      new MockMetadataModel({}), {
        getCurrentRootType: function() {
          return null;
        }
      }, selectionHandler, null);
}

// TaskController.getFileTasks should not call fileManagerPrivate.getFileTasks
// multiple times when the selected entries are not changed.
function testGetFileTasksShouldNotBeCalledMultipleTimes(callback) {
  setupFileManagerPrivate();
  var selectionHandler = new MockFileSelectionHandler();
  var fileSystem = new MockFileSystem('volumeId');
  selectionHandler.updateSelection(
      [new MockFileEntry(fileSystem, '/test.png', {})], ['image/png']);

  var controller = createTaskController(selectionHandler);

  assert(window.chrome.fileManagerPrivate.getFileTaskCalledCount_ === 0);
  controller.getFileTasks()
      .then(function(tasks) {
        assert(window.chrome.fileManagerPrivate.getFileTaskCalledCount_ === 1);
        assert(util.isSameEntries(
            tasks.entries, selectionHandler.selection.entries));
        // Make oldSelection.entries !== newSelection.entries
        selectionHandler.updateSelection(
            [new MockFileEntry(fileSystem, '/test.png', {})], ['image/png']);
        return controller.getFileTasks();
      })
      .then(function(tasks) {
        assert(window.chrome.fileManagerPrivate.getFileTaskCalledCount_ === 1);
        assert(util.isSameEntries(
            tasks.entries, selectionHandler.selection.entries));
        callback();
      })
      .catch(function(error) {
        assertNotReached(error);
        callback();
      });
}

// TaskController.getFileTasks should always return the promise whose FileTasks
// corresponds to FileSelectionHandler.selection at the time
// TaskController.getFileTasks is called.
function testGetFileTasksShouldNotReturnObsoletePromise(callback) {
  setupFileManagerPrivate();
  var selectionHandler = new MockFileSelectionHandler();
  var fileSystem = new MockFileSystem('volumeId');
  selectionHandler.updateSelection(
      [new MockFileEntry(fileSystem, '/test.png', {})], ['image/png']);

  var controller = createTaskController(selectionHandler);
  controller.getFileTasks()
      .then(function(tasks) {
        assert(util.isSameEntries(
            tasks.entries, selectionHandler.selection.entries));

        selectionHandler.updateSelection(
            [new MockFileEntry(fileSystem, '/testtest.jpg', {})],
            ['image/jpeg']);

        return controller.getFileTasks();
      })
      .then(function(tasks) {
        assert(util.isSameEntries(
            tasks.entries, selectionHandler.selection.entries));
        callback();
      })
      .catch(function(error) {
        assertNotReached(error);
        callback();
      });
}

function testGetFileTasksShouldNotCacheRejectedPromise(callback) {
  setupFileManagerPrivate();
  var selectionHandler = new MockFileSelectionHandler();
  var fileSystem = new MockFileSystem('volumeId');
  selectionHandler.updateSelection(
      [new MockFileEntry(fileSystem, '/test.png', {})], ['image/png']);

  var controller = createTaskController(selectionHandler);

  // Change FileSelection during getFileTasks call,
  // so that the promise will be rejected.
  selectionHandler.computeAdditionalCallback = function() {
    selectionHandler.updateSelection(
        [new MockFileEntry(fileSystem, '/test.png', {})], ['image/png']);
  };

  controller.getFileTasks().then(
      function(tasks) {
        assertNotReached('promise is not rejected');
        callback();
      },
      function() {
        selectionHandler.computeAdditionalCallback = function() {};
        controller.getFileTasks().then(
            function(tasks) {
              assert(util.isSameEntries(
                  tasks.entries, selectionHandler.selection.entries));
              callback();
            },
            function() {
              assertNotReached('promise is rejected');
              callback();
            });
      });
}
