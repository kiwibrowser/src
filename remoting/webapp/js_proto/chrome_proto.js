// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains various hacks needed to inform the closure compiler of
// various Chrome-specific properties and methods that is not specified in
// /third_part/closure_compiler/externs/chrome_externsions.js. It is used only
// with the closure compiler to verify the type-correctness of our code.

console.error('Proto file should not be executed.');

/** @type {{background: Object}} */
chrome.runtime.Manifest.prototype.app;

/** @type {string} */
chrome.app.window.AppWindow.prototype.id;

/**
 * @param {{rects: Array<ClientRect>}} rects
 */
chrome.app.window.AppWindow.prototype.setShape = function(rects) {};

/** @type {boolean} */
OnClickData.prototype.checked;

/**
 * @constructor
 */
chrome.socket.SendInfo = function() {};

/** @type {number} */
chrome.socket.SendInfo.prototype.resultCode;

/** @type {number} */
chrome.socket.SendInfo.prototype.bytesSent;

/** @param {function(FileWriter):void} callback */
Entry.prototype.createWriter = function(callback) {};
