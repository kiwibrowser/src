// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function getCards(manager) {
  return polymerSelectAll(manager, 'history-synced-device-card');
}

function numWindowSeparators(card) {
  return polymerSelectAll(card, ':not([hidden]).window-separator').length;
}

function assertNoSyncedTabsMessageShown(manager, stringID) {
  assertFalse(manager.$['no-synced-tabs'].hidden);
  const message = loadTimeData.getString(stringID);
  assertNotEquals(-1, manager.$['no-synced-tabs'].textContent.indexOf(message));
}

suite('<history-synced-device-manager>', function() {
  let element;

  const setForeignSessions = function(sessions) {
    element.sessionList = sessions;
  };

  setup(function() {
    element = document.createElement('history-synced-device-manager');
    element.signInState = true;
    element.searchTerm = '';
    replaceBody(element);
  });

  test('single card, single window', function() {
    const sessionList = [createSession(
        'Nexus 5',
        [createWindow(['http://www.google.com', 'http://example.com'])])];
    setForeignSessions(sessionList);

    return PolymerTest.flushTasks().then(function() {
      const card = element.$$('history-synced-device-card');
      assertEquals(
          'http://www.google.com',
          Polymer.dom(card.root)
              .querySelectorAll('.website-title')[0]
              .children[0]
              .textContent.trim());
      assertEquals(2, card.tabs.length);
    });
  });

  test('two cards, multiple windows', function() {
    const sessionList = [
      createSession(
          'Nexus 5',
          [createWindow(['http://www.google.com', 'http://example.com'])]),
      createSession(
          'Nexus 6',
          [
            createWindow(['http://test.com']),
            createWindow(['http://www.gmail.com', 'http://badssl.com'])
          ]),
    ];
    setForeignSessions(sessionList);

    return PolymerTest.flushTasks().then(function() {
      const cards = getCards(element);
      assertEquals(2, cards.length);

      // Ensure separators between windows are added appropriately.
      assertEquals(0, numWindowSeparators(cards[0]));
      assertEquals(1, numWindowSeparators(cards[1]));
    });
  });

  test('updating sessions', function() {
    const session1 = createSession(
        'Chromebook',
        [createWindow(['http://www.example.com', 'http://crbug.com'])]);
    session1.timestamp = 1000;

    const session2 =
        createSession('Nexus 5', [createWindow(['http://www.google.com'])]);

    setForeignSessions([session1, session2]);

    return PolymerTest.flushTasks()
        .then(function() {
          const session1updated = createSession('Chromebook', [
            createWindow(['http://www.example.com', 'http://crbug.com/new']),
            createWindow(['http://web.site'])
          ]);
          session1updated.timestamp = 1234;

          setForeignSessions([session1updated, session2]);

          return PolymerTest.flushTasks();
        })
        .then(function() {
          // There should only be two cards.
          const cards = getCards(element);
          assertEquals(2, cards.length);

          // There are now 2 windows in the first device.
          assertEquals(1, numWindowSeparators(cards[0]));

          // Check that the actual link changes.
          assertEquals(
              'http://crbug.com/new',
              Polymer.dom(cards[0].root)
                  .querySelectorAll('.website-title')[1]
                  .children[0]
                  .textContent.trim());
        });
  });

  test('two cards, multiple windows, search', function() {
    const sessionList = [
      createSession(
          'Nexus 5',
          [createWindow(['http://www.google.com', 'http://example.com'])]),
      createSession(
          'Nexus 6',
          [
            createWindow(['http://www.gmail.com', 'http://badssl.com']),
            createWindow(['http://test.com']),
            createWindow(['http://www.gmail.com', 'http://bagssl.com'])
          ]),
    ];
    setForeignSessions(sessionList);

    return PolymerTest.flushTasks()
        .then(function() {
          const cards = getCards(element);
          assertEquals(2, cards.length);

          // Ensure separators between windows are added appropriately.
          assertEquals(0, numWindowSeparators(cards[0]));
          assertEquals(2, numWindowSeparators(cards[1]));
          element.searchTerm = 'g';

          return PolymerTest.flushTasks();
        })
        .then(function() {
          const cards = getCards(element);

          assertEquals(0, numWindowSeparators(cards[0]));
          assertEquals(1, cards[0].tabs.length);
          assertEquals('http://www.google.com', cards[0].tabs[0].title);
          assertEquals(1, numWindowSeparators(cards[1]));
          assertEquals(3, cards[1].tabs.length);
          assertEquals('http://www.gmail.com', cards[1].tabs[0].title);
          assertEquals('http://www.gmail.com', cards[1].tabs[1].title);
          assertEquals('http://bagssl.com', cards[1].tabs[2].title);

          // Ensure the title text is rendered during searches.
          assertEquals(
              'http://www.google.com',
              Polymer.dom(cards[0].root)
                  .querySelectorAll('.website-title')[0]
                  .children[0]
                  .textContent.trim());

          element.searchTerm = 'Sans';
          return PolymerTest.flushTasks();
        })
        .then(function() {
          assertEquals(0, getCards(element).length);

          assertNoSyncedTabsMessageShown(element, 'noSearchResults');
        });
  });

  test('delete a session', function(done) {
    const sessionList = [
      createSession('Nexus 5', [createWindow(['http://www.example.com'])]),
      createSession('Pixel C', [createWindow(['http://www.badssl.com'])]),
    ];

    setForeignSessions(sessionList);

    return PolymerTest.flushTasks()
        .then(function() {
          const cards = getCards(element);
          assertEquals(2, cards.length);

          MockInteractions.tap(cards[0].$['menu-button']);
          return PolymerTest.flushTasks();
        })
        .then(function() {
          registerMessageCallback('deleteForeignSession', this, function(args) {
            assertEquals('Nexus 5', args[0]);

            // Simulate deleting the first device.
            setForeignSessions([sessionList[1]]);

            PolymerTest.flushTasks().then(function() {
              cards = getCards(element);
              assertEquals(1, cards.length);
              assertEquals('http://www.badssl.com', cards[0].tabs[0].title);
              done();
            });
          });

          MockInteractions.tap(element.$$('#menuDeleteButton'));
        });
  });

  test('delete a collapsed session', function() {
    const sessionList = [
      createSession('Nexus 5', [createWindow(['http://www.example.com'])]),
      createSession('Pixel C', [createWindow(['http://www.badssl.com'])]),
    ];

    setForeignSessions(sessionList);
    return PolymerTest.flushTasks()
        .then(function() {
          const cards = getCards(element);
          MockInteractions.tap(cards[0].$['card-heading']);
          assertFalse(cards[0].opened);

          // Simulate deleting the first device.
          setForeignSessions([sessionList[1]]);
          return PolymerTest.flushTasks();
        })
        .then(function() {
          const cards = getCards(element);
          assertTrue(cards[0].opened);
        });
  });

  test('click synced tab', function(done) {
    setForeignSessions(
        [createSession('Chromebook', [createWindow(['https://example.com'])])]);

    registerMessageCallback('openForeignSession', this, function(args) {
      assertEquals('Chromebook', args[0], 'sessionTag is correct');
      assertEquals('123', args[1], 'windowId is correct');
      assertEquals('456', args[2], 'tabId is correct');
      assertFalse(args[4], 'altKey is defined');
      assertFalse(args[5], 'ctrlKey is defined');
      assertFalse(args[6], 'metaKey is defined');
      assertFalse(args[7], 'shiftKey is defined');
      done();
    });

    PolymerTest.flushTasks().then(function() {
      const cards = getCards(element);
      const anchor = cards[0].root.querySelector('a');
      MockInteractions.tap(anchor);
    });
  });

  test('show actions menu', function() {
    setForeignSessions(
        [createSession('Chromebook', [createWindow(['https://example.com'])])]);

    return PolymerTest.flushTasks().then(function() {
      const cards = getCards(element);
      MockInteractions.tap(cards[0].$['menu-button']);
      assertTrue(element.$.menu.getIfExists().open);
    });
  });

  test('show sign in promo', function() {
    element.signInState = false;
    return PolymerTest.flushTasks()
        .then(function() {
          assertFalse(element.$['sign-in-guide'].hidden);
          element.signInState = true;
          return PolymerTest.flushTasks();
        })
        .then(function() {
          assertTrue(element.$['sign-in-guide'].hidden);
        });
  });

  test('no synced tabs message', function() {
    // When user is not logged in, there is no synced tabs.
    element.signInState = false;
    element.syncedDevices_ = [];
    return PolymerTest.flushTasks()
        .then(function() {
          assertTrue(element.$['no-synced-tabs'].hidden);

          const cards = getCards(element);
          assertEquals(0, cards.length);

          element.signInState = true;

          return PolymerTest.flushTasks();
        })
        .then(function() {
          // When user signs in, first show loading message.
          assertNoSyncedTabsMessageShown(element, 'loading');

          const sessionList = [];
          setForeignSessions(sessionList);
          return PolymerTest.flushTasks();
        })
        .then(function() {
          cards = getCards(element);
          assertEquals(0, cards.length);
          // If no synced tabs are fetched, show 'no synced tabs'.
          assertNoSyncedTabsMessageShown(element, 'noSyncedResults');

          sessionList = [createSession(
              'Nexus 5',
              [createWindow(['http://www.google.com', 'http://example.com'])])];
          setForeignSessions(sessionList);

          return PolymerTest.flushTasks();
        })
        .then(function() {
          cards = getCards(element);
          assertEquals(1, cards.length);
          // If there are any synced tabs, hide the 'no synced tabs' message.
          assertTrue(element.$['no-synced-tabs'].hidden);

          element.signInState = false;
          return PolymerTest.flushTasks();
        })
        .then(function() {
          // When user signs out, don't show the message.
          assertTrue(element.$['no-synced-tabs'].hidden);
        });
  });

  test('hide sign in promo in guest mode', function() {
    element.guestSession_ = true;
    return PolymerTest.flushTasks().then(function() {
      assertTrue(element.$['sign-in-guide'].hidden);
    });
  });

  teardown(function() {
    registerMessageCallback('openForeignSession', this, undefined);
    registerMessageCallback('deleteForeignSession', this, undefined);
  });
});
