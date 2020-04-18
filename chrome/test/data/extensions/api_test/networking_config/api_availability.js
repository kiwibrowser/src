// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.test.assertFalse(!chrome.networking.config,
                        "No networking_config namespace.");
chrome.test.assertFalse(!chrome.networking.config.setNetworkFilter,
                        "No setNetworkFilter function.");
chrome.test.assertFalse(!chrome.networking.config.finishAuthentication,
                        "No finishAuthentication function.");
chrome.test.succeed();
