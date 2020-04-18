// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var binding = apiBridge || require('binding').Binding.create('terminalPrivate');
var registerArgumentMassager = bindingUtil ?
    $Function.bind(bindingUtil.registerEventArgumentMassager, bindingUtil) :
    require('event_bindings').registerArgumentMassager;

// Custom bindings for chrome.terminalPrivate API.
registerArgumentMassager('terminalPrivate.onProcessOutput',
                         function(args, dispatch) {
  var tabId = args[0];
  var terminalId = args[1];
  try {
    // Remove tabId from event args, as it's not expected by listeners.
    dispatch(args.slice(1));
  } finally {
    chrome.terminalPrivate.ackOutput(tabId, terminalId);
  }
});

if (!apiBridge)
  exports.$set('binding', binding.generate());
