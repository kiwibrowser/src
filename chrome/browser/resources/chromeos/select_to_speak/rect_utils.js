// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Utility functions for processing rectangles and coordinates.

/**
 * Return the rect that encloses two points.
 * @param {number} x1 The first x coordinate.
 * @param {number} y1 The first y coordinate.
 * @param {number} x2 The second x coordinate.
 * @param {number} y2 The second x coordinate.
 * @return {{left: number, top: number, width: number, height: number}}
 */
function rectFromPoints(x1, y1, x2, y2) {
  var left = Math.min(x1, x2);
  var right = Math.max(x1, x2);
  var top = Math.min(y1, y2);
  var bottom = Math.max(y1, y2);
  return {left: left, top: top, width: right - left, height: bottom - top};
}

/**
 * Returns true if |rect1| and |rect2| overlap. The rects must define
 * left, top, width, and height.
 * @param {{left: number, top: number, width: number, height: number}} rect1
 * @param {{left: number, top: number, width: number, height: number}} rect2
 * @return {boolean} True if the rects overlap.
 */
function overlaps(rect1, rect2) {
  var l1 = rect1.left;
  var r1 = rect1.left + rect1.width;
  var t1 = rect1.top;
  var b1 = rect1.top + rect1.height;
  var l2 = rect2.left;
  var r2 = rect2.left + rect2.width;
  var t2 = rect2.top;
  var b2 = rect2.top + rect2.height;
  return (l1 < r2 && r1 > l2 && t1 < b2 && b1 > t2);
}
