// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Creates and returns a CommandManager which tracks what commands are executed.
 * @constructor
 * @extends {bookmarks.CommandManager}
 */
function TestCommandManager() {
  const commandManager = document.createElement('bookmarks-command-manager');
  let lastCommand = null;
  let lastCommandIds = null;

  const realHandle = commandManager.handle.bind(commandManager);
  commandManager.handle = function(command, itemIds) {
    lastCommand = command;
    lastCommandIds = itemIds;
    realHandle(command, itemIds);
  };

  /**
   * @param {Command} command
   * @param {!Array<string>} ids
   */
  commandManager.assertLastCommand = function(command, ids) {
    assertEquals(command, lastCommand);
    if (ids)
      assertDeepEquals(ids, normalizeIterable(lastCommandIds));
    lastCommand = null;
    lastCommandIds = null;
  };

  /** @param {!Array<string>} ids */
  commandManager.assertMenuOpenForIds = function(ids) {
    assertDeepEquals(ids, normalizeIterable(commandManager.menuIds_));
  };

  return commandManager;
}
