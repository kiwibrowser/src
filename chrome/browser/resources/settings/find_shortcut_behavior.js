// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Listens for a find keyboard shortcut (i.e. Ctrl/Cmd+f) wherever
 * this behavior is applied and invokes canHandleFindShortcut(). If
 * canHandleFindShortcut() returns true, handleFindShortcut() will be called.
 * Override these methods in your element in order to use this behavior.
 */

cr.exportPath('settings');

/** @polymerBehavior */
settings.FindShortcutBehaviorImpl = {
  keyBindings: {
    // <if expr="is_macosx">
    'meta+f': 'onFindShortcut_',
    // </if>
    // <if expr="not is_macosx">
    'ctrl+f': 'onFindShortcut_',
    // </if>
  },

  /** @private */
  onFindShortcut_: function(e) {
    if (!e.defaultPrevented && this.canHandleFindShortcut()) {
      this.handleFindShortcut();
      e.preventDefault();
    }
  },

  /**
   * @return {boolean}
   * @protected
   */
  canHandleFindShortcut: function() {
    assertNotReached();
  },

  /** @protected */
  handleFindShortcut: function() {
    assertNotReached();
  },
};

/** @polymerBehavior */
settings.FindShortcutBehavior = [
  Polymer.IronA11yKeysBehavior,
  settings.FindShortcutBehaviorImpl,
];
