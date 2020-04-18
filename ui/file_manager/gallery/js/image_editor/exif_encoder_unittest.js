// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * Test case for ordinal exif encoding and decoding.
 */
function testExifEncodeAndDecode() {
  var canvas = getSampleCanvas();
  var data = canvas.toDataURL('image/jpeg');

  var metadata = {
    mediaMimeType: 'image/jpeg',
    modificationTime: new Date(2015, 0, 7, 15, 30, 6),
    ifd: {
      image: {
        // Manufacture
        271: {
          id: 0x10f,
          format: 2,
          componentCount: 12,
          value: 'Manufacture\0'
        },
        // Device model
        272: {
          id: 0x110,
          format: 2,
          componentCount: 12,
          value: 'DeviceModel\0'
        },
        // GPS Pointer
        34853: {
          id: 0x8825,
          format: 4,
          componentCount: 1,
          value: 0 // The value is set by the encoder.
        }
      },
      exif: {
        // Lens model
        42036: {
          id: 0xa434,
          format: 2,
          componentCount: 10,
          value: 'LensModel\0'
        }
      },
      gps: {
        // GPS latitude ref
        1: {
          id: 0x1,
          format: 2,
          componentCount: 2,
          value: 'N\0'
        }
      }
    }
  };

  var encoder = ImageEncoder.encodeMetadata(metadata, canvas, 1);

  // Assert that ExifEncoder is returned.
  assertTrue(encoder instanceof ExifEncoder);

  var encodedResult = encoder.encode();

  // Decode encoded exif data.
  var exifParser = new ExifParser(this);

  // Redirect .log and .vlog to console.log for debugging.
  exifParser.log = function(arg) { console.log(arg); };
  exifParser.vlog = function(arg) { console.log(arg); };

  var parsedMetadata = {};
  var byteReader = new ByteReader(encodedResult);
  byteReader.readString(2 + 2); // Skip marker and size.
  exifParser.parseExifSection(parsedMetadata, encodedResult, byteReader);

  // Check ifd.image.
  assertEquals(1, parsedMetadata.ifd.image[0x112].value); // Orientation

  // Since thumbnail is compressed with JPEG, compression must be 6.
  assertEquals(6, parsedMetadata.ifd.image[0x102].value);

  // Check ifd.exif.
  assertEquals(1920, parsedMetadata.ifd.exif[0xA002].value); // PixelXDimension
  assertEquals(1080, parsedMetadata.ifd.exif[0xA003].value); // PixelYDimension

  // These fields should be copied correctly.
  // Manufacture
  assertEquals('Manufacture\0', parsedMetadata.ifd.image[0x10F].value);
  // Device model
  assertEquals('DeviceModel\0', parsedMetadata.ifd.image[0x110].value);
  // Lens model
  assertEquals('LensModel\0', parsedMetadata.ifd.exif[0xa434].value);
  // GPS latitude ref
  assertEquals('N\0', parsedMetadata.ifd.gps[0x1].value);

  // Software should be set as the Gallery app
  assertEquals('Chrome OS Gallery App\0',
      parsedMetadata.ifd.image[0x131].value);

  // Datetime should be updated.
  assertEquals('2015:01:07 15:30:06\0', parsedMetadata.ifd.image[0x132].value);

  // Thumbnail image
  assertTrue(!!parsedMetadata.thumbnailTransform);
  assertTrue(!!parsedMetadata.thumbnailURL);
}

/**
 * Helper function to encode and measure expected thumbnail size.
 * @return {number} Expected thumbnail size.
 */
function measureExpectedThumbnailSize_() {
  var canvas = getSampleCanvas();
  var metadata = {
    mediaMimeType: 'image/jpeg',
    modificationTime: new Date(2015, 0, 7, 15, 30, 6)
  };

  var encoder = ImageEncoder.encodeMetadata(metadata, canvas, 1);
  return ImageEncoder.decodeDataURL(encoder.thumbnailDataUrl).length;
};

/**
 * Helper function for testing that exif encoder drops thumbnail data if there
 * isn't enough space.
 *
 * @param {number} largeFieldValueSize Size of the large value.
 * @param {boolean} expectThumbnail True if thumbnail is expected to be written.
 */
function largeExifDataTestHelper_(largeFieldValueSize, expectThumbnail) {
  var canvas = getSampleCanvas();

  // Generate a long string.
  var longString = '0'.repeat(largeFieldValueSize - 1);
  longString += '\0';

  var metadata = {
    mediaMimeType: 'image/jpeg',
    modificationTime: new Date(2015, 0, 7, 15, 30, 6),
    ifd: {
      image: {
        // Manufacture
        271: {
          id: 0x10f,
          format: 2,
          componentCount: longString.length,
          value: longString
        }
      }
    }
  };

  var encoder = ImageEncoder.encodeMetadata(metadata, canvas, 1);

  // For failure case, an error is thrown.
  var encodedResult = encoder.encode();

  // Decode encoded exif data and check thumbnail is written or not.
  var exifParser = new ExifParser({verbose: false});
  var parsedMetadata = {};
  var byteReader = new ByteReader(encodedResult);
  byteReader.readString(2 + 2); // Skip marker and size.
  exifParser.parseExifSection(parsedMetadata, encodedResult, byteReader);

  assertEquals(expectThumbnail, !!parsedMetadata.thumbnailURL);
}

/**
 * Test case when other exif data is small as the thumbnail can fit in.
 */
function testLargeExifDataSmallCase() {
  // 158 bytes: other exif data except value of the large field.
  largeExifDataTestHelper_(
      ExifEncoder.MAXIMUM_EXIF_DATA_SIZE - measureExpectedThumbnailSize_() -
      ExifEncoder.THUMBNAIL_METADATA_SIZE - 158 - 1,
      true);
}

/**
 * Test case when other exif data is large as the thumbnail can just fit in.
 */
function testLargeExifDataBoundaryCase() {
  largeExifDataTestHelper_(
      ExifEncoder.MAXIMUM_EXIF_DATA_SIZE - measureExpectedThumbnailSize_() -
      ExifEncoder.THUMBNAIL_METADATA_SIZE - 158,
      true);
}

/**
 * Test case when other exif data is large as the thumbnail cannot fit in.
 */
function testLargeExifDataExceedsCase() {
  largeExifDataTestHelper_(
      ExifEncoder.MAXIMUM_EXIF_DATA_SIZE - measureExpectedThumbnailSize_() -
      ExifEncoder.THUMBNAIL_METADATA_SIZE - 158 + 1,
      false);
}
