// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Custom binding for the ttsEngine API.

var binding = apiBridge || require('binding').Binding.create('ttsEngine');
var registerArgumentMassager = bindingUtil ?
    $Function.bind(bindingUtil.registerEventArgumentMassager, bindingUtil) :
    require('event_bindings').registerArgumentMassager;

registerArgumentMassager('ttsEngine.onSpeak', function(args, dispatch) {
  var text = args[0];
  var options = args[1];
  var requestId = args[2];
  var sendTtsEvent = function(event) {
    chrome.ttsEngine.sendTtsEvent(requestId, event);
  };
  dispatch([text, options, sendTtsEvent]);
});

if (!apiBridge)
  exports.$set('binding', binding.generate());
