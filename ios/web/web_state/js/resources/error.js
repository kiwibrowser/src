// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Error handling APIs and listeners.
 */

goog.provide('__crWeb.error');

// Requires __crWeb.message provided by __crWeb.allFramesWebBundle.

/** Beginning of anonymouse object */
(function() {

/**
 * JavaScript errors are logged on the main application side. The handler is
 * added ASAP to catch any errors in startup.
 */
window.addEventListener('error', function(event) {
  // Sadly, event.filename and event.lineno are always 'undefined' and '0'
  // with UIWebView.
  // TODO(crbug.com/711359): the aforementioned APIs may be working now in
  // WKWebView. Evaluate any cleanup / improvement we can do here.
  __gCrWeb.message.invokeOnHost(
      {'command': 'window.error', 'message': event.message.toString()});
});

// Flush the message queue.
if (__gCrWeb.message) {
  __gCrWeb.message.invokeQueues();
}

}());  // End of anonymous object
