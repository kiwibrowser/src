// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Tests for MD Bookmarks which are run as interactive ui tests.
 * Should be used for tests which care about focus.
 */

const ROOT_PATH = '../../../../../';

GEN_INCLUDE(
    [ROOT_PATH + 'chrome/test/data/webui/polymer_interactive_ui_test.js']);

function MaterialBookmarksFocusTest() {}

MaterialBookmarksFocusTest.prototype = {
  __proto__: PolymerInteractiveUITest.prototype,

  browsePreload: 'chrome://bookmarks',

  extraLibraries: PolymerTest.getLibraries(ROOT_PATH).concat([
    'test_command_manager.js',
    'test_store.js',
    'test_util.js',
  ]),
};

TEST_F('MaterialBookmarksFocusTest', 'All', function() {
  suite('<bookmarks-folder-node>', function() {
    let rootNode;
    let store;

    function getFolderNode(id) {
      return findFolderNode(rootNode, id);
    }

    function keydown(id, key) {
      MockInteractions.keyDownOn(getFolderNode(id).$.container, '', [], key);
    }

    setup(function() {
      const nodes = testTree(
          createFolder(
              '1',
              [
                createFolder(
                    '2',
                    [
                      createFolder('3', []),
                      createFolder('4', []),
                    ]),
                createItem('5'),
              ]),
          createFolder('7', []));
      store = new bookmarks.TestStore({
        nodes: nodes,
        folderOpenState: getAllFoldersOpenState(nodes),
        selectedFolder: '1',
      });
      store.setReducersEnabled(true);
      store.replaceSingleton();

      rootNode = document.createElement('bookmarks-folder-node');
      rootNode.itemId = '0';
      rootNode.depth = -1;
      replaceBody(rootNode);
      Polymer.dom.flush();
    });

    test('keyboard selection', function() {
      function assertFocused(oldFocus, newFocus) {
        assertEquals(
            '-1', getFolderNode(oldFocus).$.container.getAttribute('tabindex'));
        assertEquals(
            '0', getFolderNode(newFocus).$.container.getAttribute('tabindex'));
        assertEquals(
            getFolderNode(newFocus).$.container,
            getFolderNode(newFocus).root.activeElement);
      }

      store.data.folderOpenState.set('2', false);
      store.notifyObservers();

      // The selected folder is focus enabled on attach.
      assertEquals(
          '0', getFolderNode('1').$.container.getAttribute('tabindex'));

      // Only the selected folder should be keyboard focusable.
      assertEquals(
          '-1', getFolderNode('2').$.container.getAttribute('tabindex'));

      store.data.search.term = 'asdf';

      // The selected folder is focus enabled even with a search term.
      assertEquals(
          '0', getFolderNode('1').$.container.getAttribute('tabindex'));

      store.data.search.term = '';

      // Give keyboard focus to the first item.
      getFolderNode('1').$.container.focus();

      // Move down into child.
      keydown('1', 'ArrowDown');

      assertDeepEquals(bookmarks.actions.selectFolder('2'), store.lastAction);
      store.data.selectedFolder = '2';
      store.notifyObservers();

      assertEquals(
          '-1', getFolderNode('1').$.container.getAttribute('tabindex'));
      assertEquals(
          '0', getFolderNode('2').$.container.getAttribute('tabindex'));
      assertFocused('1', '2');

      // Move down past closed folders.
      keydown('2', 'ArrowDown');
      assertDeepEquals(bookmarks.actions.selectFolder('7'), store.lastAction);
      assertFocused('2', '7');

      // Move down past end of list.
      store.resetLastAction();
      keydown('7', 'ArrowDown');
      assertDeepEquals(null, store.lastAction);

      // Move up past closed folders.
      keydown('7', 'ArrowUp');
      assertDeepEquals(bookmarks.actions.selectFolder('2'), store.lastAction);
      assertFocused('7', '2');

      // Move up into parent.
      keydown('2', 'ArrowUp');
      assertDeepEquals(bookmarks.actions.selectFolder('1'), store.lastAction);
      assertFocused('2', '1');

      // Move up past start of list.
      store.resetLastAction();
      keydown('1', 'ArrowUp');
      assertDeepEquals(null, store.lastAction);
    });

    test('keyboard left/right', function() {
      store.data.folderOpenState.set('2', false);
      store.notifyObservers();

      // Give keyboard focus to the first item.
      getFolderNode('1').$.container.focus();

      // Pressing right descends into first child.
      keydown('1', 'ArrowRight');
      assertDeepEquals(bookmarks.actions.selectFolder('2'), store.lastAction);

      // Pressing right on a closed folder opens that folder
      keydown('2', 'ArrowRight');
      assertDeepEquals(
          bookmarks.actions.changeFolderOpen('2', true), store.lastAction);

      // Pressing right again descends into first child.
      keydown('2', 'ArrowRight');
      assertDeepEquals(bookmarks.actions.selectFolder('3'), store.lastAction);

      // Pressing right on a folder with no children does nothing.
      store.resetLastAction();
      keydown('3', 'ArrowRight');
      assertDeepEquals(null, store.lastAction);

      // Pressing left on a folder with no children ascends to parent.
      keydown('3', 'ArrowDown');
      keydown('4', 'ArrowLeft');
      assertDeepEquals(bookmarks.actions.selectFolder('2'), store.lastAction);

      // Pressing left again closes the parent.
      keydown('2', 'ArrowLeft');
      assertDeepEquals(
          bookmarks.actions.changeFolderOpen('2', false), store.lastAction);

      // RTL flips left and right.
      document.body.style.direction = 'rtl';
      keydown('2', 'ArrowLeft');
      assertDeepEquals(
          bookmarks.actions.changeFolderOpen('2', true), store.lastAction);

      keydown('2', 'ArrowRight');
      assertDeepEquals(
          bookmarks.actions.changeFolderOpen('2', false), store.lastAction);

      document.body.style.direction = 'ltr';
    });

    test('keyboard commands are passed to command manager', function() {
      const commandManager = new TestCommandManager();
      document.body.appendChild(commandManager);
      chrome.bookmarkManagerPrivate.removeTrees = function() {};

      store.data.selection.items = new Set(['3', '4']);
      store.data.selectedFolder = '2';
      store.notifyObservers();

      getFolderNode('2').$.container.focus();
      keydown('2', 'Delete');

      commandManager.assertLastCommand(Command.DELETE, ['2']);
    });
  });

  suite('<bookmarks-list>', function() {
    let list;
    let store;
    let items;
    let commandManager;
    const multiKey = cr.isMac ? 'meta' : 'ctrl';

    function keydown(item, key, modifiers) {
      MockInteractions.keyDownOn(item, '', modifiers, key);
    }

    setup(function() {
      const nodes = testTree(createFolder('1', [
        createItem('2'),
        createItem('3'),
        createItem('4'),
        createItem('5'),
        createItem('6'),
        createFolder('7', []),
      ]));
      store = new bookmarks.TestStore({
        nodes: nodes,
        folderOpenState: getAllFoldersOpenState(nodes),
        selectedFolder: '1',
      });
      store.setReducersEnabled(true);
      store.replaceSingleton();

      list = document.createElement('bookmarks-list');
      list.style.height = '100%';
      list.style.width = '100%';
      list.style.position = 'absolute';
      replaceBody(list);
      Polymer.dom.flush();
      items = list.root.querySelectorAll('bookmarks-item');

      commandManager = new TestCommandManager();
      document.body.appendChild(commandManager);
    });

    test('simple keyboard selection', function() {
      let focusedItem = items[0];
      assertEquals('0', focusedItem.getAttribute('tabindex'));
      assertEquals(
          '0',
          focusedItem.$$('.more-vert-button button').getAttribute('tabindex'));
      focusedItem.focus();

      keydown(focusedItem, 'ArrowDown');
      focusedItem = items[1];
      assertEquals('0', focusedItem.getAttribute('tabindex'));
      assertEquals(
          '0',
          focusedItem.$$('.more-vert-button button').getAttribute('tabindex'));
      assertDeepEquals(['3'], normalizeIterable(store.data.selection.items));

      keydown(focusedItem, 'ArrowUp');
      focusedItem = items[0];
      assertEquals('0', focusedItem.getAttribute('tabindex'));
      assertDeepEquals(['2'], normalizeIterable(store.data.selection.items));

      keydown(focusedItem, 'ArrowRight');
      focusedItem = items[0];
      assertEquals(items[0], document.activeElement.root.activeElement);
      assertEquals(items[0].$.menuButton, items[0].root.activeElement);

      keydown(focusedItem, 'ArrowLeft');
      focusedItem = items[0];
      assertEquals(items[0], document.activeElement.root.activeElement);
      assertEquals(null, items[0].root.activeElement);

      keydown(focusedItem, 'End');
      focusedItem = items[5];
      assertEquals('0', focusedItem.getAttribute('tabindex'));
      assertDeepEquals(['7'], normalizeIterable(store.data.selection.items));

      // Moving past the end of the list is a no-op.
      keydown(focusedItem, 'ArrowDown');
      assertEquals('0', focusedItem.getAttribute('tabindex'));
      assertDeepEquals(['7'], normalizeIterable(store.data.selection.items));

      keydown(focusedItem, 'Home');
      focusedItem = items[0];
      assertEquals('0', focusedItem.getAttribute('tabindex'));
      assertDeepEquals(['2'], normalizeIterable(store.data.selection.items));

      // Moving past the start of the list is a no-op.
      keydown(focusedItem, 'ArrowUp');
      assertEquals('0', focusedItem.getAttribute('tabindex'));
      assertDeepEquals(['2'], normalizeIterable(store.data.selection.items));

      keydown(focusedItem, 'Escape');
      assertDeepEquals([], normalizeIterable(store.data.selection.items));

      keydown(focusedItem, 'a', multiKey);
      assertDeepEquals(
          ['2', '3', '4', '5', '6', '7'],
          normalizeIterable(store.data.selection.items));
    });

    test('shift selection', function() {
      let focusedItem = items[0];
      focusedItem.focus();

      keydown(focusedItem, 'ArrowDown', 'shift');
      focusedItem = items[1];
      assertDeepEquals(
          ['2', '3'], normalizeIterable(store.data.selection.items));

      keydown(focusedItem, 'Escape');
      focusedItem = items[1];
      assertDeepEquals([], normalizeIterable(store.data.selection.items));

      keydown(focusedItem, 'ArrowUp', 'shift');
      focusedItem = items[0];
      assertDeepEquals(
          ['2', '3'], normalizeIterable(store.data.selection.items));

      keydown(focusedItem, 'ArrowDown', 'shift');
      focusedItem = items[1];
      assertDeepEquals(['3'], normalizeIterable(store.data.selection.items));

      keydown(focusedItem, 'ArrowDown', 'shift');
      focusedItem = items[2];
      assertDeepEquals(
          ['3', '4'], normalizeIterable(store.data.selection.items));

      keydown(focusedItem, 'End', 'shift');
      focusedItem = items[2];
      assertDeepEquals(
          ['3', '4', '5', '6', '7'],
          normalizeIterable(store.data.selection.items));

      keydown(focusedItem, 'Home', 'shift');
      focusedItem = items[2];
      assertDeepEquals(
          ['2', '3'], normalizeIterable(store.data.selection.items));
    });

    test('ctrl selection', function() {
      let focusedItem = items[0];
      focusedItem.focus();

      keydown(focusedItem, ' ', multiKey);
      assertDeepEquals(['2'], normalizeIterable(store.data.selection.items));

      keydown(focusedItem, 'ArrowDown', multiKey);
      focusedItem = items[1];
      assertDeepEquals(['2'], normalizeIterable(store.data.selection.items));
      assertEquals('3', store.data.selection.anchor);

      keydown(focusedItem, 'ArrowDown', multiKey);
      focusedItem = items[2];
      assertDeepEquals(['2'], normalizeIterable(store.data.selection.items));

      keydown(focusedItem, ' ', multiKey);
      assertDeepEquals(
          ['2', '4'], normalizeIterable(store.data.selection.items));

      keydown(focusedItem, ' ', multiKey);
      assertDeepEquals(['2'], normalizeIterable(store.data.selection.items));
    });

    test('ctrl+shift selection', function() {
      let focusedItem = items[0];
      focusedItem.focus();

      keydown(focusedItem, ' ', multiKey);
      assertDeepEquals(['2'], normalizeIterable(store.data.selection.items));

      keydown(focusedItem, 'ArrowDown', multiKey);
      focusedItem = items[1];
      assertDeepEquals(['2'], normalizeIterable(store.data.selection.items));

      keydown(focusedItem, 'ArrowDown', multiKey);
      focusedItem = items[2];
      assertDeepEquals(['2'], normalizeIterable(store.data.selection.items));

      keydown(focusedItem, 'ArrowDown', [multiKey, 'shift']);
      focusedItem = items[3];
      assertDeepEquals(
          ['2', '4', '5'], normalizeIterable(store.data.selection.items));

      keydown(focusedItem, 'ArrowDown', [multiKey, 'shift']);
      focusedItem = items[3];
      assertDeepEquals(
          ['2', '4', '5', '6'], normalizeIterable(store.data.selection.items));
    });

    test('keyboard commands are passed to command manager', function() {
      chrome.bookmarkManagerPrivate.removeTrees = function() {};

      store.data.selection.items = new Set(['2', '3']);
      store.notifyObservers();

      const focusedItem = items[4];
      focusedItem.focus();

      keydown(focusedItem, 'Delete');
      // Commands should take affect on the selection, even if something else is
      // focused.
      commandManager.assertLastCommand(Command.DELETE, ['2', '3']);
    });

    test('iron-list does not steal focus on enter', function() {
      // Iron-list attempts to focus the whole <bookmarks-item> when pressing
      // enter on the menu button. This checks that we block this behavior
      // during keydown on <bookmarks-list>.
      const button = items[0].$$('.more-vert-button button');
      button.focus();
      keydown(button, 'Enter');

      assertEquals(button, items[0].root.activeElement);
    });
  });

  suite('DialogFocusManager', function() {
    let list;
    let store;
    let items;
    let commandManager;
    let dialogFocusManager;

    function waitForClose(el) {
      return new Promise(function(resolve) {
        listenOnce(el, 'close', function(e) {
          resolve();
        });
      });
    }

    function keydown(el, key) {
      MockInteractions.keyDownOn(el, '', '', key);
    }

    setup(function() {
      const nodes = testTree(createFolder('1', [
        createItem('2'),
        createItem('3'),
        createItem('4'),
        createItem('5'),
        createItem('6'),
        createFolder('7', []),
      ]));
      store = new bookmarks.TestStore({
        nodes: nodes,
        folderOpenState: getAllFoldersOpenState(nodes),
        selectedFolder: '1',
      });
      store.setReducersEnabled(true);
      store.replaceSingleton();

      list = document.createElement('bookmarks-list');
      list.style.height = '100%';
      list.style.width = '100%';
      list.style.position = 'absolute';
      replaceBody(list);
      Polymer.dom.flush();
      items = list.root.querySelectorAll('bookmarks-item');

      commandManager = new TestCommandManager();
      document.body.appendChild(commandManager);

      dialogFocusManager = new bookmarks.DialogFocusManager();
      bookmarks.DialogFocusManager.instance_ = dialogFocusManager;
    });

    test('restores focus on dialog dismissal', function() {
      const focusedItem = items[0];
      focusedItem.focus();
      assertEquals(focusedItem, dialogFocusManager.getFocusedElement_());

      commandManager.openCommandMenuAtPosition(0, 0);
      const dropdown = commandManager.$.dropdown.getIfExists();

      assertTrue(dropdown.open);
      assertNotEquals(focusedItem, dialogFocusManager.getFocusedElement_());

      keydown(dropdown, 'Escape');
      assertFalse(dropdown.open);

      return waitForClose(dropdown).then(() => {
        assertEquals(focusedItem, dialogFocusManager.getFocusedElement_());
      });
    });

    test('restores focus after stacked dialogs', function() {
      const focusedItem = items[0];
      focusedItem.focus();
      assertEquals(focusedItem, dialogFocusManager.getFocusedElement_());

      commandManager.openCommandMenuAtPosition(0, 0);
      const dropdown = commandManager.$.dropdown.getIfExists();
      dropdown.close();
      assertNotEquals(focusedItem, dialogFocusManager.getFocusedElement_());

      const editDialog = commandManager.$.editDialog.get();
      editDialog.showEditDialog(store.data.nodes['2']);

      return waitForClose(dropdown)
          .then(() => {
            editDialog.onCancelButtonTap_();
            assertNotEquals(
                focusedItem, dialogFocusManager.getFocusedElement_());

            return waitForClose(editDialog);
          })
          .then(() => {
            assertEquals(focusedItem, dialogFocusManager.getFocusedElement_());
          });
    });

    test('restores focus after multiple shows of same dialog', function() {
      let focusedItem = items[0];
      focusedItem.focus();
      assertEquals(focusedItem, dialogFocusManager.getFocusedElement_());

      commandManager.openCommandMenuAtPosition(0, 0);
      assertNotEquals(focusedItem, dialogFocusManager.getFocusedElement_());
      const dropdown = commandManager.$.dropdown.getIfExists();
      dropdown.close();

      focusedItem = items[3];
      focusedItem.focus();
      commandManager.openCommandMenuAtPosition(0, 0);

      return waitForClose(dropdown)
          .then(() => {
            assertTrue(dropdown.open);
            dropdown.close();
            assertNotEquals(
                focusedItem, dialogFocusManager.getFocusedElement_());

            return waitForClose(dropdown);
          })
          .then(() => {
            assertEquals(focusedItem, dialogFocusManager.getFocusedElement_());
          });
    });
  });

  mocha.run();
});
