// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var fileSystem;

function setUp() {
  fileSystem = new MockFileSystem('fake-media-volume');
  var filenames = [
    '/DCIM/photos0/IMG00001.jpg',
    '/DCIM/photos0/IMG00002.jpg',
    '/DCIM/photos0/IMG00003.jpg',
    '/DCIM/photos0/MyNote.txt',
    '/DCIM/photos0/.HiddenFile.jpg',
    '/DCIM/photos1/IMG00001.jpg',
    '/DCIM/photos1/IMG00002.jpg',
    '/DCIM/photos1/IMG00003.jpg'
  ];
  fileSystem.populate(filenames);
}

function testcreateEntrySet(callback) {
  var emptySelectionPromise = GalleryUtil.createEntrySet([
  ]).then(function(results) {
    assertEquals(0, results.length);
  });

  var singleSelectionPromise = GalleryUtil.createEntrySet([
    fileSystem.entries['/DCIM/photos0/IMG00002.jpg']
  ]).then(function(results) {
    assertEquals(3, results.length);
    assertEquals('/DCIM/photos0/IMG00001.jpg', results[0].fullPath);
    assertEquals('/DCIM/photos0/IMG00002.jpg', results[1].fullPath);
    assertEquals('/DCIM/photos0/IMG00003.jpg', results[2].fullPath);
  });

  // If a hidden file is selected directly by users, includes it.
  var singleHiddenSelectionPromise = GalleryUtil.createEntrySet([
    fileSystem.entries['/DCIM/photos0/.HiddenFile.jpg']
  ]).then(function(results) {
    assertEquals(4, results.length);
    assertEquals('/DCIM/photos0/.HiddenFile.jpg', results[0].fullPath);
    assertEquals('/DCIM/photos0/IMG00001.jpg', results[1].fullPath);
    assertEquals('/DCIM/photos0/IMG00002.jpg', results[2].fullPath);
    assertEquals('/DCIM/photos0/IMG00003.jpg', results[3].fullPath);
  });

  var multipleSelectionPromise = GalleryUtil.createEntrySet([
    fileSystem.entries['/DCIM/photos0/IMG00001.jpg'],
    fileSystem.entries['/DCIM/photos0/IMG00002.jpg'],
  ]).then(function(results) {
    assertEquals(2, results.length);
    assertEquals('/DCIM/photos0/IMG00001.jpg', results[0].fullPath);
    assertEquals('/DCIM/photos0/IMG00002.jpg', results[1].fullPath);
  });

  var multipleSelectionReverseOrderPromise = GalleryUtil.createEntrySet([
    fileSystem.entries['/DCIM/photos0/IMG00002.jpg'],
    fileSystem.entries['/DCIM/photos0/IMG00001.jpg'],
  ]).then(function(results) {
    assertEquals(2, results.length);
    assertEquals('/DCIM/photos0/IMG00001.jpg', results[0].fullPath);
    assertEquals('/DCIM/photos0/IMG00002.jpg', results[1].fullPath);
  });

  var multipleHiddenSelectionPromise = GalleryUtil.createEntrySet([
    fileSystem.entries['/DCIM/photos0/IMG00001.jpg'],
    fileSystem.entries['/DCIM/photos0/.HiddenFile.jpg']
  ]).then(function(results) {
    assertEquals(2, results.length);
    assertEquals('/DCIM/photos0/.HiddenFile.jpg', results[0].fullPath);
    assertEquals('/DCIM/photos0/IMG00001.jpg', results[1].fullPath);
  });

  reportPromise(Promise.all([
    emptySelectionPromise,
    singleSelectionPromise,
    singleHiddenSelectionPromise,
    multipleSelectionPromise,
    multipleSelectionReverseOrderPromise,
    multipleHiddenSelectionPromise
  ]), callback);
}
