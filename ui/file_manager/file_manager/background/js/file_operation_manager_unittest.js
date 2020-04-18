// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
'use strict';

/**
 * Mock of chrome.runtime.
 * @type {Object}
 * @const
 */
chrome.runtime = {
  lastError: null
};

/**
 * Mock of chrome.power.
 * @type {Object}
 * @const
 */
chrome.power = {
  requestKeepAwake: function() {
    chrome.power.keepAwakeRequested = true;
  },
  releaseKeepAwake: function() {
    chrome.power.keepAwakeRequested = false;
  },
  keepAwakeRequested: false
};

/**
 * Mock of chrome.fileManagerPrivate.
 * @type {Object}
 * @const
 */
chrome.fileManagerPrivate = {
  onCopyProgress: {
    addListener: function(callback) {
      chrome.fileManagerPrivate.onCopyProgress.listener_ = callback;
    },
    removeListener: function() {
      chrome.fileManagerPrivate.onCopyProgress.listener_ = null;
    },
    listener_: null
  }
};

/**
 * Logs events of file operation manager.
 * @param {!FileOperationManager} fileOperationManager A target file operation
 *     manager.
 * @constructor
 * @struct
 */
function EventLogger(fileOperationManager) {
  this.events = [];
  this.numberOfBeginEvents = 0;
  this.numberOfErrorEvents = 0;
  this.numberOfSuccessEvents = 0;
  fileOperationManager.addEventListener('copy-progress',
      this.onCopyProgress_.bind(this));
}

/**
 * Handles copy-progress event.
 * @param {Event} event An event.
 * @private
 */
EventLogger.prototype.onCopyProgress_ = function(event) {
  if (event.reason === 'BEGIN') {
    this.events.push(event);
    this.numberOfBeginEvents++;
  }
  if (event.reason === 'ERROR') {
    this.events.push(event);
    this.numberOfErrorEvents++;
  }
  if (event.reason === 'SUCCESS') {
    this.events.push(event);
    this.numberOfSuccessEvents++;
  }
};

/**
 * Provides fake implementation of chrome.fileManagerPrivate.startCopy.
 * @param {string} blockedDestination Destination url of an entry whose request
 *     should be blocked.
 * @param {!Entry} sourceEntry Source entry. Single source entry is supported.
 * @param {!Array<!FakeFileSystem>} fileSystems File systems.
 * @constructor
 * @struct
 */
function BlockableFakeStartCopy(blockedDestination, sourceEntry, fileSystems) {
  this.resolveBlockedOperationCallback = null;
  this.blockedDestination_ = blockedDestination;
  this.sourceEntry_ = sourceEntry;
  this.fileSystems_ = fileSystems;
  this.startCopyId_ = 0;
}

/**
 * A fake implemencation of startCopy function.
 * @param {!Entry} source
 * @param {!Entry} destination
 * @param {string} newName
 * @param {function(number)} callback
 */
BlockableFakeStartCopy.prototype.startCopyFunc = function(
    source, destination, newName, callback) {
  var makeStatus = function(type) {
    return {
      type: type,
      sourceUrl: source.toURL(),
      destinationUrl: destination.toURL()
    };
  };

  var completeCopyOperation = function(copyId) {
    var newPath = joinPath('/', newName);
    var fileSystem = getFileSystemForURL(
        this.fileSystems_, destination.toURL());
    fileSystem.entries[newPath] = this.sourceEntry_.clone(newPath);
    listener(copyId, makeStatus('end_copy_entry'));
    listener(copyId, makeStatus('success'));
  }.bind(this);

  this.startCopyId_++;

  callback(this.startCopyId_);
  var listener = chrome.fileManagerPrivate.onCopyProgress.listener_;
  listener(this.startCopyId_, makeStatus('begin_copy_entry'));
  listener(this.startCopyId_, makeStatus('progress'));

  if (destination.toURL() === this.blockedDestination_) {
    this.resolveBlockedOperationCallback =
        completeCopyOperation.bind(this, this.startCopyId_);
  } else {
    completeCopyOperation(this.startCopyId_);
  }
};

/**
 * Fake volume manager.
 * @constructor
 * @structs
 */
function FakeVolumeManager() {}

/**
 * Returns fake volume info.
 * @param {!Entry} entry
 * @return {VolumeInfo} A fake volume info.
 */
FakeVolumeManager.prototype.getVolumeInfo = function(entry) {
  return { volumeId: entry.filesystem.name };
};

/**
 * Returns file system of the url.
 * @param {!Array<!FakeFileSystem>} fileSystems
 * @param {string} url
 * @return {!FakeFileSystem}
 */
function getFileSystemForURL(fileSystems, url) {
  for (var i = 0; i < fileSystems.length; i++) {
    if (new RegExp('^filesystem:' + fileSystems[i].name + '/').test(url))
      return fileSystems[i];
  }
  throw new Error('Unexpected url: ' + url + '.');
}

/**
 * Size of directory.
 * @type {number}
 * @const
 */
var DIRECTORY_SIZE = -1;

/**
 * Creates test file system.
 * @param {string} id File system ID.
 * @param {Object<number>} entries Map of entries' paths and their size.
 *     If the size is equals to DIRECTORY_SIZE, the entry is directory.
 */
function createTestFileSystem(id, entries) {
  var fileSystem = new MockFileSystem(id, 'filesystem:' + id);
  for (var path in entries) {
    if (entries[path] === DIRECTORY_SIZE) {
      fileSystem.entries[path] = new MockDirectoryEntry(fileSystem, path);
    } else {
      fileSystem.entries[path] =
          new MockFileEntry(fileSystem, path, {size: entries[path]});
    }
  }
  return fileSystem;
}

/**
 * Resolves URL on the file system.
 * @param {FakeFileSystem} fileSystem Fake file system.
 * @param {string} url URL.
 * @param {function(MockEntry)} success Success callback.
 * @param {function()} failure Failure callback.
 */
function resolveTestFileSystemURL(fileSystem, url, success, failure) {
  for (var name in fileSystem.entries) {
    var entry = fileSystem.entries[name];
    if (entry.toURL() == url) {
      success(entry);
      return;
    }
  }
  failure();
}

/**
 * Waits for events until 'success'.
 * @param {FileOperationManager} fileOperationManager File operation manager.
 * @return {Promise} Promise to be fulfilled with an event list.
 */
function waitForEvents(fileOperationManager) {
  return new Promise(function(fulfill) {
    var events = [];
    fileOperationManager.addEventListener('copy-progress', function(event) {
      events.push(event);
      if (event.reason === 'SUCCESS')
        fulfill(events);
    });
    fileOperationManager.addEventListener('entries-changed', function(event) {
      events.push(event);
    });
    fileOperationManager.addEventListener('delete', function(event) {
      events.push(event);
      if (event.reason === 'SUCCESS')
        fulfill(events);
    });
  });
}

/**
 * Placeholder for mocked volume manager.
 * @type {(FakeVolumeManager|{getVolumeInfo: function()}?)}
 */
var volumeManager;

var volumeManagerFactory = {};

/**
 * Provide VolumeManager.getInstande() for FileOperationManager using mocked
 * volume manager instance.
 * @type {!Promise<(FakeVolumeManager|{getVolumeInfo: function()}?)>}
 */
volumeManagerFactory.getInstance = function() {
  return Promise.resolve(volumeManager);
};

/**
 * Test target.
 * @type {FileOperationManager}
 */
var fileOperationManager;

/**
 * Initializes the test environment.
 */
function setUp() {
}

/**
 * Tests the fileOperationUtil.resolvePath function.
 * @param {function(boolean:hasError)} callback Callback to be passed true on
 *     error.
 */
function testResolvePath(callback) {
  var fileSystem = createTestFileSystem('testVolume', {
    '/': DIRECTORY_SIZE,
    '/file': 10,
    '/directory': DIRECTORY_SIZE
  });
  var root = fileSystem.root;
  var rootPromise = fileOperationUtil.resolvePath(root, '/');
  var filePromise = fileOperationUtil.resolvePath(root, '/file');
  var directoryPromise = fileOperationUtil.resolvePath(root, '/directory');
  var errorPromise = fileOperationUtil.resolvePath(root, '/not_found').then(
      function() { assertTrue(false, 'The NOT_FOUND error is not reported.'); },
      function(error) { return error.name; });
  reportPromise(Promise.all([
    rootPromise,
    filePromise,
    directoryPromise,
    errorPromise
  ]).then(function(results) {
    assertArrayEquals([
      fileSystem.entries['/'],
      fileSystem.entries['/file'],
      fileSystem.entries['/directory'],
      'NotFoundError'
    ], results);
  }), callback);
}

/**
 * @param {function(boolean)} callback Callback to be passed true on
 *     error.
 */
function testFindEntriesRecursively(callback) {
  var fileSystem = createTestFileSystem('testVolume', {
    '/': DIRECTORY_SIZE,
    '/file.txt': 10,
    '/file (1).txt': 10,
    '/file (2).txt': 10,
    '/file (3).txt': 10,
    '/file (4).txt': 10,
    '/file (5).txt': 10,
    '/DCIM/': DIRECTORY_SIZE,
    '/DCIM/IMG_1232.txt': 10,
    '/DCIM/IMG_1233 (7).txt': 10,
    '/DCIM/IMG_1234 (8).txt': 10,
    '/DCIM/IMG_1235 (9).txt': 10,
  });

  var foundFiles = [];
  fileOperationUtil.findEntriesRecursively(
      fileSystem.root,
      function(fileEntry) {
        foundFiles.push(fileEntry);
      })
      .then(
          function() {
            assertEquals(12, foundFiles.length);
            callback(false);
          })
      .catch(callback);
}

/**
 * @param {function(boolean)} callback Callback to be passed true on
 *     error.
 */
function testFindFilesRecursively(callback) {
  var fileSystem = createTestFileSystem('testVolume', {
    '/': DIRECTORY_SIZE,
    '/file.txt': 10,
    '/file (1).txt': 10,
    '/file (2).txt': 10,
    '/file (3).txt': 10,
    '/file (4).txt': 10,
    '/file (5).txt': 10,
    '/DCIM/': DIRECTORY_SIZE,
    '/DCIM/IMG_1232.txt': 10,
    '/DCIM/IMG_1233 (7).txt': 10,
    '/DCIM/IMG_1234 (8).txt': 10,
    '/DCIM/IMG_1235 (9).txt': 10,
  });

  var foundFiles = [];
  fileOperationUtil.findFilesRecursively(
      fileSystem.root,
      function(fileEntry) {
        foundFiles.push(fileEntry);
      })
      .then(
          function() {
            assertEquals(10, foundFiles.length);
            foundFiles.forEach(
                function(entry) {
                  assertTrue(entry.isFile);
                });
            callback(false);
          })
      .catch(callback);
}

/**
 * @param {function(boolean)} callback Callback to be passed true on
 *     error.
 */
function testGatherEntriesRecursively(callback) {
  var fileSystem = createTestFileSystem('testVolume', {
    '/': DIRECTORY_SIZE,
    '/file.txt': 10,
    '/file (1).txt': 10,
    '/file (2).txt': 10,
    '/file (3).txt': 10,
    '/file (4).txt': 10,
    '/file (5).txt': 10,
    '/DCIM/': DIRECTORY_SIZE,
    '/DCIM/IMG_1232.txt': 10,
    '/DCIM/IMG_1233 (7).txt': 10,
    '/DCIM/IMG_1234 (8).txt': 10,
    '/DCIM/IMG_1235 (9).txt': 10,
  });

  fileOperationUtil.gatherEntriesRecursively(fileSystem.root)
      .then(
          function(gatheredFiles) {
            assertEquals(12, gatheredFiles.length);
            callback(false);
          })
      .catch(callback);
}

/**
 * Tests the fileOperationUtil.deduplicatePath
 * @param {function(boolean:hasError)} callback Callback to be passed true on
 *     error.
 */
function testDeduplicatePath(callback) {
  var fileSystem1 = createTestFileSystem('testVolume', {'/': DIRECTORY_SIZE});
  var fileSystem2 = createTestFileSystem('testVolume', {
    '/': DIRECTORY_SIZE,
    '/file.txt': 10
  });
  var fileSystem3 = createTestFileSystem('testVolume', {
    '/': DIRECTORY_SIZE,
    '/file.txt': 10,
    '/file (1).txt': 10,
    '/file (2).txt': 10,
    '/file (3).txt': 10,
    '/file (4).txt': 10,
    '/file (5).txt': 10,
    '/file (6).txt': 10,
    '/file (7).txt': 10,
    '/file (8).txt': 10,
    '/file (9).txt': 10,
  });

  var nonExistingPromise =
      fileOperationUtil.deduplicatePath(fileSystem1.root, 'file.txt').
      then(function(path) {
        assertEquals('file.txt', path);
      });
  var existingPathPromise =
      fileOperationUtil.deduplicatePath(fileSystem2.root, 'file.txt').
      then(function(path) {
        assertEquals('file (1).txt', path);
      });
  var moreExistingPathPromise =
      fileOperationUtil.deduplicatePath(fileSystem3.root, 'file.txt').
      then(function(path) {
        assertEquals('file (10).txt', path);
      });

  var testPromise = Promise.all([
    nonExistingPromise,
    existingPathPromise,
    moreExistingPathPromise,
  ]);
  reportPromise(testPromise, callback);
}

/**
 * Tests the fileOperationUtil.paste.
 * @param {function(boolean:hasError)} callback Callback to be passed true on
 *     error.
 */
function testCopy(callback) {
  // Prepare entries and their resolver.
  var fileSystem = createTestFileSystem('testVolume', {
    '/': DIRECTORY_SIZE,
    '/test.txt': 10,
  });
  window.webkitResolveLocalFileSystemURL =
      resolveTestFileSystemURL.bind(null, fileSystem);

  chrome.fileManagerPrivate.startCopy =
      function(source, destination, newName, callback) {
        var makeStatus = function(type) {
          return {
            type: type,
            sourceUrl: source.toURL(),
            destinationUrl: destination.toURL()
          };
        };
        callback(1);
        var listener = chrome.fileManagerPrivate.onCopyProgress.listener_;
        listener(1, makeStatus('begin_copy_entry'));
        listener(1, makeStatus('progress'));
        var newPath = joinPath('/', newName);
        fileSystem.entries[newPath] =
            fileSystem.entries['/test.txt'].clone(newPath);
        listener(1, makeStatus('end_copy_entry'));
        listener(1, makeStatus('success'));
      };

  volumeManager = new FakeVolumeManager();
  fileOperationManager = new FileOperationManager();

  // Observing manager's events.
  var eventsPromise = waitForEvents(fileOperationManager);

  // Verify the events.
  reportPromise(eventsPromise.then(function(events) {
    var firstEvent = events[0];
    assertEquals('BEGIN', firstEvent.reason);
    assertEquals(1, firstEvent.status.numRemainingItems);
    assertEquals(0, firstEvent.status.processedBytes);
    assertEquals(1, firstEvent.status.totalBytes);

    var lastEvent = events[events.length - 1];
    assertEquals('SUCCESS', lastEvent.reason);
    assertEquals(0, lastEvent.status.numRemainingItems);
    assertEquals(10, lastEvent.status.processedBytes);
    assertEquals(10, lastEvent.status.totalBytes);

    assertTrue(events.some(function(event) {
      return event.type === 'entries-changed' &&
          event.kind === util.EntryChangedKind.CREATED &&
          event.entries[0].fullPath === '/test (1).txt';
    }));

    assertFalse(events.some(function(event) {
      return event.type === 'delete';
    }));
  }), callback);

  fileOperationManager.paste(
      [fileSystem.entries['/test.txt']],
      fileSystem.entries['/'],
      false);
}

/**
 * Tests the fileOperationUtil.paste for copying files in sequential. When
 * destination volumes are same, copy operations should run in sequential.
 */
function testCopyInSequential(callback) {
  var fileSystem = createTestFileSystem('testVolume', {
    '/': DIRECTORY_SIZE,
    '/dest': DIRECTORY_SIZE,
    '/test.txt': 10
  });

  window.webkitResolveLocalFileSystemURL =
      resolveTestFileSystemURL.bind(null, fileSystem);

  var blockableFakeStartCopy = new BlockableFakeStartCopy(
      'filesystem:testVolume/dest',
      fileSystem.entries['/test.txt'],
      [fileSystem]);
  chrome.fileManagerPrivate.startCopy =
      blockableFakeStartCopy.startCopyFunc.bind(blockableFakeStartCopy);

  volumeManager = new FakeVolumeManager();
  fileOperationManager = new FileOperationManager();

  var eventLogger = new EventLogger(fileOperationManager);

  // Copy test.txt to /dest. This operation will be blocked.
  fileOperationManager.paste([fileSystem.entries['/test.txt']],
      fileSystem.entries['/dest'],
      false);

  var firstOperationTaskId;
  reportPromise(waitUntil(function() {
    // Wait until the first operation is blocked.
    return blockableFakeStartCopy.resolveBlockedOperationCallback !== null;
  }).then(function() {
    assertEquals(1, eventLogger.events.length);
    assertEquals('BEGIN', eventLogger.events[0].reason);
    firstOperationTaskId = eventLogger.events[0].taskId;

    // Copy test.txt to /. This operation should be blocked.
    fileOperationManager.paste([fileSystem.entries['/test.txt']],
        fileSystem.entries['/'],
        false);

    return waitUntil(function() {
      return fileOperationManager.getPendingCopyTasksForTesting().length === 1;
    });
  }).then(function() {
    // Asserts that the second operation is added to pending copy tasks. Current
    // implementation run tasks synchronusly after adding it to pending tasks.
    // TODO(yawano) This check deeply depends on the implementation. Find a
    //     better way to test this.
    var pendingTask = fileOperationManager.getPendingCopyTasksForTesting()[0];
    assertEquals(fileSystem.entries['/'], pendingTask.targetDirEntry);

    blockableFakeStartCopy.resolveBlockedOperationCallback();

    return waitUntil(function() {
      return eventLogger.numberOfSuccessEvents === 2;
    });
  }).then(function() {
    // Events should be the following.
    // BEGIN: first operation
    // BEGIN: second operation
    // SUCCESS: first operation
    // SUCCESS: second operation
    var events = eventLogger.events;
    assertEquals(4, events.length);
    assertEquals('BEGIN', events[0].reason);
    assertEquals(firstOperationTaskId, events[0].taskId);
    assertEquals('BEGIN', events[1].reason);
    assertTrue(events[1].taskId !== firstOperationTaskId);
    assertEquals('SUCCESS', events[2].reason);
    assertEquals(firstOperationTaskId, events[2].taskId);
    assertEquals('SUCCESS', events[3].reason);
    assertEquals(events[1].taskId, events[3].taskId);
  }), callback);
}

/**
 * Tests the fileOperationUtil.paste for copying files in paralell. When
 * destination volumes are different, copy operations can run in paralell.
 */
function testCopyInParallel(callback) {
  var fileSystemA = createTestFileSystem('volumeA', {
    '/': DIRECTORY_SIZE,
    '/test.txt': 10
  });
  var fileSystemB = createTestFileSystem('volumeB', {
    '/': DIRECTORY_SIZE,
  });
  var fileSystems = [fileSystemA, fileSystemB];

  window.webkitResolveLocalFileSystemURL = function(url, success, failure) {
    return resolveTestFileSystemURL(
        getFileSystemForURL(fileSystems, url), url, success, failure);
  };

  var blockableFakeStartCopy = new BlockableFakeStartCopy(
      'filesystem:volumeB/',
      fileSystemA.entries['/test.txt'],
      fileSystems);
  chrome.fileManagerPrivate.startCopy =
      blockableFakeStartCopy.startCopyFunc.bind(blockableFakeStartCopy);

  volumeManager = new FakeVolumeManager();
  fileOperationManager = new FileOperationManager();

  var eventLogger = new EventLogger(fileOperationManager);

  // Copy test.txt from volume A to volume B.
  fileOperationManager.paste([fileSystemA.entries['/test.txt']],
      fileSystemB.entries['/'],
      false);

  var firstOperationTaskId;
  reportPromise(waitUntil(function() {
    return blockableFakeStartCopy.resolveBlockedOperationCallback !== null;
  }).then(function() {
    assertEquals(1, eventLogger.events.length);
    assertEquals('BEGIN', eventLogger.events[0].reason);
    firstOperationTaskId = eventLogger.events[0].taskId;

    // Copy test.txt from volume A to volume A. This should not be blocked by
    // the previous operation.
    fileOperationManager.paste([fileSystemA.entries['/test.txt']],
        fileSystemA.entries['/'],
        false);

    // Wait until the second operation is completed.
    return waitUntil(function() {
      return eventLogger.numberOfSuccessEvents === 1;
    });
  }).then(function() {
    // Resolve the blocked operation.
    blockableFakeStartCopy.resolveBlockedOperationCallback();

    // Wait until the blocked operation is completed.
    return waitUntil(function() {
      return eventLogger.numberOfSuccessEvents === 2;
    });
  }).then(function() {
    // Events should be following.
    // BEGIN: first operation
    // BEGIN: second operation
    // SUCCESS: second operation
    // SUCCESS: first operation
    var events = eventLogger.events;
    assertEquals(4, events.length);
    assertEquals('BEGIN', events[0].reason);
    assertEquals(firstOperationTaskId, events[0].taskId);
    assertEquals('BEGIN', events[1].reason);
    assertTrue(firstOperationTaskId !== events[1].taskId);
    assertEquals('SUCCESS', events[2].reason);
    assertEquals(events[1].taskId, events[2].taskId);
    assertEquals('SUCCESS', events[3].reason);
    assertEquals(firstOperationTaskId, events[3].taskId);
  }), callback);
}

/**
 * Test case that a copy fails since destination volume is not available.
 */
function testCopyFails(callback) {
  var fileSystem = createTestFileSystem('testVolume', {
    '/': DIRECTORY_SIZE,
    '/test.txt': 10
  });

  volumeManager = {
    /* Mocking volume manager. */
    getVolumeInfo: function() {
      // Return null to simulate that the volume info is not available.
      return null;
    }
  };
  fileOperationManager = new FileOperationManager();

  var eventLogger = new EventLogger(fileOperationManager);

  // Copy test.txt to /.
  fileOperationManager.paste([fileSystem.entries['/test.txt']],
      fileSystem.entries['/'],
      false);

  reportPromise(waitUntil(function() {
    return eventLogger.numberOfErrorEvents === 1;
  }).then(function() {
    // Since the task fails with an error, pending copy tasks should be empty.
    assertEquals(0,
        fileOperationManager.getPendingCopyTasksForTesting().length);

    // Check events.
    var events = eventLogger.events;
    assertEquals(2, events.length);
    assertEquals('BEGIN', events[0].reason);
    assertEquals('ERROR', events[1].reason);
    assertEquals(events[0].taskId, events[1].taskId);
  }), callback);
}

/**
 * Tests the fileOperationUtil.paste for move.
 * @param {function(boolean:hasError)} callback Callback to be passed true on
 *     error.
 */
function testMove(callback) {
  // Prepare entries and their resolver.
  var fileSystem = createTestFileSystem('testVolume', {
    '/': DIRECTORY_SIZE,
    '/directory': DIRECTORY_SIZE,
    '/test.txt': 10,
  });
  window.webkitResolveLocalFileSystemURL =
      resolveTestFileSystemURL.bind(null, fileSystem);

  volumeManager = new FakeVolumeManager();
  fileOperationManager = new FileOperationManager();

  // Observing manager's events.
  var eventsPromise = waitForEvents(fileOperationManager);

  // Verify the events.
  reportPromise(eventsPromise.then(function(events) {
    var firstEvent = events[0];
    assertEquals('BEGIN', firstEvent.reason);
    assertEquals(1, firstEvent.status.numRemainingItems);
    assertEquals(0, firstEvent.status.processedBytes);
    assertEquals(1, firstEvent.status.totalBytes);

    var lastEvent = events[events.length - 1];
    assertEquals('SUCCESS', lastEvent.reason);
    assertEquals(0, lastEvent.status.numRemainingItems);
    assertEquals(1, lastEvent.status.processedBytes);
    assertEquals(1, lastEvent.status.totalBytes);

    assertTrue(events.some(function(event) {
      return event.type === 'entries-changed' &&
          event.kind === util.EntryChangedKind.DELETED &&
          event.entries[0].fullPath === '/test.txt';
    }));

    assertTrue(events.some(function(event) {
      return event.type === 'entries-changed' &&
          event.kind === util.EntryChangedKind.CREATED &&
          event.entries[0].fullPath === '/directory/test.txt';
    }));

    assertFalse(events.some(function(event) {
      return event.type === 'delete';
    }));
  }), callback);

  fileOperationManager.paste(
      [fileSystem.entries['/test.txt']],
      fileSystem.entries['/directory'],
      true);
}

/**
 * Tests the fileOperationUtil.deleteEntries.
 * @param {function(boolean:hasError)} callback Callback to be passed true on
 *     error.
 */
function testDelete(callback) {
  // Prepare entries and their resolver.
  var fileSystem = createTestFileSystem('testVolume', {
    '/': DIRECTORY_SIZE,
    '/test.txt': 10,
  });
  window.webkitResolveLocalFileSystemURL =
      resolveTestFileSystemURL.bind(null, fileSystem);

  // Observing manager's events.
  reportPromise(waitForEvents(fileOperationManager).then(function(events) {
    assertEquals('delete', events[0].type);
    assertEquals('BEGIN', events[0].reason);
    assertEquals(10, events[0].totalBytes);
    assertEquals(0, events[0].processedBytes);

    var lastEvent = events[events.length - 1];
    assertEquals('delete', lastEvent.type);
    assertEquals('SUCCESS', lastEvent.reason);
    assertEquals(10, lastEvent.totalBytes);
    assertEquals(10, lastEvent.processedBytes);

    assertFalse(events.some(function(event) {
      return event.type === 'copy-progress';
    }));
  }), callback);

  fileOperationManager.deleteEntries([fileSystem.entries['/test.txt']]);
}

/**
 * Tests the fileOperationUtil.zipSelection.
 * @param {function(boolean:hasError)} callback Callback to be passed true on
 *     error.
 */
function testZip(callback) {
  // Prepare entries and their resolver.
  var fileSystem = createTestFileSystem('testVolume', {
    '/': DIRECTORY_SIZE,
    '/test.txt': 10,
  });
  window.webkitResolveLocalFileSystemURL =
      resolveTestFileSystemURL.bind(null, fileSystem);
  chrome.fileManagerPrivate.zipSelection = function(
      sources, parent, newName, success, error) {
    var newPath = joinPath('/', newName);
    var newEntry = new MockFileEntry(fileSystem, newPath, {size: 10});
    fileSystem.entries[newPath] = newEntry;
    success(newEntry);
  };

  volumeManager = new FakeVolumeManager();
  fileOperationManager = new FileOperationManager();

  // Observing manager's events.
  reportPromise(waitForEvents(fileOperationManager).then(function(events) {
    assertEquals('copy-progress', events[0].type);
    assertEquals('BEGIN', events[0].reason);
    assertEquals(1, events[0].status.totalBytes);
    assertEquals(0, events[0].status.processedBytes);

    var lastEvent = events[events.length - 1];
    assertEquals('copy-progress', lastEvent.type);
    assertEquals('SUCCESS', lastEvent.reason);
    assertEquals(10, lastEvent.status.totalBytes);
    assertEquals(10, lastEvent.status.processedBytes);

    assertFalse(events.some(function(event) {
      return event.type === 'delete';
    }));

    assertTrue(events.some(function(event) {
      return event.type === 'entries-changed' &&
          event.entries[0].fullPath === '/test.zip';
    }));
  }), callback);

  fileOperationManager.zipSelection(
      [fileSystem.entries['/test.txt']], fileSystem.entries['/']);
}
