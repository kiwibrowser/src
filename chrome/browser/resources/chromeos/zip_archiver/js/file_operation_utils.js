// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * Utilities for file operations.
 */
var fileOperationUtils = {};

/**
 * Deduplicates file name inside |rootDirectory| directory.
 * @param {string} fileName The suggested file name.
 * @param {!DirectoryEntry} rootDirectory The root directory where new file with
 *     |fileName| will be created.
 * @return {Promise<string>}
 */
fileOperationUtils.deduplicateFileName = function(fileName, rootDirectory) {
  // Split the name into two parts. The file name/prefix and extension.
  var match = /^(.*?)?(\.[^.]*?)?$/.exec(fileName);
  var prefix = match[1];
  var ext = match[2] || '';

  var getFileName = function(filePrefix, fileNumber, fileExtension) {
    var newName = filePrefix;
    if (fileNumber > 0) {
      newName += ' (' + fileNumber + ')';
    }
    newName += fileExtension;

    return new Promise(rootDirectory.getFile.bind(
                           rootDirectory, newName, {create: false}))
        .then(function() {
          return getFileName(filePrefix, fileNumber + 1, fileExtension);
        })
        .catch(function(error) {
          if (error.code == error.NOT_FOUND_ERR) {
            return Promise.resolve(newName);
          } else {
            return Promise.reject(error);
          }
        });
  };

  return getFileName(prefix, 0, ext);
};