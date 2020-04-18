// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This module implements the public-facing API functions for the <webview> tag.

var WebViewInternal = getInternalApi ?
    getInternalApi('webViewInternal') :
    require('webViewInternal').WebViewInternal;
var WebViewImpl = require('webView').WebViewImpl;

// An array of <webview>'s public-facing API methods. Methods without custom
// implementations will be given default implementations that call into the
// internal API method with the same name in |WebViewInternal|. For example, a
// method called 'someApiMethod' would be given the following default
// implementation:
//
// WebViewImpl.prototype.someApiMethod = function(var_args) {
//   if (!this.guest.getId()) {
//     return false;
//   }
//   var args = $Array.concat([this.guest.getId()], $Array.slice(arguments));
//   $Function.apply(WebViewInternal.someApiMethod, null, args);
//   return true;
// };
//
// These default implementations come from createDefaultApiMethod() in
// web_view.js.
var WEB_VIEW_API_METHODS = [
  // Add content scripts for the guest page.
  'addContentScripts',

  // Navigates to the previous history entry.
  'back',

  // Returns whether there is a previous history entry to navigate to.
  'canGoBack',

  // Returns whether there is a subsequent history entry to navigate to.
  'canGoForward',

  // Captures the visible region of the WebView contents into a bitmap.
  'captureVisibleRegion',

  // Clears browsing data for the WebView partition.
  'clearData',

  // Injects JavaScript code into the guest page.
  'executeScript',

  // Initiates a find-in-page request.
  'find',

  // Navigates to the subsequent history entry.
  'forward',

  // Returns audio state.
  'getAudioState',

  // Returns Chrome's internal process ID for the guest web page's current
  // process.
  'getProcessId',

  // Returns the user agent string used by the webview for guest page requests.
  'getUserAgent',

  // Gets the current zoom factor.
  'getZoom',

  // Gets the current zoom mode of the webview.
  'getZoomMode',

  // Navigates to a history entry using a history index relative to the current
  // navigation.
  'go',

  // Injects CSS into the guest page.
  'insertCSS',

  // Returns whether audio is muted.
  'isAudioMuted',

  // Indicates whether or not the webview's user agent string has been
  // overridden.
  'isUserAgentOverridden',

  // Loads a data URL with a specified base URL used for relative links.
  // Optionally, a virtual URL can be provided to be shown to the user instead
  // of the data URL.
  'loadDataWithBaseUrl',

  // Prints the contents of the webview.
  'print',

  // Removes content scripts for the guest page.
  'removeContentScripts',

  // Reloads the current top-level page.
  'reload',

  // Set audio mute.
  'setAudioMuted',

  // Override the user agent string used by the webview for guest page requests.
  'setUserAgentOverride',

  // Changes the zoom factor of the page.
  'setZoom',

  // Changes the zoom mode of the webview.
  'setZoomMode',

  // Stops loading the current navigation if one is in progress.
  'stop',

  // Ends the current find session.
  'stopFinding',

  // Forcibly kills the guest web page's renderer process.
  'terminate'
];

// -----------------------------------------------------------------------------
// Custom API method implementations.

WebViewImpl.prototype.addContentScripts = function(rules) {
  return WebViewInternal.addContentScripts(this.viewInstanceId, rules);
};

WebViewImpl.prototype.back = function(callback) {
  return this.go(-1, callback);
};

WebViewImpl.prototype.canGoBack = function() {
  return this.entryCount > 1 && this.currentEntryIndex > 0;
};

WebViewImpl.prototype.canGoForward = function() {
  return this.currentEntryIndex >= 0 &&
      this.currentEntryIndex < (this.entryCount - 1);
};

WebViewImpl.prototype.executeScript = function(var_args) {
  return this.executeCode(WebViewInternal.executeScript,
                          $Array.slice(arguments));
};

WebViewImpl.prototype.forward = function(callback) {
  return this.go(1, callback);
};

WebViewImpl.prototype.getProcessId = function() {
  return this.processId;
};

WebViewImpl.prototype.getUserAgent = function() {
  return this.userAgentOverride || navigator.userAgent;
};

WebViewImpl.prototype.insertCSS = function(var_args) {
  return this.executeCode(WebViewInternal.insertCSS, $Array.slice(arguments));
};

WebViewImpl.prototype.isUserAgentOverridden = function() {
  return !!this.userAgentOverride &&
      this.userAgentOverride != navigator.userAgent;
};

WebViewImpl.prototype.loadDataWithBaseUrl = function(
    dataUrl, baseUrl, virtualUrl) {
  if (!this.guest.getId()) {
    return;
  }
  WebViewInternal.loadDataWithBaseUrl(
      this.guest.getId(), dataUrl, baseUrl, virtualUrl, function() {
        // Report any errors.
        if (chrome.runtime.lastError != undefined) {
          window.console.error(
              'Error while running webview.loadDataWithBaseUrl: ' +
                  chrome.runtime.lastError.message);
        }
      });
};

WebViewImpl.prototype.print = function() {
  return this.executeScript({code: 'window.print();'});
};

WebViewImpl.prototype.removeContentScripts = function(names) {
  return WebViewInternal.removeContentScripts(this.viewInstanceId, names);
};

WebViewImpl.prototype.setUserAgentOverride = function(userAgentOverride) {
  this.userAgentOverride = userAgentOverride;
  if (!this.guest.getId()) {
    // If we are not attached yet, then we will pick up the user agent on
    // attachment.
    return false;
  }
  WebViewInternal.overrideUserAgent(this.guest.getId(), userAgentOverride);
  return true;
};

WebViewImpl.prototype.setZoom = function(zoomFactor, callback) {
  if (!this.guest.getId()) {
    this.cachedZoomFactor = zoomFactor;
    return false;
  }
  this.cachedZoomFactor = 1;
  WebViewInternal.setZoom(this.guest.getId(), zoomFactor, callback);
  return true;
};

// -----------------------------------------------------------------------------

WebViewImpl.getApiMethods = function() {
  return WEB_VIEW_API_METHODS;
};
