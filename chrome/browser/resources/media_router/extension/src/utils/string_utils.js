// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.module('mr.StringUtils');

class StringUtils {
  /**
   * Returns a string with at least 64-bits of randomness.
   *
   * Doesn't trust Javascript's random function entirely. Uses a combination of
   * random and current timestamp, and then encodes the string in base-36 to
   * make it shorter.
   *
   * @return {string} A random string, e.g. sn1s7vb4gcic.
   */
  static getRandomString() {
    var x = 2147483648;
    return Math.floor(Math.random() * x).toString(36) +
        Math.abs(Math.floor(Math.random() * x) ^ Date.now()).toString(36);
  }
}

exports = StringUtils;
