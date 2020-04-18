// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

suite('<history-list>', function() {
  let app;
  let element;
  let toolbar;
  let TEST_HISTORY_RESULTS;
  let ADDITIONAL_RESULTS;

  suiteSetup(function() {
    TEST_HISTORY_RESULTS = [
      createHistoryEntry('2016-03-15', 'https://www.google.com'),
      createHistoryEntry('2016-03-14 10:00', 'https://www.example.com'),
      createHistoryEntry('2016-03-14 9:00', 'https://www.google.com'),
      createHistoryEntry('2016-03-13', 'https://en.wikipedia.org')
    ];
    TEST_HISTORY_RESULTS[2].starred = true;

    ADDITIONAL_RESULTS = [
      createHistoryEntry('2016-03-13 10:00', 'https://en.wikipedia.org'),
      createHistoryEntry('2016-03-13 9:50', 'https://www.youtube.com'),
      createHistoryEntry('2016-03-11', 'https://www.google.com'),
      createHistoryEntry('2016-03-10', 'https://www.example.com')
    ];
  });

  setup(function() {
    app = replaceApp();
    element = app.$.history;
    toolbar = app.$.toolbar;
    app.queryState_.incremental = true;
  });

  test('deleting single item', function(done) {
    const listContainer = app.$.history;
    app.historyResult(
        createHistoryInfo(),
        [createHistoryEntry('2015-01-01', 'http://example.com')]);

    PolymerTest.flushTasks()
        .then(function() {
          assertEquals(element.historyData_.length, 1);
          items = polymerSelectAll(element, 'history-item');
          MockInteractions.tap(items[0].$.checkbox);
          assertDeepEquals([true], element.historyData_.map(i => i.selected));
          return PolymerTest.flushTasks();
        })
        .then(function() {
          toolbar.deleteSelectedItems();
          const dialog = listContainer.$.dialog.get();
          registerMessageCallback('removeVisits', this, function() {
            PolymerTest.flushTasks()
                .then(function() {
                  deleteComplete();
                  return PolymerTest.flushTasks();
                })
                .then(function() {
                  items = polymerSelectAll(element, 'history-item');
                  assertEquals(element.historyData_.length, 0);
                  done();
                });
          });
          assertTrue(dialog.open);
          MockInteractions.tap(listContainer.$$('.action-button'));
        });
  });

  test('cancelling selection of multiple items', function() {
    app.historyResult(createHistoryInfo(), TEST_HISTORY_RESULTS);
    return PolymerTest.flushTasks().then(function() {
      const items = polymerSelectAll(element, 'history-item');

      MockInteractions.tap(items[2].$.checkbox);
      MockInteractions.tap(items[3].$.checkbox);

      // Make sure that the array of data that determines whether or not an
      // item is selected is what we expect after selecting the two items.
      assertDeepEquals(
          [false, false, true, true],
          element.historyData_.map(i => i.selected));

      toolbar.clearSelectedItems();

      // Make sure that clearing the selection updates both the array and
      // the actual history-items affected.
      assertDeepEquals(
          [false, false, false, false],
          element.historyData_.map(i => i.selected));

      assertFalse(items[2].selected);
      assertFalse(items[3].selected);
    });
  });

  test('selection of multiple items using shift click', function() {
    app.historyResult(createHistoryInfo(), TEST_HISTORY_RESULTS);
    return PolymerTest.flushTasks().then(function() {
      const items = polymerSelectAll(element, 'history-item');

      MockInteractions.tap(items[1].$.checkbox);
      assertDeepEquals(
          [false, true, false, false],
          element.historyData_.map(i => i.selected));
      assertDeepEquals([1], Array.from(element.selectedItems).sort());

      // Shift-select to the last item.
      shiftClick(items[3].$.checkbox);
      assertDeepEquals(
          [false, true, true, true], element.historyData_.map(i => i.selected));
      assertDeepEquals([1, 2, 3], Array.from(element.selectedItems).sort());

      // Shift-select back to the first item.
      shiftClick(items[0].$.checkbox);
      assertDeepEquals(
          [true, true, true, true], element.historyData_.map(i => i.selected));
      assertDeepEquals([0, 1, 2, 3], Array.from(element.selectedItems).sort());

      // Shift-deselect to the third item.
      shiftClick(items[2].$.checkbox);
      assertDeepEquals(
          [false, false, false, true],
          element.historyData_.map(i => i.selected));
      assertDeepEquals([3], Array.from(element.selectedItems).sort());

      // Select the second item.
      MockInteractions.tap(items[1].$.checkbox);
      assertDeepEquals(
          [false, true, false, true],
          element.historyData_.map(i => i.selected));
      assertDeepEquals([1, 3], Array.from(element.selectedItems).sort());

      // Shift-deselect to the last item.
      shiftClick(items[3].$.checkbox);
      assertDeepEquals(
          [false, false, false, false],
          element.historyData_.map(i => i.selected));
      assertDeepEquals([], Array.from(element.selectedItems).sort());

      // Shift-select back to the third item.
      shiftClick(items[2].$.checkbox);
      assertDeepEquals(
          [false, false, true, true],
          element.historyData_.map(i => i.selected));
      assertDeepEquals([2, 3], Array.from(element.selectedItems).sort());

      // Remove selected items.
      element.removeItemsByIndex_(Array.from(element.selectedItems));
      assertDeepEquals(
          ['https://www.google.com', 'https://www.example.com'],
          element.historyData_.map(i => i.title));
    });
  });

  test('selection of all items using ctrl + a', function() {
    app.historyResult(createHistoryInfo(), TEST_HISTORY_RESULTS);
    return PolymerTest.flushTasks().then(function() {
      const field = toolbar.$['main-toolbar'].getSearchField();
      field.blur();
      assertFalse(field.showingSearch);

      const modifier = cr.isMac ? 'meta' : 'ctrl';
      MockInteractions.pressAndReleaseKeyOn(document.body, 65, modifier, 'a');

      assertDeepEquals(
          [true, true, true, true], element.historyData_.map(i => i.selected));

      // If everything is already selected, the same shortcut will trigger
      // cancelling selection.
      MockInteractions.pressAndReleaseKeyOn(document.body, 65, modifier, 'a');
      assertDeepEquals(
          [false, false, false, false],
          element.historyData_.map(i => i.selected));
    });
  });

  test('setting first and last items', function() {
    app.historyResult(createHistoryInfo(), TEST_HISTORY_RESULTS);

    return PolymerTest.flushTasks().then(function() {
      const items = polymerSelectAll(element, 'history-item');
      assertTrue(items[0].isCardStart);
      assertTrue(items[0].isCardEnd);
      assertFalse(items[1].isCardEnd);
      assertFalse(items[2].isCardStart);
      assertTrue(items[2].isCardEnd);
      assertTrue(items[3].isCardStart);
      assertTrue(items[3].isCardEnd);
    });
  });

  test('updating history results', function() {
    app.historyResult(createHistoryInfo(), TEST_HISTORY_RESULTS);
    app.historyResult(createHistoryInfo(), ADDITIONAL_RESULTS);

    return PolymerTest.flushTasks().then(function() {
      const items = polymerSelectAll(element, 'history-item');
      assertTrue(items[3].isCardStart);
      assertTrue(items[5].isCardEnd);

      assertTrue(items[6].isCardStart);
      assertTrue(items[6].isCardEnd);

      assertTrue(items[7].isCardStart);
      assertTrue(items[7].isCardEnd);
    });
  });

  test('deleting multiple items from view', function() {
    app.historyResult(createHistoryInfo(), TEST_HISTORY_RESULTS);
    app.historyResult(createHistoryInfo(), ADDITIONAL_RESULTS);
    return PolymerTest.flushTasks()
        .then(function() {

          element.removeItemsByIndex_([2, 5, 7]);

          return PolymerTest.flushTasks();
        })
        .then(function() {
          items = polymerSelectAll(element, 'history-item');

          assertEquals(element.historyData_.length, 5);
          assertEquals(element.historyData_[0].dateRelativeDay, '2016-03-15');
          assertEquals(element.historyData_[2].dateRelativeDay, '2016-03-13');
          assertEquals(element.historyData_[4].dateRelativeDay, '2016-03-11');

          // Checks that the first and last items have been reset correctly.
          assertTrue(items[2].isCardStart);
          assertTrue(items[3].isCardEnd);
          assertTrue(items[4].isCardStart);
          assertTrue(items[4].isCardEnd);
        });
  });

  test('search results display with correct item title', function() {
    app.historyResult(
        createHistoryInfo(),
        [createHistoryEntry('2016-03-15', 'https://www.google.com')]);
    element.searchedTerm = 'Google';

    return PolymerTest.flushTasks().then(function() {
      const item = element.$$('history-item');
      assertTrue(item.isCardStart);
      const heading = item.$$('#date-accessed').textContent;
      const title = item.$.title;

      // Check that the card title displays the search term somewhere.
      const index = heading.indexOf('Google');
      assertTrue(index != -1);

      // Check that the search term is bolded correctly in the history-item.
      assertGT(title.children[0].innerHTML.indexOf('<b>google</b>'), -1);
    });
  });

  test('correct display message when no history available', function() {
    app.historyResult(createHistoryInfo(), []);

    return PolymerTest.flushTasks()
        .then(function() {
          assertFalse(element.$['no-results'].hidden);
          assertNotEquals('', element.$['no-results'].textContent.trim());
          assertTrue(element.$['infinite-list'].hidden);

          app.historyResult(createHistoryInfo(), TEST_HISTORY_RESULTS);
          return PolymerTest.flushTasks();
        })
        .then(function() {
          assertTrue(element.$['no-results'].hidden);
          assertFalse(element.$['infinite-list'].hidden);
        });
  });

  test('more from this site sends and sets correct data', function(done) {
    app.queryState_.queryingDisabled = false;
    app.historyResult(createHistoryInfo(), TEST_HISTORY_RESULTS);
    PolymerTest.flushTasks().then(function() {
      registerMessageCallback('queryHistory', this, function(info) {
        assertEquals('www.google.com', info[0]);
        app.historyResult(
            createHistoryInfo('www.google.com'), TEST_HISTORY_RESULTS);

        PolymerTest.flushTasks().then(function() {
          assertEquals(
              'www.google.com',
              toolbar.$['main-toolbar'].getSearchField().getValue());

          element.$.sharedMenu.get().close();
          MockInteractions.tap(items[0].$['menu-button']);
          assertTrue(element.$$('#menuMoreButton').hidden);

          element.$.sharedMenu.get().close();
          MockInteractions.tap(items[1].$['menu-button']);
          assertFalse(element.$$('#menuMoreButton').hidden);

          done();
        });
      });

      items = polymerSelectAll(element, 'history-item');
      MockInteractions.tap(items[0].$['menu-button']);
      app.$.history.$.sharedMenu.get();
      MockInteractions.tap(app.$.history.$$('#menuMoreButton'));
    });
  });

  // TODO(calamity): Reenable this test after fixing flakiness.
  // See http://crbug.com/640862.
  test.skip('scrolling history list causes toolbar shadow to appear', () => {
    for (let i = 0; i < 10; i++)
      app.historyResult(createHistoryInfo(), TEST_HISTORY_RESULTS);
    return PolymerTest.flushTasks()
        .then(function() {
          assertFalse(app.toolbarShadow_);
          element.$['infinite-list'].scrollToIndex(20);
          return waitForEvent(app, 'toolbar-shadow_-changed');
        })
        .then(() => {
          assertTrue(app.toolbarShadow_);
          element.$['infinite-list'].scrollToIndex(0);
          return waitForEvent(app, 'toolbar-shadow_-changed');
        })
        .then(() => {
          assertFalse(app.toolbarShadow_);
        });
  });

  test('changing search deselects items', function() {
    app.historyResult(
        createHistoryInfo('ex'),
        [createHistoryEntry('2016-06-9', 'https://www.example.com')]);
    return PolymerTest.flushTasks().then(function() {
      const item = element.$$('history-item');
      MockInteractions.tap(item.$.checkbox);

      assertEquals(1, toolbar.count);
      app.queryState_.incremental = false;

      app.historyResult(
          createHistoryInfo('ample'),
          [createHistoryEntry('2016-06-9', 'https://www.example.com')]);
      assertEquals(0, toolbar.count);
    });
  });

  test('delete items end to end', function(done) {
    const listContainer = app.$.history;
    app.historyResult(createHistoryInfo(), TEST_HISTORY_RESULTS);
    app.historyResult(createHistoryInfo(), ADDITIONAL_RESULTS);
    app.historyResult(createHistoryInfo(), [
      createHistoryEntry('2015-01-01', 'http://example.com'),
      createHistoryEntry('2015-01-01', 'http://example.com'),
      createHistoryEntry('2015-01-01', 'http://example.com')
    ]);
    PolymerTest.flushTasks()
        .then(function() {
          items = polymerSelectAll(element, 'history-item');

          MockInteractions.tap(items[2].$.checkbox);
          MockInteractions.tap(items[5].$.checkbox);
          MockInteractions.tap(items[7].$.checkbox);
          MockInteractions.tap(items[8].$.checkbox);
          MockInteractions.tap(items[9].$.checkbox);
          MockInteractions.tap(items[10].$.checkbox);

          return PolymerTest.flushTasks();
        })
        .then(function() {
          toolbar.deleteSelectedItems();

          const dialog = listContainer.$.dialog.get();
          registerMessageCallback('removeVisits', this, function() {
            PolymerTest.flushTasks()
                .then(function() {
                  deleteComplete();
                  return PolymerTest.flushTasks();
                })
                .then(function() {
                  assertEquals(element.historyData_.length, 5);
                  assertEquals(
                      element.historyData_[0].dateRelativeDay, '2016-03-15');
                  assertEquals(
                      element.historyData_[2].dateRelativeDay, '2016-03-13');
                  assertEquals(
                      element.historyData_[4].dateRelativeDay, '2016-03-11');
                  assertFalse(dialog.open);

                  // Ensure the UI is correctly updated.
                  items = polymerSelectAll(element, 'history-item');
                  assertEquals('https://www.google.com', items[0].item.title);
                  assertEquals('https://www.example.com', items[1].item.title);
                  assertEquals('https://en.wikipedia.org', items[2].item.title);
                  assertEquals('https://en.wikipedia.org', items[3].item.title);
                  assertEquals('https://www.google.com', items[4].item.title);

                  assertFalse(dialog.open);
                  done();
                });
          });

          // Confirmation dialog should appear.
          assertTrue(dialog.open);
          MockInteractions.tap(listContainer.$$('.action-button'));
        });
  });

  test('delete via menu button', function(done) {
    const listContainer = app.$.history;
    app.historyResult(createHistoryInfo(), TEST_HISTORY_RESULTS);

    PolymerTest.flushTasks().then(function() {
      items = polymerSelectAll(element, 'history-item');
      registerMessageCallback('removeVisits', this, function() {
        PolymerTest.flushTasks()
            .then(function() {
              deleteComplete();
              return PolymerTest.flushTasks();
            })
            .then(function() {
              assertDeepEquals(
                  [
                    'https://www.google.com',
                    'https://www.google.com',
                    'https://en.wikipedia.org',
                  ],
                  element.historyData_.map(item => item.title));

              // Deletion should deselect all.
              assertDeepEquals(
                  [false, false, false],
                  items.slice(0, 3).map(i => i.selected));

              done();
            });
      });

      MockInteractions.tap(items[1].$.checkbox);
      MockInteractions.tap(items[3].$.checkbox);
      MockInteractions.tap(items[1].$['menu-button']);
      app.$.history.$.sharedMenu.get();
      MockInteractions.tap(app.$.history.$$('#menuRemoveButton'));
    });
  });

  test('deleting items using shortcuts', function(done) {
    const listContainer = app.$.history;
    app.historyResult(createHistoryInfo(), TEST_HISTORY_RESULTS);
    const dialog = listContainer.$.dialog.get();
    return PolymerTest.flushTasks()
        .then(function() {
          items = polymerSelectAll(element, 'history-item');

          // Dialog should not appear when there is no item selected.
          MockInteractions.pressAndReleaseKeyOn(
              document.body, 46, '', 'Delete');
          return PolymerTest.flushTasks();
        })
        .then(function() {
          assertFalse(dialog.open);

          MockInteractions.tap(items[1].$.checkbox);
          MockInteractions.tap(items[2].$.checkbox);

          assertEquals(2, toolbar.count);

          MockInteractions.pressAndReleaseKeyOn(
              document.body, 46, '', 'Delete');
          return PolymerTest.flushTasks();
        })
        .then(function() {
          assertTrue(dialog.open);
          MockInteractions.tap(listContainer.$$('.cancel-button'));
          assertFalse(dialog.open);

          MockInteractions.pressAndReleaseKeyOn(
              document.body, 8, '', 'Backspace');
          return PolymerTest.flushTasks();
        })
        .then(function() {
          assertTrue(dialog.open);

          registerMessageCallback('removeVisits', this, function(toRemove) {
            assertEquals('https://www.example.com', toRemove[0].url);
            assertEquals('https://www.google.com', toRemove[1].url);
            assertEquals('2016-03-14 10:00 UTC', toRemove[0].timestamps[0]);
            assertEquals('2016-03-14 9:00 UTC', toRemove[1].timestamps[0]);
            done();
          });

          MockInteractions.tap(listContainer.$$('.action-button'));
        });
  });

  test('delete dialog closed on back navigation', function(done) {
    // Ensure that state changes are always mirrored to the URL.
    app.$$('history-router').$$('iron-location').dwellTime = 0;
    app.queryState_.queryingDisabled = false;
    // Navigate from chrome://history/ to chrome://history/?q=something else.
    app.fire('change-query', {search: 'something else'});
    app.historyResult(createHistoryInfo(), TEST_HISTORY_RESULTS);
    app.historyResult(createHistoryInfo(), ADDITIONAL_RESULTS);

    const listContainer = app.$.history;
    PolymerTest.flushTasks()
        .then(function() {
          items = Polymer.dom(element.root).querySelectorAll('history-item');

          MockInteractions.tap(items[2].$.checkbox);
          return PolymerTest.flushTasks();
        })
        .then(function() {
          toolbar.deleteSelectedItems();
          return PolymerTest.flushTasks();
        })
        .then(function() {
          // Confirmation dialog should appear.
          assertTrue(listContainer.$.dialog.getIfExists().open);
          // Navigate back to chrome://history.
          window.history.back();

          listenOnce(window, 'popstate', function() {
            PolymerTest.flushTasks().then(function() {
              assertFalse(listContainer.$.dialog.getIfExists().open);
              done();
            });
          });
        });
  });

  test('clicking file:// url sends message to chrome', function(done) {
    const fileURL = 'file:///home/myfile';
    app.historyResult(createHistoryInfo(), [
      createHistoryEntry('2016-03-15', fileURL),
    ]);
    PolymerTest.flushTasks().then(function() {
      const items = Polymer.dom(element.root).querySelectorAll('history-item');

      registerMessageCallback('navigateToUrl', this, function(info) {
        assertEquals(fileURL, info[0]);
        done();
      });

      MockInteractions.tap(items[0].$.title);
    });
  });

  teardown(function() {
    registerMessageCallback('removeVisits', this, undefined);
    registerMessageCallback('queryHistory', this, function() {});
    registerMessageCallback('navigateToUrl', this, undefined);
    app.fire('change-query', {search: ''});
  });
});
