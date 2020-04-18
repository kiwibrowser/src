// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview APIs used for the scroll workaround. See crbug.com/554257.
 */

goog.provide('__crWeb.scrollWorkaround');

/** Beginning of anonymous object */
(function() {

/** @private */
var webViewScrollViewIsDragging_ = false;

/**
 * Tracks whether user is in the middle of scrolling/dragging. If user is
 * scrolling, ignore window.scrollTo() until user stops scrolling.
 */
__gCrWeb['setWebViewScrollViewIsDragging'] = function(state) {
  webViewScrollViewIsDragging_ = state;
};

/** @private */
var originalWindowScrollTo_ = window.scrollTo;

/**
 * Wraps the original window.scrollTo() to suppress it as long as
 * webViewScrollViewIsDragging is true.
 */
window.scrollTo = function(x, y) {
  if (webViewScrollViewIsDragging_) return;
  originalWindowScrollTo_(x, y);
};

}());  // End of anonymous object
