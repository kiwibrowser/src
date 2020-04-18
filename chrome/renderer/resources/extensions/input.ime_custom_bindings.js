// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Custom binding for the input ime API. Only injected into the
// v8 contexts for extensions which have permission for the API.

var binding = apiBridge || require('binding').Binding.create('input.ime');
var appWindowNatives = requireNative('app_window_natives');
var registerArgumentMassager = bindingUtil ?
    $Function.bind(bindingUtil.registerEventArgumentMassager, bindingUtil) :
    require('event_bindings').registerArgumentMassager;

// TODO(crbug.com/733825): These bindings have some issues.

var inputIme;
registerArgumentMassager('input.ime.onKeyEvent',
                         function(args, dispatch) {
  var keyData = args[1];
  var result = false;
  try {
    // dispatch() is weird - it returns an object {results: array<results>} iff
    // there is at least one result value that !== undefined. Since onKeyEvent
    // has a maximum of one listener, we know that any result we find is the one
    // we're interested in.
    var dispatchResult = dispatch(args);
    if (dispatchResult && dispatchResult.results)
      result = dispatchResult.results[0];
  } catch (e) {
    console.error('Error in event handler for onKeyEvent: ' + e.stack);
  }
  if (!inputIme.onKeyEvent.async)
    inputIme.keyEventHandled(keyData.requestId, result);
});

binding.registerCustomHook(function(api) {
 inputIme = api.compiledApi;

  var originalAddListener = inputIme.onKeyEvent.addListener;
  inputIme.onKeyEvent.addListener = function(cb, opt_extraInfo) {
    inputIme.onKeyEvent.async = false;
    if (opt_extraInfo instanceof Array) {
      for (var i = 0; i < opt_extraInfo.length; ++i) {
        if (opt_extraInfo[i] == 'async') {
          inputIme.onKeyEvent.async = true;
        }
      }
    }
    $Function.call(originalAddListener, this, cb);
  };

  api.apiFunctions.setCustomCallback('createWindow',
      function(name, request, callback, windowParams) {
    if (!callback) {
      return;
    }
    var view;
    if (windowParams && windowParams.frameId) {
      view = appWindowNatives.GetFrame(
          windowParams.frameId, false /* notifyBrowser */);
      view.id = windowParams.frameId;
    }
    callback(view);
  });
});

if (!apiBridge)
  exports.$set('binding', binding.generate());
