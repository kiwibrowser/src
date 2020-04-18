// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Routines used to validate and normalize arguments.
// TODO(benwells): unit test this file.

var JSONSchemaValidator = require('json_schema').JSONSchemaValidator;

var schemaValidator = new JSONSchemaValidator();

// Validate arguments.
function validate(args, parameterSchemas) {
  if (args.length > parameterSchemas.length)
    throw new Error('Too many arguments.');
  for (var i = 0; i < parameterSchemas.length; ++i) {
    if ($Object.hasOwnProperty(args, i) && args[i] !== null &&
        args[i] !== undefined) {
      schemaValidator.resetErrors();
      schemaValidator.validate(args[i], parameterSchemas[i]);
      if (schemaValidator.errors.length == 0)
        continue;
      var message = 'Invalid value for argument ' + (i + 1) + '. ';
      $Array.forEach(schemaValidator.errors, function(err) {
        if (err.path) {
          message += "Property '" + err.path + "': ";
        }
        message += err.message;
        message = message.substring(0, message.length - 1);
        message += ', ';
      });
      message = message.substring(0, message.length - 2);
      message += '.';
      throw new Error(message);
    } else if (!parameterSchemas[i].optional) {
      throw new Error('Parameter ' + (i + 1) + ' (' +
          parameterSchemas[i].name + ') is required.');
    }
  }
}

// Generate all possible signatures for a given API function.
function getSignatures(parameterSchemas) {
  if (parameterSchemas.length === 0)
    return [[]];
  var signatures = [];
  $Object.setPrototypeOf(signatures, null);
  $Object.setPrototypeOf(parameterSchemas, null);
  var remaining = getSignatures($Array.slice(parameterSchemas, 1));
  $Object.setPrototypeOf(remaining, null);
  for (var i = 0; i < remaining.length; ++i)
    $Array.push(signatures, $Array.concat([parameterSchemas[0]], remaining[i]))
  if (parameterSchemas[0].optional)
    return $Array.concat(signatures, remaining);
  return signatures;
};

// Return true if arguments match a given signature's schema.
function argumentsMatchSignature(args, candidateSignature) {
  if (args.length != candidateSignature.length)
    return false;
  for (var i = 0; i < candidateSignature.length; ++i) {
    var argType =  JSONSchemaValidator.getType(args[i]);
    if (!schemaValidator.isValidSchemaType(argType, candidateSignature[i]))
      return false;
  }
  return true;
};

// Finds the function signature for the given arguments.
function resolveSignature(args, definedSignature) {
  var candidateSignatures = getSignatures(definedSignature);
  for (var i = 0; i < candidateSignatures.length; ++i) {
    if (argumentsMatchSignature(args, candidateSignatures[i]))
      return candidateSignatures[i];
  }
  return null;
};

// Returns a string representing the defined signature of the API function.
// Example return value for chrome.windows.getCurrent:
// "windows.getCurrent(optional object populate, function callback)"
function getParameterSignatureString(name, definedSignature) {
  var getSchemaTypeString = function(schema) {
    var schemaTypes = schemaValidator.getAllTypesForSchema(schema);
    var typeName = $Array.join(schemaTypes, ' or ') + ' ' + schema.name;
    if (schema.optional)
      return 'optional ' + typeName;
    return typeName;
  };
  var typeNames = $Array.map(definedSignature, getSchemaTypeString);
  return name + '(' + $Array.join(typeNames, ', ') + ')';
};

// Returns a string representing a call to an API function.
// Example return value for call: chrome.windows.get(1, callback) is:
// "windows.get(int, function)"
function getArgumentSignatureString(name, args) {
  var typeNames = $Array.map(args, JSONSchemaValidator.getType);
  return name + '(' + $Array.join(typeNames, ', ') + ')';
};

// Finds the correct signature for the given arguments, then validates the
// arguments against that signature. Returns a 'normalized' arguments list
// where nulls are inserted where optional parameters were omitted.
// |args| is expected to be an array.
function normalizeArgumentsAndValidate(args, funDef) {
  if (funDef.allowAmbiguousOptionalArguments) {
    validate(args, funDef.definition.parameters);
    return args;
  }
  var definedSignature = funDef.definition.parameters;
  var resolvedSignature = resolveSignature(args, definedSignature);
  if (!resolvedSignature)
    throw new Error('Invocation of form ' +
        getArgumentSignatureString(funDef.name, args) +
        " doesn't match definition " +
        getParameterSignatureString(funDef.name, definedSignature));
  validate(args, resolvedSignature);
  var normalizedArgs = [];
  $Object.setPrototypeOf(normalizedArgs, null);
  var ai = 0;
  for (var si = 0; si < definedSignature.length; ++si) {
    if (definedSignature[si] === resolvedSignature[ai])
      $Array.push(normalizedArgs, args[ai++]);
    else
      $Array.push(normalizedArgs, null);
  }
  return normalizedArgs;
};

// Validates that a given schema for an API function is not ambiguous.
function isFunctionSignatureAmbiguous(functionDef) {
  if (functionDef.allowAmbiguousOptionalArguments)
    return false;
  var signaturesAmbiguous = function(signature1, signature2) {
    if (signature1.length != signature2.length)
      return false;
    for (var i = 0; i < signature1.length; i++) {
      if (!schemaValidator.checkSchemaOverlap(
          signature1[i], signature2[i]))
        return false;
    }
    return true;
  };
  var candidateSignatures = getSignatures(functionDef.parameters);
  for (var i = 0; i < candidateSignatures.length; ++i) {
    for (var j = i + 1; j < candidateSignatures.length; ++j) {
      if (signaturesAmbiguous(candidateSignatures[i], candidateSignatures[j]))
        return true;
    }
  }
  return false;
};

exports.$set('isFunctionSignatureAmbiguous', isFunctionSignatureAmbiguous);
exports.$set('normalizeArgumentsAndValidate', normalizeArgumentsAndValidate);
exports.$set('schemaValidator', schemaValidator);
exports.$set('validate', validate);
