// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Custom binding for the tabs API.

var binding = apiBridge || require('binding').Binding.create('tabs');

var messaging = require('messaging');
var OpenChannelToTab = requireNative('messaging_natives').OpenChannelToTab;
var sendRequestIsDisabled = requireNative('process').IsSendRequestDisabled();
var forEach = require('utils').forEach;

binding.registerCustomHook(function(bindingsAPI, extensionId) {
  var apiFunctions = bindingsAPI.apiFunctions;
  var tabs = bindingsAPI.compiledApi;

  apiFunctions.setHandleRequest('connect', function(tabId, connectInfo) {
    var name = '';
    var frameId = -1;
    if (connectInfo) {
      name = connectInfo.name || name;
      frameId = connectInfo.frameId;
      if (typeof frameId == 'undefined' || frameId === null || frameId < 0)
        frameId = -1;
    }
    var portId = OpenChannelToTab(tabId, frameId, extensionId, name);
    return messaging.createPort(portId, name);
  });

  apiFunctions.setHandleRequest('sendRequest',
                                function(tabId, request, responseCallback) {
    if (sendRequestIsDisabled)
      throw new Error(sendRequestIsDisabled);
    var port = tabs.connect(tabId, {name: messaging.kRequestChannel});
    messaging.sendMessageImpl(port, request, responseCallback);
  });

  apiFunctions.setHandleRequest('sendMessage',
      function(tabId, message, options, responseCallback) {
    var connectInfo = {
      name: messaging.kMessageChannel
    };
    if (options) {
      forEach(options, function(k, v) {
        connectInfo[k] = v;
      });
    }

    var port = tabs.connect(tabId, connectInfo);
    messaging.sendMessageImpl(port, message, responseCallback);
  });
});

if (!apiBridge)
  exports.$set('binding', binding.generate());
