// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Custom binding for the Display Source API.

var binding = apiBridge || require('binding').Binding.create('displaySource');
var chrome = requireNative('chrome').GetChrome();
var natives = requireNative('display_source');
var logging = requireNative('logging');

var jsLastError = bindingUtil ? undefined : require('lastError');
function setLastError(name, message) {
  if (bindingUtil)
    bindingUtil.setLastError(message);
  else
    jsLastError.set(name, message, null, chrome);
}
function clearLastError() {
  if (bindingUtil)
    bindingUtil.clearLastError();
  else
    jsLastError.clear(chrome);
}

var callbacksInfo = {};

function callbackWrapper(callback, method, message) {
  if (callback == undefined)
    return;

  try {
    if (message !== null)
      setLastError(method, message);
    callback();
  } finally {
    clearLastError();
  }
}

function callCompletionCallback(callbackId, error_message) {
  try {
    var callbackInfo = callbacksInfo[callbackId];
    logging.DCHECK(callbackInfo != null);
    callbackWrapper(callbackInfo.callback, callbackInfo.method, error_message);
  } finally {
    delete callbacksInfo[callbackId];
  }
}

binding.registerCustomHook(function(bindingsAPI, extensionId) {
  var apiFunctions = bindingsAPI.apiFunctions;
  apiFunctions.setHandleRequest(
      'startSession', function(sessionInfo, callback) {
        try {
          var callId = natives.StartSession(sessionInfo);
          callbacksInfo[callId] = {
            callback: callback,
            method: 'displaySource.startSession'
          };
        } catch (e) {
          callbackWrapper(callback, 'displaySource.startSession', e.message);
        }
      });
  apiFunctions.setHandleRequest(
      'terminateSession', function(sink_id, callback) {
        try {
          var callId = natives.TerminateSession(sink_id);
          callbacksInfo[callId] = {
            callback: callback,
            method: 'displaySource.terminateSession'
          };
        } catch (e) {
          callbackWrapper(
              callback, 'displaySource.terminateSession', e.message);
        }
      });
});

if (!apiBridge)
  exports.$set('binding', binding.generate());

// Called by C++.
exports.$set('callCompletionCallback', callCompletionCallback);
