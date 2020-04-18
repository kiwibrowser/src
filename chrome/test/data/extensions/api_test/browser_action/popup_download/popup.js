// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function run_tests() {
  var anchor = document.querySelector("a");
  anchor.click();
}

window.addEventListener("load", function() {
    chrome.test.notifyPass();
}, false);
