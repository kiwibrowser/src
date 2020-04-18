// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * A helper namespace used by integration_tests.js.
 */
var testsHelper = {
  /**
   * The base URL where all test archives are located.
   * @private {string}
   * @const
   */
  TEST_FILES_BASE_URL_: 'http://localhost:9876/test-files/',

  /**
   * Define information for the volumes to check.
   * @type {!Array<!Object>}
   */
  volumesInformation: [],

  /**
   * The local storage that contains the volumes state to restore after suspend
   * event, restarts, crashes, etc. The key is used to differentiate between
   * different values stored in the local storage. For our extension only
   * unpacker.app.STORAGE_KEY is used.
   * @type {!Object<string, !Object>}
   */
  localStorageState: {},

  /**
   * A cache with downloaded files' Blob objects. The key is the file path and
   * the value is a Promise that fulfills with the file's Blob object.
   * @private {!Object<string, !Promise>}
   */
  fileBlobCache_: {},

  /**
   * An app window factory for chrome.app.window.create(). May be changed during
   * tests to create specific fake windows.
   * @type {?function(string, CreateWindowOptions, function(AppWindow))}
   */
  createAppWindow: null,

  /**
   * Downloads a file in order to use it inside the tests. The download
   * operation is required in order to obtain a Blob object for the file,
   * object that is needed by the Decompressor to read the archive's file data
   * or other file's content that must be compared.
   * @param {string} filePath The file path in 'test-files/' directory.
   * @return {!Promise} A promise that fulfills with the file's blob or rejects
   *     with the download failure error.
   */
  getFileBlob: function(filePath) {
    if (testsHelper.fileBlobCache_[filePath])
      return Promise.resolve(testsHelper.fileBlobCache_[filePath]);

    return new Promise(function(fulfill, reject) {
             var xhr = new XMLHttpRequest();
             xhr.open('GET', testsHelper.TEST_FILES_BASE_URL_ + filePath);
             xhr.responseType = 'blob';

             xhr.onload = fulfill.bind(null, xhr);

             xhr.onerror = reject.bind(null, xhr);

             xhr.send(null);
           })
        .then(
            function(xhr) {
              if (xhr.readyState === 4) {
                if (xhr.status === 200) {
                  testsHelper.fileBlobCache_[filePath] = xhr.response;
                  return xhr.response;  // The blob.
                } else {
                  return Promise.reject(xhr.statusText + ': ' + filePath);
                }
              }
            },
            function(xhr) {
              return Promise.reject(xhr.statusText + ': ' + filePath);
            });
  },

  /**
   * Downloads a file's blob and converts it to an Int8Array that can be used
   * for comparisons.
   * @param {string} filePath The file path in 'test-files/' directory.
   * @return {!Promise} A Promise which fulfills with the data as an Int8Array
   *     or rejects with any received error.
   */
  getAndReadFileBlobPromise: function(filePath) {
    return testsHelper.getFileBlob(filePath).then(function(blob) {
      return new Promise(function(fulfill, reject) {
        var fileReader = new FileReader();
        fileReader.onload = function(event) {
          fulfill(new Int8Array(event.target.result));
        };
        fileReader.onerror = reject;
        fileReader.readAsArrayBuffer(blob);
      });
    });
  },

  /**
   * Initializes Chrome APIs.
   */
  initChromeApis: function() {
    // Local storage API.
    chrome.storage = {
      local: {
        set: function() {
          // 'set' must be a function before we can stub it with a custom
          // function. This is a sinon requirement.
        },
        get: function() {
          // Must be a function for same reasons as 'set'.
        }
      }
    };

    sinon.stub(chrome.storage.local, 'set', function(state, opt_successfulSet) {
      // Save the state in the local storage in a different memory.
      testsHelper.localStorageState = JSON.parse(JSON.stringify(state));
      if (opt_successfulSet)
        opt_successfulSet();
    });

    sinon.stub(chrome.storage.local, 'get', function(state, onSuccess) {
      // Make a deep copy as testsHelper.localStorageState is the data on the
      // local storage and not in memory. This way the extension will work on a
      // different memory which is the case in real scenarios.
      var localStorageState =
          JSON.parse(JSON.stringify(testsHelper.localStorageState));
      onSuccess(localStorageState);
    });

    // File system API.
    chrome.fileSystem = {
      retainEntry: sinon.stub(),
      restoreEntry: sinon.stub(),
      getDisplayPath: sinon.stub()
    };

    testsHelper.volumesInformation.forEach(function(volume) {
      chrome.fileSystem.retainEntry.withArgs(volume.entry)
          .returns(volume.entryId);
      chrome.fileSystem.restoreEntry.withArgs(volume.entryId)
          .callsArgWith(1, volume.entry);
      chrome.fileSystem.getDisplayPath.withArgs(volume.entry)
          .callsArgWith(1, volume.fileSystemId);
    });
    chrome.fileSystem.retainEntry.throws('Invalid argument for retainEntry.');
    chrome.fileSystem.restoreEntry.throws('Invalid argument for restoreEntry.');
    chrome.fileSystem.getDisplayPath.throws(
        'Invalid argument for displayPath.');

    // File system provider API.
    chrome.fileSystemProvider = {
      mount: sinon.stub(),
      unmount: sinon.stub(),
      get: function(fileSystemId, callback) {
        var volumeInfoList =
            testsHelper.volumesInformation.filter(function(volumeInfo) {
              return volumeInfo.fileSystemId == fileSystemId;
            });
        if (volumeInfoList.length == 0)
          callback(null);  // No easy way to set lastError.
        else
          callback(volumeInfoList[0].fileSystemMetadata);
      },
      getAll: sinon.stub().callsArgWith(
          0, testsHelper.volumesInformation.map(function(volumeInfo) {
            return volumeInfo.fileSystemMetadata;
          }))
    };
    testsHelper.volumesInformation.forEach(function(volume) {
      chrome.fileSystemProvider.mount
          .withArgs({
            fileSystemId: volume.fileSystemId,
            displayName: volume.entry.name,
            openedFilesLimit: 1
          })
          .callsArg(1);
      chrome.fileSystemProvider.unmount
          .withArgs({fileSystemId: volume.fileSystemId})
          .callsArg(1);
    });
    chrome.fileSystemProvider.mount.throws('Invalid argument for mount.');
    chrome.fileSystemProvider.unmount.throws('Invalid argument for unmount.');

    // Chrome runtime API.
    chrome.runtime = {
      // Contains 'lastError' property which is checked in case
      // chrome.fileSystem.restoreEntry fails. By default 'lastError' should be
      // undefined as no error is returned.
      getManifest: sinon.stub().returns({icons: {}})
    };

    // Chrome notifications API.
    chrome.notifications = {create: sinon.stub(), clear: sinon.stub()};

    // Chrome i18n API.
    chrome.i18n = {
      getMessage: sinon.stub(),
      getUILanguage: sinon.stub().returns('ja')
    };

    // Chrome app window API. Used for the passphrase dialog only.
    chrome.app.window = {create: testsHelper.createAppWindow};
  },

  /**
   * Initializes the tests helper. Should call Promise.then to finish
   * initialization as it is done asynchronously.
   * @param {!Array<!Object>} archivesToTest A list with data about
   *     archives to test. The archives should be present in 'test-files/'
   *     directory. It has 4 properties: 'name', a string representing the
   *     archive's name which has the same value as the file system id,
   *     'afterOnLaunchTests', the tests to run after on launch event,
   *     'afterSuspendTests', the tests to run after suspend page event and
   *     'afterRestartTests', which are the tests to run after restart.
   * @return {Promise} A promise that will finish initialization asynchronously.
   */
  init: function(archivesToTest) {
    // Create promises to obtain archives blob.
    return Promise.all(archivesToTest.map(function(archiveData) {
      // Inititialization is done outside of the promise in order for Mocha to
      // correctly identify the number of testsHelper.volumesInformation when
      // it initialiazes tests. In case this is done in the promise, Mocha
      // will think there is no volumeInformation because at the time the
      // JavaScript test file is parssed testsHelper.volumesInformation will
      // still be empty.
      var fileSystemId = archiveData.name;

      var volumeInformation = {
        expectedMetadata: archiveData.expectedMetadata,
        fileSystemId: fileSystemId,
        entry: {
          file: null,  // Lazy initialization in Promise.
          name: archiveData.name + '_name'
        },
        /**
         * Default type is Entry, but we can't create an Entry object directly
         * with new. String should work because chrome APIs are stubbed.
         */
        entryId: archiveData.name + '_entry',
        /**
         * A functions with archive's tests to run after on launch event.
         * @type {function()}
         */
        afterOnLaunchTests: archiveData.afterOnLaunchTests,
        /**
         * A functions with archive's tests to run after suspend page event.
         * These tests are similiar to above tests just that they should restore
         * volume's state and opened files.
         * @type {function()}
         */
        afterSuspendTests: archiveData.afterSuspendTests,
        /**
         * A functions with archive's tests to run after restart.
         * These tests are similiar to above tests just that they should restore
         * only the volume's state.
         * @type {function()}
         */
        afterRestartTests: archiveData.afterRestartTests,
        /**
         * File system metadata to be returned as part of chrome.
         * fileSystemProvider.getAll() and get().
         * @type {!Object}
         */
        fileSystemMetadata: {
          fileSystemId: fileSystemId,
          displayName: archiveData.name,
          writable: false,
          openedFilesLimit: 1,
          openedFiles: []
        }
      };

      testsHelper.volumesInformation.push(volumeInformation);

      return testsHelper.getFileBlob(archiveData.name).then(function(blob) {
        volumeInformation.entry.file = sinon.stub().callsArgWith(0, blob);
      });
    }));
  },

  /**
   * Create a read file request promise.
   * @param {!unpacker.types.FileSystemId} fileSystemId
   * @param {!unpacker.types.RequestId} readRequestid The read request id.
   * @param {!unpacker.types.RequestId} openRequestId The open request id.
   * @param {number} offset The offset from where read should be done.
   * @param {number} length The number of bytes to read.
   * @return {!Promise} A read file request promise. It fulfills with an
   *     Int8Array buffer containing the requested data or rejects with
   *     ProviderError.
   */
  createReadFilePromise: function(
      fileSystemId, readRequestId, openRequestId, offset, length) {
    var options = {
      fileSystemId: fileSystemId,
      requestId: readRequestId,
      openRequestId: openRequestId,
      offset: offset,
      length: length
    };

    var result = new Int8Array(length);
    var resultOffset = 0;
    return new Promise(function(fulfill, reject) {
      unpacker.app.onReadFileRequested(options, function(arrayBuffer, hasMore) {
        result.set(new Int8Array(arrayBuffer), resultOffset);
        resultOffset += arrayBuffer.byteLength;

        if (hasMore)  // onSuccess will be called again.
          return;

        // Received less data than requested so truncate result buffer.
        if (resultOffset < length)
          result = result.subarray(0, resultOffset);

        fulfill(result);
      }, reject /* In case of errors just reject with ProviderError. */);
    });
  },
};
