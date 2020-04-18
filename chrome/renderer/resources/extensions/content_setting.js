// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Custom binding for the contentSettings API.

var sendRequest = require('sendRequest').sendRequest;
var validate = require('schemaUtils').validate;

// Some content types have been removed and no longer correspond to a real
// content setting. Instead, these always return a fixed dummy value, and issue
// a warning when accessed. This maps the content type name to the dummy value.
var DEPRECATED_CONTENT_TYPES = {
  __proto__: null,

  fullscreen: 'allow',
  mouselock: 'allow',
};

function extendSchema(schema) {
  var extendedSchema = $Array.slice(schema);
  $Array.unshift(extendedSchema, {'type': 'string'});
  return extendedSchema;
}

function ContentSetting(contentType, settingSchema, schema) {
  var getFunctionParameters = function(name) {
    var f = $Array.filter(
                schema.functions, function(f) { return f.name === name; })[0];
    return f.parameters;
  };
  this.get = function(details, callback) {
    var getSchema = getFunctionParameters('get');
    validate([details, callback], getSchema);

    var dummySetting = DEPRECATED_CONTENT_TYPES[contentType];
    if (dummySetting !== undefined) {
      console.warn('contentSettings.' + contentType + ' is deprecated; it will '
                   + 'always return \'' + dummySetting + '\'.');
      $Function.apply(callback, undefined, [{setting: dummySetting}]);
      return;
    }

    return sendRequest('contentSettings.get',
                       [contentType, details, callback],
                       extendSchema(getSchema));
  };

  this.set = function(details, callback) {
    // We check if the setting is deprecated first, since the validation will
    // fail for deprecated types.
    if ($Object.hasOwnProperty(DEPRECATED_CONTENT_TYPES, contentType)) {
      console.warn('contentSettings.' + contentType + ' is deprecated; setting '
                   + 'it has no effect.');
      $Function.apply(callback, undefined, []);
      return;
    }

    // The set schema included in the Schema object is generic, since it varies
    // per-setting. However, this is only ever for a single setting, so we can
    // enforce the types more thoroughly.
    var rawSetSchema = getFunctionParameters('set');
    var rawSettingParam = rawSetSchema[0];
    var props = $Object.assign({}, rawSettingParam.properties);
    props.setting = settingSchema;
    var modSettingParam = {
      name: rawSettingParam.name,
      type: rawSettingParam.type,
      properties: props,
    };
    var modSetSchema = $Array.slice(rawSetSchema);
    modSetSchema[0] = modSettingParam;
    validate([details, callback], modSetSchema);

    return sendRequest('contentSettings.set',
                       [contentType, details, callback],
                       extendSchema(modSetSchema));
  };

  this.clear = function(details, callback) {
    var clearSchema = getFunctionParameters('clear');
    validate([details, callback], clearSchema);

    if ($Object.hasOwnProperty(DEPRECATED_CONTENT_TYPES, contentType)) {
      console.warn('contentSettings.' + contentType + ' is deprecated; '
                   + 'clearing it has no effect.');
      $Function.apply(callback, undefined, []);
      return;
    }

    return sendRequest('contentSettings.clear',
                       [contentType, details, callback],
                       extendSchema(clearSchema));
  };

  this.getResourceIdentifiers = function(callback) {
    var schema = getFunctionParameters('getResourceIdentifiers');
    validate([callback], schema);

    if ($Object.hasOwnProperty(DEPRECATED_CONTENT_TYPES, contentType)) {
      $Function.apply(callback, undefined, []);
      return;
    }

    return sendRequest(
        'contentSettings.getResourceIdentifiers',
        [contentType, callback],
        extendSchema(schema));
  };
}

exports.$set('ContentSetting', ContentSetting);
