// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var kHundredChars = '';
var kTestFileName = 'test.txt';

for (var i = 0; i < 10; i++) {
  kHundredChars += '0123456789';
}

function FileSystemWriteError() {
  chrome.test.fail('Filesystem write error');
}

// Appends 50 bytes of data to the file.
function AppendDataToFile(fileEntry, numChars) {
  fileEntry.createWriter(function(fileWriter) {

    fileWriter.onwriteend = function(e) {
      chrome.test.sendMessage('write_complete', null);
      chrome.test.succeed();
      chrome.app.window.current().close();
    };

    fileWriter.onerror = function(e) {
      FileSystemWriteError();
    };

    fileWriter.seek(fileWriter.length);
    var str = '';

    // Avoid many loops for large data packets.
    var iterations = Math.floor(numChars / 100);
    for (var i = 0; i < iterations; ++i) {
      str += kHundredChars;
    }

    iterations = numChars % 100;
    for (var i = 0; i < iterations; ++i) {
      str += 'a';
    }

    var blob = new Blob([str], {type: 'text/plain'});
    fileWriter.write(blob);
  });
}

function WriteData(numChars) {
  window.webkitRequestFileSystem(
      PERSISTENT,
      16384,
      function(fs) {
        fs.root.getFile(
            kTestFileName,
            {create: true},
            function(fileEntry) {
              AppendDataToFile(fileEntry, numChars);
            },
            FileSystemWriteError);
      }, FileSystemWriteError);
}

onload = function() {
  chrome.test.sendMessage('launched', function(reply) {
    var numChars = parseInt(reply);
    if (isNaN(numChars)) {
      chrome.test.fail('Expected number of chars from browser');
      return;
    }

    WriteData(numChars);
  });
};
