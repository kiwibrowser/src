// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @suppress {checkTypes}
 *
 * @fileoverview
 * Browser test for the scenario below:
 * 1. Resize the client window to various sizes and verify the existence of
 *    horizontal and/or vertical scroll-bars.
 * 2. TODO(jamiewalch): Connect to a host and toggle various combinations of
 *    scale and resize; repeat test 1.
 * 3. TODO(jamiewalch): Disconnect; repeat test 1.
 */

'use strict';

/** @constructor */
browserTest.Scrollbars = function() {
  this.scroller_ = document.getElementById('scroller');
  this.SCROLLBAR_WIDTH_ = 16;
  this.BORDER_WIDTH_ = 1;

  // The top border is already accounted for by getBoundingClientRect, but
  // the bottom border is not.
  var marker = document.getElementById('bottom-marker');
  this.CONTENT_HEIGHT_ =
      marker.getBoundingClientRect().top + this.BORDER_WIDTH_;

  // The width of the content is computed from the width of a <section> (690px)
  // plus the margin of the "inset" class (20px). There's no easy way to get
  // that without hard-coding it. In fact, this is a bit simplistic because
  // the horizontal space required by the header depends on the length of the
  // product name.
  this.CONTENT_WIDTH_ = 690 + 20 + 2 * this.BORDER_WIDTH_;

};


browserTest.Scrollbars.prototype.run = function(data) {
  if (!base.isAppsV2()) {
    browserTest.fail(
        'Scroll-bar testing requires resizing the app window, which can ' +
        'only be done programmatically in apps v2.');
  }

  // Verify that scrollbars are added/removed correctly on the home screen.
  this.verifyHomeScreenScrollbars_()
      .then(browserTest.pass, browserTest.fail);
};


/**
 * Verify the test cases for the home-screen.
 * @return {Promise}
 */
browserTest.Scrollbars.prototype.verifyHomeScreenScrollbars_ = function() {
  // Note that, due to crbug.com/240772, if the window already has
  // scroll-bars, they will not be removed if the window size is
  // increased by less than the scroll-bar width. We work around that
  // when connected to a host because we know how big the content is
  // (in fact, testing this work-around is the main motivation for
  // writing this test), but it's not worth it for the home screen,
  // so make the window large not to require scrollbars before each test.
  var tooWide = this.CONTENT_WIDTH_ + 100;
  var tooTall = this.CONTENT_HEIGHT_ + 100;
  var removeScrollbars = this.resize_.bind(this, tooWide, tooTall);

  // Verify there are no scroll-bars if the window is as big as it needs
  // to be.
  return removeScrollbars()
  .then(this.resizeAndVerifyScroll_(
        this.CONTENT_WIDTH_,
        this.CONTENT_HEIGHT_,
        false, false))

  // Verify there is a vertical scroll-bar if the window is shorter than it
  // needs to be.
  .then(removeScrollbars)
  .then(this.resizeAndVerifyScroll_.bind(
        this,
        this.CONTENT_WIDTH_ + this.SCROLLBAR_WIDTH_,
        this.CONTENT_HEIGHT_ - 1,
        false, true))

  // Verify there is a horizontal scroll-bar if the window is narrow than it
  // needs to be.
  .then(removeScrollbars)
  .then(this.resizeAndVerifyScroll_.bind(
        this,
        this.CONTENT_WIDTH_ - 1,
        this.CONTENT_HEIGHT_ + this.SCROLLBAR_WIDTH_,
        true, false))

  // Verify there are both horizontal and vertical scroll-bars, even if one
  // is only needed as a result of the space occupied by the other.
  .then(removeScrollbars)
  .then(this.resizeAndVerifyScroll_.bind(
        this,
        this.CONTENT_WIDTH_,
        this.CONTENT_HEIGHT_ - 1,
        true, true))
  .then(removeScrollbars)
  .then(this.resizeAndVerifyScroll_.bind(
        this,
        this.CONTENT_WIDTH_ - 1,
        this.CONTENT_HEIGHT_,
        true, true))

  // Verify there are both horizontal and vertical scroll-bars, if both are
  // required independently.
  .then(removeScrollbars)
  .then(this.resizeAndVerifyScroll_.bind(
        this,
        this.CONTENT_WIDTH_ - 1,
        this.CONTENT_HEIGHT_ - 1,
        true, true));
};


/**
 * Returns whether or not horizontal and vertical scroll-bars are expected
 * and visible. To do this, it performs a hit-test close to the right and
 * bottom edges of the scroller <div>; since the content of that <div> fills
 * it completely, the hit-test will return the content unless there is a
 * scroll-bar visible on the corresponding edge, in which case it will return
 * the scroller <div> itself.
 *
 * @return {{horizontal: boolean, vertical:boolean}}
 * @private
 */
browserTest.Scrollbars.prototype.getScrollbarState_ = function() {
  var rect = this.scroller_.getBoundingClientRect();
  var rightElement = document.elementFromPoint(
      rect.right - 1, (rect.top + rect.bottom) / 2);
  var bottomElement = document.elementFromPoint(
      (rect.left + rect.right) / 2, rect.bottom - 1);
  return {
    horizontal: bottomElement === this.scroller_,
    vertical: rightElement === this.scroller_
  };
};


/**
 * Returns a promise that resolves if the scroll-bar state is as expected, or
 * rejects otherwise.
 *
 * @param {boolean} horizontalExpected
 * @param {boolean} verticalExpected
 * @return {Promise}
 * @private
 */
browserTest.Scrollbars.prototype.verifyScrollbarState_ =
    function(horizontalExpected, verticalExpected) {
  var scrollbarState = this.getScrollbarState_();
  if (scrollbarState.horizontal && !horizontalExpected) {
    return Promise.reject(new Error(
        'Horizontal scrollbar present but not expected.'));
  } else if (!scrollbarState.horizontal && horizontalExpected) {
    return Promise.reject(new Error(
        'Horizontal scrollbar expected but not present.'));
  } else if (scrollbarState.vertical && !verticalExpected) {
    return Promise.reject(new Error(
        'Vertical scrollbar present but not expected.'));
  } else if (!scrollbarState.vertical && verticalExpected) {
    return Promise.reject(new Error(
        'Vertical scrollbar expected but not present.'));
  }
  return Promise.resolve();
};


/**
 * @param {number} width
 * @param {number} height
 * @return {Promise} A promise that will be fulfilled when the window has
 *     been resized and it's safe to test scroll-bar visibility.
 * @private
 */
browserTest.Scrollbars.prototype.resize_ = function(width, height) {
  var win = chrome.app.window.current();
  win.outerBounds.width = width;
  win.outerBounds.height = height;
  // Chrome takes a while to update the scroll-bars, so don't resolve
  // immediately. Waiting for the onBoundsChanged event would be cleaner,
  // but isn't reliable.
  return base.Promise.sleep(500);
};


/**
 * @param {number} width
 * @param {number} height
 * @param {boolean} horizontalExpected
 * @param {boolean} verticalExpected
 * @return {Promise} A promise that will be fulfilled when the window has
 *     been resized and it's safe to test scroll-bar visibility.
 * @private
 */
browserTest.Scrollbars.prototype.resizeAndVerifyScroll_ =
    function(width, height, horizontalExpected, verticalExpected) {
  return this.resize_(width, height).then(
      this.verifyScrollbarState_.bind(
          this, horizontalExpected, verticalExpected));
};
