// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * Constants for selectors.
 */
var TREEITEM_DRIVE = '#directory-tree > div:nth-child(1) > .tree-children ' +
    '> div:nth-child(1) ';
var TREEITEM_A = TREEITEM_DRIVE + '> .tree-children > div:nth-child(1) ';
var TREEITEM_B = TREEITEM_A + '> .tree-children > div:nth-child(1) ';
var TREEITEM_C = TREEITEM_B + '> .tree-children > div:nth-child(1) ';
var TREEITEM_D = TREEITEM_DRIVE + '> .tree-children > div:nth-child(2) ';
var TREEITEM_E = TREEITEM_D + '> .tree-children > div:nth-child(1) ';
var EXPAND_ICON = '> .tree-row > .expand-icon';
var ITEM_ICON = '> .tree-row > .item-icon';
var EXPANDED_SUBTREE = '> .tree-children[expanded]';

/**
 * Entry set which is used for this test.
 * @type {Array<TestEntryInfo>}
 * @const
 */
var ENTRY_SET = [
  ENTRIES.directoryA,
  ENTRIES.directoryB,
  ENTRIES.directoryC,
  ENTRIES.directoryD,
  ENTRIES.directoryE,
  ENTRIES.directoryF
];

/**
 * Constants for each folders.
 * @type {Array<Object>}
 * @const
 */
var DIRECTORY = {
  Drive: {
    contents: [
      ENTRIES.directoryA.getExpectedRow(), ENTRIES.directoryD.getExpectedRow()
    ],
    name: 'Drive',
    navItem: '#tree-item-autogen-id-4',
    treeItem: TREEITEM_DRIVE
  },
  A: {
    contents: [ENTRIES.directoryB.getExpectedRow()],
    name: 'A',
    navItem: '#tree-item-autogen-id-14',
    treeItem: TREEITEM_A
  },
  B: {
    contents: [ENTRIES.directoryC.getExpectedRow()],
    name: 'B',
    treeItem: TREEITEM_B
  },
  C: {
    contents: [],
    name: 'C',
    navItem: '#tree-item-autogen-id-14',
    treeItem: TREEITEM_C
  },
  D: {
    contents: [ENTRIES.directoryE.getExpectedRow()],
    name: 'D',
    navItem: '#tree-item-autogen-id-13',
    treeItem: TREEITEM_D
  },
  E: {
    contents: [ENTRIES.directoryF.getExpectedRow()],
    name: 'E',
    treeItem: TREEITEM_E
  }
};

/**
 * Opens two file manager windows.
 * @return {Promise} Promise fulfilled with an array containing two window IDs.
 */
function openWindows() {
  return Promise.all([
    openNewWindow(null, RootPath.DRIVE),
    openNewWindow(null, RootPath.DRIVE)
  ]).then(function(windowIds) {
    return Promise.all([
      remoteCall.waitForElement(windowIds[0], '#detail-table'),
      remoteCall.waitForElement(windowIds[1], '#detail-table')
    ]).then(function() {
      return windowIds;
    });
  });
}

/**
 * Expands tree item on the directory tree by clicking expand icon.
 * @param {string} windowId ID of target window.
 * @param {Object} directory Directory whose tree item should be expanded.
 * @return {Promise} Promise fulfilled on success.
 */
function expandTreeItem(windowId, directory) {
  return remoteCall.waitForElement(
      windowId, directory.treeItem + EXPAND_ICON).then(function() {
    return remoteCall.callRemoteTestUtil(
        'fakeMouseClick', windowId, [directory.treeItem + EXPAND_ICON]);
  }).then(function(result) {
    chrome.test.assertTrue(result);
    return remoteCall.waitForElement(windowId,
                                     directory.treeItem + EXPANDED_SUBTREE);
  });
}

/**
 * Expands whole directory tree.
 * @param {string} windowId ID of target window.
 * @return {Promise} Promise fulfilled on success.
 */
function expandDirectoryTree(windowId) {
  return expandTreeItem(windowId, DIRECTORY.Drive).then(function() {
    return expandTreeItem(windowId, DIRECTORY.A);
  }).then(function() {
    return expandTreeItem(windowId, DIRECTORY.B);
  }).then(function() {
    return expandTreeItem(windowId, DIRECTORY.D);
  });
}

/**
 * Makes |directory| the current directory.
 * @param {string} windowId ID of target window.
 * @param {Object} directory Directory which should be a current directory.
 * @return {Promise} Promise fulfilled on success.
 */
function navigateToDirectory(windowId, directory) {
  return remoteCall.waitForElement(
      windowId, directory.treeItem + ITEM_ICON).then(function() {
    return remoteCall.callRemoteTestUtil(
        'fakeMouseClick', windowId, [directory.treeItem + ITEM_ICON]);
  }).then(function(result) {
    chrome.test.assertTrue(result);
    return remoteCall.waitForFiles(windowId, directory.contents);
  });
}

/**
 * Creates folder shortcut to |directory|.
 * The current directory must be a parent of the |directory|.
 * @param {string} windowId ID of target window.
 * @param {Object} directory Directory of shortcut to be created.
 * @return {Promise} Promise fulfilled on success.
 */
function createShortcut(windowId, directory) {
  return remoteCall.callRemoteTestUtil(
      'selectFile', windowId, [directory.name]).then(function(result) {
    chrome.test.assertTrue(result);
    return remoteCall.waitForElement(windowId, ['.table-row[selected]']);
  }).then(function() {
    return remoteCall.callRemoteTestUtil(
        'fakeMouseRightClick', windowId, ['.table-row[selected]']);
  }).then(function(result) {
    chrome.test.assertTrue(result);
    return remoteCall.waitForElement(
        windowId, '#file-context-menu:not([hidden])');
  }).then(function() {
    return remoteCall.waitForElement(
        windowId,
        '[command="#create-folder-shortcut"]:not([hidden]):not([disabled])');
  }).then(function() {
    return remoteCall.callRemoteTestUtil(
        'fakeMouseClick', windowId,
        ['[command="#create-folder-shortcut"]:not([hidden]):not([disabled])']);
  }).then(function(result) {
    chrome.test.assertTrue(result);
    return remoteCall.waitForElement(windowId, directory.navItem);
  });
}

/**
 * Removes folder shortcut to |directory|.
 * The current directory must be a parent of the |directory|.
 * @param {string} windowId ID of target window.
 * @param {Object} directory Directory of shortcut ot be removed.
 * @return {Promise} Promise fullfilled on success.
 */
function removeShortcut(windowId, directory) {
  // Focus the item first since actions are calculated asynchronously. The
  // context menu wouldn't show if there are no visible items. Focusing first,
  // will force the actions controller to refresh actions.
  // TODO(mtomasz): Remove this hack (if possible).
  return remoteCall.callRemoteTestUtil('focus',
      windowId, [directory.navItem]).then(function(result) {
    chrome.test.assertTrue(result);
    return remoteCall.callRemoteTestUtil(
      'fakeMouseRightClick',
      windowId,
      [directory.navItem]);
  }).then(function(result) {
    chrome.test.assertTrue(result);
    return remoteCall.waitForElement(
        windowId, '#roots-context-menu:not([hidden])');
  }).then(function() {
    return remoteCall.waitForElement(
        windowId,
        '[command="#remove-folder-shortcut"]:not([hidden]):not([disabled])');
  }).then(function() {
    return remoteCall.callRemoteTestUtil(
        'fakeMouseClick', windowId,
        ['#roots-context-menu [command="#remove-folder-shortcut"]:' +
         'not([hidden])']);
  }).then(function(result) {
    chrome.test.assertTrue(result);
    return remoteCall.waitForElementLost(windowId, directory.navItem);
  });
}

/**
 * Waits until the current directory become |currentDir| and folder shortcut to
 * |shortcutDir| is selected.
 * @param {string} windowId ID of target window.
 * @param {Object} currentDir Directory which should be a current directory.
 * @param {Object} shortcutDir Directory whose shortcut should be selected.
 * @return {Promise} Promise fullfilled on success.
 */
function expectSelection(windowId, currentDir, shortcutDir) {
  return remoteCall.waitForFiles(windowId, currentDir.contents).
      then(function() {
        return remoteCall.waitForElement(
            windowId, shortcutDir.navItem + '[selected]');
      });
}

/**
 * Clicks folder shortcut to |directory|.
 * @param {string} windowId ID of target window.
 * @param {Object} directory Directory whose shortcut will be clicked.
 * @return {Promise} Promise fullfilled with result of fakeMouseClick.
 */
function clickShortcut(windowId, directory) {
  return remoteCall.waitForElement(windowId, directory.navItem).
    then(function() {
      return remoteCall.callRemoteTestUtil(
          'fakeMouseClick', windowId, [directory.navItem]);
    });
}

/**
 * Creates some shortcuts and traverse them and some other directories.
 */
testcase.traverseFolderShortcuts = function() {
  var windowId;
  StepsRunner.run([
    // Set up each window.
    function() {
      addEntries(['drive'], ENTRY_SET, this.next);
    },
    function(result) {
      chrome.test.assertTrue(result);
      openNewWindow(null, RootPath.DRIVE).then(this.next);
    },
    function(inWindowId) {
      windowId = inWindowId;
      remoteCall.waitForElement(windowId, '#detail-table').then(this.next);
    },
    function() {
      expandDirectoryTree(windowId).then(this.next);
    },
    function() {
      remoteCall.waitForFiles(windowId, DIRECTORY.Drive.contents).
          then(this.next);
    },

    // Create shortcut to D
    function() {
      createShortcut(windowId, DIRECTORY.D).then(this.next);
    },

    // Create sortcut to C
    function() {
      navigateToDirectory(windowId, DIRECTORY.B).then(this.next);
    },
    function() {
      createShortcut(windowId, DIRECTORY.C).then(this.next);
    },

    // Click shortcut to drive.
    // Current directory should be Drive root.
    // Shortcut to Drive root should be selected.
    function() {
      clickShortcut(windowId, DIRECTORY.Drive).then(this.next);
    },
    function() {
      expectSelection(
          windowId, DIRECTORY.Drive, DIRECTORY.Drive).then(this.next);
    },

    // Press Ctrl+5 to select 5th shortcut.
    // Current directory should be D.
    // Shortcut to C should be selected.
    function() {
      remoteCall.callRemoteTestUtil(
          'fakeKeyDown', windowId,
          ['#file-list', '5', 'U+0034', true, false, false], this.next);
    },
    function(result) {
      chrome.test.assertTrue(result);
      expectSelection(windowId, DIRECTORY.D, DIRECTORY.D).then(this.next);
    },

    // Press UP to select 4th shortcut.
    // Current directory should remain D.
    // Shortcut to C should be selected.
    function() {
      remoteCall.callRemoteTestUtil('fakeKeyDown', windowId,
          ['#directory-tree', 'ArrowUp', 'Up', false, false, false], this.next);
    },
    function(result) {
      chrome.test.assertTrue(result);
      expectSelection(windowId, DIRECTORY.D, DIRECTORY.C).then(this.next);
    },

    // Press Enter to change the directory to C.
    // Current directory should be C.
    function() {
      remoteCall.callRemoteTestUtil('fakeKeyDown', windowId,
          ['#directory-tree', 'Enter', 'Enter', false, false, false],
           this.next);
    },
    function(result) {
      chrome.test.assertTrue(result);
      expectSelection(windowId, DIRECTORY.C, DIRECTORY.C).then(this.next);
    },

    function() {
      checkIfNoErrorsOccured(this.next);
    }
  ]);
};

/**
 * Adds and removes shortcuts from other window and check if the active
 * directories and selected navigation items are correct.
 */
testcase.addRemoveFolderShortcuts = function() {
  var windowId1;
  var windowId2;
  StepsRunner.run([
    // Set up each window.
    function() {
      addEntries(['drive'], ENTRY_SET, this.next);
    },
    function(result) {
      chrome.test.assertTrue(result);
      openWindows().then(this.next);
    },
    function(windowIds) {
      windowId1 = windowIds[0];
      windowId2 = windowIds[1];
      expandDirectoryTree(windowId1).then(this.next);
    },
    function() {
      expandDirectoryTree(windowId2).then(this.next);
    },
    function() {
      remoteCall.waitForFiles(windowId1, DIRECTORY.Drive.contents).
          then(this.next);
    },
    function() {
      remoteCall.waitForFiles(windowId2, DIRECTORY.Drive.contents).
          then(this.next);
    },

    // Create shortcut to D
    function() {
      createShortcut(windowId1, DIRECTORY.D).then(this.next);
    },

    // Click D.
    // Current directory should be D.
    // Shortcut to D should be selected.
    function() {
      clickShortcut(windowId1, DIRECTORY.D).then(this.next);
    },
    function() {
      expectSelection(windowId1, DIRECTORY.D, DIRECTORY.D).then(this.next);
    },

    // Create shortcut to A in another window.
    function() {
      createShortcut(windowId2, DIRECTORY.A).then(this.next);
    },

    // The index of shortcut to D is changed.
    // Current directory should remain D.
    // Shortcut to D should keep selected.
    function() {
      expectSelection(windowId1, DIRECTORY.D, DIRECTORY.D).then(this.next);
    },

    // Remove shortcut to D in another window.
    function() {
      removeShortcut(windowId2, DIRECTORY.D).then(this.next);
    },

    // Directory D in the directory tree should be selected.
    function() {
      remoteCall.waitForElement(windowId1, TREEITEM_D + '[selected]').
          then(this.next);
    },

    function() {
      checkIfNoErrorsOccured(this.next);
    }
  ]);
};

