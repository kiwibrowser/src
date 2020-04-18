// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Base class for image metadata parsers that only need to look at a short
 * fragment at the start of the file.
 * @param {MetadataParserLogger} parent Parent object.
 * @param {string} type Image type.
 * @param {RegExp} urlFilter RegExp to match URLs.
 * @param {number} headerSize Size of header.
 * @constructor
 * @struct
 * @extends {ImageParser}
 */
function SimpleImageParser(parent, type, urlFilter, headerSize) {
  ImageParser.call(this, parent, type, urlFilter);
  this.headerSize = headerSize;
}

SimpleImageParser.prototype.__proto__ = ImageParser.prototype;

/**
 * @param {File} file File to be parses.
 * @param {Object} metadata Metadata object of the file.
 * @param {function(Object)} callback Success callback.
 * @param {function(string)} errorCallback Error callback.
 */
SimpleImageParser.prototype.parse = function(
    file, metadata, callback, errorCallback) {
  var self = this;
  MetadataParser.readFileBytes(
      file, 0, this.headerSize,
      function(file, br) {
        try {
          self.parseHeader(metadata, br);
          callback(metadata);
        } catch (e) {
          errorCallback(e.toString());
        }
      },
      errorCallback);
};

/**
 * Parse header of an image. Inherited class must implement this.
 * @param {Object} metadata Dictionary to store the parsed metadata.
 * @param {ByteReader} byteReader Reader for header binary data.
 */
SimpleImageParser.prototype.parseHeader = function(metadata, byteReader) {};

/**
 * Parser for the header of png files.
 * @param {MetadataParserLogger} parent Parent object.
 * @extends {SimpleImageParser}
 * @constructor
 * @struct
 */
function PngParser(parent) {
  SimpleImageParser.call(this, parent, 'png', /\.png$/i, 24);
}

PngParser.prototype = {__proto__: SimpleImageParser.prototype};

/**
 * @override
 */
PngParser.prototype.parseHeader = function(metadata, br) {
  br.setByteOrder(ByteReader.BIG_ENDIAN);

  var signature = br.readString(8);
  if (signature != '\x89PNG\x0D\x0A\x1A\x0A')
    throw new Error('Invalid PNG signature: ' + signature);

  br.seek(12);
  var ihdr = br.readString(4);
  if (ihdr != 'IHDR')
    throw new Error('Missing IHDR chunk');

  metadata.width = br.readScalar(4);
  metadata.height = br.readScalar(4);
};

registerParserClass(PngParser);

/**
 * Parser for the header of bmp files.
 * @param {MetadataParserLogger} parent Parent object.
 * @constructor
 * @extends {SimpleImageParser}
 * @struct
 */
function BmpParser(parent) {
  SimpleImageParser.call(this, parent, 'bmp', /\.bmp$/i, 28);
}

BmpParser.prototype = {__proto__: SimpleImageParser.prototype};

/**
 * @override
 */
BmpParser.prototype.parseHeader = function(metadata, br) {
  br.setByteOrder(ByteReader.LITTLE_ENDIAN);

  var signature = br.readString(2);
  if (signature != 'BM')
    throw new Error('Invalid BMP signature: ' + signature);

  br.seek(18);
  metadata.width = br.readScalar(4);
  metadata.height = br.readScalar(4);
};

registerParserClass(BmpParser);

/**
 * Parser for the header of gif files.
 * @param {MetadataParserLogger} parent Parent object.
 * @constructor
 * @extends {SimpleImageParser}
 * @struct
 */
function GifParser(parent) {
  SimpleImageParser.call(this, parent, 'gif', /\.Gif$/i, 10);
}

GifParser.prototype = {__proto__: SimpleImageParser.prototype};

/**
 * @override
 */
GifParser.prototype.parseHeader = function(metadata, br) {
  br.setByteOrder(ByteReader.LITTLE_ENDIAN);

  var signature = br.readString(6);
  if (!signature.match(/GIF8(7|9)a/))
    throw new Error('Invalid GIF signature: ' + signature);

  metadata.width = br.readScalar(2);
  metadata.height = br.readScalar(2);
};

registerParserClass(GifParser);

/**
 * Parser for the header of webp files.
 * @param {MetadataParserLogger} parent Parent object.
 * @constructor
 * @extends {SimpleImageParser}
 * @struct
 */
function WebpParser(parent) {
  SimpleImageParser.call(this, parent, 'webp', /\.webp$/i, 30);
}

WebpParser.prototype = {__proto__: SimpleImageParser.prototype};

/**
 * @override
 */
WebpParser.prototype.parseHeader = function(metadata, br) {
  br.setByteOrder(ByteReader.LITTLE_ENDIAN);

  var riffSignature = br.readString(4);
  if (riffSignature != 'RIFF')
    throw new Error('Invalid RIFF signature: ' + riffSignature);

  br.seek(8);
  var webpSignature = br.readString(4);
  if (webpSignature != 'WEBP')
    throw new Error('Invalid WEBP signature: ' + webpSignature);

  var chunkFormat = br.readString(4);
  switch (chunkFormat) {
    // VP8 lossy bitstream format.
    case 'VP8 ':
      br.seek(23);
      var lossySignature = br.readScalar(2) | (br.readScalar(1) << 16);
      if (lossySignature != 0x2a019d) {
        throw new Error('Invalid VP8 lossy bitstream signature: ' +
            lossySignature);
      }
      var dimensionBits = br.readScalar(4);
      metadata.width = dimensionBits & 0x3fff;
      metadata.height = (dimensionBits >> 16) & 0x3fff;
      break;

    // VP8 lossless bitstream format.
    case 'VP8L':
      br.seek(20);
      var losslessSignature = br.readScalar(1);
      if (losslessSignature != 0x2f) {
        throw new Error('Invalid VP8 lossless bitstream signature: ' +
            losslessSignature);
      }
      var dimensionBits = br.readScalar(4);
      metadata.width = (dimensionBits & 0x3fff) + 1;
      metadata.height = ((dimensionBits >> 14) & 0x3fff) + 1;
      break;

    // VP8 extended file format.
    case 'VP8X':
      br.seek(20);
      // Read 24-bit value. ECMAScript assures left-to-right evaluation order.
      metadata.width = (br.readScalar(2) | (br.readScalar(1) << 16)) + 1;
      metadata.height = (br.readScalar(2) | (br.readScalar(1) << 16)) + 1;
      break;

    default:
      throw new Error('Invalid chunk format: ' + chunkFormat);
  }
};

registerParserClass(WebpParser);

/**
 * Parser for the header of .ico icon files.
 * @param {MetadataParserLogger} parent Parent metadata dispatcher object.
 * @constructor
 * @extends {SimpleImageParser}
 */
function IcoParser(parent) {
  SimpleImageParser.call(this, parent, 'ico', /\.ico$/i, 8);
}

IcoParser.prototype = {__proto__: SimpleImageParser.prototype};

/**
 * @override
 */
IcoParser.prototype.parseHeader = function(metadata, byteReader) {
  byteReader.setByteOrder(ByteReader.LITTLE_ENDIAN);

  var signature = byteReader.readString(4);
  if (signature !== '\x00\x00\x00\x01')
    throw new Error('Invalid ICO signature: ' + signature);

  byteReader.seek(2);
  metadata.width = byteReader.readScalar(1);
  metadata.height = byteReader.readScalar(1);
};

registerParserClass(IcoParser);
