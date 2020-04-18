// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/payments/payment_request.h"

#include <stddef.h>
#include <utility>
#include "base/location.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/public/platform/web_feature.mojom-blink.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/bindings/core/v8/script_regexp.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_string_resource.h"
#include "third_party/blink/renderer/bindings/modules/v8/v8_android_pay_method_data.h"
#include "third_party/blink/renderer/bindings/modules/v8/v8_basic_card_request.h"
#include "third_party/blink/renderer/bindings/modules/v8/v8_payment_details_update.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/dom/events/event_queue.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/core/event_type_names.h"
#include "third_party/blink/renderer/core/frame/deprecation.h"
#include "third_party/blink/renderer/core/frame/frame_owner.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/frame/use_counter.h"
#include "third_party/blink/renderer/core/frame/web_feature.h"
#include "third_party/blink/renderer/core/html/html_iframe_element.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"
#include "third_party/blink/renderer/core/inspector/console_types.h"
#include "third_party/blink/renderer/modules/event_target_modules_names.h"
#include "third_party/blink/renderer/modules/payments/android_pay_method_data.h"
#include "third_party/blink/renderer/modules/payments/android_pay_tokenization.h"
#include "third_party/blink/renderer/modules/payments/basic_card_helper.h"
#include "third_party/blink/renderer/modules/payments/basic_card_request.h"
#include "third_party/blink/renderer/modules/payments/html_iframe_element_payments.h"
#include "third_party/blink/renderer/modules/payments/payment_address.h"
#include "third_party/blink/renderer/modules/payments/payment_details_init.h"
#include "third_party/blink/renderer/modules/payments/payment_details_update.h"
#include "third_party/blink/renderer/modules/payments/payment_item.h"
#include "third_party/blink/renderer/modules/payments/payment_request_update_event.h"
#include "third_party/blink/renderer/modules/payments/payment_response.h"
#include "third_party/blink/renderer/modules/payments/payment_shipping_option.h"
#include "third_party/blink/renderer/modules/payments/payments_validators.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/feature_policy/feature_policy.h"
#include "third_party/blink/renderer/platform/mojo/mojo_helper.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/uuid.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/wtf/hash_set.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"

namespace {

using ::payments::mojom::blink::CanMakePaymentQueryResult;
using ::payments::mojom::blink::PaymentAddress;
using ::payments::mojom::blink::PaymentAddressPtr;
using ::payments::mojom::blink::PaymentCurrencyAmount;
using ::payments::mojom::blink::PaymentCurrencyAmountPtr;
using ::payments::mojom::blink::PaymentDetailsModifierPtr;
using ::payments::mojom::blink::PaymentDetailsPtr;
using ::payments::mojom::blink::PaymentErrorReason;
using ::payments::mojom::blink::PaymentItemPtr;
using ::payments::mojom::blink::PaymentMethodDataPtr;
using ::payments::mojom::blink::PaymentOptionsPtr;
using ::payments::mojom::blink::PaymentResponsePtr;
using ::payments::mojom::blink::PaymentShippingOptionPtr;
using ::payments::mojom::blink::PaymentShippingType;

}  // namespace

namespace mojo {

template <>
struct TypeConverter<PaymentCurrencyAmountPtr, blink::PaymentCurrencyAmount> {
  static PaymentCurrencyAmountPtr Convert(
      const blink::PaymentCurrencyAmount& input) {
    PaymentCurrencyAmountPtr output = PaymentCurrencyAmount::New();
    output->currency = input.currency().UpperASCII();
    output->value = input.value();
    return output;
  }
};

template <>
struct TypeConverter<PaymentItemPtr, blink::PaymentItem> {
  static PaymentItemPtr Convert(const blink::PaymentItem& input) {
    PaymentItemPtr output = payments::mojom::blink::PaymentItem::New();
    output->label = input.label();
    output->amount = PaymentCurrencyAmount::From(input.amount());
    output->pending = input.pending();
    return output;
  }
};

template <>
struct TypeConverter<PaymentShippingOptionPtr, blink::PaymentShippingOption> {
  static PaymentShippingOptionPtr Convert(
      const blink::PaymentShippingOption& input) {
    PaymentShippingOptionPtr output =
        payments::mojom::blink::PaymentShippingOption::New();
    output->id = input.id();
    output->label = input.label();
    output->amount = PaymentCurrencyAmount::From(input.amount());
    output->selected = input.hasSelected() && input.selected();
    return output;
  }
};

template <>
struct TypeConverter<PaymentOptionsPtr, blink::PaymentOptions> {
  static PaymentOptionsPtr Convert(const blink::PaymentOptions& input) {
    PaymentOptionsPtr output = payments::mojom::blink::PaymentOptions::New();
    output->request_payer_name = input.requestPayerName();
    output->request_payer_email = input.requestPayerEmail();
    output->request_payer_phone = input.requestPayerPhone();
    output->request_shipping = input.requestShipping();

    if (input.shippingType() == "delivery")
      output->shipping_type = PaymentShippingType::DELIVERY;
    else if (input.shippingType() == "pickup")
      output->shipping_type = PaymentShippingType::PICKUP;
    else
      output->shipping_type = PaymentShippingType::SHIPPING;

    return output;
  }
};

}  // namespace mojo

namespace blink {
namespace {

// If the website does not call complete() 60 seconds after show() has been
// resolved, then behave as if the website called complete("fail").
static const int kCompleteTimeoutSeconds = 60;

// Validates ShippingOption or PaymentItem, which happen to have identical
// fields, except for "id", which is present only in ShippingOption.
template <typename T>
void ValidateShippingOptionOrPaymentItem(const T& item,
                                         const String& item_name,
                                         ExecutionContext& execution_context,
                                         ExceptionState& exception_state) {
  DCHECK(item.hasLabel());
  DCHECK(item.hasAmount());
  DCHECK(item.amount().hasValue());
  DCHECK(item.amount().hasCurrency());

  if (item.label().length() > PaymentRequest::kMaxStringLength) {
    exception_state.ThrowTypeError("The label for " + item_name +
                                   " cannot be longer than 1024 characters");
    return;
  }

  if (item.amount().currency().length() > PaymentRequest::kMaxStringLength) {
    exception_state.ThrowTypeError("The currency code for " + item_name +
                                   " cannot be longer than 1024 characters");
    return;
  }

  if (item.amount().value().length() > PaymentRequest::kMaxStringLength) {
    exception_state.ThrowTypeError("The amount value for " + item_name +
                                   " cannot be longer than 1024 characters");
    return;
  }

  String error_message;
  if (!PaymentsValidators::IsValidAmountFormat(item.amount().value(), item_name,
                                               &error_message)) {
    exception_state.ThrowTypeError(error_message);
    return;
  }

  if (item.label().IsEmpty()) {
    execution_context.AddConsoleMessage(ConsoleMessage::Create(
        kJSMessageSource, kErrorMessageLevel,
        "Empty " + item_name + " label may be confusing the user"));
    return;
  }

  if (!PaymentsValidators::IsValidCurrencyCodeFormat(item.amount().currency(),
                                                     &error_message)) {
    exception_state.ThrowRangeError(error_message);
    return;
  }
}

void ValidateAndConvertDisplayItems(const HeapVector<PaymentItem>& input,
                                    const String& item_names,
                                    Vector<PaymentItemPtr>& output,
                                    ExecutionContext& execution_context,
                                    ExceptionState& exception_state) {
  if (input.size() > PaymentRequest::kMaxListSize) {
    exception_state.ThrowTypeError("At most 1024 " + item_names + " allowed");
    return;
  }

  for (const PaymentItem& item : input) {
    ValidateShippingOptionOrPaymentItem(item, item_names, execution_context,
                                        exception_state);
    if (exception_state.HadException())
      return;
    output.push_back(payments::mojom::blink::PaymentItem::From(item));
  }
}

// Validates and converts |input| shipping options into |output|. Throws an
// exception if the data is not valid, except for duplicate identifiers, which
// returns an empty |output| instead of throwing an exception. There's no need
// to clear |output| when an exception is thrown, because the caller takes care
// of deleting |output|.
void ValidateAndConvertShippingOptions(
    const HeapVector<PaymentShippingOption>& input,
    Vector<PaymentShippingOptionPtr>& output,
    String& shipping_option_output,
    ExecutionContext& execution_context,
    ExceptionState& exception_state) {
  if (input.size() > PaymentRequest::kMaxListSize) {
    exception_state.ThrowTypeError("At most 1024 shipping options allowed");
    return;
  }

  HashSet<String> unique_ids;
  for (const PaymentShippingOption& option : input) {
    ValidateShippingOptionOrPaymentItem(option, "shippingOptions",
                                        execution_context, exception_state);
    if (exception_state.HadException())
      return;

    DCHECK(option.hasId());
    if (option.id().length() > PaymentRequest::kMaxStringLength) {
      exception_state.ThrowTypeError(
          "Shipping option ID cannot be longer than 1024 characters");
      return;
    }

    if (option.id().IsEmpty()) {
      execution_context.AddConsoleMessage(ConsoleMessage::Create(
          kJSMessageSource, kWarningMessageLevel,
          "Empty shipping option ID may be hard to debug"));
      return;
    }

    if (unique_ids.Contains(option.id())) {
      exception_state.ThrowTypeError(
          "Cannot have duplicate shipping option identifiers");
      return;
    }

    if (option.selected())
      shipping_option_output = option.id();

    unique_ids.insert(option.id());

    output.push_back(
        payments::mojom::blink::PaymentShippingOption::From(option));
  }
}

void ValidateAndConvertTotal(const PaymentItem& input,
                             const String& item_name,
                             PaymentItemPtr& output,
                             ExecutionContext& execution_context,
                             ExceptionState& exception_state) {
  ValidateShippingOptionOrPaymentItem(input, item_name, execution_context,
                                      exception_state);
  if (exception_state.HadException())
    return;

  if (input.amount().value()[0] == '-') {
    exception_state.ThrowTypeError("Total amount value should be non-negative");
    return;
  }

  output = payments::mojom::blink::PaymentItem::From(input);
}

// Parses Android Pay data to avoid parsing JSON in the browser.
void SetAndroidPayMethodData(const ScriptValue& input,
                             PaymentMethodDataPtr& output,
                             ExceptionState& exception_state) {
  AndroidPayMethodData android_pay;
  V8AndroidPayMethodData::ToImpl(input.GetIsolate(), input.V8Value(),
                                 android_pay, exception_state);
  if (exception_state.HadException())
    return;

  if (android_pay.hasEnvironment() && android_pay.environment() == "TEST")
    output->environment = payments::mojom::blink::AndroidPayEnvironment::TEST;

  if (android_pay.hasMerchantName() &&
      android_pay.merchantName().length() > PaymentRequest::kMaxStringLength) {
    exception_state.ThrowTypeError(
        "Android Pay merchant name cannot be longer than 1024 characters");
    return;
  }
  output->merchant_name = android_pay.merchantName();

  if (android_pay.hasMerchantId() &&
      android_pay.merchantId().length() > PaymentRequest::kMaxStringLength) {
    exception_state.ThrowTypeError(
        "Android Pay merchant id cannot be longer than 1024 characters");
    return;
  }
  output->merchant_id = android_pay.merchantId();

  // 0 means the merchant did not specify or it was an invalid value
  output->min_google_play_services_version = 0;
  if (android_pay.hasMinGooglePlayServicesVersion()) {
    bool ok = false;
    int min_google_play_services_version =
        android_pay.minGooglePlayServicesVersion().ToIntStrict(&ok);
    if (ok) {
      output->min_google_play_services_version =
          min_google_play_services_version;
    }
  }

  // 0 means the merchant did not specify or it was an invalid value
  output->api_version = 0;
  if (android_pay.hasApiVersion())
    output->api_version = android_pay.apiVersion();

  if (android_pay.hasAllowedCardNetworks()) {
    using ::payments::mojom::blink::AndroidPayCardNetwork;

    const struct {
      const AndroidPayCardNetwork code;
      const char* const name;
    } kAndroidPayNetwork[] = {{AndroidPayCardNetwork::AMEX, "AMEX"},
                              {AndroidPayCardNetwork::DISCOVER, "DISCOVER"},
                              {AndroidPayCardNetwork::MASTERCARD, "MASTERCARD"},
                              {AndroidPayCardNetwork::VISA, "VISA"}};

    for (const String& allowed_card_network :
         android_pay.allowedCardNetworks()) {
      for (size_t i = 0; i < arraysize(kAndroidPayNetwork); ++i) {
        if (allowed_card_network == kAndroidPayNetwork[i].name) {
          output->allowed_card_networks.push_back(kAndroidPayNetwork[i].code);
          break;
        }
      }
    }
  }

  if (android_pay.hasPaymentMethodTokenizationParameters()) {
    const blink::AndroidPayTokenization& tokenization =
        android_pay.paymentMethodTokenizationParameters();
    output->tokenization_type =
        payments::mojom::blink::AndroidPayTokenization::UNSPECIFIED;
    if (tokenization.hasTokenizationType()) {
      using ::payments::mojom::blink::AndroidPayTokenization;

      const struct {
        const AndroidPayTokenization code;
        const char* const name;
      } kAndroidPayTokenization[] = {
          {AndroidPayTokenization::GATEWAY_TOKEN, "GATEWAY_TOKEN"},
          {AndroidPayTokenization::NETWORK_TOKEN, "NETWORK_TOKEN"}};

      for (size_t i = 0; i < arraysize(kAndroidPayTokenization); ++i) {
        if (tokenization.tokenizationType() ==
            kAndroidPayTokenization[i].name) {
          output->tokenization_type = kAndroidPayTokenization[i].code;
          break;
        }
      }
    }

    if (tokenization.hasParameters()) {
      const Vector<String>& keys =
          tokenization.parameters().GetPropertyNames(exception_state);
      if (exception_state.HadException())
        return;
      if (keys.size() > PaymentRequest::kMaxListSize) {
        exception_state.ThrowTypeError(
            "At most 1024 tokenization parameters allowed for Android Pay");
        return;
      }
      String value;
      for (const String& key : keys) {
        if (!DictionaryHelper::Get(tokenization.parameters(), key, value))
          continue;
        if (key.length() > PaymentRequest::kMaxStringLength) {
          exception_state.ThrowTypeError(
              "Android Pay tokenization parameter key cannot be longer than "
              "1024 characters");
          return;
        }
        if (value.length() > PaymentRequest::kMaxStringLength) {
          exception_state.ThrowTypeError(
              "Android Pay tokenization parameter value cannot be longer than "
              "1024 characters");
          return;
        }
        output->parameters.push_back(
            payments::mojom::blink::AndroidPayTokenizationParameter::New());
        output->parameters.back()->key = key;
        output->parameters.back()->value = value;
      }
    }
  }
}

// Parses basic-card data to avoid parsing JSON in the browser.
void SetBasicCardMethodData(const ScriptValue& input,
                            PaymentMethodDataPtr& output,
                            ExceptionState& exception_state) {
  BasicCardHelper::ParseBasiccardData(input, output->supported_networks,
                                      output->supported_types, exception_state);
}

void StringifyAndParseMethodSpecificData(
    const Vector<String>& supported_methods,
    const ScriptValue& input,
    PaymentMethodDataPtr& output,
    ExceptionState& exception_state) {
  DCHECK(!input.IsEmpty());
  v8::Local<v8::String> value;
  if (!input.V8Value()->IsObject() ||
      !v8::JSON::Stringify(input.GetContext(), input.V8Value().As<v8::Object>())
           .ToLocal(&value)) {
    exception_state.ThrowTypeError(
        "Payment method data should be a JSON-serializable object");
    return;
  }

  output->stringified_data =
      V8StringToWebCoreString<String>(value, kDoNotExternalize);

  if (output->stringified_data.length() >
      PaymentRequest::kMaxJSONStringLength) {
    exception_state.ThrowTypeError(
        "JSON serialization of payment method data should be no longer than "
        "1048576 characters");
    return;
  }

  // Serialize payment method specific data to be sent to the payment apps. The
  // payment apps are responsible for validating and processing their method
  // data asynchronously. Do not throw exceptions here.
  if (supported_methods.Contains("https://android.com/pay") ||
      supported_methods.Contains("https://google.com/pay")) {
    SetAndroidPayMethodData(input, output, exception_state);
    if (exception_state.HadException())
      exception_state.ClearException();
  }
  if (RuntimeEnabledFeatures::PaymentRequestBasicCardEnabled() &&
      supported_methods.Contains("basic-card")) {
    SetBasicCardMethodData(input, output, exception_state);
    if (exception_state.HadException())
      exception_state.ClearException();
  }
}

void CountPaymentRequestNetworkNameInSupportedMethods(
    const Vector<String>& supported_methods,
    ExecutionContext& execution_context) {
  if (BasicCardHelper::ContainsNetworkNames(supported_methods)) {
    Deprecation::CountDeprecation(
        &execution_context,
        WebFeature::kPaymentRequestNetworkNameInSupportedMethods);
  }
}

// Implements the PMI validation algorithm from:
// https://www.w3.org/TR/payment-method-id/#dfn-validate-a-payment-method-identifier
bool IsValidMethodFormat(const String& identifier) {
  KURL url(NullURL(), identifier);
  if (url.IsValid()) {
    // URL PMI validation rules:
    // https://www.w3.org/TR/payment-method-id/#dfn-validate-a-url-based-payment-method-identifier
    return url.Protocol() == "https" && url.User().IsEmpty() &&
           url.Pass().IsEmpty();
  } else {
    // Syntax for a valid standardized PMI:
    // https://www.w3.org/TR/payment-method-id/#dfn-syntax-of-a-standardized-payment-method-identifier
    return ScriptRegexp("^[a-z]+[0-9a-z]*(-[a-z]+[0-9a-z]*)*$",
                        kTextCaseSensitive)
               .Match(identifier) == 0;
  }
}

void ValidateAndConvertPaymentDetailsModifiers(
    const HeapVector<PaymentDetailsModifier>& input,
    Vector<PaymentDetailsModifierPtr>& output,
    ExecutionContext& execution_context,
    ExceptionState& exception_state) {
  if (input.size() > PaymentRequest::kMaxListSize) {
    exception_state.ThrowTypeError("At most 1024 modifiers allowed");
    return;
  }

  for (const PaymentDetailsModifier& modifier : input) {
    output.push_back(payments::mojom::blink::PaymentDetailsModifier::New());
    if (modifier.hasTotal()) {
      ValidateAndConvertTotal(modifier.total(), "modifier total",
                              output.back()->total, execution_context,
                              exception_state);
      if (exception_state.HadException())
        return;
    }

    if (modifier.hasAdditionalDisplayItems()) {
      ValidateAndConvertDisplayItems(modifier.additionalDisplayItems(),
                                     "additional display items in modifier",
                                     output.back()->additional_display_items,
                                     execution_context, exception_state);
      if (exception_state.HadException())
        return;
    }

    Vector<String> supported_methods;
    if (modifier.supportedMethods().IsString()) {
      supported_methods.push_back(modifier.supportedMethods().GetAsString());
    }
    if (modifier.supportedMethods().IsStringSequence()) {
      supported_methods = modifier.supportedMethods().GetAsStringSequence();
      if (supported_methods.size() > 1) {
        Deprecation::CountDeprecation(
            &execution_context,
            WebFeature::kPaymentRequestSupportedMethodsArray);
      }
    }
    if (supported_methods.IsEmpty()) {
      exception_state.ThrowTypeError(
          "Must specify at least one payment method identifier");
      return;
    }

    if (supported_methods.size() > PaymentRequest::kMaxListSize) {
      exception_state.ThrowTypeError(
          "At most 1024 supportedMethods allowed for modifier");
      return;
    }

    for (const String& method : supported_methods) {
      if (method.length() > PaymentRequest::kMaxStringLength) {
        exception_state.ThrowTypeError(
            "Supported method name for identifier cannot be longer than 1024 "
            "characters");
        return;
      }
      if (!IsValidMethodFormat(method)) {
        exception_state.ThrowRangeError(
            "Invalid payment method identifier format");
        return;
      }
    }
    CountPaymentRequestNetworkNameInSupportedMethods(supported_methods,
                                                     execution_context);

    output.back()->method_data =
        payments::mojom::blink::PaymentMethodData::New();
    output.back()->method_data->supported_methods = supported_methods;

    if (modifier.hasData() && !modifier.data().IsEmpty()) {
      StringifyAndParseMethodSpecificData(supported_methods, modifier.data(),
                                          output.back()->method_data,
                                          exception_state);
    } else {
      output.back()->method_data->stringified_data = "";
    }
  }
}

void ValidateAndConvertPaymentDetailsBase(const PaymentDetailsBase& input,
                                          const PaymentOptions& options,
                                          PaymentDetailsPtr& output,
                                          String& shipping_option_output,
                                          ExecutionContext& execution_context,
                                          ExceptionState& exception_state) {
  if (input.hasDisplayItems()) {
    ValidateAndConvertDisplayItems(input.displayItems(), "display items",
                                   output->display_items, execution_context,
                                   exception_state);
    if (exception_state.HadException())
      return;
  }

  // If requestShipping is specified and there are shipping options to validate,
  // proceed with validation.
  if (options.requestShipping() && input.hasShippingOptions()) {
    ValidateAndConvertShippingOptions(
        input.shippingOptions(), output->shipping_options,
        shipping_option_output, execution_context, exception_state);
    if (exception_state.HadException())
      return;
  } else {
    shipping_option_output = String();
  }

  if (input.hasModifiers()) {
    ValidateAndConvertPaymentDetailsModifiers(
        input.modifiers(), output->modifiers, execution_context,
        exception_state);
  }
}

void ValidateAndConvertPaymentDetailsInit(const PaymentDetailsInit& input,
                                          const PaymentOptions& options,
                                          PaymentDetailsPtr& output,
                                          String& shipping_option_output,
                                          ExecutionContext& execution_context,
                                          ExceptionState& exception_state) {
  DCHECK(input.hasTotal());
  ValidateAndConvertTotal(input.total(), "total", output->total,
                          execution_context, exception_state);
  if (exception_state.HadException())
    return;

  ValidateAndConvertPaymentDetailsBase(input, options, output,
                                       shipping_option_output,
                                       execution_context, exception_state);
}

void ValidateAndConvertPaymentDetailsUpdate(const PaymentDetailsUpdate& input,
                                            const PaymentOptions& options,
                                            PaymentDetailsPtr& output,
                                            String& shipping_option_output,
                                            ExecutionContext& execution_context,
                                            ExceptionState& exception_state) {
  ValidateAndConvertPaymentDetailsBase(input, options, output,
                                       shipping_option_output,
                                       execution_context, exception_state);
  if (exception_state.HadException())
    return;

  if (input.hasTotal()) {
    ValidateAndConvertTotal(input.total(), "total", output->total,
                            execution_context, exception_state);
    if (exception_state.HadException())
      return;
  }

  if (input.hasError() && !input.error().IsNull()) {
    String error_message;
    if (!PaymentsValidators::IsValidErrorMsgFormat(input.error(),
                                                   &error_message)) {
      exception_state.ThrowTypeError(error_message);
      return;
    }
    output->error = input.error();
  } else {
    output->error = "";
  }
}

void ValidateAndConvertPaymentMethodData(
    const HeapVector<PaymentMethodData>& input,
    Vector<payments::mojom::blink::PaymentMethodDataPtr>& output,
    HashSet<String>& method_names,
    ExecutionContext& execution_context,
    ExceptionState& exception_state) {
  if (input.IsEmpty()) {
    exception_state.ThrowTypeError("At least one payment method is required");
    return;
  }

  if (input.size() > PaymentRequest::kMaxListSize) {
    exception_state.ThrowTypeError(
        "At most 1024 payment methods are supported");
    return;
  }

  for (const PaymentMethodData payment_method_data : input) {
    Vector<String> supported_methods;
    if (payment_method_data.supportedMethods().IsString()) {
      supported_methods.push_back(
          payment_method_data.supportedMethods().GetAsString());
    }
    if (payment_method_data.supportedMethods().IsStringSequence()) {
      supported_methods =
          payment_method_data.supportedMethods().GetAsStringSequence();
      if (supported_methods.size() > 1) {
        Deprecation::CountDeprecation(
            &execution_context,
            WebFeature::kPaymentRequestSupportedMethodsArray);
      }
    }
    if (supported_methods.IsEmpty()) {
      exception_state.ThrowTypeError(
          "Each payment method needs to include at least one payment method "
          "identifier");
      return;
    }

    if (supported_methods.size() > PaymentRequest::kMaxListSize) {
      exception_state.ThrowTypeError(
          "At most 1024 payment method identifiers are supported");
      return;
    }

    for (const String identifier : supported_methods) {
      if (identifier.length() > PaymentRequest::kMaxStringLength) {
        exception_state.ThrowTypeError(
            "A payment method identifier cannot be longer than 1024 "
            "characters");
        return;
      }
      if (!IsValidMethodFormat(identifier)) {
        exception_state.ThrowRangeError(
            "Invalid payment method identifier format");
        return;
      }
      method_names.insert(identifier);
    }

    CountPaymentRequestNetworkNameInSupportedMethods(supported_methods,
                                                     execution_context);

    output.push_back(payments::mojom::blink::PaymentMethodData::New());
    output.back()->supported_methods = supported_methods;

    if (payment_method_data.hasData() &&
        !payment_method_data.data().IsEmpty()) {
      StringifyAndParseMethodSpecificData(supported_methods,
                                          payment_method_data.data(),
                                          output.back(), exception_state);
    } else {
      output.back()->stringified_data = "";
    }
  }
}

bool AllowedToUsePaymentRequest(const Frame* frame) {
  // To determine whether a Document object |document| is allowed to use the
  // feature indicated by attribute name |allowpaymentrequest|, run these steps:

  // 1. If |document| has no browsing context, then return false.
  if (!frame)
    return false;

  if (!IsSupportedInFeaturePolicy(mojom::FeaturePolicyFeature::kPayment)) {
    // 2. If |document|'s browsing context is a top-level browsing context, then
    // return true.
    if (frame->IsMainFrame())
      return true;

    // 3. If |document|'s browsing context has a browsing context container that
    // is an iframe element with an |allowpaymentrequest| attribute specified,
    // and whose node document is allowed to use the feature indicated by
    // |allowpaymentrequest|, then return true.
    if (frame->Owner() && frame->Owner()->AllowPaymentRequest())
      return AllowedToUsePaymentRequest(frame->Tree().Parent());

    // 4. Return false.
    return false;
  }

  // 2. If Feature Policy is enabled, return the policy for "payment" feature.
  return frame->IsFeatureEnabled(mojom::FeaturePolicyFeature::kPayment);
}

void WarnIgnoringQueryQuotaForCanMakePayment(
    ExecutionContext& execution_context) {
  execution_context.AddConsoleMessage(ConsoleMessage::Create(
      kJSMessageSource, kWarningMessageLevel,
      "Quota reached for PaymentRequest.canMakePayment(). This would normally "
      "reject the promise, but allowing continued usage on localhost and "
      "file:// scheme origins."));
}

}  // namespace

PaymentRequest* PaymentRequest::Create(
    ExecutionContext* execution_context,
    const HeapVector<PaymentMethodData>& method_data,
    const PaymentDetailsInit& details,
    ExceptionState& exception_state) {
  return new PaymentRequest(execution_context, method_data, details,
                            PaymentOptions(), exception_state);
}

PaymentRequest* PaymentRequest::Create(
    ExecutionContext* execution_context,
    const HeapVector<PaymentMethodData>& method_data,
    const PaymentDetailsInit& details,
    const PaymentOptions& options,
    ExceptionState& exception_state) {
  return new PaymentRequest(execution_context, method_data, details, options,
                            exception_state);
}

PaymentRequest::~PaymentRequest() = default;

ScriptPromise PaymentRequest::show(ScriptState* script_state) {
  if (!payment_provider_.is_bound() || show_resolver_) {
    return ScriptPromise::RejectWithDOMException(
        script_state,
        DOMException::Create(kInvalidStateError, "Already called show() once"));
  }

  if (!script_state->ContextIsValid() || !LocalDOMWindow::From(script_state) ||
      !LocalDOMWindow::From(script_state)->GetFrame()) {
    return ScriptPromise::RejectWithDOMException(
        script_state, DOMException::Create(kInvalidStateError,
                                           "Cannot show the payment request"));
  }

  // TODO(crbug.com/825270): Reject with SecurityError DOMException if triggered
  // without user activation.
  bool is_user_gesture = Frame::HasTransientUserActivation(GetFrame());
  if (!is_user_gesture) {
    UseCounter::Count(GetExecutionContext(),
                      WebFeature::kPaymentRequestShowWithoutGesture);
  }

  // TODO(crbug.com/779126): add support for handling payment requests in
  // immersive mode.
  if (GetFrame()->GetDocument()->GetSettings()->GetImmersiveModeEnabled()) {
    return ScriptPromise::RejectWithDOMException(
        script_state,
        DOMException::Create(kInvalidStateError, "Page popups are suppressed"));
  }

  payment_provider_->Show(is_user_gesture);

  show_resolver_ = ScriptPromiseResolver::Create(script_state);
  return show_resolver_->Promise();
}

ScriptPromise PaymentRequest::abort(ScriptState* script_state) {
  if (!script_state->ContextIsValid()) {
    return ScriptPromise::RejectWithDOMException(
        script_state,
        DOMException::Create(kInvalidStateError, "Cannot abort payment"));
  }

  if (abort_resolver_) {
    return ScriptPromise::RejectWithDOMException(
        script_state,
        DOMException::Create(kInvalidStateError,
                             "Cannot abort() again until the previous abort() "
                             "has resolved or rejected"));
  }

  if (!show_resolver_) {
    return ScriptPromise::RejectWithDOMException(
        script_state,
        DOMException::Create(kInvalidStateError,
                             "Never called show(), so nothing to abort"));
  }

  abort_resolver_ = ScriptPromiseResolver::Create(script_state);
  payment_provider_->Abort();
  return abort_resolver_->Promise();
}

ScriptPromise PaymentRequest::canMakePayment(ScriptState* script_state) {
  if (!payment_provider_.is_bound() || show_resolver_ ||
      can_make_payment_resolver_ || !script_state->ContextIsValid()) {
    return ScriptPromise::RejectWithDOMException(
        script_state, DOMException::Create(kInvalidStateError,
                                           "Cannot query payment request"));
  }

  payment_provider_->CanMakePayment();

  can_make_payment_resolver_ = ScriptPromiseResolver::Create(script_state);
  return can_make_payment_resolver_->Promise();
}

bool PaymentRequest::HasPendingActivity() const {
  return show_resolver_ || complete_resolver_;
}

const AtomicString& PaymentRequest::InterfaceName() const {
  return EventTargetNames::PaymentRequest;
}

ExecutionContext* PaymentRequest::GetExecutionContext() const {
  return ContextLifecycleObserver::GetExecutionContext();
}

ScriptPromise PaymentRequest::Complete(ScriptState* script_state,
                                       PaymentComplete result) {
  if (!script_state->ContextIsValid()) {
    return ScriptPromise::RejectWithDOMException(
        script_state,
        DOMException::Create(kInvalidStateError, "Cannot complete payment"));
  }

  if (complete_resolver_) {
    return ScriptPromise::RejectWithDOMException(
        script_state, DOMException::Create(kInvalidStateError,
                                           "Already called complete() once"));
  }

  if (!complete_timer_.IsActive()) {
    return ScriptPromise::RejectWithDOMException(
        script_state,
        DOMException::Create(
            kInvalidStateError,
            "Timed out after 60 seconds, complete() called too late"));
  }

  // User has cancelled the transaction while the website was processing it.
  if (!payment_provider_) {
    return ScriptPromise::RejectWithDOMException(
        script_state, DOMException::Create(kAbortError, "Request cancelled"));
  }

  complete_timer_.Stop();

  // The payment provider should respond in PaymentRequest::OnComplete().
  payment_provider_->Complete(payments::mojom::blink::PaymentComplete(result));

  complete_resolver_ = ScriptPromiseResolver::Create(script_state);
  return complete_resolver_->Promise();
}

void PaymentRequest::OnUpdatePaymentDetails(
    const ScriptValue& details_script_value) {
  if (!show_resolver_ || !payment_provider_)
    return;

  PaymentDetailsUpdate details;
  ExceptionState exception_state(v8::Isolate::GetCurrent(),
                                 ExceptionState::kConstructionContext,
                                 "PaymentDetailsUpdate");
  V8PaymentDetailsUpdate::ToImpl(details_script_value.GetIsolate(),
                                 details_script_value.V8Value(), details,
                                 exception_state);
  if (exception_state.HadException()) {
    show_resolver_->Reject(exception_state.GetException());
    ClearResolversAndCloseMojoConnection();
    return;
  }

  if (!details.hasTotal()) {
    show_resolver_->Reject(
        DOMException::Create(kSyntaxError, "Total required"));
    ClearResolversAndCloseMojoConnection();
    return;
  }

  PaymentDetailsPtr validated_details =
      payments::mojom::blink::PaymentDetails::New();
  ValidateAndConvertPaymentDetailsUpdate(
      details, options_, validated_details, shipping_option_,
      *GetExecutionContext(), exception_state);
  if (exception_state.HadException()) {
    show_resolver_->Reject(exception_state.GetException());
    ClearResolversAndCloseMojoConnection();
    return;
  }

  if (!options_.requestShipping())
    validated_details->shipping_options.clear();

  payment_provider_->UpdateWith(std::move(validated_details));
}

void PaymentRequest::OnUpdatePaymentDetailsFailure(const String& error) {
  if (show_resolver_)
    show_resolver_->Reject(DOMException::Create(kAbortError, error));
  if (complete_resolver_)
    complete_resolver_->Reject(DOMException::Create(kAbortError, error));
  ClearResolversAndCloseMojoConnection();
}

void PaymentRequest::Trace(blink::Visitor* visitor) {
  visitor->Trace(options_);
  visitor->Trace(shipping_address_);
  visitor->Trace(show_resolver_);
  visitor->Trace(complete_resolver_);
  visitor->Trace(abort_resolver_);
  visitor->Trace(can_make_payment_resolver_);
  EventTargetWithInlineData::Trace(visitor);
  ContextLifecycleObserver::Trace(visitor);
}

void PaymentRequest::OnCompleteTimeoutForTesting() {
  complete_timer_.Stop();
  OnCompleteTimeout(nullptr);
}

PaymentRequest::PaymentRequest(ExecutionContext* execution_context,
                               const HeapVector<PaymentMethodData>& method_data,
                               const PaymentDetailsInit& details,
                               const PaymentOptions& options,
                               ExceptionState& exception_state)
    : ContextLifecycleObserver(execution_context),
      options_(options),
      client_binding_(this),
      complete_timer_(
          execution_context->GetTaskRunner(TaskType::kMiscPlatformAPI),
          this,
          &PaymentRequest::OnCompleteTimeout) {
  DCHECK(GetExecutionContext()->IsSecureContext());

  if (!AllowedToUsePaymentRequest(GetFrame())) {
    exception_state.ThrowSecurityError(
        "Must be in a top-level browsing context or an iframe needs to specify "
        "'allowpaymentrequest' explicitly");
    return;
  }

  if (details.hasId() &&
      details.id().length() > PaymentRequest::kMaxStringLength) {
    exception_state.ThrowTypeError("ID cannot be longer than 1024 characters");
    return;
  }

  PaymentDetailsPtr validated_details =
      payments::mojom::blink::PaymentDetails::New();
  validated_details->id = id_ =
      details.hasId() ? details.id() : CreateCanonicalUUIDString();

  Vector<payments::mojom::blink::PaymentMethodDataPtr> validated_method_data;
  ValidateAndConvertPaymentMethodData(method_data, validated_method_data,
                                      method_names_, *GetExecutionContext(),
                                      exception_state);
  if (exception_state.HadException())
    return;

  ValidateAndConvertPaymentDetailsInit(details, options_, validated_details,
                                       shipping_option_, *GetExecutionContext(),
                                       exception_state);
  if (exception_state.HadException())
    return;

  if (options_.requestShipping())
    shipping_type_ = options_.shippingType();
  else
    validated_details->shipping_options.clear();

  DCHECK(shipping_type_.IsNull() || shipping_type_ == "shipping" ||
         shipping_type_ == "delivery" || shipping_type_ == "pickup");

  GetFrame()->GetInterfaceProvider().GetInterface(
      mojo::MakeRequest(&payment_provider_));
  payment_provider_.set_connection_error_handler(
      WTF::Bind(&PaymentRequest::OnError, WrapWeakPersistent(this),
                PaymentErrorReason::UNKNOWN));

  payments::mojom::blink::PaymentRequestClientPtr client;
  client_binding_.Bind(mojo::MakeRequest(&client));
  payment_provider_->Init(
      std::move(client), std::move(validated_method_data),
      std::move(validated_details),
      payments::mojom::blink::PaymentOptions::From(options_));
}

void PaymentRequest::ContextDestroyed(ExecutionContext*) {
  ClearResolversAndCloseMojoConnection();
}

void PaymentRequest::OnShippingAddressChange(PaymentAddressPtr address) {
  DCHECK(show_resolver_);
  DCHECK(!complete_resolver_);

  String error_message;
  if (!PaymentsValidators::IsValidShippingAddress(address, &error_message)) {
    show_resolver_->Reject(DOMException::Create(kSyntaxError, error_message));
    ClearResolversAndCloseMojoConnection();
    return;
  }

  shipping_address_ = new PaymentAddress(std::move(address));

  PaymentRequestUpdateEvent* event = PaymentRequestUpdateEvent::Create(
      GetExecutionContext(), EventTypeNames::shippingaddresschange);
  event->SetTarget(this);
  event->SetPaymentDetailsUpdater(this);
  DispatchEvent(event);
  if (!event->is_waiting_for_update()) {
    GetExecutionContext()->AddConsoleMessage(ConsoleMessage::Create(
        kJSMessageSource, kWarningMessageLevel,
        "No updateWith() call in 'shippingaddresschange' event handler. User "
        "may see outdated line items and total."));
    payment_provider_->NoUpdatedPaymentDetails();
  }
}

void PaymentRequest::OnShippingOptionChange(const String& shipping_option_id) {
  DCHECK(show_resolver_);
  DCHECK(!complete_resolver_);
  shipping_option_ = shipping_option_id;

  PaymentRequestUpdateEvent* event = PaymentRequestUpdateEvent::Create(
      GetExecutionContext(), EventTypeNames::shippingoptionchange);
  event->SetTarget(this);
  event->SetPaymentDetailsUpdater(this);
  DispatchEvent(event);
  if (!event->is_waiting_for_update()) {
    GetExecutionContext()->AddConsoleMessage(ConsoleMessage::Create(
        kJSMessageSource, kWarningMessageLevel,
        "No updateWith() call in 'shippingoptionchange' event handler. User "
        "may see outdated line items and total."));
    payment_provider_->NoUpdatedPaymentDetails();
  }
}

void PaymentRequest::OnPaymentResponse(PaymentResponsePtr response) {
  DCHECK(show_resolver_);
  DCHECK(!complete_resolver_);
  DCHECK(!complete_timer_.IsActive());

  if (options_.requestShipping()) {
    if (!response->shipping_address || response->shipping_option.IsEmpty()) {
      show_resolver_->Reject(DOMException::Create(kSyntaxError));
      ClearResolversAndCloseMojoConnection();
      return;
    }

    String error_message;
    if (!PaymentsValidators::IsValidShippingAddress(response->shipping_address,
                                                    &error_message)) {
      show_resolver_->Reject(DOMException::Create(kSyntaxError, error_message));
      ClearResolversAndCloseMojoConnection();
      return;
    }

    shipping_address_ =
        new PaymentAddress(std::move(response->shipping_address));
    shipping_option_ = response->shipping_option;
  } else {
    if (response->shipping_address || !response->shipping_option.IsNull()) {
      show_resolver_->Reject(DOMException::Create(kSyntaxError));
      ClearResolversAndCloseMojoConnection();
      return;
    }
  }

  if ((options_.requestPayerName() && response->payer_name.IsEmpty()) ||
      (options_.requestPayerEmail() && response->payer_email.IsEmpty()) ||
      (options_.requestPayerPhone() && response->payer_phone.IsEmpty()) ||
      (!options_.requestPayerName() && !response->payer_name.IsNull()) ||
      (!options_.requestPayerEmail() && !response->payer_email.IsNull()) ||
      (!options_.requestPayerPhone() && !response->payer_phone.IsNull())) {
    show_resolver_->Reject(DOMException::Create(kSyntaxError));
    ClearResolversAndCloseMojoConnection();
    return;
  }

  complete_timer_.StartOneShot(kCompleteTimeoutSeconds, FROM_HERE);

  show_resolver_->Resolve(new PaymentResponse(
      std::move(response), shipping_address_.Get(), this, id_));

  // Do not close the mojo connection here. The merchant website should call
  // PaymentResponse::complete(String), which will be forwarded over the mojo
  // connection to display a success or failure message to the user.
  show_resolver_.Clear();
}

void PaymentRequest::OnError(PaymentErrorReason error) {
  ExceptionCode ec = kUnknownError;
  String message;

  switch (error) {
    case PaymentErrorReason::USER_CANCEL: {
      ec = kAbortError;
      message = "Request cancelled";
      break;
    }

    case PaymentErrorReason::NOT_SUPPORTED: {
      ec = kNotSupportedError;
      DCHECK_LE(1U, method_names_.size());
      auto it = method_names_.begin();
      if (method_names_.size() == 1U) {
        message = "The payment method \"" + *it + "\" is not supported";
      } else {
        StringBuilder sb;
        sb.Append("The payment methods \"");
        sb.Append(*it);
        sb.Append("\"");
        while (++it != method_names_.end()) {
          sb.Append(", \"");
          sb.Append(*it);
          sb.Append("\"");
        }
        sb.Append(" are not supported");
        message = sb.ToString();
      }
      break;
    }

    case PaymentErrorReason::UNKNOWN: {
      ec = kUnknownError;
      message = "Request failed";
      break;
    }
  }

  DCHECK(!message.IsEmpty());

  // If the user closes PaymentRequest UI after PaymentResponse.complete() has
  // been called, the PaymentResponse.complete() promise should be resolved with
  // undefined instead of rejecting.
  if (complete_resolver_) {
    DCHECK(error == PaymentErrorReason::USER_CANCEL ||
           error == PaymentErrorReason::UNKNOWN);
    complete_resolver_->Resolve();
  }

  if (show_resolver_)
    show_resolver_->Reject(DOMException::Create(ec, message));

  if (abort_resolver_)
    abort_resolver_->Reject(DOMException::Create(ec, message));

  if (can_make_payment_resolver_)
    can_make_payment_resolver_->Reject(DOMException::Create(ec, message));

  ClearResolversAndCloseMojoConnection();
}

void PaymentRequest::OnComplete() {
  DCHECK(complete_resolver_);
  complete_resolver_->Resolve();
  ClearResolversAndCloseMojoConnection();
}

void PaymentRequest::OnAbort(bool aborted_successfully) {
  DCHECK(abort_resolver_);
  DCHECK(show_resolver_);

  if (!aborted_successfully) {
    abort_resolver_->Reject(DOMException::Create(
        kInvalidStateError, "Unable to abort the payment"));
    abort_resolver_.Clear();
    return;
  }

  show_resolver_->Reject(
      DOMException::Create(kAbortError, "The website has aborted the payment"));
  abort_resolver_->Resolve();
  ClearResolversAndCloseMojoConnection();
}

void PaymentRequest::OnCanMakePayment(CanMakePaymentQueryResult result) {
  DCHECK(can_make_payment_resolver_);

  switch (result) {
    case CanMakePaymentQueryResult::WARNING_CAN_MAKE_PAYMENT:
      WarnIgnoringQueryQuotaForCanMakePayment(*GetExecutionContext());
      FALLTHROUGH;
    case CanMakePaymentQueryResult::CAN_MAKE_PAYMENT:
      can_make_payment_resolver_->Resolve(true);
      break;
    case CanMakePaymentQueryResult::WARNING_CANNOT_MAKE_PAYMENT:
      WarnIgnoringQueryQuotaForCanMakePayment(*GetExecutionContext());
      FALLTHROUGH;
    case CanMakePaymentQueryResult::CANNOT_MAKE_PAYMENT:
      can_make_payment_resolver_->Resolve(false);
      break;
    case CanMakePaymentQueryResult::QUERY_QUOTA_EXCEEDED:
      can_make_payment_resolver_->Reject(DOMException::Create(
          kNotAllowedError, "Not allowed to check whether can make payment"));
      break;
  }

  can_make_payment_resolver_.Clear();
}

void PaymentRequest::WarnNoFavicon() {
  GetExecutionContext()->AddConsoleMessage(
      ConsoleMessage::Create(kJSMessageSource, kWarningMessageLevel,
                             "Favicon not found for PaymentRequest UI. User "
                             "may not recognize the website."));
}

void PaymentRequest::OnCompleteTimeout(TimerBase*) {
  GetExecutionContext()->AddConsoleMessage(ConsoleMessage::Create(
      kJSMessageSource, kErrorMessageLevel,
      "Timed out waiting for a PaymentResponse.complete() call."));
  payment_provider_->Complete(payments::mojom::blink::PaymentComplete(kFail));
  ClearResolversAndCloseMojoConnection();
}

void PaymentRequest::ClearResolversAndCloseMojoConnection() {
  complete_timer_.Stop();
  complete_resolver_.Clear();
  show_resolver_.Clear();
  abort_resolver_.Clear();
  can_make_payment_resolver_.Clear();
  if (client_binding_.is_bound())
    client_binding_.Close();
  payment_provider_.reset();
}

}  // namespace blink
