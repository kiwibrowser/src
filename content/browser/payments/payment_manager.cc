// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/payments/payment_manager.h"

#include <utility>

#include "base/bind.h"
#include "base/optional.h"
#include "content/browser/payments/payment_app.pb.h"
#include "content/browser/payments/payment_app_context_impl.h"
#include "content/browser/payments/payment_app_database.h"
#include "content/browser/service_worker/service_worker_context_wrapper.h"
#include "content/browser/service_worker/service_worker_registration.h"
#include "content/public/browser/browser_thread.h"
#include "url/origin.h"

namespace content {

PaymentManager::~PaymentManager() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
}

PaymentManager::PaymentManager(
    PaymentAppContextImpl* payment_app_context,
    mojo::InterfaceRequest<payments::mojom::PaymentManager> request)
    : payment_app_context_(payment_app_context),
      binding_(this, std::move(request)),
      weak_ptr_factory_(this) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(payment_app_context);

  binding_.set_connection_error_handler(base::BindOnce(
      &PaymentManager::OnConnectionError, base::Unretained(this)));
}

void PaymentManager::Init(const GURL& context_url, const std::string& scope) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  should_set_payment_app_info_ = true;
  context_url_ = context_url;
  scope_ = GURL(scope);

  if (!context_url_.is_valid()) {
    binding_.CloseWithReason(0U, "Invalid context URL.");
    return;
  }
  if (!scope_.is_valid()) {
    binding_.CloseWithReason(1U, "Invalid scope URL.");
    return;
  }
  if (!url::IsSameOriginWith(context_url_, scope_)) {
    binding_.CloseWithReason(
        2U, "Scope URL is not from the same origin of the context URL.");
    return;
  }
}

void PaymentManager::DeletePaymentInstrument(
    const std::string& instrument_key,
    PaymentManager::DeletePaymentInstrumentCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  payment_app_context_->payment_app_database()->DeletePaymentInstrument(
      scope_, instrument_key, std::move(callback));
}

void PaymentManager::GetPaymentInstrument(
    const std::string& instrument_key,
    PaymentManager::GetPaymentInstrumentCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  payment_app_context_->payment_app_database()->ReadPaymentInstrument(
      scope_, instrument_key, std::move(callback));
}

void PaymentManager::KeysOfPaymentInstruments(
    PaymentManager::KeysOfPaymentInstrumentsCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  payment_app_context_->payment_app_database()->KeysOfPaymentInstruments(
      scope_, std::move(callback));
}

void PaymentManager::HasPaymentInstrument(
    const std::string& instrument_key,
    PaymentManager::HasPaymentInstrumentCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  payment_app_context_->payment_app_database()->HasPaymentInstrument(
      scope_, instrument_key, std::move(callback));
}

void PaymentManager::SetPaymentInstrument(
    const std::string& instrument_key,
    payments::mojom::PaymentInstrumentPtr details,
    PaymentManager::SetPaymentInstrumentCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (should_set_payment_app_info_) {
    payment_app_context_->payment_app_database()->WritePaymentInstrument(
        scope_, instrument_key, std::move(details),
        base::BindOnce(
            &PaymentManager::SetPaymentInstrumentIntermediateCallback,
            weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  } else {
    payment_app_context_->payment_app_database()->WritePaymentInstrument(
        scope_, instrument_key, std::move(details), std::move(callback));
  }
}

void PaymentManager::SetPaymentInstrumentIntermediateCallback(
    PaymentManager::SetPaymentInstrumentCallback callback,
    payments::mojom::PaymentHandlerStatus status) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (status != payments::mojom::PaymentHandlerStatus::SUCCESS ||
      !should_set_payment_app_info_) {
    std::move(callback).Run(status);
    return;
  }

  payment_app_context_->payment_app_database()->FetchAndUpdatePaymentAppInfo(
      context_url_, scope_, std::move(callback));
  should_set_payment_app_info_ = false;
}

void PaymentManager::ClearPaymentInstruments(
    ClearPaymentInstrumentsCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  payment_app_context_->payment_app_database()->ClearPaymentInstruments(
      scope_, std::move(callback));
}

void PaymentManager::SetUserHint(const std::string& user_hint) {
  payment_app_context_->payment_app_database()->SetPaymentAppUserHint(
      scope_, user_hint);
}

void PaymentManager::OnConnectionError() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  payment_app_context_->PaymentManagerHadConnectionError(this);
}

}  // namespace content
