// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

suite('<bookmarks-toolbar>', function() {
  let toolbar;
  let store;
  let commandManager;

  suiteSetup(function() {
    chrome.bookmarkManagerPrivate.removeTrees = function() {};
  });

  setup(function() {
    const nodes = testTree(createFolder('1', [
      createItem('2'),
      createItem('3'),
      createFolder('4', [], {unmodifiable: 'managed'}),
      createFolder('5', []),
      createFolder(
          '6',
          [
            createItem('61'),
            createItem('62'),
          ]),
    ]));
    store = new bookmarks.TestStore({
      nodes: nodes,
      folderOpenState: getAllFoldersOpenState(nodes),
      selection: {
        items: new Set(),
        anchor: null,
      },
    });
    store.replaceSingleton();

    toolbar = document.createElement('bookmarks-toolbar');
    replaceBody(toolbar);

    commandManager = new TestCommandManager();
    document.body.appendChild(commandManager);
  });

  test('selecting multiple items shows toolbar overlay', function() {
    assertFalse(toolbar.showSelectionOverlay);

    store.data.selection.items = new Set(['2']);
    store.notifyObservers();
    assertFalse(toolbar.showSelectionOverlay);

    store.data.selection.items = new Set(['2', '3']);
    store.notifyObservers();
    assertTrue(toolbar.showSelectionOverlay);
  });

  test('overlay does not show when editing is disabled', function() {
    store.data.prefs.canEdit = false;
    store.data.selection.items = new Set(['2', '3']);
    store.notifyObservers();
    assertFalse(toolbar.showSelectionOverlay);
  });

  test('clicking overlay delete button triggers a delete command', function() {
    store.data.selection.items = new Set(['2', '3']);
    store.notifyObservers();

    Polymer.dom.flush();
    const button = toolbar.$$('cr-toolbar-selection-overlay').deleteButton;
    assertFalse(button.disabled);
    MockInteractions.tap(button);

    commandManager.assertLastCommand(Command.DELETE, ['2', '3']);
  });

  test('commands do not trigger from the search field', function() {
    store.data.selection.items = new Set(['2']);
    store.notifyObservers();

    const input = toolbar.$$('cr-toolbar').getSearchField().getSearchInput();
    const modifier = cr.isMac ? 'meta' : 'ctrl';
    MockInteractions.pressAndReleaseKeyOn(input, 67, modifier, 'c');

    commandManager.assertLastCommand(null);
  });

  test('delete button is disabled when items are unmodifiable', function() {
    store.data.nodes['3'].unmodifiable = 'managed';
    store.data.selection.items = new Set(['2', '3']);
    store.notifyObservers();
    Polymer.dom.flush();

    assertTrue(toolbar.showSelectionOverlay);
    assertTrue(
        toolbar.$$('cr-toolbar-selection-overlay').deleteButton.disabled);
  });
});
