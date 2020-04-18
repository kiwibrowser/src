// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/core/autofill_payment_instrument.h"

#include <memory>

#include "base/macros.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/browser/address_normalizer.h"
#include "components/autofill/core/browser/autofill_profile.h"
#include "components/autofill/core/browser/autofill_test_utils.h"
#include "components/autofill/core/browser/credit_card.h"
#include "components/autofill/core/browser/payments/full_card_request.h"
#include "components/autofill/core/browser/payments/payments_client.h"
#include "components/autofill/core/browser/personal_data_manager.h"
#include "components/autofill/core/browser/test_address_normalizer.h"
#include "components/autofill/core/browser/test_autofill_client.h"
#include "components/autofill/core/browser/test_personal_data_manager.h"
#include "components/payments/core/test_payment_request_delegate.h"
#include "components/strings/grit/components_strings.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

namespace payments {

namespace {

class FakePaymentInstrumentDelegate : public PaymentInstrument::Delegate {
 public:
  FakePaymentInstrumentDelegate() {}

  void OnInstrumentDetailsReady(
      const std::string& method_name,
      const std::string& stringified_details) override {
    on_instrument_details_ready_called_ = true;
  }

  void OnInstrumentDetailsError() override {
    on_instrument_details_error_called_ = true;
  }

  bool WasOnInstrumentDetailsReadyCalled() {
    return on_instrument_details_ready_called_;
  }

  bool WasOnInstrumentDetailsErrorCalled() {
    return on_instrument_details_error_called_;
  }

 private:
  bool on_instrument_details_ready_called_ = false;
  bool on_instrument_details_error_called_ = false;
};

class FakePaymentRequestDelegate
    : public PaymentRequestDelegate,
      public autofill::payments::PaymentsClientUnmaskDelegate {
 public:
  FakePaymentRequestDelegate()
      : locale_("en-US"),
        last_committed_url_("https://shop.com"),
        personal_data_("en-US"),
        request_context_(new net::TestURLRequestContextGetter(
            base::ThreadTaskRunnerHandle::Get())),
        payments_client_(request_context_.get(),
                         nullptr,
                         nullptr,
                         this,
                         nullptr),
        full_card_request_(&autofill_client_,
                           &payments_client_,
                           &personal_data_) {}
  void ShowDialog(PaymentRequest* request) override {}

  void CloseDialog() override {}

  void ShowErrorMessage() override {}

  autofill::PersonalDataManager* GetPersonalDataManager() override {
    return nullptr;
  }

  const std::string& GetApplicationLocale() const override { return locale_; }

  bool IsIncognito() const override { return false; }

  bool IsSslCertificateValid() override { return true; }

  const GURL& GetLastCommittedURL() const override {
    return last_committed_url_;
  }

  void DoFullCardRequest(
      const autofill::CreditCard& credit_card,
      base::WeakPtr<autofill::payments::FullCardRequest::ResultDelegate>
          result_delegate) override {
    full_card_request_card_ = credit_card;
    full_card_result_delegate_ = result_delegate;
  }

  autofill::AddressNormalizer* GetAddressNormalizer() override {
    return &address_normalizer_;
  }

  void CompleteFullCardRequest() {
    full_card_result_delegate_->OnFullCardRequestSucceeded(
        full_card_request_, full_card_request_card_, base::ASCIIToUTF16("123"));
  }

  autofill::RegionDataLoader* GetRegionDataLoader() override { return nullptr; }

  ukm::UkmRecorder* GetUkmRecorder() override { return nullptr; }

 private:
  std::string locale_;
  const GURL last_committed_url_;
  autofill::TestAddressNormalizer address_normalizer_;
  autofill::PersonalDataManager personal_data_;
  scoped_refptr<net::TestURLRequestContextGetter> request_context_;
  autofill::TestAutofillClient autofill_client_;
  autofill::payments::PaymentsClient payments_client_;
  autofill::payments::FullCardRequest full_card_request_;
  autofill::CreditCard full_card_request_card_;
  base::WeakPtr<autofill::payments::FullCardRequest::ResultDelegate>
      full_card_result_delegate_;
  DISALLOW_COPY_AND_ASSIGN(FakePaymentRequestDelegate);
};

}  // namespace

class AutofillPaymentInstrumentTest : public testing::Test {
 protected:
  AutofillPaymentInstrumentTest()
      : address_(autofill::test::GetFullProfile()),
        local_card_(autofill::test::GetCreditCard()),
        billing_profiles_({&address_}) {
    local_card_.set_billing_address_id(address_.guid());
  }

  autofill::CreditCard& local_credit_card() { return local_card_; }
  const std::vector<autofill::AutofillProfile*>& billing_profiles() {
    return billing_profiles_;
  }

 private:
  autofill::AutofillProfile address_;
  autofill::CreditCard local_card_;
  std::vector<autofill::AutofillProfile*> billing_profiles_;

  DISALLOW_COPY_AND_ASSIGN(AutofillPaymentInstrumentTest);
};

// A valid local credit card is a valid instrument for payment.
TEST_F(AutofillPaymentInstrumentTest, IsCompleteForPayment) {
  AutofillPaymentInstrument instrument(
      "visa", local_credit_card(),
      /*matches_merchant_card_type_exactly=*/true, billing_profiles(), "en-US",
      nullptr);
  EXPECT_TRUE(instrument.IsCompleteForPayment());
  EXPECT_TRUE(instrument.GetMissingInfoLabel().empty());
}

// An expired local card is still a valid instrument for payment.
TEST_F(AutofillPaymentInstrumentTest, IsCompleteForPayment_Expired) {
  autofill::CreditCard& card = local_credit_card();
  card.SetExpirationYear(2016);  // Expired.
  AutofillPaymentInstrument instrument(
      "visa", card, /*matches_merchant_card_type_exactly=*/true,
      billing_profiles(), "en-US", nullptr);
  EXPECT_TRUE(instrument.IsCompleteForPayment());
  EXPECT_EQ(base::string16(), instrument.GetMissingInfoLabel());
}

// A local card with no name is not a valid instrument for payment.
TEST_F(AutofillPaymentInstrumentTest, IsCompleteForPayment_NoName) {
  autofill::CreditCard& card = local_credit_card();
  card.SetInfo(autofill::AutofillType(autofill::CREDIT_CARD_NAME_FULL),
               base::ASCIIToUTF16(""), "en-US");
  base::string16 missing_info;
  AutofillPaymentInstrument instrument(
      "visa", card, /*matches_merchant_card_type_exactly=*/true,
      billing_profiles(), "en-US", nullptr);
  EXPECT_FALSE(instrument.IsCompleteForPayment());
  EXPECT_EQ(l10n_util::GetStringUTF16(IDS_PAYMENTS_NAME_ON_CARD_REQUIRED),
            instrument.GetMissingInfoLabel());
}

// A local card with no name is not a valid instrument for payment.
TEST_F(AutofillPaymentInstrumentTest, IsCompleteForPayment_NoNumber) {
  autofill::CreditCard& card = local_credit_card();
  card.SetNumber(base::ASCIIToUTF16(""));
  base::string16 missing_info;
  AutofillPaymentInstrument instrument(
      "visa", card, /*matches_merchant_card_type_exactly=*/true,
      billing_profiles(), "en-US", nullptr);
  EXPECT_FALSE(instrument.IsCompleteForPayment());
  EXPECT_EQ(l10n_util::GetStringUTF16(
                IDS_PAYMENTS_CARD_NUMBER_INVALID_VALIDATION_MESSAGE),
            instrument.GetMissingInfoLabel());
}

// A local card with no billing address id is not a valid instrument for
// payment.
TEST_F(AutofillPaymentInstrumentTest, IsCompleteForPayment_NoBillinbAddressId) {
  autofill::CreditCard& card = local_credit_card();
  card.set_billing_address_id("");
  base::string16 missing_info;
  AutofillPaymentInstrument instrument(
      "visa", card, /*matches_merchant_card_type_exactly=*/true,
      billing_profiles(), "en-US", nullptr);
  EXPECT_FALSE(instrument.IsCompleteForPayment());
  EXPECT_EQ(
      l10n_util::GetStringUTF16(IDS_PAYMENTS_CARD_BILLING_ADDRESS_REQUIRED),
      instrument.GetMissingInfoLabel());
}

// A local card with an invalid billing address id is not a valid instrument for
// payment.
TEST_F(AutofillPaymentInstrumentTest,
       IsCompleteForPayment_InvalidBillinbAddressId) {
  autofill::CreditCard& card = local_credit_card();
  card.set_billing_address_id("InvalidBillingAddressId");
  base::string16 missing_info;
  AutofillPaymentInstrument instrument(
      "visa", card, /*matches_merchant_card_type_exactly=*/true,
      billing_profiles(), "en-US", nullptr);
  EXPECT_FALSE(instrument.IsCompleteForPayment());
  EXPECT_EQ(
      l10n_util::GetStringUTF16(IDS_PAYMENTS_CARD_BILLING_ADDRESS_REQUIRED),
      instrument.GetMissingInfoLabel());
}

// A local card with no name and no number is not a valid instrument for
// payment.
TEST_F(AutofillPaymentInstrumentTest,
       IsCompleteForPayment_MultipleThingsMissing) {
  autofill::CreditCard& card = local_credit_card();
  card.SetNumber(base::ASCIIToUTF16(""));
  card.SetInfo(autofill::AutofillType(autofill::CREDIT_CARD_NAME_FULL),
               base::ASCIIToUTF16(""), "en-US");
  AutofillPaymentInstrument instrument(
      "visa", card, /*matches_merchant_card_type_exactly=*/true,
      billing_profiles(), "en-US", nullptr);
  EXPECT_FALSE(instrument.IsCompleteForPayment());
  EXPECT_EQ(l10n_util::GetStringUTF16(IDS_PAYMENTS_MORE_INFORMATION_REQUIRED),
            instrument.GetMissingInfoLabel());
}

// A Masked (server) card is a valid instrument for payment.
TEST_F(AutofillPaymentInstrumentTest, IsCompleteForPayment_MaskedCard) {
  autofill::CreditCard card = autofill::test::GetMaskedServerCard();
  ASSERT_GT(billing_profiles().size(), 0UL);
  card.set_billing_address_id(billing_profiles()[0]->guid());
  AutofillPaymentInstrument instrument(
      "visa", card, /*matches_merchant_card_type_exactly=*/true,
      billing_profiles(), "en-US", nullptr);
  EXPECT_TRUE(instrument.IsCompleteForPayment());
  EXPECT_TRUE(instrument.GetMissingInfoLabel().empty());
}

// An expired masked (server) card is still a valid instrument for payment.
TEST_F(AutofillPaymentInstrumentTest, IsCompleteForPayment_ExpiredMaskedCard) {
  autofill::CreditCard card = autofill::test::GetMaskedServerCard();
  ASSERT_GT(billing_profiles().size(), 0UL);
  card.set_billing_address_id(billing_profiles()[0]->guid());
  card.SetExpirationYear(2016);  // Expired.
  AutofillPaymentInstrument instrument(
      "visa", card, /*matches_merchant_card_type_exactly=*/true,
      billing_profiles(), "en-US", nullptr);
  EXPECT_TRUE(instrument.IsCompleteForPayment());
  EXPECT_EQ(base::string16(), instrument.GetMissingInfoLabel());
}

// An expired card is a valid instrument for canMakePayment.
TEST_F(AutofillPaymentInstrumentTest, IsValidForCanMakePayment_Minimal) {
  autofill::CreditCard& card = local_credit_card();
  card.SetExpirationYear(2016);  // Expired.
  AutofillPaymentInstrument instrument(
      "visa", card, /*matches_merchant_card_type_exactly=*/true,
      billing_profiles(), "en-US", nullptr);
  EXPECT_TRUE(instrument.IsValidForCanMakePayment());
}

// An expired Masked (server) card is a valid instrument for canMakePayment.
TEST_F(AutofillPaymentInstrumentTest, IsValidForCanMakePayment_MaskedCard) {
  autofill::CreditCard card = autofill::test::GetMaskedServerCard();
  card.SetExpirationYear(2016);  // Expired.
  AutofillPaymentInstrument instrument(
      "visa", card, /*matches_merchant_card_type_exactly=*/true,
      billing_profiles(), "en-US", nullptr);
  EXPECT_TRUE(instrument.IsValidForCanMakePayment());
}

// A card with no name is not a valid instrument for canMakePayment.
TEST_F(AutofillPaymentInstrumentTest, IsValidForCanMakePayment_NoName) {
  autofill::CreditCard& card = local_credit_card();
  card.SetInfo(autofill::AutofillType(autofill::CREDIT_CARD_NAME_FULL),
               base::ASCIIToUTF16(""), "en-US");
  AutofillPaymentInstrument instrument(
      "visa", card, /*matches_merchant_card_type_exactly=*/true,
      billing_profiles(), "en-US", nullptr);
  EXPECT_FALSE(instrument.IsValidForCanMakePayment());
}

// A card with no number is not a valid instrument for canMakePayment.
TEST_F(AutofillPaymentInstrumentTest, IsValidForCanMakePayment_NoNumber) {
  autofill::CreditCard& card = local_credit_card();
  card.SetNumber(base::ASCIIToUTF16(""));
  AutofillPaymentInstrument instrument(
      "visa", card, /*matches_merchant_card_type_exactly=*/true,
      billing_profiles(), "en-US", nullptr);
  EXPECT_FALSE(instrument.IsValidForCanMakePayment());
}

// Tests that the autofill instrument only calls OnInstrumentDetailsReady when
// the billing address has been normalized and the card has been unmasked.
TEST_F(AutofillPaymentInstrumentTest,
       InvokePaymentApp_NormalizationBeforeUnmask) {
  auto personal_data_manager =
      std::make_unique<autofill::TestPersonalDataManager>();
  TestPaymentRequestDelegate delegate(personal_data_manager.get());
  delegate.DelayFullCardRequestCompletion();
  delegate.test_address_normalizer()->DelayNormalization();

  autofill::CreditCard& card = local_credit_card();
  card.SetNumber(base::ASCIIToUTF16(""));
  AutofillPaymentInstrument instrument(
      "visa", card, /*matches_merchant_card_type_exactly=*/true,
      billing_profiles(), "en-US", &delegate);

  FakePaymentInstrumentDelegate instrument_delegate;

  instrument.InvokePaymentApp(&instrument_delegate);
  EXPECT_FALSE(instrument_delegate.WasOnInstrumentDetailsReadyCalled());
  EXPECT_FALSE(instrument_delegate.WasOnInstrumentDetailsErrorCalled());

  delegate.test_address_normalizer()->CompleteAddressNormalization();
  EXPECT_FALSE(instrument_delegate.WasOnInstrumentDetailsReadyCalled());
  EXPECT_FALSE(instrument_delegate.WasOnInstrumentDetailsErrorCalled());

  delegate.CompleteFullCardRequest();
  EXPECT_TRUE(instrument_delegate.WasOnInstrumentDetailsReadyCalled());
  EXPECT_FALSE(instrument_delegate.WasOnInstrumentDetailsErrorCalled());
}

// Tests that the autofill instrument only calls OnInstrumentDetailsReady when
// the billing address has been normalized and the card has been unmasked.
TEST_F(AutofillPaymentInstrumentTest,
       InvokePaymentApp_UnmaskBeforeNormalization) {
  auto personal_data_manager =
      std::make_unique<autofill::TestPersonalDataManager>();
  TestPaymentRequestDelegate delegate(personal_data_manager.get());
  delegate.DelayFullCardRequestCompletion();
  delegate.test_address_normalizer()->DelayNormalization();

  autofill::CreditCard& card = local_credit_card();
  card.SetNumber(base::ASCIIToUTF16(""));
  AutofillPaymentInstrument instrument(
      "visa", card, /*matches_merchant_card_type_exactly=*/true,
      billing_profiles(), "en-US", &delegate);

  FakePaymentInstrumentDelegate instrument_delegate;

  instrument.InvokePaymentApp(&instrument_delegate);
  EXPECT_FALSE(instrument_delegate.WasOnInstrumentDetailsReadyCalled());
  EXPECT_FALSE(instrument_delegate.WasOnInstrumentDetailsErrorCalled());

  delegate.CompleteFullCardRequest();
  EXPECT_FALSE(instrument_delegate.WasOnInstrumentDetailsReadyCalled());
  EXPECT_FALSE(instrument_delegate.WasOnInstrumentDetailsErrorCalled());

  delegate.test_address_normalizer()->CompleteAddressNormalization();
  EXPECT_TRUE(instrument_delegate.WasOnInstrumentDetailsReadyCalled());
  EXPECT_FALSE(instrument_delegate.WasOnInstrumentDetailsErrorCalled());
}

}  // namespace payments
