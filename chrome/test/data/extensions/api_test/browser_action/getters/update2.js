// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var pass = chrome.test.callbackPass;

chrome.tabs.getSelected(null, function(tab) {
  chrome.browserAction.setPopup({tabId: tab.id, popup: 'newPopup.html'})
  chrome.browserAction.setTitle({tabId: tab.id, title: 'newTitle'});
  chrome.browserAction.setBadgeBackgroundColor({
    tabId: tab.id,
    color: [0, 0, 0, 0]
  });
  chrome.browserAction.setBadgeText({tabId: tab.id, text: 'newText'});

  chrome.test.runTests([
    function getBadgeText() {
      chrome.browserAction.getBadgeText({tabId: tab.id}, pass(function(result) {
        chrome.test.assertEq("newText", result);
      }));
    },

    function getBadgeBackgroundColor() {
      chrome.browserAction.getBadgeBackgroundColor({tabId: tab.id},
                                                   pass(function(result) {
        chrome.test.assertEq([0, 0, 0, 0], result);
      }));
    },

    function getPopup() {
      chrome.browserAction.getPopup({tabId: tab.id}, pass(function(result) {
        chrome.test.assertTrue(
            /chrome-extension\:\/\/[a-p]{32}\/newPopup\.html/.test(result));
      }));
    },

    function getTitle() {
      chrome.browserAction.getTitle({tabId: tab.id}, pass(function(result) {
        chrome.test.assertEq("newTitle", result);
      }));
    }
  ]);
});
