// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PAYMENTS_CORE_TEST_PAYMENT_REQUEST_DELEGATE_H_
#define COMPONENTS_PAYMENTS_CORE_TEST_PAYMENT_REQUEST_DELEGATE_H_

#include <string>

#include "base/message_loop/message_loop.h"
#include "base/single_thread_task_runner.h"
#include "components/autofill/core/browser/payments/full_card_request.h"
#include "components/autofill/core/browser/payments/payments_client.h"
#include "components/autofill/core/browser/test_address_normalizer.h"
#include "components/autofill/core/browser/test_autofill_client.h"
#include "components/payments/core/payment_request_delegate.h"
#include "net/url_request/url_request_test_util.h"

namespace payments {

class TestPaymentsClientDelegate
    : public autofill::payments::PaymentsClientUnmaskDelegate {
 public:
  TestPaymentsClientDelegate();
  ~TestPaymentsClientDelegate();

 private:
  // autofill::payments::PaymentsClientUnmaskDelegate:
  void OnDidGetRealPan(autofill::AutofillClient::PaymentsRpcResult result,
                       const std::string& real_pan) override;
};

class TestURLRequestContextGetter : public net::URLRequestContextGetter {
 public:
  explicit TestURLRequestContextGetter(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner);
  net::URLRequestContext* GetURLRequestContext() override;
  scoped_refptr<base::SingleThreadTaskRunner> GetNetworkTaskRunner()
      const override;

 private:
  ~TestURLRequestContextGetter() override;

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
};

class TestPaymentRequestDelegate : public PaymentRequestDelegate {
 public:
  explicit TestPaymentRequestDelegate(
      autofill::PersonalDataManager* personal_data_manager);
  ~TestPaymentRequestDelegate() override;

  // PaymentRequestDelegate
  void ShowDialog(PaymentRequest* request) override {}
  void CloseDialog() override {}
  void ShowErrorMessage() override {}
  void ShowProcessingSpinner() override {}
  autofill::PersonalDataManager* GetPersonalDataManager() override;
  const std::string& GetApplicationLocale() const override;
  bool IsIncognito() const override;
  bool IsSslCertificateValid() override;
  const GURL& GetLastCommittedURL() const override;
  void DoFullCardRequest(
      const autofill::CreditCard& credit_card,
      base::WeakPtr<autofill::payments::FullCardRequest::ResultDelegate>
          result_delegate) override;
  autofill::AddressNormalizer* GetAddressNormalizer() override;
  autofill::RegionDataLoader* GetRegionDataLoader() override;
  ukm::UkmRecorder* GetUkmRecorder() override;
  std::string GetAuthenticatedEmail() const override;
  PrefService* GetPrefService() override;
  bool IsBrowserWindowActive() const override;

  autofill::TestAddressNormalizer* test_address_normalizer();
  void DelayFullCardRequestCompletion();
  void CompleteFullCardRequest();

 private:
  base::MessageLoop loop_;
  TestPaymentsClientDelegate payments_client_delegate_;
  autofill::PersonalDataManager* personal_data_manager_;
  std::string locale_;
  const GURL last_committed_url_;
  autofill::TestAddressNormalizer address_normalizer_;
  scoped_refptr<TestURLRequestContextGetter> request_context_;
  autofill::TestAutofillClient autofill_client_;
  autofill::payments::PaymentsClient payments_client_;
  autofill::payments::FullCardRequest full_card_request_;

  bool instantaneous_full_card_request_result_ = true;
  autofill::CreditCard full_card_request_card_;
  base::WeakPtr<autofill::payments::FullCardRequest::ResultDelegate>
      full_card_result_delegate_;
  DISALLOW_COPY_AND_ASSIGN(TestPaymentRequestDelegate);
};

}  // namespace payments

#endif  // COMPONENTS_PAYMENTS_CORE_TEST_PAYMENT_REQUEST_DELEGATE_H_
