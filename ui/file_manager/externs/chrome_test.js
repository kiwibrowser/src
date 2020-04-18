// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Externs generated from namespace: test */

/**
 * @const
 */
chrome.test = {};

/**
 * Gives configuration options set by the test.
 * @param {Function} callback
 */
chrome.test.getConfig = function(callback) {};

/**
 * Notifies the browser process that test code running in the extension failed.
 *  This is only used for internal unit testing.
 * @param {string} message
 */
chrome.test.notifyFail = function(message) {};

/**
 * Notifies the browser process that test code running in the extension passed.
 *  This is only used for internal unit testing.
 * @param {string=} message
 */
chrome.test.notifyPass = function(message) {};

/**
 * Logs a message during internal unit testing.
 * @param {string} message
 */
chrome.test.log = function(message) {};

/**
 * Sends a string message to the browser process, generating a Notification
 * that C++ test code can wait for.
 * @param {string} message
 * @param {Function=} callback
 */
chrome.test.sendMessage = function(message, callback) {};

/**
 */
chrome.test.callbackAdded = function() {};

/**
 */
chrome.test.runNextTest = function() {};

/**
 * @param {?=} message
 */
chrome.test.fail = function(message) {};

/**
 * @param {?=} message
 */
chrome.test.succeed = function(message) {};

/**
 * Returns an instance of the ModuleSystem for the given context.
 * @param {Object} context
 */
chrome.test.getModuleSystem = function(context) {};

/**
 * @param {?} test
 * @param {string=} message
 */
chrome.test.assertTrue = function(test, message) {};

/**
 * @param {?} test
 * @param {string=} message
 */
chrome.test.assertFalse = function(test, message) {};

/**
 * @param {?} test
 * @param {boolean} expected
 * @param {string=} message
 */
chrome.test.assertBool = function(test, expected, message) {};

/**
 * @param {?=} expected
 * @param {?=} actual
 */
chrome.test.checkDeepEq = function(expected, actual) {};

/**
 * @param {?=} expected
 * @param {?=} actual
 * @param {string=} message
 */
chrome.test.assertEq = function(expected, actual, message) {};

/**
 */
chrome.test.assertNoLastError = function() {};

/**
 * @param {string} expectedError
 */
chrome.test.assertLastError = function(expectedError) {};

/**
 * @param {Object} self
 * @param {Array} args
 * @param {?} message
 * @param {Function} fn
 */
chrome.test.assertThrows = function(self, args, message, fn) {};

/**
 * @param {string=} expectedError
 * @param {Function=} func
 */
chrome.test.callback = function(expectedError, func) {};

/**
 * @param {?} event
 * @param {Function} func
 */
chrome.test.listenOnce = function(event, func) {};

/**
 * @param {?} event
 * @param {Function} func
 */
chrome.test.listenForever = function(event, func) {};

/**
 * @param {Function=} func
 */
chrome.test.callbackPass = function(func) {};

/**
 * @param {string} expectedError
 * @param {Function=} func
 */
chrome.test.callbackFail = function(expectedError, func) {};

/**
 * @param {Array} tests
 */
chrome.test.runTests = function(tests) {};

/**
 */
chrome.test.getApiFeatures = function() {};

/**
 * @param {Array=} apiNames
 */
chrome.test.getApiDefinitions = function(apiNames) {};

/**
 */
chrome.test.isProcessingUserGesture = function() {};

/**
 * Runs the callback in the context of a user gesture.
 * @param {Function} callback
 */
chrome.test.runWithUserGesture = function(callback) {};

/**
 * Sends a string message one round trip from the renderer to the browser
 * process and back.
 * @param {string} message
 * @param {Function} callback
 */
chrome.test.waitForRoundTrip = function(message, callback) {};

/**
 * Sets the function to be called when an exception occurs. By default this is
 * a function which fails the test. This is reset for every test run through
 * $ref:test.runTests.
 * @param {Function} callback
 */
chrome.test.setExceptionHandler = function(callback) {};

/** @type {!ChromeEvent} */
chrome.test.onMessage;
