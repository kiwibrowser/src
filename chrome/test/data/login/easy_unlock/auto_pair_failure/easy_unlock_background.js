// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.easyUnlockPrivate.onStartAutoPairing.addListener(function() {
  chrome.easyUnlockPrivate.setAutoPairingResult({
    success: false,
    errorMessage: 'Test failure'
  });
});
