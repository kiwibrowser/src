// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @interface */
var InputDeviceCapabilities;

/** @type {?InputDeviceCapabilities} */
Event.prototype.sourceCapabilities;

/**
 * @interface
 */
var AnimationEffect = function() {};

/**
 * @param {Element} target
 * @param {!Array<!Object>} frames
 * @param {(number|Object)=} timing
 * @constructor
 * @implements {AnimationEffect}
 */
var KeyframeEffect = function(target, frames, timing) {};

/**
 * @param {!Array<!AnimationEffect>} group
 * @constructor
 * @implements {AnimationEffect}
 */
var GroupEffect = function(group) {};

/**
 * @interface
 */
var Animation = function() {};

/**
 * @return {undefined}
 */
Animation.prototype.cancel = function() {};

/**
 * @type {!Promise}
 */
Animation.prototype.finished;

document.timeline = {};

/**
 * @param {!AnimationEffect} effect
 * @return {!Animation}
 */
document.timeline.play = function(effect) {};

/**
 * @param {!number} index
 * @return {undefined}
 */
Element.prototype.selectIndex = function(index) {};
