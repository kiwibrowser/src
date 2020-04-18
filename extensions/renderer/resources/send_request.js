// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var exceptionHandler = require('uncaught_exception_handler');
var lastError = require('lastError');
var logging = requireNative('logging');
var natives = requireNative('sendRequest');
var validate = require('schemaUtils').validate;

var safeCallbackApply = exceptionHandler.safeCallbackApply;

// All outstanding requests from sendRequest().
var requests = { __proto__: null };

// Used to prevent double Activity Logging for API calls that use both custom
// bindings and ExtensionFunctions (via sendRequest).
var calledSendRequest = false;

// Callback handling.
function handleResponse(requestId, name, success, responseList, error) {
  // The chrome objects we will set lastError on. Really we should only be
  // setting this on the callback's chrome object, but set on ours too since
  // it's conceivable that something relies on that.
  var callerChrome = chrome;

  try {
    var request = requests[requestId];
    logging.DCHECK(request != null);

    // lastError needs to be set on the caller's chrome object no matter what,
    // though chances are it's the same as ours (it will be different when
    // calling API methods on other contexts).
    if (request.callback) {
      var global = natives.GetGlobal(request.callback);
      callerChrome = global ? global.chrome : callerChrome;
    }

    lastError.clear(chrome);
    if (callerChrome !== chrome)
      lastError.clear(callerChrome);

    if (!success) {
      if (!error)
        error = "Unknown error.";
      lastError.set(name, error, request.stack, chrome);
      if (callerChrome !== chrome)
        lastError.set(name, error, request.stack, callerChrome);
    }

    if (request.customCallback) {
      safeCallbackApply(name,
                        request,
                        request.customCallback,
                        $Array.concat([name, request, request.callback],
                                      responseList));
    } else if (request.callback) {
      // Validate callback in debug only -- and only when the
      // caller has provided a callback. Implementations of api
      // calls may not return data if they observe the caller
      // has not provided a callback.
      if (logging.DCHECK_IS_ON() && !error) {
        if (!request.callbackSchema.parameters)
          throw new Error(name + ": no callback schema defined");
        validate(responseList, request.callbackSchema.parameters);
      }
      safeCallbackApply(name, request, request.callback, responseList);
    }

    if (error && !lastError.hasAccessed(chrome)) {
      // The native call caused an error, but the developer might not have
      // checked runtime.lastError.
      lastError.reportIfUnchecked(name, callerChrome, request.stack);
    }
  } finally {
    delete requests[requestId];
    lastError.clear(chrome);
    if (callerChrome !== chrome)
      lastError.clear(callerChrome);
  }
}

function prepareRequest(args, argSchemas) {
  var request = { __proto__: null };
  var argCount = args.length;

  // Look for callback param.
  if (argSchemas.length > 0 &&
      argSchemas[argSchemas.length - 1].type == "function") {
    request.callback = args[args.length - 1];
    request.callbackSchema = argSchemas[argSchemas.length - 1];
    --argCount;
  }

  request.args = $Array.slice(args, 0, argCount);
  return request;
}

// Send an API request and optionally register a callback.
// |optArgs| is an object with optional parameters as follows:
// - customCallback: a callback that should be called instead of the standard
//   callback.
// - forIOThread: true if this function should be handled on the browser IO
//   thread.
// - preserveNullInObjects: true if it is safe for null to be in objects.
// - stack: An optional string that contains the stack trace, to be displayed
//   to the user if an error occurs.
function sendRequest(functionName, args, argSchemas, optArgs) {
  calledSendRequest = true;
  if (!optArgs)
    optArgs = { __proto__: null };
  logging.DCHECK(optArgs.__proto__ == null);
  var request = prepareRequest(args, argSchemas);
  request.stack = optArgs.stack || exceptionHandler.getExtensionStackTrace();
  if (optArgs.customCallback) {
    request.customCallback = optArgs.customCallback;
  }

  var hasCallback = request.callback || optArgs.customCallback;
  var requestId =
      natives.StartRequest(functionName, request.args, hasCallback,
                           optArgs.forIOThread, optArgs.preserveNullInObjects);
  delete request.args;
  request.id = requestId;
  requests[requestId] = request;
}

function getCalledSendRequest() {
  return calledSendRequest;
}

function clearCalledSendRequest() {
  calledSendRequest = false;
}

exports.$set('sendRequest', sendRequest);
exports.$set('getCalledSendRequest', getCalledSendRequest);
exports.$set('clearCalledSendRequest', clearCalledSendRequest);

// Called by C++.
exports.$set('handleResponse', handleResponse);
