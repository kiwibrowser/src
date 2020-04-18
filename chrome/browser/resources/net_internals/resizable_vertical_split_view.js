// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * This view implements a vertically split display with a draggable divider.
 *
 *                  <<-- sizer -->>
 *
 *  +----------------------++----------------+
 *  |                      ||                |
 *  |                      ||                |
 *  |                      ||                |
 *  |                      ||                |
 *  |       leftView       ||   rightView    |
 *  |                      ||                |
 *  |                      ||                |
 *  |                      ||                |
 *  |                      ||                |
 *  |                      ||                |
 *  +----------------------++----------------+
 *
 * @param {!View} leftView The widget to position on the left.
 * @param {!View} rightView The widget to position on the right.
 * @param {!DivView} sizerView The widget that will serve as draggable divider.
 */
var ResizableVerticalSplitView = (function() {
  'use strict';

  // Minimum width to size panels to, in pixels.
  var MIN_PANEL_WIDTH = 50;

  // We inherit from View.
  var superClass = View;

  /**
   * @constructor
   */
  function ResizableVerticalSplitView(leftView, rightView, sizerView) {
    // Call superclass's constructor.
    superClass.call(this);

    this.leftView_ = leftView;
    this.rightView_ = rightView;
    this.sizerView_ = sizerView;

    this.mouseDragging_ = false;
    this.touchDragging_ = false;

    // Setup the "sizer" so it can be dragged left/right to reposition the
    // vertical split.  The start event must occur within the sizer's node,
    // but subsequent events may occur anywhere.
    var node = sizerView.getNode();
    node.addEventListener('mousedown', this.onMouseDragSizerStart_.bind(this));
    window.addEventListener('mousemove', this.onMouseDragSizer_.bind(this));
    window.addEventListener('mouseup', this.onMouseDragSizerEnd_.bind(this));

    node.addEventListener('touchstart', this.onTouchDragSizerStart_.bind(this));
    window.addEventListener('touchmove', this.onTouchDragSizer_.bind(this));
    window.addEventListener('touchend', this.onTouchDragSizerEnd_.bind(this));
    window.addEventListener(
        'touchcancel', this.onTouchDragSizerEnd_.bind(this));
  }

  ResizableVerticalSplitView.prototype = {
    // Inherit the superclass's methods.
    __proto__: superClass.prototype,

    /**
     * Sets the width of the left view.
     * @param {Integer} px The number of pixels
     */
    setLeftSplit: function(px) {
      this.leftSplit_ = px;
    },

    /**
     * Repositions all of the elements to fit the window.
     */
    setGeometry: function(left, top, width, height) {
      superClass.prototype.setGeometry.call(this, left, top, width, height);

      // If this is the first setGeometry(), initialize the split point at 50%.
      if (!this.leftSplit_)
        this.leftSplit_ = parseInt((width / 2).toFixed(0));

      // Calculate the horizontal split points.
      var leftboxWidth = this.leftSplit_;
      var sizerWidth = this.sizerView_.getWidth();
      var rightboxWidth = width - (leftboxWidth + sizerWidth);

      // Don't let the right pane get too small.
      if (rightboxWidth < MIN_PANEL_WIDTH) {
        rightboxWidth = MIN_PANEL_WIDTH;
        leftboxWidth = width - (sizerWidth + rightboxWidth);
      }

      // Position the boxes using calculated split points.
      this.leftView_.setGeometry(left, top, leftboxWidth, height);
      this.sizerView_.setGeometry(
          this.leftView_.getRight(), top, sizerWidth, height);
      this.rightView_.setGeometry(
          this.sizerView_.getRight(), top, rightboxWidth, height);
    },

    show: function(isVisible) {
      superClass.prototype.show.call(this, isVisible);
      this.leftView_.show(isVisible);
      this.sizerView_.show(isVisible);
      this.rightView_.show(isVisible);
    },

    /**
     * Called once the sizer is clicked on. Starts moving the sizer in response
     * to future mouse movement.
     */
    onMouseDragSizerStart_: function(event) {
      this.mouseDragging_ = true;
      event.preventDefault();
    },

    /**
     * Called when the mouse has moved.
     */
    onMouseDragSizer_: function(event) {
      if (!this.mouseDragging_)
        return;
      // If dragging has started, move the sizer.
      this.onDragSizer_(event.pageX);
      event.preventDefault();
    },

    /**
     * Called once the mouse has been released.
     */
    onMouseDragSizerEnd_: function(event) {
      if (!this.mouseDragging_)
        return;
      // Dragging is over.
      this.mouseDragging_ = false;
      event.preventDefault();
    },

    /**
     * Called when the user touches the sizer.  Starts moving the sizer in
     * response to future touch events.
     */
    onTouchDragSizerStart_: function(event) {
      this.touchDragging_ = true;
      event.preventDefault();
    },

    /**
     * Called when the mouse has moved after dragging started.
     */
    onTouchDragSizer_: function(event) {
      if (!this.touchDragging_)
        return;
      // If dragging has started, move the sizer.
      this.onDragSizer_(event.touches[0].pageX);
      event.preventDefault();
    },

    /**
     * Called once the user stops touching the screen.
     */
    onTouchDragSizerEnd_: function(event) {
      if (!this.touchDragging_)
        return;
      // Dragging is over.
      this.touchDragging_ = false;
      event.preventDefault();
    },

    /**
     * Common code used for both mouse and touch dragging.
     */
    onDragSizer_: function(pageX) {
      // Convert from page coordinates, to view coordinates.
      this.leftSplit_ = (pageX - this.getLeft());

      // Avoid shrinking the left box too much.
      this.leftSplit_ = Math.max(this.leftSplit_, MIN_PANEL_WIDTH);
      // Avoid shrinking the right box too much.
      this.leftSplit_ =
          Math.min(this.leftSplit_, this.getWidth() - MIN_PANEL_WIDTH);

      // Force a layout with the new |leftSplit_|.
      this.setGeometry(
          this.getLeft(), this.getTop(), this.getWidth(), this.getHeight());
    },
  };

  return ResizableVerticalSplitView;
})();
