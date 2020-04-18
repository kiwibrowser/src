// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Sets up for directory tree context menu test. In addition to normal setup, we
 * add destination directory.
 */
function setupForDirectoryTreeContextMenuTest() {
  var windowId;
  return setupAndWaitUntilReady(
      null, RootPath.DOWNLOADS).then(function(results) {
    windowId = results.windowId;

    // Add destination directory.
    return new addEntries(['local'], [
      new TestEntryInfo(
          EntryType.DIRECTORY, null, 'destination', null, SharedOption.NONE,
          'Jan 1, 1980, 11:59 PM', 'destination', '--', 'Folder')
    ]);
  }).then(function() {
    return windowId;
  });
}

/**
 * @const
 */
var ITEMS_IN_DEST_DIR_BEFORE_PASTE = TestEntryInfo.getExpectedRows([]);

/**
 * @const
 */
var ITEMS_IN_DEST_DIR_AFTER_PASTE = TestEntryInfo.getExpectedRows([
  new TestEntryInfo(
      EntryType.DIRECTORY, null, 'photos',
      null, SharedOption.NONE, 'Jan 1, 1980, 11:59 PM',
      'photos', '--', 'Folder')
]);

/**
 * Expands tree item.
 */
function expandTreeItemInDirectoryTree(windowId, query) {
  return remoteCall.waitForElement(windowId, query).then(function() {
    return remoteCall.callRemoteTestUtil('queryAllElements', windowId,
        [`${query}[expanded]`]).then(function(elements) {
      // If it's already expanded, do nothing.
      if (elements.length > 0)
        return;

      // Focus to directory tree.
      return remoteCall.callRemoteTestUtil(
          'focus', windowId, ['#directory-tree']).then(function() {
        // Expand download volume.
        return remoteCall.callRemoteTestUtil('fakeMouseClick', windowId,
            [`${query} .expand-icon`]);
      });
    });
  });
}

/**
 * Expands download volume in directory tree.
 */
function expandDownloadVolumeInDirectoryTree(windowId) {
  return expandTreeItemInDirectoryTree(
      windowId, '[volume-type-for-testing="downloads"]');
}

/**
 * Expands directory tree for specified path.
 * TODO(yawano): Move this to remote_call.js
 */
function expandDirectoryTreeFor(windowId, path) {
  return expandDirectoryTreeForInternal_(windowId, path.split('/'), 0);
}

/**
 * Internal function for expanding directory tree for specified path.
 */
function expandDirectoryTreeForInternal_(windowId, components, index) {
  if (index >= components.length - 1)
    return Promise.resolve();

  if (index === 0) {
    return expandDownloadVolumeInDirectoryTree(windowId).then(function() {
      return expandDirectoryTreeForInternal_(windowId, components, index + 1);
    });
  }

  var path = `/${components.slice(1, index + 1).join('/')}`;
  return expandTreeItemInDirectoryTree(
      windowId, `[full-path-for-testing="${path}"]`).then(function() {
    return expandDirectoryTreeForInternal_(windowId, components, index + 1);
  });
}

/**
 * Navigates to specified directory on Download volume by using directory tree.
 */
function navigateWithDirectoryTree(windowId, path) {
  return expandDirectoryTreeFor(windowId, path).then(function() {
    // Select target path.
    return remoteCall.callRemoteTestUtil('fakeMouseClick', windowId,
        [`[full-path-for-testing="${path}"]`]);
  }).then(function() {
    // Wait until the Files app is navigated to the path.
    return remoteCall.waitUntilCurrentDirectoryIsChanged(
        windowId, `/Downloads${path}`);
  });
}

/**
 * Clicks context menu item of id in directory tree.
 */
function clickDirectoryTreeContextMenuItem(windowId, path, id) {
  return remoteCall.callRemoteTestUtil('focus', windowId,
      [`[full-path-for-testing="${path}"]`]).then(function() {
    // Right click photos directory.
    return remoteCall.callRemoteTestUtil('fakeMouseRightClick', windowId,
        [`[full-path-for-testing="${path}"]`]);
  }).then(function() {
    // Wait for context menu.
    return remoteCall.waitForElement(windowId,
        `#directory-tree-context-menu > [command="#${id}"]:not([disabled])`);
  }).then(function() {
    // Click menu item.
    return remoteCall.callRemoteTestUtil('fakeMouseClick', windowId,
        [`#directory-tree-context-menu > [command="#${id}"]`]);
  });
}

/**
 * Navigates to destination directory and test paste operation to check whether
 * the paste operation is done correctly or not. This method does NOT check
 * source entry is deleted or not for cut operation.
 */
function navigateToDestinationDirectoryAndTestPaste(windowId) {
  // Navigates to destination directory.
  return navigateWithDirectoryTree(windowId, '/destination').then(function() {
    // Confirm files before paste.
    return remoteCall.waitForFiles(windowId, ITEMS_IN_DEST_DIR_BEFORE_PASTE,
        {ignoreLastModifiedTime: true});
  }).then(function() {
    // Paste
    return remoteCall.callRemoteTestUtil('fakeKeyDown', windowId,
        ['body', 'v', 'U+0056' /* v */, true /* ctrl */, false, false]);
  }).then(function() {
    // Confirm the photos directory is pasted correctly.
    return remoteCall.waitForFiles(windowId, ITEMS_IN_DEST_DIR_AFTER_PASTE,
        {ignoreLastModifiedTime: true});
  });
}

/**
 * Rename photos directory to specified name by using directory tree.
 */
function renamePhotosDirectoryTo(windowId, newName, useKeyboardShortcut) {
  return (useKeyboardShortcut ?
      remoteCall.callRemoteTestUtil(
          'fakeKeyDown', windowId,
          ['body', 'Enter', 'Enter', true /* ctrl */, false, false]) :
      clickDirectoryTreeContextMenuItem(windowId, '/photos', 'rename')
      ).then(function() {
    return remoteCall.waitForElement(windowId, '.tree-row > input');
  }).then(function() {
    return remoteCall.callRemoteTestUtil(
        'inputText', windowId, ['.tree-row > input', newName]);
  }).then(function() {
    return remoteCall.callRemoteTestUtil(
        'fakeKeyDown', windowId,
        ['.tree-row > input', 'Enter', 'Enter', false, false, false]);
  });
}

/**
 * Renames directory and confirm current directory is moved to the renamed
 * directory.
 */
function renameDirectoryFromDirectoryTreeSuccessCase(useKeyboardShortcut) {
  var windowId;
  return setupForDirectoryTreeContextMenuTest().then(function(id) {
    windowId = id;
    return navigateWithDirectoryTree(windowId, '/photos');
  }).then(function() {
    return renamePhotosDirectoryTo(windowId, 'New photos', useKeyboardShortcut);
  }).then(function() {
    // Confirm that current directory has moved to new folder.
    return remoteCall.waitUntilCurrentDirectoryIsChanged(
        windowId, '/Downloads/New photos');
  });
}

/**
 * Renames directory and confirms that an alert dialog is shown.
 */
function renameDirectoryFromDirectoryTreeAndConfirmAlertDialog(newName) {
  var windowId;
  return setupForDirectoryTreeContextMenuTest().then(function(id) {
    windowId = id;
    return navigateWithDirectoryTree(windowId, '/photos');
  }).then(function() {
    return renamePhotosDirectoryTo(windowId, newName, false);
  }).then(function() {
    // Confirm that a dialog is shown.
    return remoteCall.waitForElement(windowId, '.cr-dialog-container.shown');
  });
}

/**
 * Creates directory from directory tree.
 */
function createDirectoryFromDirectoryTree(
    useKeyboardShortcut, changeCurrentDirectory) {
  var windowId;
  return setupForDirectoryTreeContextMenuTest().then(function(id) {
    windowId = id;

    if (changeCurrentDirectory)
      return navigateWithDirectoryTree(windowId, '/photos');
    else
      return expandDownloadVolumeInDirectoryTree(windowId);
  }).then(function() {
    if (useKeyboardShortcut) {
      return remoteCall.callRemoteTestUtil('fakeKeyDown', windowId,
          ['body', 'e', 'U+0045' /* e */, true /* ctrl */, false, false]);
    } else {
      return clickDirectoryTreeContextMenuItem(
          windowId, '/photos', 'new-folder');
    }
  }).then(function() {
    return remoteCall.waitForElement(windowId, '.tree-row > input');
  }).then(function() {
    return remoteCall.callRemoteTestUtil(
        'inputText', windowId, ['.tree-row > input', 'test']);
  }).then(function() {
    return remoteCall.callRemoteTestUtil(
        'fakeKeyDown', windowId,
        ['.tree-row > input', 'Enter', 'Enter', false, false, false]);
  }).then(function() {
    // Confirm that new directory is added to the directory tree.
    return remoteCall.waitForElement(
        windowId, '[full-path-for-testing="/photos/test"]');
  }).then(function() {
    // Confirm that current directory is not changed at this timing.
    return remoteCall.waitUntilCurrentDirectoryIsChanged(
        windowId, changeCurrentDirectory ? '/Downloads/photos' : '/Downloads');
  }).then(function() {
    // Confirm that new directory is actually created by navigating to it.
    return navigateWithDirectoryTree(windowId, '/photos/test');
  });
}

/**
 * Tests copying a directory from directory tree with context menu.
 */
testcase.dirCopyWithContextMenu = function() {
  var windowId;
  testPromise(setupForDirectoryTreeContextMenuTest().then(function(id) {
    windowId = id;
    return navigateWithDirectoryTree(windowId, '/photos');
  }).then(function() {
    return clickDirectoryTreeContextMenuItem(windowId, '/photos', 'copy');
  }).then(function() {
    return navigateToDestinationDirectoryAndTestPaste(windowId);
  }));
};

/**
 * Tests copying a directory from directory tree with the keyboard shortcut.
 */
testcase.dirCopyWithKeyboard = function() {
  var windowId;
  testPromise(setupForDirectoryTreeContextMenuTest().then(function(id) {
    windowId = id;
    return navigateWithDirectoryTree(windowId, '/photos');
  }).then(function() {
    // Press Ctrl+C.
    return remoteCall.callRemoteTestUtil('fakeKeyDown', windowId,
        ['body', 'c', 'U+0043' /* c */, true /* ctrl */, false, false]);
  }).then(function() {
    return navigateToDestinationDirectoryAndTestPaste(windowId);
  }));
};

/**
 * Tests copying a directory without changing the current directory.
 */
testcase.dirCopyWithoutChangingCurrent = function() {
  var windowId;
  testPromise(setupForDirectoryTreeContextMenuTest().then(function(id) {
    windowId = id;
    return expandDownloadVolumeInDirectoryTree(windowId);
  }).then(function() {
    return clickDirectoryTreeContextMenuItem(windowId, '/photos', 'copy');
  }).then(function() {
    return navigateToDestinationDirectoryAndTestPaste(windowId);
  }));
};

/**
 * Tests cutting a directory with the context menu.
 */
testcase.dirCutWithContextMenu = function() {
  var windowId;
  testPromise(setupForDirectoryTreeContextMenuTest().then(function(id) {
    windowId = id;
    return navigateWithDirectoryTree(windowId, '/photos');
  }).then(function() {
    return clickDirectoryTreeContextMenuItem(windowId, '/photos', 'cut');
  }).then(function() {
    return navigateToDestinationDirectoryAndTestPaste(windowId);
  }).then(function() {
    // Confirm that directory tree is updated.
    return remoteCall.waitForElementLost(
        windowId, '[full-path-for-testing="/photos"]');
  }));
};

/**
 * Tests cutting a directory with the keyboard shortcut.
 */
testcase.dirCutWithKeyboard = function() {
  var windowId;
  testPromise(setupForDirectoryTreeContextMenuTest().then(function(id) {
    windowId = id;
    return navigateWithDirectoryTree(windowId, '/photos');
  }).then(function() {
    // Press Ctrl+X.
    return remoteCall.callRemoteTestUtil('fakeKeyDown', windowId,
        ['body', 'x', 'U+0058' /* x */, true /* ctrl */, false, false]);
  }).then(function() {
    return navigateToDestinationDirectoryAndTestPaste(windowId);
  }).then(function() {
     // Confirm that directory tree is updated.
    return remoteCall.waitForElementLost(
        windowId, '[full-path-for-testing="/photos"]');
  }));
};

/**
 * Tests cutting a directory without changing the current directory.
 */
testcase.dirCutWithoutChangingCurrent = function() {
  var windowId;
  testPromise(setupForDirectoryTreeContextMenuTest().then(function(id) {
    windowId = id;
    return expandDownloadVolumeInDirectoryTree(windowId);
  }).then(function() {
    return clickDirectoryTreeContextMenuItem(windowId, '/photos', 'cut');
  }).then(function() {
    return navigateToDestinationDirectoryAndTestPaste(windowId);
  }).then(function() {
    return remoteCall.waitForElementLost(
        windowId, '[full-path-for-testing="/photos"]');
  }));
};

/**
 * Tests pasting into folder with the context menu.
 */
testcase.dirPasteWithContextMenu = function() {
  var windowId;
  testPromise(setupForDirectoryTreeContextMenuTest().then(function(id) {
    // Copy photos directory as a test data.
    windowId = id;
    return navigateWithDirectoryTree(windowId, '/photos');
  }).then(function() {
    return remoteCall.callRemoteTestUtil('fakeKeyDown', windowId,
        ['body', 'c', 'U+0043' /* c */, true /* ctrl */, false, false]);
  }).then(function() {
    return navigateWithDirectoryTree(windowId, '/destination');
  }).then(function() {
    // Confirm files before paste.
    return remoteCall.waitForFiles(windowId, ITEMS_IN_DEST_DIR_BEFORE_PASTE,
        {ignoreLastModifiedTime: true});
  }).then(function() {
    return clickDirectoryTreeContextMenuItem(
        windowId, '/destination', 'paste-into-folder');
  }).then(function() {
    // Confirm the photos directory is pasted correctly.
    return remoteCall.waitForFiles(windowId, ITEMS_IN_DEST_DIR_AFTER_PASTE,
        {ignoreLastModifiedTime: true});
  }).then(function() {
    // Expand the directory tree.
    return remoteCall.waitForElement(windowId,
        '[full-path-for-testing="/destination"] .expand-icon');
  }).then(function() {
    return remoteCall.callRemoteTestUtil('fakeMouseClick', windowId,
        ['[full-path-for-testing="/destination"] .expand-icon']);
  }).then(function() {
    // Confirm the copied directory is added to the directory tree.
    return remoteCall.waitForElement(windowId,
        '[full-path-for-testing="/destination/photos"]');
  }));
};

/**
 * Tests pasting into a folder without changing the current directory.
 */
testcase.dirPasteWithoutChangingCurrent = function() {
  var windowId;
  testPromise(setupForDirectoryTreeContextMenuTest().then(function(id) {
    windowId = id;
    return expandDownloadVolumeInDirectoryTree(windowId);
  }).then(function() {
    return clickDirectoryTreeContextMenuItem(windowId, '/photos', 'copy');
  }).then(function() {
    return clickDirectoryTreeContextMenuItem(
        windowId, '/destination', 'paste-into-folder');
  }).then(function() {
    return remoteCall.waitForElement(windowId,
        '[full-path-for-testing="/destination"][may-have-children]');
  }).then(function() {
    return remoteCall.callRemoteTestUtil('fakeMouseClick', windowId,
        ['[full-path-for-testing="/destination"] .expand-icon']);
  }).then(function() {
    // Confirm the copied directory is added to the directory tree.
    return remoteCall.waitForElement(windowId,
        '[full-path-for-testing="/destination/photos"]');
  }));
};

/**
 * Tests renaming a folder with the context menu.
 */
testcase.dirRenameWithContextMenu = function() {
  testPromise(renameDirectoryFromDirectoryTreeSuccessCase(
      false /* do not use keyboard shortcut */));
};

/**
 * Tests renaming folder with the keyboard shortcut.
 */
testcase.dirRenameWithKeyboard = function() {
  testPromise(renameDirectoryFromDirectoryTreeSuccessCase(
      true /* use keyboard shortcut */));
};

/**
 * Tests renaming folder without changing the current directory.
 */
testcase.dirRenameWithoutChangingCurrent = function() {
  var windowId;
  testPromise(setupForDirectoryTreeContextMenuTest().then(function(id) {
    windowId = id;
    return expandDownloadVolumeInDirectoryTree(windowId);
  }).then(function() {
    return remoteCall.waitForElement(
        windowId, '[full-path-for-testing="/photos"]');
  }).then(function() {
    return renamePhotosDirectoryTo(
        windowId, 'New photos', false /* Do not use keyboard shortcut. */);
  }).then(function() {
    return remoteCall.waitForElementLost(
        windowId, '[full-path-for-testing="/photos"]');
  }).then(function() {
    return remoteCall.waitForElement(
        windowId, '[full-path-for-testing="/New photos"]');
  }));
};

/**
 * Tests renaming a folder to an empty string.
 */
testcase.dirRenameToEmptyString = function() {
  testPromise(renameDirectoryFromDirectoryTreeAndConfirmAlertDialog(''));
};

/**
 * Tests renaming folder an existing name.
 */
testcase.dirRenameToExisting = function() {
  testPromise(renameDirectoryFromDirectoryTreeAndConfirmAlertDialog(
      'destination'));
};

/**
 * Tests creating a folder with the context menu.
 */
testcase.dirCreateWithContextMenu = function() {
  testPromise(createDirectoryFromDirectoryTree(
      false /* do not use keyboard shortcut */,
      true /* change current directory */));
};

/**
 * Tests creating a folder with the keyboard shortcut.
 */
testcase.dirCreateWithKeyboard = function() {
  testPromise(createDirectoryFromDirectoryTree(
      true /* use keyboard shortcut */,
      true /* change current directory */));
};

/**
 * Tests creating folder without changing the current directory.
 */
testcase.dirCreateWithoutChangingCurrent = function() {
  testPromise(createDirectoryFromDirectoryTree(
      false /* Do not use keyboard shortcut */,
      false /* Do not change current directory */));
};
