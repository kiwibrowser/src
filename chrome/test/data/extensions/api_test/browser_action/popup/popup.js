// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function notify_pass() {
  // We should be done.  Let the test harness know.
  chrome.test.notifyPass();
}

function run_tests() {
  // Compute the size of the popup.
  var width = 0;
  var height = 0;
  if (localStorage.height) {
    height = parseInt(localStorage.height);
  }
  if (localStorage.width) {
    width = parseInt(localStorage.width);
  }

  // Set the div's size.
  var test = document.getElementById("test");
  test.style.width = width + "px";
  test.style.height = height + "px";
  chrome.test.log("height: " + test.offsetHeight);
  chrome.test.log("width: " + test.offsetWidth);

  height += 500;
  width += 500;
  localStorage.height = JSON.stringify(height);
  localStorage.width = JSON.stringify(width);

  // Allow for the pop-up resize to happen (asynchronously)
  // before saying that the test is done.
  window.setTimeout(notify_pass, 0);
}

window.addEventListener("load", function() {
    window.setTimeout(run_tests, 0)
}, false);
