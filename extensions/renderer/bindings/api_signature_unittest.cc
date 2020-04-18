// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/bindings/api_signature.h"

#include "base/values.h"
#include "extensions/renderer/bindings/api_binding_test.h"
#include "extensions/renderer/bindings/api_binding_test_util.h"
#include "extensions/renderer/bindings/api_invocation_errors.h"
#include "extensions/renderer/bindings/api_type_reference_map.h"
#include "extensions/renderer/bindings/argument_spec.h"
#include "extensions/renderer/bindings/argument_spec_builder.h"
#include "gin/converter.h"
#include "gin/dictionary.h"

namespace extensions {
namespace {

using SpecVector = std::vector<std::unique_ptr<ArgumentSpec>>;

std::unique_ptr<APISignature> OneString() {
  SpecVector specs;
  specs.push_back(ArgumentSpecBuilder(ArgumentType::STRING, "string").Build());
  return std::make_unique<APISignature>(std::move(specs));
}

std::unique_ptr<APISignature> StringAndInt() {
  SpecVector specs;
  specs.push_back(ArgumentSpecBuilder(ArgumentType::STRING, "string").Build());
  specs.push_back(ArgumentSpecBuilder(ArgumentType::INTEGER, "int").Build());
  return std::make_unique<APISignature>(std::move(specs));
}

std::unique_ptr<APISignature> StringOptionalIntAndBool() {
  SpecVector specs;
  specs.push_back(ArgumentSpecBuilder(ArgumentType::STRING, "string").Build());
  specs.push_back(
      ArgumentSpecBuilder(ArgumentType::INTEGER, "int").MakeOptional().Build());
  specs.push_back(ArgumentSpecBuilder(ArgumentType::BOOLEAN, "bool").Build());
  return std::make_unique<APISignature>(std::move(specs));
}

std::unique_ptr<APISignature> OneObject() {
  SpecVector specs;
  specs.push_back(
      ArgumentSpecBuilder(ArgumentType::OBJECT, "obj")
          .AddProperty("prop1",
                       ArgumentSpecBuilder(ArgumentType::STRING).Build())
          .AddProperty(
              "prop2",
              ArgumentSpecBuilder(ArgumentType::STRING).MakeOptional().Build())
          .Build());
  return std::make_unique<APISignature>(std::move(specs));
}

std::unique_ptr<APISignature> NoArgs() {
  return std::make_unique<APISignature>(SpecVector());
}

std::unique_ptr<APISignature> IntAndCallback() {
  SpecVector specs;
  specs.push_back(ArgumentSpecBuilder(ArgumentType::INTEGER, "int").Build());
  specs.push_back(
      ArgumentSpecBuilder(ArgumentType::FUNCTION, "callback").Build());
  return std::make_unique<APISignature>(std::move(specs));
}

std::unique_ptr<APISignature> IntAndOptionalCallback() {
  SpecVector specs;
  specs.push_back(ArgumentSpecBuilder(ArgumentType::INTEGER, "int").Build());
  specs.push_back(ArgumentSpecBuilder(ArgumentType::FUNCTION, "callback")
                      .MakeOptional()
                      .Build());
  return std::make_unique<APISignature>(std::move(specs));
}

std::unique_ptr<APISignature> OptionalIntAndCallback() {
  SpecVector specs;
  specs.push_back(
      ArgumentSpecBuilder(ArgumentType::INTEGER, "int").MakeOptional().Build());
  specs.push_back(
      ArgumentSpecBuilder(ArgumentType::FUNCTION, "callback").Build());
  return std::make_unique<APISignature>(std::move(specs));
}

std::unique_ptr<APISignature> OptionalCallback() {
  SpecVector specs;
  specs.push_back(ArgumentSpecBuilder(ArgumentType::FUNCTION, "callback")
                      .MakeOptional()
                      .Build());
  return std::make_unique<APISignature>(std::move(specs));
}

std::unique_ptr<APISignature> IntAnyOptionalObjectOptionalCallback() {
  SpecVector specs;
  specs.push_back(ArgumentSpecBuilder(ArgumentType::INTEGER, "int").Build());
  specs.push_back(ArgumentSpecBuilder(ArgumentType::ANY, "any").Build());
  specs.push_back(
      ArgumentSpecBuilder(ArgumentType::OBJECT, "obj")
          .AddProperty(
              "prop",
              ArgumentSpecBuilder(ArgumentType::INTEGER).MakeOptional().Build())
          .MakeOptional()
          .Build());
  specs.push_back(ArgumentSpecBuilder(ArgumentType::FUNCTION, "callback")
                      .MakeOptional()
                      .Build());
  return std::make_unique<APISignature>(std::move(specs));
}

std::unique_ptr<APISignature> RefObj() {
  SpecVector specs;
  specs.push_back(
      ArgumentSpecBuilder(ArgumentType::REF, "obj").SetRef("refObj").Build());
  return std::make_unique<APISignature>(std::move(specs));
}

std::unique_ptr<APISignature> RefEnum() {
  SpecVector specs;
  specs.push_back(
      ArgumentSpecBuilder(ArgumentType::REF, "enum").SetRef("refEnum").Build());
  return std::make_unique<APISignature>(std::move(specs));
}

std::unique_ptr<APISignature> OptionalObjectAndCallback() {
  SpecVector specs;
  specs.push_back(
      ArgumentSpecBuilder(ArgumentType::OBJECT, "obj")
          .AddProperty(
              "prop1",
              ArgumentSpecBuilder(ArgumentType::INTEGER).MakeOptional().Build())
          .MakeOptional()
          .Build());
  specs.push_back(ArgumentSpecBuilder(ArgumentType::FUNCTION, "callback")
                      .MakeOptional()
                      .Build());
  return std::make_unique<APISignature>(std::move(specs));
}

std::unique_ptr<APISignature> OptionalIntAndNumber() {
  SpecVector specs;
  specs.push_back(
      ArgumentSpecBuilder(ArgumentType::INTEGER, "int").MakeOptional().Build());
  specs.push_back(ArgumentSpecBuilder(ArgumentType::DOUBLE, "num").Build());
  return std::make_unique<APISignature>(std::move(specs));
}

std::unique_ptr<APISignature> OptionalIntAndInt() {
  SpecVector specs;
  specs.push_back(
      ArgumentSpecBuilder(ArgumentType::INTEGER, "int").MakeOptional().Build());
  specs.push_back(ArgumentSpecBuilder(ArgumentType::INTEGER, "int2").Build());
  return std::make_unique<APISignature>(std::move(specs));
}

std::vector<v8::Local<v8::Value>> StringToV8Vector(
    v8::Local<v8::Context> context,
    const char* args) {
  v8::Local<v8::Value> v8_args = V8ValueFromScriptSource(context, args);
  EXPECT_FALSE(v8_args.IsEmpty());
  EXPECT_TRUE(v8_args->IsArray());
  std::vector<v8::Local<v8::Value>> vector_args;
  EXPECT_TRUE(gin::ConvertFromV8(context->GetIsolate(), v8_args, &vector_args));
  return vector_args;
}

}  // namespace

class APISignatureTest : public APIBindingTest {
 public:
  APISignatureTest()
      : type_refs_(APITypeReferenceMap::InitializeTypeCallback()) {}
  ~APISignatureTest() override = default;

  void SetUp() override {
    APIBindingTest::SetUp();

    std::unique_ptr<ArgumentSpec> ref_obj_spec =
        ArgumentSpecBuilder(ArgumentType::OBJECT)
            .AddProperty("prop1",
                         ArgumentSpecBuilder(ArgumentType::STRING).Build())
            .AddProperty("prop2", ArgumentSpecBuilder(ArgumentType::INTEGER)
                                      .MakeOptional()
                                      .Build())
            .Build();
    type_refs_.AddSpec("refObj", std::move(ref_obj_spec));

    type_refs_.AddSpec("refEnum", ArgumentSpecBuilder(ArgumentType::STRING)
                                      .SetEnums({"alpha", "beta"})
                                      .Build());
  }

  void ExpectPass(const APISignature& signature,
                  base::StringPiece arg_values,
                  base::StringPiece expected_parsed_args,
                  bool expect_callback) {
    RunTest(signature, arg_values, expected_parsed_args, expect_callback, true,
            std::string());
  }

  void ExpectFailure(const APISignature& signature,
                     base::StringPiece arg_values,
                     const std::string& expected_error) {
    RunTest(signature, arg_values, base::StringPiece(), false, false,
            expected_error);
  }

  void ExpectResponsePass(const APISignature& signature,
                          base::StringPiece arg_values) {
    RunResponseTest(signature, arg_values, base::nullopt);
  }

  void ExpectResponseFailure(const APISignature& signature,
                             base::StringPiece arg_values,
                             const std::string& expected_error) {
    RunResponseTest(signature, arg_values, expected_error);
  }

  const APITypeReferenceMap& type_refs() const { return type_refs_; }

 private:
  void RunTest(const APISignature& signature,
               base::StringPiece arg_values,
               base::StringPiece expected_parsed_args,
               bool expect_callback,
               bool should_succeed,
               const std::string& expected_error) {
    SCOPED_TRACE(arg_values);
    v8::Local<v8::Context> context = MainContext();
    v8::Local<v8::Value> v8_args = V8ValueFromScriptSource(context, arg_values);
    ASSERT_FALSE(v8_args.IsEmpty());
    ASSERT_TRUE(v8_args->IsArray());
    std::vector<v8::Local<v8::Value>> vector_args;
    ASSERT_TRUE(gin::ConvertFromV8(isolate(), v8_args, &vector_args));

    std::unique_ptr<base::ListValue> result;
    v8::Local<v8::Function> callback;
    std::string error;
    bool success = signature.ParseArgumentsToJSON(
        context, vector_args, type_refs_, &result, &callback, &error);
    EXPECT_EQ(should_succeed, success);
    ASSERT_EQ(should_succeed, !!result);
    EXPECT_EQ(expect_callback, !callback.IsEmpty());
    if (should_succeed) {
      EXPECT_EQ(ReplaceSingleQuotes(expected_parsed_args),
                ValueToString(*result));
    } else {
      EXPECT_EQ(expected_error, error);
    }
  }

  void RunResponseTest(const APISignature& signature,
                       base::StringPiece arg_values,
                       base::Optional<std::string> expected_error) {
    SCOPED_TRACE(arg_values);
    v8::Local<v8::Context> context = MainContext();
    v8::Local<v8::Value> v8_args = V8ValueFromScriptSource(context, arg_values);
    ASSERT_FALSE(v8_args.IsEmpty());
    ASSERT_TRUE(v8_args->IsArray());
    std::vector<v8::Local<v8::Value>> vector_args;
    ASSERT_TRUE(gin::ConvertFromV8(isolate(), v8_args, &vector_args));

    std::string error;
    bool should_succeed = !expected_error;
    bool success =
        signature.ValidateResponse(context, vector_args, type_refs_, &error);
    EXPECT_EQ(should_succeed, success) << error;
    ASSERT_EQ(should_succeed, error.empty());
    if (!should_succeed)
      EXPECT_EQ(*expected_error, error);
  }

  APITypeReferenceMap type_refs_;

  DISALLOW_COPY_AND_ASSIGN(APISignatureTest);
};

TEST_F(APISignatureTest, BasicSignatureParsing) {
  using namespace api_errors;

  v8::HandleScope handle_scope(isolate());

  {
    SCOPED_TRACE("OneString");
    auto signature = OneString();
    ExpectPass(*signature, "['foo']", "['foo']", false);
    ExpectPass(*signature, "['']", "['']", false);
    ExpectFailure(*signature, "[1]", NoMatchingSignature());
    ExpectFailure(*signature, "[]", NoMatchingSignature());
    ExpectFailure(*signature, "[{}]", NoMatchingSignature());
    ExpectFailure(*signature, "['foo', 'bar']", NoMatchingSignature());
  }

  {
    SCOPED_TRACE("StringAndInt");
    auto signature = StringAndInt();
    ExpectPass(*signature, "['foo', 42]", "['foo',42]", false);
    ExpectPass(*signature, "['foo', -1]", "['foo',-1]", false);
    ExpectFailure(*signature, "[1]", NoMatchingSignature());
    ExpectFailure(*signature, "['foo'];", NoMatchingSignature());
    ExpectFailure(*signature, "[1, 'foo']", NoMatchingSignature());
    ExpectFailure(*signature, "['foo', 'foo']", NoMatchingSignature());
    ExpectFailure(*signature, "['foo', '1']", NoMatchingSignature());
    ExpectFailure(*signature, "['foo', 2.3]", NoMatchingSignature());
  }

  {
    SCOPED_TRACE("StringOptionalIntAndBool");
    auto signature = StringOptionalIntAndBool();
    ExpectPass(*signature, "['foo', 42, true]", "['foo',42,true]", false);
    ExpectPass(*signature, "['foo', true]", "['foo',null,true]", false);
    ExpectFailure(*signature, "['foo', 'bar', true]", NoMatchingSignature());
  }

  {
    SCOPED_TRACE("OneObject");
    auto signature = OneObject();
    ExpectPass(*signature, "[{prop1: 'foo'}]", "[{'prop1':'foo'}]", false);
    ExpectFailure(*signature,
                  "[{ get prop1() { throw new Error('Badness'); } }]",
                  ArgumentError("obj", ScriptThrewError()));
  }

  {
    SCOPED_TRACE("NoArgs");
    auto signature = NoArgs();
    ExpectPass(*signature, "[]", "[]", false);
    ExpectFailure(*signature, "[0]", NoMatchingSignature());
    ExpectFailure(*signature, "['']", NoMatchingSignature());
    ExpectFailure(*signature, "[null]", NoMatchingSignature());
    ExpectFailure(*signature, "[undefined]", NoMatchingSignature());
  }

  {
    SCOPED_TRACE("IntAndCallback");
    auto signature = IntAndCallback();
    ExpectPass(*signature, "[1, function() {}]", "[1]", true);
    ExpectFailure(*signature, "[function() {}]", NoMatchingSignature());
    ExpectFailure(*signature, "[1]", NoMatchingSignature());
  }

  {
    SCOPED_TRACE("OptionalIntAndCallback");
    auto signature = OptionalIntAndCallback();
    ExpectPass(*signature, "[1, function() {}]", "[1]", true);
    ExpectPass(*signature, "[function() {}]", "[null]", true);
    ExpectFailure(*signature, "[1]", NoMatchingSignature());
  }

  {
    SCOPED_TRACE("OptionalCallback");
    auto signature = OptionalCallback();
    ExpectPass(*signature, "[function() {}]", "[]", true);
    ExpectPass(*signature, "[]", "[]", false);
    ExpectPass(*signature, "[undefined]", "[]", false);
    ExpectFailure(*signature, "[0]", NoMatchingSignature());
  }

  {
    SCOPED_TRACE("IntAnyOptionalObjectOptionalCallback");
    auto signature = IntAnyOptionalObjectOptionalCallback();
    ExpectPass(*signature, "[4, {foo: 'bar'}, function() {}]",
               "[4,{'foo':'bar'},null]", true);
    ExpectPass(*signature, "[4, {foo: 'bar'}]", "[4,{'foo':'bar'},null]",
               false);
    ExpectPass(*signature, "[4, {foo: 'bar'}, {}]", "[4,{'foo':'bar'},{}]",
               false);
    ExpectFailure(*signature, "[4, function() {}]",
                  ArgumentError("any", UnserializableValue()));
    ExpectFailure(*signature, "[4]", NoMatchingSignature());
  }

  {
    SCOPED_TRACE("OptionalObjectAndCallback");
    auto signature = OptionalObjectAndCallback();
    ExpectPass(*signature, "[{prop1: 1}]", "[{'prop1':1}]", false);
    ExpectPass(*signature, "[]", "[null]", false);
    ExpectPass(*signature, "[null]", "[null]", false);
    ExpectFailure(
        *signature, "[{prop1: 'str'}]",
        ArgumentError("obj", PropertyError("prop1", InvalidType(kTypeInteger,
                                                                kTypeString))));
    ExpectFailure(
        *signature, "[{prop1: 'str'}, function() {}]",
        ArgumentError("obj", PropertyError("prop1", InvalidType(kTypeInteger,
                                                                kTypeString))));
  }

  {
    SCOPED_TRACE("OptionalIntAndNumber");
    auto signature = OptionalIntAndNumber();
    ExpectPass(*signature, "[1.0, 1.0]", "[1,1.0]", false);
    ExpectPass(*signature, "[1, 1]", "[1,1.0]", false);
    ExpectPass(*signature, "[1.0]", "[null,1.0]", false);
    ExpectPass(*signature, "[1]", "[null,1.0]", false);
    ExpectFailure(*signature, "[1.0, null]", NoMatchingSignature());
    ExpectFailure(*signature, "[1, null]", NoMatchingSignature());
  }

  {
    SCOPED_TRACE("OptionalIntAndInt");
    auto signature = OptionalIntAndInt();
    ExpectPass(*signature, "[1.0, 1.0]", "[1,1]", false);
    ExpectPass(*signature, "[1, 1]", "[1,1]", false);
    ExpectPass(*signature, "[1.0]", "[null,1]", false);
    ExpectPass(*signature, "[1]", "[null,1]", false);
    ExpectFailure(*signature, "[1.0, null]", NoMatchingSignature());
    ExpectFailure(*signature, "[1, null]", NoMatchingSignature());
  }
}

TEST_F(APISignatureTest, TypeRefsTest) {
  using namespace api_errors;

  v8::HandleScope handle_scope(isolate());

  {
    auto signature = RefObj();
    ExpectPass(*signature, "[{prop1: 'foo'}]", "[{'prop1':'foo'}]", false);
    ExpectPass(*signature, "[{prop1: 'foo', prop2: 2}]",
               "[{'prop1':'foo','prop2':2}]", false);
    ExpectFailure(
        *signature, "[{prop1: 'foo', prop2: 'a'}]",
        ArgumentError("obj", PropertyError("prop2", InvalidType(kTypeInteger,
                                                                kTypeString))));
  }

  {
    auto signature = RefEnum();
    ExpectPass(*signature, "['alpha']", "['alpha']", false);
    ExpectPass(*signature, "['beta']", "['beta']", false);
    ExpectFailure(*signature, "['gamma']",
                  ArgumentError("enum", InvalidEnumValue({"alpha", "beta"})));
  }
}

TEST_F(APISignatureTest, ExpectedSignature) {
  EXPECT_EQ("string string", OneString()->GetExpectedSignature());
  EXPECT_EQ("string string, integer int",
            StringAndInt()->GetExpectedSignature());
  EXPECT_EQ("string string, optional integer int, boolean bool",
            StringOptionalIntAndBool()->GetExpectedSignature());
  EXPECT_EQ("object obj", OneObject()->GetExpectedSignature());
  EXPECT_EQ("", NoArgs()->GetExpectedSignature());
  EXPECT_EQ("integer int, function callback",
            IntAndCallback()->GetExpectedSignature());
  EXPECT_EQ("optional integer int, function callback",
            OptionalIntAndCallback()->GetExpectedSignature());
  EXPECT_EQ("optional function callback",
            OptionalCallback()->GetExpectedSignature());
  EXPECT_EQ(
      "integer int, any any, optional object obj, optional function callback",
      IntAnyOptionalObjectOptionalCallback()->GetExpectedSignature());
  EXPECT_EQ("refObj obj", RefObj()->GetExpectedSignature());
  EXPECT_EQ("refEnum enum", RefEnum()->GetExpectedSignature());
}

TEST_F(APISignatureTest, ParseIgnoringSchema) {
  v8::HandleScope handle_scope(isolate());
  v8::Local<v8::Context> context = MainContext();

  {
    // Test with providing an optional callback.
    auto signature = IntAndOptionalCallback();
    std::vector<v8::Local<v8::Value>> v8_args =
        StringToV8Vector(context, "[1, function() {}]");
    v8::Local<v8::Function> callback;
    std::unique_ptr<base::ListValue> parsed;
    EXPECT_TRUE(signature->ConvertArgumentsIgnoringSchema(context, v8_args,
                                                          &parsed, &callback));
    ASSERT_TRUE(parsed);
    EXPECT_EQ("[1]", ValueToString(*parsed));
    EXPECT_FALSE(callback.IsEmpty());
  }

  {
    // Test with omitting the optional callback.
    auto signature = IntAndOptionalCallback();
    std::vector<v8::Local<v8::Value>> v8_args =
        StringToV8Vector(context, "[1, null]");
    v8::Local<v8::Function> callback;
    std::unique_ptr<base::ListValue> parsed;
    EXPECT_TRUE(signature->ConvertArgumentsIgnoringSchema(context, v8_args,
                                                          &parsed, &callback));
    ASSERT_TRUE(parsed);
    EXPECT_EQ("[1]", ValueToString(*parsed));
    EXPECT_TRUE(callback.IsEmpty());
  }

  {
    // Test with providing something completely different than the spec, which
    // is (unfortunately) allowed and used.
    auto signature = OneString();
    std::vector<v8::Local<v8::Value>> v8_args =
        StringToV8Vector(context, "[{not: 'a string'}]");
    v8::Local<v8::Function> callback;
    std::unique_ptr<base::ListValue> parsed;
    EXPECT_TRUE(signature->ConvertArgumentsIgnoringSchema(context, v8_args,
                                                          &parsed, &callback));
    ASSERT_TRUE(parsed);
    EXPECT_EQ(R"([{"not":"a string"}])", ValueToString(*parsed));
    EXPECT_TRUE(callback.IsEmpty());
  }

  {
    auto signature = OneObject();
    std::vector<v8::Local<v8::Value>> v8_args = StringToV8Vector(
        context, "[{prop1: 'foo', other: 'bar', nullProp: null}]");
    v8::Local<v8::Function> callback;
    std::unique_ptr<base::ListValue> parsed;
    EXPECT_TRUE(signature->ConvertArgumentsIgnoringSchema(context, v8_args,
                                                          &parsed, &callback));
    ASSERT_TRUE(parsed);
    EXPECT_EQ(R"([{"other":"bar","prop1":"foo"}])", ValueToString(*parsed));
    EXPECT_TRUE(callback.IsEmpty());
  }
}

TEST_F(APISignatureTest, ParseArgumentsToV8) {
  v8::HandleScope handle_scope(isolate());
  v8::Local<v8::Context> context = MainContext();

  // Test that parsing a signature returns values that are free of tricky
  // getters. This is more thoroughly tested in the ArgumentSpec conversion
  // unittests, but verify that it applies to signature parsing.
  auto signature = OneObject();
  constexpr char kTrickyArgs[] = R"(
      [{
        get prop1() {
          if (this.got)
            return 'bar';
          this.got = true;
          return 'foo';
        },
        prop2: 'baz'
      }])";
  std::vector<v8::Local<v8::Value>> args =
      StringToV8Vector(context, kTrickyArgs);

  std::vector<v8::Local<v8::Value>> args_out;
  std::string error;
  ASSERT_TRUE(signature->ParseArgumentsToV8(context, args, type_refs(),
                                            &args_out, &error));
  ASSERT_EQ(1u, args_out.size());
  ASSERT_TRUE(args_out[0]->IsObject());
  gin::Dictionary dict(isolate(), args_out[0].As<v8::Object>());

  std::string prop1;
  ASSERT_TRUE(dict.Get("prop1", &prop1));
  EXPECT_EQ("foo", prop1);

  std::string prop2;
  ASSERT_TRUE(dict.Get("prop2", &prop2));
  EXPECT_EQ("baz", prop2);
}

// Tests response validation, which is stricter than typical validation.
TEST_F(APISignatureTest, ValidateResponse) {
  using namespace api_errors;

  v8::HandleScope handle_scope(isolate());

  {
    auto signature = StringAndInt();
    ExpectResponsePass(*signature, "['hello', 42]");
    ExpectResponseFailure(
        *signature, "['hello', 'goodbye']",
        ArgumentError("int", InvalidType(kTypeInteger, kTypeString)));
  }

  {
    auto signature = StringOptionalIntAndBool();
    ExpectResponsePass(*signature, "['hello', 42, true]");
    ExpectResponsePass(*signature, "['hello', null, true]");
    // Responses are not allowed to omit optional inner parameters.
    ExpectResponseFailure(
        *signature, "['hello', true]",
        ArgumentError("int", InvalidType(kTypeInteger, kTypeBoolean)));
  }

  {
    SpecVector specs;
    specs.push_back(
        ArgumentSpecBuilder(ArgumentType::STRING, "string").Build());
    specs.push_back(ArgumentSpecBuilder(ArgumentType::INTEGER, "int")
                        .MakeOptional()
                        .Build());
    auto signature = std::make_unique<APISignature>(std::move(specs));
    // Responses *are* allowed to omit optional trailing parameters (which will
    // then be `undefined` to the caller).
    ExpectResponsePass(*signature, "['hello']");

    ExpectResponseFailure(
        *signature, "['hello', true]",
        ArgumentError("int", InvalidType(kTypeInteger, kTypeBoolean)));
  }
}

}  // namespace extensions
