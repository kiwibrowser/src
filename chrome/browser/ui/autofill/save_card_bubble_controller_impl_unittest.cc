// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/autofill/save_card_bubble_controller_impl.h"

#include <stddef.h>
#include <utility>

#include "base/json/json_reader.h"
#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/histogram_tester.h"
#include "base/values.h"
#include "chrome/browser/ui/autofill/save_card_bubble_view.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/browser_with_test_window_test.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "components/autofill/core/browser/autofill_metrics.h"
#include "components/autofill/core/browser/autofill_test_utils.h"
#include "components/autofill/core/browser/credit_card.h"
#include "components/autofill/core/common/autofill_pref_names.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/navigation_handle.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::Bucket;
using testing::ElementsAre;

namespace autofill {

class TestSaveCardBubbleControllerImpl : public SaveCardBubbleControllerImpl {
 public:
  static void CreateForTesting(content::WebContents* web_contents) {
    web_contents->SetUserData(
        UserDataKey(),
        std::make_unique<TestSaveCardBubbleControllerImpl>(web_contents));
  }

  explicit TestSaveCardBubbleControllerImpl(content::WebContents* web_contents)
      : SaveCardBubbleControllerImpl(web_contents) {}

  void set_elapsed(base::TimeDelta elapsed) { elapsed_ = elapsed; }

  void SimulateNavigation() {
    content::RenderFrameHost* rfh = web_contents()->GetMainFrame();
    std::unique_ptr<content::NavigationHandle> navigation_handle =
        content::NavigationHandle::CreateNavigationHandleForTesting(
            GURL(), rfh, true);
    // Destructor calls DidFinishNavigation.
  }

 protected:
  base::TimeDelta Elapsed() const override { return elapsed_; }

 private:
  base::TimeDelta elapsed_;
};

class SaveCardBubbleControllerImplTest : public BrowserWithTestWindowTest {
 public:
  SaveCardBubbleControllerImplTest() {}

  void SetUp() override {
    BrowserWithTestWindowTest::SetUp();
    AddTab(browser(), GURL("about:blank"));
    content::WebContents* web_contents =
        browser()->tab_strip_model()->GetActiveWebContents();
    TestSaveCardBubbleControllerImpl::CreateForTesting(web_contents);
    user_prefs::UserPrefs::Get(web_contents->GetBrowserContext())
        ->SetInteger(
            prefs::kAutofillAcceptSaveCreditCardPromptState,
            prefs::PREVIOUS_SAVE_CREDIT_CARD_PROMPT_USER_DECISION_NONE);
  }

  BrowserWindow* CreateBrowserWindow() override {
    return new SaveCardBubbleTestBrowserWindow();
  }

  void SetLegalMessage(const std::string& message_json) {
    std::unique_ptr<base::Value> value(base::JSONReader::Read(message_json));
    ASSERT_TRUE(value);
    base::DictionaryValue* dictionary;
    ASSERT_TRUE(value->GetAsDictionary(&dictionary));
    std::unique_ptr<base::DictionaryValue> legal_message =
        dictionary->CreateDeepCopy();
    controller()->ShowBubbleForUpload(CreditCard(), std::move(legal_message),
                                      base::Bind(&SaveCardCallback));
  }

  void ShowLocalBubble(CreditCard* card = nullptr) {
    controller()->ShowBubbleForLocalSave(
        card ? CreditCard(*card)
             : autofill::test::GetCreditCard(),  // Visa by default
        base::Bind(&SaveCardCallback));
  }

  void ShowUploadBubble() {
    SetLegalMessage(
        "{"
        "  \"line\" : [ {"
        "     \"template\": \"This is the entire message.\""
        "  } ]"
        "}");
  }

  void CloseAndReshowBubble() {
    controller()->OnBubbleClosed();
    controller()->ReshowBubble();
  }

 protected:
  TestSaveCardBubbleControllerImpl* controller() {
    return static_cast<TestSaveCardBubbleControllerImpl*>(
        TestSaveCardBubbleControllerImpl::FromWebContents(
            browser()->tab_strip_model()->GetActiveWebContents()));
  }

 private:
  class TestSaveCardBubbleView final : public SaveCardBubbleView {
    void Hide() override {}
  };

  class SaveCardBubbleTestBrowserWindow : public TestBrowserWindow {
   public:
    SaveCardBubbleView* ShowSaveCreditCardBubble(
        content::WebContents* contents,
        SaveCardBubbleController* controller,
        bool user_gesture) override {
      if (!save_card_bubble_view_)
        save_card_bubble_view_.reset(new TestSaveCardBubbleView());
      return save_card_bubble_view_.get();
    }

   private:
    std::unique_ptr<TestSaveCardBubbleView> save_card_bubble_view_;
  };

  static void SaveCardCallback() {}

  DISALLOW_COPY_AND_ASSIGN(SaveCardBubbleControllerImplTest);
};

// Tests that the legal message lines vector is empty when doing a local save so
// that no legal messages will be shown to the user in that case.
TEST_F(SaveCardBubbleControllerImplTest, LegalMessageLinesEmptyOnLocalSave) {
  ShowUploadBubble();
  controller()->OnBubbleClosed();
  ShowLocalBubble();
  EXPECT_TRUE(controller()->GetLegalMessageLines().empty());
}

TEST_F(SaveCardBubbleControllerImplTest, Metrics_Local_FirstShow_ShowBubble) {
  base::HistogramTester histogram_tester;
  ShowLocalBubble();

  EXPECT_THAT(
      histogram_tester.GetAllSamples(
          "Autofill.SaveCreditCardPrompt.Local.FirstShow"),
      ElementsAre(Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOW_REQUESTED, 1),
                  Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOWN, 1)));
}

TEST_F(SaveCardBubbleControllerImplTest, Metrics_Local_Reshows_ShowBubble) {
  ShowLocalBubble();

  base::HistogramTester histogram_tester;
  CloseAndReshowBubble();

  EXPECT_THAT(
      histogram_tester.GetAllSamples(
          "Autofill.SaveCreditCardPrompt.Local.Reshows"),
      ElementsAre(Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOW_REQUESTED, 1),
                  Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOWN, 1)));
}

TEST_F(SaveCardBubbleControllerImplTest, Metrics_Upload_FirstShow_ShowBubble) {
  base::HistogramTester histogram_tester;
  ShowUploadBubble();

  EXPECT_THAT(
      histogram_tester.GetAllSamples(
          "Autofill.SaveCreditCardPrompt.Upload.FirstShow"),
      ElementsAre(Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOW_REQUESTED, 1),
                  Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOWN, 1)));
}

TEST_F(SaveCardBubbleControllerImplTest, Metrics_Upload_Reshows_ShowBubble) {
  ShowUploadBubble();

  base::HistogramTester histogram_tester;
  CloseAndReshowBubble();

  EXPECT_THAT(
      histogram_tester.GetAllSamples(
          "Autofill.SaveCreditCardPrompt.Upload.Reshows"),
      ElementsAre(Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOW_REQUESTED, 1),
                  Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOWN, 1)));
}

TEST_F(SaveCardBubbleControllerImplTest, Metrics_Local_FirstShow_SaveButton) {
  ShowLocalBubble();

  base::HistogramTester histogram_tester;
  controller()->OnSaveButton();
  controller()->OnBubbleClosed();

  histogram_tester.ExpectUniqueSample(
      "Autofill.SaveCreditCardPrompt.Local.FirstShow",
      AutofillMetrics::SAVE_CARD_PROMPT_END_ACCEPTED, 1);
}

TEST_F(SaveCardBubbleControllerImplTest, Metrics_Local_Reshows_SaveButton) {
  ShowLocalBubble();
  CloseAndReshowBubble();

  base::HistogramTester histogram_tester;
  controller()->OnSaveButton();
  controller()->OnBubbleClosed();

  histogram_tester.ExpectUniqueSample(
      "Autofill.SaveCreditCardPrompt.Local.Reshows",
      AutofillMetrics::SAVE_CARD_PROMPT_END_ACCEPTED, 1);
}

TEST_F(SaveCardBubbleControllerImplTest, Metrics_Local_FirstShow_CancelButton) {
  ShowLocalBubble();

  base::HistogramTester histogram_tester;
  controller()->OnCancelButton();
  controller()->OnBubbleClosed();

  histogram_tester.ExpectUniqueSample(
      "Autofill.SaveCreditCardPrompt.Local.FirstShow",
      AutofillMetrics::SAVE_CARD_PROMPT_END_DENIED, 1);
}

TEST_F(SaveCardBubbleControllerImplTest, Metrics_Local_Reshows_CancelButton) {
  ShowLocalBubble();
  CloseAndReshowBubble();

  base::HistogramTester histogram_tester;
  controller()->OnCancelButton();
  controller()->OnBubbleClosed();

  histogram_tester.ExpectUniqueSample(
      "Autofill.SaveCreditCardPrompt.Local.Reshows",
      AutofillMetrics::SAVE_CARD_PROMPT_END_DENIED, 1);
}

TEST_F(SaveCardBubbleControllerImplTest,
       Metrics_Local_FirstShow_CancelButton_FirstShow) {
  base::HistogramTester histogram_tester;
  ShowLocalBubble();
  controller()->OnCancelButton();
  controller()->OnBubbleClosed();

  ShowLocalBubble();
  controller()->OnCancelButton();
  controller()->OnBubbleClosed();

  EXPECT_THAT(
      histogram_tester.GetAllSamples(
          "Autofill.SaveCreditCardPrompt.Local.FirstShow"),
      ElementsAre(Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOW_REQUESTED, 2),
                  Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOWN, 2),
                  Bucket(AutofillMetrics::SAVE_CARD_PROMPT_END_DENIED, 2)));
  EXPECT_THAT(
      histogram_tester.GetAllSamples(
          "Autofill.SaveCreditCardPrompt.Local.FirstShow.PreviouslyDenied"),
      ElementsAre(Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOW_REQUESTED, 1),
                  Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOWN, 1),
                  Bucket(AutofillMetrics::SAVE_CARD_PROMPT_END_DENIED, 1)));
  EXPECT_THAT(
      histogram_tester.GetAllSamples("Autofill.SaveCreditCardPrompt.Local."
                                     "FirstShow.NoPreviousDecision"),
      ElementsAre(Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOW_REQUESTED, 1),
                  Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOWN, 1),
                  Bucket(AutofillMetrics::SAVE_CARD_PROMPT_END_DENIED, 1)));
}

TEST_F(SaveCardBubbleControllerImplTest,
       Metrics_Local_FirstShow_CancelButton_FirstShow_SaveButton_FirstShow) {
  base::HistogramTester histogram_tester;
  ShowLocalBubble();
  controller()->OnCancelButton();
  controller()->OnBubbleClosed();

  ShowLocalBubble();
  controller()->OnSaveButton();
  controller()->OnBubbleClosed();

  ShowLocalBubble();

  EXPECT_THAT(
      histogram_tester.GetAllSamples(
          "Autofill.SaveCreditCardPrompt.Local.FirstShow"),
      ElementsAre(Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOW_REQUESTED, 3),
                  Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOWN, 3),
                  Bucket(AutofillMetrics::SAVE_CARD_PROMPT_END_ACCEPTED, 1),
                  Bucket(AutofillMetrics::SAVE_CARD_PROMPT_END_DENIED, 1)));
  EXPECT_THAT(
      histogram_tester.GetAllSamples("Autofill.SaveCreditCardPrompt.Local."
                                     "FirstShow.NoPreviousDecision"),
      ElementsAre(Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOW_REQUESTED, 1),
                  Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOWN, 1),
                  Bucket(AutofillMetrics::SAVE_CARD_PROMPT_END_DENIED, 1)));
  EXPECT_THAT(
      histogram_tester.GetAllSamples(
          "Autofill.SaveCreditCardPrompt.Local.FirstShow.PreviouslyDenied"),
      ElementsAre(Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOW_REQUESTED, 1),
                  Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOWN, 1),
                  Bucket(AutofillMetrics::SAVE_CARD_PROMPT_END_ACCEPTED, 1)));
  EXPECT_THAT(
      histogram_tester.GetAllSamples(
          "Autofill.SaveCreditCardPrompt.Local.FirstShow.PreviouslyAccepted"),
      ElementsAre(Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOW_REQUESTED, 1),
                  Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOWN, 1)));
}

TEST_F(SaveCardBubbleControllerImplTest,
       Metrics_Upload_FirstShow_CancelButton_FirstShow_SaveButton_FirstShow) {
  base::HistogramTester histogram_tester;
  ShowUploadBubble();
  controller()->OnCancelButton();
  controller()->OnBubbleClosed();

  ShowUploadBubble();
  controller()->OnSaveButton();
  controller()->OnBubbleClosed();

  ShowUploadBubble();

  EXPECT_THAT(
      histogram_tester.GetAllSamples(
          "Autofill.SaveCreditCardPrompt.Upload.FirstShow"),
      ElementsAre(Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOW_REQUESTED, 3),
                  Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOWN, 3),
                  Bucket(AutofillMetrics::SAVE_CARD_PROMPT_END_ACCEPTED, 1),
                  Bucket(AutofillMetrics::SAVE_CARD_PROMPT_END_DENIED, 1)));
  EXPECT_THAT(
      histogram_tester.GetAllSamples("Autofill.SaveCreditCardPrompt.Upload."
                                     "FirstShow.NoPreviousDecision"),
      ElementsAre(Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOW_REQUESTED, 1),
                  Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOWN, 1),
                  Bucket(AutofillMetrics::SAVE_CARD_PROMPT_END_DENIED, 1)));
  EXPECT_THAT(
      histogram_tester.GetAllSamples(
          "Autofill.SaveCreditCardPrompt.Upload.FirstShow.PreviouslyDenied"),
      ElementsAre(Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOW_REQUESTED, 1),
                  Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOWN, 1),
                  Bucket(AutofillMetrics::SAVE_CARD_PROMPT_END_ACCEPTED, 1)));
  EXPECT_THAT(
      histogram_tester.GetAllSamples(
          "Autofill.SaveCreditCardPrompt.Upload.FirstShow.PreviouslyAccepted"),
      ElementsAre(Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOW_REQUESTED, 1),
                  Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOWN, 1)));
}

TEST_F(SaveCardBubbleControllerImplTest,
       Metrics_Local_FirstShow_CancelButton_Reshows) {
  base::HistogramTester histogram_tester;
  ShowLocalBubble();
  CloseAndReshowBubble();

  EXPECT_THAT(
      histogram_tester.GetAllSamples("Autofill.SaveCreditCardPrompt.Local."
                                     "Reshows.NoPreviousDecision"),
      ElementsAre(Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOW_REQUESTED, 1),
                  Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOWN, 1)));
}

TEST_F(SaveCardBubbleControllerImplTest,
       Metrics_Local_FirstShow_Reshows_Reshows) {
  base::HistogramTester histogram_tester;
  ShowLocalBubble();
  CloseAndReshowBubble();
  CloseAndReshowBubble();

  EXPECT_THAT(
      histogram_tester.GetAllSamples(
          "Autofill.SaveCreditCardPrompt.Local.Reshows"),
      ElementsAre(Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOW_REQUESTED, 2),
                  Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOWN, 2)));
  EXPECT_THAT(
      histogram_tester.GetAllSamples("Autofill.SaveCreditCardPrompt.Local."
                                     "Reshows.NoPreviousDecision"),
      ElementsAre(Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOW_REQUESTED, 2),
                  Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOWN, 2)));
}

TEST_F(SaveCardBubbleControllerImplTest,
       Metrics_Local_FirstShow_NavigateWhileShowing) {
  ShowLocalBubble();

  base::HistogramTester histogram_tester;
  // The bubble should still stick around for up to kSurviveNavigationSeconds
  // (5) seconds regardless of navigation.
  controller()->set_elapsed(base::TimeDelta::FromSeconds(3));

  controller()->SimulateNavigation();

  histogram_tester.ExpectTotalCount(
      "Autofill.SaveCreditCardPrompt.Local.FirstShow", 0);

  // Wait 3 more seconds (6 total); bubble should go away on next navigation.
  controller()->set_elapsed(base::TimeDelta::FromSeconds(6));

  controller()->SimulateNavigation();

  histogram_tester.ExpectUniqueSample(
      "Autofill.SaveCreditCardPrompt.Local.FirstShow",
      AutofillMetrics::SAVE_CARD_PROMPT_END_NAVIGATION_SHOWING, 1);
}

TEST_F(SaveCardBubbleControllerImplTest,
       Metrics_Local_Reshows_NavigateWhileShowing) {
  ShowLocalBubble();
  CloseAndReshowBubble();

  base::HistogramTester histogram_tester;
  // The bubble should still stick around for up to kSurviveNavigationSeconds
  // (5) seconds regardless of navigation.
  controller()->set_elapsed(base::TimeDelta::FromSeconds(3));

  controller()->SimulateNavigation();

  histogram_tester.ExpectTotalCount(
      "Autofill.SaveCreditCardPrompt.Local.Reshows", 0);

  // Wait 3 more seconds (6 total); bubble should go away on next navigation.
  controller()->set_elapsed(base::TimeDelta::FromSeconds(6));

  controller()->SimulateNavigation();

  histogram_tester.ExpectUniqueSample(
      "Autofill.SaveCreditCardPrompt.Local.Reshows",
      AutofillMetrics::SAVE_CARD_PROMPT_END_NAVIGATION_SHOWING, 1);
}

TEST_F(SaveCardBubbleControllerImplTest,
       Metrics_Upload_FirstShow_NavigateWhileShowing) {
  ShowUploadBubble();

  base::HistogramTester histogram_tester;
  // The bubble should still stick around for up to kSurviveNavigationSeconds
  // (5) seconds regardless of navigation.
  controller()->set_elapsed(base::TimeDelta::FromSeconds(3));

  controller()->SimulateNavigation();

  histogram_tester.ExpectTotalCount(
      "Autofill.SaveCreditCardPrompt.Upload.FirstShow", 0);

  // Wait 3 more seconds (6 total); bubble should go away on next navigation.
  controller()->set_elapsed(base::TimeDelta::FromSeconds(6));

  controller()->SimulateNavigation();

  histogram_tester.ExpectUniqueSample(
      "Autofill.SaveCreditCardPrompt.Upload.FirstShow",
      AutofillMetrics::SAVE_CARD_PROMPT_END_NAVIGATION_SHOWING, 1);
}

TEST_F(SaveCardBubbleControllerImplTest,
       Metrics_Upload_Reshows_NavigateWhileShowing) {
  ShowUploadBubble();
  CloseAndReshowBubble();

  base::HistogramTester histogram_tester;
  // The bubble should still stick around for up to kSurviveNavigationSeconds
  // (5) seconds regardless of navigation.
  controller()->set_elapsed(base::TimeDelta::FromSeconds(3));

  controller()->SimulateNavigation();

  histogram_tester.ExpectTotalCount(
      "Autofill.SaveCreditCardPrompt.Upload.Reshows", 0);

  // Wait 3 more seconds (6 total); bubble should go away on next navigation.
  controller()->set_elapsed(base::TimeDelta::FromSeconds(6));

  controller()->SimulateNavigation();

  histogram_tester.ExpectUniqueSample(
      "Autofill.SaveCreditCardPrompt.Upload.Reshows",
      AutofillMetrics::SAVE_CARD_PROMPT_END_NAVIGATION_SHOWING, 1);
}

TEST_F(SaveCardBubbleControllerImplTest,
       Metrics_Local_FirstShow_NavigateWhileHidden) {
  ShowLocalBubble();

  base::HistogramTester histogram_tester;
  controller()->OnBubbleClosed();
  // Fake-navigate after bubble has been visible for a long time.
  controller()->set_elapsed(base::TimeDelta::FromMinutes(1));
  controller()->SimulateNavigation();

  histogram_tester.ExpectUniqueSample(
      "Autofill.SaveCreditCardPrompt.Local.FirstShow",
      AutofillMetrics::SAVE_CARD_PROMPT_END_NAVIGATION_HIDDEN, 1);
}

TEST_F(SaveCardBubbleControllerImplTest,
       Metrics_Local_Reshows_NavigateWhileHidden) {
  ShowLocalBubble();
  CloseAndReshowBubble();

  base::HistogramTester histogram_tester;
  controller()->OnBubbleClosed();
  // Fake-navigate after bubble has been visible for a long time.
  controller()->set_elapsed(base::TimeDelta::FromMinutes(1));
  controller()->SimulateNavigation();

  histogram_tester.ExpectUniqueSample(
      "Autofill.SaveCreditCardPrompt.Local.Reshows",
      AutofillMetrics::SAVE_CARD_PROMPT_END_NAVIGATION_HIDDEN, 1);
}

TEST_F(SaveCardBubbleControllerImplTest,
       Metrics_Upload_FirstShow_NavigateWhileHidden) {
  ShowUploadBubble();

  base::HistogramTester histogram_tester;
  controller()->OnBubbleClosed();
  // Fake-navigate after bubble has been visible for a long time.
  controller()->set_elapsed(base::TimeDelta::FromMinutes(1));
  controller()->SimulateNavigation();

  histogram_tester.ExpectUniqueSample(
      "Autofill.SaveCreditCardPrompt.Upload.FirstShow",
      AutofillMetrics::SAVE_CARD_PROMPT_END_NAVIGATION_HIDDEN, 1);
}

TEST_F(SaveCardBubbleControllerImplTest,
       Metrics_Upload_Reshows_NavigateWhileHidden) {
  ShowUploadBubble();
  CloseAndReshowBubble();

  base::HistogramTester histogram_tester;
  controller()->OnBubbleClosed();
  // Fake-navigate after bubble has been visible for a long time.
  controller()->set_elapsed(base::TimeDelta::FromMinutes(1));
  controller()->SimulateNavigation();

  histogram_tester.ExpectUniqueSample(
      "Autofill.SaveCreditCardPrompt.Upload.Reshows",
      AutofillMetrics::SAVE_CARD_PROMPT_END_NAVIGATION_HIDDEN, 1);
}

TEST_F(SaveCardBubbleControllerImplTest, Metrics_Local_FirstShow_LearnMore) {
  ShowLocalBubble();

  base::HistogramTester histogram_tester;
  controller()->OnLearnMoreClicked();

  histogram_tester.ExpectUniqueSample(
      "Autofill.SaveCreditCardPrompt.Local.FirstShow",
      AutofillMetrics::SAVE_CARD_PROMPT_DISMISS_CLICK_LEARN_MORE, 1);
}

TEST_F(SaveCardBubbleControllerImplTest, Metrics_Local_Reshows_LearnMore) {
  ShowLocalBubble();
  CloseAndReshowBubble();

  base::HistogramTester histogram_tester;
  controller()->OnLearnMoreClicked();

  histogram_tester.ExpectUniqueSample(
      "Autofill.SaveCreditCardPrompt.Local.Reshows",
      AutofillMetrics::SAVE_CARD_PROMPT_DISMISS_CLICK_LEARN_MORE, 1);
}

TEST_F(SaveCardBubbleControllerImplTest,
       Metrics_Upload_FirstShow_LegalMessageLink) {
  ShowUploadBubble();

  base::HistogramTester histogram_tester;
  controller()->OnLegalMessageLinkClicked(GURL("http://www.example.com"));

  histogram_tester.ExpectUniqueSample(
      "Autofill.SaveCreditCardPrompt.Upload.FirstShow",
      AutofillMetrics::SAVE_CARD_PROMPT_DISMISS_CLICK_LEGAL_MESSAGE, 1);
}

TEST_F(SaveCardBubbleControllerImplTest,
       Metrics_Upload_Reshows_LegalMessageLink) {
  ShowUploadBubble();
  CloseAndReshowBubble();

  base::HistogramTester histogram_tester;
  controller()->OnLegalMessageLinkClicked(GURL("http://www.example.com"));

  histogram_tester.ExpectUniqueSample(
      "Autofill.SaveCreditCardPrompt.Upload.Reshows",
      AutofillMetrics::SAVE_CARD_PROMPT_DISMISS_CLICK_LEGAL_MESSAGE, 1);
}

// SAVE_CARD_PROMPT_END_INVALID_LEGAL_MESSAGE is only possible for
// Upload.FirstShow.
TEST_F(SaveCardBubbleControllerImplTest,
       Metrics_Upload_FirstShow_InvalidLegalMessage) {
  base::HistogramTester histogram_tester;

  // Legal message is invalid because it's missing the url.
  SetLegalMessage(
      "{"
      "  \"line\" : [ {"
      "     \"template\": \"Panda {0}.\","
      "     \"template_parameter\": [ {"
      "        \"display_text\": \"bear\""
      "     } ]"
      "  } ]"
      "}");

  EXPECT_THAT(
      histogram_tester.GetAllSamples(
          "Autofill.SaveCreditCardPrompt.Upload.FirstShow"),
      ElementsAre(
          Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOW_REQUESTED, 1),
          Bucket(AutofillMetrics::SAVE_CARD_PROMPT_END_INVALID_LEGAL_MESSAGE,
                 1)));
}

TEST_F(SaveCardBubbleControllerImplTest, OnlyOneActiveBubble_RepeatedLocal) {
  base::HistogramTester histogram_tester;
  ShowLocalBubble();
  ShowLocalBubble();
  ShowLocalBubble();
  EXPECT_THAT(
      histogram_tester.GetAllSamples(
          "Autofill.SaveCreditCardPrompt.Local.FirstShow"),
      ElementsAre(Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOW_REQUESTED, 1),
                  Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOWN, 1)));
}

TEST_F(SaveCardBubbleControllerImplTest, OnlyOneActiveBubble_RepeatedUpload) {
  base::HistogramTester histogram_tester;
  ShowUploadBubble();
  ShowUploadBubble();
  ShowUploadBubble();
  EXPECT_THAT(
      histogram_tester.GetAllSamples(
          "Autofill.SaveCreditCardPrompt.Upload.FirstShow"),
      ElementsAre(Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOW_REQUESTED, 1),
                  Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOWN, 1)));
}

TEST_F(SaveCardBubbleControllerImplTest, OnlyOneActiveBubble_LocalThenUpload) {
  base::HistogramTester histogram_tester;
  ShowLocalBubble();
  ShowUploadBubble();
  ShowUploadBubble();
  EXPECT_THAT(
      histogram_tester.GetAllSamples(
          "Autofill.SaveCreditCardPrompt.Local.FirstShow"),
      ElementsAre(Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOW_REQUESTED, 1),
                  Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOWN, 1)));
  EXPECT_TRUE(
      histogram_tester
          .GetAllSamples("Autofill.SaveCreditCardPrompt.Upload.FirstShow")
          .empty());
}

TEST_F(SaveCardBubbleControllerImplTest, OnlyOneActiveBubble_UploadThenLocal) {
  base::HistogramTester histogram_tester;
  ShowUploadBubble();
  ShowLocalBubble();
  ShowLocalBubble();
  EXPECT_THAT(
      histogram_tester.GetAllSamples(
          "Autofill.SaveCreditCardPrompt.Upload.FirstShow"),
      ElementsAre(Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOW_REQUESTED, 1),
                  Bucket(AutofillMetrics::SAVE_CARD_PROMPT_SHOWN, 1)));
  EXPECT_TRUE(
      histogram_tester
          .GetAllSamples("Autofill.SaveCreditCardPrompt.Local.FirstShow")
          .empty());
}

}  // namespace autofill
