// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @constructor
 * @struct
 * @extends {HTMLElement}
 *
 * TODO(yawano): This extern file is a temporary fix. Create or migrate to
 *     proper extern definitions of paper elements.
 */
function PaperRipple() {}

PaperRipple.prototype.simulatedRipple = function() {};

/**
 * @param {Event=} event
 */
PaperRipple.prototype.downAction = function(event) {};

/**
 * @param {Event=} event
 */
PaperRipple.prototype.upAction = function(event) {};
