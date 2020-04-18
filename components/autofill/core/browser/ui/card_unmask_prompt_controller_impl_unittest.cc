// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/ui/card_unmask_prompt_controller_impl.h"

#include <stddef.h>

#include <memory>

#include "base/bind.h"
#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/histogram_tester.h"
#include "components/autofill/core/browser/autofill_client.h"
#include "components/autofill/core/browser/autofill_metrics.h"
#include "components/autofill/core/browser/autofill_test_utils.h"
#include "components/autofill/core/browser/credit_card.h"
#include "components/autofill/core/browser/ui/card_unmask_prompt_view.h"
#include "components/autofill/core/common/autofill_pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace autofill {

using base::ASCIIToUTF16;

class TestCardUnmaskDelegate : public CardUnmaskDelegate {
 public:
  TestCardUnmaskDelegate() : weak_factory_(this) {}

  virtual ~TestCardUnmaskDelegate() {}

  // CardUnmaskDelegate implementation.
  void OnUnmaskResponse(const UnmaskResponse& response) override {
    response_ = response;
  }
  void OnUnmaskPromptClosed() override {}

  const UnmaskResponse& response() { return response_; }

  base::WeakPtr<TestCardUnmaskDelegate> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

 private:
  UnmaskResponse response_;
  base::WeakPtrFactory<TestCardUnmaskDelegate> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(TestCardUnmaskDelegate);
};

class TestCardUnmaskPromptView : public CardUnmaskPromptView {
 public:
  void Show() override {}
  void ControllerGone() override {}
  void DisableAndWaitForVerification() override {}
  void GotVerificationResult(const base::string16& error_message,
                             bool allow_retry) override {}
};

class TestCardUnmaskPromptController : public CardUnmaskPromptControllerImpl {
 public:
  explicit TestCardUnmaskPromptController(
      TestingPrefServiceSimple* pref_service)
      : CardUnmaskPromptControllerImpl(pref_service, false),
        can_store_locally_(true),
        weak_factory_(this) {}

  bool CanStoreLocally() const override { return can_store_locally_; }

  void set_can_store_locally(bool can) { can_store_locally_ = can; }

  void SetCreditCardForTesting(CreditCard card) {
    CardUnmaskPromptControllerImpl::SetCreditCardForTesting(card);
  }

  base::WeakPtr<TestCardUnmaskPromptController> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

 private:
  bool can_store_locally_;
  base::WeakPtrFactory<TestCardUnmaskPromptController> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(TestCardUnmaskPromptController);
};

class CardUnmaskPromptControllerImplGenericTest {
 public:
  CardUnmaskPromptControllerImplGenericTest() {}

  void ShowPrompt() {
    controller_->ShowPrompt(test_unmask_prompt_view_.get(),
                            test::GetMaskedServerCard(),
                            AutofillClient::UNMASK_FOR_AUTOFILL,
                            delegate_->GetWeakPtr());
  }

  void ShowPromptAmex() {
    controller_->ShowPrompt(test_unmask_prompt_view_.get(),
                            test::GetMaskedServerCardAmex(),
                            AutofillClient::UNMASK_FOR_AUTOFILL,
                            delegate_->GetWeakPtr());
  }

  void ShowPromptAndSimulateResponse(bool should_store_pan) {
    ShowPrompt();
    controller_->OnUnmaskResponse(ASCIIToUTF16("444"),
                                  ASCIIToUTF16("01"),
                                  ASCIIToUTF16("2050"),
                                  should_store_pan);
    EXPECT_EQ(
        should_store_pan,
        pref_service_->GetBoolean(
            prefs::kAutofillWalletImportStorageCheckboxState));
  }

 protected:
  void SetImportCheckboxState(bool value) {
    pref_service_->SetBoolean(
        prefs::kAutofillWalletImportStorageCheckboxState, value);
  }

  std::unique_ptr<TestCardUnmaskPromptView> test_unmask_prompt_view_;
  std::unique_ptr<TestingPrefServiceSimple> pref_service_;
  std::unique_ptr<TestCardUnmaskPromptController> controller_;
  std::unique_ptr<TestCardUnmaskDelegate> delegate_;

 private:
  DISALLOW_COPY_AND_ASSIGN(CardUnmaskPromptControllerImplGenericTest);
};

class CardUnmaskPromptControllerImplTest
    : public CardUnmaskPromptControllerImplGenericTest,
      public testing::Test {
 public:
  CardUnmaskPromptControllerImplTest() {}
  ~CardUnmaskPromptControllerImplTest() override {}

  void SetUp() override {
    test_unmask_prompt_view_.reset(new TestCardUnmaskPromptView());
    pref_service_.reset(new TestingPrefServiceSimple());
    controller_.reset(new TestCardUnmaskPromptController(pref_service_.get()));
    delegate_.reset(new TestCardUnmaskDelegate());
    pref_service_->registry()->RegisterBooleanPref(
        prefs::kAutofillWalletImportStorageCheckboxState, false);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(CardUnmaskPromptControllerImplTest);
};

TEST_F(CardUnmaskPromptControllerImplTest, LogShown) {
  base::HistogramTester histogram_tester;
  ShowPrompt();

  histogram_tester.ExpectUniqueSample(
      "Autofill.UnmaskPrompt.Events",
      AutofillMetrics::UNMASK_PROMPT_SHOWN, 1);
}

TEST_F(CardUnmaskPromptControllerImplTest, LogClosedNoAttempts) {
  ShowPrompt();
  base::HistogramTester histogram_tester;
  controller_->OnUnmaskDialogClosed();

  histogram_tester.ExpectBucketCount(
      "Autofill.UnmaskPrompt.Events",
      AutofillMetrics::UNMASK_PROMPT_CLOSED_NO_ATTEMPTS, 1);
}

TEST_F(CardUnmaskPromptControllerImplTest, LogClosedAbandonUnmasking) {
  ShowPromptAndSimulateResponse(false);
  base::HistogramTester histogram_tester;

  controller_->OnUnmaskDialogClosed();

  histogram_tester.ExpectBucketCount(
      "Autofill.UnmaskPrompt.Events",
      AutofillMetrics::UNMASK_PROMPT_CLOSED_ABANDON_UNMASKING, 1);
}

TEST_F(CardUnmaskPromptControllerImplTest, LogClosedFailedToUnmaskRetriable) {
  ShowPromptAndSimulateResponse(false);
  controller_->OnVerificationResult(AutofillClient::TRY_AGAIN_FAILURE);
  base::HistogramTester histogram_tester;

  controller_->OnUnmaskDialogClosed();

  histogram_tester.ExpectBucketCount(
      "Autofill.UnmaskPrompt.Events",
      AutofillMetrics
          ::UNMASK_PROMPT_CLOSED_FAILED_TO_UNMASK_RETRIABLE_FAILURE,
      1);
}

TEST_F(CardUnmaskPromptControllerImplTest,
       LogClosedFailedToUnmaskNonRetriable) {
  ShowPromptAndSimulateResponse(false);
  controller_->OnVerificationResult(AutofillClient::PERMANENT_FAILURE);
  base::HistogramTester histogram_tester;

  controller_->OnUnmaskDialogClosed();

  histogram_tester.ExpectBucketCount(
      "Autofill.UnmaskPrompt.Events",
      AutofillMetrics
          ::UNMASK_PROMPT_CLOSED_FAILED_TO_UNMASK_NON_RETRIABLE_FAILURE,
      1);
}

TEST_F(CardUnmaskPromptControllerImplTest, LogUnmaskedCardFirstAttempt) {
  ShowPromptAndSimulateResponse(false);
  base::HistogramTester histogram_tester;

  controller_->OnVerificationResult(AutofillClient::SUCCESS);
  controller_->OnUnmaskDialogClosed();

  histogram_tester.ExpectBucketCount(
      "Autofill.UnmaskPrompt.Events",
      AutofillMetrics::UNMASK_PROMPT_UNMASKED_CARD_FIRST_ATTEMPT, 1);
}

TEST_F(CardUnmaskPromptControllerImplTest, LogUnmaskedCardAfterFailure) {
  ShowPromptAndSimulateResponse(false);
  controller_->OnVerificationResult(AutofillClient::TRY_AGAIN_FAILURE);
  controller_->OnUnmaskResponse(ASCIIToUTF16("444"),
                                ASCIIToUTF16("01"),
                                ASCIIToUTF16("2050"),
                                false /* should_store_pan */);
  base::HistogramTester histogram_tester;

  controller_->OnVerificationResult(AutofillClient::SUCCESS);
  controller_->OnUnmaskDialogClosed();

  histogram_tester.ExpectBucketCount(
      "Autofill.UnmaskPrompt.Events",
      AutofillMetrics::UNMASK_PROMPT_UNMASKED_CARD_AFTER_FAILED_ATTEMPTS, 1);
}

TEST_F(CardUnmaskPromptControllerImplTest, LogSavedCardLocally) {
  ShowPromptAndSimulateResponse(true);
  base::HistogramTester histogram_tester;

  controller_->OnVerificationResult(AutofillClient::SUCCESS);
  controller_->OnUnmaskDialogClosed();

  histogram_tester.ExpectBucketCount(
      "Autofill.UnmaskPrompt.Events",
      AutofillMetrics::UNMASK_PROMPT_SAVED_CARD_LOCALLY, 1);
}

TEST_F(CardUnmaskPromptControllerImplTest, LogDidOptIn) {
  SetImportCheckboxState(false);
  ShowPromptAndSimulateResponse(true);
  base::HistogramTester histogram_tester;
  controller_->OnUnmaskDialogClosed();

  histogram_tester.ExpectBucketCount(
      "Autofill.UnmaskPrompt.Events",
      AutofillMetrics::UNMASK_PROMPT_LOCAL_SAVE_DID_OPT_IN, 1);
}

TEST_F(CardUnmaskPromptControllerImplTest, LogDidNotOptIn) {
  SetImportCheckboxState(false);
  ShowPromptAndSimulateResponse(false);
  base::HistogramTester histogram_tester;
  controller_->OnUnmaskDialogClosed();

  histogram_tester.ExpectBucketCount(
      "Autofill.UnmaskPrompt.Events",
      AutofillMetrics::UNMASK_PROMPT_LOCAL_SAVE_DID_NOT_OPT_IN, 1);
}

TEST_F(CardUnmaskPromptControllerImplTest, LogDidOptOut) {
  SetImportCheckboxState(true);
  ShowPromptAndSimulateResponse(false);
  base::HistogramTester histogram_tester;
  controller_->OnUnmaskDialogClosed();

  histogram_tester.ExpectBucketCount(
      "Autofill.UnmaskPrompt.Events",
      AutofillMetrics::UNMASK_PROMPT_LOCAL_SAVE_DID_OPT_OUT, 1);
}

TEST_F(CardUnmaskPromptControllerImplTest, LogDidNotOptOut) {
  SetImportCheckboxState(true);
  ShowPromptAndSimulateResponse(true);
  base::HistogramTester histogram_tester;
  controller_->OnUnmaskDialogClosed();

  histogram_tester.ExpectBucketCount(
      "Autofill.UnmaskPrompt.Events",
      AutofillMetrics::UNMASK_PROMPT_LOCAL_SAVE_DID_NOT_OPT_OUT, 1);
}

TEST_F(CardUnmaskPromptControllerImplTest, DontLogForHiddenCheckbox) {
  controller_->set_can_store_locally(false);
  ShowPromptAndSimulateResponse(false);
  base::HistogramTester histogram_tester;
  controller_->OnUnmaskDialogClosed();

  histogram_tester.ExpectBucketCount(
      "Autofill.UnmaskPrompt.Events",
      AutofillMetrics::UNMASK_PROMPT_LOCAL_SAVE_DID_OPT_IN, 0);
  histogram_tester.ExpectBucketCount(
      "Autofill.UnmaskPrompt.Events",
      AutofillMetrics::UNMASK_PROMPT_LOCAL_SAVE_DID_NOT_OPT_IN, 0);
  histogram_tester.ExpectBucketCount(
      "Autofill.UnmaskPrompt.Events",
      AutofillMetrics::UNMASK_PROMPT_LOCAL_SAVE_DID_OPT_OUT, 0);
  histogram_tester.ExpectBucketCount(
      "Autofill.UnmaskPrompt.Events",
      AutofillMetrics::UNMASK_PROMPT_LOCAL_SAVE_DID_NOT_OPT_OUT, 0);
}

TEST_F(CardUnmaskPromptControllerImplTest, LogDurationNoAttempts) {
  ShowPrompt();
  base::HistogramTester histogram_tester;

  controller_->OnUnmaskDialogClosed();

  histogram_tester.ExpectTotalCount("Autofill.UnmaskPrompt.Duration", 1);
  histogram_tester.ExpectTotalCount("Autofill.UnmaskPrompt.Duration.NoAttempts",
                                    1);
}

TEST_F(CardUnmaskPromptControllerImplTest, LogDurationAbandonUnmasking) {
  ShowPromptAndSimulateResponse(false);
  base::HistogramTester histogram_tester;

  controller_->OnUnmaskDialogClosed();

  histogram_tester.ExpectTotalCount("Autofill.UnmaskPrompt.Duration", 1);
  histogram_tester.ExpectTotalCount(
      "Autofill.UnmaskPrompt.Duration.AbandonUnmasking", 1);
}

TEST_F(CardUnmaskPromptControllerImplTest, LogDurationFailedToUnmaskRetriable) {
  ShowPromptAndSimulateResponse(false);
  controller_->OnVerificationResult(AutofillClient::TRY_AGAIN_FAILURE);
  base::HistogramTester histogram_tester;

  controller_->OnUnmaskDialogClosed();

  histogram_tester.ExpectTotalCount("Autofill.UnmaskPrompt.Duration", 1);
  histogram_tester.ExpectTotalCount("Autofill.UnmaskPrompt.Duration.Failure",
                                    1);
}

TEST_F(CardUnmaskPromptControllerImplTest,
       LogDurationFailedToUnmaskNonRetriable) {
  ShowPromptAndSimulateResponse(false);
  controller_->OnVerificationResult(AutofillClient::PERMANENT_FAILURE);
  base::HistogramTester histogram_tester;

  controller_->OnUnmaskDialogClosed();

  histogram_tester.ExpectTotalCount("Autofill.UnmaskPrompt.Duration", 1);
  histogram_tester.ExpectTotalCount("Autofill.UnmaskPrompt.Duration.Failure",
                                    1);
}

TEST_F(CardUnmaskPromptControllerImplTest, LogDurationCardFirstAttempt) {
  ShowPromptAndSimulateResponse(false);
  base::HistogramTester histogram_tester;

  controller_->OnVerificationResult(AutofillClient::SUCCESS);
  controller_->OnUnmaskDialogClosed();

  histogram_tester.ExpectTotalCount("Autofill.UnmaskPrompt.Duration", 1);
  histogram_tester.ExpectTotalCount("Autofill.UnmaskPrompt.Duration.Success",
                                    1);
}

TEST_F(CardUnmaskPromptControllerImplTest,
       LogDurationUnmaskedCardAfterFailure) {
  ShowPromptAndSimulateResponse(false);
  controller_->OnVerificationResult(AutofillClient::TRY_AGAIN_FAILURE);
  controller_->OnUnmaskResponse(
      base::ASCIIToUTF16("444"), base::ASCIIToUTF16("01"),
      base::ASCIIToUTF16("2050"), false /* should_store_pan */);
  base::HistogramTester histogram_tester;

  controller_->OnVerificationResult(AutofillClient::SUCCESS);
  controller_->OnUnmaskDialogClosed();

  histogram_tester.ExpectTotalCount("Autofill.UnmaskPrompt.Duration", 1);
  histogram_tester.ExpectTotalCount("Autofill.UnmaskPrompt.Duration.Success",
                                    1);
}

TEST_F(CardUnmaskPromptControllerImplTest, LogTimeBeforeAbandonUnmasking) {
  ShowPromptAndSimulateResponse(false);
  base::HistogramTester histogram_tester;

  controller_->OnUnmaskDialogClosed();

  histogram_tester.ExpectTotalCount(
      "Autofill.UnmaskPrompt.TimeBeforeAbandonUnmasking", 1);
}

TEST_F(CardUnmaskPromptControllerImplTest, LogRealPanResultSuccess) {
  ShowPromptAndSimulateResponse(false);
  base::HistogramTester histogram_tester;
  controller_->OnVerificationResult(AutofillClient::SUCCESS);

  histogram_tester.ExpectBucketCount("Autofill.UnmaskPrompt.GetRealPanResult",
                                     AutofillMetrics::PAYMENTS_RESULT_SUCCESS,
                                     1);
}

TEST_F(CardUnmaskPromptControllerImplTest, LogRealPanTryAgainFailure) {
  ShowPromptAndSimulateResponse(false);
  base::HistogramTester histogram_tester;

  controller_->OnVerificationResult(AutofillClient::TRY_AGAIN_FAILURE);

  histogram_tester.ExpectBucketCount(
      "Autofill.UnmaskPrompt.GetRealPanResult",
      AutofillMetrics::PAYMENTS_RESULT_TRY_AGAIN_FAILURE, 1);
}

TEST_F(CardUnmaskPromptControllerImplTest, LogUnmaskingDurationResultSuccess) {
  ShowPromptAndSimulateResponse(false);
  base::HistogramTester histogram_tester;

  controller_->OnVerificationResult(AutofillClient::SUCCESS);

  histogram_tester.ExpectTotalCount("Autofill.UnmaskPrompt.UnmaskingDuration",
                                    1);
  histogram_tester.ExpectTotalCount(
      "Autofill.UnmaskPrompt.UnmaskingDuration.Success", 1);
}

TEST_F(CardUnmaskPromptControllerImplTest,
       LogUnmaskingDurationTryAgainFailure) {
  ShowPromptAndSimulateResponse(false);
  base::HistogramTester histogram_tester;

  controller_->OnVerificationResult(AutofillClient::TRY_AGAIN_FAILURE);

  histogram_tester.ExpectTotalCount("Autofill.UnmaskPrompt.UnmaskingDuration",
                                    1);
  histogram_tester.ExpectTotalCount(
      "Autofill.UnmaskPrompt.UnmaskingDuration.Failure", 1);
}

struct CvcCase {
  const char* input;
  bool valid;
  // null when |valid| is false.
  const char* canonicalized_input;
};

class CvcInputValidationTest : public CardUnmaskPromptControllerImplGenericTest,
                               public testing::TestWithParam<CvcCase> {
 public:
  CvcInputValidationTest() {}
  ~CvcInputValidationTest() override {}

  void SetUp() override {
    test_unmask_prompt_view_.reset(new TestCardUnmaskPromptView());
    pref_service_.reset(new TestingPrefServiceSimple());
    controller_.reset(new TestCardUnmaskPromptController(pref_service_.get()));
    delegate_.reset(new TestCardUnmaskDelegate());
    pref_service_->registry()->RegisterBooleanPref(
        prefs::kAutofillWalletImportStorageCheckboxState, false);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(CvcInputValidationTest);
};

TEST_P(CvcInputValidationTest, CvcInputValidation) {
  auto cvc_case = GetParam();
  ShowPrompt();
  EXPECT_EQ(cvc_case.valid,
            controller_->InputCvcIsValid(ASCIIToUTF16(cvc_case.input)));
  if (!cvc_case.valid)
    return;

  controller_->OnUnmaskResponse(ASCIIToUTF16(cvc_case.input), ASCIIToUTF16("1"),
                                ASCIIToUTF16("2050"), false);
  EXPECT_EQ(ASCIIToUTF16(cvc_case.canonicalized_input),
            delegate_->response().cvc);
}

INSTANTIATE_TEST_CASE_P(CardUnmaskPromptControllerImplTest,
                        CvcInputValidationTest,
                        testing::Values(CvcCase{"123", true, "123"},
                                        CvcCase{"123 ", true, "123"},
                                        CvcCase{" 1234 ", false},
                                        CvcCase{"IOU", false}));

class CvcInputAmexValidationTest
    : public CardUnmaskPromptControllerImplGenericTest,
      public testing::TestWithParam<CvcCase> {
 public:
  CvcInputAmexValidationTest() {}
  ~CvcInputAmexValidationTest() override {}

  void SetUp() override {
    test_unmask_prompt_view_.reset(new TestCardUnmaskPromptView());
    pref_service_.reset(new TestingPrefServiceSimple());
    controller_.reset(new TestCardUnmaskPromptController(pref_service_.get()));
    delegate_.reset(new TestCardUnmaskDelegate());
    pref_service_->registry()->RegisterBooleanPref(
        prefs::kAutofillWalletImportStorageCheckboxState, false);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(CvcInputAmexValidationTest);
};

TEST_P(CvcInputAmexValidationTest, CvcInputValidation) {
  auto cvc_case_amex = GetParam();
  ShowPromptAmex();
  EXPECT_EQ(cvc_case_amex.valid,
            controller_->InputCvcIsValid(ASCIIToUTF16(cvc_case_amex.input)));
  if (!cvc_case_amex.valid)
    return;

  controller_->OnUnmaskResponse(ASCIIToUTF16(cvc_case_amex.input),
                                base::string16(), base::string16(), false);
  EXPECT_EQ(ASCIIToUTF16(cvc_case_amex.canonicalized_input),
            delegate_->response().cvc);
}

INSTANTIATE_TEST_CASE_P(CardUnmaskPromptControllerImplTest,
                        CvcInputAmexValidationTest,
                        testing::Values(CvcCase{"123", false},
                                        CvcCase{"123 ", false},
                                        CvcCase{"1234", true, "1234"},
                                        CvcCase{"\t1234 ", true, "1234"},
                                        CvcCase{" 1234", true, "1234"},
                                        CvcCase{"IOU$", false}));

struct ExpirationDateTestCase {
  const char* input_month;
  const char* input_year;
  bool valid;
};

class ExpirationDateValidationTest
    : public CardUnmaskPromptControllerImplGenericTest,
      public testing::TestWithParam<ExpirationDateTestCase> {
 public:
  ExpirationDateValidationTest() {}
  ~ExpirationDateValidationTest() override {}

  void SetUp() override {
    test_unmask_prompt_view_.reset(new TestCardUnmaskPromptView());
    pref_service_.reset(new TestingPrefServiceSimple());
    controller_.reset(new TestCardUnmaskPromptController(pref_service_.get()));
    delegate_.reset(new TestCardUnmaskDelegate());
    pref_service_->registry()->RegisterBooleanPref(
        prefs::kAutofillWalletImportStorageCheckboxState, false);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ExpirationDateValidationTest);
};

TEST_P(ExpirationDateValidationTest, ExpirationDateValidation) {
  auto exp_case = GetParam();
  ShowPrompt();
  EXPECT_EQ(exp_case.valid, controller_->InputExpirationIsValid(
                                ASCIIToUTF16(exp_case.input_month),
                                ASCIIToUTF16(exp_case.input_year)));
}

INSTANTIATE_TEST_CASE_P(
    CardUnmaskPromptControllerImplTest,
    ExpirationDateValidationTest,
    testing::Values(ExpirationDateTestCase{"01", "2040", true},
                    ExpirationDateTestCase{"1", "2040", true},
                    ExpirationDateTestCase{"1", "40", true},
                    ExpirationDateTestCase{"10", "40", true},
                    ExpirationDateTestCase{"01", "1940", false},
                    ExpirationDateTestCase{"13", "2040", false}));

}  // namespace autofill
