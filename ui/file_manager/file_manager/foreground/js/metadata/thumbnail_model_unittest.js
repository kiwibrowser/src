// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var imageEntry = {
  name: 'image.jpg',
  toURL: function() { return 'filesystem://A'; }
};

var nonImageEntry = {
  name: 'note.txt',
  toURL: function() { return 'filesystem://B'; }
};

var metadata;
var contentMetadata;
var thumbnailModel;

function setUp() {
  metadata = new MetadataItem();
  metadata.modificationTime = new Date(2015, 0, 1);
  metadata.present = true;
  metadata.thumbnailUrl = 'EXTERNAL_THUMBNAIL_URL';
  metadata.customIconUrl = 'CUSTOM_ICON_URL';
  metadata.contentThumbnailUrl = 'CONTENT_THUMBNAIL_URL';
  metadata.contentThumbnailTransform = 'CONTENT_THUMBNAIL_TRANSFORM';
  metadata.contentImageTransform = 'CONTENT_IMAGE_TRANSFORM';

  thumbnailModel = new ThumbnailModel({
      get: function(entries, names) {
        var result = new MetadataItem();
        for (var i = 0; i < names.length; i++) {
          var name = names[i];
          result[name] = metadata[name];
        }
        return Promise.resolve([result]);
      }});
}

function testThumbnailModelGetBasic(callback) {
  reportPromise(thumbnailModel.get([imageEntry]).then(function(results) {
    assertEquals(1, results.length);
    assertEquals(
        new Date(2015, 0, 1).toString(),
        results[0].filesystem.modificationTime.toString());
    assertEquals('EXTERNAL_THUMBNAIL_URL', results[0].external.thumbnailUrl);
    assertEquals('CUSTOM_ICON_URL', results[0].external.customIconUrl);
    assertTrue(results[0].external.present);
    assertEquals('CONTENT_THUMBNAIL_URL', results[0].thumbnail.url);
    assertEquals('CONTENT_THUMBNAIL_TRANSFORM', results[0].thumbnail.transform);
    assertEquals('CONTENT_IMAGE_TRANSFORM', results[0].media.imageTransform);
  }), callback);
}

function testThumbnailModelGetNotPresent(callback) {
  metadata.present = false;
  reportPromise(thumbnailModel.get([imageEntry]).then(function(results) {
    assertEquals(1, results.length);
    assertEquals(
        new Date(2015, 0, 1).toString(),
        results[0].filesystem.modificationTime.toString());
    assertEquals('EXTERNAL_THUMBNAIL_URL', results[0].external.thumbnailUrl);
    assertEquals('CUSTOM_ICON_URL', results[0].external.customIconUrl);
    assertFalse(results[0].external.present);
    assertEquals(undefined, results[0].thumbnail.url);
    assertEquals(undefined, results[0].thumbnail.transform);
    assertEquals(undefined, results[0].media.imageTransform);
  }), callback);
}

function testThumbnailModelGetNonImage(callback) {
  reportPromise(thumbnailModel.get([nonImageEntry]).then(function(results) {
    assertEquals(1, results.length);
    assertEquals(
        new Date(2015, 0, 1).toString(),
        results[0].filesystem.modificationTime.toString());
    assertEquals('EXTERNAL_THUMBNAIL_URL', results[0].external.thumbnailUrl);
    assertEquals('CUSTOM_ICON_URL', results[0].external.customIconUrl);
    assertTrue(results[0].external.present);
    assertEquals(undefined, results[0].thumbnail.url);
    assertEquals(undefined, results[0].thumbnail.transform);
    assertEquals(undefined, results[0].media.imageTransform);
  }), callback);
}
