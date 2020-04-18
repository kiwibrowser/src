// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// -----------------------------------------------------------------------------
// NOTE: If you change this file you need to touch
// extension_renderer_resources.grd to have your change take effect.
// -----------------------------------------------------------------------------

//==============================================================================
// This file contains a class that implements a subset of JSON Schema.
// See: http://www.json.com/json-schema-proposal/ for more details.
//
// The following features of JSON Schema are not implemented:
// - requires
// - unique
// - disallow
// - union types (but replaced with 'choices')
//
// The following properties are not applicable to the interface exposed by
// this class:
// - options
// - readonly
// - title
// - description
// - format
// - default
// - transient
// - hidden
//
// There are also these departures from the JSON Schema proposal:
// - function and undefined types are supported
// - null counts as 'unspecified' for optional values
// - added the 'choices' property, to allow specifying a list of possible types
//   for a value
// - by default an "object" typed schema does not allow additional properties.
//   if present, "additionalProperties" is to be a schema against which all
//   additional properties will be validated.
//==============================================================================

var utils = require('utils');
var loggingNative = requireNative('logging');
var schemaRegistry = requireNative('schema_registry');
var CHECK = loggingNative.CHECK;
var DCHECK = loggingNative.DCHECK;
var WARNING = loggingNative.WARNING;

function loadTypeSchema(typeName, defaultSchema) {
  var parts = $String.split(typeName, '.');
  if (parts.length == 1) {
    if (defaultSchema == null) {
      WARNING('Trying to reference "' + typeName + '" ' +
              'with neither namespace nor default schema.');
      return null;
    }
    var types = defaultSchema.types;
  } else {
    var schemaName = $Array.join($Array.slice(parts, 0, parts.length - 1), '.');
    var types = schemaRegistry.GetSchema(schemaName).types;
  }
  for (var i = 0; i < types.length; ++i) {
    if (types[i].id == typeName)
      return types[i];
  }
  return null;
}

function isInstanceOfClass(instance, className) {
  while ((instance = instance.__proto__)) {
    if (instance.constructor.name == className)
      return true;
  }
  return false;
}

function isOptionalValue(value) {
  return value === undefined || value === null;
}

function enumToString(enumValue) {
  if (enumValue.name === undefined)
    return enumValue;

  return enumValue.name;
}

/**
 * Validates an instance against a schema and accumulates errors. Usage:
 *
 * var validator = new JSONSchemaValidator();
 * validator.validate(inst, schema);
 * if (validator.errors.length == 0)
 *   console.log("Valid!");
 * else
 *   console.log(validator.errors);
 *
 * The errors property contains a list of objects. Each object has two
 * properties: "path" and "message". The "path" property contains the path to
 * the key that had the problem, and the "message" property contains a sentence
 * describing the error.
 */
function JSONSchemaValidator() {
  this.errors = [];
  this.types = [];
}
$Object.setPrototypeOf(JSONSchemaValidator.prototype, null);

var messages = {
  __proto__: null,

  invalidEnum: 'Value must be one of: [*].',
  propertyRequired: 'Property is required.',
  unexpectedProperty: 'Unexpected property.',
  arrayMinItems: 'Array must have at least * items.',
  arrayMaxItems: 'Array must not have more than * items.',
  itemRequired: 'Item is required.',
  stringMinLength: 'String must be at least * characters long.',
  stringMaxLength: 'String must not be more than * characters long.',
  stringPattern: 'String must match the pattern: *.',
  numberFiniteNotNan: 'Value must not be *.',
  numberMinValue: 'Value must not be less than *.',
  numberMaxValue: 'Value must not be greater than *.',
  numberIntValue: 'Value must fit in a 32-bit signed integer.',
  numberMaxDecimal: 'Value must not have more than * decimal places.',
  invalidType: "Expected '*' but got '*'.",
  invalidTypeIntegerNumber:
      "Expected 'integer' but got 'number', consider using Math.round().",
  invalidChoice: 'Value does not match any valid type choices.',
  invalidPropertyType: 'Missing property type.',
  schemaRequired: 'Schema value required.',
  unknownSchemaReference: 'Unknown schema reference: *.',
  notInstance: 'Object must be an instance of *.',
};

/**
 * Builds an error message. Key is the property in the |errors| object, and
 * |opt_replacements| is an array of values to replace "*" characters with.
 */
utils.defineProperty(JSONSchemaValidator, 'formatError',
                     function(key, opt_replacements) {
  var message = messages[key];
  if (opt_replacements) {
    for (var i = 0; i < opt_replacements.length; ++i) {
      DCHECK($String.indexOf(message, '*') != -1, message);
      message = $String.replace(message, '*', opt_replacements[i]);
    }
  }
  DCHECK($String.indexOf(message, '*') == -1)
  return message;
});

/**
 * Classifies a value as one of the JSON schema primitive types. Note that we
 * don't explicitly disallow 'function', because we want to allow functions in
 * the input values.
 */
utils.defineProperty(JSONSchemaValidator, 'getType', function(value) {
  // If we can determine the type safely in JS, it's fastest to do it here.
  // However, Object types are difficult to classify, so we have to do it in
  // C++.
  var s = typeof value;
  if (s === 'object')
    return value === null ? 'null' : schemaRegistry.GetObjectType(value);
  if (s === 'number')
    return value % 1 === 0 ? 'integer' : 'number';
  return s;
});

/**
 * Add types that may be referenced by validated schemas that reference them
 * with "$ref": <typeId>. Each type must be a valid schema and define an
 * "id" property.
 */
JSONSchemaValidator.prototype.addTypes = function(typeOrTypeList) {
  function addType(validator, type) {
    if (!type.id)
      throw new Error("Attempt to addType with missing 'id' property");
    validator.types[type.id] = type;
  }

  if ($Array.isArray(typeOrTypeList)) {
    for (var i = 0; i < typeOrTypeList.length; ++i) {
      addType(this, typeOrTypeList[i]);
    }
  } else {
    addType(this, typeOrTypeList);
  }
}

/**
 * Returns a list of strings of the types that this schema accepts.
 */
JSONSchemaValidator.prototype.getAllTypesForSchema = function(schema) {
  var schemaTypes = [];
  if (schema.type)
    $Array.push(schemaTypes, schema.type);
  if (schema.choices) {
    for (var i = 0; i < schema.choices.length; ++i) {
      var choiceTypes = this.getAllTypesForSchema(schema.choices[i]);
      schemaTypes = $Array.concat(schemaTypes, choiceTypes);
    }
  }
  var ref = schema['$ref'];
  if (ref) {
    var type = this.getOrAddType(ref);
    CHECK(type, 'Could not find type ' + ref);
    schemaTypes = $Array.concat(schemaTypes, this.getAllTypesForSchema(type));
  }
  return schemaTypes;
};

JSONSchemaValidator.prototype.getOrAddType = function(typeName) {
  if (!this.types[typeName])
    this.types[typeName] = loadTypeSchema(typeName);
  return this.types[typeName];
};

/**
 * Returns true if |schema| would accept an argument of type |type|.
 */
JSONSchemaValidator.prototype.isValidSchemaType = function(type, schema) {
  if (type == 'any')
    return true;

  // TODO(kalman): I don't understand this code. How can type be "null"?
  if (schema.optional && (type == 'null' || type == 'undefined'))
    return true;

  var schemaTypes = this.getAllTypesForSchema(schema);
  for (var i = 0; i < schemaTypes.length; ++i) {
    if (schemaTypes[i] == 'any' || type == schemaTypes[i] ||
        (type == 'integer' && schemaTypes[i] == 'number'))
      return true;
  }

  return false;
};

/**
 * Returns true if there is a non-null argument that both |schema1| and
 * |schema2| would accept.
 */
JSONSchemaValidator.prototype.checkSchemaOverlap = function(schema1, schema2) {
  var schema1Types = this.getAllTypesForSchema(schema1);
  for (var i = 0; i < schema1Types.length; ++i) {
    if (this.isValidSchemaType(schema1Types[i], schema2))
      return true;
  }
  return false;
};

/**
 * Validates an instance against a schema. The instance can be any JavaScript
 * value and will be validated recursively. When this method returns, the
 * |errors| property will contain a list of errors, if any.
 */
JSONSchemaValidator.prototype.validate = function(instance, schema, opt_path) {
  var path = opt_path || '';

  if (!schema) {
    this.addError(path, 'schemaRequired');
    return;
  }

  // If this schema defines itself as reference type, save it in this.types.
  if (schema.id)
    this.types[schema.id] = schema;

  // If the schema has an extends property, the instance must validate against
  // that schema too.
  if (schema.extends)
    this.validate(instance, schema.extends, path);

  // If the schema has a $ref property, the instance must validate against
  // that schema too. It must be present in this.types to be referenced.
  var ref = schema.$ref;
  if (ref) {
    if (!this.getOrAddType(ref))
      this.addError(path, 'unknownSchemaReference', [ref]);
    else
      this.validate(instance, this.getOrAddType(ref), path)
  }

  // If the schema has a choices property, the instance must validate against at
  // least one of the items in that array.
  if (schema.choices) {
    this.validateChoices(instance, schema, path);
    return;
  }

  // If the schema has an enum property, the instance must be one of those
  // values.
  if (schema.enum) {
    if (!this.validateEnum(instance, schema, path))
      return;
  }

  if (schema.type && schema.type != 'any') {
    if (!this.validateType(instance, schema, path))
      return;

    // Type-specific validation.
    switch (schema.type) {
      case 'object':
        this.validateObject(instance, schema, path);
        break;
      case 'array':
        this.validateArray(instance, schema, path);
        break;
      case 'string':
        this.validateString(instance, schema, path);
        break;
      case 'number':
      case 'integer':
        this.validateNumber(instance, schema, path);
        break;
    }
  }
};

/**
 * Validates an instance against a choices schema. The instance must match at
 * least one of the provided choices.
 */
JSONSchemaValidator.prototype.validateChoices =
    function(instance, schema, path) {
  var originalErrors = this.errors;

  for (var i = 0; i < schema.choices.length; ++i) {
    this.errors = [];
    this.validate(instance, schema.choices[i], path);
    if (this.errors.length == 0) {
      this.errors = originalErrors;
      return;
    }
  }

  this.errors = originalErrors;
  this.addError(path, 'invalidChoice');
};

/**
 * Validates an instance against a schema with an enum type. Populates the
 * |errors| property, and returns a boolean indicating whether the instance
 * validates.
 */
JSONSchemaValidator.prototype.validateEnum = function(instance, schema, path) {
  for (var i = 0; i < schema.enum.length; ++i) {
    if (instance === enumToString(schema.enum[i]))
      return true;
  }

  this.addError(path, 'invalidEnum',
                [$Array.join($Array.map(schema.enum, enumToString), ', ')]);
  return false;
};

/**
 * Validates an instance against an object schema and populates the errors
 * property.
 */
JSONSchemaValidator.prototype.validateObject =
    function(instance, schema, path) {
  if (schema.properties) {
    $Array.forEach($Object.keys(schema.properties), function(prop) {
      var propPath = path ? path + '.' + prop : prop;
      if (schema.properties[prop] == undefined) {
        this.addError(propPath, 'invalidPropertyType');
      } else if (instance[prop] !== undefined && instance[prop] !== null) {
        this.validate(instance[prop], schema.properties[prop], propPath);
      } else if (!schema.properties[prop].optional) {
        this.addError(propPath, 'propertyRequired');
      }
    }, this);
  }

  // If "instanceof" property is set, check that this object inherits from
  // the specified constructor (function).
  if (schema.isInstanceOf) {
    if (!isInstanceOfClass(instance, schema.isInstanceOf))
      this.addError(path || '', 'notInstance', [schema.isInstanceOf]);
  }

  // Exit early from additional property check if "type":"any" is defined.
  if (schema.additionalProperties &&
      schema.additionalProperties.type &&
      schema.additionalProperties.type == 'any') {
    return;
  }

  // By default, additional properties are not allowed on instance objects. This
  // can be overridden by setting the additionalProperties property to a schema
  // which any additional properties must validate against.
  $Array.forEach($Object.keys(instance), function(prop) {
    if (schema.properties && $Object.hasOwnProperty(schema.properties, prop))
      return;

    var propPath = path ? path + '.' + prop : prop;
    if (schema.additionalProperties)
      this.validate(instance[prop], schema.additionalProperties, propPath);
    else
      this.addError(propPath, 'unexpectedProperty');
  }, this);
};

/**
 * Validates an instance against an array schema and populates the errors
 * property.
 */
JSONSchemaValidator.prototype.validateArray = function(instance, schema, path) {
  var typeOfItems = JSONSchemaValidator.getType(schema.items);

  if (typeOfItems == 'object') {
    if (schema.minItems && instance.length < schema.minItems) {
      this.addError(path, 'arrayMinItems', [schema.minItems]);
    }

    if (typeof schema.maxItems != 'undefined' &&
        instance.length > schema.maxItems) {
      this.addError(path, 'arrayMaxItems', [schema.maxItems]);
    }

    // If the items property is a single schema, each item in the array must
    // have that schema.
    for (var i = 0; i < instance.length; ++i) {
      this.validate(instance[i], schema.items, path + '.' + i);
    }
  } else if (typeOfItems == 'array') {
    // If the items property is an array of schemas, each item in the array must
    // validate against the corresponding schema.
    for (var i = 0; i < schema.items.length; ++i) {
      var itemPath = path ? path + '.' + i : $String.self(i);
      if ($Object.hasOwnProperty(instance, i) &&
          !isOptionalValue(instance[i])) {
        this.validate(instance[i], schema.items[i], itemPath);
      } else if (!schema.items[i].optional) {
        this.addError(itemPath, 'itemRequired');
      }
    }

    if (schema.additionalProperties) {
      for (var i = schema.items.length; i < instance.length; ++i) {
        var itemPath = path ? path + '.' + i : $String.self(i);
        this.validate(instance[i], schema.additionalProperties, itemPath);
      }
    } else if (instance.length > schema.items.length) {
      this.addError(path, 'arrayMaxItems', [schema.items.length]);
    }
  }
};

/**
 * Validates a string and populates the errors property.
 */
JSONSchemaValidator.prototype.validateString =
    function(instance, schema, path) {
  if (schema.minLength && instance.length < schema.minLength)
    this.addError(path, 'stringMinLength', [schema.minLength]);

  if (schema.maxLength && instance.length > schema.maxLength)
    this.addError(path, 'stringMaxLength', [schema.maxLength]);

  if (schema.pattern && !schema.pattern.test(instance))
    this.addError(path, 'stringPattern', [schema.pattern]);
};

/**
 * Validates a number and populates the errors property. The instance is
 * assumed to be a number.
 */
JSONSchemaValidator.prototype.validateNumber =
    function(instance, schema, path) {
  // Forbid NaN, +Infinity, and -Infinity.  Our APIs don't use them, and
  // JSON serialization encodes them as 'null'.  Re-evaluate supporting
  // them if we add an API that could reasonably take them as a parameter.
  if (isNaN(instance) ||
      instance == Number.POSITIVE_INFINITY ||
      instance == Number.NEGATIVE_INFINITY )
    this.addError(path, 'numberFiniteNotNan', [instance]);

  if (schema.minimum !== undefined && instance < schema.minimum)
    this.addError(path, 'numberMinValue', [schema.minimum]);

  if (schema.maximum !== undefined && instance > schema.maximum)
    this.addError(path, 'numberMaxValue', [schema.maximum]);

  // Check for integer values outside of -2^31..2^31-1.
  if (schema.type === 'integer' && (instance | 0) !== instance)
    this.addError(path, 'numberIntValue', []);

  // We don't have a saved copy of Math, and it's not worth it just for a
  // 10^x function.
  var getPowerOfTen = function(pow) {
    // '10' is kind of an arbitrary number of maximum decimal places, but it
    // ensures we don't do anything crazy, and we should never need to restrict
    // decimals to a number higher than that.
    DCHECK(pow >= 1 && pow <= 10);
    DCHECK(pow % 1 === 0);
    var multiplier = 10;
    while (--pow)
      multiplier *= 10;
    return multiplier;
  };
  if (schema.maxDecimal &&
      (instance * getPowerOfTen(schema.maxDecimal)) % 1) {
    this.addError(path, 'numberMaxDecimal', [schema.maxDecimal]);
  }
};

/**
 * Validates the primitive type of an instance and populates the errors
 * property. Returns true if the instance validates, false otherwise.
 */
JSONSchemaValidator.prototype.validateType = function(instance, schema, path) {
  var actualType = JSONSchemaValidator.getType(instance);
  if (schema.type == actualType ||
      (schema.type == 'number' && actualType == 'integer')) {
    return true;
  } else if (schema.type == 'integer' && actualType == 'number') {
    this.addError(path, 'invalidTypeIntegerNumber');
    return false;
  } else {
    this.addError(path, 'invalidType', [schema.type, actualType]);
    return false;
  }
};

/**
 * Adds an error message. |key| is an index into the |messages| object.
 * |replacements| is an array of values to replace '*' characters in the
 * message.
 */
JSONSchemaValidator.prototype.addError = function(path, key, replacements) {
  $Array.push(this.errors, {
    __proto__: null,
    path: path,
    message: JSONSchemaValidator.formatError(key, replacements)
  });
};

/**
 * Resets errors to an empty list so you can call 'validate' again.
 */
JSONSchemaValidator.prototype.resetErrors = function() {
  this.errors = [];
};

exports.$set('JSONSchemaValidator', JSONSchemaValidator);
exports.$set('loadTypeSchema', loadTypeSchema);
