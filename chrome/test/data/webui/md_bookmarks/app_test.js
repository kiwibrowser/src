// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

suite('<bookmarks-app>', function() {
  let app;
  let store;

  function resetStore() {
    store = new bookmarks.TestStore({});
    store.acceptInitOnce();
    store.replaceSingleton();

    chrome.bookmarks.getTree = function(fn) {
      fn([
        createFolder(
            '0',
            [
              createFolder(
                  '1',
                  [
                    createFolder('11', []),
                  ]),
            ]),
      ]);
    };
  }

  setup(function() {
    window.localStorage.clear();
    resetStore();

    app = document.createElement('bookmarks-app');
    replaceBody(app);
  });

  test('write and load closed folder state', function() {
    const folderOpenStateList = [['1', true]];
    const folderOpenState = new Map(folderOpenStateList);
    store.data.folderOpenState = folderOpenState;
    store.notifyObservers();

    // Ensure closed folders are written to local storage.
    assertDeepEquals(
        JSON.stringify(Array.from(folderOpenState)),
        window.localStorage[LOCAL_STORAGE_FOLDER_STATE_KEY]);

    resetStore();
    app = document.createElement('bookmarks-app');
    replaceBody(app);

    // Ensure closed folders are read from local storage.
    assertDeepEquals(
        folderOpenStateList, normalizeIterable(store.data.folderOpenState));
  });

  test('write and load sidebar width', function() {
    assertEquals(getComputedStyle(app.$.sidebar).width, app.sidebarWidth_);

    const sidebarWidth = '500px';
    app.$.sidebar.style.width = sidebarWidth;
    cr.dispatchSimpleEvent(app.$.splitter, 'resize');
    assertEquals(
        sidebarWidth, window.localStorage[LOCAL_STORAGE_TREE_WIDTH_KEY]);

    app = document.createElement('bookmarks-app');
    replaceBody(app);

    assertEquals(sidebarWidth, app.$.sidebar.style.width);
  });

  test('focus ring hides and restores', function() {
    return PolymerTest.flushTasks().then(() => {
      const list = app.$$('bookmarks-list');
      const item = list.root.querySelectorAll('bookmarks-item')[0];
      const getFocusAttribute = () =>
          app.getAttribute(bookmarks.HIDE_FOCUS_RING_ATTRIBUTE);

      assertEquals(null, getFocusAttribute());

      MockInteractions.tap(item);
      assertEquals('', getFocusAttribute());

      MockInteractions.keyDownOn(item, 16, [], 'Shift');
      assertEquals('', getFocusAttribute());

      // This event is also captured by the bookmarks-list and propagation is
      // stopped. Regardless, it should clear the focus first.
      MockInteractions.keyDownOn(item, 40, [], 'ArrowDown');
      assertEquals(null, getFocusAttribute());
    });
  });
});
