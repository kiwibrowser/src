// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * This variable is checked in SelectFileDialogExtensionBrowserTest.
 * @type {number}
 */
window.JSErrorCount = 0;

/**
 * Count uncaught exceptions.
 */
window.onerror = function(message, url) {
  // Analytics raises errors if the tracker instance is initiated in guest mode.
  // crbug.com/459983
  if (url === 'chrome://resources/js/analytics.js')
    return;
  window.JSErrorCount++;
};

// Overrides console.error() to count errors.
/**
 * @param {...*} var_args Message to be logged.
 */
console.error = (function() {
  var orig = console.error;
  return function() {
    window.JSErrorCount++;
    return orig.apply(this, arguments);
  };
})();

// Overrides console.assert() to count errors.
/**
 * @param {boolean} condition If false, log a message and stack trace.
 * @param {...*} var_args Objects to.
 */
console.assert = (function() {
  var orig = console.assert;
  return function(condition) {
    if (!condition)
      window.JSErrorCount++;
    return orig.apply(this, arguments);
  };
})();

/**
 * Wraps the function to use it as a callback.
 * This does:
 *  - Capture the stack trace in case of error.
 *  - Bind this object
 *
 * @param {Object} thisObject Object to be used as this.
 * @return {Function} Wrapped function.
 */
Function.prototype.wrap = function(thisObject) {
  var func = this;
  var liveStack = (new Error('Stack trace before async call')).stack;
  if (thisObject === undefined)
    thisObject = null;

  return function wrappedCallback() {
    try {
      return func.apply(thisObject, arguments);
    } catch (e) {
      console.error('Exception happens in callback.', liveStack);

      window.JSErrorCount++;
      throw e;
    }
  }
};
