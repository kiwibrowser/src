// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var Event = require('event_bindings').Event;
var forEach = require('utils').forEach;
// Note: Beware sneaky getters/setters when using GetAvailbility(). Use safe/raw
// variables as arguments.
var GetAvailability = requireNative('v8_context').GetAvailability;
var exceptionHandler = require('uncaught_exception_handler');
var lastError = require('lastError');
var loadTypeSchema = require('json_schema').loadTypeSchema;
var logActivity = requireNative('activityLogger');
var logging = requireNative('logging');
var process = requireNative('process');
var schemaRegistry = requireNative('schema_registry');
var schemaUtils = require('schemaUtils');
var sendRequestHandler = require('sendRequest');

var contextType = process.GetContextType();
var extensionId = process.GetExtensionId();
var manifestVersion = process.GetManifestVersion();
var platform = process.GetPlatform();
var sendRequest = sendRequestHandler.sendRequest;

// Stores the name and definition of each API function, with methods to
// modify their behaviour (such as a custom way to handle requests to the
// API, a custom callback, etc).
function APIFunctions(namespace) {
  this.apiFunctions_ = { __proto__: null };
  this.unavailableApiFunctions_ = { __proto__: null };
  this.namespace = namespace;
}

APIFunctions.prototype = {
  __proto__: null,
};

APIFunctions.prototype.register = function(apiName, apiFunction) {
  this.apiFunctions_[apiName] = apiFunction;
};

// Registers a function as existing but not available, meaning that calls to
// the set* methods that reference this function should be ignored rather
// than throwing Errors.
APIFunctions.prototype.registerUnavailable = function(apiName) {
  this.unavailableApiFunctions_[apiName] = apiName;
};

APIFunctions.prototype.setHook_ =
    function(apiName, propertyName, customizedFunction) {
  if ($Object.hasOwnProperty(this.unavailableApiFunctions_, apiName))
    return;
  if (!$Object.hasOwnProperty(this.apiFunctions_, apiName))
    throw new Error('Tried to set hook for unknown API "' + apiName + '"');
  this.apiFunctions_[apiName][propertyName] = customizedFunction;
};

APIFunctions.prototype.setHandleRequest =
    function(apiName, customizedFunction) {
  var prefix = this.namespace;
  return this.setHook_(apiName, 'handleRequest',
    function() {
      var ret = $Function.apply(customizedFunction, this, arguments);
      // Logs API calls to the Activity Log if it doesn't go through an
      // ExtensionFunction.
      if (!sendRequestHandler.getCalledSendRequest())
        logActivity.LogAPICall(extensionId, prefix + "." + apiName,
            $Array.slice(arguments));
      return ret;
    });
};

APIFunctions.prototype.setUpdateArgumentsPostValidate =
    function(apiName, customizedFunction) {
  return this.setHook_(
    apiName, 'updateArgumentsPostValidate', customizedFunction);
};

APIFunctions.prototype.setUpdateArgumentsPreValidate =
    function(apiName, customizedFunction) {
  return this.setHook_(
    apiName, 'updateArgumentsPreValidate', customizedFunction);
};

APIFunctions.prototype.setCustomCallback =
    function(apiName, customizedFunction) {
  return this.setHook_(apiName, 'customCallback', customizedFunction);
};

function isPlatformSupported(schemaNode, platform) {
  return !schemaNode.platforms ||
      $Array.indexOf(schemaNode.platforms, platform) > -1;
}

function isManifestVersionSupported(schemaNode, manifestVersion) {
  return !schemaNode.maximumManifestVersion ||
      manifestVersion <= schemaNode.maximumManifestVersion;
}

function isSchemaNodeSupported(schemaNode, platform, manifestVersion) {
  return isPlatformSupported(schemaNode, platform) &&
      isManifestVersionSupported(schemaNode, manifestVersion);
}

function createCustomType(type) {
  var jsModuleName = type.js_module;
  logging.CHECK(jsModuleName, 'Custom type ' + type.id +
                ' has no "js_module" property.');
  // This list contains all types that has a js_module property. It is ugly to
  // hard-code them here, but the number of APIs that use js_module has not
  // changed since the introduction of js_modules in crbug.com/222156.
  // This whitelist serves as an extra line of defence to avoid exposing
  // arbitrary extension modules when the |type| definition is poisoned.
  var whitelistedModules = [
    'ChromeSetting',
    'ContentSetting',
    'EasyUnlockProximityRequired',
    'StorageArea',
  ];
  logging.CHECK($Array.indexOf(whitelistedModules, jsModuleName) !== -1,
                'Module ' + jsModuleName + ' does not define a custom type.');
  var jsModule = require(jsModuleName);
  logging.CHECK(jsModule, 'No module ' + jsModuleName + ' found for ' +
                type.id + '.');
  var customType = jsModule[jsModuleName];
  logging.CHECK(customType, jsModuleName + ' must export itself.');
  return customType;
}

function Binding(apiName) {
  this.apiName_ = apiName;
  this.apiFunctions_ = new APIFunctions(apiName);
  this.customHooks_ = [];
};

$Object.defineProperty(Binding, 'create', {
  __proto__: null,
  configurable: false,
  enumerable: false,
  value: function(apiName) { return new Binding(apiName); },
  writable: false,
});

Binding.prototype = {
  // Sneaky workaround for Object.prototype getters/setters - our prototype
  // isn't Object.prototype. SafeBuiltins (e.g. $Object.hasOwnProperty())
  // should still work.
  __proto__: null,

  // Forward-declare properties.
  apiName_: undefined,
  apiFunctions_: undefined,
  customEvent_: undefined,
  customHooks_: undefined,

  // The API through which the ${api_name}_custom_bindings.js files customize
  // their API bindings beyond what can be generated.
  //
  // There are 2 types of customizations available: those which are required in
  // order to do the schema generation (registerCustomEvent and
  // registerCustomType), and those which can only run after the bindings have
  // been generated (registerCustomHook).

  // Registers a custom event type for the API identified by |namespace|.
  // |event| is the event's constructor.
  registerCustomEvent: function(event) {
    this.customEvent_ = event;
  },

  // Registers a function |hook| to run after the schema for all APIs has been
  // generated.  The hook is passed as its first argument an "API" object to
  // interact with, and second the current extension ID. See where
  // |customHooks| is used.
  registerCustomHook: function(fn) {
    $Array.push(this.customHooks_, fn);
  },

  // TODO(kalman/cduvall): Refactor this so |runHooks_| is not needed.
  runHooks_: function(api, schema) {
    $Array.forEach(this.customHooks_, function(hook) {
      if (!isSchemaNodeSupported(schema, platform, manifestVersion))
        return;

      if (!hook)
        return;

      hook({
        __proto__: null,
        apiFunctions: this.apiFunctions_,
        schema: schema,
        compiledApi: api
      }, extensionId, contextType);
    }, this);
  },

  // Generates the bindings from the schema for |this.apiName_| and integrates
  // any custom bindings that might be present.
  generate: function() {
    // NB: It's important to load the schema during generation rather than
    // setting it beforehand so that we're more confident the schema we're
    // loading is real, and not one that was injected by a page intercepting
    // Binding.generate.
    // Additionally, since the schema is an object returned from a native
    // handler, its properties don't have the custom getters/setters that a page
    // may have put on Object.prototype, and the object is frozen by v8.
    var schema = schemaRegistry.GetSchema(this.apiName_);

    function shouldCheckUnprivileged() {
      var shouldCheck = 'unprivileged' in schema;
      if (shouldCheck)
        return shouldCheck;

      $Array.forEach(['functions', 'events'], function(type) {
        if ($Object.hasOwnProperty(schema, type)) {
          $Array.forEach(schema[type], function(node) {
            if ('unprivileged' in node)
              shouldCheck = true;
          });
        }
      });
      if (shouldCheck)
        return shouldCheck;

      for (var property in schema.properties) {
        if ($Object.hasOwnProperty(schema, property) &&
            'unprivileged' in schema.properties[property]) {
          shouldCheck = true;
          break;
        }
      }
      return shouldCheck;
    }
    var checkUnprivileged = shouldCheckUnprivileged();

    // TODO(kalman/cduvall): Make GetAvailability handle this, then delete the
    // supporting code.
    if (!isSchemaNodeSupported(schema, platform, manifestVersion)) {
      console.error('chrome.' + schema.namespace + ' is not supported on ' +
                    'this platform or manifest version');
      return undefined;
    }

    var mod = {};

    var namespaces = $String.split(schema.namespace, '.');
    for (var index = 0, name; name = namespaces[index]; index++) {
      mod[name] = mod[name] || {};
      mod = mod[name];
    }

    if (schema.types) {
      $Array.forEach(schema.types, function(t) {
        if (!isSchemaNodeSupported(t, platform, manifestVersion))
          return;

        // Add types to global schemaValidator; the types we depend on from
        // other namespaces will be added as needed.
        schemaUtils.schemaValidator.addTypes(t);

        // Generate symbols for enums.
        var enumValues = t['enum'];
        if (enumValues) {
          // Type IDs are qualified with the namespace during compilation,
          // unfortunately, so remove it here.
          logging.DCHECK($String.substr(t.id, 0, schema.namespace.length) ==
                             schema.namespace);
          // Note: + 1 because it ends in a '.', e.g., 'fooApi.Type'.
          var id = $String.substr(t.id, schema.namespace.length + 1);
          mod[id] = {};
          $Array.forEach(enumValues, function(enumValue) {
            // Note: enums can be declared either as a list of strings
            // ['foo', 'bar'] or as a list of objects
            // [{'name': 'foo'}, {'name': 'bar'}].
            enumValue = $Object.hasOwnProperty(enumValue, 'name') ?
                enumValue.name : enumValue;
            if (enumValue) {  // Avoid setting any empty enums.
              // Make all properties in ALL_CAPS_STYLE.
              //
              // The built-in versions of $String.replace call other built-ins,
              // which may be clobbered. Instead, manually build the property
              // name.
              //
              // If the first character is a digit (we know it must be one of
              // a digit, a letter, or an underscore), precede it with an
              // underscore.
              var propertyName = ($RegExp.exec(/\d/, enumValue[0])) ? '_' : '';
              for (var i = 0; i < enumValue.length; ++i) {
                var next;
                if (i > 0 && $RegExp.exec(/[a-z]/, enumValue[i-1]) &&
                    $RegExp.exec(/[A-Z]/, enumValue[i])) {
                  // Replace myEnum-Foo with my_Enum-Foo:
                  next = '_' + enumValue[i];
                } else if ($RegExp.exec(/\W/, enumValue[i])) {
                  // Replace my_Enum-Foo with my_Enum_Foo:
                  next = '_';
                } else {
                  next = enumValue[i];
                }
                propertyName += next;
              }
              // Uppercase (replace my_Enum_Foo with MY_ENUM_FOO):
              propertyName = $String.toUpperCase(propertyName);
              mod[id][propertyName] = enumValue;
            }
          });
        }
      }, this);
    }

    // TODO(cduvall): Take out when all APIs have been converted to features.
    // Returns whether access to the content of a schema should be denied,
    // based on the presence of "unprivileged" and whether this is an
    // extension process (versus e.g. a content script).
    function isSchemaAccessAllowed(itemSchema) {
      return (contextType == 'BLESSED_EXTENSION') ||
             schema.unprivileged ||
             itemSchema.unprivileged;
    };

    // Setup Functions.
    if (schema.functions) {
      $Array.forEach(schema.functions, function(functionDef) {
        if (functionDef.name in mod) {
          throw new Error('Function ' + functionDef.name +
                          ' already defined in ' + schema.namespace);
        }

        if (!isSchemaNodeSupported(functionDef, platform, manifestVersion)) {
          this.apiFunctions_.registerUnavailable(functionDef.name);
          return;
        }

        var apiFunction = { __proto__: null };
        apiFunction.definition = functionDef;
        apiFunction.name = schema.namespace + '.' + functionDef.name;

        if (!GetAvailability(apiFunction.name).is_available ||
            (checkUnprivileged && !isSchemaAccessAllowed(functionDef))) {
          this.apiFunctions_.registerUnavailable(functionDef.name);
          return;
        }

        // TODO(aa): It would be best to run this in a unit test, but in order
        // to do that we would need to better factor this code so that it
        // doesn't depend on so much v8::Extension machinery.
        if (logging.DCHECK_IS_ON() &&
            schemaUtils.isFunctionSignatureAmbiguous(apiFunction.definition)) {
          throw new Error(
              apiFunction.name + ' has ambiguous optional arguments. ' +
              'To implement custom disambiguation logic, add ' +
              '"allowAmbiguousOptionalArguments" to the function\'s schema.');
        }

        this.apiFunctions_.register(functionDef.name, apiFunction);

        mod[functionDef.name] = $Function.bind(function() {
          var args = $Array.slice(arguments);
          $Object.setPrototypeOf(args, null);
          if (this.updateArgumentsPreValidate)
            args = $Function.apply(this.updateArgumentsPreValidate, this, args);

          args = schemaUtils.normalizeArgumentsAndValidate(args, this);
          if (this.updateArgumentsPostValidate) {
            args = $Function.apply(this.updateArgumentsPostValidate,
                                   this,
                                   args);
          }

          sendRequestHandler.clearCalledSendRequest();

          var retval;
          if (this.handleRequest) {
            retval = $Function.apply(this.handleRequest, this, args);
          } else {
            var optArgs = {
              __proto__: null,
              forIOThread: functionDef.forIOThread,
              customCallback: this.customCallback
            };
            retval = sendRequest(this.name, args,
                                 this.definition.parameters,
                                 optArgs);
          }
          sendRequestHandler.clearCalledSendRequest();

          // Validate return value if in sanity check mode.
          if (logging.DCHECK_IS_ON() && this.definition.returns)
            schemaUtils.validate([retval], [this.definition.returns]);
          return retval;
        }, apiFunction);
      }, this);
    }

    // Setup Events
    if (schema.events) {
      $Array.forEach(schema.events, function(eventDef) {
        if (eventDef.name in mod) {
          throw new Error('Event ' + eventDef.name +
                          ' already defined in ' + schema.namespace);
        }
        if (!isSchemaNodeSupported(eventDef, platform, manifestVersion))
          return;

        var eventName = schema.namespace + "." + eventDef.name;
        if (!GetAvailability(eventName).is_available ||
            (checkUnprivileged && !isSchemaAccessAllowed(eventDef))) {
          return;
        }

        var options = eventDef.options || {};
        if (eventDef.filters && eventDef.filters.length > 0)
          options.supportsFilters = true;

        var parameters = eventDef.parameters;
        if (this.customEvent_) {
          mod[eventDef.name] = new this.customEvent_(
              eventName, parameters, eventDef.extraParameters, options);
        } else {
          mod[eventDef.name] = new Event(eventName, parameters, options);
        }
      }, this);
    }

    function addProperties(m, parentDef) {
      var properties = parentDef.properties;
      if (!properties)
        return;

      forEach(properties, function(propertyName, propertyDef) {
        if (propertyName in m)
          return;  // TODO(kalman): be strict like functions/events somehow.
        if (!isSchemaNodeSupported(propertyDef, platform, manifestVersion))
          return;
        if (!GetAvailability(schema.namespace + "." +
              propertyName).is_available ||
            (checkUnprivileged && !isSchemaAccessAllowed(propertyDef))) {
          return;
        }

        // |value| is eventually added to |m|, the exposed API. Make copies
        // of everything from the schema. (The schema is also frozen, so as long
        // as we don't make any modifications, shallow copies are fine.)
        var value;
        if ($Array.isArray(propertyDef.value))
          value = $Array.slice(propertyDef.value);
        else if (typeof propertyDef.value === 'object')
          value = $Object.assign({}, propertyDef.value);
        else
          value = propertyDef.value;

        if (value) {
          // Values may just have raw types as defined in the JSON, such
          // as "WINDOW_ID_NONE": { "value": -1 }. We handle this here.
          // TODO(kalman): enforce that things with a "value" property can't
          // define their own types.
          var type = propertyDef.type || typeof(value);
          if (type === 'integer' || type === 'number') {
            value = parseInt(value);
          } else if (type === 'boolean') {
            value = value === 'true';
          } else if (propertyDef['$ref']) {
            var ref = propertyDef['$ref'];
            var type = loadTypeSchema(propertyDef['$ref'], schema);
            logging.CHECK(type, 'Schema for $ref type ' + ref + ' not found');
            var constructor = createCustomType(type);
            var args = value;
            logging.DCHECK($Array.isArray(args));
            $Array.push(args, type);
            // For an object propertyDef, |value| is an array of constructor
            // arguments, but we want to pass the arguments directly (i.e.
            // not as an array), so we have to fake calling |new| on the
            // constructor.
            value = { __proto__: constructor.prototype };
            $Function.apply(constructor, value, args);
            // Recursively add properties.
            addProperties(value, propertyDef);
          } else if (type === 'object') {
            // Recursively add properties.
            addProperties(value, propertyDef);
          } else if (type !== 'string') {
            throw new Error('NOT IMPLEMENTED (extension_api.json error): ' +
                'Cannot parse values for type "' + type + '"');
          }
          m[propertyName] = value;
        }
      });
    };

    addProperties(mod, schema);

    // This generate() call is considered successful if any functions,
    // properties, or events were created.
    var success = ($Object.keys(mod).length > 0);

    // Special case: webViewRequest is a vacuous API which just copies its
    // implementation from declarativeWebRequest.
    //
    // TODO(kalman): This would be unnecessary if we did these checks after the
    // hooks (i.e. this.runHooks_(mod)). The reason we don't is to be very
    // conservative with running any JS which might actually be for an API
    // which isn't available, but this is probably overly cautious given the
    // C++ is only giving us APIs which are available. FIXME.
    if (schema.namespace == 'webViewRequest') {
      success = true;
    }

    // Special case: runtime.lastError is only occasionally set, so
    // specifically check its availability.
    if (schema.namespace == 'runtime' &&
        GetAvailability('runtime.lastError').is_available) {
      success = true;
    }

    if (!success) {
      var availability = GetAvailability(schema.namespace);
      // If an API was available it should have been successfully generated.
      logging.DCHECK(!availability.is_available,
                     schema.namespace + ' was available but not generated');
      console.error('chrome.' + schema.namespace + ' is not available: ' +
                    availability.message);
      return;
    }

    this.runHooks_(mod, schema);
    return mod;
  }
};

exports.$set('Binding', Binding);
