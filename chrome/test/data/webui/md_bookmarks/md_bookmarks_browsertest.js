// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Test suite for the Material Design bookmarks page.
 */
const ROOT_PATH = '../../../../../';

GEN_INCLUDE(
    [ROOT_PATH + 'chrome/test/data/webui/polymer_browser_test_base.js']);
GEN('#include "chrome/browser/prefs/incognito_mode_prefs.h"');
GEN('#include "chrome/browser/ui/webui/md_bookmarks/md_bookmarks_browsertest.h"');

function MaterialBookmarksBrowserTest() {}

MaterialBookmarksBrowserTest.prototype = {
  __proto__: PolymerTest.prototype,

  browsePreload: 'chrome://bookmarks',

  typedefCppFixture: 'MdBookmarksBrowserTest',

  extraLibraries: PolymerTest.getLibraries(ROOT_PATH).concat([
    'test_command_manager.js',
    'test_store.js',
    'test_timer_proxy.js',
    'test_util.js',
  ]),

  /** override */
  runAccessibilityChecks: true,
};

function MaterialBookmarksActionsTest() {}

MaterialBookmarksActionsTest.prototype = {
  __proto__: MaterialBookmarksBrowserTest.prototype,

  extraLibraries: MaterialBookmarksBrowserTest.prototype.extraLibraries.concat([
    'actions_test.js',
  ]),
};

TEST_F('MaterialBookmarksActionsTest', 'All', function() {
  mocha.run();
});

function MaterialBookmarksAppTest() {}

MaterialBookmarksAppTest.prototype = {
  __proto__: MaterialBookmarksBrowserTest.prototype,

  extraLibraries: MaterialBookmarksBrowserTest.prototype.extraLibraries.concat([
    'app_test.js',
  ]),
};

TEST_F('MaterialBookmarksAppTest', 'All', function() {
  mocha.run();
});

function MaterialBookmarksCommandManagerTest() {}

MaterialBookmarksCommandManagerTest.prototype = {
  __proto__: MaterialBookmarksBrowserTest.prototype,

  extraLibraries: MaterialBookmarksBrowserTest.prototype.extraLibraries.concat([
    'command_manager_test.js',
  ]),
};

TEST_F('MaterialBookmarksCommandManagerTest', 'All', function() {
  mocha.run();
});

function MaterialBookmarksDNDManagerTest() {}

MaterialBookmarksDNDManagerTest.prototype = {
  __proto__: MaterialBookmarksBrowserTest.prototype,

  extraLibraries: MaterialBookmarksBrowserTest.prototype.extraLibraries.concat([
    'dnd_manager_test.js',
  ]),
};

// http://crbug.com/803570 : Flaky on Win 7 (dbg)
GEN('#if defined(OS_WIN) && !defined(NDEBUG)');
GEN('#define MAYBE_All DISABLED_All');
GEN('#else');
GEN('#define MAYBE_All All');
GEN('#endif');

TEST_F('MaterialBookmarksDNDManagerTest', 'MAYBE_All', function() {
  mocha.run();
});

function MaterialBookmarksEditDialogTest() {}

MaterialBookmarksEditDialogTest.prototype = {
  __proto__: MaterialBookmarksBrowserTest.prototype,

  extraLibraries: MaterialBookmarksBrowserTest.prototype.extraLibraries.concat([
    'edit_dialog_test.js',
  ]),
};

TEST_F('MaterialBookmarksEditDialogTest', 'All', function() {
  mocha.run();
});

function MaterialBookmarksItemTest() {}

MaterialBookmarksItemTest.prototype = {
  __proto__: MaterialBookmarksBrowserTest.prototype,

  extraLibraries: MaterialBookmarksBrowserTest.prototype.extraLibraries.concat([
    'item_test.js',
  ]),
};

TEST_F('MaterialBookmarksItemTest', 'All', function() {
  mocha.run();
});

function MaterialBookmarksListTest() {}

MaterialBookmarksListTest.prototype = {
  __proto__: MaterialBookmarksBrowserTest.prototype,

  extraLibraries: MaterialBookmarksBrowserTest.prototype.extraLibraries.concat([
    'list_test.js',
  ]),
};

TEST_F('MaterialBookmarksListTest', 'All', function() {
  mocha.run();
});

function MaterialBookmarksReducersTest() {}

MaterialBookmarksReducersTest.prototype = {
  __proto__: MaterialBookmarksBrowserTest.prototype,

  extraLibraries: MaterialBookmarksBrowserTest.prototype.extraLibraries.concat([
    'reducers_test.js',
  ]),
};

TEST_F('MaterialBookmarksReducersTest', 'All', function() {
  mocha.run();
});

function MaterialBookmarksRouterTest() {}

MaterialBookmarksRouterTest.prototype = {
  __proto__: MaterialBookmarksBrowserTest.prototype,

  extraLibraries: MaterialBookmarksBrowserTest.prototype.extraLibraries.concat([
    'router_test.js',
  ]),
};

TEST_F('MaterialBookmarksRouterTest', 'All', function() {
  mocha.run();
});

function MaterialBookmarksFolderNodeTest() {}

MaterialBookmarksFolderNodeTest.prototype = {
  __proto__: MaterialBookmarksBrowserTest.prototype,

  extraLibraries: MaterialBookmarksBrowserTest.prototype.extraLibraries.concat([
    'folder_node_test.js',
  ]),
};

TEST_F('MaterialBookmarksFolderNodeTest', 'All', function() {
  mocha.run();
});

function MaterialBookmarksToastManagerTest() {}

MaterialBookmarksToastManagerTest.prototype = {
  __proto__: MaterialBookmarksBrowserTest.prototype,

  extraLibraries: MaterialBookmarksBrowserTest.prototype.extraLibraries.concat([
    'toast_manager_test.js',
  ]),
};

TEST_F('MaterialBookmarksToastManagerTest', 'All', function() {
  mocha.run();
});

function MaterialBookmarksPolicyTest() {}

MaterialBookmarksPolicyTest.prototype = {
  __proto__: MaterialBookmarksBrowserTest.prototype,

  testGenPreamble: function() {
    GEN('SetIncognitoAvailability(IncognitoModePrefs::DISABLED);');
    GEN('SetCanEditBookmarks(false);');
  },

  extraLibraries: MaterialBookmarksBrowserTest.prototype.extraLibraries.concat([
    'policy_test.js',
  ]),
};

TEST_F('MaterialBookmarksPolicyTest', 'All', function() {
  mocha.run();
});

function MaterialBookmarksStoreTest() {}

MaterialBookmarksStoreTest.prototype = {
  __proto__: MaterialBookmarksBrowserTest.prototype,

  extraLibraries: MaterialBookmarksBrowserTest.prototype.extraLibraries.concat([
    'store_test.js',
  ]),
};

TEST_F('MaterialBookmarksStoreTest', 'All', function() {
  mocha.run();
});

function MaterialBookmarksToolbarTest() {}

MaterialBookmarksToolbarTest.prototype = {
  __proto__: MaterialBookmarksBrowserTest.prototype,

  extraLibraries: MaterialBookmarksBrowserTest.prototype.extraLibraries.concat([
    'toolbar_test.js',
  ]),
};

TEST_F('MaterialBookmarksToolbarTest', 'All', function() {
  mocha.run();
});

function MaterialBookmarksUtilTest() {}

MaterialBookmarksUtilTest.prototype = {
  __proto__: MaterialBookmarksBrowserTest.prototype,

  extraLibraries: MaterialBookmarksBrowserTest.prototype.extraLibraries.concat([
    'util_test.js',
  ]),
};

TEST_F('MaterialBookmarksUtilTest', 'All', function() {
  mocha.run();
});
