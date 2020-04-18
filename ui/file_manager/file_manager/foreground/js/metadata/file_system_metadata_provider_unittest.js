// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var entryA = {
  toURL: function() { return "filesystem://A"; },
  getMetadata: function(fulfill, reject) {
    Promise.resolve({modificationTime: new Date(2015, 1, 1), size: 1024}).then(
        fulfill, reject);
  }
};

var entryB = {
  toURL: function() { return "filesystem://B"; },
  getMetadata: function(fulfill, reject) {
    Promise.resolve({modificationTime: new Date(2015, 2, 2), size: 2048}).then(
        fulfill, reject);
  }
};

function testFileSystemMetadataProviderBasic(callback) {
  var provider = new FileSystemMetadataProvider();
  var names = [
    'modificationTime', 'size', 'contentMimeType', 'present',
    'availableOffline'];
  reportPromise(provider.get([
    new MetadataRequest(entryA, names),
    new MetadataRequest(entryB, names)
  ]).then(function(results) {
    assertEquals(2, results.length);
    assertEquals(
        new Date(2015, 1, 1).toString(),
        results[0].modificationTime.toString());
    assertEquals(1024, results[0].size);
    assertTrue(results[0].present);
    assertTrue(results[0].availableOffline);
    assertEquals(
        new Date(2015, 2, 2).toString(),
        results[1].modificationTime.toString());
    assertEquals(2048, results[1].size);
    assertTrue(results[1].present);
    assertTrue(results[1].availableOffline);
  }), callback);
}

function testFileSystemMetadataProviderPartialRequest(callback) {
  var provider = new FileSystemMetadataProvider();
  reportPromise(provider.get(
      [new MetadataRequest(entryA, ['modificationTime', 'size'])]).then(
      function(results) {
        assertEquals(1, results.length);
        assertEquals(
            new Date(2015, 1, 1).toString(),
            results[0].modificationTime.toString());
        assertEquals(1024, results[0].size);
      }), callback);
}
