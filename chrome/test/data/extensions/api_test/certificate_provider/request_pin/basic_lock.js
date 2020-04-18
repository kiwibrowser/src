// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// The script requests pin once and never send any other request again. Used to
// test when the user closes the dialog while request is processing.
function userInputCallback(codeValue) {
  if (chrome.runtime.lastError) {
    console.error('Error: ' + chrome.runtime.lastError.message);
    return;
  }
}

chrome.certificateProvider.requestPin({signRequestId: 123}, userInputCallback);
