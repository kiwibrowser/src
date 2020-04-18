// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var Event = require('event_bindings').Event;
var sendRequest = require('sendRequest').sendRequest;
var validate = require('schemaUtils').validate;

function extendSchema(schema) {
  var extendedSchema = $Array.slice(schema);
  $Array.unshift(extendedSchema, {'type': 'string'});
  return extendedSchema;
}

// TODO(devlin): Maybe find a way to combine this and ContentSetting.
function ChromeSetting(prefKey, valueSchema, schema) {
  var getFunctionParameters = function(name) {
    var f = $Array.filter(
                schema.functions, function(f) { return f.name === name; })[0];
    return f.parameters;
  };
  this.get = function(details, callback) {
    var getSchema = getFunctionParameters('get');
    validate([details, callback], getSchema);
    return sendRequest('types.ChromeSetting.get',
                       [prefKey, details, callback],
                       extendSchema(getSchema));
  };
  this.set = function(details, callback) {
    // The set schema included in the Schema object is generic, since it varies
    // per-setting. However, this is only ever for a single setting, so we can
    // enforce the types more thoroughly.
    var rawSetSchema = getFunctionParameters('set');
    var rawSettingParam = rawSetSchema[0];
    var props = $Object.assign({}, rawSettingParam.properties);
    props.value = valueSchema;
    var modSettingParam = {
      name: rawSettingParam.name,
      type: rawSettingParam.type,
      properties: props,
    };
    var modSetSchema = $Array.slice(rawSetSchema);
    modSetSchema[0] = modSettingParam;
    validate([details, callback], modSetSchema);
    return sendRequest('types.ChromeSetting.set',
                       [prefKey, details, callback],
                       extendSchema(modSetSchema));
  };
  this.clear = function(details, callback) {
    var clearSchema = getFunctionParameters('clear');
    validate([details, callback], clearSchema);
    return sendRequest('types.ChromeSetting.clear',
                       [prefKey, details, callback],
                       extendSchema(clearSchema));
  };
  this.onChange = new Event('types.ChromeSetting.' + prefKey + '.onChange');
};

exports.$set('ChromeSetting', ChromeSetting);
