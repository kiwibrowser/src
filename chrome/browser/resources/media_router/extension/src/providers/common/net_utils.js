// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Utilites for handling network data.

 */

goog.module('mr.NetUtils');
goog.module.declareLegacyNamespace();



/** @const @private {!RegExp} */
exports.IPV4_REGEXP_ =
    new RegExp('^([0-9]{1,3})\\.([0-9]{1,3})\\.([0-9]{1,3})\\.([0-9]{1,3})$');

/** @const @private {!Array<number>} */
exports.IPV4_PRIVATE_ADDRESS_MASKS_ = [
  // 10.0.0.0/8
  4278190080,
  // 172.16.0.0/12
  4293918720,
  // 192.168.0.0/16
  4294901760
];

/** @const @private {!Array<number>} */
exports.IPV4_PRIVATE_ADDRESS_SUBNETS_ = [
  // 10.0.0.0/8
  167772160,
  // 172.16.0.0/12
  2886729728,
  // 192.168.0.0/16
  3232235520
];

/**
 * Parses an IPv4 dotted-quad address into an array of four numbers, or
 * returns null if the argument cannot be parsed.
 *
 * @param {string} ipAddress
 * @return {?Array<number>} Array of four integers 0-255 or null.
 */
exports.parseIPv4Address = function(ipAddress) {
  const matches = ipAddress.match(exports.IPV4_REGEXP_);
  if (!matches || matches.length != 5) return null;
  const result = [];
  for (let i = 0; i < 4; i++) {
    result[i] = Number.parseInt(matches[i + 1], 10);
    if (result[i] < 0 || result[i] > 255) return null;
  }
  return result;
};

/**
 * Returns true if ipAddress can be parsed into a valid IPv4 private network
 * address.
 *
 * @param {string} ipAddress
 * @return {boolean} True if ipAddress is a valid IPv4 private network address.
 */
exports.isPrivateIPv4Address = function(ipAddress) {
  const parsedAddress = exports.parseIPv4Address(ipAddress);
  if (!parsedAddress) return false;
  // >>> 0 converts a signed integer to unsigned, so bitwise operations are
  // sensible.
  const addressValue = (parsedAddress[0] << 24 | parsedAddress[1] << 16 |
                        parsedAddress[2] << 8 | parsedAddress[0]) >>>
      0;
  for (let i = 0; i < 3; i++) {
    if ((addressValue & exports.IPV4_PRIVATE_ADDRESS_MASKS_[i]) >>> 0 ==
        exports.IPV4_PRIVATE_ADDRESS_SUBNETS_[i])
      return true;
  }
  return false;
};

/**
 * @param {string} url A url.
 * @return {!HTMLAnchorElement} The result of parsing url.
 */
exports.parseUrl = function(url) {
  const a = document.createElement('a');
  a.href = url;
  return /** @type {!HTMLAnchorElement} */ (a);
};

/**
 * Non-exhaustive list of HTTP status codes. Add new codes as needed here.
 * @enum {number}
 */
const HttpStatus = {
  NOT_FOUND: 404,
};

exports.HttpStatus = HttpStatus;
