// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Test case:
 * - Source image: 200x50
 * - Target: max size is 100x100
 */
function testNormalImage() {
  var source = new Image();
  source.width = 200;
  source.height = 50;
  var options = {
    maxWidth: 100,
    maxHeight: 100,
    orientation: ImageOrientation.fromDriveOrientation(0)
  };
  var result = ImageLoaderUtil.calculateCopyParameters(source, options);
  assertEquals(0, result.source.x);
  assertEquals(0, result.source.y);
  assertEquals(200, result.source.width);
  assertEquals(50, result.source.height);
  assertEquals(0, result.target.x);
  assertEquals(0, result.target.y);
  assertEquals(100, result.target.width);
  assertEquals(25, result.target.height);
  assertEquals(100, result.canvas.width);
  assertEquals(25, result.canvas.height);
};

/**
 * Test case:
 * - Source image: 50x200 90 deg clock-wise rotated image.
 * - Target: max size is 100x100
 */
function testRotatedImage() {
  var source = new Image();
  source.width = 50;
  source.height = 200;
  var options = {
    maxWidth: 100,
    maxHeight: 100,
    orientation: ImageOrientation.fromDriveOrientation(1)
  };
  var result = ImageLoaderUtil.calculateCopyParameters(source, options);
  assertEquals(0, result.source.x);
  assertEquals(0, result.source.y);
  assertEquals(50, result.source.width);
  assertEquals(200, result.source.height);
  assertEquals(0, result.target.x);
  assertEquals(0, result.target.y);
  assertEquals(25, result.target.width);
  assertEquals(100, result.target.height);
  assertEquals(100, result.canvas.width);
  assertEquals(25, result.canvas.height);
}

/**
 * Test case:
 * - Source image: 800x100
 * - Target: 50x50 cropped image.
 */
function testCroppedImage() {
  var source = new Image();
  source.width = 800;
  source.height = 100;
  var options = {
    width: 50,
    height: 50,
    crop: true,
    orientation: ImageOrientation.fromDriveOrientation(0)
  };
  var result = ImageLoaderUtil.calculateCopyParameters(source, options);
  assertEquals(350, result.source.x);
  assertEquals(0, result.source.y);
  assertEquals(100, result.source.width);
  assertEquals(100, result.source.height);
  assertEquals(0, result.target.x);
  assertEquals(0, result.target.y);
  assertEquals(50, result.target.width);
  assertEquals(50, result.target.height);
  assertEquals(50, result.canvas.width);
  assertEquals(50, result.canvas.height);
}

/**
 * Test case:
 * - Source image: 200x25
 * - Target: 50x50 cropped image.
 */
function testCroppedImageWithResize() {
  var source = new Image();
  source.width = 200;
  source.height = 25;
  var options = {
    width: 50,
    height: 50,
    crop: true,
    orientation: ImageOrientation.fromDriveOrientation(0)
  };
  var result = ImageLoaderUtil.calculateCopyParameters(source, options);
  assertEquals(87, result.source.x);
  assertEquals(0, result.source.y);
  assertEquals(25, result.source.width);
  assertEquals(25, result.source.height);
  assertEquals(0, result.target.x);
  assertEquals(0, result.target.y);
  assertEquals(50, result.target.width);
  assertEquals(50, result.target.height);
  assertEquals(50, result.canvas.width);
  assertEquals(50, result.canvas.height);
}

/**
 * Test case:
 * - Source image: 20x10
 * - Target: 50x50 cropped image.
 */
function testCroppedTinyImage() {
  var source = new Image();
  source.width = 20;
  source.height = 10;
  var options = {
    width: 50,
    height: 50,
    crop: true,
    orientation: ImageOrientation.fromDriveOrientation(0)
  };
  var result = ImageLoaderUtil.calculateCopyParameters(source, options);
  assertEquals(5, result.source.x);
  assertEquals(0, result.source.y);
  assertEquals(10, result.source.width);
  assertEquals(10, result.source.height);
  assertEquals(0, result.target.x);
  assertEquals(0, result.target.y);
  assertEquals(50, result.target.width);
  assertEquals(50, result.target.height);
  assertEquals(50, result.canvas.width);
  assertEquals(50, result.canvas.height);
}

/**
 * Test case:
 * - Source image: 100x400 90 degree clock-wise rotated.
 * - Target: 50x50 cropped image
 */
function testCroppedRotatedImage() {
  var source = new Image();
  source.width = 100;
  source.height = 400;
  var options = {
    width: 50,
    height: 50,
    crop: true,
    orientation: ImageOrientation.fromDriveOrientation(1)
  };
  var result = ImageLoaderUtil.calculateCopyParameters(source, options);
  assertEquals(0, result.source.x);
  assertEquals(150, result.source.y);
  assertEquals(100, result.source.width);
  assertEquals(100, result.source.height);
  assertEquals(0, result.target.x);
  assertEquals(0, result.target.y);
  assertEquals(50, result.target.width);
  assertEquals(50, result.target.height);
  assertEquals(50, result.canvas.width);
  assertEquals(50, result.canvas.height);
}
