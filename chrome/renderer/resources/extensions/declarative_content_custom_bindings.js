// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Custom binding for the declarativeContent API.

var binding =
    apiBridge || require('binding').Binding.create('declarativeContent');

if (!apiBridge) {
  var utils = require('utils');
  var validate = require('schemaUtils').validate;
  var canonicalizeCompoundSelector =
      requireNative('css_natives').CanonicalizeCompoundSelector;
}

var setIcon = require('setIcon').setIcon;

binding.registerCustomHook(function(api) {
  var declarativeContent = api.compiledApi;

  if (apiBridge) {
    // Validation for most types is done in the native C++ with native bindings,
    // but setIcon is funny (and sadly broken). Ideally, we can move this
    // validation entirely into the native code, and this whole file can go
    // away.
    var nativeSetIcon = declarativeContent.SetIcon;
    declarativeContent.SetIcon = function(parameters) {
      // TODO(devlin): This is very, very wrong. setIcon() is potentially
      // asynchronous (in the case of a path being specified), which means this
      // becomes an "asynchronous constructor". Errors can be thrown *after* the
      // `new declarativeContent.SetIcon(...)` call, and in the async cases,
      // this wouldn't work when we immediately add the action via an API call
      // (e.g.,
      //   chrome.declarativeContent.onPageChange.addRules(
      //       [{conditions: ..., actions: [ new SetIcon(...) ]}]);
      // ). Some of this is tracked in http://crbug.com/415315.
      setIcon(parameters, $Function.bind(function(data) {
        // Fake calling the original function as a constructor.
        $Object.setPrototypeOf(this, nativeSetIcon.prototype);
        $Function.apply(nativeSetIcon, this, [data]);
      }, this));
    };
    return;
  }

  // Returns the schema definition of type |typeId| defined in |namespace|.
  function getSchema(typeId) {
    return utils.lookup(api.schema.types,
                        'id',
                        'declarativeContent.' + typeId);
  }

  // Helper function for the constructor of concrete datatypes of the
  // declarative content API.
  // Makes sure that |this| contains the union of parameters and
  // {'instanceType': 'declarativeContent.' + typeId} and validates the
  // generated union dictionary against the schema for |typeId|.
  function setupInstance(instance, parameters, typeId) {
    for (var key in parameters) {
      if ($Object.hasOwnProperty(parameters, key)) {
        instance[key] = parameters[key];
      }
    }
    instance.instanceType = 'declarativeContent.' + typeId;
    var schema = getSchema(typeId);
    validate([instance], [schema]);
  }

  function canonicalizeCssSelectors(selectors) {
    for (var i = 0; i < selectors.length; i++) {
      var canonicalizedSelector = canonicalizeCompoundSelector(selectors[i]);
      if (canonicalizedSelector == '') {
        throw new Error(
            'Element of \'css\' array must be a ' +
            'list of valid compound selectors: ' +
            selectors[i]);
      }
      selectors[i] = canonicalizedSelector;
    }
  }

  // Setup all data types for the declarative content API.
  declarativeContent.PageStateMatcher = function(parameters) {
    setupInstance(this, parameters, 'PageStateMatcher');
    if ($Object.hasOwnProperty(this, 'css')) {
      canonicalizeCssSelectors(this.css);
    }
  };
  declarativeContent.ShowPageAction = function(parameters) {
    setupInstance(this, parameters, 'ShowPageAction');
  };
  declarativeContent.RequestContentScript = function(parameters) {
    setupInstance(this, parameters, 'RequestContentScript');
  };
  // TODO(rockot): Do not expose this in M39 stable. Making this restriction
  // possible will take some extra work. See http://crbug.com/415315
  // Note: See also the SetIcon wrapper above for more issues.
  declarativeContent.SetIcon = function(parameters) {
    setIcon(parameters, $Function.bind(function(data) {
      setupInstance(this, data, 'SetIcon');
    }, this));
  };
});

if (!apiBridge)
  exports.$set('binding', binding.generate());
