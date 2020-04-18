// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Externs generated from namespace: commandLinePrivate */

/**
 * @const
 */
chrome.commandLinePrivate = {};

/**
 * Returns whether a switch is specified on the command line when launching
 * Chrome.
 * @param {string} name The name of a command line switch, without leading
 * "--", such as "enable-experimental-extension-apis".
 * @param {function(boolean)} callback
 */
chrome.commandLinePrivate.hasSwitch = function(name, callback) {};
