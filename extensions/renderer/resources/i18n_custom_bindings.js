// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Custom binding for the i18n API.

var binding = apiBridge || require('binding').Binding.create('i18n');

var i18nNatives = requireNative('i18n');
var GetL10nMessage = i18nNatives.GetL10nMessage;
var GetL10nUILanguage = i18nNatives.GetL10nUILanguage;
var DetectTextLanguage = i18nNatives.DetectTextLanguage;

binding.registerCustomHook(function(bindingsAPI, extensionId) {
  var apiFunctions = bindingsAPI.apiFunctions;

  apiFunctions.setUpdateArgumentsPreValidate('getMessage', function() {
    var args = $Array.slice(arguments);

    // The first argument is the message, and should be a string.
    var message = args[0];
    if (typeof(message) !== 'string') {
      console.warn(extensionId + ': the first argument to getMessage should ' +
                   'be type "string", was ' + message +
                   ' (type "' + typeof(message) + '")');
      args[0] = String(message);
    }

    return args;
  });

  apiFunctions.setHandleRequest('getMessage',
                                function(messageName, substitutions) {
    return GetL10nMessage(messageName, substitutions, extensionId);
  });

  apiFunctions.setHandleRequest('getUILanguage', function() {
    return GetL10nUILanguage();
  });

  apiFunctions.setHandleRequest('detectLanguage', function(text, callback) {
    window.setTimeout(function() {
      var response = DetectTextLanguage(text);
      callback(response);
    }, 0);
  });
});

if (!apiBridge)
  exports.$set('binding', binding.generate());
