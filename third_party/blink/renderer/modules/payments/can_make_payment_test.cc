// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Tests for PaymentRequest::canMakePayment().

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_testing.h"
#include "third_party/blink/renderer/modules/payments/payment_request.h"
#include "third_party/blink/renderer/modules/payments/payment_test_helper.h"

namespace blink {
namespace {

using payments::mojom::blink::CanMakePaymentQueryResult;
using payments::mojom::blink::PaymentErrorReason;
using payments::mojom::blink::PaymentRequestClient;

TEST(CanMakePaymentTest, RejectPromiseOnUserCancel) {
  V8TestingScope scope;
  PaymentRequestMockFunctionScope funcs(scope.GetScriptState());
  MakePaymentRequestOriginSecure(scope.GetDocument());
  PaymentRequest* request = PaymentRequest::Create(
      scope.GetExecutionContext(), BuildPaymentMethodDataForTest(),
      BuildPaymentDetailsInitForTest(), scope.GetExceptionState());

  request->canMakePayment(scope.GetScriptState())
      .Then(funcs.ExpectNoCall(), funcs.ExpectCall());

  static_cast<PaymentRequestClient*>(request)->OnError(
      PaymentErrorReason::USER_CANCEL);
}

TEST(CanMakePaymentTest, RejectPromiseOnUnknownError) {
  V8TestingScope scope;
  PaymentRequestMockFunctionScope funcs(scope.GetScriptState());
  MakePaymentRequestOriginSecure(scope.GetDocument());
  PaymentRequest* request = PaymentRequest::Create(
      scope.GetExecutionContext(), BuildPaymentMethodDataForTest(),
      BuildPaymentDetailsInitForTest(), scope.GetExceptionState());

  request->canMakePayment(scope.GetScriptState())
      .Then(funcs.ExpectNoCall(), funcs.ExpectCall());

  static_cast<PaymentRequestClient*>(request)->OnError(
      PaymentErrorReason::UNKNOWN);
}

TEST(CanMakePaymentTest, RejectDuplicateRequest) {
  V8TestingScope scope;
  PaymentRequestMockFunctionScope funcs(scope.GetScriptState());
  MakePaymentRequestOriginSecure(scope.GetDocument());
  PaymentRequest* request = PaymentRequest::Create(
      scope.GetExecutionContext(), BuildPaymentMethodDataForTest(),
      BuildPaymentDetailsInitForTest(), scope.GetExceptionState());
  request->canMakePayment(scope.GetScriptState());

  request->canMakePayment(scope.GetScriptState())
      .Then(funcs.ExpectNoCall(), funcs.ExpectCall());
}

TEST(CanMakePaymentTest, RejectQueryQuotaExceeded) {
  V8TestingScope scope;
  PaymentRequestMockFunctionScope funcs(scope.GetScriptState());
  MakePaymentRequestOriginSecure(scope.GetDocument());
  PaymentRequest* request = PaymentRequest::Create(
      scope.GetExecutionContext(), BuildPaymentMethodDataForTest(),
      BuildPaymentDetailsInitForTest(), scope.GetExceptionState());

  request->canMakePayment(scope.GetScriptState())
      .Then(funcs.ExpectNoCall(), funcs.ExpectCall());

  static_cast<PaymentRequestClient*>(request)->OnCanMakePayment(
      CanMakePaymentQueryResult::QUERY_QUOTA_EXCEEDED);
}

TEST(CanMakePaymentTest, ReturnCannotMakeCanMakePayment) {
  V8TestingScope scope;
  PaymentRequestMockFunctionScope funcs(scope.GetScriptState());
  MakePaymentRequestOriginSecure(scope.GetDocument());
  PaymentRequest* request = PaymentRequest::Create(
      scope.GetExecutionContext(), BuildPaymentMethodDataForTest(),
      BuildPaymentDetailsInitForTest(), scope.GetExceptionState());
  String captor;
  request->canMakePayment(scope.GetScriptState())
      .Then(funcs.ExpectCall(&captor), funcs.ExpectNoCall());

  static_cast<PaymentRequestClient*>(request)->OnCanMakePayment(
      CanMakePaymentQueryResult::CANNOT_MAKE_PAYMENT);

  v8::MicrotasksScope::PerformCheckpoint(scope.GetScriptState()->GetIsolate());
  EXPECT_EQ("false", captor);
}

TEST(CanMakePaymentTest, ReturnCanMakePayment) {
  V8TestingScope scope;
  PaymentRequestMockFunctionScope funcs(scope.GetScriptState());
  MakePaymentRequestOriginSecure(scope.GetDocument());
  PaymentRequest* request = PaymentRequest::Create(
      scope.GetExecutionContext(), BuildPaymentMethodDataForTest(),
      BuildPaymentDetailsInitForTest(), scope.GetExceptionState());
  String captor;
  request->canMakePayment(scope.GetScriptState())
      .Then(funcs.ExpectCall(&captor), funcs.ExpectNoCall());

  static_cast<PaymentRequestClient*>(request)->OnCanMakePayment(
      CanMakePaymentQueryResult::CAN_MAKE_PAYMENT);

  v8::MicrotasksScope::PerformCheckpoint(scope.GetScriptState()->GetIsolate());
  EXPECT_EQ("true", captor);
}

}  // namespace
}  // namespace blink
