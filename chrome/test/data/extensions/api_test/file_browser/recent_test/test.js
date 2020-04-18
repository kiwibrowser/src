// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Test files should be created before running the test extension.
 */

function getVolumeMetadataList() {
  return new Promise(function(resolve, reject) {
    chrome.fileManagerPrivate.getVolumeMetadataList(resolve);
  });
}

function requestFileSystem(volumeId) {
  return new Promise(function(resolve, reject) {
    chrome.fileSystem.requestFileSystem(
        {volumeId: volumeId},
        function(fileSystem) {
          if (!fileSystem) {
            reject(new Error('Failed to acquire volume.'));
          }
          resolve(fileSystem);
        });
  });
}

function requestAllFileSystems() {
  return getVolumeMetadataList().then(function(volumes) {
    return Promise.all(volumes.map(function(volume) {
      return requestFileSystem(volume.volumeId);
    }));
  });
}

// Run the tests.
requestAllFileSystems().then(function() {
  chrome.test.runTests([
    function testGetRecentFiles() {
      chrome.fileManagerPrivate.getRecentFiles(
          'native_source', chrome.test.callbackPass(function(entries) {
            var found = false;
            for (var i = 0; i < entries.length; ++i) {
              if (entries[i].name === 'all-justice.jpg') {
                found = true;
              }
            }
            chrome.test.assertTrue(found, 'all-justice.jpg not found');
          }));
    }
  ]);
});
