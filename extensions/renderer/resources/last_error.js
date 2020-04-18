// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var GetAvailability = requireNative('v8_context').GetAvailability;
var GetGlobal = requireNative('sendRequest').GetGlobal;

// Utility for setting chrome.*.lastError.
//
// A utility here is useful for two reasons:
//  1. For backwards compatibility we need to set chrome.extension.lastError,
//     but not all contexts actually have access to the extension namespace.
//  2. When calling across contexts, the global object that gets lastError set
//     needs to be that of the caller. We force callers to explicitly specify
//     the chrome object to try to prevent bugs here.

/**
 * Sets the last error for |name| on |targetChrome| to |message| with an
 * optional |stack|.
 */
function set(name, message, stack, targetChrome) {
  if (!targetChrome) {
    var errorMessage = name + ': ' + message;
    if (stack != null && stack != '')
      errorMessage += '\n' + stack;
    throw new Error('No chrome object to set error: ' + errorMessage);
  }
  clear(targetChrome);  // in case somebody has set a sneaky getter/setter

  var errorObject = { message: message };
  if (GetAvailability('extension.lastError').is_available)
    targetChrome.extension.lastError = errorObject;

  assertRuntimeIsAvailable();

  // We check to see if developers access runtime.lastError in order to decide
  // whether or not to log it in the (error) console.
  privates(targetChrome.runtime).accessedLastError = false;
  $Object.defineProperty(targetChrome.runtime, 'lastError', {
      configurable: true,
      get: function() {
        privates(targetChrome.runtime).accessedLastError = true;
        return errorObject;
      },
      set: function(error) {
        errorObject = errorObject;
      }});
};

/**
 * Check if anyone has checked chrome.runtime.lastError since it was set.
 * @param {Object} targetChrome the Chrome object to check.
 * @return boolean True if the lastError property was set.
 */
function hasAccessed(targetChrome) {
  assertRuntimeIsAvailable();
  return privates(targetChrome.runtime).accessedLastError === true;
}

/**
 * Check whether there is an error set on |targetChrome| without setting
 * |accessedLastError|.
 * @param {Object} targetChrome the Chrome object to check.
 * @return boolean Whether lastError has been set.
 */
function hasError(targetChrome) {
  if (!targetChrome)
    throw new Error('No target chrome to check');

  assertRuntimeIsAvailable();
  return $Object.hasOwnProperty(targetChrome.runtime, 'lastError');
};

/**
 * Clears the last error on |targetChrome|.
 */
function clear(targetChrome) {
  if (!targetChrome)
    throw new Error('No target chrome to clear error');

  if (GetAvailability('extension.lastError').is_available)
   delete targetChrome.extension.lastError;

  assertRuntimeIsAvailable();
  delete targetChrome.runtime.lastError;
  delete privates(targetChrome.runtime).accessedLastError;
};

function assertRuntimeIsAvailable() {
  // chrome.runtime should always be available, but maybe it's disappeared for
  // some reason? Add debugging for http://crbug.com/258526.
  var runtimeAvailability = GetAvailability('runtime.lastError');
  if (!runtimeAvailability.is_available) {
    throw new Error('runtime.lastError is not available: ' +
                    runtimeAvailability.message);
  }
  if (!chrome.runtime)
    throw new Error('runtime namespace is null or undefined');
}

/**
 * Runs |callback(args)| with last error args as in set().
 *
 * The target chrome object is the global object's of the callback, so this
 * method won't work if the real callback has been wrapped (etc).
 */
function run(name, message, stack, callback, args) {
  var global = GetGlobal(callback);
  var targetChrome = global && global.chrome;
  set(name, message, stack, targetChrome);
  try {
    $Function.apply(callback, undefined, args);
  } finally {
    reportIfUnchecked(name, targetChrome, stack);
    clear(targetChrome);
  }
}

/**
 * Checks whether chrome.runtime.lastError has been accessed if set.
 * If it was set but not accessed, the error is reported to the console.
 *
 * @param {string=} name - name of API.
 * @param {Object} targetChrome - the Chrome object to check.
 * @param {string=} stack - Stack trace of the call up to the error.
 */
function reportIfUnchecked(name, targetChrome, stack) {
  if (hasAccessed(targetChrome) || !hasError(targetChrome))
    return;
  var message = targetChrome.runtime.lastError.message;
  console.error("Unchecked runtime.lastError while running " +
      (name || "unknown") + ": " + message + (stack ? "\n" + stack : ""));
}

exports.$set('clear', clear);
exports.$set('hasAccessed', hasAccessed);
exports.$set('hasError', hasError);
exports.$set('set', set);
exports.$set('run', run);
exports.$set('reportIfUnchecked', reportIfUnchecked);
