// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Custom binding for the runtime API.

var binding = apiBridge || require('binding').Binding.create('runtime');

var messaging = require('messaging');
var runtimeNatives = requireNative('runtime');
var messagingNatives = requireNative('messaging_natives');
var process = requireNative('process');
var utils = require('utils');
var getBindDirectoryEntryCallback =
    require('fileEntryBindingUtil').getBindDirectoryEntryCallback;

binding.registerCustomHook(function(binding, id, contextType) {
  var apiFunctions = binding.apiFunctions;
  var runtime = binding.compiledApi;

  //
  // Unprivileged APIs.
  //

  if (id != '')
    utils.defineProperty(runtime, 'id', id);

  apiFunctions.setHandleRequest('getManifest', function() {
    return runtimeNatives.GetManifest();
  });

  apiFunctions.setHandleRequest('getURL', function(path) {
    path = $String.self(path);
    if (!path.length || path[0] != '/')
      path = '/' + path;
    return 'chrome-extension://' + id + path;
  });

  var sendMessageUpdateArguments = messaging.sendMessageUpdateArguments;
  apiFunctions.setUpdateArgumentsPreValidate(
      'sendMessage',
      $Function.bind(sendMessageUpdateArguments, null, 'sendMessage',
                     true /* hasOptionsArgument */));
  apiFunctions.setUpdateArgumentsPreValidate(
      'sendNativeMessage',
      $Function.bind(sendMessageUpdateArguments, null, 'sendNativeMessage',
                     false /* hasOptionsArgument */));

  apiFunctions.setHandleRequest(
      'sendMessage',
      function(targetId, message, options, responseCallback) {
    var connectOptions = $Object.assign({
      __proto__: null,
      name: messaging.kMessageChannel,
    }, options);
    var port = runtime.connect(targetId, connectOptions);
    messaging.sendMessageImpl(port, message, responseCallback);
  });

  apiFunctions.setHandleRequest('sendNativeMessage',
                                function(targetId, message, responseCallback) {
    var port = runtime.connectNative(targetId);
    messaging.sendMessageImpl(port, message, responseCallback);
  });

  apiFunctions.setHandleRequest('connect', function(targetId, connectInfo) {
    if (!targetId) {
      // id is only defined inside extensions. If we're in a webpage, the best
      // we can do at this point is to fail.
      if (!id) {
        throw new Error('chrome.runtime.connect() called from a webpage must ' +
                        'specify an Extension ID (string) for its first ' +
                        'argument');
      }
      targetId = id;
    }

    var name = '';
    if (connectInfo && connectInfo.name)
      name = connectInfo.name;

    var includeTlsChannelId =
      !!(connectInfo && connectInfo.includeTlsChannelId);

    var portId = messagingNatives.OpenChannelToExtension(targetId, name,
                                                         includeTlsChannelId);
    if (portId >= 0)
      return messaging.createPort(portId, name);
  });

  //
  // Privileged APIs.
  //
  if (contextType != 'BLESSED_EXTENSION')
    return;

  apiFunctions.setHandleRequest('connectNative',
                                function(nativeAppName) {
    var portId = messagingNatives.OpenChannelToNativeApp(nativeAppName);
    if (portId >= 0)
      return messaging.createPort(portId, '');
    throw new Error('Error connecting to native app: ' + nativeAppName);
  });

  apiFunctions.setCustomCallback('getBackgroundPage',
                                 function(name, request, callback, response) {
    if (callback) {
      var bg =
          runtimeNatives.GetExtensionViews(-1, -1, 'BACKGROUND')[0] || null;
      callback(bg);
    }
  });

  apiFunctions.setCustomCallback('getPackageDirectoryEntry',
                                 getBindDirectoryEntryCallback());
});

if (!apiBridge)
  exports.$set('binding', binding.generate());
