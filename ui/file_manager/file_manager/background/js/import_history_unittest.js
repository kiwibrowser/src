// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @const {string} */
var FILE_LAST_MODIFIED = new Date("Dec 4 1968").toString();

/** @const {string} */
var FILE_SIZE = 1234;

/** @const {string} */
var FILE_PATH = 'test/data';

/** @const {string} */
var DEVICE = importer.Destination.DEVICE;

/** @const {string} */
var GOOGLE_DRIVE = importer.Destination.GOOGLE_DRIVE;

/**
 * Space Cloud: Your source for interstellar cloud storage.
 * @const {string}
 */
var SPACE_CAMP = 'Space Camp';

/** @type {!MockFileSystem|undefined} */
var testFileSystem;

/** @type {!MockFileEntry|undefined} */
var testFileEntry;

/** @type {!importer.TestLogger} */
var testLogger;

/** @type {!importer.RecordStorage|undefined} */
var storage;

/** @type {!Promise<!importer.PersistentImportHistory>|undefined} */
var historyProvider;

/** @type {Promise} */
var testPromise;

// Set up the test components.
function setUp() {
  setupChromeApis();
  installTestLogger();

  testFileSystem = new MockFileSystem('abc-123', 'filesystem:abc-123');
  testFileEntry = new MockFileEntry(
      testFileSystem,
      FILE_PATH, {
        size: FILE_SIZE,
        modificationTime: FILE_LAST_MODIFIED
      });
  testFileSystem.entries[FILE_PATH] = testFileEntry;

  storage = new TestRecordStorage();

  var history = new importer.PersistentImportHistory(
      importer.createMetadataHashcode,
      storage);

  historyProvider = history.whenReady();
}

function tearDown() {
  testLogger.errorRecorder.assertCallCount(0);
  testPromise = null;
}

function testWasCopied_FalseForUnknownEntry(callback) {
  // TestRecordWriter is pre-configured with a Space Cloud entry
  // but not for this file.
  testPromise = historyProvider.then(
      function(history) {
        return history.wasCopied(testFileEntry, SPACE_CAMP).then(assertFalse);
      });

  reportPromise(testPromise, callback);
}

function testWasCopied_TrueForKnownEntryLoadedFromStorage(callback) {
  // TestRecordWriter is pre-configured with this entry.
  testPromise = historyProvider.then(
      function(history) {
        return history.wasCopied(testFileEntry, GOOGLE_DRIVE).then(assertTrue);
      });

  reportPromise(testPromise, callback);
}

function testWasImported_TrueForKnownEntrySetAtRuntime(callback) {
  testPromise = historyProvider.then(
      function(history) {
        return history.markImported(testFileEntry, SPACE_CAMP).then(
            function() {
              return history.wasImported(testFileEntry, SPACE_CAMP)
                  .then(assertTrue);
            });
      });

  reportPromise(testPromise, callback);
}

function testWasCopied_TrueForKnownEntryLoadedFromStorage(callback) {
  // TestRecordWriter is pre-configured with this entry.
  testPromise = historyProvider.then(
      function(history) {
        return history.wasCopied(testFileEntry, GOOGLE_DRIVE).then(assertTrue);
      });

  reportPromise(testPromise, callback);
}

function testMarkCopied_FiresChangedEvent(callback) {
  testPromise = historyProvider.then(
      function(history) {
        var recorder = new TestCallRecorder();
        history.addObserver(recorder.callback);
        return history.markCopied(testFileEntry, SPACE_CAMP, 'url1').then(
            function() {
              return Promise.resolve()
                  .then(
                      function() {
                        recorder.assertCallCount(1);
                        assertEquals(
                            importer.ImportHistoryState.COPIED,
                            recorder.getLastArguments()[0]['state']);
                      });
            });
      });

  reportPromise(testPromise, callback);
}

function testMarkImported_ByUrl(callback) {
  var destinationUrl = 'filesystem:chrome-extension://abc/photos/splosion.jpg';
  testPromise = historyProvider.then(
      function(history) {
        return history.markCopied(testFileEntry, SPACE_CAMP, destinationUrl)
            .then(
                function() {
                  return history.markImportedByUrl(destinationUrl)
                      .then(
                          function() {
                            return history.wasImported(
                                testFileEntry,
                                SPACE_CAMP)
                                .then(assertTrue);
                          });
                });
      });

  reportPromise(testPromise, callback);
}

function testWasImported_FalseForUnknownEntry(callback) {
  // TestRecordWriter is pre-configured with a Space Cloud entry
  // but not for this file.
  testPromise = historyProvider.then(
      function(history) {
        return history.wasImported(testFileEntry, SPACE_CAMP).then(assertFalse);
      });

  reportPromise(testPromise, callback);
}

function testWasImported_TrueForKnownEntryLoadedFromStorage(callback) {
  // TestRecordWriter is pre-configured with this entry.
  testPromise = historyProvider.then(
      function(history) {
        return history.wasImported(testFileEntry, GOOGLE_DRIVE)
            .then(assertTrue);
      });

  reportPromise(testPromise, callback);
}

function testWasImported_TrueForKnownEntrySetAtRuntime(callback) {
  testPromise = historyProvider.then(
      function(history) {
        return history.markImported(testFileEntry, SPACE_CAMP).then(
            function() {
              return history.wasImported(testFileEntry, SPACE_CAMP)
                  .then(assertTrue);
            });
      });

  reportPromise(testPromise, callback);
}

function testMarkImport_FiresChangedEvent(callback) {
  testPromise = historyProvider.then(
      function(history) {
        var recorder = new TestCallRecorder();
        history.addObserver(recorder.callback);
        return history.markImported(testFileEntry, SPACE_CAMP).then(
            function() {
              return Promise.resolve()
                  .then(
                      function() {
                        recorder.assertCallCount(1);
                        assertEquals(
                            importer.ImportHistoryState.IMPORTED,
                            recorder.getLastArguments()[0]['state']);
                      });
            });
      });

  reportPromise(testPromise, callback);
}

function testHistoryObserver_Unsubscribe(callback) {
  testPromise = historyProvider.then(
      function(history) {
        var recorder = new TestCallRecorder();
        history.addObserver(recorder.callback);
        history.removeObserver(recorder.callback);

        var promises = [];
        promises.push(history.markCopied(testFileEntry, SPACE_CAMP, 'url2'));
        promises.push(history.markImported(testFileEntry, SPACE_CAMP));
        return Promise.all(promises).then(
            function() {
              return Promise.resolve()
                  .then(
                      function() {
                        recorder.assertCallCount(0);
                      });
            });
      });

  reportPromise(testPromise, callback);
}

function testRecordStorage_RemembersPreviouslyWrittenRecords(callback) {
  var recorder = new TestCallRecorder();
  testPromise = createRealStorage(['recordStorageTest.data'])
      .then(
          function(storage) {
            return storage.write(['abc', '123']).then(
                function() {
                  return storage.readAll(recorder.callback).then(
                      function() {
                        recorder.assertCallCount(1);
                      });
                });
          });

  reportPromise(testPromise, callback);
}

function testRecordStorage_LoadsRecordsFromMultipleHistoryFiles(callback) {
  var recorder = new TestCallRecorder();

  var remoteData = createRealStorage(['multiStorage-1.data'])
      .then(
          function(storage) {
            return storage.write(['remote-data', '98765432']);
          });
  var moreRemoteData = createRealStorage(['multiStorage-2.data'])
      .then(
          function(storage) {
            return storage.write(['antarctica-data', '777777777777']);
          });

  testPromise = Promise.all([remoteData, moreRemoteData]).then(
    function() {
      return createRealStorage([
        'multiStorage-0.data',
        'multiStorage-1.data',
        'multiStorage-2.data'])
        .then(
            function(storage) {
              var writePromises = [
                storage.write(['local-data', '111'])
              ];
              return Promise.all(writePromises)
                  .then(
                      function() {
                        return storage.readAll(recorder.callback).then(
                            function() {
                              recorder.assertCallCount(3);
                              assertEquals(
                                  'local-data',
                                  recorder.getArguments(0)[0][0]);
                              assertEquals(
                                  'remote-data',
                                  recorder.getArguments(1)[0][0]);
                              assertEquals(
                                  'antarctica-data',
                                  recorder.getArguments(2)[0][0]);
                            });
                      });
            });
    });

  reportPromise(testPromise, callback);
}

function testRecordStorage_SerializingOperations(callback) {
  var recorder = new TestCallRecorder();
  testPromise = createRealStorage(['recordStorageTestForSerializing.data'])
      .then(
          function(storage) {
            var writePromises = [];
            var WRITES_COUNT = 20;
            for (var i = 0; i < WRITES_COUNT; i++)
              writePromises.push(storage.write(['abc', '123']));
            var readAllPromise = storage.readAll(recorder.callback).then(
              function() {
                recorder.assertCallCount(WRITES_COUNT);
              });
            // Write an extra record, which must be executed afte reading is
            // completed.
            writePromises.push(storage.write(['abc', '123']));
            return Promise.all(writePromises.concat([readAllPromise]));
          });

  reportPromise(testPromise, callback);
}

function testCreateMetadataHashcode(callback) {
  var promise =
      importer.createMetadataHashcode(testFileEntry).then(function(hashcode) {
        // Note that the expression matches at least 4 numbers
        // in the last segment, since we hard code the byte
        // size in our test file to a four digit size.
        // In reality it will vary.
        assertEquals(
            0, hashcode.search(/[\-0-9]{9,}_[0-9]{4,}/),
            'Hashcode (' + hashcode + ') does not match next pattern.');
      });

  reportPromise(promise, callback);
}

/**
 * Installs stub APIs.
 */
function setupChromeApis() {
  new MockChromeStorageAPI();
  chrome = chrome || {};
  chrome.fileManagerPrivate = {};
  chrome.fileManagerPrivate.onFileTransfersUpdated = {
    addListener: function() {}
  };
  chrome.syncFileSystem = {};
}

/**
 * Installs stub APIs.
 */
function installTestLogger() {
  testLogger = new importer.TestLogger();
  importer.getLogger = function() {
    return testLogger;
  };
}

/**
 * @param {!Array<string>} fileNames
 * @return {!Promise<!importer.RecordStorage>}
 */
function createRealStorage(fileNames) {
  var filePromises = fileNames.map(createFileEntry);
  var tracker = new TestTracker();
  return Promise.all(filePromises)
      .then(
          function(fileEntries) {
            return new importer.FileBasedRecordStorage(fileEntries, tracker);
          });
}

/**
 * Creates a *real* FileEntry in the DOM filesystem.
 *
 * @param {string} fileName
 * @return {!Promise<!FileEntry>}
 */
function createFileEntry(fileName) {
  return new Promise(
      function(resolve, reject) {
        var onFileSystemReady = function(fileSystem) {
          fileSystem.root.getFile(
              fileName,
              {
                create: true,
                exclusive: false
              },
              resolve,
              reject);
        };

        window.webkitRequestFileSystem(
            TEMPORARY,
            1024 * 1024,
            onFileSystemReady,
            reject);
      });
}

/**
 * In-memory test implementation of {@code RecordStorage}.
 *
 * @constructor
 * @implements {importer.RecordStorage}
 * @struct
 */
var TestRecordStorage = function() {

  var timeStamp = importer.toSecondsFromEpoch(
        FILE_LAST_MODIFIED);
  // Pre-populate the store with some "previously written" data <wink>.
  /** @private {!Array<!Array<string>>} */
  this.records_ = [
    [1,
      timeStamp + '_' + FILE_SIZE, GOOGLE_DRIVE],
    [0,
      timeStamp + '_' + FILE_SIZE,
      'google-drive',
      '$/some/url/snazzy.pants',
      '$/someother/url/snazzy.pants'],
    [1, '99999_99999', SPACE_CAMP]
  ];

  /**
   * @override
   * @this {TestRecordStorage}
   */
  this.readAll = function(recordCallback) {
    this.records_.forEach(recordCallback);
    return Promise.resolve(this.records_);
  };

  /**
   * @override
   * @this {TestRecordStorage}
   */
  this.write = function(record) {
    this.records_.push(record);
    return Promise.resolve();
  };
};

/**
 * Test implementation of SyncFileEntryProvider.
 *
 * @constructor
 * @implements {importer.SyncFileEntryProvider}
 * @final
 * @struct
 *
 * @param {!FileEntry} fileEntry
 */
var TestSyncFileEntryProvider = function(fileEntry) {
  /** @private {!FileEntry} */
  this.fileEntry_ = fileEntry;

  /**
   * @override
   * @this {TestSyncFileEntryProvider}
   */
  this.getSyncFileEntry = function() {
    return Promise.resolve(this.fileEntry_);
  };
};
