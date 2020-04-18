// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Check for support
if (window.chrome && window.chrome.app && window.chrome.app.isInstalled) {
  document.getElementById('supported').className = '';
} else {
  document.getElementById('unsupported').className = '';
}
var bgWinUrl = "background.html#yay";
var bgWinName = "bgNotifier";

function openBackgroundWindow() {
  window.open(bgWinUrl, bgWinName, "background");
}

function closeBackgroundWindow() {
  var w = window.open(bgWinUrl, bgWinName, "background");
  w.close();
}

document.addEventListener('DOMContentLoaded', function() {
  document.querySelector('#openBackgroundWindow').addEventListener(
    'click', openBackgroundWindow);
  document.querySelector('#closeBackgroundWindow').addEventListener(
    'click', closeBackgroundWindow);
});