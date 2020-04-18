// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Custom binding for the webViewRequest API.

var binding = apiBridge || require('binding').Binding.create('webViewRequest');

var declarativeWebRequestSchema =
    requireNative('schema_registry').GetSchema('declarativeWebRequest');

var utils = bindingUtil ? undefined : require('utils');
var validate = bindingUtil ? undefined : require('schemaUtils').validate;

function validateType(schemaTypes, typeName, value) {
  if (bindingUtil) {
    bindingUtil.validateType(typeName, value);
  } else {
    var schema = utils.lookup(schemaTypes, 'id', typeName);
    validate([value], [schema]);
  }
}

binding.registerCustomHook(function(api) {
  var webViewRequest = api.compiledApi;

  // Helper function for the constructor of concrete datatypes of the
  // declarative webRequest API.
  // Makes sure that |this| contains the union of parameters and
  // {'instanceType': 'declarativeWebRequest.' + typeId} and validates the
  // generated union dictionary against the schema for |typeId|.
  function setupInstance(instance, parameters, typeId) {
    for (var key in parameters) {
      if ($Object.hasOwnProperty(parameters, key)) {
        instance[key] = parameters[key];
      }
    }

    var qualifiedType = 'declarativeWebRequest.' + typeId;
    instance.instanceType = qualifiedType;
    validateType(bindingUtil ? undefined : declarativeWebRequestSchema.types,
                 qualifiedType, instance);
  }

  // Setup all data types for the declarative webRequest API from the schema.
  for (var i = 0; i < declarativeWebRequestSchema.types.length; ++i) {
    var typeSchema = declarativeWebRequestSchema.types[i];
    var typeId = $String.replace(typeSchema.id, 'declarativeWebRequest.', '');
    var action = function(typeId) {
      return function(parameters) {
        setupInstance(this, parameters, typeId);
      };
    }(typeId);
    webViewRequest[typeId] = action;
  }
});

if (!apiBridge)
  exports.$set('binding', binding.generate());
