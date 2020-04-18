// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function testGenerateAppForLink(url, title, error, func) {
  chrome.test.runWithUserGesture(function() {
    if (error)
      chrome.management.generateAppForLink(
          url, title, callback(function(data) {}, error));
    else
      chrome.management.generateAppForLink(url, title, callback(func));
  });
}

var tests = [
  function generateAppForLinkWithoutUserGesture() {
    chrome.management.generateAppForLink(
        "http://google.com", "test", callback(function() {},
            "chrome.management.generateAppForLink requires a user gesture."));
  },

  function generateAppForInvalidLink() {
    testGenerateAppForLink("", "test", "The URL \"\" is invalid.");
    testGenerateAppForLink("aaaa", "test", "The URL \"aaaa\" is invalid.");
    testGenerateAppForLink("http1://google.com", "test",
        "The URL \"http1://google.com\" is invalid.");
    testGenerateAppForLink("chrome://about", "test",
        "The URL \"chrome://about\" is invalid.");
    testGenerateAppForLink("chrome-extension://test/test", "test",
        "The URL \"chrome-extension://test/test\" is invalid.");
  },

  function generateAppWithEmptyTitle() {
    testGenerateAppForLink("http://google.com", "",
        "The title can not be empty.");
  },

  function generateAppForLinkWithShortURL() {
    var url = "http://google.com", title = "testApp";
    testGenerateAppForLink(
        url, title, null, function(data) {
          assertEq("http://google.com/", data.appLaunchUrl);
          assertEq(title, data.name);
          // There is no favicon in the test browser, so only 4 icons will
          // be created.
          assertEq(6, data.icons.length);
          assertEq(32, data.icons[0].size);
          assertEq(48, data.icons[1].size);
          assertEq(64, data.icons[2].size);
          assertEq(96, data.icons[3].size);
          assertEq(128, data.icons[4].size);
          assertEq(256, data.icons[5].size);

          chrome.management.getAll(callback(function(items) {
            assertTrue(getItemNamed(items, title) != null);
          }));
        });
  },

  function generateAppForLinkWithLongURL() {
    var url = "http://google.com/page/page?aa=bb&cc=dd", title = "test App 2";
    testGenerateAppForLink(
        url, title, null, function(data) {
          assertEq(url, data.appLaunchUrl);
          assertEq(title, data.name);
          assertEq(6, data.icons.length);
          assertEq(32, data.icons[0].size);
          assertEq(48, data.icons[1].size);
          assertEq(64, data.icons[2].size);
          assertEq(96, data.icons[3].size);
          assertEq(128, data.icons[4].size);
          assertEq(256, data.icons[5].size);

          chrome.management.getAll(callback(function(items) {
            assertTrue(getItemNamed(items, title) != null);
          }));
        });
  }
];

chrome.test.runTests(tests);

