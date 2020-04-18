// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Scripts to allow page console.log() etc. output to be seen on the console
// of the host application.

goog.provide('__crWeb.console');

// Requires __crWeb.message provided by __crWeb.allFramesWebBundle.

/**
 * Namespace for this module.
 */
__gCrWeb.console = {};

/* Beginning of anonymous object. */
(function() {
function sendConsoleMessage(method, originalArgs) {
  var message, slicedArgs = Array.prototype.slice.call(originalArgs);
  try {
    message = slicedArgs.join(' ');
  } catch (err) {
  }
  __gCrWeb.message.invokeOnHost({
    'command': 'console',
    'method': method,
    'message': message,
    'origin': document.location.origin
  });
}

console.log = function() {
  sendConsoleMessage('log', arguments);
};

console.debug = function() {
  sendConsoleMessage('debug', arguments);
};

console.info = function() {
  sendConsoleMessage('info', arguments);
};

console.warn = function() {
  sendConsoleMessage('warn', arguments);
};

console.error = function() {
  sendConsoleMessage('error', arguments);
};
}());
