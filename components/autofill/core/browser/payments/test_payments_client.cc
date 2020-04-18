// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/payments/test_payments_client.h"

#include "base/strings/utf_string_conversions.h"

namespace autofill {
namespace payments {

TestPaymentsClient::TestPaymentsClient(
    net::URLRequestContextGetter* context_getter,
    PrefService* pref_service,
    identity::IdentityManager* identity_manager,
    payments::PaymentsClientUnmaskDelegate* unmask_delegate,
    payments::PaymentsClientSaveDelegate* save_delegate)
    : PaymentsClient(context_getter,
                     pref_service,
                     identity_manager,
                     unmask_delegate,
                     save_delegate),
      save_delegate_(save_delegate) {}

TestPaymentsClient::~TestPaymentsClient() {}

void TestPaymentsClient::GetUploadDetails(
    const std::vector<AutofillProfile>& addresses,
    const int detected_values,
    const std::string& pan_first_six,
    const std::vector<const char*>& active_experiments,
    const std::string& app_locale) {
  detected_values_ = detected_values;
  pan_first_six_ = pan_first_six;
  active_experiments_ = active_experiments;
  save_delegate_->OnDidGetUploadDetails(
      app_locale == "en-US" ? AutofillClient::SUCCESS
                            : AutofillClient::PERMANENT_FAILURE,
      base::ASCIIToUTF16("this is a context token"),
      std::unique_ptr<base::DictionaryValue>(nullptr));
}

void TestPaymentsClient::UploadCard(
    const payments::PaymentsClient::UploadRequestDetails& request_details) {
  active_experiments_ = request_details.active_experiments;
  save_delegate_->OnDidUploadCard(AutofillClient::SUCCESS, server_id_);
}

void TestPaymentsClient::SetSaveDelegate(
    payments::PaymentsClientSaveDelegate* save_delegate) {
  save_delegate_ = save_delegate;
  payments::PaymentsClient::SetSaveDelegate(save_delegate);
}

void TestPaymentsClient::SetServerIdForCardUpload(std::string server_id) {
  server_id_ = server_id;
}

int TestPaymentsClient::GetDetectedValuesSetInRequest() const {
  return detected_values_;
}

std::string TestPaymentsClient::GetPanFirstSixSetInRequest() const {
  return pan_first_six_;
}

std::vector<const char*> TestPaymentsClient::GetActiveExperimentsSetInRequest()
    const {
  return active_experiments_;
}

}  // namespace payments
}  // namespace autofill
