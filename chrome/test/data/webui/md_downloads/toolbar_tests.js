// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

suite('toolbar tests', function() {
  /** @type {!downloads.Toolbar} */
  let toolbar;

  setup(function() {
    class TestSearchService extends downloads.SearchService {
      loadMore() { /* Prevent chrome.send(). */
      }
    }

    toolbar = document.createElement('downloads-toolbar');
    downloads.SearchService.instance_ = new TestSearchService;
    document.body.appendChild(toolbar);
  });

  test('resize closes more options menu', function() {
    MockInteractions.tap(toolbar.$.moreActions);
    assertTrue(toolbar.$.moreActionsMenu.open);

    window.dispatchEvent(new CustomEvent('resize'));
    assertFalse(toolbar.$.moreActionsMenu.open);
  });

  test('search starts spinner', function() {
    toolbar.$.toolbar.fire('search-changed', 'a');
    assertTrue(toolbar.spinnerActive);

    // Pretend the manager got results and set this to false.
    toolbar.spinnerActive = false;

    toolbar.$.toolbar.fire('search-changed', 'a ');  // Same term plus a space.
    assertFalse(toolbar.spinnerActive);
  });
});
