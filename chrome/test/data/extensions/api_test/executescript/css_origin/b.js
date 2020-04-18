// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.runtime.onMessage.addListener(function(message, sender, sendResponse) {
  var element = document.getElementById(message.id);
  var style = getComputedStyle(element);
  var response = {
    color: style.getPropertyValue('color'),
    backgroundColor: style.getPropertyValue('background-color')
  };
  sendResponse(response);
});
