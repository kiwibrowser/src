// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function backgroundInstall() {
  chrome.inlineInstallPrivate.install("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
                                      function(error, errorCode) {
    if (error  == "Must be called from a foreground page")
      chrome.test.sendMessage("success");
    else
      console.error("Did not receive expected error; got '" + error + "'");
  });
}

// This gets set to the name of the test we want to run, either locally in this
// background page or in a window we open up.
var testName;

chrome.test.sendMessage("ready", function (response) {
  testName = response;
  if (testName == "backgroundInstall")
    backgroundInstall();
  else
    chrome.app.window.create("page.html");
});
