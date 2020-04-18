// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

suite('<bookmarks-toast-manager>', function() {
  let toastManager;

  setup(function() {
    toastManager = document.createElement('bookmarks-toast-manager');
    replaceBody(toastManager);
  });

  test('simple show/hide', function() {
    toastManager.show('test', false);
    assertEquals('test', toastManager.$.content.textContent);
    assertTrue(toastManager.$.button.hidden);

    toastManager.hide();

    toastManager.show('test', true);
    assertFalse(toastManager.$.button.hidden);

    toastManager.hide();
    assertEquals(
        'hidden', window.getComputedStyle(toastManager.$.button).visibility);
  });
});
