// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Enum for WebDriver status codes.
 * @enum {number}
 */
var StatusCode = {
  OK: 0,
  UNKNOWN_ERROR: 13,
  JAVASCRIPT_ERROR: 17,
  SCRIPT_TIMEOUT: 28,
};

/**
 * Dictionary key for asynchronous script info.
 * @const
 */
var ASYNC_INFO_KEY = '$chrome_asyncScriptInfo';

/**
* Return the information of asynchronous script execution.
*
* @return {Object<?>} Information of asynchronous script execution.
*/
function getAsyncScriptInfo() {
  if (!(ASYNC_INFO_KEY in document))
    document[ASYNC_INFO_KEY] = {'id': 0};
  return document[ASYNC_INFO_KEY];
}

/**
* Execute the given script and save its asynchronous result.
*
* If script1 finishes after script2 is executed, then script1's result will be
* discarded while script2's will be saved.
*
* @param {string} script The asynchronous script to be executed. The script
*     should be a proper function body. It will be wrapped in a function and
*     invoked with the given arguments and, as the final argument, a callback
*     function to invoke to report the asynchronous result.
* @param {!Array<*>} args Arguments to be passed to the script.
* @param {boolean} isUserSupplied Whether the script is supplied by the user.
*     If not, UnknownError will be used instead of JavaScriptError if an
*     exception occurs during the script, and an additional error callback will
*     be supplied to the script.
* @param {?number} opt_timeoutMillis The timeout, in milliseconds, to use.
*     If the timeout is exceeded and the callback has not been invoked, a error
*     result will be saved and future invocation of the callback will be
*     ignored.
*/
function executeAsyncScript(script, args, isUserSupplied, opt_timeoutMillis) {
  var info = getAsyncScriptInfo();
  info.id++;
  delete info.result;
  var id = info.id;

  function report(status, value) {
    if (id != info.id)
      return;
    info.id++;
    // Undefined value is skipped when the object is converted to JSON.
    // Replace it with null so we don't lose the value.
    if (value === undefined)
      value = null;
    info.result = {status: status, value: value};
  }
  function reportValue(value) {
    report(StatusCode.OK, value);
  }
  function reportScriptError(error) {
    var code = isUserSupplied ? StatusCode.JAVASCRIPT_ERROR :
                                (error.code || StatusCode.UNKNOWN_ERROR);
    var message = error.message;
    if (error.stack) {
      message += "\nJavaScript stack:\n" + error.stack;
    }
    report(code, message);
  }
  args.push(reportValue);
  if (!isUserSupplied)
    args.push(reportScriptError);

  try {
    new Function(script).apply(null, args);
  } catch (error) {
    reportScriptError(error);
    return;
  }

  if (typeof(opt_timeoutMillis) != 'undefined') {
    window.setTimeout(function() {
      var code = isUserSupplied ? StatusCode.SCRIPT_TIMEOUT :
                                  StatusCode.UNKNOWN_ERROR;
      var errorMsg = 'result was not received in ' + opt_timeoutMillis / 1000 +
                     ' seconds';
      report(code, errorMsg);
    }, opt_timeoutMillis);
  }
}
