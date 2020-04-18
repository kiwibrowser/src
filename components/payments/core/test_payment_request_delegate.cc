// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/core/test_payment_request_delegate.h"

#include "base/strings/utf_string_conversions.h"

namespace payments {

TestPaymentRequestDelegate::TestPaymentRequestDelegate(
    autofill::PersonalDataManager* personal_data_manager)
    : personal_data_manager_(personal_data_manager),
      locale_("en-US"),
      last_committed_url_("https://shop.com"),
      request_context_(new TestURLRequestContextGetter(loop_.task_runner())),
      payments_client_(request_context_.get(),
                       nullptr,
                       nullptr,
                       /*unmask_delegate=*/&payments_client_delegate_,
                       /*save_delegate=*/nullptr),
      full_card_request_(&autofill_client_,
                         &payments_client_,
                         personal_data_manager) {}

TestPaymentRequestDelegate::~TestPaymentRequestDelegate() {}

autofill::PersonalDataManager*
TestPaymentRequestDelegate::GetPersonalDataManager() {
  return personal_data_manager_;
}

const std::string& TestPaymentRequestDelegate::GetApplicationLocale() const {
  return locale_;
}

bool TestPaymentRequestDelegate::IsIncognito() const {
  return false;
}

bool TestPaymentRequestDelegate::IsSslCertificateValid() {
  return true;
}

const GURL& TestPaymentRequestDelegate::GetLastCommittedURL() const {
  return last_committed_url_;
}

void TestPaymentRequestDelegate::DoFullCardRequest(
    const autofill::CreditCard& credit_card,
    base::WeakPtr<autofill::payments::FullCardRequest::ResultDelegate>
        result_delegate) {
  if (instantaneous_full_card_request_result_) {
    result_delegate->OnFullCardRequestSucceeded(full_card_request_, credit_card,
                                                base::ASCIIToUTF16("123"));
    return;
  }

  full_card_request_card_ = credit_card;
  full_card_result_delegate_ = result_delegate;
}

autofill::AddressNormalizer*
TestPaymentRequestDelegate::GetAddressNormalizer() {
  return &address_normalizer_;
}

autofill::RegionDataLoader* TestPaymentRequestDelegate::GetRegionDataLoader() {
  return nullptr;
}

ukm::UkmRecorder* TestPaymentRequestDelegate::GetUkmRecorder() {
  return nullptr;
}

autofill::TestAddressNormalizer*
TestPaymentRequestDelegate::test_address_normalizer() {
  return &address_normalizer_;
}

void TestPaymentRequestDelegate::DelayFullCardRequestCompletion() {
  instantaneous_full_card_request_result_ = false;
}

void TestPaymentRequestDelegate::CompleteFullCardRequest() {
  DCHECK(instantaneous_full_card_request_result_ == false);
  full_card_result_delegate_->OnFullCardRequestSucceeded(
      full_card_request_, full_card_request_card_, base::ASCIIToUTF16("123"));
}

std::string TestPaymentRequestDelegate::GetAuthenticatedEmail() const {
  return "";
}

PrefService* TestPaymentRequestDelegate::GetPrefService() {
  return nullptr;
}

bool TestPaymentRequestDelegate::IsBrowserWindowActive() const {
  return true;
}

TestPaymentsClientDelegate::TestPaymentsClientDelegate() {}

TestPaymentsClientDelegate::~TestPaymentsClientDelegate() {}

void TestPaymentsClientDelegate::OnDidGetRealPan(
    autofill::AutofillClient::PaymentsRpcResult result,
    const std::string& real_pan) {}

TestURLRequestContextGetter::TestURLRequestContextGetter(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : task_runner_(task_runner) {}

TestURLRequestContextGetter::~TestURLRequestContextGetter() {}

net::URLRequestContext* TestURLRequestContextGetter::GetURLRequestContext() {
  return nullptr;
}

scoped_refptr<base::SingleThreadTaskRunner>
TestURLRequestContextGetter::GetNetworkTaskRunner() const {
  return task_runner_;
}

}  // namespace payments
