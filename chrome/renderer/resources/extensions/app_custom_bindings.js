// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Custom binding for the app API.

var appNatives = requireNative('app');
var process = requireNative('process');
var extensionId = process.GetExtensionId();
var logActivity = requireNative('activityLogger');

function wrapForLogging(fun) {
  if (!extensionId)
    return fun;  // nothing interesting to log without an extension

  return function() {
    // TODO(ataly): We need to make sure we use the right prototype for
    // fun.apply. Array slice can either be rewritten or similarly defined.
    logActivity.LogAPICall(extensionId, "app." + fun.name,
        $Array.slice(arguments));
    return $Function.apply(fun, this, arguments);
  };
}

// This becomes chrome.app
var app = {
  getIsInstalled: wrapForLogging(appNatives.GetIsInstalled),
  getDetails: wrapForLogging(appNatives.GetDetails),
  runningState: wrapForLogging(appNatives.GetRunningState)
};

// Tricky; "getIsInstalled" is actually exposed as the getter "isInstalled",
// but we don't have a way to express this in the schema JSON (nor is it
// worth it for this one special case).
//
// So, define it manually, and let the getIsInstalled function act as its
// documentation.
var isInstalled = wrapForLogging(appNatives.GetIsInstalled);
$Object.defineProperty(
    app, 'isInstalled',
    {
      __proto__: null,
      configurable: true,
      enumerable: true,
      get: function() { return isInstalled(); },
    });

// Called by app_bindings.cc.
function onInstallStateResponse(state, callbackId) {
  var callback = callbacks[callbackId];
  delete callbacks[callbackId];
  if (typeof callback == 'function') {
    try {
      callback(state);
    } catch (e) {
      console.error('Exception in chrome.app.installState response handler: ' +
                    e.stack);
    }
  }
}

// TODO(kalman): move this stuff to its own custom bindings.
var callbacks = { __proto__: null };
var nextCallbackId = 1;

function getInstallState(callback) {
  var callbackId = nextCallbackId++;
  callbacks[callbackId] = callback;
  appNatives.GetInstallState(callbackId);
}

$Object.defineProperty(
    app, 'installState',
    {
      __proto__: null,
      configurable: true,
      enumerable: true,
      value: wrapForLogging(getInstallState),
      writable: true,
    });

exports.$set('binding', app);
exports.$set('onInstallStateResponse', onInstallStateResponse);
