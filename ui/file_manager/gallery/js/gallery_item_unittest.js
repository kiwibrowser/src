// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Mock of ImageUtil and metrics.
 */
var ImageUtil = {
  getMetricName: function() {},
};
var metrics = {
  recordEnum: function() {},
  recordInterval: function() {},
  startInterval: function() {}
};

/**
 * Mock of ImageEncoder. Since some test changes the behavior of ImageEncoder,
 * this is initialized in setUp().
 */
var ImageEncoder;

/**
 * Load time data.
 */
loadTimeData.data = {
  DRIVE_DIRECTORY_LABEL: '',
  DOWNLOADS_DIRECTORY_LABEL: ''
};

function setUp() {
  ImageEncoder = {
    encodeMetadata: function() {},
    getBlob: function() {}
  };
}

/**
 * Returns a mock of metadata model.
 * @private
 * @return {!MetadataModel}
 */
function getMockMetadataModel() {
  return {
    get: function(entries, names) {
      return Promise.resolve([
        {size: 200}
      ]);
    },
    notifyEntriesChanged: function() {
    }
  };
}

/**
 * Tests for GalleryItem#saveToFile.
 */
function testSaveToFile(callback) {
  var fileSystem = new MockFileSystem('volumeId');
  fileSystem.populate(['/test.jpg']);
  var entry = fileSystem.entries['/test.jpg'];
  entry.createWriter = function(callback) {
    callback({
      write: function() {
        Promise.resolve().then(function() {
          this.onwriteend();
        }.bind(this));
      },
      truncate: function() {
        this.write();
      }
    });
  };
  var entryChanged = false;
  var metadataModel = getMockMetadataModel();
  metadataModel.notifyEntriesChanged = function() {
    entryChanged = true;
  };

  var item = new GalleryItem(
      entry,
      {isReadOnly: false},
      {size: 100},
      {},
      /* original */ true);
  assertEquals(100, item.getMetadataItem().size);
  assertFalse(entryChanged);
  reportPromise(
      new Promise(item.saveToFile.bind(
          item,
          {
            getLocationInfo: function() { return {}; },
            getVolumeInfo: function() { return {}; }
          },
          metadataModel,
          /* fallbackDir */ null,
          document.createElement('canvas'),
          true /* overwrite */)).then(function() {
            assertEquals(200, item.getMetadataItem().size);
            assertTrue(entryChanged);
          }), callback);
}

/**
 * Tests for GalleryItem#saveToFile. In this test case, fileWriter.write fails
 * with an error.
 */
function testSaveToFileWriteFailCase(callback) {
  var fileSystem = new MockFileSystem('volumeId');
  fileSystem.populate(['/test.jpg']);
  var entry = fileSystem.entries['/test.jpg'];

  entry.createWriter = function(callback) {
    callback({
      write: function() {
        Promise.resolve().then(function() {
          this.onerror(new Error());
        }.bind(this));
      },
      truncate: function() {
        Promise.resolve().then(function() {
          this.onwriteend();
        }.bind(this));
      }
    });
  };

  var item = new GalleryItem(
      entry,
      {isReadOnly: false},
      {size: 100},
      {},
      /* original */ true);
  reportPromise(
      new Promise(item.saveToFile.bind(
          item,
          {
            getLocationInfo: function() { return {}; },
            getVolumeInfo: function() { return {}; }
          },
          getMockMetadataModel(),
          /* fallbackDir */ null,
          document.createElement('canvas'),
          true /* overwrite */)).then(function(result) {
            assertFalse(result);
          }), callback);
}

/**
 * Tests for GalleryItem#saveToFile. In this test case, ImageEncoder.getBlob
 * fails with an error. This test case confirms that no write operation runs
 * when it fails to get a blob of new image.
 */
function testSaveToFileGetBlobFailCase(callback) {
  ImageEncoder.getBlob = function() {
    throw new Error();
  };

  var fileSystem = new MockFileSystem('volumeId');
  fileSystem.populate(['/test.jpg']);
  var entry = fileSystem.entries['/test.jpg'];

  var writeOperationRun = false;
  entry.createWriter = function(callback) {
    callback({
      write: function() {
        Promise.resolve().then(function() {
          writeOperationRun = true;
          this.onwriteend();
        }.bind(this));
      },
      truncate: function() {
        Promise.resolve().then(function() {
          writeOperationRun = true;
          this.onwriteend();
        }.bind(this));
      }
    });
  };

  var item = new GalleryItem(
      entry,
      {isReadOnly: false},
      {size: 100},
      {},
      /* original */ true);
  reportPromise(
      new Promise(item.saveToFile.bind(
          item,
          {
            getLocationInfo: function() { return {}; },
            getVolumeInfo: function() { return {}; }
          },
          getMockMetadataModel(),
          /* fallbackDir */ null,
          document.createElement('canvas'),
          true /* overwrite*/)).then(function(result) {
            assertFalse(result);
            assertFalse(writeOperationRun);
          }), callback);
}

function testSaveToFileRaw(callback) {
  var fileSystem = new MockFileSystem('volumeId');
  fileSystem.populate(['/test.arw']);
  fileSystem.entries['/'].getFile = function(name, options, success, error) {
    if (options.create) {
      assertEquals('test - Edited.jpg', name);
      fileSystem.populate(['/test - Edited.jpg']);
      var entry = fileSystem.entries['/test - Edited.jpg'];
      entry.createWriter = function(callback) {
        callback({
          write: function() {
            Promise.resolve().then(function() {
              this.onwriteend();
            }.bind(this));
          },
          truncate: function() {
            this.write();
          }
        });
      };
    }
    MockDirectoryEntry.prototype.getFile.apply(this, arguments);
  };
  var entryChanged = false;
  var metadataModel = getMockMetadataModel();
  metadataModel.notifyEntriesChanged = function() {
    entryChanged = true;
  };

  var item = new GalleryItem(
      fileSystem.entries['/test.arw'],
      {isReadOnly: false},
      {size: 100},
      {},
      /* original */ true);
  assertEquals(100, item.getMetadataItem().size);
  assertFalse(entryChanged);
  reportPromise(
      new Promise(item.saveToFile.bind(
          item,
          {
            getLocationInfo: function() { return {}; },
            getVolumeInfo: function() { return {}; }
          },
          metadataModel,
          /* fallbackDir */ null,
          document.createElement('canvas'),
          false /* not overwrite */)).then(function(success) {
            assertTrue(success);
            assertEquals(200, item.getMetadataItem().size);
            assertTrue(entryChanged);
            assertFalse(item.isOriginal());
          }), callback);
}

function testIsWritableFile() {
  var downloads = new MockFileSystem('downloads');
  var removable = new MockFileSystem('removable');
  var mtp = new MockFileSystem('mtp');

  var volumeTypes = {
    downloads: VolumeManagerCommon.VolumeType.DOWNLOADS,
    removable: VolumeManagerCommon.VolumeType.REMOVABLE,
    mtp: VolumeManagerCommon.VolumeType.MTP
  };

  // Mock volume manager.
  var volumeManager = {
    getVolumeInfo: function(entry) {
      return {
        volumeType: volumeTypes[entry.filesystem.name]
      };
    }
  };

  var getGalleryItem = function(path, fileSystem, isReadOnly) {
    return new GalleryItem(new MockEntry(fileSystem, path),
        {isReadOnly: isReadOnly},
        {size: 100},
        {},
        true /* original */);
  };

  // Jpeg file on downloads.
  assertTrue(getGalleryItem(
      '/test.jpg', downloads, false /* not read only */).
      isWritableFile(volumeManager));

  // Png file on downloads.
  assertTrue(getGalleryItem(
      '/test.png', downloads, false /* not read only */).
      isWritableFile(volumeManager));

  // Webp file on downloads.
  assertFalse(getGalleryItem(
      '/test.webp', downloads, false /* not read only */).
      isWritableFile(volumeManager));

  // Jpeg file on non-writable volume.
  assertFalse(getGalleryItem(
      '/test.jpg', removable, true /* read only */).
      isWritableFile(volumeManager));

  // Jpeg file on mtp volume.
  assertFalse(getGalleryItem(
      '/test.jpg', mtp, false /* not read only */).
      isWritableFile(volumeManager));
};
