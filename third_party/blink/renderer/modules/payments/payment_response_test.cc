// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/payments/payment_response.h"

#include <memory>
#include <utility>
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/bindings/core/v8/script_value.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_testing.h"
#include "third_party/blink/renderer/modules/payments/payment_address.h"
#include "third_party/blink/renderer/modules/payments/payment_completer.h"
#include "third_party/blink/renderer/modules/payments/payment_test_helper.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"

namespace blink {
namespace {

class MockPaymentCompleter
    : public GarbageCollectedFinalized<MockPaymentCompleter>,
      public PaymentCompleter {
  USING_GARBAGE_COLLECTED_MIXIN(MockPaymentCompleter);
  WTF_MAKE_NONCOPYABLE(MockPaymentCompleter);

 public:
  MockPaymentCompleter() {
    ON_CALL(*this, Complete(testing::_, testing::_))
        .WillByDefault(testing::ReturnPointee(&dummy_promise_));
  }

  ~MockPaymentCompleter() override = default;

  MOCK_METHOD2(Complete, ScriptPromise(ScriptState*, PaymentComplete result));

  void Trace(blink::Visitor* visitor) override {}

 private:
  ScriptPromise dummy_promise_;
};

TEST(PaymentResponseTest, DataCopiedOver) {
  V8TestingScope scope;
  payments::mojom::blink::PaymentResponsePtr input =
      BuildPaymentResponseForTest();
  input->method_name = "foo";
  input->stringified_details = "{\"transactionId\": 123}";
  input->shipping_option = "standardShippingOption";
  input->payer_name = "Jon Doe";
  input->payer_email = "abc@gmail.com";
  input->payer_phone = "0123";
  MockPaymentCompleter* complete_callback = new MockPaymentCompleter;

  PaymentResponse* output =
      new PaymentResponse(std::move(input), nullptr, complete_callback, "id");

  EXPECT_EQ("foo", output->methodName());
  EXPECT_EQ("standardShippingOption", output->shippingOption());
  EXPECT_EQ("Jon Doe", output->payerName());
  EXPECT_EQ("abc@gmail.com", output->payerEmail());
  EXPECT_EQ("0123", output->payerPhone());
  EXPECT_EQ("id", output->requestId());

  ScriptValue details =
      output->details(scope.GetScriptState(), scope.GetExceptionState());

  ASSERT_FALSE(scope.GetExceptionState().HadException());
  ASSERT_TRUE(details.V8Value()->IsObject());

  ScriptValue transaction_id(
      scope.GetScriptState(),
      details.V8Value().As<v8::Object>()->Get(
          V8String(scope.GetScriptState()->GetIsolate(), "transactionId")));

  ASSERT_TRUE(transaction_id.V8Value()->IsNumber());
  EXPECT_EQ(123, transaction_id.V8Value().As<v8::Number>()->Value());
}

TEST(PaymentResponseTest, PaymentResponseDetailsJSONObject) {
  V8TestingScope scope;
  payments::mojom::blink::PaymentResponsePtr input =
      BuildPaymentResponseForTest();
  input->stringified_details = "transactionId";
  MockPaymentCompleter* complete_callback = new MockPaymentCompleter;
  PaymentResponse* output =
      new PaymentResponse(std::move(input), nullptr, complete_callback, "id");

  ScriptValue details =
      output->details(scope.GetScriptState(), scope.GetExceptionState());

  ASSERT_TRUE(scope.GetExceptionState().HadException());
}

TEST(PaymentResponseTest, CompleteCalledWithSuccess) {
  V8TestingScope scope;
  payments::mojom::blink::PaymentResponsePtr input =
      BuildPaymentResponseForTest();
  input->method_name = "foo";
  input->stringified_details = "{\"transactionId\": 123}";
  MockPaymentCompleter* complete_callback = new MockPaymentCompleter;
  PaymentResponse* output =
      new PaymentResponse(std::move(input), nullptr, complete_callback, "id");

  EXPECT_CALL(*complete_callback,
              Complete(scope.GetScriptState(), PaymentCompleter::kSuccess));

  output->complete(scope.GetScriptState(), "success");
}

TEST(PaymentResponseTest, CompleteCalledWithFailure) {
  V8TestingScope scope;
  payments::mojom::blink::PaymentResponsePtr input =
      BuildPaymentResponseForTest();
  input->method_name = "foo";
  input->stringified_details = "{\"transactionId\": 123}";
  MockPaymentCompleter* complete_callback = new MockPaymentCompleter;
  PaymentResponse* output =
      new PaymentResponse(std::move(input), nullptr, complete_callback, "id");

  EXPECT_CALL(*complete_callback,
              Complete(scope.GetScriptState(), PaymentCompleter::kFail));

  output->complete(scope.GetScriptState(), "fail");
}

TEST(PaymentResponseTest, JSONSerializerTest) {
  V8TestingScope scope;
  payments::mojom::blink::PaymentResponsePtr input =
      payments::mojom::blink::PaymentResponse::New();
  input->method_name = "foo";
  input->stringified_details = "{\"transactionId\": 123}";
  input->shipping_option = "standardShippingOption";
  input->payer_email = "abc@gmail.com";
  input->payer_phone = "0123";
  input->payer_name = "Jon Doe";
  input->shipping_address = payments::mojom::blink::PaymentAddress::New();
  input->shipping_address->country = "US";
  input->shipping_address->language_code = "en";
  input->shipping_address->script_code = "Latn";
  input->shipping_address->address_line.push_back("340 Main St");
  input->shipping_address->address_line.push_back("BIN1");
  input->shipping_address->address_line.push_back("First floor");
  PaymentAddress* address =
      new PaymentAddress(std::move(input->shipping_address));

  PaymentResponse* output = new PaymentResponse(std::move(input), address,
                                                new MockPaymentCompleter, "id");
  ScriptValue json_object = output->toJSONForBinding(scope.GetScriptState());
  EXPECT_TRUE(json_object.IsObject());

  String json_string = V8StringToWebCoreString<String>(
      v8::JSON::Stringify(scope.GetContext(),
                          json_object.V8Value().As<v8::Object>())
          .ToLocalChecked(),
      kDoNotExternalize);
  String expected =
      "{\"requestId\":\"id\",\"methodName\":\"foo\",\"details\":{"
      "\"transactionId\":123},"
      "\"shippingAddress\":{\"country\":\"US\",\"addressLine\":[\"340 Main "
      "St\","
      "\"BIN1\",\"First "
      "floor\"],\"region\":\"\",\"city\":\"\",\"dependentLocality\":"
      "\"\",\"postalCode\":\"\",\"sortingCode\":\"\",\"languageCode\":\"en-"
      "Latn\","
      "\"organization\":\"\",\"recipient\":\"\",\"phone\":\"\"},"
      "\"shippingOption\":"
      "\"standardShippingOption\",\"payerName\":\"Jon Doe\","
      "\"payerEmail\":\"abc@gmail.com\",\"payerPhone\":\"0123\"}";
  EXPECT_EQ(expected, json_string);
}

}  // namespace
}  // namespace blink
