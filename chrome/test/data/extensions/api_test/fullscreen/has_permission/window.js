// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

window.onload = function () {
  function requestFullscreen() {
    document.onwebkitfullscreenchange = chrome.test.succeed;
    document.onwebkitfullscreenerror = chrome.test.fail;
    chrome.test.runWithUserGesture(function() {
      document.body.webkitRequestFullscreen();
    });
  };
  document.body.onclick = requestFullscreen;  // enables manual testing.
  chrome.test.runTests([requestFullscreen]);
}
