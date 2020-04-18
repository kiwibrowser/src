/*
 * Copyright 2009, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/**
 * @fileoverview Base for all o3d sample utilties.
 *    For more information about o3d see
 *    http://code.google.com/p/o3d.
 *
 *
 * The main point of this module is to provide a central place to
 * have an init function to register an o3d namespace object because many other
 * modules need access to it.
 */

/**
 * A namespace for all the o3djs utility libraries.
 * @namespace
 */
var o3djs = o3djs || {};

/**
 * Define this because the Google internal JSCompiler needs goog.typedef below.
 */
var goog = goog || {};

/**
 * A macro for defining composite types.
 *
 * By assigning goog.typedef to a name, this tells Google internal JSCompiler
 * that this is not the name of a class, but rather it's the name of a composite
 * type.
 *
 * For example,
 * /** @type {Array|NodeList} / goog.ArrayLike = goog.typedef;
 * will tell JSCompiler to replace all appearances of goog.ArrayLike in type
 * definitions with the union of Array and NodeList.
 *
 * Does nothing in uncompiled code.
 */
goog.typedef = true;

/**
 * Reference to the global context.  In most cases this will be 'window'.
 */
o3djs.global = this;

/**
 * Flag used to force a function to run in the browser when it is called
 * from V8.
 * @type {boolean}
 */
o3djs.BROWSER_ONLY = true;

/**
 * Array of namespaces that have been provided.
 * @private
 * @type {!Array.<string>}
 */
o3djs.provided_ = [];

/**
 * Creates object stubs for a namespace. When present in a file,
 * o3djs.provide also indicates that the file defines the indicated
 * object.
 * @param {string} name name of the object that this file defines.
 */
o3djs.provide = function(name) {
  // Ensure that the same namespace isn't provided twice.
  if (o3djs.getObjectByName(name) &&
      !o3djs.implicitNamespaces_[name]) {
    throw 'Namespace "' + name + '" already declared.';
  }

  var namespace = name;
  while ((namespace = namespace.substring(0, namespace.lastIndexOf('.')))) {
    o3djs.implicitNamespaces_[namespace] = true;
  }

  o3djs.exportPath_(name);
  o3djs.provided_.push(name);
};


/**
 * Namespaces implicitly defined by o3djs.provide. For example,
 * o3djs.provide('o3djs.events.Event') implicitly declares
 * that 'o3djs' and 'o3djs.events' must be namespaces.
 *
 * @type {Object}
 * @private
 */
o3djs.implicitNamespaces_ = {};

/**
 * Builds an object structure for the provided namespace path,
 * ensuring that names that already exist are not overwritten. For
 * example:
 * "a.b.c" -> a = {};a.b={};a.b.c={};
 * Used by o3djs.provide and o3djs.exportSymbol.
 * @param {string} name name of the object that this file defines.
 * @param {Object} opt_object the object to expose at the end of the path.
 * @param {Object} opt_objectToExportTo The object to add the path to; default
 *     is |o3djs.global|.
 * @private
 */
o3djs.exportPath_ = function(name, opt_object, opt_objectToExportTo) {
  var parts = name.split('.');
  var cur = opt_objectToExportTo || o3djs.global;
  var part;

  // Internet Explorer exhibits strange behavior when throwing errors from
  // methods externed in this manner.  See the testExportSymbolExceptions in
  // base_test.html for an example.
  if (!(parts[0] in cur) && cur.execScript) {
    cur.execScript('var ' + parts[0]);
  }

  // Parentheses added to eliminate strict JS warning in Firefox.
  while (parts.length && (part = parts.shift())) {
    if (!parts.length && o3djs.isDef(opt_object)) {
      // last part and we have an object; use it.
      cur[part] = opt_object;
    } else if (cur[part]) {
      cur = cur[part];
    } else {
      cur = cur[part] = {};
    }
  }
};


/**
 * Returns an object based on its fully qualified external name.  If you are
 * using a compilation pass that renames property names beware that using this
 * function will not find renamed properties.
 *
 * @param {string} name The fully qualified name.
 * @param {Object} opt_obj The object within which to look; default is
 *     |o3djs.global|.
 * @return {Object} The object or, if not found, null.
 */
o3djs.getObjectByName = function(name, opt_obj) {
  var parts = name.split('.');
  var cur = opt_obj || o3djs.global;
  for (var pp = 0; pp < parts.length; ++pp) {
    var part = parts[pp];
    if (cur[part]) {
      cur = cur[part];
    } else {
      return null;
    }
  }
  return cur;
};


/**
 * Implements a system for the dynamic resolution of dependencies.
 * @param {string} rule Rule to include, in the form o3djs.package.part.
 */
o3djs.require = function(rule) {
  // TODO(gman): For some unknown reason, when we call
  // o3djs.util.getScriptTagText_ it calls
  // document.getElementsByTagName('script') and for some reason the scripts do
  // not always show up. Calling it here seems to fix that as long as we
  // actually ask for the length, at least in FF 3.5.1 It would be nice to
  // figure out why.
  var dummy = document.getElementsByTagName('script').length;

  // if the object already exists we do not need do do anything
  if (o3djs.getObjectByName(rule)) {
    return;
  }
  var path = o3djs.getPathFromRule_(rule);
  if (path) {
    o3djs.included_[path] = true;
    o3djs.writeScripts_();
  } else {
    throw new Error('o3djs.require could not find: ' + rule);
  }
};


/**
 * Path for included scripts.
 * @type {string}
 */
o3djs.basePath = '';


/**
 * Object used to keep track of urls that have already been added. This
 * record allows the prevention of circular dependencies.
 * @type {Object}
 * @private
 */
o3djs.included_ = {};


/**
 * This object is used to keep track of dependencies and other data that is
 * used for loading scripts.
 * @private
 * @type {Object}
 */
o3djs.dependencies_ = {
  visited: {},  // used when resolving dependencies to prevent us from
                // visiting the file twice.
  written: {}  // used to keep track of script files we have written.
};


/**
 * Tries to detect the base path of the o3djs-base.js script that
 * bootstraps the o3djs libraries.
 * @private
 */
o3djs.findBasePath_ = function() {
  var doc = o3djs.global.document;
  if (typeof doc == 'undefined') {
    return;
  }
  if (o3djs.global.BASE_PATH) {
    o3djs.basePath = o3djs.global.BASE_PATH;
    return;
  } else {
    // HACKHACK to hide compiler warnings :(
    o3djs.global.BASE_PATH = null;
  }
  var scripts = doc.getElementsByTagName('script');
  for (var script, i = 0; script = scripts[i]; i++) {
    var src = script.src;
    var l = src.length;
    if (src.substr(l - 13) == 'o3djs/base.js') {
      o3djs.basePath = src.substr(0, l - 13);
      return;
    }
  }
};


/**
 * Writes a script tag if, and only if, that script hasn't already been added
 * to the document.  (Must be called at execution time.)
 * @param {string} src Script source.
 * @private
 */
o3djs.writeScriptTag_ = function(src) {
  var doc = o3djs.global.document;
  if (typeof doc != 'undefined' &&
      !o3djs.dependencies_.written[src]) {
    o3djs.dependencies_.written[src] = true;
    doc.write('<script type="application/javascript" src="' +
              src + '"></' + 'script>');
  }
};


/**
 * Resolves dependencies based on the dependencies added using addDependency
 * and calls writeScriptTag_ in the correct order.
 * @private
 */
o3djs.writeScripts_ = function() {
  // the scripts we need to write this time.
  var scripts = [];
  var seenScript = {};
  var deps = o3djs.dependencies_;

  function visitNode(path) {
    if (path in deps.written) {
      return;
    }

    // we have already visited this one. We can get here if we have cyclic
    // dependencies.
    if (path in deps.visited) {
      if (!(path in seenScript)) {
        seenScript[path] = true;
        scripts.push(path);
      }
      return;
    }

    deps.visited[path] = true;

    if (!(path in seenScript)) {
      seenScript[path] = true;
      scripts.push(path);
    }
  }

  for (var path in o3djs.included_) {
    if (!deps.written[path]) {
      visitNode(path);
    }
  }

  for (var i = 0; i < scripts.length; i++) {
    if (scripts[i]) {
      o3djs.writeScriptTag_(o3djs.basePath + scripts[i]);
    } else {
      throw Error('Undefined script input');
    }
  }
};


/**
 * Looks at the dependency rules and tries to determine the script file that
 * fulfills a particular rule.
 * @param {string} rule In the form o3djs.namespace.Class or
 *     project.script.
 * @return {string?} Url corresponding to the rule, or null.
 * @private
 */
o3djs.getPathFromRule_ = function(rule) {
  var parts = rule.split('.');
  return parts.join('/') + '.js';
};

o3djs.findBasePath_();

/**
 * Returns true if the specified value is not |undefined|.
 * WARNING: Do not use this to test if an object has a property. Use the in
 * operator instead.
 * @param {*} val Variable to test.
 * @return {boolean} Whether variable is defined.
 */
o3djs.isDef = function(val) {
  return typeof val != 'undefined';
};


/**
 * Exposes an unobfuscated global namespace path for the given object.
 * Note that fields of the exported object *will* be obfuscated,
 * unless they are exported in turn via this function or
 * o3djs.exportProperty.
 *
 * <p>Also handy for making public items that are defined in anonymous
 * closures.
 *
 * ex. o3djs.exportSymbol('Foo', Foo);
 *
 * ex. o3djs.exportSymbol('public.path.Foo.staticFunction',
 *                        Foo.staticFunction);
 *     public.path.Foo.staticFunction();
 *
 * ex. o3djs.exportSymbol('public.path.Foo.prototype.myMethod',
 *                        Foo.prototype.myMethod);
 *     new public.path.Foo().myMethod();
 *
 * @param {string} publicPath Unobfuscated name to export.
 * @param {Object} object Object the name should point to.
 * @param {Object} opt_objectToExportTo The object to add the path to; default
 *     is |o3djs.global|.
 */
o3djs.exportSymbol = function(publicPath, object, opt_objectToExportTo) {
  o3djs.exportPath_(publicPath, object, opt_objectToExportTo);
};

/**
 * This string contains JavaScript code to initialize a new V8 instance.
 * @private
 * @type {string}
 */
o3djs.v8Initializer_ = '';

/**
 * This array contains references to objects that v8 needs to bind to when
 * it initializes.
 * @private
 * @type {!Array.<Object>}
 */
o3djs.v8InitializerArgs_ = [];

/**
 * Converts any JavaScript value to a string representation that when evaluated
 * will result in an equal value.
 * @param {*} value Any value.
 * @return {string} A string representation for the value.
 * @private
 */
o3djs.valueToString_ = function(value) {
  switch (typeof(value)) {
    case 'undefined':
      return 'undefined';
    case 'string':
      var escaped = escape(value);
      if (escaped === value) {
        return '"' + value + '"';
      } else {
        return 'unescape("' + escaped + '")';
      }
    case 'object':
      if (value === null) {
        return 'null';
      } else {
        // TODO: all the other builtin JavaScript objects like Date,
        // Number, Boolean, etc.
        if (value instanceof RegExp) {
          var result =
              'new RegExp(' + o3djs.valueToString_(value.source) + ', "';
          if (value.global) {
            result += 'g';
          }
          if (value.ignoreCase) {
            result += 'i';
          }
          if (value.multiline) {
            result += 'm';
          }
          result += '")';
          return result;
        } else if (o3djs.base.isArray(value)) {
          var valueAsArray = /** @type {!Array} */ (value);
          var result = '[';
          var separator = '';
          for (var i = 0; i < valueAsArray.length; ++i) {
            result += separator + o3djs.valueToString_(valueAsArray[i]);
            separator = ',';
          }
          result += ']\n';
          return result;
        } else {
          var valueAsObject = /** @type {!Object} */ (value);
          var result = '{\n';
          var separator = '';
          for (var propertyName in valueAsObject) {
            result += separator + '"' + propertyName + '": ' +
              o3djs.valueToString_(valueAsObject[propertyName]);
            separator = ',';
          }
          result += '}\n';
          return result;
        }
      }
    default:
      return value.toString()
  }
};

/**
 * Given an object holding a namespace and the name of that namespace,
 * generates a string that when evaluated will populate the namespace.
 * @param {!Object} namespaceObject An object holding a namespace.
 * @param {string} namespaceName The name of the namespace.
 * @param {!Array.<Object>} opt_args An array of objects that will be used
 *     together with the initializer string to populate a namespace. The args
 *     may be referenced from initializer code as args_[i] where i is the index
 *     in the array.
 * @return {string} A string that will populate the namespace.
 * @private
 */
o3djs.namespaceInitializer_ = function(namespaceObject,
                                       namespaceName,
                                       opt_args) {
  var result = namespaceName + ' = {};\n';
  for (var propertyName in namespaceObject) {
    var propertyNamespaceName = namespaceName + '.' + propertyName;
    var propertyValue = namespaceObject[propertyName];
    if (typeof(propertyValue) === 'object' && propertyValue !== null &&
        !o3djs.base.isArray(propertyValue) &&
        !(propertyValue instanceof RegExp)) {
      result += o3djs.namespaceInitializer_(propertyValue,
                                            propertyNamespaceName);
    } else {
      var valueAsString = o3djs.valueToString_(propertyValue);

      // If this is a browser only function then bind to the browser version
      // of the function rather than create a new function in V8.
      if (typeof(propertyValue) == 'function' &&
          valueAsString.indexOf('o3djs.BROWSER_ONLY') != -1) {
        valueAsString = 'args_[' + opt_args.length + ']';
        opt_args.push(propertyValue);
      }
      result += propertyNamespaceName + ' = ' + valueAsString + ';\n';

      if (typeof(propertyValue) === 'function' && propertyValue.prototype) {
        result += o3djs.namespaceInitializer_(
            propertyValue.prototype,
            propertyNamespaceName + '.prototype');
      }
    }
  }
  return result;
};

o3djs.provide('o3djs.base');

/**
 * The base module for o3djs.
 * @namespace
 */
o3djs.base = o3djs.base || {};

/**
 * The a Javascript copy of the o3d namespace object. (holds constants, enums,
 * etc...)
 * @type {o3d.o3d}
 */
o3djs.base.o3d = null;

/**
 * Snapshots the current state of all provided namespaces. This state will be
 * used to initialize future V8 instances. It is automatically
 * called by o3djs.util.makeClients.
 */
o3djs.base.snapshotProvidedNamespaces = function()  {
  // Snapshot the V8 initializer string from the current state of browser
  // JavaScript the first time this is called.
  o3djs.v8Initializer_ = 'function(args_) {\n';
  o3djs.v8InitializerArgs_ = [];
  for (var i = 0; i < o3djs.provided_.length; ++i) {
    var object = o3djs.getObjectByName(o3djs.provided_[i]);
    o3djs.v8Initializer_ += o3djs.namespaceInitializer_(
        /** @type {!Object} */ (object),
        o3djs.provided_[i],
        o3djs.v8InitializerArgs_);
  }

  o3djs.v8Initializer_ += '}\n';
};

/**
 * Initializes the o3djs.sample library in a v8 instance. This should be called
 * for every V8 instance that uses the sample library. It is automatically
 * called by o3djs.util.makeClients.
 * @param {!o3d.plugin} clientObject O3D.Plugin Object.
 */
o3djs.base.initV8 = function(clientObject)  {
  var v8Init = function(initializer, args) {
    // Set up the o3djs namespace.
    var o3djsBrowser = o3djs;
    o3djs = {};
    o3djs.browser = o3djsBrowser;
    o3djs.global = (function() { return this; })();

    o3djs.require = function(rule) {}
    o3djs.provide = function(rule) {}

    // Evaluate the initializer string with the arguments containing bindings
    // to browser side objects.
    eval('(' + initializer + ')')(args);

    // Make sure this points to the o3d namespace for this particular
    // instance of the plugin.
    o3djs.base.o3d = plugin.o3d;
  };

  clientObject.eval(v8Init.toString())(o3djs.v8Initializer_,
                                       o3djs.v8InitializerArgs_);
};

/**
 * Initializes the o3djs.sample library.
 * Basically all it does is record the o3djs.namespace object which is used by
 * other functions to look up o3d constants.
 *
 * @param {!Element} clientObject O3D.Plugin Object.
 */
o3djs.base.init = function(clientObject)  {
  function recursivelyCopyProperties(object) {
    var copy = {};
    var hasProperties = false;
    for (var key in object) {
      var property = object[key];
      if (typeof property == 'object' || typeof property == 'function') {
        property = recursivelyCopyProperties(property);
      }
      if (typeof property != 'undefined') {
        copy[key] = property;
        hasProperties = true;
      }
    }
    return hasProperties ? copy : undefined;
  }
  try {
    o3djs.base.o3d = recursivelyCopyProperties(clientObject.o3d);
  } catch (e) {
    // Firefox 2 raises an exception when trying to enumerate a NPObject
    o3djs.base.o3d = clientObject.o3d;
  }
  // Because of a bug in chrome, it is not possible for the browser to enumerate
  // the properties of an NPObject.
  // Chrome bug: http://code.google.com/p/chromium/issues/detail?id=5743
  o3djs.base.o3d = o3djs.base.o3d || clientObject.o3d;
};

/**
 * Determine whether a value is an array. Do not use instanceof because that
 * will not work for V8 arrays (the browser thinks they are Objects).
 * @param {*} value A value.
 * @return {boolean} Whether the value is an array.
 */
o3djs.base.isArray = function(value) {
  var valueAsObject = /** @type {!Object} */ (value);
  return typeof(value) === 'object' && value !== null &&
      'length' in valueAsObject && 'splice' in valueAsObject;
};

/**
 * Check if the o3djs library has been initialized.
 * @return {boolean} true if ready, false if not.
 */
o3djs.base.ready = function() {
  return o3djs.base.o3d != null;
};

/**
 * A stub for later optionally converting obfuscated names
 * @private
 * @param {string} name Name to un-obfuscate.
 * @return {string} un-obfuscated name.
 */
o3djs.base.maybeDeobfuscateFunctionName_ = function(name) {
  return name;
};

/**
 * Makes one class inherit from another.
 * @param {!Object} subClass Class that wants to inherit.
 * @param {!Object} superClass Class to inherit from.
 */
o3djs.base.inherit = function(subClass, superClass) {
  /**
   * TmpClass.
   * @ignore
   * @constructor
   */
  var TmpClass = function() { };
  TmpClass.prototype = superClass.prototype;
  subClass.prototype = new TmpClass();
};

/**
 * Parses an error stack from an exception
 * @param {!Exception} excp The exception to get a stack trace from.
 * @return {!Array.<string>} An array of strings of the stack trace.
 */
o3djs.base.parseErrorStack = function(excp) {
  var stack = [];
  var name;
  var line;

  if (!excp || !excp.stack) {
    return stack;
  }

  var stacklist = excp.stack.split('\n');

  for (var i = 0; i < stacklist.length - 1; i++) {
    var framedata = stacklist[i];

    name = framedata.match(/^([a-zA-Z0-9_$]*)/)[1];
    if (name) {
      name = o3djs.base.maybeDeobfuscateFunctionName_(name);
    } else {
      name = 'anonymous';
    }

    var result = framedata.match(/(.*:[0-9]+)$/);
    line = result && result[1];

    if (!line) {
      line = '(unknown)';
    }

    stack[stack.length] = name + ' : ' + line
  }

  // remove top level anonymous functions to match IE
  var omitRegexp = /^anonymous :/;
  while (stack.length && omitRegexp.exec(stack[stack.length - 1])) {
    stack.length = stack.length - 1;
  }

  return stack;
};

/**
 * Gets a function name from a function object.
 * @param {!function(...): *} aFunction The function object to try to get a
 *      name from.
 * @return {string} function name or 'anonymous' if not found.
 */
o3djs.base.getFunctionName = function(aFunction) {
  var regexpResult = aFunction.toString().match(/function(\s*)(\w*)/);
  if (regexpResult && regexpResult.length >= 2 && regexpResult[2]) {
    return o3djs.base.maybeDeobfuscateFunctionName_(regexpResult[2]);
  }
  return 'anonymous';
};

/**
 * Pretty prints an exception's stack, if it has one.
 * @param {Array.<string>} stack An array of errors.
 * @return {string} The pretty stack.
 */
o3djs.base.formatErrorStack = function(stack) {
  var result = '';
  for (var i = 0; i < stack.length; i++) {
    result += '> ' + stack[i] + '\n';
  }
  return result;
};

/**
 * Gets a stack trace as a string.
 * @param {number} stripCount The number of entries to strip from the top of the
 *     stack. Example: Pass in 1 to remove yourself from the stack trace.
 * @return {string} The stack trace.
 */
o3djs.base.getStackTrace = function(stripCount) {
  var result = '';

  if (typeof(arguments.caller) != 'undefined') { // IE, not ECMA
    for (var a = arguments.caller; a != null; a = a.caller) {
      result += '> ' + o3djs.base.getFunctionName(a.callee) + '\n';
      if (a.caller == a) {
        result += '*';
        break;
      }
    }
  } else { // Mozilla, not ECMA
    // fake an exception so we can get Mozilla's error stack
    var testExcp;
    try {
      eval('var var;');
    } catch (testExcp) {
      var stack = o3djs.base.parseErrorStack(testExcp);
      result += o3djs.base.formatErrorStack(stack.slice(3 + stripCount,
                                                        stack.length));
    }
  }

  return result;
};

/**
 * Sets the error handler on a client to a handler that displays an alert on the
 * first error.
 * @param {!o3d.Client} client The client object of the plugin.
 */
o3djs.base.setErrorHandler = function(client) {
  client.setErrorCallback(
      function(msg) {
        // Clear the error callback. Otherwise if the callback is happening
        // during rendering it's possible the user will not be able to
        // get out of an infinite loop of alerts.
        client.clearErrorCallback();
        alert('ERROR: ' + msg + '\n' + o3djs.base.getStackTrace(1));
      });
};

/**
 * Returns true if the user's browser is Microsoft IE.
 * @return {boolean} true if the user's browser is Microsoft IE.
 */
o3djs.base.IsMSIE = function() {
  var ua = navigator.userAgent.toLowerCase();
  var msie = /msie/.test(ua) && !/opera/.test(ua);
  return msie;
};
/**
 * Returns true if the user's browser is Chrome 1.0, that requires a workaround
 * to create the plugin.
 * @return {boolean} true if the user's browser is Chrome 1.0.
 */
o3djs.base.IsChrome10 = function() {
  return navigator.userAgent.indexOf('Chrome/1.0') >= 0;
};
