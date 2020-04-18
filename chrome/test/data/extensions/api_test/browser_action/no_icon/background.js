// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var canvas = document.getElementById("canvas").getContext('2d').getImageData(
    0, 0, 21, 21);
var canvasHD = document.getElementById("canvas").getContext('2d').getImageData(
    0, 0, 42, 42);

var setIconParamQueue = [
  {imageData: canvas},
  {path: 'icon.png'},
  {imageData: {'21': canvas, '42': canvasHD}},
  {path: {'21': 'icon.png', '42': 'icon2.png'}},
  {imageData: {'21': canvas}},
  {path: {'21': 'icon.png'}},
  {imageData: {'42': canvasHD}},
  {imageData: {}},
  {path: {}},
];

// Called when the user clicks on the browser action.
chrome.browserAction.onClicked.addListener(function(windowId) {
  if (setIconParamQueue.length == 0) {
    chrome.test.notifyFail("Queue of params for test cases unexpectedly empty");
    return;
  }

  try {
    chrome.browserAction.setIcon(setIconParamQueue.shift(), function() {
      chrome.test.notifyPass();});
  } catch (error) {
    console.log(error.message);
    chrome.test.notifyFail(error.message);
  }
});

chrome.test.notifyPass();
