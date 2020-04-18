// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @constructor
 * @extends {Window}
 */
var BackgroundWindow = function() {};

/**
 * @type {FileBrowserBackground}
 */
BackgroundWindow.prototype.background;

/**
 * @param {Window} window
 */
BackgroundWindow.prototype.registerDialog = function(window) {};

/**
 * @type {!Object}
 */
BackgroundWindow.prototype.launcher = {};

/**
 * @param {Object=} opt_appState App state.
 * @param {number=} opt_id Window id.
 * TODO(oka): We intentionally omit optional launchType and callback parameters
 * here because to do so we need to define |LaunchType| in this file, but then
 * gyp v1 fails due to double definition of |LaunchType|. Since no foreground
 * scripts set launchType parameter, we can omit them though it's hacky. Let's
 * add them back after v1 is gone.
 */
BackgroundWindow.prototype.launcher.launchFileManager =
    function(opt_appState, opt_id) {};
