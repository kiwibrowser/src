// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @interface
 */
function DragTarget(){};

/**
 * This definition is required to satisfy
 * ui/file_manager/file_manager/foreground/js/ui/drag_selector.js.
 *
 * @param {number} x
 * @param {number} y
 * @param {number=} opt_width
 * @param {number=} opt_height
 * @return {Array<number>}
 */
DragTarget.prototype.getHitElements = function(x, y, opt_width, opt_height) {};
