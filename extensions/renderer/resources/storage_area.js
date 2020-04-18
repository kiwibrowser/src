// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var normalizeArgumentsAndValidate =
    require('schemaUtils').normalizeArgumentsAndValidate
var sendRequest = require('sendRequest').sendRequest;

function extendSchema(schema) {
  var extendedSchema = $Array.slice(schema);
  $Array.unshift(extendedSchema, {'type': 'string'});
  return extendedSchema;
}

// TODO(devlin): Combine parts of this and other custom types (ChromeSetting,
// ContentSetting, etc).
function StorageArea(namespace, schema) {
  // Binds an API function for a namespace to its browser-side call, e.g.
  // storage.sync.get('foo') -> (binds to) ->
  // storage.get('sync', 'foo').
  var self = this;
  function bindApiFunction(functionName) {
    var rawFunSchema =
        $Array.filter(schema.functions,
                      function(f) { return f.name === functionName; })[0];
    // normalizeArgumentsAndValidate expects a function schema of the form
    // { name: <name>, definition: <definition> }.
    var funSchema = {
      __proto__: null,
      name: rawFunSchema.name,
      definition: rawFunSchema
    };
    self[functionName] = function() {
      var args = $Array.slice(arguments);
      args = normalizeArgumentsAndValidate(args, funSchema);
      return sendRequest(
          'storage.' + functionName,
          $Array.concat([namespace], args),
          extendSchema(funSchema.definition.parameters),
          {__proto__: null, preserveNullInObjects: true});
    };
  }
  var apiFunctions = ['get', 'set', 'remove', 'clear', 'getBytesInUse'];
  $Array.forEach(apiFunctions, bindApiFunction);
}

exports.$set('StorageArea', StorageArea);
