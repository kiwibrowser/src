// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.runtime.onMessageExternal.addListener(
    function(message, sender, sendResponse) {
  var optionsPages = chrome.extension.getViews().filter(function(view) {
        return view.document.location.pathname == '/options.html';
  });
  var response = { hasOptionsPage: optionsPages.length > 0 };
  sendResponse(response);
});
