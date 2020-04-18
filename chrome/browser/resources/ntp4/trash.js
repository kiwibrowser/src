// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Trash
 * This is the class for the trash can that appears when dragging an app.
 */

cr.define('ntp', function() {
  'use strict';

  /**
   * @constructor
   * @extends {HTMLDivElement}
   * @implements {cr.ui.DragWrapperDelegate}
   */
  function Trash(trash) {
    trash.__proto__ = Trash.prototype;
    trash.initialize();
    return trash;
  }

  Trash.prototype = {
    __proto__: HTMLDivElement.prototype,

    initialize: function(element) {
      this.dragWrapper_ = new cr.ui.DragWrapper(this, this);
    },

    /**
     * Determines whether we are interested in the drag data for |e|.
     * @param {Event} e The event from drag enter.
     * @return {boolean} True if we are interested in the drag data for |e|.
     */
    shouldAcceptDrag: function(e) {
      var tile = ntp.getCurrentlyDraggingTile();
      if (!tile)
        return false;

      return tile.firstChild.canBeRemoved();
    },

    /** @override */
    doDragOver: function(e) {
      ntp.getCurrentlyDraggingTile().dragClone.classList.add(
          'hovering-on-trash');
      ntp.setCurrentDropEffect(e.dataTransfer, 'move');
      e.preventDefault();
    },

    /** @override */
    doDragEnter: function(e) {
      this.doDragOver(e);
    },

    /** @override */
    doDrop: function(e) {
      e.preventDefault();

      var tile = ntp.getCurrentlyDraggingTile();
      tile.firstChild.removeFromChrome();
      tile.landedOnTrash = true;
    },

    /** @override */
    doDragLeave: function(e) {
      ntp.getCurrentlyDraggingTile().dragClone.classList.remove(
          'hovering-on-trash');
    },
  };

  return {
    Trash: Trash,
  };
});
