// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

suite('bookmarks.Store', function() {
  let store;

  setup(function() {
    const nodes = testTree(createFolder('1', [
      createItem('11'),
      createItem('12'),
      createItem('13'),
    ]));
    store = new bookmarks.TestStore({
      nodes: nodes,
      folderOpenState: getAllFoldersOpenState(nodes),
    });
    store.setReducersEnabled(true);
    store.replaceSingleton();
  });

  test('batch mode disables updates', function() {
    let lastStateChange = null;
    const observer = {
      onStateChanged: function(state) {
        lastStateChange = state;
      },
    };

    store.addObserver(observer);
    store.beginBatchUpdate();

    store.dispatch(
        bookmarks.actions.removeBookmark('11', '1', 0, store.data.nodes));
    assertEquals(null, lastStateChange);
    store.dispatch(
        bookmarks.actions.removeBookmark('12', '1', 0, store.data.nodes));
    assertEquals(null, lastStateChange);

    store.endBatchUpdate();
    assertDeepEquals(['13'], lastStateChange.nodes['1'].children);
  });
});

suite('bookmarks.StoreClient', function() {
  let store;
  let client;

  function update(newState) {
    store.notifyObservers_(newState);
    Polymer.dom.flush();
  }

  function getRenderedItems() {
    return Array.from(client.root.querySelectorAll('.item'))
        .map((div) => div.textContent.trim());
  }

  suiteSetup(function() {
    document.body.innerHTML = `
      <dom-module is="test-store-client">
        <template>
          <template is="dom-repeat" items="[[items]]">
            <div class="item">[[item]]</div>
          </template>
        </template>
      </dom-module>
    `;

    Polymer({
      is: 'test-store-client',

      behaviors: [bookmarks.StoreClient],

      properties: {
        items: {
          type: Array,
          observer: 'itemsChanged_',
        },
      },

      attached: function() {
        this.hasChanged = false;
        this.watch('items', function(state) {
          return state.items;
        });
        this.updateFromStore();
      },

      itemsChanged_: function(newItems, oldItems) {
        if (oldItems)
          this.hasChanged = true;
      },
    });
  });

  setup(function() {
    PolymerTest.clearBody();

    // Reset store instance:
    bookmarks.Store.instance_ = new bookmarks.Store();
    store = bookmarks.Store.getInstance();
    store.init({
      items: ['apple', 'banana', 'cantaloupe'],
      count: 3,
    });

    client = document.createElement('test-store-client');
    document.body.appendChild(client);
    Polymer.dom.flush();
  });

  test('renders initial data', function() {
    assertDeepEquals(['apple', 'banana', 'cantaloupe'], getRenderedItems());
  });

  test('renders changes to watched state', function() {
    const newItems = ['apple', 'banana', 'courgette', 'durian'];
    const newState = Object.assign({}, store.data, {
      items: newItems,
    });
    update(newState);

    assertTrue(client.hasChanged);
    assertDeepEquals(newItems, getRenderedItems());
  });

  test('ignores changes to other subtrees', function() {
    const newState = Object.assign({}, store.data, {count: 2});
    update(newState);

    assertFalse(client.hasChanged);
  });
});
