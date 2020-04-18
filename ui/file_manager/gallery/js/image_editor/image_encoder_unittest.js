// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
'use strict';

/**
 * Encodes an image to data URL by using ImageEncoder.getBlob.
 */
function encodeAnImageAsDataURL(canvas, metadata, imageQuality) {
  return new Promise(function(resolve, reject) {
    // Thumbnail quality is fixed to 1.0 in this test.
    var encoder = ImageEncoder.encodeMetadata(metadata, canvas, 1.0);
    var blob = ImageEncoder.getBlob(canvas, encoder, imageQuality);

    var fileReader = new FileReader();
    fileReader.onerror = function() {
      reject(fileReader.error);
    };
    fileReader.onloadend = function() {
      resolve(fileReader.result);
    };
    fileReader.readAsDataURL(blob);
  });
}

/**
 * Test case for png image.
 * Png image should be saved as a png image.
 */
function testPngImage(callback) {
  var canvas = getSampleCanvas();

  var metadata = {
    mediaMimeType: 'image/png'
  };

  reportPromise(encodeAnImageAsDataURL(canvas, metadata, 0.9).then(
      function(result) {
        assertEquals(canvas.toDataURL('image/png'), result);
      }), callback);
}

/**
 * Test case for jpeg image.
 * Jpeg image should be saved as a jpeg image. Since we don't include
 * exif_encoder.js in this test, no metadata is added to the blob.
 */
function testJpegImage(callback) {
  var canvas = getSampleCanvas();

  var metadata = {
    mediaMimeType: 'image/jpeg'
  };

  reportPromise(encodeAnImageAsDataURL(canvas, metadata, 0.9).then(
      function(result) {
        assertEquals(canvas.toDataURL('image/jpeg', 0.9), result);
      }), callback);
}


/**
 * Test case of webp image.
 * Image should be saved as a image/png since chrome doesn't support to
 * encode other than image/jpeg or image/png.
 */
function testWebpImage(callback) {
  var canvas = getSampleCanvas();

  var metadata = {
    mediaMimeType: 'image/webp'
  };

  reportPromise(encodeAnImageAsDataURL(canvas, metadata, 0.9).then(
      function(result) {
        assertEquals(canvas.toDataURL('image/png'), result);
      }), callback);
}

/**
 * Test case for broken metadata.
 */
function testWithBrokenMetadata() {
  var canvas = getSampleCanvas();

  var metadata = {
    // No mimetype field.
  };

  // An exception should be thrown if metadata is broken.
  assertThrows(function() {
    var encoder = ImageEncoder.encodeMetadata(metadata, canvas);
  });
}
