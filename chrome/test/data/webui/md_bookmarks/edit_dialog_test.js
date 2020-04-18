// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

suite('<bookmarks-edit-dialog>', function() {
  let dialog;
  let lastUpdate;
  let lastCreation;

  suiteSetup(function() {
    chrome.bookmarks.update = function(id, edit) {
      lastUpdate.id = id;
      lastUpdate.edit = edit;
    };
    chrome.bookmarks.create = function(node) {
      lastCreation = node;
    };
  });

  setup(function() {
    lastUpdate = {};
    lastCreation = {};
    dialog = document.createElement('bookmarks-edit-dialog');
    replaceBody(dialog);
  });

  test('editing an item shows the url field', function() {
    const item = createItem('0');
    dialog.showEditDialog(item);

    assertFalse(dialog.$.url.hidden);
  });

  test('editing a folder hides the url field', function() {
    const folder = createFolder('0', []);
    dialog.showEditDialog(folder);

    assertTrue(dialog.$.url.hidden);
  });

  test('adding a folder hides the url field', function() {
    dialog.showAddDialog(true, '1');
    assertTrue(dialog.$.url.hidden);
  });

  test('editing passes the correct details to the update', function() {
    // Editing an item without changing anything.
    const item = createItem('1', {url: 'http://website.com', title: 'website'});
    dialog.showEditDialog(item);

    MockInteractions.tap(dialog.$.saveButton);

    assertEquals(item.id, lastUpdate.id);
    assertEquals(item.url, lastUpdate.edit.url);
    assertEquals(item.title, lastUpdate.edit.title);

    // Editing a folder, changing the title.
    const folder = createFolder('2', [], {title: 'Cool Sites'});
    dialog.showEditDialog(folder);
    dialog.titleValue_ = 'Awesome websites';

    MockInteractions.tap(dialog.$.saveButton);

    assertEquals(folder.id, lastUpdate.id);
    assertEquals(undefined, lastUpdate.edit.url);
    assertEquals('Awesome websites', lastUpdate.edit.title);
  });

  test('add passes the correct details to the backend', function() {
    dialog.showAddDialog(false, '1');

    dialog.titleValue_ = 'Permission Site';
    dialog.urlValue_ = 'permission.site';
    Polymer.dom.flush();

    MockInteractions.tap(dialog.$.saveButton);

    assertEquals('1', lastCreation.parentId);
    assertEquals('http://permission.site', lastCreation.url);
    assertEquals('Permission Site', lastCreation.title);
  });

  test('validates urls correctly', function() {
    dialog.urlValue_ = 'http://www.example.com';
    assertTrue(dialog.validateUrl_());

    dialog.urlValue_ = 'https://a@example.com:8080';
    assertTrue(dialog.validateUrl_());

    dialog.urlValue_ = 'example.com';
    Polymer.dom.flush();
    assertTrue(dialog.validateUrl_());
    Polymer.dom.flush();
    assertEquals('http://example.com', dialog.urlValue_);

    dialog.urlValue_ = '';
    assertFalse(dialog.validateUrl_());

    dialog.urlValue_ = '~~~example.com~~~';
    assertFalse(dialog.validateUrl_());
  });

  test('doesn\'t save when URL is invalid', function() {
    const item = createItem('0');
    dialog.showEditDialog(item);

    dialog.urlValue_ = '';

    Polymer.dom.flush();
    MockInteractions.tap(dialog.$.saveButton);
    Polymer.dom.flush();

    assertTrue(dialog.$.url.invalid);
    assertTrue(dialog.$.dialog.open);
  });
});
