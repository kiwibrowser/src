// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Support code for the Contextual Search unittests feature.
 *
 */


/**
 * Namespace for this file.  Depends on __gCrWeb['contextualSearch'] having
 * already been injected.
 */
__gCrWeb['contextualSearch_unittest'] = {};

/* Anyonymizing block */
new function() {

/**
 * Generate a tap event on an element. Remove the span around the element.
 * @param {string} elementID The ID of the element to tap.
 * @return {object} Empty if element did not trigger CS. Else, the CS context.
 */
__gCrWeb['contextualSearch'].tapOnElement = function(elementID) {
  var element = document.getElementById(elementID);
  if (element) {
    var rect = element.getBoundingClientRect();
    var relativeX = (rect.left + document.body.scrollLeft);
    var relativeY = (rect.top + document.body.scrollTop);
    var touch = document.createEvent('TouchEvent');
    touch.initUIEvent('touchend', true, true);
    element.dispatchEvent(touch);
    return __gCrWeb.contextualSearch.handleTapAtPoint(
        (relativeX + rect.width / 2) / document.documentElement.scrollWidth,
        (relativeY + rect.height / 2) / document.documentElement.scrollHeight);
  }
  return null;
};

/* Anyonymizing block end */
}
