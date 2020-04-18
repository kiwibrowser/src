// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @type {!MockFileOperationManager} */
var progressCenter;

/** @type {!TestMediaScanner} */
var mediaScanner;

/** @type {!importer.MediaImportHandler} */
var mediaImporter;

/** @type {!importer.TestImportHistory} */
var importHistory;

/** @param {!importer.DispositionChecker.CheckerFunction} */
var dispositionChecker;

/** @type {!VolumeInfo} */
var drive;

/** @type {!MockCopyTo} */
var mockCopier;

/** @type {!MockFileSystem} */
var destinationFileSystem;

/** @type {!importer.TestDuplicateFinder.Factory} */
var duplicateFinderFactory;

/** @type {!Promise<!DirectoryEntry>} */
var destinationFactory;

/** @type {!MockDriveSyncHandler} */
var driveSyncHandler;

// Set up string assets.
loadTimeData.data = {
  CLOUD_IMPORT_ITEMS_REMAINING: '',
  DRIVE_DIRECTORY_LABEL: 'My Drive',
  DOWNLOADS_DIRECTORY_LABEL: 'Downloads'
};

var chrome;

function setUp() {
  // Set up mock chrome APIs.
  chrome = {
    power: {
      requestKeepAwakeWasCalled: false,
      requestKeepAwakeStatus: false,
      requestKeepAwake: function() {
        chrome.power.requestKeepAwakeWasCalled = true;
        chrome.power.requestKeepAwakeStatus = true;
      },
      releaseKeepAwake: function() {
        chrome.power.requestKeepAwakeStatus = false;
      }
    },
    fileManagerPrivate: {
      setEntryTag: function() {}
    }
  };
  new MockCommandLinePrivate();
  importer.setupTestLogger();

  progressCenter = new MockProgressCenter();

  // Replaces fileOperationUtil.copyTo with test function.
  mockCopier = new MockCopyTo();

  var volumeManager = new MockVolumeManager();
  drive = volumeManager.getCurrentProfileVolumeInfo(
      VolumeManagerCommon.VolumeType.DRIVE);
  // Create fake parented and non-parented roots.
  drive.fileSystem.populate([
    '/root/',
    '/other/'
  ]);

  MockVolumeManager.installMockSingleton(volumeManager);

  importHistory = new importer.TestImportHistory();

  // This is the default disposition checker used by mediaImporter.
  // Tests can replace this at runtime if they want specialized behaviors.
  dispositionChecker = function() {
    return Promise.resolve(importer.Disposition.ORIGINAL);
  };

  mediaScanner = new TestMediaScanner();
  destinationFileSystem = new MockFileSystem('googleDriveFilesystem');
  destinationFactory = Promise.resolve(destinationFileSystem.root);
  duplicateFinderFactory = new importer.TestDuplicateFinder.Factory();
  driveSyncHandler = new MockDriveSyncHandler();

  mediaImporter = new importer.MediaImportHandler(
      progressCenter, importHistory, function(entry, destination) {
        return dispositionChecker(entry, destination);
      }, new TestTracker(), driveSyncHandler);
}

function testImportMedia(callback) {
  var media = setupFileSystem([
    '/DCIM/photos0/IMG00001.jpg',
    '/DCIM/photos0/IMG00002.jpg',
    '/DCIM/photos0/IMG00003.jpg',
    '/DCIM/photos1/IMG00004.jpg',
    '/DCIM/photos1/IMG00005.jpg',
    '/DCIM/photos1/IMG00006.jpg'
  ]);

  var scanResult = new TestScanResult(media);
  var importTask = mediaImporter.importFromScanResult(
      scanResult,
      importer.Destination.GOOGLE_DRIVE,
      destinationFactory);
  var whenImportDone = new Promise(
      function(resolve, reject) {
        importTask.addObserver(
            /**
             * @param {!importer.TaskQueue.UpdateType} updateType
             * @param {!importer.TaskQueue.Task} task
             */
            function(updateType, task) {
              switch (updateType) {
                case importer.TaskQueue.UpdateType.COMPLETE:
                  resolve();
                  break;
                case importer.TaskQueue.UpdateType.ERROR:
                  reject(new Error(importer.TaskQueue.UpdateType.ERROR));
                  break;
              }
            });
      });

  reportPromise(
      whenImportDone.then(
        function() {
          var copiedEntries = destinationFileSystem.root.getAllChildren();
          assertEquals(media.length, copiedEntries.length);
        }),
      callback);

  scanResult.finalize();
}

function testImportMedia_skipAndMarkDuplicatedFiles(callback) {
  var DUPLICATED_FILE_PATH_1 = '/DCIM/photos0/duplicated_1.jpg';
  var DUPLICATED_FILE_PATH_2 = '/DCIM/photos0/duplicated_2.jpg';
  var ORIGINAL_FILE_NAME = 'new_image.jpg';
  var ORIGINAL_FILE_SRC_PATH = '/DCIM/photos0/' + ORIGINAL_FILE_NAME;
  var ORIGINAL_FILE_DEST_PATH = '/' + ORIGINAL_FILE_NAME;
  var media = setupFileSystem([
    DUPLICATED_FILE_PATH_1,
    ORIGINAL_FILE_NAME,
    DUPLICATED_FILE_PATH_2,
  ]);

  dispositionChecker = function(entry, destination) {
    if (entry.fullPath == DUPLICATED_FILE_PATH_1) {
      return Promise.resolve(importer.Disposition.HISTORY_DUPLICATE);
    }
    if (entry.fullPath == DUPLICATED_FILE_PATH_2) {
      return Promise.resolve(importer.Disposition.CONTENT_DUPLICATE);
    }
    return Promise.resolve(importer.Disposition.ORIGINAL);
  };
  mediaImporter = new importer.MediaImportHandler(
      progressCenter, importHistory, dispositionChecker, new TestTracker(),
      driveSyncHandler);
  var scanResult = new TestScanResult(media);
  var importTask = mediaImporter.importFromScanResult(
      scanResult,
      importer.Destination.GOOGLE_DRIVE,
      destinationFactory);
  var whenImportDone = new Promise(
      function(resolve, reject) {
        importTask.addObserver(
            /**
             * @param {!importer.TaskQueue.UpdateType} updateType
             * @param {!importer.TaskQueue.Task} task
             */
            function(updateType, task) {
              switch (updateType) {
                case importer.TaskQueue.UpdateType.COMPLETE:
                  resolve();
                  break;
                case importer.TaskQueue.UpdateType.ERROR:
                  reject(new Error(importer.TaskQueue.UpdateType.ERROR));
                  break;
              }
            });
      });

  reportPromise(
      whenImportDone.then(
          function() {
            // Only the new file should be copied.
            var copiedEntries = destinationFileSystem.root.getAllChildren();
            assertEquals(1, copiedEntries.length);
            assertEquals(ORIGINAL_FILE_DEST_PATH, copiedEntries[0].fullPath);
            importHistory.assertCopied(media[1],
                                       importer.Destination.GOOGLE_DRIVE);
            // The 2 duplicated files should be marked as imported.
            [media[0], media[2]].forEach(
                /** @param {!FileEntry} entry */
                function(entry) {
                  importHistory.assertImported(
                      entry, importer.Destination.GOOGLE_DRIVE);
                });
          }), callback);

  scanResult.finalize();
}

function testImportMedia_EmploysEncodedUrls(callback) {
  var media = setupFileSystem([
    '/DCIM/photos0/Mom and Dad.jpg',
  ]);

  var scanResult = new TestScanResult(media);
  var importTask = mediaImporter.importFromScanResult(
      scanResult,
      importer.Destination.GOOGLE_DRIVE,
      destinationFactory);

  var promise = new Promise(
      function(resolve, reject) {
        importTask.addObserver(
            /**
             * @param {!importer.TaskQueue.UpdateType} updateType
             * @param {!importer.TaskQueue.Task} task
             */
            function(updateType, task) {
              switch (updateType) {
                case importer.TaskQueue.UpdateType.COMPLETE:
                  resolve(destinationFileSystem.root.getAllChildren());
                  break;
                case importer.TaskQueue.UpdateType.ERROR:
                  reject('Task failed :(');
                  break;
              }
            });
      })
      .then(
        function(copiedEntries) {
          var expected = 'Mom%20and%20Dad.jpg';
          var url = copiedEntries[0].toURL();
          assertTrue(url.length > expected.length);
          var actual = url.substring(url.length - expected.length);
          assertEquals(expected, actual);
        });

  reportPromise(promise, callback);

  scanResult.finalize();
}

// Verifies that when files with duplicate names are imported, that they don't
// overwrite one another.
function testImportMediaWithDuplicateFilenames(callback) {
  var media = setupFileSystem([
    '/DCIM/photos0/IMG00001.jpg',
    '/DCIM/photos0/IMG00002.jpg',
    '/DCIM/photos0/IMG00003.jpg',
    '/DCIM/photos1/IMG00001.jpg',
    '/DCIM/photos1/IMG00002.jpg',
    '/DCIM/photos1/IMG00003.jpg'
  ]);

  var scanResult = new TestScanResult(media);
  var importTask = mediaImporter.importFromScanResult(
      scanResult,
      importer.Destination.GOOGLE_DRIVE,
      destinationFactory);
  var whenImportDone = new Promise(
      function(resolve, reject) {
        importTask.addObserver(
            /**
             * @param {!importer.TaskQueue.UpdateType} updateType
             * @param {!importer.TaskQueue.Task} task
             */
            function(updateType, task) {
              switch (updateType) {
                case importer.TaskQueue.UpdateType.COMPLETE:
                  resolve();
                  break;
                case importer.TaskQueue.UpdateType.ERROR:
                  reject(new Error(importer.TaskQueue.UpdateType.ERROR));
                  break;
              }
            });
      });

  // Verify that we end up with 6, and not 3, destination entries.
  reportPromise(
      whenImportDone.then(
        function() {
          var copiedEntries = destinationFileSystem.root.getAllChildren();
          assertEquals(media.length, copiedEntries.length);
        }),
      callback);

  scanResult.finalize();
}

function testKeepAwakeDuringImport(callback) {
  var media = setupFileSystem([
    '/DCIM/photos0/IMG00001.jpg',
    '/DCIM/photos0/IMG00002.jpg',
    '/DCIM/photos0/IMG00003.jpg',
    '/DCIM/photos1/IMG00004.jpg',
    '/DCIM/photos1/IMG00005.jpg',
    '/DCIM/photos1/IMG00006.jpg'
  ]);

  var scanResult = new TestScanResult(media);
  var importTask = mediaImporter.importFromScanResult(
      scanResult,
      importer.Destination.GOOGLE_DRIVE,
      destinationFactory);
  var whenImportDone = new Promise(
      function(resolve, reject) {
        importTask.addObserver(
            /**
             * @param {!importer.TaskQueue.UpdateType} updateType
             * @param {!importer.TaskQueue.Task} task
             */
            function(updateType, task) {
              // Assert that keepAwake is set while the task is active.
              assertTrue(chrome.power.requestKeepAwakeStatus);
              switch (updateType) {
                case importer.TaskQueue.UpdateType.COMPLETE:
                  resolve();
                  break;
                case importer.TaskQueue.UpdateType.ERROR:
                  reject(new Error(importer.TaskQueue.UpdateType.ERROR));
                  break;
              }
            });
      });

  reportPromise(
      whenImportDone.then(
        function() {
          assertTrue(chrome.power.requestKeepAwakeWasCalled);
          assertFalse(chrome.power.requestKeepAwakeStatus);
          var copiedEntries = destinationFileSystem.root.getAllChildren();
          assertEquals(media.length, copiedEntries.length);
        }),
      callback);

  scanResult.finalize();
}

function testUpdatesHistoryAfterImport(callback) {
  var entries = setupFileSystem([
    '/DCIM/photos0/IMG00001.jpg',
    '/DCIM/photos1/IMG00003.jpg',
    '/DCIM/photos0/DRIVEDUPE00001.jpg',
    '/DCIM/photos1/DRIVEDUPE99999.jpg'
  ]);

  var newFiles = entries.slice(0, 2);
  var dupeFiles = entries.slice(2);

  var scanResult = new TestScanResult(entries.slice(0, 2));
  scanResult.duplicateFileEntries = dupeFiles;
  var importTask = mediaImporter.importFromScanResult(
      scanResult,
      importer.Destination.GOOGLE_DRIVE,
      destinationFactory);

  var whenImportDone = new Promise(
      function(resolve, reject) {
        importTask.addObserver(
            /**
             * @param {!importer.TaskQueue.UpdateType} updateType
             * @param {!importer.TaskQueue.Task} task
             */
            function(updateType, task) {
              switch (updateType) {
                case importer.TaskQueue.UpdateType.COMPLETE:
                  resolve();
                  break;
                case importer.TaskQueue.UpdateType.ERROR:
                  reject(new Error(importer.TaskQueue.UpdateType.ERROR));
                  break;
              }
            });
      });

  var promise = whenImportDone.then(
      function() {
        mockCopier.copiedFiles.forEach(
            /** @param {!MockCopyTo.CopyInfo} copy */
            function(copy) {
              importHistory.assertCopied(
                  copy.source, importer.Destination.GOOGLE_DRIVE);
            });
        dupeFiles.forEach(
            /** @param {!FileEntry} entry */
            function(entry) {
              importHistory.assertImported(
                  entry, importer.Destination.GOOGLE_DRIVE);
            });
      });

  scanResult.finalize();
  reportPromise(promise, callback);
}

function testTagsEntriesAfterImport(callback) {
  var entries = setupFileSystem([
    '/DCIM/photos0/IMG00001.jpg',
    '/DCIM/photos1/IMG00003.jpg'
  ]);

  var scanResult = new TestScanResult(entries);
  var importTask = mediaImporter.importFromScanResult(
      scanResult,
      importer.Destination.GOOGLE_DRIVE,
      destinationFactory);
  var whenImportDone = new Promise(
      function(resolve, reject) {
        importTask.addObserver(
            /**
             * @param {!importer.TaskQueue.UpdateType} updateType
             * @param {!importer.TaskQueue.Task} task
             */
            function(updateType, task) {
              switch (updateType) {
                case importer.TaskQueue.UpdateType.COMPLETE:
                  resolve();
                  break;
                case importer.TaskQueue.UpdateType.ERROR:
                  reject(new Error(importer.TaskQueue.UpdateType.ERROR));
                  break;
              }
            });
      });

  var taggedEntries = [];
  // Replace chrome.fileManagerPrivate.setEntryTag with a listener.
  chrome.fileManagerPrivate.setEntryTag = function(entry) {
    taggedEntries.push(entry);
  };

  reportPromise(
      whenImportDone.then(
          function() {
            assertEquals(entries.length, taggedEntries.length);
          }),
      callback);

  scanResult.finalize();
}

// Tests that cancelling an import works properly.
function testImportCancellation(callback) {
  var media = setupFileSystem([
    '/DCIM/photos0/IMG00001.jpg',
    '/DCIM/photos0/IMG00002.jpg',
    '/DCIM/photos0/IMG00003.jpg',
    '/DCIM/photos1/IMG00004.jpg',
    '/DCIM/photos1/IMG00005.jpg',
    '/DCIM/photos1/IMG00006.jpg'
  ]);

  /** @const {number} */
  var EXPECTED_COPY_COUNT = 3;

  var scanResult = new TestScanResult(media);
  var importTask = mediaImporter.importFromScanResult(
      scanResult,
      importer.Destination.GOOGLE_DRIVE,
      destinationFactory);
  var whenImportCancelled = new Promise(
      function(resolve, reject) {
        importTask.addObserver(
            /**
             * @param {!importer.TaskQueue.UpdateType} updateType
             * @param {!importer.TaskQueue.Task} task
             */
            function(updateType, task) {
              if (updateType === importer.TaskQueue.UpdateType.CANCELED) {
                resolve();
              }
            });
      });

  // Simulate cancellation after the expected number of copies is done.
  var copyCount = 0;
  importTask.addObserver(function(updateType) {
    if (updateType ===
        importer.MediaImportHandler.ImportTask.UpdateType.ENTRY_CHANGED) {
      copyCount++;
      if (copyCount === EXPECTED_COPY_COUNT) {
        importTask.requestCancel();
      }
    }
  });

  reportPromise(
      whenImportCancelled.then(
        function() {
          var copiedEntries = destinationFileSystem.root.getAllChildren();
          assertEquals(EXPECTED_COPY_COUNT, copiedEntries.length);
        }),
      callback);

  scanResult.finalize();
}

function testImportWithErrors(callback) {

  // Quiet the logger just in this test where we expect errors.
  // Elsewhere, it's better for errors to be seen by test authors.
  importer.setupTestLogger().quiet();

  var media = setupFileSystem([
    '/DCIM/photos0/IMG00001.jpg',
    '/DCIM/photos0/IMG00002.jpg',
    '/DCIM/photos0/IMG00003.jpg',
    '/DCIM/photos1/IMG00004.jpg',
    '/DCIM/photos1/IMG00005.jpg',
    '/DCIM/photos1/IMG00006.jpg'
  ]);

  /** @const {number} */
  var EXPECTED_COPY_COUNT = 5;

  var scanResult = new TestScanResult(media);
  var importTask = mediaImporter.importFromScanResult(
      scanResult,
      importer.Destination.GOOGLE_DRIVE,
      destinationFactory);
  var whenImportDone = new Promise(
      function(resolve, reject) {
        importTask.addObserver(
            /**
             * @param {!importer.TaskQueue.UpdateType} updateType
             * @param {!importer.TaskQueue.Task} task
             */
            function(updateType, task) {
              if (updateType === importer.TaskQueue.UpdateType.COMPLETE) {
                resolve();
              }
            });
      });

  // Simulate an error after 3 imports.
  var copyCount = 0;
  importTask.addObserver(function(updateType) {
    if (updateType ===
        importer.MediaImportHandler.ImportTask.UpdateType.ENTRY_CHANGED) {
      copyCount++;
      if (copyCount === 3) {
        mockCopier.simulateOneError();
      }
    }
  });

  // Verify that the error didn't result in some files not being copied.
  reportPromise(
      whenImportDone.then(
        function() {
          var copiedEntries = destinationFileSystem.root.getAllChildren();
          assertEquals(EXPECTED_COPY_COUNT, copiedEntries.length);
        }),
      callback);

  scanResult.finalize();
}

/**
 * @param {!Array<string>} fileNames
 * @return {!Array<!Entry>}
 */
function setupFileSystem(fileNames) {
  // Set up a filesystem with some files.
  var fileSystem = new MockFileSystem('fake-media-volume');
  fileSystem.populate(fileNames);

  return fileNames.map(
      function(filename) {
        return fileSystem.entries[filename];
      });
}

/**
 * Replaces fileOperationUtil.copyTo with some mock functionality for testing.
 * @constructor
 */
function MockCopyTo() {
  /** @type {!Array<!MockCopyTo.CopyInfo>} */
  this.copiedFiles = [];

  // Replace with test function.
  fileOperationUtil.copyTo = this.copyTo_.bind(this);

  this.simulateError_ = false;

  this.entryChangedCallback_ = null;
  this.progressCallback_ = null;
  this.successCallback_ = null;
  this.errorCallback_ = null;
}

/**
 * @typedef {{
 *   source: source,
 *   destination: parent,
 *   newName: newName
 * }}
 */
MockCopyTo.CopyInfo;

/**
 * Makes the mock copier simulate an error the next time copyTo_ is called.
 */
MockCopyTo.prototype.simulateOneError = function() {
  this.simulateError_ = true;
};

/**
 * A mock to replace fileOperationUtil.copyTo.  See the original for details.
 * @param {Entry} source
 * @param {DirectoryEntry} parent
 * @param {string} newName
 * @param {function(string, Entry)} entryChangedCallback
 * @param {function(string, number)} progressCallback
 * @param {function(Entry)} successCallback
 * @param {function(DOMError)} errorCallback
 * @return {function()}
 */
MockCopyTo.prototype.copyTo_ = function(source, parent, newName,
    entryChangedCallback, progressCallback, successCallback, errorCallback) {
  this.entryChangedCallback_ = entryChangedCallback;
  this.progressCallback_ = progressCallback;
  this.successCallback_ = successCallback;
  this.errorCallback_ = errorCallback;

  if (this.simulateError_) {
    this.simulateError_ = false;
    this.errorCallback_(new Error('test error'));
    return;
  }

  // Log the copy, then copy the file.
  this.copiedFiles.push({
    source: source,
    destination: parent,
    newName: newName
  });
  source.copyTo(
      parent,
      newName,
      function(newEntry) {
        this.entryChangedCallback_(source.toURL(), parent);
        this.successCallback_(newEntry);
      }.bind(this),
      this.errorCallback_.bind(this));
};
