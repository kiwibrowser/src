// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var app_id = "adjpiofaikamijlfbhehkldllbdcbmeb";
var extension_id = "ecglahbcnmdpdciemllbhojghbkagdje";

function successfulInstall() {
  chrome.inlineInstallPrivate.install(app_id,
                                      function(err, errorCode) {
    if (err.length == 0 && errorCode == "success") {
      chrome.test.sendMessage("success");
    } else {
      console.error("unexpected result, error:" + err + " errorCode:" +
                    errorCode);
    }
  });
}

function expectError(id, expectedErrorCode, expectedError) {
  chrome.inlineInstallPrivate.install(id, function(err, errorCode) {
    if (errorCode == expectedErrorCode &&
        (typeof(expectedError) == "undefined" || expectedError == err)) {
        chrome.test.sendMessage("success");
    } else {
      console.error("unexpected result, error:" + err + ", errorCode:" +
                    errorCode);
    }
  });
}

chrome.runtime.getBackgroundPage(function(bg) {
  console.error("testName is " + bg.testName);

  if (bg.testName == "successfulInstall") {
    successfulInstall();
  } else if (bg.testName == "noGesture") {
    expectError(app_id, "notPermitted", "Must be called with a user gesture");
  } else if (bg.testName == "onlyApps") {
    expectError(extension_id, "notPermitted");
  }
});
