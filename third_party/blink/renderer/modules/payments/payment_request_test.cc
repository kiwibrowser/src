// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/payments/payment_request.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_testing.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/modules/payments/payment_test_helper.h"
#include "third_party/blink/renderer/platform/heap/heap_allocator.h"

namespace blink {
namespace {

TEST(PaymentRequestTest, NoExceptionWithValidData) {
  V8TestingScope scope;
  MakePaymentRequestOriginSecure(scope.GetDocument());
  PaymentRequest::Create(
      scope.GetExecutionContext(), BuildPaymentMethodDataForTest(),
      BuildPaymentDetailsInitForTest(), scope.GetExceptionState());

  EXPECT_FALSE(scope.GetExceptionState().HadException());
}

TEST(PaymentRequestTest, SupportedMethodListRequired) {
  V8TestingScope scope;
  MakePaymentRequestOriginSecure(scope.GetDocument());
  PaymentRequest::Create(
      scope.GetExecutionContext(), HeapVector<PaymentMethodData>(),
      BuildPaymentDetailsInitForTest(), scope.GetExceptionState());

  EXPECT_TRUE(scope.GetExceptionState().HadException());
  EXPECT_EQ(kV8TypeError, scope.GetExceptionState().Code());
}

TEST(PaymentRequestTest, NullShippingOptionWhenNoOptionsAvailable) {
  V8TestingScope scope;
  MakePaymentRequestOriginSecure(scope.GetDocument());
  PaymentDetailsInit details;
  details.setTotal(BuildPaymentItemForTest());
  PaymentOptions options;
  options.setRequestShipping(true);

  PaymentRequest* request = PaymentRequest::Create(
      scope.GetExecutionContext(), BuildPaymentMethodDataForTest(), details,
      options, scope.GetExceptionState());

  EXPECT_TRUE(request->shippingOption().IsNull());
}

TEST(PaymentRequestTest, NullShippingOptionWhenMultipleOptionsAvailable) {
  V8TestingScope scope;
  MakePaymentRequestOriginSecure(scope.GetDocument());
  PaymentDetailsInit details;
  details.setTotal(BuildPaymentItemForTest());
  details.setShippingOptions(
      HeapVector<PaymentShippingOption>(2, BuildShippingOptionForTest()));
  PaymentOptions options;
  options.setRequestShipping(true);

  PaymentRequest* request = PaymentRequest::Create(
      scope.GetExecutionContext(), BuildPaymentMethodDataForTest(), details,
      options, scope.GetExceptionState());

  EXPECT_TRUE(request->shippingOption().IsNull());
}

TEST(PaymentRequestTest, DontSelectSingleAvailableShippingOptionByDefault) {
  V8TestingScope scope;
  MakePaymentRequestOriginSecure(scope.GetDocument());
  PaymentDetailsInit details;
  details.setTotal(BuildPaymentItemForTest());
  details.setShippingOptions(HeapVector<PaymentShippingOption>(
      1, BuildShippingOptionForTest(kPaymentTestDataId,
                                    kPaymentTestOverwriteValue, "standard")));

  PaymentRequest* request = PaymentRequest::Create(
      scope.GetExecutionContext(), BuildPaymentMethodDataForTest(), details,
      scope.GetExceptionState());

  EXPECT_TRUE(request->shippingOption().IsNull());
}

TEST(PaymentRequestTest,
     DontSelectSingleAvailableShippingOptionWhenShippingNotRequested) {
  V8TestingScope scope;
  MakePaymentRequestOriginSecure(scope.GetDocument());
  PaymentDetailsInit details;
  details.setTotal(BuildPaymentItemForTest());
  details.setShippingOptions(
      HeapVector<PaymentShippingOption>(1, BuildShippingOptionForTest()));
  PaymentOptions options;
  options.setRequestShipping(false);

  PaymentRequest* request = PaymentRequest::Create(
      scope.GetExecutionContext(), BuildPaymentMethodDataForTest(), details,
      options, scope.GetExceptionState());

  EXPECT_TRUE(request->shippingOption().IsNull());
}

TEST(PaymentRequestTest,
     DontSelectSingleUnselectedShippingOptionWhenShippingRequested) {
  V8TestingScope scope;
  MakePaymentRequestOriginSecure(scope.GetDocument());
  PaymentDetailsInit details;
  details.setTotal(BuildPaymentItemForTest());
  details.setShippingOptions(
      HeapVector<PaymentShippingOption>(1, BuildShippingOptionForTest()));
  PaymentOptions options;
  options.setRequestShipping(true);

  PaymentRequest* request = PaymentRequest::Create(
      scope.GetExecutionContext(), BuildPaymentMethodDataForTest(), details,
      options, scope.GetExceptionState());

  EXPECT_TRUE(request->shippingOption().IsNull());
}

TEST(PaymentRequestTest,
     SelectSingleSelectedShippingOptionWhenShippingRequested) {
  V8TestingScope scope;
  MakePaymentRequestOriginSecure(scope.GetDocument());
  PaymentDetailsInit details;
  details.setTotal(BuildPaymentItemForTest());
  HeapVector<PaymentShippingOption> shipping_options(
      1, BuildShippingOptionForTest(kPaymentTestDataId,
                                    kPaymentTestOverwriteValue, "standard"));
  shipping_options[0].setSelected(true);
  details.setShippingOptions(shipping_options);
  PaymentOptions options;
  options.setRequestShipping(true);

  PaymentRequest* request = PaymentRequest::Create(
      scope.GetExecutionContext(), BuildPaymentMethodDataForTest(), details,
      options, scope.GetExceptionState());

  EXPECT_EQ("standard", request->shippingOption());
}

TEST(PaymentRequestTest,
     SelectOnlySelectedShippingOptionWhenShippingRequested) {
  V8TestingScope scope;
  MakePaymentRequestOriginSecure(scope.GetDocument());
  PaymentDetailsInit details;
  details.setTotal(BuildPaymentItemForTest());
  HeapVector<PaymentShippingOption> shipping_options(2);
  shipping_options[0] = BuildShippingOptionForTest(
      kPaymentTestDataId, kPaymentTestOverwriteValue, "standard");
  shipping_options[0].setSelected(true);
  shipping_options[1] = BuildShippingOptionForTest(
      kPaymentTestDataId, kPaymentTestOverwriteValue, "express");
  details.setShippingOptions(shipping_options);
  PaymentOptions options;
  options.setRequestShipping(true);

  PaymentRequest* request = PaymentRequest::Create(
      scope.GetExecutionContext(), BuildPaymentMethodDataForTest(), details,
      options, scope.GetExceptionState());

  EXPECT_EQ("standard", request->shippingOption());
}

TEST(PaymentRequestTest,
     SelectLastSelectedShippingOptionWhenShippingRequested) {
  V8TestingScope scope;
  MakePaymentRequestOriginSecure(scope.GetDocument());
  PaymentDetailsInit details;
  details.setTotal(BuildPaymentItemForTest());
  HeapVector<PaymentShippingOption> shipping_options(2);
  shipping_options[0] = BuildShippingOptionForTest(
      kPaymentTestDataId, kPaymentTestOverwriteValue, "standard");
  shipping_options[0].setSelected(true);
  shipping_options[1] = BuildShippingOptionForTest(
      kPaymentTestDataId, kPaymentTestOverwriteValue, "express");
  shipping_options[1].setSelected(true);
  details.setShippingOptions(shipping_options);
  PaymentOptions options;
  options.setRequestShipping(true);

  PaymentRequest* request = PaymentRequest::Create(
      scope.GetExecutionContext(), BuildPaymentMethodDataForTest(), details,
      options, scope.GetExceptionState());

  EXPECT_EQ("express", request->shippingOption());
}

TEST(PaymentRequestTest, NullShippingTypeWhenRequestShippingIsFalse) {
  V8TestingScope scope;
  MakePaymentRequestOriginSecure(scope.GetDocument());
  PaymentDetailsInit details;
  details.setTotal(BuildPaymentItemForTest());
  PaymentOptions options;
  options.setRequestShipping(false);

  PaymentRequest* request = PaymentRequest::Create(
      scope.GetExecutionContext(), BuildPaymentMethodDataForTest(), details,
      options, scope.GetExceptionState());

  EXPECT_TRUE(request->shippingType().IsNull());
}

TEST(PaymentRequestTest,
     DefaultShippingTypeWhenRequestShippingIsTrueWithNoSpecificType) {
  V8TestingScope scope;
  MakePaymentRequestOriginSecure(scope.GetDocument());
  PaymentDetailsInit details;
  details.setTotal(BuildPaymentItemForTest());
  PaymentOptions options;
  options.setRequestShipping(true);

  PaymentRequest* request = PaymentRequest::Create(
      scope.GetExecutionContext(), BuildPaymentMethodDataForTest(), details,
      options, scope.GetExceptionState());

  EXPECT_EQ("shipping", request->shippingType());
}

TEST(PaymentRequestTest, DeliveryShippingTypeWhenShippingTypeIsDelivery) {
  V8TestingScope scope;
  MakePaymentRequestOriginSecure(scope.GetDocument());
  PaymentDetailsInit details;
  details.setTotal(BuildPaymentItemForTest());
  PaymentOptions options;
  options.setRequestShipping(true);
  options.setShippingType("delivery");

  PaymentRequest* request = PaymentRequest::Create(
      scope.GetExecutionContext(), BuildPaymentMethodDataForTest(), details,
      options, scope.GetExceptionState());

  EXPECT_EQ("delivery", request->shippingType());
}

TEST(PaymentRequestTest, PickupShippingTypeWhenShippingTypeIsPickup) {
  V8TestingScope scope;
  MakePaymentRequestOriginSecure(scope.GetDocument());
  PaymentDetailsInit details;
  details.setTotal(BuildPaymentItemForTest());
  PaymentOptions options;
  options.setRequestShipping(true);
  options.setShippingType("pickup");

  PaymentRequest* request = PaymentRequest::Create(
      scope.GetExecutionContext(), BuildPaymentMethodDataForTest(), details,
      options, scope.GetExceptionState());

  EXPECT_EQ("pickup", request->shippingType());
}

TEST(PaymentRequestTest, RejectShowPromiseOnInvalidShippingAddress) {
  V8TestingScope scope;
  PaymentRequestMockFunctionScope funcs(scope.GetScriptState());
  MakePaymentRequestOriginSecure(scope.GetDocument());
  PaymentRequest* request = PaymentRequest::Create(
      scope.GetExecutionContext(), BuildPaymentMethodDataForTest(),
      BuildPaymentDetailsInitForTest(), scope.GetExceptionState());
  EXPECT_FALSE(scope.GetExceptionState().HadException());

  request->show(scope.GetScriptState())
      .Then(funcs.ExpectNoCall(), funcs.ExpectCall());

  static_cast<payments::mojom::blink::PaymentRequestClient*>(request)
      ->OnShippingAddressChange(payments::mojom::blink::PaymentAddress::New());
}

TEST(PaymentRequestTest, OnShippingOptionChange) {
  V8TestingScope scope;
  PaymentRequestMockFunctionScope funcs(scope.GetScriptState());
  MakePaymentRequestOriginSecure(scope.GetDocument());
  PaymentRequest* request = PaymentRequest::Create(
      scope.GetExecutionContext(), BuildPaymentMethodDataForTest(),
      BuildPaymentDetailsInitForTest(), scope.GetExceptionState());
  EXPECT_FALSE(scope.GetExceptionState().HadException());

  request->show(scope.GetScriptState())
      .Then(funcs.ExpectNoCall(), funcs.ExpectNoCall());

  static_cast<payments::mojom::blink::PaymentRequestClient*>(request)
      ->OnShippingOptionChange("standardShipping");
}

TEST(PaymentRequestTest, CannotCallShowTwice) {
  V8TestingScope scope;
  PaymentRequestMockFunctionScope funcs(scope.GetScriptState());
  MakePaymentRequestOriginSecure(scope.GetDocument());
  PaymentRequest* request = PaymentRequest::Create(
      scope.GetExecutionContext(), BuildPaymentMethodDataForTest(),
      BuildPaymentDetailsInitForTest(), scope.GetExceptionState());
  EXPECT_FALSE(scope.GetExceptionState().HadException());
  request->show(scope.GetScriptState());

  request->show(scope.GetScriptState())
      .Then(funcs.ExpectNoCall(), funcs.ExpectCall());
}

TEST(PaymentRequestTest, CannotShowAfterAborted) {
  V8TestingScope scope;
  PaymentRequestMockFunctionScope funcs(scope.GetScriptState());
  MakePaymentRequestOriginSecure(scope.GetDocument());
  PaymentRequest* request = PaymentRequest::Create(
      scope.GetExecutionContext(), BuildPaymentMethodDataForTest(),
      BuildPaymentDetailsInitForTest(), scope.GetExceptionState());
  EXPECT_FALSE(scope.GetExceptionState().HadException());
  request->show(scope.GetScriptState());
  request->abort(scope.GetScriptState());
  static_cast<payments::mojom::blink::PaymentRequestClient*>(request)->OnAbort(
      true);

  request->show(scope.GetScriptState())
      .Then(funcs.ExpectNoCall(), funcs.ExpectCall());
}

TEST(PaymentRequestTest, RejectShowPromiseOnErrorPaymentMethodNotSupported) {
  V8TestingScope scope;
  PaymentRequestMockFunctionScope funcs(scope.GetScriptState());
  MakePaymentRequestOriginSecure(scope.GetDocument());
  PaymentRequest* request = PaymentRequest::Create(
      scope.GetExecutionContext(), BuildPaymentMethodDataForTest(),
      BuildPaymentDetailsInitForTest(), scope.GetExceptionState());
  EXPECT_FALSE(scope.GetExceptionState().HadException());

  String error_message;
  request->show(scope.GetScriptState())
      .Then(funcs.ExpectNoCall(), funcs.ExpectCall(&error_message));

  static_cast<payments::mojom::blink::PaymentRequestClient*>(request)->OnError(
      payments::mojom::blink::PaymentErrorReason::NOT_SUPPORTED);

  v8::MicrotasksScope::PerformCheckpoint(scope.GetScriptState()->GetIsolate());
  EXPECT_EQ("NotSupportedError: The payment method \"foo\" is not supported",
            error_message);
}

TEST(PaymentRequestTest, RejectShowPromiseOnErrorCancelled) {
  V8TestingScope scope;
  PaymentRequestMockFunctionScope funcs(scope.GetScriptState());
  MakePaymentRequestOriginSecure(scope.GetDocument());
  PaymentRequest* request = PaymentRequest::Create(
      scope.GetExecutionContext(), BuildPaymentMethodDataForTest(),
      BuildPaymentDetailsInitForTest(), scope.GetExceptionState());
  EXPECT_FALSE(scope.GetExceptionState().HadException());

  String error_message;
  request->show(scope.GetScriptState())
      .Then(funcs.ExpectNoCall(), funcs.ExpectCall(&error_message));

  static_cast<payments::mojom::blink::PaymentRequestClient*>(request)->OnError(
      payments::mojom::blink::PaymentErrorReason::USER_CANCEL);

  v8::MicrotasksScope::PerformCheckpoint(scope.GetScriptState()->GetIsolate());
  EXPECT_EQ("AbortError: Request cancelled", error_message);
}

TEST(PaymentRequestTest, RejectShowPromiseOnUpdateDetailsFailure) {
  V8TestingScope scope;
  PaymentRequestMockFunctionScope funcs(scope.GetScriptState());
  MakePaymentRequestOriginSecure(scope.GetDocument());
  PaymentRequest* request = PaymentRequest::Create(
      scope.GetExecutionContext(), BuildPaymentMethodDataForTest(),
      BuildPaymentDetailsInitForTest(), scope.GetExceptionState());
  EXPECT_FALSE(scope.GetExceptionState().HadException());

  String error_message;
  request->show(scope.GetScriptState())
      .Then(funcs.ExpectNoCall(), funcs.ExpectCall(&error_message));

  request->OnUpdatePaymentDetailsFailure("oops");

  v8::MicrotasksScope::PerformCheckpoint(scope.GetScriptState()->GetIsolate());
  EXPECT_EQ("AbortError: oops", error_message);
}

TEST(PaymentRequestTest, IgnoreUpdatePaymentDetailsAfterShowPromiseResolved) {
  V8TestingScope scope;
  PaymentRequestMockFunctionScope funcs(scope.GetScriptState());
  MakePaymentRequestOriginSecure(scope.GetDocument());
  PaymentRequest* request = PaymentRequest::Create(
      scope.GetExecutionContext(), BuildPaymentMethodDataForTest(),
      BuildPaymentDetailsInitForTest(), scope.GetExceptionState());
  EXPECT_FALSE(scope.GetExceptionState().HadException());
  request->show(scope.GetScriptState())
      .Then(funcs.ExpectCall(), funcs.ExpectNoCall());
  static_cast<payments::mojom::blink::PaymentRequestClient*>(request)
      ->OnPaymentResponse(BuildPaymentResponseForTest());

  request->OnUpdatePaymentDetails(
      ScriptValue::From(scope.GetScriptState(), "foo"));
}

TEST(PaymentRequestTest, RejectShowPromiseOnNonPaymentDetailsUpdate) {
  V8TestingScope scope;
  PaymentRequestMockFunctionScope funcs(scope.GetScriptState());
  MakePaymentRequestOriginSecure(scope.GetDocument());
  PaymentRequest* request = PaymentRequest::Create(
      scope.GetExecutionContext(), BuildPaymentMethodDataForTest(),
      BuildPaymentDetailsInitForTest(), scope.GetExceptionState());
  EXPECT_FALSE(scope.GetExceptionState().HadException());

  request->show(scope.GetScriptState())
      .Then(funcs.ExpectNoCall(), funcs.ExpectCall());

  request->OnUpdatePaymentDetails(
      ScriptValue::From(scope.GetScriptState(), "NotPaymentDetails"));
}

TEST(PaymentRequestTest, RejectShowPromiseOnInvalidPaymentDetailsUpdate) {
  V8TestingScope scope;
  PaymentRequestMockFunctionScope funcs(scope.GetScriptState());
  MakePaymentRequestOriginSecure(scope.GetDocument());
  PaymentRequest* request = PaymentRequest::Create(
      scope.GetExecutionContext(), BuildPaymentMethodDataForTest(),
      BuildPaymentDetailsInitForTest(), scope.GetExceptionState());
  EXPECT_FALSE(scope.GetExceptionState().HadException());

  request->show(scope.GetScriptState())
      .Then(funcs.ExpectNoCall(), funcs.ExpectCall());

  request->OnUpdatePaymentDetails(ScriptValue::From(
      scope.GetScriptState(),
      FromJSONString(scope.GetScriptState()->GetIsolate(),
                     scope.GetScriptState()->GetContext(), "{\"total\": {}}",
                     scope.GetExceptionState())));
  EXPECT_FALSE(scope.GetExceptionState().HadException());
}

TEST(PaymentRequestTest,
     ClearShippingOptionOnPaymentDetailsUpdateWithoutShippingOptions) {
  V8TestingScope scope;
  PaymentRequestMockFunctionScope funcs(scope.GetScriptState());
  MakePaymentRequestOriginSecure(scope.GetDocument());
  PaymentDetailsInit details;
  details.setTotal(BuildPaymentItemForTest());
  PaymentOptions options;
  options.setRequestShipping(true);
  PaymentRequest* request = PaymentRequest::Create(
      scope.GetExecutionContext(), BuildPaymentMethodDataForTest(), details,
      options, scope.GetExceptionState());
  EXPECT_FALSE(scope.GetExceptionState().HadException());
  EXPECT_TRUE(request->shippingOption().IsNull());
  request->show(scope.GetScriptState())
      .Then(funcs.ExpectNoCall(), funcs.ExpectNoCall());
  String detail_with_shipping_options =
      "{\"total\": {\"label\": \"Total\", \"amount\": {\"currency\": \"USD\", "
      "\"value\": \"5.00\"}},"
      "\"shippingOptions\": [{\"id\": \"standardShippingOption\", \"label\": "
      "\"Standard shipping\", \"amount\": {\"currency\": \"USD\", \"value\": "
      "\"5.00\"}, \"selected\": true}]}";
  request->OnUpdatePaymentDetails(ScriptValue::From(
      scope.GetScriptState(),
      FromJSONString(scope.GetScriptState()->GetIsolate(),
                     scope.GetScriptState()->GetContext(),
                     detail_with_shipping_options, scope.GetExceptionState())));
  EXPECT_FALSE(scope.GetExceptionState().HadException());
  EXPECT_EQ("standardShippingOption", request->shippingOption());
  String detail_without_shipping_options =
      "{\"total\": {\"label\": \"Total\", \"amount\": {\"currency\": \"USD\", "
      "\"value\": \"5.00\"}}}";

  request->OnUpdatePaymentDetails(
      ScriptValue::From(scope.GetScriptState(),
                        FromJSONString(scope.GetScriptState()->GetIsolate(),
                                       scope.GetScriptState()->GetContext(),
                                       detail_without_shipping_options,
                                       scope.GetExceptionState())));

  EXPECT_FALSE(scope.GetExceptionState().HadException());
  EXPECT_TRUE(request->shippingOption().IsNull());
}

TEST(
    PaymentRequestTest,
    ClearShippingOptionOnPaymentDetailsUpdateWithMultipleUnselectedShippingOptions) {
  V8TestingScope scope;
  PaymentRequestMockFunctionScope funcs(scope.GetScriptState());
  MakePaymentRequestOriginSecure(scope.GetDocument());
  PaymentOptions options;
  options.setRequestShipping(true);
  PaymentRequest* request = PaymentRequest::Create(
      scope.GetExecutionContext(), BuildPaymentMethodDataForTest(),
      BuildPaymentDetailsInitForTest(), options, scope.GetExceptionState());
  EXPECT_FALSE(scope.GetExceptionState().HadException());
  request->show(scope.GetScriptState())
      .Then(funcs.ExpectNoCall(), funcs.ExpectNoCall());
  String detail =
      "{\"total\": {\"label\": \"Total\", \"amount\": {\"currency\": \"USD\", "
      "\"value\": \"5.00\"}},"
      "\"shippingOptions\": [{\"id\": \"slow\", \"label\": \"Slow\", "
      "\"amount\": {\"currency\": \"USD\", \"value\": \"5.00\"}},"
      "{\"id\": \"fast\", \"label\": \"Fast\", \"amount\": {\"currency\": "
      "\"USD\", \"value\": \"50.00\"}}]}";

  request->OnUpdatePaymentDetails(
      ScriptValue::From(scope.GetScriptState(),
                        FromJSONString(scope.GetScriptState()->GetIsolate(),
                                       scope.GetScriptState()->GetContext(),
                                       detail, scope.GetExceptionState())));
  EXPECT_FALSE(scope.GetExceptionState().HadException());

  EXPECT_TRUE(request->shippingOption().IsNull());
}

TEST(PaymentRequestTest, UseTheSelectedShippingOptionFromPaymentDetailsUpdate) {
  V8TestingScope scope;
  PaymentRequestMockFunctionScope funcs(scope.GetScriptState());
  MakePaymentRequestOriginSecure(scope.GetDocument());
  PaymentOptions options;
  options.setRequestShipping(true);
  PaymentRequest* request = PaymentRequest::Create(
      scope.GetExecutionContext(), BuildPaymentMethodDataForTest(),
      BuildPaymentDetailsInitForTest(), options, scope.GetExceptionState());
  EXPECT_FALSE(scope.GetExceptionState().HadException());
  request->show(scope.GetScriptState())
      .Then(funcs.ExpectNoCall(), funcs.ExpectNoCall());
  String detail =
      "{\"total\": {\"label\": \"Total\", \"amount\": {\"currency\": \"USD\", "
      "\"value\": \"5.00\"}},"
      "\"shippingOptions\": [{\"id\": \"slow\", \"label\": \"Slow\", "
      "\"amount\": {\"currency\": \"USD\", \"value\": \"5.00\"}},"
      "{\"id\": \"fast\", \"label\": \"Fast\", \"amount\": {\"currency\": "
      "\"USD\", \"value\": \"50.00\"}, \"selected\": true}]}";

  request->OnUpdatePaymentDetails(
      ScriptValue::From(scope.GetScriptState(),
                        FromJSONString(scope.GetScriptState()->GetIsolate(),
                                       scope.GetScriptState()->GetContext(),
                                       detail, scope.GetExceptionState())));
  EXPECT_FALSE(scope.GetExceptionState().HadException());

  EXPECT_EQ("fast", request->shippingOption());
}

TEST(PaymentRequestTest, NoExceptionWithErrorMessageInUpdate) {
  V8TestingScope scope;
  PaymentRequestMockFunctionScope funcs(scope.GetScriptState());
  MakePaymentRequestOriginSecure(scope.GetDocument());
  PaymentRequest* request = PaymentRequest::Create(
      scope.GetExecutionContext(), BuildPaymentMethodDataForTest(),
      BuildPaymentDetailsInitForTest(), scope.GetExceptionState());
  EXPECT_FALSE(scope.GetExceptionState().HadException());

  request->show(scope.GetScriptState())
      .Then(funcs.ExpectNoCall(), funcs.ExpectNoCall());
  String detail_with_error_msg =
      "{\"total\": {\"label\": \"Total\", \"amount\": {\"currency\": \"USD\", "
      "\"value\": \"5.00\"}},"
      "\"error\": \"This is an error message.\"}";

  request->OnUpdatePaymentDetails(ScriptValue::From(
      scope.GetScriptState(),
      FromJSONString(scope.GetScriptState()->GetIsolate(),
                     scope.GetScriptState()->GetContext(),
                     detail_with_error_msg, scope.GetExceptionState())));
  EXPECT_FALSE(scope.GetExceptionState().HadException());
}

TEST(PaymentRequestTest,
     ShouldResolveWithExceptionIfIDsOfShippingOptionsAreDuplicated) {
  V8TestingScope scope;
  PaymentRequestMockFunctionScope funcs(scope.GetScriptState());
  MakePaymentRequestOriginSecure(scope.GetDocument());
  PaymentDetailsInit details;
  details.setTotal(BuildPaymentItemForTest());
  HeapVector<PaymentShippingOption> shipping_options(2);
  shipping_options[0] = BuildShippingOptionForTest(
      kPaymentTestDataId, kPaymentTestOverwriteValue, "standard");
  shipping_options[0].setSelected(true);
  shipping_options[1] = BuildShippingOptionForTest(
      kPaymentTestDataId, kPaymentTestOverwriteValue, "standard");
  details.setShippingOptions(shipping_options);
  PaymentOptions options;
  options.setRequestShipping(true);
  PaymentRequest::Create(scope.GetExecutionContext(),
                         BuildPaymentMethodDataForTest(), details, options,
                         scope.GetExceptionState());
  EXPECT_TRUE(scope.GetExceptionState().HadException());
}

TEST(PaymentRequestTest, DetailsIdIsSet) {
  V8TestingScope scope;
  MakePaymentRequestOriginSecure(scope.GetDocument());
  PaymentDetailsInit details;
  details.setTotal(BuildPaymentItemForTest());
  details.setId("my_payment_id");

  PaymentRequest* request = PaymentRequest::Create(
      scope.GetExecutionContext(), BuildPaymentMethodDataForTest(), details,
      scope.GetExceptionState());

  EXPECT_EQ("my_payment_id", request->id());
}

}  // namespace
}  // namespace blink
