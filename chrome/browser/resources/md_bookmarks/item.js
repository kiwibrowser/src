// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'bookmarks-item',

  behaviors: [
    bookmarks.StoreClient,
  ],

  properties: {
    itemId: {
      type: String,
      observer: 'onItemIdChanged_',
    },

    ironListTabIndex: String,

    /** @private {BookmarkNode} */
    item_: {
      type: Object,
      observer: 'onItemChanged_',
    },

    /** @private */
    isSelectedItem_: {
      type: Boolean,
      reflectToAttribute: true,
    },

    /** @private */
    isFolder_: Boolean,
  },

  hostAttributes: {
    'role': 'listitem',
  },

  observers: [
    'updateFavicon_(item_.url)',
  ],

  listeners: {
    'click': 'onClick_',
    'dblclick': 'onDblClick_',
    'contextmenu': 'onContextMenu_',
    'keydown': 'onKeydown_',
    'auxclick': 'onMiddleClick_',
    'mousedown': 'cancelMiddleMouseBehavior_',
    'mouseup': 'cancelMiddleMouseBehavior_',
  },

  /** @override */
  attached: function() {
    this.watch('item_', (store) => store.nodes[this.itemId]);
    this.watch(
        'isSelectedItem_', (store) => !!store.selection.items.has(this.itemId));

    this.updateFromStore();
  },

  /** @return {BookmarksItemElement} */
  getDropTarget: function() {
    return this;
  },

  /**
   * @param {Event} e
   * @private
   */
  onContextMenu_: function(e) {
    e.preventDefault();
    e.stopPropagation();
    this.focus();
    if (!this.isSelectedItem_)
      this.selectThisItem_();

    this.fire('open-command-menu', {
      x: e.clientX,
      y: e.clientY,
      source: MenuSource.ITEM,
    });
  },

  /**
   * @param {Event} e
   * @private
   */
  onMenuButtonClick_: function(e) {
    e.stopPropagation();
    e.preventDefault();
    this.selectThisItem_();
    this.fire('open-command-menu', {
      targetElement: e.target,
      source: MenuSource.ITEM,
    });
  },

  /**
   * @param {Event} e
   * @private
   */
  onMenuButtonDblClick_: function(e) {
    e.stopPropagation();
  },

  /** @private */
  selectThisItem_: function() {
    this.dispatch(bookmarks.actions.selectItem(this.itemId, this.getState(), {
      clear: true,
      range: false,
      toggle: false,
    }));
  },

  /** @private */
  onItemIdChanged_: function() {
    // TODO(tsergeant): Add a histogram to measure whether this assertion fails
    // for real users.
    assert(this.getState().nodes[this.itemId]);
    this.updateFromStore();
  },

  /** @private */
  onItemChanged_: function() {
    this.isFolder_ = !this.item_.url;
    this.setAttribute(
        'aria-label',
        this.item_.title || this.item_.url ||
            loadTimeData.getString('folderLabel'));
  },

  /**
   * @param {MouseEvent} e
   * @private
   */
  onClick_: function(e) {
    // Ignore double clicks so that Ctrl double-clicking an item won't deselect
    // the item before opening.
    if (e.detail != 2) {
      const addKey = cr.isMac ? e.metaKey : e.ctrlKey;
      this.dispatch(bookmarks.actions.selectItem(this.itemId, this.getState(), {
        clear: !addKey,
        range: e.shiftKey,
        toggle: addKey && !e.shiftKey,
      }));
    }
    e.stopPropagation();
    e.preventDefault();
  },

  /**
   * @private
   * @param {KeyboardEvent} e
   */
  onKeydown_: function(e) {
    if (e.key == 'ArrowLeft')
      this.focus();
    else if (e.key == 'ArrowRight')
      this.$.menuButton.focus();
  },

  /**
   * @param {MouseEvent} e
   * @private
   */
  onDblClick_: function(e) {
    if (!this.isSelectedItem_)
      this.selectThisItem_();

    const commandManager = bookmarks.CommandManager.getInstance();
    const itemSet = this.getState().selection.items;
    if (commandManager.canExecute(Command.OPEN, itemSet))
      commandManager.handle(Command.OPEN, itemSet);
  },

  /**
   * @param {MouseEvent} e
   * @private
   */
  onMiddleClick_: function(e) {
    if (e.button != 1)
      return;

    this.selectThisItem_();
    if (this.isFolder_)
      return;

    const commandManager = bookmarks.CommandManager.getInstance();
    const itemSet = this.getState().selection.items;
    const command = e.shiftKey ? Command.OPEN : Command.OPEN_NEW_TAB;
    if (commandManager.canExecute(command, itemSet))
      commandManager.handle(command, itemSet);
  },

  /**
   * Prevent default middle-mouse behavior. On Windows, this prevents autoscroll
   * (during mousedown), and on Linux this prevents paste (during mouseup).
   * @param {MouseEvent} e
   * @private
   */
  cancelMiddleMouseBehavior_: function(e) {
    if (e.button == 1)
      e.preventDefault();
  },

  /**
   * @param {string} url
   * @private
   */
  updateFavicon_: function(url) {
    this.$.icon.className = url ? 'website-icon' : 'folder-icon';
    this.$.icon.style.backgroundImage = url ? cr.icon.getFavicon(url) : null;
  },

  /** @private */
  getButtonAriaLabel_: function() {
    return loadTimeData.getStringF(
        'moreActionsButtonAxLabel', this.item_.title);
  }
});
