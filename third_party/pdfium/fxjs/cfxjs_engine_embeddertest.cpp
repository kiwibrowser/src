// Copyright 2015 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "fxjs/cfxjs_engine.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "testing/js_embedder_test.h"

namespace {

const double kExpected0 = 6.0;
const double kExpected1 = 7.0;
const double kExpected2 = 8.0;

const wchar_t kScript0[] = L"fred = 6";
const wchar_t kScript1[] = L"fred = 7";
const wchar_t kScript2[] = L"fred = 8";

}  // namespace

class CFXJSEngineEmbedderTest : public JSEmbedderTest {
 public:
  Optional<IJS_Runtime::JS_Error> ExecuteInCurrentContext(
      const WideString& script) {
    auto* current_engine =
        CFXJS_Engine::EngineFromIsolateCurrentContext(isolate());
    return current_engine->Execute(script);
  }

  void CheckAssignmentInCurrentContext(double expected) {
    auto* current_engine =
        CFXJS_Engine::EngineFromIsolateCurrentContext(isolate());
    v8::Local<v8::Object> This = current_engine->GetThisObj();
    v8::Local<v8::Value> fred =
        current_engine->GetObjectProperty(This, L"fred");
    EXPECT_TRUE(fred->IsNumber());
    EXPECT_EQ(expected, current_engine->ToDouble(fred));
  }
};

TEST_F(CFXJSEngineEmbedderTest, Getters) {
  v8::Isolate::Scope isolate_scope(isolate());
  v8::HandleScope handle_scope(isolate());
  v8::Context::Scope context_scope(GetV8Context());

  Optional<IJS_Runtime::JS_Error> err =
      ExecuteInCurrentContext(WideString(kScript1));
  EXPECT_FALSE(err);
  CheckAssignmentInCurrentContext(kExpected1);
}

TEST_F(CFXJSEngineEmbedderTest, MultipleEngines) {
  v8::Isolate::Scope isolate_scope(isolate());
  v8::HandleScope handle_scope(isolate());

  CFXJS_Engine engine1(isolate());
  engine1.InitializeEngine();

  CFXJS_Engine engine2(isolate());
  engine2.InitializeEngine();

  v8::Local<v8::Context> context1 = engine1.GetV8Context();
  v8::Local<v8::Context> context2 = engine2.GetV8Context();

  v8::Context::Scope context_scope(GetV8Context());
  Optional<IJS_Runtime::JS_Error> err =
      ExecuteInCurrentContext(WideString(kScript0));
  EXPECT_FALSE(err);
  CheckAssignmentInCurrentContext(kExpected0);

  {
    v8::Context::Scope context_scope1(context1);
    Optional<IJS_Runtime::JS_Error> err =
        ExecuteInCurrentContext(WideString(kScript1));
    EXPECT_FALSE(err);
    CheckAssignmentInCurrentContext(kExpected1);
  }
  {
    v8::Context::Scope context_scope2(context2);
    Optional<IJS_Runtime::JS_Error> err =
        ExecuteInCurrentContext(WideString(kScript2));
    EXPECT_FALSE(err);
    CheckAssignmentInCurrentContext(kExpected2);
  }

  CheckAssignmentInCurrentContext(kExpected0);

  {
    v8::Context::Scope context_scope1(context1);
    CheckAssignmentInCurrentContext(kExpected1);
    {
      v8::Context::Scope context_scope2(context2);
      CheckAssignmentInCurrentContext(kExpected2);
    }
    CheckAssignmentInCurrentContext(kExpected1);
  }
  {
    v8::Context::Scope context_scope2(context2);
    CheckAssignmentInCurrentContext(kExpected2);
    {
      v8::Context::Scope context_scope1(context1);
      CheckAssignmentInCurrentContext(kExpected1);
    }
    CheckAssignmentInCurrentContext(kExpected2);
  }

  CheckAssignmentInCurrentContext(kExpected0);

  engine1.ReleaseEngine();
  engine2.ReleaseEngine();
}

TEST_F(CFXJSEngineEmbedderTest, JSCompileError) {
  v8::Isolate::Scope isolate_scope(isolate());
  v8::HandleScope handle_scope(isolate());
  v8::Context::Scope context_scope(GetV8Context());

  Optional<IJS_Runtime::JS_Error> err =
      ExecuteInCurrentContext(L"functoon(x) { return x+1; }");
  EXPECT_TRUE(err);
  EXPECT_EQ(L"SyntaxError: Unexpected token {", err->exception);
  EXPECT_EQ(1, err->line);
  EXPECT_EQ(12, err->column);
}

TEST_F(CFXJSEngineEmbedderTest, JSRuntimeError) {
  v8::Isolate::Scope isolate_scope(isolate());
  v8::HandleScope handle_scope(isolate());
  v8::Context::Scope context_scope(GetV8Context());

  Optional<IJS_Runtime::JS_Error> err =
      ExecuteInCurrentContext(L"let a = 3;\nundefined.colour");
  EXPECT_TRUE(err);
  EXPECT_EQ(L"TypeError: Cannot read property 'colour' of undefined",
            err->exception);
  EXPECT_EQ(2, err->line);
  EXPECT_EQ(10, err->column);
}
