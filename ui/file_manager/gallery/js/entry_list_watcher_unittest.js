// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var chrome;
var mockFileSystem;

function webkitResolveLocalFileSystemURL(url, callback) {
  var paths = Object.keys(mockFileSystem.entries);
  for (var i = 0; i < paths.length; i++) {
    var entry = mockFileSystem.entries[paths[i]];
    if (url === entry.toURL()) {
      delete chrome.runtime['lastError'];
      callback(entry);
      return;
    }
  }
  chrome.runtime.lastError = {
    name: 'Not found.'
  };
  callback(null);
};

function setUp() {
  chrome = {
    fileManagerPrivate: {
      onDirectoryChanged: new MockAPIEvent(),
      addFileWatch: function(entry, callback) {
        this.watchedURLs[entry.toURL()] = true;
        callback();
      },
      removeFileWatch: function(entry, callback) {
        delete this.watchedURLs[entry.toURL()];
        callback();
      },
      watchedURLs: {}
    },
    runtime: {
      // For lastError.
    }
  };

  mockFileSystem = new MockFileSystem('volumeId', 'filesystem://rootURL');
  mockFileSystem.entries['/'] = new MockDirectoryEntry(mockFileSystem, '/');
  mockFileSystem.entries['/A.txt'] =
      new MockFileEntry(mockFileSystem, '/A.txt', {});
  mockFileSystem.entries['/B.txt'] =
      new MockFileEntry(mockFileSystem, '/B.txt', {});
  mockFileSystem.entries['/C/'] = new MockDirectoryEntry(mockFileSystem, '/C/');
  mockFileSystem.entries['/C/D.txt'] =
      new MockFileEntry(mockFileSystem, '/C/D.txt', {});
}

function testAddWatcher() {
  var list = new cr.ui.ArrayDataModel([
    mockFileSystem.entries['/A.txt']
  ]);
  var watcher = new EntryListWatcher(list);
  assertArrayEquals(
      ['filesystem://rootURL/'],
      Object.keys(chrome.fileManagerPrivate.watchedURLs));
  list.push(mockFileSystem.entries['/C/D.txt']);
  assertArrayEquals(
      ['filesystem://rootURL/', 'filesystem://rootURL/C/'],
      Object.keys(chrome.fileManagerPrivate.watchedURLs).sort());
}

function testRemoveWatcher() {
  var list = new cr.ui.ArrayDataModel([
    mockFileSystem.entries['/A.txt'],
    mockFileSystem.entries['/C/D.txt']
  ]);
  var watcher = new EntryListWatcher(list);
  assertArrayEquals(
      ['filesystem://rootURL/', 'filesystem://rootURL/C/'],
      Object.keys(chrome.fileManagerPrivate.watchedURLs).sort());
  list.splice(1, 1);
  assertArrayEquals(
      ['filesystem://rootURL/'],
      Object.keys(chrome.fileManagerPrivate.watchedURLs));

}

function testEntryRemoved(callback) {
  var list = new cr.ui.ArrayDataModel([
    mockFileSystem.entries['/A.txt'],
    mockFileSystem.entries['/B.txt']
  ]);

  var watcher = new EntryListWatcher(list);
  var splicedPromise = new Promise(function(fulfill) {
    list.addEventListener('splice', fulfill);
  });

  var deletedB = mockFileSystem.entries['/B.txt'];
  delete mockFileSystem.entries['/B.txt'];
  assertArrayEquals(
      ['filesystem://rootURL/'],
      Object.keys(chrome.fileManagerPrivate.watchedURLs));
  chrome.fileManagerPrivate.onDirectoryChanged.dispatch(
      {entry: mockFileSystem.entries['/']});

  reportPromise(splicedPromise.then(function(event) {
    assertEquals(1, event.removed.length);
    assertEquals(deletedB, event.removed[0]);
  }), callback);
}
