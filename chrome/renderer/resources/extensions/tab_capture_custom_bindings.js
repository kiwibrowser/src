// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Custom binding for the Tab Capture API.

var binding = apiBridge || require('binding').Binding.create('tabCapture');

var jsLastError = bindingUtil ? undefined : require('lastError');
function runCallbackWithLastError(name, message, stack, callback, args) {
  if (bindingUtil) {
    bindingUtil.runCallbackWithLastError(message, function() {
      $Function.apply(callback, null, args);
    });
  } else {
    jsLastError.run(name, message, stack, callback, args);
  }
}

binding.registerCustomHook(function(bindingsAPI, extensionId) {
  var apiFunctions = bindingsAPI.apiFunctions;

  function proxyToGetUserMedia(name, request, callback, response) {
    if (!callback)
      return;

    if (!response) {
      // When the response is missing, runtime.lastError has already been set.
      // See chrome/browser/extensions/api/tab_capture/tab_capture_api.cc.
      callback(null);
      return;
    }

    // Convenience function for processing webkitGetUserMedia() error objects to
    // provide runtime.lastError messages for the tab capture API.
    function getErrorMessage(error, fallbackMessage) {
      if (!error || (typeof error.message != 'string'))
        return fallbackMessage;
      return error.message.replace(/(navigator\.)?(webkit)?GetUserMedia/gi,
                                   name);
    }

    var options = {};
    if (response.audioConstraints)
      options.audio = response.audioConstraints;
    if (response.videoConstraints)
      options.video = response.videoConstraints;
    try {
      navigator.webkitGetUserMedia(
          options,
          function onSuccess(media_stream) {
            callback(media_stream);
          },
          function onError(error) {
            runCallbackWithLastError(
                name,
                getErrorMessage(error, "Failed to start MediaStream."),
                request.stack,
                function() { callback(null); });
          });
    } catch (error) {
      runCallbackWithLastError(
          name, getErrorMessage(error, "Invalid argument(s)."), request.stack,
          function() { callback(null); });
    }
  }

  apiFunctions.setCustomCallback('capture', proxyToGetUserMedia);
  apiFunctions.setCustomCallback('captureOffscreenTab', proxyToGetUserMedia);
});

if (!apiBridge)
  exports.$set('binding', binding.generate());
