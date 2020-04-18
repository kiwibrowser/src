// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Test suite for the Material Design history page.
 */

const ROOT_PATH = '../../../../../';

GEN_INCLUDE(
    [ROOT_PATH + 'chrome/test/data/webui/polymer_browser_test_base.js']);
GEN('#include "base/command_line.h"');
GEN('#include "chrome/test/data/webui/history_ui_browsertest.h"');

function MaterialHistoryBrowserTest() {}

MaterialHistoryBrowserTest.prototype = {
  __proto__: PolymerTest.prototype,

  browsePreload: 'chrome://history',

  /** @override */
  runAccessibilityChecks: false,

  extraLibraries: PolymerTest.getLibraries(ROOT_PATH).concat([
    'test_util.js',
  ]),

  /** @override */
  setUp: function() {
    PolymerTest.prototype.setUp.call(this);

    suiteSetup(function() {
      // Wait for the top-level app element to be upgraded.
      return waitForAppUpgrade()
          .then(function() {
            // <iron-list>#_maxPages controls the default number of "pages" of
            // "physical" (i.e. DOM) elements to render. Some of these tests
            // rely on rendering up to 3 "pages" of items, which was previously
            // the default, changeed to 2 for performance reasons. TODO(dbeam):
            // maybe trim down the number of items created in the tests? Or
            // don't touch <iron-list>'s physical items as much?
            Array.from(document.querySelectorAll('* /deep/ iron-list'))
                .forEach(function(ironList) {
                  ironList._maxPages = 3;
                });
          })
          .then(function() {
            return md_history.ensureLazyLoaded();
          })
          .then(function() {
            $('history-app').queryState_.queryingDisabled = true;
          });
    });
  },
};

function MaterialHistoryBrowserServiceTest() {}

MaterialHistoryBrowserServiceTest.prototype = {
  __proto__: MaterialHistoryBrowserTest.prototype,

  extraLibraries: MaterialHistoryBrowserTest.prototype.extraLibraries.concat([
    'browser_service_test.js',
  ]),
};

TEST_F('MaterialHistoryBrowserServiceTest', 'All', function() {
  mocha.run();
});

function MaterialHistoryDrawerTest() {}

MaterialHistoryDrawerTest.prototype = {
  __proto__: MaterialHistoryBrowserTest.prototype,

  extraLibraries: MaterialHistoryBrowserTest.prototype.extraLibraries.concat([
    'history_drawer_test.js',
  ]),
};

TEST_F('MaterialHistoryDrawerTest', 'All', function() {
  mocha.run();
});

function MaterialHistoryItemTest() {}

MaterialHistoryItemTest.prototype = {
  __proto__: MaterialHistoryBrowserTest.prototype,

  extraLibraries: MaterialHistoryBrowserTest.prototype.extraLibraries.concat([
    'history_item_test.js',
  ]),
};

TEST_F('MaterialHistoryItemTest', 'All', function() {
  mocha.run();
});

function MaterialHistoryListTest() {}

MaterialHistoryListTest.prototype = {
  __proto__: MaterialHistoryBrowserTest.prototype,

  extraLibraries: MaterialHistoryBrowserTest.prototype.extraLibraries.concat([
    'history_list_test.js',
  ]),
};

// Times out on debug builders because the History page can take several seconds
// to load in a Debug build. See https://crbug.com/669227.
GEN('#if !defined(NDEBUG)');
GEN('#define MAYBE_All DISABLED_All');
GEN('#else');
GEN('#define MAYBE_All All');
GEN('#endif');

TEST_F('MaterialHistoryListTest', 'MAYBE_All', function() {
  mocha.run();
});

function MaterialHistoryMetricsTest() {}

MaterialHistoryMetricsTest.prototype = {
  __proto__: MaterialHistoryBrowserTest.prototype,

  extraLibraries: MaterialHistoryBrowserTest.prototype.extraLibraries.concat([
    'history_metrics_test.js',
  ]),
};

TEST_F('MaterialHistoryMetricsTest', 'All', function() {
  mocha.run();
});

function MaterialHistoryOverflowMenuTest() {}

MaterialHistoryOverflowMenuTest.prototype = {
  __proto__: MaterialHistoryBrowserTest.prototype,

  extraLibraries: MaterialHistoryBrowserTest.prototype.extraLibraries.concat([
    'history_overflow_menu_test.js',
  ]),
};

TEST_F('MaterialHistoryOverflowMenuTest', 'All', function() {
  mocha.run();
});

function MaterialHistoryRoutingTest() {}

MaterialHistoryRoutingTest.prototype = {
  __proto__: MaterialHistoryBrowserTest.prototype,

  extraLibraries: MaterialHistoryBrowserTest.prototype.extraLibraries.concat([
    'history_routing_test.js',
  ]),
};

TEST_F('MaterialHistoryRoutingTest', 'All', function() {
  md_history.history_routing_test.registerTests();
  mocha.run();
});

function MaterialHistoryRoutingWithQueryParamTest() {}

MaterialHistoryRoutingWithQueryParamTest.prototype = {
  __proto__: MaterialHistoryRoutingTest.prototype,

  browsePreload: 'chrome://history/?q=query',

  /** @override */
  setUp: function() {
    PolymerTest.prototype.setUp.call(this);
    // This message handler needs to be registered before the test since the
    // query can happen immediately after the element is upgraded. However,
    // since there may be a delay as well, the test might check the global var
    // too early as well. In this case the test will have overtaken the
    // callback.
    registerMessageCallback('queryHistory', this, function(info) {
      window.historyQueryInfo = info;
    });

    suiteSetup(function() {
      // Wait for the top-level app element to be upgraded.
      return waitForAppUpgrade().then(function() {
        md_history.ensureLazyLoaded();
      });
    });
  },
};

TEST_F('MaterialHistoryRoutingWithQueryParamTest', 'All', function() {
  md_history.history_routing_test_with_query_param.registerTests();
  mocha.run();
});

function MaterialHistorySyncedTabsTest() {}

MaterialHistorySyncedTabsTest.prototype = {
  __proto__: MaterialHistoryBrowserTest.prototype,

  extraLibraries: MaterialHistoryBrowserTest.prototype.extraLibraries.concat([
    'history_synced_tabs_test.js',
  ]),
};

TEST_F('MaterialHistorySyncedTabsTest', 'All', function() {
  mocha.run();
});

function MaterialHistorySupervisedUserTest() {}

MaterialHistorySupervisedUserTest.prototype = {
  __proto__: MaterialHistoryBrowserTest.prototype,

  typedefCppFixture: 'HistoryUIBrowserTest',

  testGenPreamble: function() {
    GEN('  SetDeleteAllowed(false);');
  },

  extraLibraries: MaterialHistoryBrowserTest.prototype.extraLibraries.concat([
    'history_supervised_user_test.js',
  ]),
};

TEST_F('MaterialHistorySupervisedUserTest', 'All', function() {
  mocha.run();
});

function MaterialHistoryToolbarTest() {}

MaterialHistoryToolbarTest.prototype = {
  __proto__: MaterialHistoryBrowserTest.prototype,

  extraLibraries: MaterialHistoryBrowserTest.prototype.extraLibraries.concat([
    'history_toolbar_test.js',
  ]),
};

TEST_F('MaterialHistoryToolbarTest', 'All', function() {
  mocha.run();
});
