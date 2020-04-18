// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <map>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/ui/views/payments/payment_request_browsertest_base.h"
#include "components/autofill/core/browser/autofill_test_utils.h"
#include "components/autofill/core/browser/credit_card.h"
#include "content/public/common/content_features.h"
#include "content/public/test/browser_test_utils.h"

namespace payments {

class PaymentRequestCanMakePaymentQueryTest
    : public PaymentRequestBrowserTestBase {
 protected:
  PaymentRequestCanMakePaymentQueryTest() {}

  void SetUpOnMainThread() override {
    PaymentRequestBrowserTestBase::SetUpOnMainThread();

    // By default for these tests, can make payment is enabled. Individual tests
    // may override to false.
    SetCanMakePaymentEnabledPref(true);
  }

  void CallCanMakePayment() {
    ResetEventWaiterForSequence({DialogEvent::CAN_MAKE_PAYMENT_CALLED,
                                 DialogEvent::CAN_MAKE_PAYMENT_RETURNED});
    ASSERT_TRUE(content::ExecuteScript(GetActiveWebContents(), "buy();"));
    WaitForObservedEvent();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(PaymentRequestCanMakePaymentQueryTest);
};

// Visa is required, and user has a visa instrument.
IN_PROC_BROWSER_TEST_F(PaymentRequestCanMakePaymentQueryTest,
                       CanMakePayment_Supported) {
  NavigateTo("/payment_request_can_make_payment_query_test.html");
  const autofill::CreditCard card = autofill::test::GetCreditCard();  // Visa.
  AddCreditCard(card);

  CallCanMakePayment();

  ExpectBodyContains({"true"});
}

// Visa is required, and user has a visa instrument, but canMakePayment is
// disabled by user preference.
IN_PROC_BROWSER_TEST_F(PaymentRequestCanMakePaymentQueryTest,
                       CanMakePayment_SupportedButDisabled) {
  SetCanMakePaymentEnabledPref(false);
  NavigateTo("/payment_request_can_make_payment_query_test.html");
  const autofill::CreditCard card = autofill::test::GetCreditCard();  // Visa.
  AddCreditCard(card);

  CallCanMakePayment();

  ExpectBodyContains({"false"});
}

// Pages without a valid SSL certificate always get "false" from
// .canMakePayment().
IN_PROC_BROWSER_TEST_F(PaymentRequestCanMakePaymentQueryTest,
                       CanMakePayment_InvalidSSL) {
  NavigateTo("/payment_request_can_make_payment_query_test.html");
  SetInvalidSsl();

  const autofill::CreditCard card = autofill::test::GetCreditCard();  // Visa.
  AddCreditCard(card);

  CallCanMakePayment();

  ExpectBodyContains({"false"});
}

// Visa is required, user has a visa instrument, and user is in incognito
// mode.
IN_PROC_BROWSER_TEST_F(PaymentRequestCanMakePaymentQueryTest,
                       CanMakePayment_Supported_InIncognitoMode) {
  NavigateTo("/payment_request_can_make_payment_query_test.html");
  SetIncognito();

  const autofill::CreditCard card = autofill::test::GetCreditCard();  // Visa.
  AddCreditCard(card);

  CallCanMakePayment();

  ExpectBodyContains({"true"});
}

// Visa is required, and user doesn't have a visa instrument.
IN_PROC_BROWSER_TEST_F(PaymentRequestCanMakePaymentQueryTest,
                       CanMakePayment_NotSupported) {
  NavigateTo("/payment_request_can_make_payment_query_test.html");
  const autofill::CreditCard card = autofill::test::GetCreditCard2();  // Amex.
  AddCreditCard(card);

  CallCanMakePayment();

  ExpectBodyContains({"false"});
}

// Visa is required, user doesn't have a visa instrument and the user is in
// incognito mode. In this case canMakePayment returns false as in a normal
// profile.
IN_PROC_BROWSER_TEST_F(PaymentRequestCanMakePaymentQueryTest,
                       CanMakePayment_NotSupported_InIncognitoMode) {
  NavigateTo("/payment_request_can_make_payment_query_test.html");
  SetIncognito();

  const autofill::CreditCard card = autofill::test::GetCreditCard2();  // Amex.
  AddCreditCard(card);

  CallCanMakePayment();

  ExpectBodyContains({"false"});
}

class PaymentRequestCanMakePaymentQueryCCTest
    : public PaymentRequestBrowserTestBase {
 protected:
  PaymentRequestCanMakePaymentQueryCCTest() {}

  // If |visa| is true, then the method data is:
  //
  //   [{supportedMethods: ['visa']}]
  //
  // If |visa| is false, then the method data is:
  //
  //   [{supportedMethods: ['mastercard']}]
  void CallCanMakePayment(bool visa) {
    ResetEventWaiterForSequence({DialogEvent::CAN_MAKE_PAYMENT_CALLED,
                                 DialogEvent::CAN_MAKE_PAYMENT_RETURNED});
    ASSERT_TRUE(content::ExecuteScript(GetActiveWebContents(),
                                       visa ? "buy();" : "other_buy();"));
    WaitForObservedEvent();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(PaymentRequestCanMakePaymentQueryCCTest);
};

IN_PROC_BROWSER_TEST_F(PaymentRequestCanMakePaymentQueryCCTest, QueryQuota) {
  NavigateTo("/payment_request_can_make_payment_query_cc_test.html");
  // Query "visa" payment method.
  CallCanMakePayment(/*visa=*/true);

  // User does not have a visa card.
  ExpectBodyContains({"false"});

  // Query "mastercard" payment method.
  CallCanMakePayment(/*visa=*/false);

  // Query quota exceeded.
  ExpectBodyContains({"NotAllowedError"});

  AddCreditCard(autofill::test::GetCreditCard());  // visa

  // Query "visa" payment method.
  CallCanMakePayment(/*visa=*/true);

  // User now has a visa card. The query is cached, but the result is always
  // fresh.
  ExpectBodyContains({"true"});

  // Query "mastercard" payment method.
  CallCanMakePayment(/*visa=*/false);

  // Query quota exceeded.
  ExpectBodyContains({"NotAllowedError"});
}

// canMakePayment should return result in incognito mode as in normal mode to
// avoid incognito mode detection.
IN_PROC_BROWSER_TEST_F(PaymentRequestCanMakePaymentQueryCCTest,
                       QueryQuotaInIncognito) {
  NavigateTo("/payment_request_can_make_payment_query_cc_test.html");
  SetIncognito();

  // Query "visa" payment method.
  CallCanMakePayment(/*visa=*/true);

  ExpectBodyContains({"false"});

  // Query "mastercard" payment method.
  CallCanMakePayment(/*visa=*/false);

  ExpectBodyContains({"NotAllowedError"});

  AddCreditCard(autofill::test::GetCreditCard());  // visa

  // Query "visa" payment method.
  CallCanMakePayment(/*visa=*/true);

  ExpectBodyContains({"true"});

  // Query "mastercard" payment method.
  CallCanMakePayment(/*visa=*/false);

  ExpectBodyContains({"NotAllowedError"});
}

class PaymentRequestCanMakePaymentQueryPMITest
    : public PaymentRequestBrowserTestBase {
 protected:
  enum class CheckFor {
    BASIC_VISA,
    BASIC_CARD,
    ALICE_PAY,
    BOB_PAY,
    BOB_PAY_AND_BASIC_CARD,
    BOB_PAY_AND_VISA,
  };

  PaymentRequestCanMakePaymentQueryPMITest() {
    script_[CheckFor::BASIC_VISA] = "checkBasicVisa();";
    script_[CheckFor::BASIC_CARD] = "checkBasicCard();";
    script_[CheckFor::ALICE_PAY] = "checkAlicePay();";
    script_[CheckFor::BOB_PAY] = "checkBobPay();";
    script_[CheckFor::BOB_PAY_AND_BASIC_CARD] = "checkBobPayAndBasicCard();";
    script_[CheckFor::BOB_PAY_AND_VISA] = "checkBobPayAndVisa();";
  }

  void CallCanMakePayment(CheckFor check_for) {
    ResetEventWaiterForSequence({DialogEvent::CAN_MAKE_PAYMENT_CALLED,
                                 DialogEvent::CAN_MAKE_PAYMENT_RETURNED});
    ASSERT_TRUE(
        content::ExecuteScript(GetActiveWebContents(), script_[check_for]));
    WaitForObservedEvent();
  }

 private:
  std::map<CheckFor, std::string> script_;
  DISALLOW_COPY_AND_ASSIGN(PaymentRequestCanMakePaymentQueryPMITest);
};

IN_PROC_BROWSER_TEST_F(PaymentRequestCanMakePaymentQueryPMITest,
                       QueryQuotaForBasicCards) {
  NavigateTo("/payment_request_payment_method_identifier_test.html");
  // Query "basic-card" payment method with "supportedNetworks": ["visa"] in the
  // payment method specific data.
  CallCanMakePayment(CheckFor::BASIC_VISA);

  // User does not have a visa card.
  ExpectBodyContains({"false"});

  // Query "basic-card" payment method without "supportedNetworks" parameter.
  CallCanMakePayment(CheckFor::BASIC_CARD);

  // Query quota exceeded.
  ExpectBodyContains({"NotAllowedError"});

  AddCreditCard(autofill::test::GetCreditCard());  // visa

  // Query "basic-card" payment method with "supportedNetworks": ["visa"] in the
  // payment method specific data.
  CallCanMakePayment(CheckFor::BASIC_VISA);

  // User now has a visa card. The query is cached, but the result is always
  // fresh.
  ExpectBodyContains({"true"});

  // Query "basic-card" payment method without "supportedNetworks" parameter.
  CallCanMakePayment(CheckFor::BASIC_CARD);

  // Query quota exceeded.
  ExpectBodyContains({"NotAllowedError"});
}

// canMakePayment() should return result as in normal mode to avoid incognito
// mode detection.
IN_PROC_BROWSER_TEST_F(PaymentRequestCanMakePaymentQueryPMITest,
                       QueryQuotaForBasicCardsInIncognito) {
  NavigateTo("/payment_request_payment_method_identifier_test.html");
  SetIncognito();

  // Query "basic-card" payment method with "supportedNetworks": ["visa"] in the
  // payment method specific data.
  CallCanMakePayment(CheckFor::BASIC_VISA);

  ExpectBodyContains({"false"});

  // Query "basic-card" payment method without "supportedNetworks" parameter.
  CallCanMakePayment(CheckFor::BASIC_CARD);

  ExpectBodyContains({"NotAllowedError"});

  AddCreditCard(autofill::test::GetCreditCard());  // visa

  // Query "basic-card" payment method with "supportedNetworks": ["visa"] in the
  // payment method specific data.
  CallCanMakePayment(CheckFor::BASIC_VISA);

  ExpectBodyContains({"true"});

  // Query "basic-card" payment method without "supportedNetworks" parameter.
  CallCanMakePayment(CheckFor::BASIC_CARD);

  ExpectBodyContains({"NotAllowedError"});
}

// If the device does not have any payment apps installed, canMakePayment()
// should return false for them.
IN_PROC_BROWSER_TEST_F(PaymentRequestCanMakePaymentQueryPMITest,
                       QueryQuotaForPaymentApps) {
  NavigateTo("/payment_request_payment_method_identifier_test.html");
  CallCanMakePayment(CheckFor::ALICE_PAY);

  ExpectBodyContains({"false"});

  CallCanMakePayment(CheckFor::BOB_PAY);

  ExpectBodyContains({"NotAllowedError"});

  CallCanMakePayment(CheckFor::ALICE_PAY);

  ExpectBodyContains({"false"});
}

// If the device does not have any payment apps installed, canMakePayment()
// queries for both payment apps and basic-card depend only on what cards the
// user has on file.
IN_PROC_BROWSER_TEST_F(PaymentRequestCanMakePaymentQueryPMITest,
                       QueryQuotaForPaymentAppsAndCards) {
  NavigateTo("/payment_request_payment_method_identifier_test.html");
  CallCanMakePayment(CheckFor::BOB_PAY_AND_VISA);

  ExpectBodyContains({"false"});

  CallCanMakePayment(CheckFor::BOB_PAY_AND_BASIC_CARD);

  ExpectBodyContains({"NotAllowedError"});

  AddCreditCard(autofill::test::GetCreditCard2());  // Amex

  CallCanMakePayment(CheckFor::BOB_PAY_AND_VISA);

  ExpectBodyContains({"false"});

  CallCanMakePayment(CheckFor::BOB_PAY_AND_BASIC_CARD);

  ExpectBodyContains({"NotAllowedError"});

  AddCreditCard(autofill::test::GetCreditCard());  // Visa

  CallCanMakePayment(CheckFor::BOB_PAY_AND_VISA);

  ExpectBodyContains({"true"});

  CallCanMakePayment(CheckFor::BOB_PAY_AND_BASIC_CARD);

  ExpectBodyContains({"NotAllowedError"});
}

// Querying for payment apps in incognito returns result as normal mode to avoid
// incognito mode detection. Multiple queries for different apps are rejected
// with NotSupportedError to avoid user fingerprinting.
IN_PROC_BROWSER_TEST_F(PaymentRequestCanMakePaymentQueryPMITest,
                       QueryQuotaForPaymentAppsInIncognitoMode) {
  NavigateTo("/payment_request_payment_method_identifier_test.html");
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(features::kServiceWorkerPaymentApps);

  SetIncognito();

  CallCanMakePayment(CheckFor::ALICE_PAY);

  ExpectBodyContains({"false"});

  CallCanMakePayment(CheckFor::BOB_PAY);

  ExpectBodyContains({"NotAllowedError"});

  CallCanMakePayment(CheckFor::ALICE_PAY);

  ExpectBodyContains({"false"});

  CallCanMakePayment(CheckFor::BOB_PAY);

  ExpectBodyContains({"NotAllowedError"});
}

// Querying for both payment apps and autofill cards in incognito returns result
// as in normal mode to avoid incognito mode detection.
// Multiple queries for different payment methods are rejected with
// NotSupportedError to avoid user fingerprinting.
IN_PROC_BROWSER_TEST_F(PaymentRequestCanMakePaymentQueryPMITest,
                       NoQueryQuotaForPaymentAppsAndCardsInIncognito) {
  NavigateTo("/payment_request_payment_method_identifier_test.html");
  SetIncognito();

  CallCanMakePayment(CheckFor::BOB_PAY_AND_VISA);

  ExpectBodyContains({"false"});

  CallCanMakePayment(CheckFor::BOB_PAY_AND_BASIC_CARD);

  ExpectBodyContains({"NotAllowedError"});

  AddCreditCard(autofill::test::GetCreditCard2());  // Amex

  CallCanMakePayment(CheckFor::BOB_PAY_AND_VISA);

  ExpectBodyContains({"false"});

  CallCanMakePayment(CheckFor::BOB_PAY_AND_BASIC_CARD);

  ExpectBodyContains({"NotAllowedError"});

  AddCreditCard(autofill::test::GetCreditCard());  // Visa

  CallCanMakePayment(CheckFor::BOB_PAY_AND_VISA);

  ExpectBodyContains({"true"});

  CallCanMakePayment(CheckFor::BOB_PAY_AND_BASIC_CARD);

  ExpectBodyContains({"NotAllowedError"});
}

}  // namespace payments
