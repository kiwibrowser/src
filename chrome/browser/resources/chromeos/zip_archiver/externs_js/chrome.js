// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * The Chrome File System Provider API.
 * @see https://developer.chrome.com/apps/fileSystemProvider
 * @const
 */
chrome.fileSystemProvider = {};

/**
 * @see https://developer.chrome.com/apps/fileSystemProvider#method-get
 * @param {string} fileSystemId
 * @param {function(!FileSystemInfo)} callback
 */
chrome.fileSystemProvider.get = function(fileSystemId, callback) {};

/**
 * @see https://developer.chrome.com/apps/fileSystemProvider#method-mount
 * @param {!Object} options
 * @param {function(...)=} opt_callback
 */
chrome.fileSystemProvider.mount = function(options, opt_callback) {};

/**
 * @see https://developer.chrome.com/apps/fileSystemProvider#method-unmount
 * @param {!Object} options
 * @param {function(...)=} opt_callback
 */
chrome.fileSystemProvider.unmount = function(options, opt_callback) {};

/**
 * @see
 * https://developer.chrome.com/apps/fileSystemProvider#event-onUnmountRequested
 * @typedef {!Event}
 */
chrome.fileSystemProvider.onUnmountRequested;

/**
 * @see
 * https://developer.chrome.com/apps/fileSystemProvider#event-onGetMetadataRequested
 * @typedef {!Event}
 */
chrome.fileSystemProvider.onGetMetadataRequested;

/**
 * @see
 * https://developer.chrome.com/apps/fileSystemProvider#event-onReadDirectoryRequested
 * @typedef {!Event}
 */
chrome.fileSystemProvider.onReadDirectoryRequested;

/**
 * @see
 * https://developer.chrome.com/apps/fileSystemProvider#event-onOpenFileRequested
 * @typedef {!Event}
 */
chrome.fileSystemProvider.onOpenFileRequested;

/**
 * @see
 * https://developer.chrome.com/apps/fileSystemProvider#event-onCloseFileRequested
 * @typedef {!Event}
 */
chrome.fileSystemProvider.onCloseFileRequested;

/**
 * @see
 * https://developer.chrome.com/apps/fileSystemProvider#event-onReadFileRequested
 * @typedef {!Event}
 */
chrome.fileSystemProvider.onReadFileRequested;

/**
 * @see https://developer.chrome.com/apps/fileSystemProvider#type-FileSystemInfo
 * @typedef {!Object}
 */
var FileSystemInfo;

/**
 * @see https://developer.chrome.com/apps/fileSystemProvider#type-ProviderError
 * @typedef {string}
 */
var ProviderError;

/**
 * @see https://developer.chrome.com/apps/fileSystemProvider#type-EntryMetadata
 * @typedef {!Object}
 */
var EntryMetadata;

/**
 * @see https://developer.chrome.com/apps/fileSystemProvider#type-OpenFileMode
 * @typedef {string}
 */
var OpenFileMode;

/**
 * THe Chrome Manifest Icons. Not defined in externs/chrome_extensions.js.
 * @see https://developer.chrome.com/apps/manifest/icons
 * @typedef {!Array<string>}
 */
chrome.runtime.Manifest.prototype.icons;
