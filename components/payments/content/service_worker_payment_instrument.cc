// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/content/service_worker_payment_instrument.h"

#include "base/bind_helpers.h"
#include "base/stl_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/payments/content/payment_request_converter.h"
#include "components/payments/core/payment_request_delegate.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/payment_app_provider.h"
#include "content/public/browser/web_contents.h"
#include "ui/gfx/image/image_skia.h"
#include "url/origin.h"

namespace payments {

// Service worker payment app provides icon through bitmap, so set 0 as invalid
// resource Id.
ServiceWorkerPaymentInstrument::ServiceWorkerPaymentInstrument(
    content::BrowserContext* browser_context,
    const GURL& top_origin,
    const GURL& frame_origin,
    const PaymentRequestSpec* spec,
    std::unique_ptr<content::StoredPaymentApp> stored_payment_app_info,
    PaymentRequestDelegate* payment_request_delegate)
    : PaymentInstrument(0, PaymentInstrument::Type::SERVICE_WORKER_APP),
      browser_context_(browser_context),
      top_origin_(top_origin),
      frame_origin_(frame_origin),
      spec_(spec),
      stored_payment_app_info_(std::move(stored_payment_app_info)),
      delegate_(nullptr),
      payment_request_delegate_(payment_request_delegate),
      can_make_payment_result_(false),
      needs_installation_(false),
      weak_ptr_factory_(this) {
  DCHECK(browser_context_);
  DCHECK(top_origin_.is_valid());
  DCHECK(frame_origin_.is_valid());
  DCHECK(spec_);

  if (stored_payment_app_info_->icon) {
    icon_image_ =
        gfx::ImageSkia::CreateFrom1xBitmap(*(stored_payment_app_info_->icon))
            .DeepCopy();
  } else {
    // Create an empty icon image to avoid using invalid icon resource id.
    icon_image_ = gfx::ImageSkia::CreateFrom1xBitmap(SkBitmap()).DeepCopy();
  }
}

// Service worker payment app provides icon through bitmap, so set 0 as invalid
// resource Id.
ServiceWorkerPaymentInstrument::ServiceWorkerPaymentInstrument(
    content::WebContents* web_contents,
    const GURL& top_origin,
    const GURL& frame_origin,
    const PaymentRequestSpec* spec,
    std::unique_ptr<WebAppInstallationInfo> installable_payment_app_info,
    const std::string& enabled_method,
    PaymentRequestDelegate* payment_request_delegate)
    : PaymentInstrument(0, PaymentInstrument::Type::SERVICE_WORKER_APP),
      top_origin_(top_origin),
      frame_origin_(frame_origin),
      spec_(spec),
      delegate_(nullptr),
      payment_request_delegate_(payment_request_delegate),
      can_make_payment_result_(false),
      needs_installation_(true),
      web_contents_(web_contents),
      installable_web_app_info_(std::move(installable_payment_app_info)),
      installable_enabled_method_(enabled_method),
      weak_ptr_factory_(this) {
  DCHECK(web_contents_);
  DCHECK(top_origin_.is_valid());
  DCHECK(frame_origin_.is_valid());
  DCHECK(spec_);

  if (installable_web_app_info_->icon) {
    icon_image_ =
        gfx::ImageSkia::CreateFrom1xBitmap(*(installable_web_app_info_->icon))
            .DeepCopy();
  } else {
    // Create an empty icon image to avoid using invalid icon resource id.
    icon_image_ = gfx::ImageSkia::CreateFrom1xBitmap(SkBitmap()).DeepCopy();
  }
}

ServiceWorkerPaymentInstrument::~ServiceWorkerPaymentInstrument() {
  // TODO(crbug.com/782270): Implement abort InstallAndInvokePaymentApp for
  // payment app that needs installation.
  if (delegate_ && !needs_installation_) {
    // If there's a payment in progress, abort it before destroying this
    // so that it can close its window. Since the PaymentRequest will be
    // destroyed, pass an empty callback to the payment app.
    content::PaymentAppProvider::GetInstance()->AbortPayment(
        browser_context_, stored_payment_app_info_->registration_id,
        base::DoNothing());
  }
}

void ServiceWorkerPaymentInstrument::ValidateCanMakePayment(
    ValidateCanMakePaymentCallback callback) {
  // Returns true for payment app that needs installation.
  if (needs_installation_) {
    OnCanMakePayment(std::move(callback), true);
    return;
  }

  // Returns true if we are in incognito (avoiding sending the event to the
  // payment handler).
  if (payment_request_delegate_->IsIncognito()) {
    OnCanMakePayment(std::move(callback), true);
    return;
  }

  // Do not send CanMakePayment event to payment apps that have not been
  // explicitly verified.
  if (!stored_payment_app_info_->has_explicitly_verified_methods) {
    OnCanMakePayment(std::move(callback), true);
    return;
  }

  mojom::CanMakePaymentEventDataPtr event_data =
      CreateCanMakePaymentEventData();
  if (event_data.is_null()) {
    // This could only happen if this instrument only supports non-url based
    // payment methods of the payment request, then return true
    // and do not send CanMakePaymentEvent to the payment app.
    OnCanMakePayment(std::move(callback), true);
    return;
  }

  content::PaymentAppProvider::GetInstance()->CanMakePayment(
      browser_context_, stored_payment_app_info_->registration_id,
      std::move(event_data),
      base::BindOnce(&ServiceWorkerPaymentInstrument::OnCanMakePayment,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

mojom::CanMakePaymentEventDataPtr
ServiceWorkerPaymentInstrument::CreateCanMakePaymentEventData() {
  std::set<std::string> requested_url_methods;
  for (const auto& method : spec_->payment_method_identifiers_set()) {
    GURL url_method(method);
    if (url_method.is_valid()) {
      requested_url_methods.insert(method);
    }
  }
  std::set<std::string> supported_methods;
  supported_methods.insert(stored_payment_app_info_->enabled_methods.begin(),
                           stored_payment_app_info_->enabled_methods.end());
  std::set<std::string> supported_url_methods =
      base::STLSetIntersection<std::set<std::string>>(requested_url_methods,
                                                      supported_methods);
  // Only fire CanMakePaymentEvent if this instrument supports non-url based
  // payment methods of the payment request.
  if (supported_url_methods.empty())
    return nullptr;

  mojom::CanMakePaymentEventDataPtr event_data =
      mojom::CanMakePaymentEventData::New();

  event_data->top_origin = top_origin_;
  event_data->payment_request_origin = frame_origin_;

  for (const auto& modifier : spec_->details().modifiers) {
    std::vector<std::string>::const_iterator it =
        modifier->method_data->supported_methods.begin();
    for (; it != modifier->method_data->supported_methods.end(); it++) {
      if (supported_url_methods.find(*it) != supported_url_methods.end())
        break;
    }
    if (it == modifier->method_data->supported_methods.end())
      continue;

    event_data->modifiers.emplace_back(modifier.Clone());
  }

  for (const auto& data : spec_->method_data()) {
    std::vector<std::string>::const_iterator it =
        data->supported_methods.begin();
    for (; it != data->supported_methods.end(); it++) {
      if (supported_url_methods.find(*it) != supported_url_methods.end())
        break;
    }
    if (it == data->supported_methods.end())
      continue;

    event_data->method_data.push_back(data.Clone());
  }

  return event_data;
}

void ServiceWorkerPaymentInstrument::OnCanMakePayment(
    ValidateCanMakePaymentCallback callback,
    bool result) {
  can_make_payment_result_ = result;

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), this, result));
}

void ServiceWorkerPaymentInstrument::InvokePaymentApp(Delegate* delegate) {
  delegate_ = delegate;

  if (needs_installation_) {
    content::PaymentAppProvider::GetInstance()->InstallAndInvokePaymentApp(
        web_contents_, CreatePaymentRequestEventData(),
        installable_web_app_info_->name,
        installable_web_app_info_->icon == nullptr
            ? SkBitmap()
            : *(installable_web_app_info_->icon),
        installable_web_app_info_->sw_js_url,
        installable_web_app_info_->sw_scope,
        installable_web_app_info_->sw_use_cache, installable_enabled_method_,
        base::BindOnce(&ServiceWorkerPaymentInstrument::OnPaymentAppInvoked,
                       weak_ptr_factory_.GetWeakPtr()));
  } else {
    content::PaymentAppProvider::GetInstance()->InvokePaymentApp(
        browser_context_, stored_payment_app_info_->registration_id,
        CreatePaymentRequestEventData(),
        base::BindOnce(&ServiceWorkerPaymentInstrument::OnPaymentAppInvoked,
                       weak_ptr_factory_.GetWeakPtr()));
  }

  payment_request_delegate_->ShowProcessingSpinner();
}

mojom::PaymentRequestEventDataPtr
ServiceWorkerPaymentInstrument::CreatePaymentRequestEventData() {
  mojom::PaymentRequestEventDataPtr event_data =
      mojom::PaymentRequestEventData::New();

  event_data->top_origin = top_origin_;
  event_data->payment_request_origin = frame_origin_;

  if (spec_->details().id.has_value())
    event_data->payment_request_id = spec_->details().id.value();

  event_data->total = spec_->details().total->amount.Clone();

  std::unordered_set<std::string> supported_methods;
  if (needs_installation_) {
    supported_methods.insert(installable_enabled_method_);
  } else {
    supported_methods.insert(stored_payment_app_info_->enabled_methods.begin(),
                             stored_payment_app_info_->enabled_methods.end());
  }
  for (const auto& modifier : spec_->details().modifiers) {
    std::vector<std::string>::const_iterator it =
        modifier->method_data->supported_methods.begin();
    for (; it != modifier->method_data->supported_methods.end(); it++) {
      if (supported_methods.find(*it) != supported_methods.end())
        break;
    }
    if (it == modifier->method_data->supported_methods.end())
      continue;

    event_data->modifiers.emplace_back(modifier.Clone());
  }

  for (const auto& data : spec_->method_data()) {
    std::vector<std::string>::const_iterator it =
        data->supported_methods.begin();
    for (; it != data->supported_methods.end(); it++) {
      if (supported_methods.find(*it) != supported_methods.end())
        break;
    }
    if (it == data->supported_methods.end())
      continue;

    event_data->method_data.push_back(data.Clone());
  }

  return event_data;
}

void ServiceWorkerPaymentInstrument::OnPaymentAppInvoked(
    mojom::PaymentHandlerResponsePtr response) {
  DCHECK(delegate_);

  if (delegate_ != nullptr) {
    delegate_->OnInstrumentDetailsReady(response->method_name,
                                        response->stringified_details);
    delegate_ = nullptr;
  }
}

bool ServiceWorkerPaymentInstrument::IsCompleteForPayment() const {
  return true;
}

bool ServiceWorkerPaymentInstrument::IsExactlyMatchingMerchantRequest() const {
  return true;
}

base::string16 ServiceWorkerPaymentInstrument::GetMissingInfoLabel() const {
  NOTREACHED();
  return base::string16();
}

bool ServiceWorkerPaymentInstrument::IsValidForCanMakePayment() const {
  // This instrument should not be used when can_make_payment_result_ is false
  // , so this interface should not be invoked.
  DCHECK(can_make_payment_result_);

  // Returns false for PaymentRequest.CanMakePayment query if the app needs
  // installation.
  if (needs_installation_)
    return false;

  return true;
}

void ServiceWorkerPaymentInstrument::RecordUse() {
  NOTIMPLEMENTED();
}

base::string16 ServiceWorkerPaymentInstrument::GetLabel() const {
  return base::UTF8ToUTF16(needs_installation_
                               ? installable_web_app_info_->name
                               : stored_payment_app_info_->name);
}

base::string16 ServiceWorkerPaymentInstrument::GetSublabel() const {
  if (needs_installation_) {
    DCHECK(GURL(installable_web_app_info_->sw_scope).is_valid());
    return base::UTF8ToUTF16(
        url::Origin::Create(GURL(installable_web_app_info_->sw_scope)).host());
  }
  return base::UTF8ToUTF16(
      url::Origin::Create(stored_payment_app_info_->scope).host());
}

bool ServiceWorkerPaymentInstrument::IsValidForModifier(
    const std::vector<std::string>& methods,
    bool supported_networks_specified,
    const std::set<std::string>& supported_networks,
    bool supported_types_specified,
    const std::set<autofill::CreditCard::CardType>& supported_types) const {
  // Payment app that needs installation only supports url based payment
  // methods.
  if (needs_installation_) {
    return std::find(methods.begin(), methods.end(),
                     installable_enabled_method_) != methods.end();
  }

  std::vector<std::string> matched_methods;
  for (const auto& modifier_supported_method : methods) {
    if (base::ContainsValue(stored_payment_app_info_->enabled_methods,
                            modifier_supported_method)) {
      matched_methods.emplace_back(modifier_supported_method);
    }
  }

  if (matched_methods.empty())
    return false;

  // Return true if 'basic-card' is not the only matched payment method. This
  // assumes that there is no duplicated payment methods.
  if (matched_methods.size() > 1U || matched_methods[0] != "basic-card")
    return true;

  // Checking the capabilities of this instrument against the modifier.
  // Return true if both card networks and types are not specified in the
  // modifier.
  if (!supported_networks_specified && !supported_types_specified)
    return true;

  // Return false if no capabilities for this instrument.
  if (stored_payment_app_info_->capabilities.empty())
    return false;

  uint32_t i = 0;
  for (; i < stored_payment_app_info_->capabilities.size(); i++) {
    if (supported_networks_specified) {
      std::set<std::string> app_supported_networks;
      for (const auto& network :
           stored_payment_app_info_->capabilities[i].supported_card_networks) {
        app_supported_networks.insert(GetBasicCardNetworkName(
            static_cast<mojom::BasicCardNetwork>(network)));
      }

      if (base::STLSetIntersection<std::set<std::string>>(
              app_supported_networks, supported_networks)
              .empty()) {
        continue;
      }
    }

    if (supported_types_specified) {
      std::set<autofill::CreditCard::CardType> app_supported_types;
      for (const auto& type :
           stored_payment_app_info_->capabilities[i].supported_card_types) {
        app_supported_types.insert(
            GetBasicCardType(static_cast<mojom::BasicCardType>(type)));
      }

      if (base::STLSetIntersection<std::set<autofill::CreditCard::CardType>>(
              app_supported_types, supported_types)
              .empty()) {
        continue;
      }
    }

    break;
  }

  // i >= stored_payment_app_info_->capabilities.size() indicates no matched
  // capabilities.
  return i < stored_payment_app_info_->capabilities.size();
}

const gfx::ImageSkia* ServiceWorkerPaymentInstrument::icon_image_skia() const {
  return icon_image_.get();
}

}  // namespace payments
