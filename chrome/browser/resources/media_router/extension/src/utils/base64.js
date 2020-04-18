// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.module('mr.base64');

/**
 * @const {!Map<string, string>}
 */
const URL_SAFE_MAP = new Map().set('+', '-').set('/', '_').set('=', '.');


/**
 * @const {!Map<string, string>}
 */
const INVERSE_URL_SAFE_MAP =
    new Map().set('-', '+').set('_', '/').set('.', '=');


/**
 * Decodes a base64 string using either the normal or URL-safe alphabet.
 * @param {string} encoded
 * @return {string}
 */
function decodeString(encoded) {
  return atob(encoded.replace(/[-_.]/g, c => INVERSE_URL_SAFE_MAP.get(c)));
}


/**
 * Encodes an array of byte values in base64.
 * @param {!Array<number>} data An array of byte values.
 * @param {boolean} urlSafe If true, uses a URL-safe base64 alphabet.
 * @return {string} The encoded data.
 */
function encodeArray(data, urlSafe) {
  const encoded = btoa(String.fromCharCode(...data));
  return urlSafe ? encoded.replace(/[+/=]/g, c => URL_SAFE_MAP.get(c)) :
                   encoded;
}


exports = {
  decodeString,
  encodeArray,
};
