// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var model;
var fileSystem;
var item;

/**
 * Mock thumbnail model.
 */
function ThumbnailModel() {
}

ThumbnailModel.prototype.get = function(entries) {
  return Promise.resolve(entries.map(function() {
    return {};
  }));
};

function setUp() {
  model = new GalleryDataModel(
      /* Mock MetadataModel */{
        get: function() {
          return Promise.resolve([{}]);
        }
      },
      /* Mock EntryListWatcher */{});
  fileSystem = new MockFileSystem('volumeId');
  model.fallbackSaveDirectory = fileSystem.root;
}

function testSaveItemOverwrite(callback) {
  var item = new GalleryItem(
      new MockEntry(fileSystem, '/test.jpg'),
      null,
      /* metadataItem */ {},
      /* thumbnailMetadataItem */ {},
      /* original */ true);

  // Mocking the saveToFile method.
  item.saveToFile = function(
      volumeManager,
      metadataModel,
      fallbackDir,
      canvas,
      overwrite,
      callback) {
    callback(true);
  };
  model.push(item);
  reportPromise(
      model.saveItem({}, item, document.createElement('canvas'),
          true /* overwrite */).
          then(function() { assertEquals(1, model.length); }),
      callback);
}

function testSaveItemToNewFile(callback) {
  var item = new GalleryItem(
      new MockEntry(fileSystem, '/test.webp'),
      null,
      /* metadataItem */ {},
      /* thumbnailMetadataItem */ {},
      /* original */ true);

  // Mocking the saveToFile method. In this case, Gallery saves to a new file
  // since it cannot overwrite to webp image file.
  item.saveToFile = function(
      volumeManager,
      metadataModel,
      fallbackDir,
      canvas,
      overwrite,
      callback) {
    // Gallery item track new file.
    this.entry_ = new MockEntry(fileSystem, '/test (1).png');
    this.original_ = false;
    callback(true);
  };
  model.push(item);
  reportPromise(
      model.saveItem({}, item, document.createElement('canvas'),
          false /* not overwrite */).
          then(function() {
            assertEquals(2, model.length);
            assertEquals('test (1).png', model.item(0).getFileName());
            assertFalse(model.item(0).isOriginal());
            assertEquals('test.webp', model.item(1).getFileName());
            assertTrue(model.item(1).isOriginal());
          }),
      callback);
}
