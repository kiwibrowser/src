// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var tests = [
  /**
   * Test that some key elements exist and that they have the appropriate
   * constructor name. This verifies that polymer is working correctly.
   */
  function testHasElements() {
    var elementNames = [
      'viewer-pdf-toolbar',
      'viewer-zoom-toolbar',
      'viewer-password-screen',
      'viewer-error-screen'
    ];
    for (var i = 0; i < elementNames.length; i++) {
      var elements = document.querySelectorAll(elementNames[i]);
      chrome.test.assertEq(1, elements.length);
      var element = elements[0];
      chrome.test.assertTrue(
          String(element.constructor).indexOf(elementNames[i]) != -1);
    }
    chrome.test.succeed();
  },

  /**
   * Test that the plugin element exists and is navigated to the correct URL.
   */
  function testPluginElement() {
    var plugin = document.getElementById('plugin');
    chrome.test.assertEq('embed', plugin.localName);

    chrome.test.assertTrue(
        plugin.getAttribute('src').indexOf('/pdf/test.pdf') != -1);
    chrome.test.succeed();
  },

  /**
   * Test that shouldIgnoreKeyEvents correctly searches through the shadow DOM
   * to find input fields.
   */
  function testIgnoreKeyEvents() {
    // Test that the traversal through the shadow DOM works correctly.
    var toolbar = document.getElementById('toolbar');
    toolbar.$.pageselector.$.input.focus();
    chrome.test.assertTrue(shouldIgnoreKeyEvents(toolbar));

    // Test case where the active element has a shadow root of its own.
    toolbar.$.buttons.children[1].focus();
    chrome.test.assertFalse(shouldIgnoreKeyEvents(toolbar));

    chrome.test.assertFalse(
        shouldIgnoreKeyEvents(document.getElementById('plugin')));

    chrome.test.succeed();
  },

  /**
   * Test that the bookmarks menu can be closed by clicking the plugin and
   * pressing escape.
   */
  function testOpenCloseBookmarks() {
    var toolbar = $('toolbar');
    toolbar.show();
    var dropdown = toolbar.$.bookmarks;
    var plugin = $('plugin');
    var ESC_KEY = 27;

    // Clicking on the plugin should close the bookmarks menu.
    chrome.test.assertFalse(dropdown.dropdownOpen);
    MockInteractions.tap(dropdown.$.icon);
    chrome.test.assertTrue(dropdown.dropdownOpen);
    MockInteractions.tap(plugin);
    chrome.test.assertFalse(dropdown.dropdownOpen,
        "Clicking plugin closes dropdown");

    MockInteractions.tap(dropdown.$.icon);
    chrome.test.assertTrue(dropdown.dropdownOpen);
    MockInteractions.pressAndReleaseKeyOn(document, ESC_KEY);
    chrome.test.assertFalse(dropdown.dropdownOpen,
        "Escape key closes dropdown");
    chrome.test.assertTrue(toolbar.opened,
        "First escape key does not close toolbar");

    MockInteractions.pressAndReleaseKeyOn(document, ESC_KEY);
    chrome.test.assertFalse(toolbar.opened,
        "Second escape key closes toolbar");

    chrome.test.succeed();
  },

  /**
   * Test that the PDF filename is correctly extracted from URLs with query
   * parameters and fragments.
   */
  function testGetFilenameFromURL(url) {
    chrome.test.assertEq(
        'path.pdf',
        getFilenameFromURL(
            'http://example/com/path/with/multiple/sections/path.pdf'));

    chrome.test.assertEq(
        'fragment.pdf',
        getFilenameFromURL('http://example.com/fragment.pdf#zoom=100/Title'));

    chrome.test.assertEq(
        'query.pdf', getFilenameFromURL('http://example.com/query.pdf?p=a/b'));

    chrome.test.assertEq(
        'both.pdf',
        getFilenameFromURL('http://example.com/both.pdf?p=a/b#zoom=100/Title'));

    chrome.test.assertEq(
        'name with spaces.pdf',
        getFilenameFromURL('http://example.com/name%20with%20spaces.pdf'));

    chrome.test.assertEq(
        'invalid%EDname.pdf',
        getFilenameFromURL('http://example.com/invalid%EDname.pdf'));

    chrome.test.succeed();
  }
];

chrome.test.runTests(tests);
