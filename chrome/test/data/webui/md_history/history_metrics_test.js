// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @constructor
 * @extends {md_history.BrowserService}
 */
const TestMetricsBrowserService = function() {
  this.histogramMap = {};
  this.actionMap = {};
};

suite('Metrics', function() {
  let service;
  let app;
  let histogramMap;
  let actionMap;

  suiteSetup(function() {
    disableLinkClicks();

    TestMetricsBrowserService.prototype = {
      __proto__: md_history.BrowserService.prototype,

      /** @override */
      recordHistogram: function(histogram, value, max) {
        assertTrue(value < max);

        if (!(histogram in this.histogramMap))
          this.histogramMap[histogram] = {};

        if (!(value in this.histogramMap[histogram]))
          this.histogramMap[histogram][value] = 0;

        this.histogramMap[histogram][value]++;
      },

      /** @override */
      recordAction: function(action) {
        if (!(action in this.actionMap))
          this.actionMap[action] = 0;

        this.actionMap[action]++;
      },

      /** @override */
      deleteItems: function() {
        return PolymerTest.flushTasks();
      }
    };
  });

  setup(function() {
    md_history.BrowserService.instance_ = new TestMetricsBrowserService();
    service = md_history.BrowserService.getInstance();

    actionMap = service.actionMap;
    histogramMap = service.histogramMap;

    app = replaceApp();
    updateSignInState(false);
    return PolymerTest.flushTasks();
  });

  test('History.HistoryPageView', function() {
    app.grouped_ = true;

    const histogram = histogramMap['History.HistoryPageView'];
    assertEquals(1, histogram[HistoryPageViewHistogram.HISTORY]);

    app.selectedPage_ = 'syncedTabs';
    assertEquals(1, histogram[HistoryPageViewHistogram.SIGNIN_PROMO]);
    updateSignInState(true);
    return PolymerTest.flushTasks().then(() => {
      assertEquals(1, histogram[HistoryPageViewHistogram.SYNCED_TABS]);
      app.selectedPage_ = 'history';
      assertEquals(2, histogram[HistoryPageViewHistogram.HISTORY]);
    });
  });

  test('history-list', function() {
    const historyEntry =
        createHistoryEntry('2015-01-01', 'http://www.google.com');
    historyEntry.starred = true;
    app.historyResult(createHistoryInfo(), [
      createHistoryEntry('2015-01-01', 'http://www.example.com'), historyEntry
    ]);

    return PolymerTest.flushTasks()
        .then(() => {
          const items = polymerSelectAll(app.$.history, 'history-item');
          MockInteractions.tap(items[1].$$('#bookmark-star'));
          assertEquals(1, actionMap['BookmarkStarClicked']);
          MockInteractions.tap(items[1].$.title);
          assertEquals(1, actionMap['EntryLinkClick']);
          assertEquals(1, histogramMap['HistoryPage.ClickPosition'][1]);
          assertEquals(1, histogramMap['HistoryPage.ClickPositionSubset'][1]);

          app.fire('change-query', {search: 'goog'});
          assertEquals(1, actionMap['Search']);
          app.set('queryState_.incremental', true);
          app.historyResult(createHistoryInfo('goog'), [
            createHistoryEntry('2015-01-01', 'http://www.google.com'),
            createHistoryEntry('2015-01-01', 'http://www.google.com'),
            createHistoryEntry('2015-01-01', 'http://www.google.com')
          ]);
          return PolymerTest.flushTasks();
        })
        .then(() => {
          items = polymerSelectAll(app.$.history, 'history-item');
          MockInteractions.tap(items[0].$.title);
          assertEquals(1, actionMap['SearchResultClick']);
          assertEquals(1, histogramMap['HistoryPage.ClickPosition'][0]);
          assertEquals(1, histogramMap['HistoryPage.ClickPositionSubset'][0]);
          MockInteractions.tap(items[0].$.checkbox);
          MockInteractions.tap(items[4].$.checkbox);
          return PolymerTest.flushTasks();
        })
        .then(() => {
          app.$.toolbar.deleteSelectedItems();
          assertEquals(1, actionMap['RemoveSelected']);
          return PolymerTest.flushTasks();
        })
        .then(() => {
          MockInteractions.tap(app.$.history.$$('.cancel-button'));
          assertEquals(1, actionMap['CancelRemoveSelected']);
          app.$.toolbar.deleteSelectedItems();
          return PolymerTest.flushTasks();
        })
        .then(() => {
          MockInteractions.tap(app.$.history.$$('.action-button'));
          assertEquals(1, actionMap['ConfirmRemoveSelected']);
          return PolymerTest.flushTasks();
        })
        .then(() => {
          items = polymerSelectAll(app.$.history, 'history-item');
          MockInteractions.tap(items[0].$['menu-button']);
          return PolymerTest.flushTasks();
        })
        .then(() => {
          MockInteractions.tap(app.$.history.$$('#menuRemoveButton'));
          return PolymerTest.flushTasks();
        })
        .then(() => {
          assertEquals(1, histogramMap['HistoryPage.RemoveEntryPosition'][0]);
          assertEquals(
              1, histogramMap['HistoryPage.RemoveEntryPositionSubset'][0]);
        });
  });

  test('synced-device-manager', function() {
    app.selectedPage_ = 'syncedTabs';
    let histogram;
    let menuButton;
    return PolymerTest.flushTasks()
        .then(() => {
          histogram = histogramMap[SYNCED_TABS_HISTOGRAM_NAME];
          assertEquals(1, histogram[SyncedTabsHistogram.INITIALIZED]);

          const sessionList = [
            createSession('Nexus 5', [createWindow([
                            'http://www.google.com', 'http://example.com'
                          ])]),
            createSession(
                'Nexus 6',
                [
                  createWindow(['http://test.com']),
                  createWindow(['http://www.gmail.com', 'http://badssl.com'])
                ]),
          ];
          setForeignSessions(sessionList);
          return PolymerTest.flushTasks();
        })
        .then(() => {
          assertEquals(1, histogram[SyncedTabsHistogram.HAS_FOREIGN_DATA]);
          return PolymerTest.flushTasks();
        })
        .then(() => {
          cards = polymerSelectAll(
              app.$$('#synced-devices'), 'history-synced-device-card');
          MockInteractions.tap(cards[0].$['card-heading']);
          assertEquals(1, histogram[SyncedTabsHistogram.COLLAPSE_SESSION]);
          MockInteractions.tap(cards[0].$['card-heading']);
          assertEquals(1, histogram[SyncedTabsHistogram.EXPAND_SESSION]);
          MockInteractions.tap(polymerSelectAll(cards[0], '.website-title')[0]);
          assertEquals(1, histogram[SyncedTabsHistogram.LINK_CLICKED]);

          menuButton = cards[0].$['menu-button'];
          MockInteractions.tap(menuButton);
          return PolymerTest.flushTasks();
        })
        .then(() => {
          MockInteractions.tap(app.$$('#synced-devices').$$('#menuOpenButton'));
          assertEquals(1, histogram[SyncedTabsHistogram.OPEN_ALL]);

          MockInteractions.tap(menuButton);
          return PolymerTest.flushTasks();
        })
        .then(() => {
          MockInteractions.tap(
              app.$$('#synced-devices').$$('#menuDeleteButton'));
          assertEquals(1, histogram[SyncedTabsHistogram.HIDE_FOR_NOW]);
        });
  });
});
