// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * This variable is checked in SelectFileDialogExtensionBrowserTest.
 * @type {number}
 */
window.JSErrorCount = 0;

/**
 * Counts uncaught exceptions.
 */
window.onerror = function() { window.JSErrorCount++; };

/**
 * Opens the gallery window and waits until it is ready.
 *
 * @param {Array<string>} urls URLs to be opened.
 * @param {function(string)} callback Completion callback with the new window's
 *     App ID.
 */
test.util.async.openGallery = function(urls, callback) {
  openGalleryWindow(urls, false).then(callback);
};

/**
 * Gets the metadata of the entry.
 *
 * @param {string} url URL to be get the metadata.
 * @param {function(string)} callback Completion callback with the metadata.
 */
test.util.async.getMetadata = function(url, callback) {
  util.URLsToEntries([url]).then(function(result) {
    if (result.entries.length != 1) {
      callback(null);
    } else {
      result.entries[0].getMetadata(function(metadata) {
        callback({
          modificationTime: metadata.modificationTime,
          size: metadata.size
        });
      });
    }
  });
};

/**
 * Changes the value of the input element.
 *
 * @param {Window} contentWindow Window to be tested.
 * @param {string} query Query for the input element.
 * @param {string} newValue Value to be assigned.
 */
test.util.sync.changeValue = function(contentWindow, query, newValue) {
  var element = contentWindow.document.querySelector(query);
  element.value = newValue;
  chrome.test.assertTrue(element.dispatchEvent(new Event('change')));
};

/**
 * Changes the file name.
 *
 * @param {Window} contentWindow Window to be tested.
 * @param {string} newName Name to be newly assigned.
 */
test.util.sync.changeName = function(contentWindow, newName) {
  var nameBox = contentWindow.document.querySelector('.filename-spacer input');
  nameBox.focus();
  nameBox.value = newName;
  nameBox.blur();
};

// Register the test utils.
test.util.registerRemoteTestUtils();
