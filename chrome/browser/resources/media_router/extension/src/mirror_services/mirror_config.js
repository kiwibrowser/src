// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Mirroring configs.
 */

goog.provide('mr.mirror.Config');
goog.require('mr.PlatformUtils');


/**
 * True if TDLS is supported by the browser and platform.
 * @const {boolean}
 */
mr.mirror.Config.isTDLSSupportedByPlatform = Boolean(
    typeof chrome != 'undefined' && chrome.networkingPrivate &&
    chrome.networkingPrivate.setWifiTDLSEnabledState &&
    mr.PlatformUtils.getCurrentOS() == mr.PlatformUtils.OS.CHROMEOS);


/**
 * True if desktop audio capture is available.
 * @const {boolean}
 */
mr.mirror.Config.isDesktopAudioCaptureAvailable =
    // Audio capture is supported on ChromeOS with Chrome version
    // 30.0.1584.0 and up, and Chrome 31 on Windows.
    [mr.PlatformUtils.OS.CHROMEOS, mr.PlatformUtils.OS.WINDOWS].includes(
        mr.PlatformUtils.getCurrentOS());
