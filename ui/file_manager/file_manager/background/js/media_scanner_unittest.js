// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Stub out the metrics package.
 * @type {!Object<!string, !Function>}
 */
var metrics = {
  recordTime: function() {},
  recordValue: function() {}
};

/** @type {!importer.DefaultMediaScanner} */
var scanner;

/** @type {!importer.TestImportHistory} */
var importHistory;

/** @type {!importer.TestDirectoryWatcher} */
var watcher;

/**
 * @type {function(!FileEntry, !importer.Destination):
 *     !Promise<!importer.Disposition>}
 */
var dispositionChecker;

// Set up the test components.
function setUp() {


  importHistory = new importer.TestImportHistory();
  // This is the default disposition checker.
  // Tests can replace this at runtime if they
  // want specialized behaviors.
  dispositionChecker = function() {
    return Promise.resolve(importer.Disposition.ORIGINAL);
  };

  scanner = new importer.DefaultMediaScanner(
      /** @param {!FileEntry} entry */
      function(entry) {
        return Promise.resolve(entry.name);
      },
      function(entry, destination) {
        return dispositionChecker(entry, destination);
      },
      function(callback) {
        watcher = new TestDirectoryWatcher(callback);
        return watcher;
      });
}

/**
 * Verifies that scanning an empty filesystem produces an empty list.
 */
function testEmptySourceList() {
  assertThrows(
    function() {
      scanner.scanFiles([]);
    });
}

function testIsScanning(callback) {
  var filenames = [
    'happy',
    'thoughts'
  ];
  reportPromise(
      makeTestFileSystemRoot('testIsScanning')
          .then(populateDir.bind(null, filenames))
          .then(
              /**
               * Scans the directory.
               * @param {!DirectoryEntry} root
               */
              function(root) {
                var results = scanner.scanDirectory(root);
                assertFalse(results.isFinal());
              }),
      callback);
}

function testObserverNotifiedOnScanFinish(callback) {
  var filenames = [
    'happy',
    'thoughts'
  ];
  makeTestFileSystemRoot('testObserverNotifiedOnScanFinish')
      .then(populateDir.bind(null, filenames))
      .then(
          /**
           * Scans the directory.
           * @param {!DirectoryEntry} root
           */
          function(root) {
            // Kick off a scan so we can get notified of a scan being finished.
            // We kick this off first so we can capture the result for
            // use in an assert. Promises ensure the scan won't finish
            // until after our funciton is fully processed.
            var result = scanner.scanDirectory(root);
            scanner.addObserver(
                function(eventType, scanResult) {
                  assertEquals(importer.ScanEvent.FINALIZED, eventType);
                  assertEquals(result, scanResult);
                  callback(false);
                });
          })
      .catch(
          function() {
            callback(true);
          });
}

/**
 * Verifies that scanFiles slurps up all specified files.
 */
function testScanFiles(callback) {
  var filenames = [
    'foo',
    'foo.jpg',
    'bar.gif',
    'baz.avi'
  ];
  var expectedFiles = [
    '/testScanFiles/foo.jpg',
    '/testScanFiles/bar.gif',
    '/testScanFiles/baz.avi'
  ];
  reportPromise(
      makeTestFileSystemRoot('testScanFiles')
          .then(populateDir.bind(null, filenames))
          .then(fileOperationUtil.gatherEntriesRecursively)
          .then(
              /** @param {!Array<!FileEntry>} files */
              function(files) {
                return scanner.scanFiles(files).whenFinal();
              })
          .then(assertFilesFound.bind(null, expectedFiles)),
      callback);
}

/**
 * Verifies that scanFiles skips duplicated files.
 */
function testScanFilesIgnoresPreviousImports(callback) {
  var filenames = [
    'oldimage1234.jpg',    // a history duplicate
    'driveimage1234.jpg',  // a content duplicate
    'foo.jpg',
    'bar.gif',
    'baz.avi'
  ];

  // Replace the default dispositionChecker with a function
  // that treats our dupes accordingly.
  dispositionChecker = function(entry, destination) {
    if (entry.name === filenames[0]) {
      return Promise.resolve(importer.Disposition.HISTORY_DUPLICATE);
    }
    if (entry.name === filenames[1]) {
      return Promise.resolve(importer.Disposition.CONTENT_DUPLICATE);
    }
    return Promise.resolve(importer.Disposition.ORIGINAL);
  };

  var expectedFiles = [
    '/testScanFilesIgnoresPreviousImports/foo.jpg',
    '/testScanFilesIgnoresPreviousImports/bar.gif',
    '/testScanFilesIgnoresPreviousImports/baz.avi'
  ];
  reportPromise(
      makeTestFileSystemRoot('testScanFilesIgnoresPreviousImports')
          .then(populateDir.bind(null, filenames))
          .then(fileOperationUtil.gatherEntriesRecursively)
          .then(
              /** @param {!Array<!FileEntry>} files */
              function(files) {
                return scanner.scanFiles(files).whenFinal();
              })
          .then(assertFilesFound.bind(null, expectedFiles)),
      callback);
}

/**
 * Verifies that scanning a simple single-level directory structure works.
 */
function testEmptyScanResults(callback) {
  var filenames = [
    'happy',
    'thoughts'
  ];
  reportPromise(
      makeTestFileSystemRoot('testEmptyScanResults')
          .then(populateDir.bind(null, filenames))
          .then(
              /**
               * Scans the directory.
               * @param {!DirectoryEntry} root
               */
              function(root) {
                return scanner.scanDirectory(root).whenFinal();
              })
          .then(assertFilesFound.bind(null, [])),
      callback);
}

/**
 * Verifies that scanning a simple single-level directory structure works.
 */
function testSingleLevel(callback) {
  var filenames = [
    'foo',
    'foo.jpg',
    'bar.gif',
    'baz.avi',
    'foo.mp3',
    'bar.txt'
  ];
  var expectedFiles = [
    '/testSingleLevel/foo.jpg',
    '/testSingleLevel/bar.gif',
    '/testSingleLevel/baz.avi'
  ];
  reportPromise(
      makeTestFileSystemRoot('testSingleLevel')
          .then(populateDir.bind(null, filenames))
          .then(
              /**
               * Scans the directory.
               * @param {!DirectoryEntry} root
               */
              function(root) {
                return scanner.scanDirectory(root).whenFinal();
              })
          .then(assertFilesFound.bind(null, expectedFiles)),
      callback);
}

/**
 * Verifies that scanning a simple single-level directory produces 100%
 * progress at completion.
 */
function testProgress(callback) {
  var filenames = [
    'foo',
    'foo.jpg',
    'bar.gif',
    'baz.avi',
    'foo.mp3',
    'bar.txt'
  ];
  var expectedFiles = [
    '/testProgress/foo.jpg',
    '/testProgress/bar.gif',
    '/testProgress/baz.avi'
  ];
  reportPromise(
      makeTestFileSystemRoot('testProgress')
          .then(populateDir.bind(null, filenames))
          .then(
              /**
               * Scans the directory.
               * @param {!DirectoryEntry} root
               */
              function(root) {
                return scanner.scanDirectory(root).whenFinal();
              })
          .then(assertProgress.bind(null, 100)),
      callback);
}

/**
 * Verifies that scanning ignores previously imported entries.
 */
function testIgnoresPreviousImports(callback) {
  importHistory.importedPaths[
      '/testIgnoresPreviousImports/oldimage1234.jpg'] =
          [importer.Destination.GOOGLE_DRIVE];
  var filenames = [
    'oldimage1234.jpg',    // a history duplicate
    'driveimage1234.jpg',  // a content duplicate
    'foo.jpg',
    'bar.gif',
    'baz.avi'
  ];

  // Replace the default dispositionChecker with a function
  // that treats our dupes accordingly.
  dispositionChecker = function(entry, destination) {
    if (entry.name === filenames[0]) {
      return Promise.resolve(importer.Disposition.HISTORY_DUPLICATE);
    }
    if (entry.name === filenames[1]) {
      return Promise.resolve(importer.Disposition.CONTENT_DUPLICATE);
    }
    return Promise.resolve(importer.Disposition.ORIGINAL);
  };

  var expectedFiles = [
    '/testIgnoresPreviousImports/foo.jpg',
    '/testIgnoresPreviousImports/bar.gif',
    '/testIgnoresPreviousImports/baz.avi'
  ];

  var promise = makeTestFileSystemRoot('testIgnoresPreviousImports')
                    .then(populateDir.bind(null, filenames))
                    .then(
                        /**
                         * Scans the directory.
                         * @param {!DirectoryEntry} root
                         */
                        function(root) {
                          return scanner.scanDirectory(root).whenFinal();
                        })
                    .then(assertFilesFound.bind(null, expectedFiles));

  reportPromise(promise, callback);
}

function testTracksDuplicates(callback) {
  importHistory.importedPaths[
      '/testTracksDuplicates/oldimage1234.jpg'] =
          [importer.Destination.GOOGLE_DRIVE];
  var filenames = [
    'oldimage1234.jpg',    // a history duplicate
    'driveimage1234.jpg',  // a content duplicate
    'driveimage9999.jpg',  // a content duplicate
    'bar.gif',
    'baz.avi'
  ];

  // Replace the default dispositionChecker with a function
  // that treats our dupes accordingly.
  dispositionChecker = function(entry, destination) {
    if (entry.name === filenames[0]) {
      return Promise.resolve(importer.Disposition.HISTORY_DUPLICATE);
    }
    if (entry.name === filenames[1]) {
      return Promise.resolve(importer.Disposition.CONTENT_DUPLICATE);
    }
    if (entry.name === filenames[2]) {
      return Promise.resolve(importer.Disposition.CONTENT_DUPLICATE);
    }
    return Promise.resolve(importer.Disposition.ORIGINAL);
  };

  var expectedDuplicates = [
    '/testTracksDuplicates/driveimage1234.jpg',
    '/testTracksDuplicates/driveimage9999.jpg'
  ];


  var promise = makeTestFileSystemRoot('testTracksDuplicates')
      .then(populateDir.bind(null, filenames))
      .then(
          /**
           * Scans the directory.
           * @param {!DirectoryEntry} root
           */
          function(root) {
            return scanner.scanDirectory(root).whenFinal();
          })
      .then(assertDuplicatesFound.bind(null, expectedDuplicates));

  reportPromise(promise, callback);
}

function testMultiLevel(callback) {
  var filenames = [
    'foo.jpg',
    'bar',
    [
      'dir1',
      'bar.0.jpg'
    ],
    [
      'dir2',
      'bar.1.gif',
      [
        'dir3',
        'bar.1.0.avi'
      ]
    ]
  ];
  var expectedFiles = [
    '/testMultiLevel/foo.jpg',
    '/testMultiLevel/dir1/bar.0.jpg',
    '/testMultiLevel/dir2/bar.1.gif',
    '/testMultiLevel/dir2/dir3/bar.1.0.avi'
  ];

  reportPromise(
      makeTestFileSystemRoot('testMultiLevel')
          .then(populateDir.bind(null, filenames))
          .then(
              /**
               * Scans the directory.
               * @param {!DirectoryEntry} root
               */
              function(root) {
                return scanner.scanDirectory(root).whenFinal();
              })
          .then(assertFilesFound.bind(null, expectedFiles)),
      callback);
}

function testDedupesFilesInScanResult(callback) {
  var filenames = [
    'foo.jpg',
    'bar.jpg',
    [
      'dir1',
      'foo.jpg',
      'bar.jpg'
    ],
    [
      'dir2',
      'foo.jpg',
      'bar.jpg',
      [
        'dir3',
        'foo.jpg',
        'bar.jpg'
      ]
    ]
  ];
  var expectedFiles = [
    '/testDedupesFilesInScanResult/foo.jpg',
    '/testDedupesFilesInScanResult/bar.jpg'
  ];

  reportPromise(
      makeTestFileSystemRoot('testDedupesFilesInScanResult')
          .then(populateDir.bind(null, filenames))
          .then(
              /**
               * Scans the directory.
               * @param {!DirectoryEntry} root
               */
              function(root) {
                return scanner.scanDirectory(root).whenFinal();
              })
          .then(assertFilesFound.bind(null, expectedFiles)),
      callback);
}

/**
 * Verifies that scanning a simple single-level directory structure works.
 */
function testDefaultScanResult() {
  var hashGenerator = function(file) {
    return file.toURL();
  };
  var scan = new importer.DefaultScanResult(hashGenerator);

  // 0 before we set candidate count
  assertProgress(0, scan);

  scan.setCandidateCount(3);
  assertProgress(0, scan);

  scan.onCandidatesProcessed(1);
  assertProgress(33, scan);
  scan.onCandidatesProcessed(1);
  assertProgress(66, scan);
  scan.onCandidatesProcessed(1);
  assertProgress(100, scan);
}

function testInvalidation(callback) {
  var invalidatePromise = new Promise(function(fulfill) {
    scanner.addObserver(fulfill);
  });
  reportPromise(
      makeTestFileSystemRoot('testInvalidation')
          .then(populateDir.bind(null, ['DCIM']))
          .then(
              /**
               * Scans the directories.
               * @param {!DirectoryEntry} root
               */
              function(root) {
                scan = scanner.scanDirectory(root);
                watcher.callback();
                return invalidatePromise;
              }),
              callback);
}

/**
 * Verifies the results of the media scan are as expected.
 * @param {number} expected, 0-100
 * @param {!importer.ScanResults} scan
 * @return {!importer.ScanResults}
 */
function assertProgress(expected, scan) {
  assertEquals(expected, scan.getStatistics().progress);
  return scan;
}

/**
 * Verifies the results of the media scan are as expected.
 * @param {!Array<string>} expected
 * @param {!importer.ScanResults} scan
 * @return {!importer.ScanResults}
 */
function assertFilesFound(expected, scan) {
  assertFileEntryPathsEqual(expected, scan.getFileEntries());
  return scan;
  // TODO(smckay): Add coverage for getScanDurationMs && getTotalBytes.
}

/**
 * Verifies the results of the media scan are as expected.
 * @param {!Array<string>} expected
 * @param {!importer.ScanResults} scan
 * @return {!importer.ScanResults}
 */
function assertDuplicatesFound(expected, scan) {
  assertFileEntryPathsEqual(expected, scan.getDuplicateFileEntries());
  return scan;
}

/**
 * Creates a subdirectory within a temporary file system for testing.
 *
 * @param {string} directoryName Name of the test directory to create.  Must be
 *     unique within this test suite.
 */
function makeTestFileSystemRoot(directoryName) {
  function makeTestFilesystem() {
    return new Promise(function(resolve, reject) {
      window.webkitRequestFileSystem(
          window.TEMPORARY,
          1024 * 1024,
          resolve,
          reject);
    });
  }

  return makeTestFilesystem()
      .then(
          // Create a directory, pretend that's the root.
          function(fs) {
            return new Promise(function(resolve, reject) {
              fs.root.getDirectory(
                    directoryName,
                    {
                      create: true,
                      exclusive: true
                    },
                  resolve,
                  reject);
            });
          });
}

/**
 * Creates a set of files in the given directory.
 * @param {!Array<!Array|string>} filenames A (potentially nested) array of
 *     strings, reflecting a directory structure.
 * @param {!DirectoryEntry} dir The root of the directory tree.
 * @return {!Promise<!DirectoryEntry>} The root of the newly populated
 *     directory tree.
 */
function populateDir(filenames, dir) {
  return Promise.all(
      filenames.map(
          function(filename) {
            if (filename instanceof Array) {
              return new Promise(
                  function(resolve, reject) {
                    dir.getDirectory(
                        filename[0],
                        {create: true},
                        resolve,
                        reject);
                  })
                  .then(populateDir.bind(null, filename));
            } else {
              return new Promise(
                  function(resolve, reject) {
                    dir.getFile(filename, {create: true}, resolve, reject);
                  });
            }
          })).then(
              function() {
                return dir;
              });
}
