// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/background_fetch/background_fetch_settled_fetches.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_testing.h"
#include "third_party/blink/renderer/core/fetch/request.h"

namespace blink {

namespace {

WebVector<WebBackgroundFetchSettledFetch> CreateSettledFetches(
    const std::vector<String>& request_urls) {
  WebVector<WebBackgroundFetchSettledFetch> settled_fetches;
  settled_fetches.reserve(request_urls.size());
  for (const auto& request_url : request_urls) {
    WebBackgroundFetchSettledFetch settled_fetch;
    settled_fetch.request.SetURL(WebURL(KURL(request_url)));
    settled_fetches.emplace_back(settled_fetch);
  }
  return settled_fetches;
}

class TestScriptFunctionHandler {
  // Stats filled by the functions executed by test promises.
  struct CallStats {
    size_t num_calls = 0;
    ScriptValue value;
  };

  // ScriptFunction run by test promises. Extracts the resolved value.
  class TestScriptFunction : public ScriptFunction {
   public:
    static v8::Local<v8::Function> CreateFunction(ScriptState* script_state,
                                                  CallStats* stats) {
      return (new TestScriptFunction(script_state, stats))->BindToV8Function();
    }

    ScriptValue Call(ScriptValue value) override {
      stats_->value = value;
      stats_->num_calls++;
      return ScriptValue();
    }

   private:
    TestScriptFunction(ScriptState* script_state, CallStats* stats)
        : ScriptFunction(script_state), stats_(stats) {}
    // Pointer to the private CallStats member variable in
    // TestScriptFunctionHandler. Whenever the associated function is called,
    // the CallStats variable is updated. Internal values can be accessed via
    // the public getters.
    CallStats* stats_;
  };

 public:
  TestScriptFunctionHandler() = default;

  v8::Local<v8::Function> GetFunction(ScriptState* script_state) {
    return TestScriptFunction::CreateFunction(script_state, &stats_);
  }

  size_t NumCalls() const { return stats_.num_calls; }
  ScriptValue Value() const { return stats_.value; }

 private:
  CallStats stats_;
};

ScriptValue ResolvePromise(ScriptState* script_state, ScriptPromise& promise) {
  TestScriptFunctionHandler resolved;
  TestScriptFunctionHandler rejected;

  promise.Then(resolved.GetFunction(script_state),
               rejected.GetFunction(script_state));

  v8::MicrotasksScope::PerformCheckpoint(promise.GetIsolate());

  EXPECT_EQ(1ul, resolved.NumCalls());
  EXPECT_EQ(0ul, rejected.NumCalls());

  return resolved.Value();
}

}  // namespace

TEST(BackgroundFetchSettledFetchesTest, MatchNullValue) {
  V8TestingScope scope;
  RequestOrUSVString null_request;

  auto settled_fetches = CreateSettledFetches({"foo.com"});
  auto* bgf_settled_fetches = BackgroundFetchSettledFetches::Create(
      scope.GetScriptState(), settled_fetches);

  ScriptPromise promise =
      bgf_settled_fetches->match(scope.GetScriptState(), null_request);

  ScriptValue value = ResolvePromise(scope.GetScriptState(), promise);
  EXPECT_TRUE(value.IsNull());
}

TEST(BackgroundFetchSettledFetchesTest, MatchUSVString) {
  V8TestingScope scope;
  auto matched_request = RequestOrUSVString::FromUSVString("http://foo.com/");
  auto unmatched_request = RequestOrUSVString::FromUSVString("http://bar.com/");

  auto settled_fetches = CreateSettledFetches(
      {"http://t1.net/", "http://foo.com/", "http://t3.net/"});
  auto* bgf_settled_fetches = BackgroundFetchSettledFetches::Create(
      scope.GetScriptState(), settled_fetches);

  ScriptPromise matched_promise =
      bgf_settled_fetches->match(scope.GetScriptState(), matched_request);
  ScriptPromise unmatched_promise =
      bgf_settled_fetches->match(scope.GetScriptState(), unmatched_request);

  ScriptValue matched_value =
      ResolvePromise(scope.GetScriptState(), matched_promise);
  EXPECT_TRUE(matched_value.IsObject());

  ScriptValue unmatched_value =
      ResolvePromise(scope.GetScriptState(), unmatched_promise);
  EXPECT_TRUE(unmatched_value.IsNull());
}

TEST(BackgroundFetchSettledFetchesTest, MatchRequest) {
  V8TestingScope scope;

  auto matched_request = RequestOrUSVString::FromRequest(Request::Create(
      scope.GetScriptState(), "http://foo.com/", scope.GetExceptionState()));
  auto unmatched_request = RequestOrUSVString::FromRequest(Request::Create(
      scope.GetScriptState(), "http://bar.com/", scope.GetExceptionState()));

  auto settled_fetches = CreateSettledFetches(
      {"http://t1.net/", "http://foo.com/", "http://t3.net/"});
  auto* bgf_settled_fetches = BackgroundFetchSettledFetches::Create(
      scope.GetScriptState(), settled_fetches);

  ScriptPromise matched_promise =
      bgf_settled_fetches->match(scope.GetScriptState(), matched_request);
  ScriptPromise unmatched_promise =
      bgf_settled_fetches->match(scope.GetScriptState(), unmatched_request);

  ScriptValue matched_value =
      ResolvePromise(scope.GetScriptState(), matched_promise);
  EXPECT_TRUE(matched_value.IsObject());

  ScriptValue unmatched_value =
      ResolvePromise(scope.GetScriptState(), unmatched_promise);
  EXPECT_TRUE(unmatched_value.IsNull());
}

}  // namespace blink
