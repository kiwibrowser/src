// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_JSON_SCHEMA_JSON_SCHEMA_VALIDATOR_UNITTEST_BASE_H_
#define COMPONENTS_JSON_SCHEMA_JSON_SCHEMA_VALIDATOR_UNITTEST_BASE_H_

#include "testing/gtest/include/gtest/gtest.h"

namespace base {
class DictionaryValue;
class ListValue;
class Value;
}

// Base class for unit tests for JSONSchemaValidator. There is currently only
// one implementation, JSONSchemaValidatorCPPTest.
//
// TODO(aa): Refactor extensions/test/data/json_schema_test.js into
// JSONSchemaValidatorJSTest that inherits from this.
class JSONSchemaValidatorTestBase : public testing::Test {
 public:
  JSONSchemaValidatorTestBase();

  void RunTests();

 protected:
  virtual void ExpectValid(const std::string& test_source,
                           base::Value* instance,
                           base::DictionaryValue* schema,
                           base::ListValue* types) = 0;

  virtual void ExpectNotValid(const std::string& test_source,
                              base::Value* instance,
                              base::DictionaryValue* schema,
                              base::ListValue* types,
                              const std::string& expected_error_path,
                              const std::string& expected_error_message) = 0;

 private:
  void TestComplex();
  void TestStringPattern();
  void TestEnum();
  void TestChoices();
  void TestExtends();
  void TestObject();
  void TestTypeReference();
  void TestArrayTuple();
  void TestArrayNonTuple();
  void TestString();
  void TestNumber();
  void TestTypeClassifier();
  void TestTypes();
};

#endif  // COMPONENTS_JSON_SCHEMA_JSON_SCHEMA_VALIDATOR_UNITTEST_BASE_H_
