// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var tests = [
  /**
   * Test that the correct bookmarks were loaded for test-bookmarks.pdf.
   */
  function testHasCorrectBookmarks() {
    var bookmarks = viewer.bookmarks;

    // Load all relevant bookmarks.
    chrome.test.assertEq(3, bookmarks.length);
    var firstBookmark = bookmarks[0];
    var secondBookmark = bookmarks[1];
    var uriBookmark = bookmarks[2];
    chrome.test.assertEq(1, firstBookmark.children.length);
    chrome.test.assertEq(0, secondBookmark.children.length);
    var firstNestedBookmark = firstBookmark.children[0];

    // Check titles.
    chrome.test.assertEq('First Section',
                         firstBookmark.title);
    chrome.test.assertEq('First Subsection',
                         firstNestedBookmark.title);
    chrome.test.assertEq('Second Section',
                         secondBookmark.title);
    chrome.test.assertEq('URI Bookmark', uriBookmark.title);

    // Check bookmark fields.
    chrome.test.assertEq(0, firstBookmark.page);
    chrome.test.assertEq(667, firstBookmark.y);
    chrome.test.assertEq(undefined, firstBookmark.uri);

    chrome.test.assertEq(1, firstNestedBookmark.page);
    chrome.test.assertEq(667, firstNestedBookmark.y);
    chrome.test.assertEq(undefined, firstNestedBookmark.uri);

    chrome.test.assertEq(2, secondBookmark.page);
    chrome.test.assertEq(667, secondBookmark.y);
    chrome.test.assertEq(undefined, secondBookmark.uri);

    chrome.test.assertEq(undefined, uriBookmark.page);
    chrome.test.assertEq(undefined, uriBookmark.y);
    chrome.test.assertEq('http://www.chromium.org', uriBookmark.uri);

    chrome.test.succeed();
  },

  /**
   * Test that a bookmark is followed when clicked in test-bookmarks.pdf.
   */
  function testFollowBookmark() {
    var bookmarkContent = Polymer.Base.create('viewer-bookmarks-content', {
      bookmarks: viewer.bookmarks,
      depth: 1
    });

    Polymer.dom.flush();

    var rootBookmarks =
        bookmarkContent.shadowRoot.querySelectorAll('viewer-bookmark');
    chrome.test.assertEq(3, rootBookmarks.length, "three root bookmarks");
    MockInteractions.tap(rootBookmarks[0].$.expand);

    Polymer.dom.flush();

    var subBookmarks =
        rootBookmarks[0].shadowRoot.querySelectorAll('viewer-bookmark');
    chrome.test.assertEq(1, subBookmarks.length, "one sub bookmark");

    var lastPageChange;
    var lastXChange;
    var lastYChange;
    var lastUriNavigation;
    bookmarkContent.addEventListener('change-page', function(e) {
      lastPageChange = e.detail.page;
      lastXChange = undefined;
      lastYChange = undefined;
      lastUriNavigation = undefined;
    });
    bookmarkContent.addEventListener('change-page-and-xy', function(e) {
      lastPageChange = e.detail.page;
      lastXChange = e.detail.x;
      lastYChange = e.detail.y;
      lastUriNavigation = undefined;
    });
    bookmarkContent.addEventListener('navigate', function(e) {
      lastPageChange = undefined;
      lastXChange = undefined;
      lastYChange = undefined;
      lastUriNavigation = e.detail.uri;
    });

    function testTapTarget(tapTarget, expectedEvent) {
      lastPageChange = undefined;
      lastXChange = undefined;
      lastYChange = undefined;
      lastUriNavigation = undefined;
      MockInteractions.tap(tapTarget);
      chrome.test.assertEq(expectedEvent.page, lastPageChange);
      chrome.test.assertEq(expectedEvent.x, lastXChange);
      chrome.test.assertEq(expectedEvent.y, lastYChange);
      chrome.test.assertEq(expectedEvent.uri, lastUriNavigation);
    }

    testTapTarget(rootBookmarks[0].$.item, {page: 0, x: 0, y: 667})
    testTapTarget(subBookmarks[0].$.item, {page: 1, x: 0, y: 667})
    testTapTarget(rootBookmarks[1].$.item, {page: 2, x: 0, y: 667})
    testTapTarget(rootBookmarks[2].$.item, {uri: "http://www.chromium.org"})

    chrome.test.succeed();
  }
];

var scriptingAPI = new PDFScriptingAPI(window, window);
scriptingAPI.setLoadCallback(function() {
  chrome.test.runTests(tests);
});
