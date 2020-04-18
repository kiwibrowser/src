// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @param {MetadataParserLogger} parent Parent object.
 * @param {string} type Parser type.
 * @param {RegExp} urlFilter RegExp to match URLs.
 * @constructor
 * @struct
 */
function MetadataParser(parent, type, urlFilter) {
  this.parent_ = parent;
  this.type = type;
  this.urlFilter = urlFilter;
  this.verbose = parent.verbose;
  this.mimeType = 'unknown';
}

/**
 * Output an error message.
 * @param {...(Object|string)} var_args Arguments.
 */
MetadataParser.prototype.error = function(var_args) {
  this.parent_.error.apply(this.parent_, arguments);
};

/**
 * Output a log message.
 * @param {...(Object|string)} var_args Arguments.
 */
MetadataParser.prototype.log = function(var_args) {
  this.parent_.log.apply(this.parent_, arguments);
};

/**
 * Output a log message if |verbose| flag is on.
 * @param {...(Object|string)} var_args Arguments.
 */
MetadataParser.prototype.vlog = function(var_args) {
  if (this.verbose)
    this.parent_.log.apply(this.parent_, arguments);
};

/**
 * @return {Object} Metadata object with the minimal set of properties.
 */
MetadataParser.prototype.createDefaultMetadata = function() {
  return {
    type: this.type,
    mimeType: this.mimeType
  };
};

/**
 * Utility function to read specified range of bytes from file
 * @param {File} file The file to read.
 * @param {number} begin Starting byte(included).
 * @param {number} end Last byte(excluded).
 * @param {function(File, ByteReader)} callback Callback to invoke.
 * @param {function(string)} onError Error handler.
 */
MetadataParser.readFileBytes = function(file, begin, end, callback, onError) {
  var fileReader = new FileReader();
  fileReader.onerror = function(event) {
    onError(event.type);
  };
  fileReader.onloadend = function() {
    callback(file, new ByteReader(
        /** @type {ArrayBuffer} */ (fileReader.result)));
  };
  fileReader.readAsArrayBuffer(file.slice(begin, end));
};

/**
 * Base class for image metadata parsers.
 * @param {MetadataParserLogger} parent Parent object.
 * @param {string} type Image type.
 * @param {RegExp} urlFilter RegExp to match URLs.
 * @constructor
 * @struct
 * @extends {MetadataParser}
 */
function ImageParser(parent, type, urlFilter) {
  MetadataParser.apply(this, arguments);
  this.mimeType = 'image/' + this.type;
}

ImageParser.prototype = {__proto__: MetadataParser.prototype};
