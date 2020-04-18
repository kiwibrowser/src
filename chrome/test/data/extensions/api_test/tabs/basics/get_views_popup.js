// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.test.runTests([
  function canGetViewsOfEmptyPopup() {
    testGetNewWindowView({type: "popup"}, []);
  },

  function canGetViewsOfPopupWithUrl() {
    var URLS = ["a.html"];
    testGetNewWindowView({type: "popup", url: URLS}, URLS);
  }
]);
