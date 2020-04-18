// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Platform related utilities methods.

 */

goog.provide('mr.PlatformUtils');


/**
 * Returns true if the extension is running on Windows and Windows 8 or
 * newer.
 *
 * @return {boolean} Whether the extension is running on Windows 8 or
 *                   newer.
 */
mr.PlatformUtils.isWindows8OrNewer = function() {
  // See MSDN for different windows versions:
  // http://msdn.microsoft.com/en-us/library/ms537503(v=vs.85).aspx
  const [, version] = navigator.userAgent.match(/Windows NT (\d+.\d+)/) || [];

  // Windows 8 has version number 6.2.
  return parseFloat(version) >= 6.2;
};


/**
 * Enumeration of possible current operating systems.
 * @enum {string}
 */
mr.PlatformUtils.OS = {
  CHROMEOS: 'ChromeOS',
  WINDOWS: 'Windows',
  MAC: 'Mac',
  LINUX: 'Linux',
  OTHER: 'Other'
};


/**
 * Returns the current OS.
 * @return {!mr.PlatformUtils.OS}
 */
mr.PlatformUtils.getCurrentOS = function() {
  const userAgent = navigator.userAgent;
  if (userAgent.includes('CrOS')) {
    return mr.PlatformUtils.OS.CHROMEOS;
  }
  if (userAgent.includes('Windows')) {
    return mr.PlatformUtils.OS.WINDOWS;
  }
  if (userAgent.includes('Macintosh')) {
    return mr.PlatformUtils.OS.MAC;
  }
  if (userAgent.includes('Linux')) {
    return mr.PlatformUtils.OS.LINUX;
  }
  return mr.PlatformUtils.OS.OTHER;
};
