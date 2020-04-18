// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/content/service_worker_payment_instrument.h"

#include <memory>

#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "components/payments/core/payment_request_delegate.h"
#include "content/public/browser/stored_payment_app.h"
#include "content/public/test/test_browser_context.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/modules/payments/payment_request.mojom.h"

namespace payments {
namespace {

class MockPaymentRequestDelegate : public PaymentRequestDelegate {
 public:
  MockPaymentRequestDelegate() {}
  ~MockPaymentRequestDelegate() override {}
  MOCK_METHOD1(ShowDialog, void(PaymentRequest* request));
  MOCK_METHOD0(CloseDialog, void());
  MOCK_METHOD0(ShowErrorMessage, void());
  MOCK_METHOD0(ShowProcessingSpinner, void());
  MOCK_CONST_METHOD0(IsBrowserWindowActive, bool());
  MOCK_METHOD0(GetPersonalDataManager, autofill::PersonalDataManager*());
  MOCK_CONST_METHOD0(GetApplicationLocale, const std::string&());
  MOCK_CONST_METHOD0(IsIncognito, bool());
  MOCK_METHOD0(IsSslCertificateValid, bool());
  MOCK_CONST_METHOD0(GetLastCommittedURL, const GURL&());
  MOCK_METHOD2(
      DoFullCardRequest,
      void(const autofill::CreditCard& credit_card,
           base::WeakPtr<autofill::payments::FullCardRequest::ResultDelegate>
               result_delegate));
  MOCK_METHOD0(GetAddressNormalizer, autofill::AddressNormalizer*());
  MOCK_METHOD0(GetRegionDataLoader, autofill::RegionDataLoader*());
  MOCK_METHOD0(GetUkmRecorder, ukm::UkmRecorder*());
  MOCK_CONST_METHOD0(GetAuthenticatedEmail, std::string());
  MOCK_METHOD0(GetPrefService, PrefService*());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockPaymentRequestDelegate);
};

}  // namespace

class ServiceWorkerPaymentInstrumentTest : public testing::Test,
                                           public PaymentRequestSpec::Observer {
 public:
  ServiceWorkerPaymentInstrumentTest() {}
  ~ServiceWorkerPaymentInstrumentTest() override {}

 protected:
  void OnSpecUpdated() override {}

  void SetUp() override {
    mojom::PaymentDetailsPtr details = mojom::PaymentDetails::New();
    mojom::PaymentItemPtr total = mojom::PaymentItem::New();
    mojom::PaymentCurrencyAmountPtr amount =
        mojom::PaymentCurrencyAmount::New();
    amount->value = "5.00";
    amount->currency = "USD";
    total->amount = std::move(amount);
    details->total = std::move(total);
    details->id = base::Optional<std::string>("123456");

    mojom::PaymentDetailsModifierPtr modifier_1 =
        mojom::PaymentDetailsModifier::New();
    modifier_1->total = mojom::PaymentItem::New();
    modifier_1->total->amount = mojom::PaymentCurrencyAmount::New();
    modifier_1->total->amount->currency = "USD";
    modifier_1->total->amount->value = "4.00";
    modifier_1->method_data = mojom::PaymentMethodData::New();
    modifier_1->method_data->supported_methods = {"basic-card"};
    details->modifiers.push_back(std::move(modifier_1));

    mojom::PaymentDetailsModifierPtr modifier_2 =
        mojom::PaymentDetailsModifier::New();
    modifier_2->total = mojom::PaymentItem::New();
    modifier_2->total->amount = mojom::PaymentCurrencyAmount::New();
    modifier_2->total->amount->currency = "USD";
    modifier_2->total->amount->value = "3.00";
    modifier_2->method_data = mojom::PaymentMethodData::New();
    modifier_2->method_data->supported_methods = {"https://bobpay.com"};
    details->modifiers.push_back(std::move(modifier_2));

    mojom::PaymentDetailsModifierPtr modifier_3 =
        mojom::PaymentDetailsModifier::New();
    modifier_3->total = mojom::PaymentItem::New();
    modifier_3->total->amount = mojom::PaymentCurrencyAmount::New();
    modifier_3->total->amount->currency = "USD";
    modifier_3->total->amount->value = "2.00";
    modifier_3->method_data = mojom::PaymentMethodData::New();
    modifier_3->method_data->supported_methods = {"https://alicepay.com"};
    details->modifiers.push_back(std::move(modifier_3));

    std::vector<mojom::PaymentMethodDataPtr> method_data;
    mojom::PaymentMethodDataPtr entry_1 = mojom::PaymentMethodData::New();
    entry_1->supported_methods.push_back("basic-card");
    entry_1->supported_networks.push_back(mojom::BasicCardNetwork::UNIONPAY);
    entry_1->supported_networks.push_back(mojom::BasicCardNetwork::JCB);
    entry_1->supported_networks.push_back(mojom::BasicCardNetwork::VISA);
    entry_1->supported_types.push_back(mojom::BasicCardType::DEBIT);
    method_data.push_back(std::move(entry_1));

    mojom::PaymentMethodDataPtr entry_2 = mojom::PaymentMethodData::New();
    entry_2->supported_methods.push_back("https://bobpay.com");
    method_data.push_back(std::move(entry_2));

    spec_ = std::make_unique<PaymentRequestSpec>(
        mojom::PaymentOptions::New(), std::move(details),
        std::move(method_data), this, "en-US");
  }

  void TearDown() override {}

  void CreateServiceWorkerPaymentInstrument(bool with_url_method) {
    std::unique_ptr<content::StoredPaymentApp> stored_app =
        std::make_unique<content::StoredPaymentApp>();
    stored_app->registration_id = 123456;
    stored_app->scope = GURL("https://bobpay.com");
    stored_app->name = "bobpay";
    stored_app->icon.reset(new SkBitmap());
    stored_app->enabled_methods.emplace_back("basic-card");
    if (with_url_method)
      stored_app->enabled_methods.emplace_back("https://bobpay.com");
    stored_app->capabilities.emplace_back(content::StoredCapabilities());
    stored_app->capabilities.back().supported_card_networks.emplace_back(
        static_cast<int32_t>(mojom::BasicCardNetwork::UNIONPAY));
    stored_app->capabilities.back().supported_card_networks.emplace_back(
        static_cast<int32_t>(mojom::BasicCardNetwork::JCB));
    stored_app->capabilities.back().supported_card_types.emplace_back(
        static_cast<int32_t>(mojom::BasicCardType::DEBIT));
    stored_app->user_hint = "Visa 4012 ... 1881";
    stored_app->prefer_related_applications = false;

    instrument_ = std::make_unique<ServiceWorkerPaymentInstrument>(
        &browser_context_, GURL("https://testmerchant.com"),
        GURL("https://testmerchant.com/bobpay"), spec_.get(),
        std::move(stored_app), &delegate_);
  }

  ServiceWorkerPaymentInstrument* GetInstrument() { return instrument_.get(); }

  mojom::PaymentRequestEventDataPtr CreatePaymentRequestEventData() {
    return instrument_->CreatePaymentRequestEventData();
  }

  mojom::CanMakePaymentEventDataPtr CreateCanMakePaymentEventData() {
    return instrument_->CreateCanMakePaymentEventData();
  }

 private:
  MockPaymentRequestDelegate delegate_;
  content::TestBrowserThreadBundle thread_bundle_;
  content::TestBrowserContext browser_context_;

  std::unique_ptr<PaymentRequestSpec> spec_;
  std::unique_ptr<ServiceWorkerPaymentInstrument> instrument_;

  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerPaymentInstrumentTest);
};

// Test instrument info and status are correct.
TEST_F(ServiceWorkerPaymentInstrumentTest, InstrumentInfo) {
  CreateServiceWorkerPaymentInstrument(true);

  EXPECT_TRUE(GetInstrument()->IsCompleteForPayment());
  EXPECT_TRUE(GetInstrument()->IsExactlyMatchingMerchantRequest());

  EXPECT_EQ(base::UTF16ToUTF8(GetInstrument()->GetLabel()), "bobpay");
  EXPECT_EQ(base::UTF16ToUTF8(GetInstrument()->GetSublabel()), "bobpay.com");
  EXPECT_NE(GetInstrument()->icon_image_skia(), nullptr);
}

// Test payment request event data can be correctly constructed for invoking
// InvokePaymentApp.
TEST_F(ServiceWorkerPaymentInstrumentTest, CreatePaymentRequestEventData) {
  CreateServiceWorkerPaymentInstrument(true);

  mojom::PaymentRequestEventDataPtr event_data =
      CreatePaymentRequestEventData();

  EXPECT_EQ(event_data->top_origin.spec(), "https://testmerchant.com/");
  EXPECT_EQ(event_data->payment_request_origin.spec(),
            "https://testmerchant.com/bobpay");

  EXPECT_EQ(event_data->method_data.size(), 2U);
  EXPECT_EQ(event_data->method_data[0]->supported_methods.size(), 1U);
  EXPECT_EQ(event_data->method_data[0]->supported_methods[0], "basic-card");
  EXPECT_EQ(event_data->method_data[0]->supported_networks.size(), 3U);
  EXPECT_EQ(event_data->method_data[0]->supported_types.size(), 1U);
  EXPECT_EQ(event_data->method_data[1]->supported_methods.size(), 1U);
  EXPECT_EQ(event_data->method_data[1]->supported_methods[0],
            "https://bobpay.com");

  EXPECT_EQ(event_data->total->currency, "USD");
  EXPECT_EQ(event_data->total->value, "5.00");
  EXPECT_EQ(event_data->payment_request_id, "123456");

  EXPECT_EQ(event_data->modifiers.size(), 2U);
  EXPECT_EQ(event_data->modifiers[0]->total->amount->value, "4.00");
  EXPECT_EQ(event_data->modifiers[0]->total->amount->currency, "USD");
  EXPECT_EQ(event_data->modifiers[0]->method_data->supported_methods[0],
            "basic-card");
  EXPECT_EQ(event_data->modifiers[1]->total->amount->value, "3.00");
  EXPECT_EQ(event_data->modifiers[1]->total->amount->currency, "USD");
  EXPECT_EQ(event_data->modifiers[1]->method_data->supported_methods[0],
            "https://bobpay.com");
}

// Test CanMakePaymentEventData can be correctly constructed for invoking
// Validate.
TEST_F(ServiceWorkerPaymentInstrumentTest, CreateCanMakePaymentEvent) {
  CreateServiceWorkerPaymentInstrument(false);
  mojom::CanMakePaymentEventDataPtr event_data =
      CreateCanMakePaymentEventData();
  EXPECT_TRUE(event_data.is_null());

  CreateServiceWorkerPaymentInstrument(true);
  event_data = CreateCanMakePaymentEventData();
  EXPECT_FALSE(event_data.is_null());

  EXPECT_EQ(event_data->top_origin.spec(), "https://testmerchant.com/");
  EXPECT_EQ(event_data->payment_request_origin.spec(),
            "https://testmerchant.com/bobpay");

  EXPECT_EQ(event_data->method_data.size(), 1U);
  EXPECT_EQ(event_data->method_data[0]->supported_methods.size(), 1U);
  EXPECT_EQ(event_data->method_data[0]->supported_methods[0],
            "https://bobpay.com");

  EXPECT_EQ(event_data->modifiers.size(), 1U);
  EXPECT_EQ(event_data->modifiers[0]->total->amount->value, "3.00");
  EXPECT_EQ(event_data->modifiers[0]->total->amount->currency, "USD");
  EXPECT_EQ(event_data->modifiers[0]->method_data->supported_methods[0],
            "https://bobpay.com");
}

// Test modifiers can be matched based on capabilities.
TEST_F(ServiceWorkerPaymentInstrumentTest, IsValidForModifier) {
  CreateServiceWorkerPaymentInstrument(true);

  EXPECT_TRUE(GetInstrument()->IsValidForModifier({"basic-card"}, false, {},
                                                  false, {}));

  EXPECT_TRUE(GetInstrument()->IsValidForModifier({"https://bobpay.com"}, true,
                                                  {}, true, {}));

  EXPECT_TRUE(GetInstrument()->IsValidForModifier(
      {"basic-card", "https://bobpay.com"}, false, {}, false, {}));

  EXPECT_TRUE(GetInstrument()->IsValidForModifier(
      {"basic-card", "https://bobpay.com"}, true, {"mastercard"}, false, {}));

  EXPECT_FALSE(GetInstrument()->IsValidForModifier({"basic-card"}, true,
                                                   {"mastercard"}, false, {}));

  EXPECT_TRUE(GetInstrument()->IsValidForModifier({"basic-card"}, true,
                                                  {"unionpay"}, false, {}));

  EXPECT_TRUE(GetInstrument()->IsValidForModifier(
      {"basic-card"}, true, {"unionpay"}, true,
      {autofill::CreditCard::CardType::CARD_TYPE_DEBIT,
       autofill::CreditCard::CardType::CARD_TYPE_CREDIT}));

  EXPECT_FALSE(GetInstrument()->IsValidForModifier(
      {"basic-card"}, true, {"unionpay"}, true,
      {autofill::CreditCard::CardType::CARD_TYPE_CREDIT}));
}

}  // namespace payments
