// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var AssertTrue = requireNative('assert').AssertTrue;
var JSONSchemaValidator = require('json_schema').JSONSchemaValidator;
var LOG = requireNative('logging').LOG;

function assertValid(type, instance, schema, types) {
  var validator = new JSONSchemaValidator();
  if (types)
    validator.addTypes(types);
  validator["validate" + type](instance, schema, "");
  var success = true;
  if (validator.errors.length != 0) {
    LOG("Got unexpected errors");
    for (var i = 0; i < validator.errors.length; i++) {
      LOG(validator.errors[i].message + "  path: " + validator.errors[i].path);
    }
    success = false;
  }
  AssertTrue(success);
}

function assertNotValid(type, instance, schema, errors, types) {
  var validator = new JSONSchemaValidator();
  if (types)
    validator.addTypes(types);
  validator["validate" + type](instance, schema, "");
  AssertTrue(validator.errors.length === errors.length);
  var success = true;
  for (var i = 0; i < errors.length; i++) {
    if (validator.errors[i].message == errors[i]) {
      LOG("Got expected error: " + validator.errors[i].message +
          " for path: " + validator.errors[i].path);
    } else {
      LOG("Missed expected error: " + errors[i] + ". Got: " +
          validator.errors[i].message + " instead.");
      success = false;
    }
  }
  AssertTrue(success);
}

function assertListConsistsOfElements(list, elements) {
  var success = true;
  for (var li = 0; li < list.length; li++) {
    for (var ei = 0; ei < elements.length && list[li] != elements[ei]; ei++) { }
    if (ei == elements.length) {
      LOG("Expected type not found: " + list[li]);
      success = false;
    }
  }
  AssertTrue(success);
}

function assertEqualSets(set1, set2) {
  assertListConsistsOfElements(set1, set2);
  assertListConsistsOfElements(set2, set1);
}

function formatError(key, replacements) {
  return JSONSchemaValidator.formatError(key, replacements);
}

function testFormatError() {
  AssertTrue(formatError("propertyRequired") == "Property is required.");
  AssertTrue(formatError("invalidEnum", ["foo, bar"]) ==
         "Value must be one of: [foo, bar].");
  AssertTrue(formatError("invalidType", ["foo", "bar"]) ==
         "Expected 'foo' but got 'bar'.");
}

function testComplex() {
  var schema = {
    type: "array",
    items: [
      {
        type: "object",
        properties: {
          id: {
            type: "integer",
            minimum: 1
          },
          url: {
            type: "string",
            pattern: /^https?\:/,
            optional: true
          },
          index: {
            type: "integer",
            minimum: 0,
            optional: true
          },
          selected: {
            type: "boolean",
            optional: true
          }
        }
      },
      { type: "function", optional: true },
      { type: "function", optional: true }
    ]
  };

  var instance = [
    {
      id: 42,
      url: "http://www.google.com/",
      index: 2,
      selected: true
    },
    function(){},
    function(){}
  ];

  assertValid("", instance, schema);
  instance.length = 2;
  assertValid("", instance, schema);
  instance.length = 1;
  instance.push({});
  assertNotValid("", instance, schema,
                 [formatError("invalidType", ["function", "object"])]);
  instance[1] = function(){};

  instance[0].url = "foo";
  assertNotValid("", instance, schema,
                 [formatError("stringPattern",
                              [schema.items[0].properties.url.pattern])]);
  delete instance[0].url;
  assertValid("", instance, schema);

  instance[0].id = 0;
  assertNotValid("", instance, schema,
                 [formatError("numberMinValue",
                              [schema.items[0].properties.id.minimum])]);
}

function testEnum() {
  var schema = {
    enum: ["foo", 42, false]
  };

  assertValid("", "foo", schema);
  assertValid("", 42, schema);
  assertValid("", false, schema);
  assertNotValid("", "42", schema, [formatError("invalidEnum",
                                                [schema.enum.join(", ")])]);
  assertNotValid("", null, schema, [formatError("invalidEnum",
                                                [schema.enum.join(", ")])]);
}

function testChoices() {
  var schema = {
    choices: [
      { type: "null" },
      { type: "undefined" },
      { type: "integer", minimum:42, maximum:42 },
      { type: "object", properties: { foo: { type: "string" } } }
    ]
  }
    assertValid("", null, schema);
    assertValid("", undefined, schema);
    assertValid("", 42, schema);
    assertValid("", {foo: "bar"}, schema);

    assertNotValid("", "foo", schema, [formatError("invalidChoice", [])]);
    assertNotValid("", [], schema, [formatError("invalidChoice", [])]);
    assertNotValid("", {foo: 42}, schema, [formatError("invalidChoice", [])]);
}

function testExtends() {
  var parent = {
    type: "number"
  }
  var schema = {
    extends: parent
  };

  assertValid("", 42, schema);
  assertNotValid("", "42", schema,
                 [formatError("invalidType", ["number", "string"])]);

  // Make the derived schema more restrictive
  parent.minimum = 43;
  assertNotValid("", 42, schema, [formatError("numberMinValue", [43])]);
  assertValid("", 43, schema);
}

function testObject() {
  var schema = {
    properties: {
      "foo": {
        type: "string"
      },
      "bar": {
        type: "integer"
      }
    }
  };

  assertValid("Object", {foo:"foo", bar:42}, schema);
  assertNotValid("Object", {foo:"foo", bar:42,"extra":true}, schema,
                 [formatError("unexpectedProperty")]);
  assertNotValid("Object", {foo:"foo"}, schema,
                 [formatError("propertyRequired")]);
  assertNotValid("Object", {foo:"foo", bar:"42"}, schema,
                 [formatError("invalidType", ["integer", "string"])]);

  schema.additionalProperties = { type: "any" };
  assertValid("Object", {foo:"foo", bar:42, "extra":true}, schema);
  assertValid("Object", {foo:"foo", bar:42, "extra":"foo"}, schema);

  schema.additionalProperties = { type: "boolean" };
  assertValid("Object", {foo:"foo", bar:42, "extra":true}, schema);
  assertNotValid("Object", {foo:"foo", bar:42, "extra":"foo"}, schema,
                 [formatError("invalidType", ["boolean", "string"])]);

  schema.properties.bar.optional = true;
  assertValid("Object", {foo:"foo", bar:42}, schema);
  assertValid("Object", {foo:"foo"}, schema);
  assertValid("Object", {foo:"foo", bar:null}, schema);
  assertValid("Object", {foo:"foo", bar:undefined}, schema);
  assertNotValid("Object", {foo:"foo", bar:"42"}, schema,
                 [formatError("invalidType", ["integer", "string"])]);
}

function testTypeReference() {
  var referencedTypes = [
    {
      id: "MinLengthString",
      type: "string",
      minLength: 2
    },
    {
      id: "Max10Int",
      type: "integer",
      maximum: 10
    }
  ];

  var schema = {
    type: "object",
    properties: {
      "foo": {
        type: "string"
      },
      "bar": {
        $ref: "Max10Int"
      },
      "baz": {
        $ref: "MinLengthString"
      }
    }
  };

  var schemaInlineReference = {
    type: "object",
    properties: {
      "foo": {
        type: "string"
      },
      "bar": {
        id: "NegativeInt",
        type: "integer",
        maximum: 0
      },
      "baz": {
        $ref: "NegativeInt"
      }
    }
  }

  // Valid type references externally added.
  assertValid("", {foo:"foo",bar:4,baz:"ab"}, schema, referencedTypes);

  // Valida type references internally defined.
  assertValid("", {foo:"foo",bar:-4,baz:-2}, schemaInlineReference);

  // Failures in validation, but succesful schema reference.
  assertNotValid("", {foo:"foo",bar:4,baz:"a"}, schema,
                [formatError("stringMinLength", [2])], referencedTypes);
  assertNotValid("", {foo:"foo",bar:20,baz:"abc"}, schema,
                [formatError("numberMaxValue", [10])], referencedTypes);

  // Remove MinLengthString type.
  referencedTypes.shift();
  assertNotValid("", {foo:"foo",bar:4,baz:"ab"}, schema,
                 [formatError("unknownSchemaReference", ["MinLengthString"])],
                 referencedTypes);

  // Remove internal type "NegativeInt"
  delete schemaInlineReference.properties.bar;
  assertNotValid("", {foo:"foo",baz:-2}, schemaInlineReference,
                 [formatError("unknownSchemaReference", ["NegativeInt"])]);
}

function testArrayTuple() {
  var schema = {
    items: [
      {
        type: "string"
      },
      {
        type: "integer"
      }
    ]
  };

  assertValid("Array", ["42", 42], schema);
  assertNotValid("Array", ["42", 42, "anything"], schema,
                 [formatError("arrayMaxItems", [schema.items.length])]);
  assertNotValid("Array", ["42"], schema, [formatError("itemRequired")]);
  assertNotValid("Array", [42, 42], schema,
                 [formatError("invalidType", ["string", "integer"])]);

  schema.additionalProperties = { type: "any" };
  assertValid("Array", ["42", 42, "anything"], schema);
  assertValid("Array", ["42", 42, []], schema);

  schema.additionalProperties = { type: "boolean" };
  assertNotValid("Array", ["42", 42, "anything"], schema,
                 [formatError("invalidType", ["boolean", "string"])]);
  assertValid("Array", ["42", 42, false], schema);

  schema.items[0].optional = true;
  assertValid("Array", ["42", 42], schema);
  assertValid("Array", [, 42], schema);
  assertValid("Array", [null, 42], schema);
  assertValid("Array", [undefined, 42], schema);
  assertNotValid("Array", [42, 42], schema,
                 [formatError("invalidType", ["string", "integer"])]);
}

function testArrayNonTuple() {
  var schema = {
    items: {
      type: "string"
    },
    minItems: 2,
    maxItems: 3
  };

  assertValid("Array", ["x", "x"], schema);
  assertValid("Array", ["x", "x", "x"], schema);

  assertNotValid("Array", ["x"], schema,
                 [formatError("arrayMinItems", [schema.minItems])]);
  assertNotValid("Array", ["x", "x", "x", "x"], schema,
                 [formatError("arrayMaxItems", [schema.maxItems])]);
  assertNotValid("Array", [42], schema,
                 [formatError("arrayMinItems", [schema.minItems]),
                  formatError("invalidType", ["string", "integer"])]);
}

function testString() {
  var schema = {
    minLength: 1,
    maxLength: 10,
    pattern: /^x/
  };

  assertValid("String", "x", schema);
  assertValid("String", "xxxxxxxxxx", schema);

  assertNotValid("String", "y", schema,
                 [formatError("stringPattern", [schema.pattern])]);
  assertNotValid("String", "xxxxxxxxxxx", schema,
                 [formatError("stringMaxLength", [schema.maxLength])]);
  assertNotValid("String", "", schema,
                 [formatError("stringMinLength", [schema.minLength]),
                  formatError("stringPattern", [schema.pattern])]);
}

function testNumber() {
  var schema = {
    minimum: 1,
    maximum: 100,
    maxDecimal: 2
  };

  assertValid("Number", 1, schema);
  assertValid("Number", 50, schema);
  assertValid("Number", 100, schema);
  assertValid("Number", 88.88, schema);

  assertNotValid("Number", 0.5, schema,
                 [formatError("numberMinValue", [schema.minimum])]);
  assertNotValid("Number", 100.1, schema,
                 [formatError("numberMaxValue", [schema.maximum])]);
  assertNotValid("Number", 100.111, schema,
                 [formatError("numberMaxValue", [schema.maximum]),
                  formatError("numberMaxDecimal", [schema.maxDecimal])]);

  var nan = 0/0;
  AssertTrue(isNaN(nan));
  assertNotValid("Number", nan, schema,
                 [formatError("numberFiniteNotNan", ["NaN"])]);

  assertNotValid("Number", Number.POSITIVE_INFINITY, schema,
                 [formatError("numberFiniteNotNan", ["Infinity"]),
                  formatError("numberMaxValue", [schema.maximum])
                 ]);

  assertNotValid("Number", Number.NEGATIVE_INFINITY, schema,
                 [formatError("numberFiniteNotNan", ["-Infinity"]),
                  formatError("numberMinValue", [schema.minimum])
                 ]);
}

function testIntegerBounds() {
  assertValid("Number",           0, {type:"integer"});
  assertValid("Number",          -1, {type:"integer"});
  assertValid("Number",  2147483647, {type:"integer"});
  assertValid("Number", -2147483648, {type:"integer"});
  assertNotValid("Number",           0.5, {type:"integer"},
                 [formatError("numberIntValue", [])]);
  assertNotValid("Number", 10000000000,   {type:"integer"},
                 [formatError("numberIntValue", [])]);
  assertNotValid("Number",  2147483647.5, {type:"integer"},
                 [formatError("numberIntValue", [])]);
  assertNotValid("Number",  2147483648,   {type:"integer"},
                 [formatError("numberIntValue", [])]);
  assertNotValid("Number",  2147483649,   {type:"integer"},
                 [formatError("numberIntValue", [])]);
  assertNotValid("Number", -2147483649,   {type:"integer"},
                 [formatError("numberIntValue", [])]);
}

function testType() {
  // valid
  assertValid("Type", {}, {type:"object"});
  assertValid("Type", [], {type:"array"});
  assertValid("Type", function(){}, {type:"function"});
  assertValid("Type", "foobar", {type:"string"});
  assertValid("Type", "", {type:"string"});
  assertValid("Type", 88.8, {type:"number"});
  assertValid("Type", 42, {type:"number"});
  assertValid("Type", 0, {type:"number"});
  assertValid("Type", 42, {type:"integer"});
  assertValid("Type", 0, {type:"integer"});
  assertValid("Type", true, {type:"boolean"});
  assertValid("Type", false, {type:"boolean"});
  assertValid("Type", null, {type:"null"});
  assertValid("Type", undefined, {type:"undefined"});
  assertValid("Type", new ArrayBuffer(1), {type:"binary"});
  assertValid("Type", otherContextArrayBufferContainer.value, {type:"binary"});

  // not valid
  assertNotValid("Type", [], {type: "object"},
                 [formatError("invalidType", ["object", "array"])]);
  assertNotValid("Type", null, {type: "object"},
                 [formatError("invalidType", ["object", "null"])]);
  assertNotValid("Type", function(){}, {type: "object"},
                 [formatError("invalidType", ["object", "function"])]);
  assertNotValid("Type", 42, {type: "array"},
                 [formatError("invalidType", ["array", "integer"])]);
  assertNotValid("Type", 42, {type: "string"},
                 [formatError("invalidType", ["string", "integer"])]);
  assertNotValid("Type", "42", {type: "number"},
                 [formatError("invalidType", ["number", "string"])]);
  assertNotValid("Type", 88.8, {type: "integer"},
                 [formatError("invalidTypeIntegerNumber")]);
  assertNotValid("Type", 1, {type: "boolean"},
                 [formatError("invalidType", ["boolean", "integer"])]);
  assertNotValid("Type", false, {type: "null"},
                 [formatError("invalidType", ["null", "boolean"])]);
  assertNotValid("Type", undefined, {type: "null"},
                 [formatError("invalidType", ["null", "undefined"])]);
  assertNotValid("Type", {}, {type: "function"},
                 [formatError("invalidType", ["function", "object"])]);
}

function testGetAllTypesForSchema() {
  var referencedTypes = [
    {
      id: "ChoicesRef",
      choices: [
        { type: "integer" },
        { type: "string" }
      ]
    },
    {
      id: "ObjectRef",
      type: "object",
    }
  ];

  var arraySchema = {
    type: "array"
  };

  var choicesSchema = {
    choices: [
      { type: "object" },
      { type: "function" }
    ]
  };

  var objectRefSchema = {
    $ref: "ObjectRef"
  };

  var complexSchema = {
    choices: [
      { $ref: "ChoicesRef" },
      { type: "function" },
      { $ref: "ObjectRef" }
    ]
  };

  var validator = new JSONSchemaValidator();
  validator.addTypes(referencedTypes);

  var arraySchemaTypes = validator.getAllTypesForSchema(arraySchema);
  assertEqualSets(arraySchemaTypes, ["array"]);

  var choicesSchemaTypes = validator.getAllTypesForSchema(choicesSchema);
  assertEqualSets(choicesSchemaTypes, ["object", "function"]);

  var objectRefSchemaTypes = validator.getAllTypesForSchema(objectRefSchema);
  assertEqualSets(objectRefSchemaTypes, ["object"]);

  var complexSchemaTypes = validator.getAllTypesForSchema(complexSchema);
  assertEqualSets(complexSchemaTypes,
      ["integer", "string", "function", "object"]);
}

function testIsValidSchemaType() {
  var referencedTypes = [
    {
      id: "ChoicesRef",
      choices: [
        { type: "integer" },
        { type: "string" }
      ]
    }
  ];

  var objectSchema = {
    type: "object",
    optional: true
  };

  var complexSchema = {
    choices: [
      { $ref: "ChoicesRef" },
      { type: "function" },
    ]
  };

  var validator = new JSONSchemaValidator();
  validator.addTypes(referencedTypes);

  AssertTrue(validator.isValidSchemaType("object", objectSchema));
  AssertTrue(!validator.isValidSchemaType("integer", objectSchema));
  AssertTrue(!validator.isValidSchemaType("array", objectSchema));
  AssertTrue(validator.isValidSchemaType("null", objectSchema));
  AssertTrue(validator.isValidSchemaType("undefined", objectSchema));

  AssertTrue(validator.isValidSchemaType("integer", complexSchema));
  AssertTrue(validator.isValidSchemaType("function", complexSchema));
  AssertTrue(validator.isValidSchemaType("string", complexSchema));
  AssertTrue(!validator.isValidSchemaType("object", complexSchema));
  AssertTrue(!validator.isValidSchemaType("null", complexSchema));
  AssertTrue(!validator.isValidSchemaType("undefined", complexSchema));
}

function testCheckSchemaOverlap() {
  var referencedTypes = [
    {
      id: "ChoicesRef",
      choices: [
        { type: "integer" },
        { type: "string" }
      ]
    },
    {
      id: "ObjectRef",
      type: "object",
    }
  ];

  var arraySchema = {
    type: "array"
  };

  var choicesSchema = {
    choices: [
      { type: "object" },
      { type: "function" }
    ]
  };

  var objectRefSchema = {
    $ref: "ObjectRef"
  };

  var complexSchema = {
    choices: [
      { $ref: "ChoicesRef" },
      { type: "function" },
      { $ref: "ObjectRef" }
    ]
  };

  var validator = new JSONSchemaValidator();
  validator.addTypes(referencedTypes);

  AssertTrue(!validator.checkSchemaOverlap(arraySchema, choicesSchema));
  AssertTrue(!validator.checkSchemaOverlap(arraySchema, objectRefSchema));
  AssertTrue(!validator.checkSchemaOverlap(arraySchema, complexSchema));
  AssertTrue(validator.checkSchemaOverlap(choicesSchema, objectRefSchema));
  AssertTrue(validator.checkSchemaOverlap(choicesSchema, complexSchema));
  AssertTrue(validator.checkSchemaOverlap(objectRefSchema, complexSchema));
}

function testInstanceOf() {
  function Animal() {};
  function Cat() {};
  function Dog() {};
  Cat.prototype = new Animal;
  Cat.prototype.constructor = Cat;
  Dog.prototype = new Animal;
  Dog.prototype.constructor = Dog;
  var cat = new Cat();
  var dog = new Dog();
  var num = new Number(1);

  // instanceOf should check type by walking up prototype chain.
  assertValid("", cat, {type:"object", isInstanceOf:"Cat"});
  assertValid("", cat, {type:"object", isInstanceOf:"Animal"});
  assertValid("", cat, {type:"object", isInstanceOf:"Object"});
  assertValid("", dog, {type:"object", isInstanceOf:"Dog"});
  assertValid("", dog, {type:"object", isInstanceOf:"Animal"});
  assertValid("", dog, {type:"object", isInstanceOf:"Object"});
  assertValid("", num, {type:"object", isInstanceOf:"Number"});
  assertValid("", num, {type:"object", isInstanceOf:"Object"});

  assertNotValid("", cat, {type:"object", isInstanceOf:"Dog"},
                 [formatError("notInstance", ["Dog"])]);
  assertNotValid("", dog, {type:"object", isInstanceOf:"Cat"},
                 [formatError("notInstance", ["Cat"])]);
  assertNotValid("", cat, {type:"object", isInstanceOf:"String"},
                 [formatError("notInstance", ["String"])]);
  assertNotValid("", dog, {type:"object", isInstanceOf:"String"},
                 [formatError("notInstance", ["String"])]);
  assertNotValid("", num, {type:"object", isInstanceOf:"Array"},
                [formatError("notInstance", ["Array"])]);
  assertNotValid("", num, {type:"object", isInstanceOf:"String"},
                [formatError("notInstance", ["String"])]);
}

// Tests exposed to schema_unittest.cc.
exports.testFormatError = testFormatError;
exports.testComplex = testComplex;
exports.testEnum = testEnum;
exports.testExtends = testExtends;
exports.testObject = testObject;
exports.testArrayTuple = testArrayTuple;
exports.testArrayNonTuple = testArrayNonTuple;
exports.testString = testString;
exports.testNumber = testNumber;
exports.testIntegerBounds = testIntegerBounds;
exports.testType = testType;
exports.testTypeReference = testTypeReference;
exports.testGetAllTypesForSchema = testGetAllTypesForSchema;
exports.testIsValidSchemaType = testIsValidSchemaType;
exports.testCheckSchemaOverlap = testCheckSchemaOverlap;
exports.testInstanceOf = testInstanceOf;
