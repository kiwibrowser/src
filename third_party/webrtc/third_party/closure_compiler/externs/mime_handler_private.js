// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Externs generated from namespace: mimeHandlerPrivate */

/** @const */
chrome.mimeHandlerPrivate = {};

/**
 * @typedef {?{
 *   embedded: boolean,
 *   mimeType: string,
 *   originalUrl: string,
 *   responseHeaders: !Object,
 *   streamUrl: string,
 *   tabId: number
 * }}
 */
chrome.mimeHandlerPrivate.StreamInfo;

/**
 * @param {function(!chrome.mimeHandlerPrivate.StreamInfo): void} callback
 */
chrome.mimeHandlerPrivate.getStreamInfo = function(callback) {}

/**
 * @param {function(): void=} opt_callback
 */
chrome.mimeHandlerPrivate.abortStream = function(opt_callback) {}
