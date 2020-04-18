// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var TopMidBottomView = (function() {
  'use strict';

  // We inherit from View.
  var superClass = View;

  /**
   * This view stacks three boxes -- one at the top, one at the bottom, and
   * one that fills the remaining space between those two.  Either the top
   * or the bottom bar may be null.
   *
   *  +----------------------+
   *  |      topbar          |
   *  +----------------------+
   *  |                      |
   *  |                      |
   *  |                      |
   *  |                      |
   *  |      middlebox       |
   *  |                      |
   *  |                      |
   *  |                      |
   *  |                      |
   *  |                      |
   *  +----------------------+
   *  |     bottombar        |
   *  +----------------------+
   *
   *  @constructor
   */
  function TopMidBottomView(topView, midView, bottomView) {
    superClass.call(this);

    this.topView_ = topView;
    this.midView_ = midView;
    this.bottomView_ = bottomView;
  }

  TopMidBottomView.prototype = {
    // Inherit the superclass's methods.
    __proto__: superClass.prototype,

    setGeometry: function(left, top, width, height) {
      superClass.prototype.setGeometry.call(this, left, top, width, height);

      // Calculate the vertical split points.
      var topbarHeight = 0;
      if (this.topView_)
        topbarHeight = this.topView_.getHeight();
      var bottombarHeight = 0;
      if (this.bottomView_)
        bottombarHeight = this.bottomView_.getHeight();
      var middleboxHeight = height - (topbarHeight + bottombarHeight);
      if (middleboxHeight < 0)
        middleboxHeight = 0;

      // Position the boxes using calculated split points.
      if (this.topView_)
        this.topView_.setGeometry(left, top, width, topbarHeight);
      this.midView_.setGeometry(
          left, top + topbarHeight, width, middleboxHeight);
      if (this.bottomView_) {
        this.bottomView_.setGeometry(
            left, top + topbarHeight + middleboxHeight, width, bottombarHeight);
      }
    },

    show: function(isVisible) {
      superClass.prototype.show.call(this, isVisible);
      if (this.topView_)
        this.topView_.show(isVisible);
      this.midView_.show(isVisible);
      if (this.bottomView_)
        this.bottomView_.show(isVisible);
    }
  };

  return TopMidBottomView;
})();
