// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Tests for PaymentRequest::complete().

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_testing.h"
#include "third_party/blink/renderer/modules/payments/payment_request.h"
#include "third_party/blink/renderer/modules/payments/payment_test_helper.h"

namespace blink {
namespace {

TEST(CompleteTest, CannotCallCompleteTwice) {
  V8TestingScope scope;
  PaymentRequestMockFunctionScope funcs(scope.GetScriptState());
  MakePaymentRequestOriginSecure(scope.GetDocument());
  PaymentRequest* request = PaymentRequest::Create(
      scope.GetExecutionContext(), BuildPaymentMethodDataForTest(),
      BuildPaymentDetailsInitForTest(), scope.GetExceptionState());
  EXPECT_FALSE(scope.GetExceptionState().HadException());
  request->show(scope.GetScriptState());
  static_cast<payments::mojom::blink::PaymentRequestClient*>(request)
      ->OnPaymentResponse(BuildPaymentResponseForTest());
  request->Complete(scope.GetScriptState(), PaymentCompleter::kFail);

  request->Complete(scope.GetScriptState(), PaymentCompleter::kSuccess)
      .Then(funcs.ExpectNoCall(), funcs.ExpectCall());
}

TEST(CompleteTest, ResolveCompletePromiseOnUnknownError) {
  V8TestingScope scope;
  PaymentRequestMockFunctionScope funcs(scope.GetScriptState());
  MakePaymentRequestOriginSecure(scope.GetDocument());
  PaymentRequest* request = PaymentRequest::Create(
      scope.GetExecutionContext(), BuildPaymentMethodDataForTest(),
      BuildPaymentDetailsInitForTest(), scope.GetExceptionState());
  EXPECT_FALSE(scope.GetExceptionState().HadException());
  request->show(scope.GetScriptState());
  static_cast<payments::mojom::blink::PaymentRequestClient*>(request)
      ->OnPaymentResponse(BuildPaymentResponseForTest());

  request->Complete(scope.GetScriptState(), PaymentCompleter::kSuccess)
      .Then(funcs.ExpectCall(), funcs.ExpectNoCall());

  static_cast<payments::mojom::blink::PaymentRequestClient*>(request)->OnError(
      payments::mojom::blink::PaymentErrorReason::UNKNOWN);
}

TEST(CompleteTest, ResolveCompletePromiseOnUserClosingUI) {
  V8TestingScope scope;
  PaymentRequestMockFunctionScope funcs(scope.GetScriptState());
  MakePaymentRequestOriginSecure(scope.GetDocument());
  PaymentRequest* request = PaymentRequest::Create(
      scope.GetExecutionContext(), BuildPaymentMethodDataForTest(),
      BuildPaymentDetailsInitForTest(), scope.GetExceptionState());
  EXPECT_FALSE(scope.GetExceptionState().HadException());
  request->show(scope.GetScriptState());
  static_cast<payments::mojom::blink::PaymentRequestClient*>(request)
      ->OnPaymentResponse(BuildPaymentResponseForTest());

  request->Complete(scope.GetScriptState(), PaymentCompleter::kSuccess)
      .Then(funcs.ExpectCall(), funcs.ExpectNoCall());

  static_cast<payments::mojom::blink::PaymentRequestClient*>(request)->OnError(
      payments::mojom::blink::PaymentErrorReason::USER_CANCEL);
}

// If user cancels the transaction during processing, the complete() promise
// should be rejected.
TEST(CompleteTest, RejectCompletePromiseAfterError) {
  V8TestingScope scope;
  PaymentRequestMockFunctionScope funcs(scope.GetScriptState());
  MakePaymentRequestOriginSecure(scope.GetDocument());
  PaymentRequest* request = PaymentRequest::Create(
      scope.GetExecutionContext(), BuildPaymentMethodDataForTest(),
      BuildPaymentDetailsInitForTest(), scope.GetExceptionState());
  EXPECT_FALSE(scope.GetExceptionState().HadException());
  request->show(scope.GetScriptState());
  static_cast<payments::mojom::blink::PaymentRequestClient*>(request)
      ->OnPaymentResponse(BuildPaymentResponseForTest());
  static_cast<payments::mojom::blink::PaymentRequestClient*>(request)->OnError(
      payments::mojom::blink::PaymentErrorReason::USER_CANCEL);

  request->Complete(scope.GetScriptState(), PaymentCompleter::kSuccess)
      .Then(funcs.ExpectNoCall(), funcs.ExpectCall());
}

TEST(CompleteTest, ResolvePromiseOnComplete) {
  V8TestingScope scope;
  PaymentRequestMockFunctionScope funcs(scope.GetScriptState());
  MakePaymentRequestOriginSecure(scope.GetDocument());
  PaymentRequest* request = PaymentRequest::Create(
      scope.GetExecutionContext(), BuildPaymentMethodDataForTest(),
      BuildPaymentDetailsInitForTest(), scope.GetExceptionState());
  EXPECT_FALSE(scope.GetExceptionState().HadException());
  request->show(scope.GetScriptState());
  static_cast<payments::mojom::blink::PaymentRequestClient*>(request)
      ->OnPaymentResponse(BuildPaymentResponseForTest());

  request->Complete(scope.GetScriptState(), PaymentCompleter::kSuccess)
      .Then(funcs.ExpectCall(), funcs.ExpectNoCall());

  static_cast<payments::mojom::blink::PaymentRequestClient*>(request)
      ->OnComplete();
}

TEST(CompleteTest, RejectCompletePromiseOnUpdateDetailsFailure) {
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

  String error_message;
  request->Complete(scope.GetScriptState(), PaymentCompleter::kSuccess)
      .Then(funcs.ExpectNoCall(), funcs.ExpectCall(&error_message));

  request->OnUpdatePaymentDetailsFailure("oops");

  v8::MicrotasksScope::PerformCheckpoint(scope.GetScriptState()->GetIsolate());
  EXPECT_EQ("AbortError: oops", error_message);
}

TEST(CompleteTest, RejectCompletePromiseAfterTimeout) {
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
  request->OnCompleteTimeoutForTesting();

  String error_message;
  request->Complete(scope.GetScriptState(), PaymentCompleter::kSuccess)
      .Then(funcs.ExpectNoCall(), funcs.ExpectCall(&error_message));

  v8::MicrotasksScope::PerformCheckpoint(scope.GetScriptState()->GetIsolate());
  EXPECT_EQ(
      "InvalidStateError: Timed out after 60 seconds, complete() called too "
      "late",
      error_message);
}

}  // namespace
}  // namespace blink
