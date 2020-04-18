// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

suite('item tests', function() {
  /** @type {!downloads.Item} */
  let item;

  setup(function() {
    item = document.createElement('downloads-item');
    document.body.appendChild(item);
  });

  test('dangerous downloads aren\'t linkable', function() {
    item.set('data', {
      danger_type: downloads.DangerType.DANGEROUS_FILE,
      file_externally_removed: false,
      state: downloads.States.DANGEROUS,
      url: 'http://evil.com'
    });
    Polymer.dom.flush();

    assertTrue(item.$['file-link'].hidden);
    assertFalse(item.$.url.hasAttribute('href'));
  });
});
