// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/payments/payment_event_data_conversion.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/modules/payments/web_payment_request_event_data.h"
#include "third_party/blink/renderer/bindings/core/v8/script_value.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_testing.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"

namespace blink {
namespace {

static WebPaymentCurrencyAmount CreateWebPaymentCurrencyAmountForTest() {
  WebPaymentCurrencyAmount web_currency_amount;
  web_currency_amount.currency = WebString::FromUTF8("USD");
  web_currency_amount.value = WebString::FromUTF8("9.99");
  return web_currency_amount;
}

static WebPaymentMethodData CreateWebPaymentMethodDataForTest() {
  WebPaymentMethodData web_method_data;
  WebString method = WebString::FromUTF8("foo");
  web_method_data.supported_methods = WebVector<WebString>(&method, 1);
  web_method_data.stringified_data = "{\"merchantId\":\"12345\"}";
  return web_method_data;
}

static WebCanMakePaymentEventData CreateWebCanMakePaymentEventDataForTest() {
  WebCanMakePaymentEventData web_data;
  web_data.top_origin = WebString::FromUTF8("https://example.com");
  web_data.payment_request_origin = WebString::FromUTF8("https://example.com");
  Vector<WebPaymentMethodData> method_data;
  method_data.push_back(CreateWebPaymentMethodDataForTest());
  web_data.method_data = WebVector<WebPaymentMethodData>(method_data);
  return web_data;
}

static WebPaymentRequestEventData CreateWebPaymentRequestEventDataForTest() {
  WebPaymentRequestEventData web_data;
  web_data.top_origin = WebString::FromUTF8("https://example.com");
  web_data.payment_request_origin = WebString::FromUTF8("https://example.com");
  web_data.payment_request_id = WebString::FromUTF8("payment-request-id");
  Vector<WebPaymentMethodData> method_data;
  method_data.push_back(CreateWebPaymentMethodDataForTest());
  web_data.method_data = WebVector<WebPaymentMethodData>(method_data);
  web_data.total = CreateWebPaymentCurrencyAmountForTest();
  web_data.instrument_key = WebString::FromUTF8("payment-instrument-key");
  return web_data;
}

TEST(PaymentEventDataConversionTest, ToCanMakePaymentEventData) {
  V8TestingScope scope;
  WebCanMakePaymentEventData web_data =
      CreateWebCanMakePaymentEventDataForTest();
  CanMakePaymentEventInit data =
      PaymentEventDataConversion::ToCanMakePaymentEventInit(
          scope.GetScriptState(), web_data);

  ASSERT_TRUE(data.hasTopOrigin());
  EXPECT_EQ("https://example.com", data.topOrigin());

  ASSERT_TRUE(data.hasPaymentRequestOrigin());
  EXPECT_EQ("https://example.com", data.paymentRequestOrigin());

  ASSERT_TRUE(data.hasMethodData());
  ASSERT_EQ(1UL, data.methodData().size());
  ASSERT_TRUE(data.methodData().front().hasSupportedMethods());
  ASSERT_EQ(1UL, data.methodData()
                     .front()
                     .supportedMethods()
                     .GetAsStringSequence()
                     .size());
  ASSERT_EQ("foo", data.methodData()
                       .front()
                       .supportedMethods()
                       .GetAsStringSequence()
                       .front());
  ASSERT_TRUE(data.methodData().front().hasData());
  ASSERT_TRUE(data.methodData().front().data().IsObject());
  String stringified_data = V8StringToWebCoreString<String>(
      v8::JSON::Stringify(
          scope.GetContext(),
          data.methodData().front().data().V8Value().As<v8::Object>())
          .ToLocalChecked(),
      kDoNotExternalize);
  EXPECT_EQ("{\"merchantId\":\"12345\"}", stringified_data);
}

TEST(PaymentEventDataConversionTest, ToPaymentRequestEventData) {
  V8TestingScope scope;
  WebPaymentRequestEventData web_data =
      CreateWebPaymentRequestEventDataForTest();
  PaymentRequestEventInit data =
      PaymentEventDataConversion::ToPaymentRequestEventInit(
          scope.GetScriptState(), web_data);

  ASSERT_TRUE(data.hasTopOrigin());
  EXPECT_EQ("https://example.com", data.topOrigin());

  ASSERT_TRUE(data.hasPaymentRequestOrigin());
  EXPECT_EQ("https://example.com", data.paymentRequestOrigin());

  ASSERT_TRUE(data.hasPaymentRequestId());
  EXPECT_EQ("payment-request-id", data.paymentRequestId());

  ASSERT_TRUE(data.hasMethodData());
  ASSERT_EQ(1UL, data.methodData().size());
  ASSERT_TRUE(data.methodData().front().hasSupportedMethods());
  ASSERT_EQ(1UL, data.methodData()
                     .front()
                     .supportedMethods()
                     .GetAsStringSequence()
                     .size());
  ASSERT_EQ("foo", data.methodData()
                       .front()
                       .supportedMethods()
                       .GetAsStringSequence()
                       .front());
  ASSERT_TRUE(data.methodData().front().hasData());
  ASSERT_TRUE(data.methodData().front().data().IsObject());
  String stringified_data = V8StringToWebCoreString<String>(
      v8::JSON::Stringify(
          scope.GetContext(),
          data.methodData().front().data().V8Value().As<v8::Object>())
          .ToLocalChecked(),
      kDoNotExternalize);
  EXPECT_EQ("{\"merchantId\":\"12345\"}", stringified_data);

  ASSERT_TRUE(data.hasTotal());
  ASSERT_TRUE(data.total().hasCurrency());
  EXPECT_EQ("USD", data.total().currency());
  ASSERT_TRUE(data.total().hasValue());
  EXPECT_EQ("9.99", data.total().value());

  ASSERT_TRUE(data.hasInstrumentKey());
  EXPECT_EQ("payment-instrument-key", data.instrumentKey());
}

}  // namespace
}  // namespace blink
