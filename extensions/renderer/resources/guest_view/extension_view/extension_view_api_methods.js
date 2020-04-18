// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This module implements the public-facing API functions for the
// <extensionview> tag.

var ExtensionViewInternal = getInternalApi ?
    getInternalApi('extensionViewInternal') :
    require('extensionViewInternal').ExtensionViewInternal;
var ExtensionViewImpl = require('extensionView').ExtensionViewImpl;
var ExtensionViewConstants =
    require('extensionViewConstants').ExtensionViewConstants;

// An array of <extensionview>'s public-facing API methods.
var EXTENSION_VIEW_API_METHODS = [
  // Loads the given src into extensionview. Must be called every time the
  // the extensionview should load a new page. This is the only way to set
  // the extension and src attributes. Returns a promise indicating whether
  // or not load was successful.
  'load'
];

// -----------------------------------------------------------------------------
// Custom API method implementations.

ExtensionViewImpl.prototype.load = function(src) {
  return new Promise($Function.bind(function(resolve, reject) {
    $Array.push(this.loadQueue, {src: src, resolve: resolve, reject: reject});
    this.loadNextSrc();
  }, this))
  .then($Function.bind(function onLoadResolved() {
    this.pendingLoad = null;
    this.loadNextSrc();
  }, this), $Function.bind(function onLoadRejected(reason) {
    this.pendingLoad = null;
    this.loadNextSrc();
    return Promise.reject(reason);
  }, this));
};

// -----------------------------------------------------------------------------

ExtensionViewImpl.getApiMethods = function() {
  return EXTENSION_VIEW_API_METHODS;
};
