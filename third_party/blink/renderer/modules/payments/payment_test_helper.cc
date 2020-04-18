// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/payments/payment_test_helper.h"

#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/modules/payments/payment_currency_amount.h"
#include "third_party/blink/renderer/modules/payments/payment_method_data.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/heap/heap_allocator.h"

namespace blink {
namespace {

static int g_unique_id = 0;
// PaymentItem and PaymentShippingOption have identical structure
// except for the "id" field, which is present only in PaymentShippingOption.
template <typename PaymentItemOrPaymentShippingOption>
void SetValues(PaymentItemOrPaymentShippingOption& original,
               PaymentTestDataToChange data,
               PaymentTestModificationType modification_type,
               const String& value_to_use) {
  PaymentCurrencyAmount item_amount;
  if (data == kPaymentTestDataCurrencyCode) {
    if (modification_type == kPaymentTestOverwriteValue)
      item_amount.setCurrency(value_to_use);
  } else {
    item_amount.setCurrency("USD");
  }

  if (data == kPaymentTestDataValue) {
    if (modification_type == kPaymentTestOverwriteValue)
      item_amount.setValue(value_to_use);
  } else {
    item_amount.setValue("9.99");
  }

  if (data != kPaymentTestDataAmount ||
      modification_type != kPaymentTestRemoveKey)
    original.setAmount(item_amount);

  if (data == kPaymentTestDataLabel) {
    if (modification_type == kPaymentTestOverwriteValue)
      original.setLabel(value_to_use);
  } else {
    original.setLabel("Label");
  }
}

void BuildPaymentDetailsBase(PaymentTestDetailToChange detail,
                             PaymentTestDataToChange data,
                             PaymentTestModificationType modification_type,
                             const String& value_to_use,
                             PaymentDetailsBase* details) {
  PaymentItem item;
  if (detail == kPaymentTestDetailItem)
    item = BuildPaymentItemForTest(data, modification_type, value_to_use);
  else
    item = BuildPaymentItemForTest();

  PaymentShippingOption shipping_option;
  if (detail == kPaymentTestDetailShippingOption) {
    shipping_option =
        BuildShippingOptionForTest(data, modification_type, value_to_use);
  } else {
    shipping_option = BuildShippingOptionForTest();
  }

  PaymentDetailsModifier modifier;
  if (detail == kPaymentTestDetailModifierTotal ||
      detail == kPaymentTestDetailModifierItem) {
    modifier = BuildPaymentDetailsModifierForTest(
        detail, data, modification_type, value_to_use);
  } else {
    modifier = BuildPaymentDetailsModifierForTest();
  }

  details->setDisplayItems(HeapVector<PaymentItem>(1, item));
  details->setShippingOptions(
      HeapVector<PaymentShippingOption>(1, shipping_option));
  details->setModifiers(HeapVector<PaymentDetailsModifier>(1, modifier));
}

}  // namespace

PaymentItem BuildPaymentItemForTest(
    PaymentTestDataToChange data,
    PaymentTestModificationType modification_type,
    const String& value_to_use) {
  DCHECK_NE(data, kPaymentTestDataId);
  PaymentItem item;
  SetValues(item, data, modification_type, value_to_use);
  return item;
}

PaymentShippingOption BuildShippingOptionForTest(
    PaymentTestDataToChange data,
    PaymentTestModificationType modification_type,
    const String& value_to_use) {
  PaymentShippingOption shipping_option;
  if (data == kPaymentTestDataId) {
    if (modification_type == kPaymentTestOverwriteValue)
      shipping_option.setId(value_to_use);
  } else {
    shipping_option.setId("id" + String::Number(g_unique_id++));
  }
  SetValues(shipping_option, data, modification_type, value_to_use);
  return shipping_option;
}

PaymentDetailsModifier BuildPaymentDetailsModifierForTest(
    PaymentTestDetailToChange detail,
    PaymentTestDataToChange data,
    PaymentTestModificationType modification_type,
    const String& value_to_use) {
  PaymentItem total;
  if (detail == kPaymentTestDetailModifierTotal)
    total = BuildPaymentItemForTest(data, modification_type, value_to_use);
  else
    total = BuildPaymentItemForTest();

  PaymentItem item;
  if (detail == kPaymentTestDetailModifierItem)
    item = BuildPaymentItemForTest(data, modification_type, value_to_use);
  else
    item = BuildPaymentItemForTest();

  PaymentDetailsModifier modifier;
  StringOrStringSequence supportedMethods;
  supportedMethods.SetStringSequence(Vector<String>(1, "foo"));
  modifier.setSupportedMethods(supportedMethods);
  modifier.setTotal(total);
  modifier.setAdditionalDisplayItems(HeapVector<PaymentItem>(1, item));
  return modifier;
}

PaymentDetailsInit BuildPaymentDetailsInitForTest(
    PaymentTestDetailToChange detail,
    PaymentTestDataToChange data,
    PaymentTestModificationType modification_type,
    const String& value_to_use) {
  PaymentDetailsInit details;
  BuildPaymentDetailsBase(detail, data, modification_type, value_to_use,
                          &details);

  if (detail == kPaymentTestDetailTotal) {
    details.setTotal(
        BuildPaymentItemForTest(data, modification_type, value_to_use));
  } else {
    details.setTotal(BuildPaymentItemForTest());
  }

  return details;
}

PaymentDetailsUpdate BuildPaymentDetailsUpdateForTest(
    PaymentTestDetailToChange detail,
    PaymentTestDataToChange data,
    PaymentTestModificationType modification_type,
    const String& value_to_use) {
  PaymentDetailsUpdate details;
  BuildPaymentDetailsBase(detail, data, modification_type, value_to_use,
                          &details);

  if (detail == kPaymentTestDetailTotal) {
    details.setTotal(
        BuildPaymentItemForTest(data, modification_type, value_to_use));
  } else {
    details.setTotal(BuildPaymentItemForTest());
  }

  if (detail == kPaymentTestDetailError)
    details.setError(value_to_use);

  return details;
}

PaymentDetailsUpdate BuildPaymentDetailsErrorMsgForTest(
    const String& value_to_use) {
  return BuildPaymentDetailsUpdateForTest(
      kPaymentTestDetailError, kPaymentTestDataNone, kPaymentTestOverwriteValue,
      value_to_use);
}

HeapVector<PaymentMethodData> BuildPaymentMethodDataForTest() {
  HeapVector<PaymentMethodData> method_data(1, PaymentMethodData());
  StringOrStringSequence supportedMethods;
  supportedMethods.SetStringSequence(Vector<String>(1, "foo"));
  method_data[0].setSupportedMethods(supportedMethods);
  return method_data;
}

payments::mojom::blink::PaymentResponsePtr BuildPaymentResponseForTest() {
  payments::mojom::blink::PaymentResponsePtr result =
      payments::mojom::blink::PaymentResponse::New();
  return result;
}

void MakePaymentRequestOriginSecure(Document& document) {
  document.SetSecurityOrigin(
      SecurityOrigin::Create(KURL("https://www.example.com/")));
  document.SetSecureContextStateForTesting(SecureContextState::kSecure);
}

PaymentRequestMockFunctionScope::PaymentRequestMockFunctionScope(
    ScriptState* script_state)
    : script_state_(script_state) {}

PaymentRequestMockFunctionScope::~PaymentRequestMockFunctionScope() {
  v8::MicrotasksScope::PerformCheckpoint(script_state_->GetIsolate());
  for (MockFunction* mock_function : mock_functions_) {
    testing::Mock::VerifyAndClearExpectations(mock_function);
  }
}

v8::Local<v8::Function> PaymentRequestMockFunctionScope::ExpectCall(
    String* captor) {
  mock_functions_.push_back(new MockFunction(script_state_, captor));
  EXPECT_CALL(*mock_functions_.back(), Call(testing::_));
  return mock_functions_.back()->Bind();
}

v8::Local<v8::Function> PaymentRequestMockFunctionScope::ExpectCall() {
  mock_functions_.push_back(new MockFunction(script_state_));
  EXPECT_CALL(*mock_functions_.back(), Call(testing::_));
  return mock_functions_.back()->Bind();
}

v8::Local<v8::Function> PaymentRequestMockFunctionScope::ExpectNoCall() {
  mock_functions_.push_back(new MockFunction(script_state_));
  EXPECT_CALL(*mock_functions_.back(), Call(testing::_)).Times(0);
  return mock_functions_.back()->Bind();
}

ACTION_P(SaveValueIn, captor) {
  *captor = ToCoreString(arg0.V8Value()
                             ->ToString(arg0.GetScriptState()->GetContext())
                             .ToLocalChecked());
}

PaymentRequestMockFunctionScope::MockFunction::MockFunction(
    ScriptState* script_state)
    : ScriptFunction(script_state) {
  ON_CALL(*this, Call(testing::_)).WillByDefault(testing::ReturnArg<0>());
}

PaymentRequestMockFunctionScope::MockFunction::MockFunction(
    ScriptState* script_state,
    String* captor)
    : ScriptFunction(script_state), value_(captor) {
  ON_CALL(*this, Call(testing::_))
      .WillByDefault(
          testing::DoAll(SaveValueIn(value_), testing::ReturnArg<0>()));
}

v8::Local<v8::Function> PaymentRequestMockFunctionScope::MockFunction::Bind() {
  return BindToV8Function();
}

}  // namespace blink
