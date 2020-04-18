// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/test_credit_card_save_manager.h"

#include "components/autofill/core/browser/payments/test_payments_client.h"

namespace autofill {

TestCreditCardSaveManager::TestCreditCardSaveManager(
    AutofillDriver* driver,
    AutofillClient* client,
    payments::TestPaymentsClient* payments_client,
    PersonalDataManager* personal_data_manager)
    : CreditCardSaveManager(client,
                            payments_client,
                            "en-US",
                            personal_data_manager),
      test_payments_client_(payments_client) {
  if (test_payments_client_) {
    test_payments_client_->SetSaveDelegate(this);
  }
}

TestCreditCardSaveManager::~TestCreditCardSaveManager() {}

bool TestCreditCardSaveManager::IsCreditCardUploadEnabled() {
  return credit_card_upload_enabled_;
}

void TestCreditCardSaveManager::SetCreditCardUploadEnabled(
    bool credit_card_upload_enabled) {
  credit_card_upload_enabled_ = credit_card_upload_enabled;
}

bool TestCreditCardSaveManager::CreditCardWasUploaded() {
  return credit_card_was_uploaded_;
}

void TestCreditCardSaveManager::OnDidUploadCard(
    AutofillClient::PaymentsRpcResult result,
    const std::string& server_id) {
  credit_card_was_uploaded_ = true;
  CreditCardSaveManager::OnDidUploadCard(result, server_id);
}

}  // namespace autofill
