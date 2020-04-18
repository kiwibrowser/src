// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/payments/full_card_request.h"

#include "base/command_line.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "components/autofill/core/browser/autofill_test_utils.h"
#include "components/autofill/core/browser/credit_card.h"
#include "components/autofill/core/browser/payments/payments_client.h"
#include "components/autofill/core/browser/personal_data_manager.h"
#include "components/autofill/core/browser/test_autofill_client.h"
#include "components/autofill/core/browser/test_autofill_driver.h"
#include "components/autofill/core/common/autofill_pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/testing_pref_service.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace autofill {
namespace payments {

using testing::_;

// The consumer of the full card request API.
class MockResultDelegate : public FullCardRequest::ResultDelegate,
                           public base::SupportsWeakPtr<MockResultDelegate> {
 public:
  MOCK_METHOD3(OnFullCardRequestSucceeded,
               void(const payments::FullCardRequest&,
                    const CreditCard&,
                    const base::string16&));
  MOCK_METHOD0(OnFullCardRequestFailed, void());
};

// The delegate responsible for displaying the unmask prompt UI.
class MockUIDelegate : public FullCardRequest::UIDelegate,
                       public base::SupportsWeakPtr<MockUIDelegate> {
 public:
  MOCK_METHOD3(ShowUnmaskPrompt,
               void(const CreditCard&,
                    AutofillClient::UnmaskCardReason,
                    base::WeakPtr<CardUnmaskDelegate>));
  MOCK_METHOD1(OnUnmaskVerificationResult,
               void(AutofillClient::PaymentsRpcResult));
};

// The personal data manager.
class MockPersonalDataManager : public PersonalDataManager {
 public:
  MockPersonalDataManager() : PersonalDataManager("en-US") {}
  ~MockPersonalDataManager() override {}
  MOCK_METHOD1(UpdateCreditCard, void(const CreditCard& credit_card));
  MOCK_METHOD1(UpdateServerCreditCard, void(const CreditCard& credit_card));
};

// The test fixture for full card request.
class FullCardRequestTest : public testing::Test,
                            public PaymentsClientUnmaskDelegate {
 public:
  FullCardRequestTest()
      : request_context_(new net::TestURLRequestContextGetter(
            base::ThreadTaskRunnerHandle::Get())) {
    std::unique_ptr<TestingPrefServiceSimple> pref_service(
        new TestingPrefServiceSimple());
    pref_service->registry()->RegisterDoublePref(
        prefs::kAutofillBillingCustomerNumber, 0.0);
    autofill_client_.SetPrefs(std::move(pref_service));
    payments_client_ = std::make_unique<PaymentsClient>(
        request_context_.get(), autofill_client_.GetPrefs(),
        autofill_client_.GetIdentityManager(), this, nullptr);
    request_ = std::make_unique<FullCardRequest>(
        &autofill_client_, payments_client_.get(), &personal_data_);
    // Silence the warning from PaymentsClient about matching sync and Payments
    // server types.
    base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
        "sync-url", "https://google.com");
  }

  ~FullCardRequestTest() override {}

  MockPersonalDataManager* personal_data() { return &personal_data_; }

  FullCardRequest* request() { return request_.get(); }

  CardUnmaskDelegate* card_unmask_delegate() {
    return static_cast<CardUnmaskDelegate*>(request_.get());
  }

  MockResultDelegate* result_delegate() { return &result_delegate_; }

  MockUIDelegate* ui_delegate() { return &ui_delegate_; }

  // PaymentsClientUnmaskDelegate:
  void OnDidGetRealPan(AutofillClient::PaymentsRpcResult result,
                       const std::string& real_pan) override {
    request_->OnDidGetRealPan(result, real_pan);
  }

 private:
  base::MessageLoop message_loop_;
  MockPersonalDataManager personal_data_;
  MockResultDelegate result_delegate_;
  MockUIDelegate ui_delegate_;
  TestAutofillClient autofill_client_;
  scoped_refptr<net::TestURLRequestContextGetter> request_context_;
  std::unique_ptr<PaymentsClient> payments_client_;
  std::unique_ptr<FullCardRequest> request_;

  DISALLOW_COPY_AND_ASSIGN(FullCardRequestTest);
};

// Matches the |arg| credit card to the given |record_type| and |card_number|.
MATCHER_P2(CardMatches, record_type, card_number, "") {
  return arg.record_type() == record_type &&
         arg.GetRawInfo(CREDIT_CARD_NUMBER) == base::ASCIIToUTF16(card_number);
}

// Matches the |arg| credit card to the given |record_type|, card |number|,
// expiration |month|, and expiration |year|.
MATCHER_P4(CardMatches, record_type, number, month, year, "") {
  return arg.record_type() == record_type &&
         arg.GetRawInfo(CREDIT_CARD_NUMBER) == base::ASCIIToUTF16(number) &&
         arg.GetRawInfo(CREDIT_CARD_EXP_MONTH) == base::ASCIIToUTF16(month) &&
         arg.GetRawInfo(CREDIT_CARD_EXP_4_DIGIT_YEAR) ==
             base::ASCIIToUTF16(year);
}

// Verify getting the full PAN and the CVC for a masked server card.
TEST_F(FullCardRequestTest, GetFullCardPanAndCvcForMaskedServerCard) {
  EXPECT_CALL(*result_delegate(),
              OnFullCardRequestSucceeded(
                  testing::Ref(*request()),
                  CardMatches(CreditCard::FULL_SERVER_CARD, "4111"),
                  base::ASCIIToUTF16("123")));
  EXPECT_CALL(*ui_delegate(), ShowUnmaskPrompt(_, _, _));
  EXPECT_CALL(*ui_delegate(),
              OnUnmaskVerificationResult(AutofillClient::SUCCESS));

  request()->GetFullCard(
      CreditCard(CreditCard::MASKED_SERVER_CARD, "server_id"),
      AutofillClient::UNMASK_FOR_AUTOFILL, result_delegate()->AsWeakPtr(),
      ui_delegate()->AsWeakPtr());
  CardUnmaskDelegate::UnmaskResponse response;
  response.cvc = base::ASCIIToUTF16("123");
  card_unmask_delegate()->OnUnmaskResponse(response);
  OnDidGetRealPan(AutofillClient::SUCCESS, "4111");
  card_unmask_delegate()->OnUnmaskPromptClosed();
}

// Verify getting the CVC for a local card.
TEST_F(FullCardRequestTest, GetFullCardPanAndCvcForLocalCard) {
  EXPECT_CALL(
      *result_delegate(),
      OnFullCardRequestSucceeded(testing::Ref(*request()),
                                 CardMatches(CreditCard::LOCAL_CARD, "4111"),
                                 base::ASCIIToUTF16("123")));
  EXPECT_CALL(*ui_delegate(), ShowUnmaskPrompt(_, _, _));
  EXPECT_CALL(*ui_delegate(),
              OnUnmaskVerificationResult(AutofillClient::SUCCESS));

  CreditCard card;
  test::SetCreditCardInfo(&card, nullptr, "4111", "12", "2050", "1");
  request()->GetFullCard(card, AutofillClient::UNMASK_FOR_AUTOFILL,
                         result_delegate()->AsWeakPtr(),
                         ui_delegate()->AsWeakPtr());
  CardUnmaskDelegate::UnmaskResponse response;
  response.cvc = base::ASCIIToUTF16("123");
  card_unmask_delegate()->OnUnmaskResponse(response);
  card_unmask_delegate()->OnUnmaskPromptClosed();
}

// Verify getting the CVC for an unmasked server card.
TEST_F(FullCardRequestTest, GetFullCardPanAndCvcForFullServerCard) {
  EXPECT_CALL(*result_delegate(),
              OnFullCardRequestSucceeded(
                  testing::Ref(*request()),
                  CardMatches(CreditCard::FULL_SERVER_CARD, "4111"),
                  base::ASCIIToUTF16("123")));
  EXPECT_CALL(*ui_delegate(), ShowUnmaskPrompt(_, _, _));
  EXPECT_CALL(*ui_delegate(),
              OnUnmaskVerificationResult(AutofillClient::SUCCESS));

  CreditCard full_server_card(CreditCard::FULL_SERVER_CARD, "server_id");
  test::SetCreditCardInfo(&full_server_card, nullptr, "4111", "12", "2050",
                          "1");
  request()->GetFullCard(full_server_card, AutofillClient::UNMASK_FOR_AUTOFILL,
                         result_delegate()->AsWeakPtr(),
                         ui_delegate()->AsWeakPtr());
  CardUnmaskDelegate::UnmaskResponse response;
  response.cvc = base::ASCIIToUTF16("123");
  card_unmask_delegate()->OnUnmaskResponse(response);
  card_unmask_delegate()->OnUnmaskPromptClosed();
}

// Verify getting the CVC for an unmasked server card with EXPIRED server
// status.
TEST_F(FullCardRequestTest,
       GetFullCardPanAndCvcForFullServerCardInExpiredStatus) {
  EXPECT_CALL(*result_delegate(), OnFullCardRequestSucceeded(
                                      testing::Ref(*request()),
                                      CardMatches(CreditCard::FULL_SERVER_CARD,
                                                  "4111", "12", "2051"),
                                      base::ASCIIToUTF16("123")));
  EXPECT_CALL(*ui_delegate(), ShowUnmaskPrompt(_, _, _));
  EXPECT_CALL(*personal_data(), UpdateServerCreditCard(_)).Times(0);
  EXPECT_CALL(*ui_delegate(),
              OnUnmaskVerificationResult(AutofillClient::SUCCESS));

  CreditCard full_server_card(CreditCard::FULL_SERVER_CARD, "server_id");
  test::SetCreditCardInfo(&full_server_card, nullptr, "4111", "12", "2050",
                          "1");
  full_server_card.SetServerStatus(CreditCard::EXPIRED);
  request()->GetFullCard(full_server_card, AutofillClient::UNMASK_FOR_AUTOFILL,
                         result_delegate()->AsWeakPtr(),
                         ui_delegate()->AsWeakPtr());
  CardUnmaskDelegate::UnmaskResponse response;
  response.cvc = base::ASCIIToUTF16("123");
  response.exp_year = base::ASCIIToUTF16("2051");
  response.exp_month = base::ASCIIToUTF16("12");
  card_unmask_delegate()->OnUnmaskResponse(response);
  OnDidGetRealPan(AutofillClient::SUCCESS, "4111");
  card_unmask_delegate()->OnUnmaskPromptClosed();
}

// Verify getting the CVC for an unmasked server card with OK status, but
// expiration date in the past.
TEST_F(FullCardRequestTest, GetFullCardPanAndCvcForExpiredFullServerCard) {
  EXPECT_CALL(*result_delegate(), OnFullCardRequestSucceeded(
                                      testing::Ref(*request()),
                                      CardMatches(CreditCard::FULL_SERVER_CARD,
                                                  "4111", "12", "2051"),
                                      base::ASCIIToUTF16("123")));
  EXPECT_CALL(*ui_delegate(), ShowUnmaskPrompt(_, _, _));
  EXPECT_CALL(*personal_data(), UpdateServerCreditCard(_)).Times(0);
  EXPECT_CALL(*ui_delegate(),
              OnUnmaskVerificationResult(AutofillClient::SUCCESS));

  base::Time::Exploded today;
  base::Time::Now().LocalExplode(&today);
  CreditCard full_server_card(CreditCard::FULL_SERVER_CARD, "server_id");
  test::SetCreditCardInfo(&full_server_card, nullptr, "4111", "12",
                          base::StringPrintf("%d", today.year - 1).c_str(),
                          "1");
  full_server_card.SetServerStatus(CreditCard::OK);
  request()->GetFullCard(full_server_card, AutofillClient::UNMASK_FOR_AUTOFILL,
                         result_delegate()->AsWeakPtr(),
                         ui_delegate()->AsWeakPtr());
  CardUnmaskDelegate::UnmaskResponse response;
  response.cvc = base::ASCIIToUTF16("123");
  response.exp_year = base::ASCIIToUTF16("2051");
  response.exp_month = base::ASCIIToUTF16("12");
  card_unmask_delegate()->OnUnmaskResponse(response);
  OnDidGetRealPan(AutofillClient::SUCCESS, "4111");
  card_unmask_delegate()->OnUnmaskPromptClosed();
}

// Only one request at a time should be allowed.
TEST_F(FullCardRequestTest, OneRequestAtATime) {
  EXPECT_CALL(*result_delegate(), OnFullCardRequestFailed());
  EXPECT_CALL(*ui_delegate(), ShowUnmaskPrompt(_, _, _));
  EXPECT_CALL(*ui_delegate(), OnUnmaskVerificationResult(_)).Times(0);

  request()->GetFullCard(
      CreditCard(CreditCard::MASKED_SERVER_CARD, "server_id_1"),
      AutofillClient::UNMASK_FOR_AUTOFILL, result_delegate()->AsWeakPtr(),
      ui_delegate()->AsWeakPtr());
  request()->GetFullCard(
      CreditCard(CreditCard::MASKED_SERVER_CARD, "server_id_2"),
      AutofillClient::UNMASK_FOR_PAYMENT_REQUEST,
      result_delegate()->AsWeakPtr(), ui_delegate()->AsWeakPtr());
}

// After the first request completes, it's OK to start the second request.
TEST_F(FullCardRequestTest, SecondRequestOkAfterFirstFinished) {
  EXPECT_CALL(*result_delegate(), OnFullCardRequestFailed()).Times(0);
  EXPECT_CALL(
      *result_delegate(),
      OnFullCardRequestSucceeded(testing::Ref(*request()),
                                 CardMatches(CreditCard::LOCAL_CARD, "4111"),
                                 base::ASCIIToUTF16("123")))
      .Times(2);
  EXPECT_CALL(*ui_delegate(), ShowUnmaskPrompt(_, _, _)).Times(2);
  EXPECT_CALL(*ui_delegate(),
              OnUnmaskVerificationResult(AutofillClient::SUCCESS))
      .Times(2);

  CreditCard card;
  test::SetCreditCardInfo(&card, nullptr, "4111", "12", "2050", "1");
  request()->GetFullCard(card, AutofillClient::UNMASK_FOR_AUTOFILL,
                         result_delegate()->AsWeakPtr(),
                         ui_delegate()->AsWeakPtr());
  CardUnmaskDelegate::UnmaskResponse response;
  response.cvc = base::ASCIIToUTF16("123");
  card_unmask_delegate()->OnUnmaskResponse(response);
  card_unmask_delegate()->OnUnmaskPromptClosed();

  request()->GetFullCard(card, AutofillClient::UNMASK_FOR_AUTOFILL,
                         result_delegate()->AsWeakPtr(),
                         ui_delegate()->AsWeakPtr());
  card_unmask_delegate()->OnUnmaskResponse(response);
  card_unmask_delegate()->OnUnmaskPromptClosed();
}

// If the user cancels the CVC prompt,
// FullCardRequest::Delegate::OnFullCardRequestFailed() should be invoked.
TEST_F(FullCardRequestTest, ClosePromptWithoutUserInput) {
  EXPECT_CALL(*result_delegate(), OnFullCardRequestFailed());
  EXPECT_CALL(*ui_delegate(), ShowUnmaskPrompt(_, _, _));
  EXPECT_CALL(*ui_delegate(), OnUnmaskVerificationResult(_)).Times(0);

  request()->GetFullCard(
      CreditCard(CreditCard::MASKED_SERVER_CARD, "server_id"),
      AutofillClient::UNMASK_FOR_AUTOFILL, result_delegate()->AsWeakPtr(),
      ui_delegate()->AsWeakPtr());
  card_unmask_delegate()->OnUnmaskPromptClosed();
}

// If the server provides an empty PAN with PERMANENT_FAILURE error,
// FullCardRequest::Delegate::OnFullCardRequestFailed() should be invoked.
TEST_F(FullCardRequestTest, PermanentFailure) {
  EXPECT_CALL(*result_delegate(), OnFullCardRequestFailed());
  EXPECT_CALL(*ui_delegate(), ShowUnmaskPrompt(_, _, _));
  EXPECT_CALL(*ui_delegate(),
              OnUnmaskVerificationResult(AutofillClient::PERMANENT_FAILURE));

  request()->GetFullCard(
      CreditCard(CreditCard::MASKED_SERVER_CARD, "server_id"),
      AutofillClient::UNMASK_FOR_AUTOFILL, result_delegate()->AsWeakPtr(),
      ui_delegate()->AsWeakPtr());
  CardUnmaskDelegate::UnmaskResponse response;
  response.cvc = base::ASCIIToUTF16("123");
  card_unmask_delegate()->OnUnmaskResponse(response);
  OnDidGetRealPan(AutofillClient::PERMANENT_FAILURE, "");
  card_unmask_delegate()->OnUnmaskPromptClosed();
}

// If the server provides an empty PAN with NETWORK_ERROR error,
// FullCardRequest::Delegate::OnFullCardRequestFailed() should be invoked.
TEST_F(FullCardRequestTest, NetworkError) {
  EXPECT_CALL(*result_delegate(), OnFullCardRequestFailed());
  EXPECT_CALL(*ui_delegate(), ShowUnmaskPrompt(_, _, _));
  EXPECT_CALL(*ui_delegate(),
              OnUnmaskVerificationResult(AutofillClient::NETWORK_ERROR));

  request()->GetFullCard(
      CreditCard(CreditCard::MASKED_SERVER_CARD, "server_id"),
      AutofillClient::UNMASK_FOR_AUTOFILL, result_delegate()->AsWeakPtr(),
      ui_delegate()->AsWeakPtr());
  CardUnmaskDelegate::UnmaskResponse response;
  response.cvc = base::ASCIIToUTF16("123");
  card_unmask_delegate()->OnUnmaskResponse(response);
  OnDidGetRealPan(AutofillClient::NETWORK_ERROR, "");
  card_unmask_delegate()->OnUnmaskPromptClosed();
}

// If the server provides an empty PAN with TRY_AGAIN_FAILURE, the user can
// manually cancel out of the dialog.
TEST_F(FullCardRequestTest, TryAgainFailureGiveUp) {
  EXPECT_CALL(*result_delegate(), OnFullCardRequestFailed());
  EXPECT_CALL(*ui_delegate(), ShowUnmaskPrompt(_, _, _));
  EXPECT_CALL(*ui_delegate(),
              OnUnmaskVerificationResult(AutofillClient::TRY_AGAIN_FAILURE));

  request()->GetFullCard(
      CreditCard(CreditCard::MASKED_SERVER_CARD, "server_id"),
      AutofillClient::UNMASK_FOR_AUTOFILL, result_delegate()->AsWeakPtr(),
      ui_delegate()->AsWeakPtr());
  CardUnmaskDelegate::UnmaskResponse response;
  response.cvc = base::ASCIIToUTF16("123");
  card_unmask_delegate()->OnUnmaskResponse(response);
  OnDidGetRealPan(AutofillClient::TRY_AGAIN_FAILURE, "");
  card_unmask_delegate()->OnUnmaskPromptClosed();
}

// If the server provides an empty PAN with TRY_AGAIN_FAILURE, the user can
// correct their mistake and resubmit.
TEST_F(FullCardRequestTest, TryAgainFailureRetry) {
  EXPECT_CALL(*result_delegate(), OnFullCardRequestFailed()).Times(0);
  EXPECT_CALL(*result_delegate(),
              OnFullCardRequestSucceeded(
                  testing::Ref(*request()),
                  CardMatches(CreditCard::FULL_SERVER_CARD, "4111"),
                  base::ASCIIToUTF16("123")));
  EXPECT_CALL(*ui_delegate(), ShowUnmaskPrompt(_, _, _));
  EXPECT_CALL(*ui_delegate(),
              OnUnmaskVerificationResult(AutofillClient::TRY_AGAIN_FAILURE));
  EXPECT_CALL(*ui_delegate(),
              OnUnmaskVerificationResult(AutofillClient::SUCCESS));

  request()->GetFullCard(
      CreditCard(CreditCard::MASKED_SERVER_CARD, "server_id"),
      AutofillClient::UNMASK_FOR_AUTOFILL, result_delegate()->AsWeakPtr(),
      ui_delegate()->AsWeakPtr());
  CardUnmaskDelegate::UnmaskResponse response;
  response.cvc = base::ASCIIToUTF16("789");
  card_unmask_delegate()->OnUnmaskResponse(response);
  OnDidGetRealPan(AutofillClient::TRY_AGAIN_FAILURE, "");
  response.cvc = base::ASCIIToUTF16("123");
  card_unmask_delegate()->OnUnmaskResponse(response);
  OnDidGetRealPan(AutofillClient::SUCCESS, "4111");
  card_unmask_delegate()->OnUnmaskPromptClosed();
}

// Verify updating expiration date for a masked server card.
TEST_F(FullCardRequestTest, UpdateExpDateForMaskedServerCard) {
  EXPECT_CALL(*result_delegate(), OnFullCardRequestSucceeded(
                                      testing::Ref(*request()),
                                      CardMatches(CreditCard::FULL_SERVER_CARD,
                                                  "4111", "12", "2050"),
                                      base::ASCIIToUTF16("123")));
  EXPECT_CALL(*ui_delegate(), ShowUnmaskPrompt(_, _, _));
  EXPECT_CALL(*ui_delegate(),
              OnUnmaskVerificationResult(AutofillClient::SUCCESS));

  request()->GetFullCard(
      CreditCard(CreditCard::MASKED_SERVER_CARD, "server_id"),
      AutofillClient::UNMASK_FOR_AUTOFILL, result_delegate()->AsWeakPtr(),
      ui_delegate()->AsWeakPtr());
  CardUnmaskDelegate::UnmaskResponse response;
  response.cvc = base::ASCIIToUTF16("123");
  response.exp_month = base::ASCIIToUTF16("12");
  response.exp_year = base::ASCIIToUTF16("2050");
  card_unmask_delegate()->OnUnmaskResponse(response);
  OnDidGetRealPan(AutofillClient::SUCCESS, "4111");
  card_unmask_delegate()->OnUnmaskPromptClosed();
}

// Verify updating expiration date for an unmasked server card.
TEST_F(FullCardRequestTest, UpdateExpDateForFullServerCard) {
  EXPECT_CALL(*result_delegate(), OnFullCardRequestSucceeded(
                                      testing::Ref(*request()),
                                      CardMatches(CreditCard::FULL_SERVER_CARD,
                                                  "4111", "12", "2050"),
                                      base::ASCIIToUTF16("123")));
  EXPECT_CALL(*ui_delegate(), ShowUnmaskPrompt(_, _, _));
  EXPECT_CALL(*ui_delegate(),
              OnUnmaskVerificationResult(AutofillClient::SUCCESS));

  CreditCard full_server_card(CreditCard::FULL_SERVER_CARD, "server_id");
  test::SetCreditCardInfo(&full_server_card, nullptr, "4111", "10", "2000",
                          "1");
  request()->GetFullCard(full_server_card, AutofillClient::UNMASK_FOR_AUTOFILL,
                         result_delegate()->AsWeakPtr(),
                         ui_delegate()->AsWeakPtr());
  CardUnmaskDelegate::UnmaskResponse response;
  response.cvc = base::ASCIIToUTF16("123");
  response.exp_month = base::ASCIIToUTF16("12");
  response.exp_year = base::ASCIIToUTF16("2050");
  card_unmask_delegate()->OnUnmaskResponse(response);
  OnDidGetRealPan(AutofillClient::SUCCESS, "4111");
  card_unmask_delegate()->OnUnmaskPromptClosed();
}

// Verify updating expiration date for a local card.
TEST_F(FullCardRequestTest, UpdateExpDateForLocalCard) {
  EXPECT_CALL(*result_delegate(),
              OnFullCardRequestSucceeded(
                  testing::Ref(*request()),
                  CardMatches(CreditCard::LOCAL_CARD, "4111", "12", "2051"),
                  base::ASCIIToUTF16("123")));
  EXPECT_CALL(*ui_delegate(), ShowUnmaskPrompt(_, _, _));
  EXPECT_CALL(*personal_data(),
              UpdateCreditCard(
                  CardMatches(CreditCard::LOCAL_CARD, "4111", "12", "2051")));
  EXPECT_CALL(*ui_delegate(),
              OnUnmaskVerificationResult(AutofillClient::SUCCESS));

  base::Time::Exploded today;
  base::Time::Now().LocalExplode(&today);
  CreditCard card;
  test::SetCreditCardInfo(&card, nullptr, "4111", "10",
                          base::StringPrintf("%d", today.year - 1).c_str(),
                          "1");
  request()->GetFullCard(card, AutofillClient::UNMASK_FOR_AUTOFILL,
                         result_delegate()->AsWeakPtr(),
                         ui_delegate()->AsWeakPtr());
  CardUnmaskDelegate::UnmaskResponse response;
  response.cvc = base::ASCIIToUTF16("123");
  response.exp_month = base::ASCIIToUTF16("12");
  response.exp_year = base::ASCIIToUTF16("2051");
  card_unmask_delegate()->OnUnmaskResponse(response);
  card_unmask_delegate()->OnUnmaskPromptClosed();
}

// Verify saving full PAN on disk.
TEST_F(FullCardRequestTest, SaveRealPan) {
  EXPECT_CALL(*result_delegate(), OnFullCardRequestSucceeded(
                                      testing::Ref(*request()),
                                      CardMatches(CreditCard::FULL_SERVER_CARD,
                                                  "4111", "12", "2050"),
                                      base::ASCIIToUTF16("123")));
  EXPECT_CALL(*ui_delegate(), ShowUnmaskPrompt(_, _, _));
  EXPECT_CALL(*personal_data(),
              UpdateServerCreditCard(CardMatches(CreditCard::FULL_SERVER_CARD,
                                                 "4111", "12", "2050")));
  EXPECT_CALL(*ui_delegate(),
              OnUnmaskVerificationResult(AutofillClient::SUCCESS));

  request()->GetFullCard(
      CreditCard(CreditCard::MASKED_SERVER_CARD, "server_id"),
      AutofillClient::UNMASK_FOR_AUTOFILL, result_delegate()->AsWeakPtr(),
      ui_delegate()->AsWeakPtr());
  CardUnmaskDelegate::UnmaskResponse response;
  response.cvc = base::ASCIIToUTF16("123");
  response.exp_month = base::ASCIIToUTF16("12");
  response.exp_year = base::ASCIIToUTF16("2050");
  response.should_store_pan = true;
  card_unmask_delegate()->OnUnmaskResponse(response);
  OnDidGetRealPan(AutofillClient::SUCCESS, "4111");
  card_unmask_delegate()->OnUnmaskPromptClosed();
}

// Verify getting full PAN and CVC for PaymentRequest.
TEST_F(FullCardRequestTest, UnmaskForPaymentRequest) {
  EXPECT_CALL(*result_delegate(),
              OnFullCardRequestSucceeded(
                  testing::Ref(*request()),
                  CardMatches(CreditCard::FULL_SERVER_CARD, "4111"),
                  base::ASCIIToUTF16("123")));
  EXPECT_CALL(*ui_delegate(), ShowUnmaskPrompt(_, _, _));
  EXPECT_CALL(*ui_delegate(),
              OnUnmaskVerificationResult(AutofillClient::SUCCESS));

  request()->GetFullCard(
      CreditCard(CreditCard::MASKED_SERVER_CARD, "server_id"),
      AutofillClient::UNMASK_FOR_PAYMENT_REQUEST,
      result_delegate()->AsWeakPtr(), ui_delegate()->AsWeakPtr());
  CardUnmaskDelegate::UnmaskResponse response;
  response.cvc = base::ASCIIToUTF16("123");
  card_unmask_delegate()->OnUnmaskResponse(response);
  OnDidGetRealPan(AutofillClient::SUCCESS, "4111");
  card_unmask_delegate()->OnUnmaskPromptClosed();
}

// Verify that FullCardRequest::IsGettingFullCard() is true until the server
// returns the full PAN for a masked card.
TEST_F(FullCardRequestTest, IsGettingFullCardForMaskedServerCard) {
  EXPECT_CALL(*result_delegate(),
              OnFullCardRequestSucceeded(
                  testing::Ref(*request()),
                  CardMatches(CreditCard::FULL_SERVER_CARD, "4111"),
                  base::ASCIIToUTF16("123")));
  EXPECT_CALL(*ui_delegate(), ShowUnmaskPrompt(_, _, _));
  EXPECT_CALL(*ui_delegate(),
              OnUnmaskVerificationResult(AutofillClient::SUCCESS));

  EXPECT_FALSE(request()->IsGettingFullCard());

  request()->GetFullCard(
      CreditCard(CreditCard::MASKED_SERVER_CARD, "server_id"),
      AutofillClient::UNMASK_FOR_AUTOFILL, result_delegate()->AsWeakPtr(),
      ui_delegate()->AsWeakPtr());

  EXPECT_TRUE(request()->IsGettingFullCard());

  CardUnmaskDelegate::UnmaskResponse response;
  response.cvc = base::ASCIIToUTF16("123");
  card_unmask_delegate()->OnUnmaskResponse(response);

  EXPECT_TRUE(request()->IsGettingFullCard());

  OnDidGetRealPan(AutofillClient::SUCCESS, "4111");

  EXPECT_FALSE(request()->IsGettingFullCard());

  card_unmask_delegate()->OnUnmaskPromptClosed();

  EXPECT_FALSE(request()->IsGettingFullCard());
}

// Verify that FullCardRequest::IsGettingFullCard() is true until the user types
// in the CVC for a card that is not masked.
TEST_F(FullCardRequestTest, IsGettingFullCardForLocalCard) {
  EXPECT_CALL(
      *result_delegate(),
      OnFullCardRequestSucceeded(testing::Ref(*request()),
                                 CardMatches(CreditCard::LOCAL_CARD, "4111"),
                                 base::ASCIIToUTF16("123")));
  EXPECT_CALL(*ui_delegate(), ShowUnmaskPrompt(_, _, _));
  EXPECT_CALL(*ui_delegate(),
              OnUnmaskVerificationResult(AutofillClient::SUCCESS));

  EXPECT_FALSE(request()->IsGettingFullCard());

  CreditCard card;
  test::SetCreditCardInfo(&card, nullptr, "4111", "12", "2050", "1");
  request()->GetFullCard(card, AutofillClient::UNMASK_FOR_AUTOFILL,
                         result_delegate()->AsWeakPtr(),
                         ui_delegate()->AsWeakPtr());

  EXPECT_TRUE(request()->IsGettingFullCard());

  CardUnmaskDelegate::UnmaskResponse response;
  response.cvc = base::ASCIIToUTF16("123");
  card_unmask_delegate()->OnUnmaskResponse(response);

  EXPECT_FALSE(request()->IsGettingFullCard());

  card_unmask_delegate()->OnUnmaskPromptClosed();

  EXPECT_FALSE(request()->IsGettingFullCard());
}

}  // namespace payments
}  // namespace autofill
