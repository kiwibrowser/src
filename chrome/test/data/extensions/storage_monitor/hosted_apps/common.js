// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Opens the filesystem and returns a Promise that resolves to the opened
// filesystem.
function GetFileSystem(type, size) {
  return new Promise(
      (resolve, reject) =>
          webkitRequestFileSystem(type, size, resolve, reject));
}

// Returns a .then()-chainable handler that accepts a fileystem, creates a file,
// and returns a Promise that resolves to the successfully created file.
function CreateFile(filename) {
  return (filesystem) => new Promise(
             (resolve, reject) => filesystem.root.getFile(
                 filename, {create: true}, resolve, reject));
}

// Returns a .then()-chainable handler that accepts a filesystem file, appends
// |numChars| to it, and returns a Promise that resolves to the file once the
// append operation is successful.
function AppendDataToFile(numChars) {
  return ((fileEntry) => {
    return new Promise((resolve, reject) => {
      fileEntry.createWriter((fileWriter) => {
        // FileWriter's onwriteend resolves the promise; onerror rejects it.
        fileWriter.onwriteend = (e) => resolve(fileEntry);
        fileWriter.onerror = reject;
        fileWriter.seek(fileWriter.length);

        var str = 'a'.repeat(numChars);

        var blob = new Blob([str], {type: 'text/plain'});
        console.assert(blob.size == numChars);
        fileWriter.write(blob);
      }, reject);
    });
  });
}

// Entry point to be called via ExecuteScript in the browser test C++ code.
//
// Asynchronously opens writes |numChars| to the filesystem. Returns a Promise
// that resolves upon completion.
function HostedAppWriteData(type, numChars) {
  return GetFileSystem(type, 16384)
      .then(CreateFile('test.txt'))
      .then(AppendDataToFile(numChars));
}
