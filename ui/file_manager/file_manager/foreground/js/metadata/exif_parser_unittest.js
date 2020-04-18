// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Creates a directory with specified tag. This method only supports string
 * format tag which is longer than 4 characters.
 * @param {!TypedArray} bytes Bytes to be written.
 * @param {!ExifEntry} tag An exif entry which will be written.
 */
function writeDirectory_(bytes, tag) {
  assertEquals(2, tag.format);
  assertTrue(tag.componentCount > 4);

  var byteWriter = new ByteWriter(bytes.buffer, 0);
  byteWriter.writeScalar(1, 2); // Number of fields.

  byteWriter.writeScalar(tag.id, 2);
  byteWriter.writeScalar(tag.format, 2);
  byteWriter.writeScalar(tag.componentCount, 4);
  byteWriter.forward(tag.id, 4);

  byteWriter.writeScalar(0, 4); // Offset to next IFD.

  byteWriter.resolveOffset(tag.id);
  byteWriter.writeString(tag.value);

  byteWriter.checkResolved();
}

/**
 * Parses exif data and return parsed tags.
 * @param {!TypedArray} bytes Bytes to be read.
 * @return {!Object<!Exif.Tag, !ExifEntry>} Tags.
 */
function parseExifData_(bytes) {
  var exifParser = new ExifParser(this);
  exifParser.log = function(arg) { console.log(arg); };
  exifParser.vlog = function(arg) { console.log(arg); };

  var tags = {};
  var byteReader = new ByteReader(bytes.buffer);
  assertEquals(0, exifParser.readDirectory(byteReader, tags));
  return tags;
}

/**
 * Test case a string doest not end with null character termination.
 */
function testWithoutNullCharacterTermination() {
  // Create a data which doesn't end with null character.
  var bytes = new Uint8Array(0x10000);
  writeDirectory_(bytes, {
    id: 0x10f, // Manufacture.
    format: 2, // String.
    componentCount: 11,
    value: 'Manufacture'
  });

  // Parse the data.
  var tags = parseExifData_(bytes);

  // Null character should be added at the end of value.
  var manufactureTag = tags[0x10f];
  assertEquals(12, manufactureTag.componentCount);
  assertEquals('Manufacture\0', manufactureTag.value);
}
