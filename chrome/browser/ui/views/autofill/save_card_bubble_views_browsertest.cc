// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/strings/utf_string_conversions.h"
#include "base/test/histogram_tester.h"
#include "chrome/browser/ui/autofill/save_card_bubble_controller_impl.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/autofill/save_card_bubble_views.h"
#include "chrome/browser/ui/views/autofill/save_card_bubble_views_browsertest_base.h"
#include "components/autofill/core/browser/autofill_experiments.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "ui/base/ui_base_features.h"
#include "ui/views/bubble/bubble_frame_view.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/styled_label.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/window/dialog_client_view.h"

using base::Bucket;
using testing::ElementsAre;

namespace autofill {

class SaveCardBubbleViewsFullFormBrowserTest
    : public SaveCardBubbleViewsBrowserTestBase {
 protected:
  SaveCardBubbleViewsFullFormBrowserTest()
      : SaveCardBubbleViewsBrowserTestBase(
            "/credit_card_upload_form_address_and_cc.html") {}

 private:
  DISALLOW_COPY_AND_ASSIGN(SaveCardBubbleViewsFullFormBrowserTest);
};

class SaveCardBubbleViewsFullFormWithShippingBrowserTest
    : public SaveCardBubbleViewsBrowserTestBase {
 protected:
  SaveCardBubbleViewsFullFormWithShippingBrowserTest()
      : SaveCardBubbleViewsBrowserTestBase(
            "/credit_card_upload_form_shipping_address.html") {}

 private:
  DISALLOW_COPY_AND_ASSIGN(SaveCardBubbleViewsFullFormWithShippingBrowserTest);
};

// Tests the local save bubble. Ensures that local save appears if the RPC to
// Google Payments fails unexpectedly.
IN_PROC_BROWSER_TEST_F(
    SaveCardBubbleViewsFullFormBrowserTest,
    Local_SubmittingFormShowsBubbleIfGetUploadDetailsRpcFails) {
  // Set up the Payments RPC.
  SetUploadDetailsRpcServerError();

  // Submitting the form and having the call to Payments fail should show the
  // local save bubble.
  // (Must wait for response from Payments before accessing the controller.)
  ResetEventWaiterForSequence(
      {DialogEvent::REQUESTED_UPLOAD_SAVE,
       DialogEvent::RECEIVED_GET_UPLOAD_DETAILS_RESPONSE,
       DialogEvent::OFFERED_LOCAL_SAVE});
  FillAndSubmitForm();
  WaitForObservedEvent();
  EXPECT_TRUE(
      FindViewInBubbleById(DialogViewId::MAIN_CONTENT_VIEW_LOCAL)->visible());
}

// Tests the local save bubble. Ensures that clicking the [Save] button
// successfully causes the bubble to go away.
IN_PROC_BROWSER_TEST_F(SaveCardBubbleViewsFullFormBrowserTest,
                       Local_ClickingSaveClosesBubble) {
  // Set up the Payments RPC.
  SetUploadDetailsRpcPaymentsDeclines();

  // Submitting the form and having Payments decline offering to save should
  // show the local save bubble.
  // (Must wait for response from Payments before accessing the controller.)
  ResetEventWaiterForSequence(
      {DialogEvent::REQUESTED_UPLOAD_SAVE,
       DialogEvent::RECEIVED_GET_UPLOAD_DETAILS_RESPONSE,
       DialogEvent::OFFERED_LOCAL_SAVE});
  FillAndSubmitForm();
  WaitForObservedEvent();
  EXPECT_TRUE(
      FindViewInBubbleById(DialogViewId::MAIN_CONTENT_VIEW_LOCAL)->visible());

  // Clicking [Save] should accept and close it.
  base::HistogramTester histogram_tester;
  content::TestNavigationObserver nav_observer(GetActiveWebContents(), 1);
  ClickOnDialogViewWithIdAndWait(DialogViewId::OK_BUTTON);
  // The bubble should be closed.
  // (Must wait for page navigation to complete before checking.)
  nav_observer.Wait();
  EXPECT_FALSE(GetSaveCardBubbleViews());
  // UMA should have recorded bubble acceptance.
  histogram_tester.ExpectUniqueSample(
      "Autofill.SaveCreditCardPrompt.Local.FirstShow",
      AutofillMetrics::SAVE_CARD_PROMPT_END_ACCEPTED, 1);
}

// Tests the local save bubble. Ensures that clicking the [No thanks] button
// successfully causes the bubble to go away.
IN_PROC_BROWSER_TEST_F(
    SaveCardBubbleViewsFullFormBrowserTest,
    Local_ClickingNoThanksClosesBubbleIfSecondaryUiMdExpOff) {
  // Disable the SecondaryUiMd experiment.
  scoped_feature_list_.InitAndDisableFeature(features::kSecondaryUiMd);

  // Set up the Payments RPC.
  SetUploadDetailsRpcPaymentsDeclines();

  // Submitting the form and having Payments decline offering to save should
  // show the local save bubble.
  // (Must wait for response from Payments before accessing the controller.)
  ResetEventWaiterForSequence(
      {DialogEvent::REQUESTED_UPLOAD_SAVE,
       DialogEvent::RECEIVED_GET_UPLOAD_DETAILS_RESPONSE,
       DialogEvent::OFFERED_LOCAL_SAVE});
  FillAndSubmitForm();
  WaitForObservedEvent();
  EXPECT_TRUE(
      FindViewInBubbleById(DialogViewId::MAIN_CONTENT_VIEW_LOCAL)->visible());

  // Clicking [No thanks] should cancel and close it.
  base::HistogramTester histogram_tester;
  content::TestNavigationObserver nav_observer(GetActiveWebContents(), 1);
  ClickOnDialogViewWithIdAndWait(DialogViewId::CANCEL_BUTTON);
  // The bubble should be closed.
  // (Must wait for page navigation to complete before checking.)
  nav_observer.Wait();
  EXPECT_FALSE(GetSaveCardBubbleViews());
  // UMA should have recorded bubble rejection.
  histogram_tester.ExpectUniqueSample(
      "Autofill.SaveCreditCardPrompt.Local.FirstShow",
      AutofillMetrics::SAVE_CARD_PROMPT_END_DENIED, 1);
}

// Tests the local save bubble. Ensures that the Harmony version of the bubble
// does not have a [No thanks] button (it has an [X] Close button instead.)
IN_PROC_BROWSER_TEST_F(SaveCardBubbleViewsFullFormBrowserTest,
                       Local_ShouldNotHaveNoThanksButtonIfSecondaryUiMdExpOn) {
  // Enable the SecondaryUiMd experiment.
  scoped_feature_list_.InitAndEnableFeature(features::kSecondaryUiMd);

  // Set up the Payments RPC.
  SetUploadDetailsRpcPaymentsDeclines();

  // Submitting the form and having Payments decline offering to save should
  // show the local save bubble.
  // (Must wait for response from Payments before accessing the controller.)
  ResetEventWaiterForSequence(
      {DialogEvent::REQUESTED_UPLOAD_SAVE,
       DialogEvent::RECEIVED_GET_UPLOAD_DETAILS_RESPONSE,
       DialogEvent::OFFERED_LOCAL_SAVE});
  FillAndSubmitForm();
  WaitForObservedEvent();
  EXPECT_TRUE(
      FindViewInBubbleById(DialogViewId::MAIN_CONTENT_VIEW_LOCAL)->visible());

  // Assert that the cancel button cannot be found.
  EXPECT_FALSE(FindViewInBubbleById(DialogViewId::CANCEL_BUTTON));
}

// Tests the local save bubble. Ensures that clicking the [Learn more] link
// causes the bubble to go away and opens the relevant help page.
IN_PROC_BROWSER_TEST_F(SaveCardBubbleViewsFullFormBrowserTest,
                       Local_ClickingLearnMoreClosesBubble) {
  // Set up the Payments RPC.
  SetUploadDetailsRpcPaymentsDeclines();

  // Submitting the form and having Payments decline offering to save should
  // show the local save bubble.
  // (Must wait for response from Payments before accessing the controller.)
  ResetEventWaiterForSequence(
      {DialogEvent::REQUESTED_UPLOAD_SAVE,
       DialogEvent::RECEIVED_GET_UPLOAD_DETAILS_RESPONSE,
       DialogEvent::OFFERED_LOCAL_SAVE});
  FillAndSubmitForm();
  WaitForObservedEvent();
  EXPECT_TRUE(
      FindViewInBubbleById(DialogViewId::MAIN_CONTENT_VIEW_LOCAL)->visible());

  // Click the [Learn more] link.
  content::WebContentsAddedObserver web_contents_added_observer;
  ClickOnDialogViewWithIdAndWait(DialogViewId::LEARN_MORE_LINK);

  // The bubble should be hidden after clicking the link (not preferred
  // behavior, but it's what we've got.)
  EXPECT_FALSE(GetSaveCardBubbleViews());
  // A new support page tab should have been spawned.
  content::WebContents* new_tab_contents =
      web_contents_added_observer.GetWebContents();
  EXPECT_TRUE(new_tab_contents->GetVisibleURL().spec() ==
                  "https://support.google.com/chrome/?p=settings_autofill" ||
              new_tab_contents->GetVisibleURL().spec() ==
                  "https://support.google.com/chromebook/?p=settings_autofill");
}

// Tests the local save bubble. Ensures that the bubble behaves correctly if
// dismissed and then immediately torn down (e.g. by closing browser window)
// before the asynchronous close completes. Regression test for
// https://crbug.com/842577 .
IN_PROC_BROWSER_TEST_F(SaveCardBubbleViewsFullFormBrowserTest,
                       Local_SynchronousCloseAfterAsynchronousClose) {
  // Set up the Payments RPC.
  SetUploadDetailsRpcPaymentsDeclines();

  // Submitting the form and having Payments decline offering to save should
  // show the local save bubble.
  // (Must wait for response from Payments before accessing the controller.)
  ResetEventWaiterForSequence(
      {DialogEvent::REQUESTED_UPLOAD_SAVE,
       DialogEvent::RECEIVED_GET_UPLOAD_DETAILS_RESPONSE,
       DialogEvent::OFFERED_LOCAL_SAVE});
  FillAndSubmitForm();
  WaitForObservedEvent();

  SaveCardBubbleViews* bubble = GetSaveCardBubbleViews();
  EXPECT_TRUE(bubble);
  views::Widget* bubble_widget = bubble->GetWidget();
  EXPECT_TRUE(bubble_widget);
  EXPECT_TRUE(bubble_widget->IsVisible());
  bubble->Hide();
  EXPECT_FALSE(bubble_widget->IsVisible());

  // The bubble is immediately hidden, but it can still receive events here.
  // Simulate an OS event arriving to destroy the Widget.
  bubble_widget->CloseNow();
  // |bubble| and |bubble_widget| now point to deleted objects.

  // Simulate closing the browser window.
  browser()->tab_strip_model()->CloseAllTabs();

  // Process the asynchronous close (which should do nothing).
  base::RunLoop().RunUntilIdle();
}

// Tests the upload save bubble. Ensures that clicking the [Save] button
// successfully causes the bubble to go away and sends an UploadCardRequest RPC
// to Google Payments.
IN_PROC_BROWSER_TEST_F(SaveCardBubbleViewsFullFormBrowserTest,
                       Upload_ClickingSaveClosesBubble) {
  // Set up the Payments RPC.
  SetUploadDetailsRpcPaymentsAccepts();

  // Submitting the form should show the upload save bubble and legal footer.
  // (Must wait for response from Payments before accessing the controller.)
  ResetEventWaiterForSequence(
      {DialogEvent::REQUESTED_UPLOAD_SAVE,
       DialogEvent::RECEIVED_GET_UPLOAD_DETAILS_RESPONSE});
  FillAndSubmitForm();
  WaitForObservedEvent();
  EXPECT_TRUE(
      FindViewInBubbleById(DialogViewId::MAIN_CONTENT_VIEW_UPLOAD)->visible());
  EXPECT_TRUE(FindViewInBubbleById(DialogViewId::FOOTNOTE_VIEW)->visible());

  // Clicking [Save] should accept and close it, then send an UploadCardRequest
  // to Google Payments.
  ResetEventWaiterForSequence({DialogEvent::SENT_UPLOAD_CARD_REQUEST});
  base::HistogramTester histogram_tester;
  content::TestNavigationObserver nav_observer(GetActiveWebContents(), 1);
  ClickOnDialogViewWithIdAndWait(DialogViewId::OK_BUTTON);
  // The bubble should be closed.
  // (Must wait for page navigation to complete before checking.)
  nav_observer.Wait();
  EXPECT_FALSE(GetSaveCardBubbleViews());
  // UMA should have recorded bubble acceptance.
  histogram_tester.ExpectUniqueSample(
      "Autofill.SaveCreditCardPrompt.Upload.FirstShow",
      AutofillMetrics::SAVE_CARD_PROMPT_END_ACCEPTED, 1);
}

// Tests the upload save bubble. Ensures that clicking the [No thanks] button
// successfully causes the bubble to go away.
IN_PROC_BROWSER_TEST_F(
    SaveCardBubbleViewsFullFormBrowserTest,
    Upload_ClickingNoThanksClosesBubbleIfSecondaryUiMdExpOff) {
  // Disable the SecondaryUiMd experiment.
  scoped_feature_list_.InitAndDisableFeature(features::kSecondaryUiMd);

  // Set up the Payments RPC.
  SetUploadDetailsRpcPaymentsAccepts();

  // Submitting the form should show the upload save bubble and legal footer.
  // (Must wait for response from Payments before accessing the controller.)
  ResetEventWaiterForSequence(
      {DialogEvent::REQUESTED_UPLOAD_SAVE,
       DialogEvent::RECEIVED_GET_UPLOAD_DETAILS_RESPONSE});
  FillAndSubmitForm();
  WaitForObservedEvent();
  EXPECT_TRUE(
      FindViewInBubbleById(DialogViewId::MAIN_CONTENT_VIEW_UPLOAD)->visible());
  EXPECT_TRUE(FindViewInBubbleById(DialogViewId::FOOTNOTE_VIEW)->visible());

  // Clicking [No thanks] should cancel and close it.
  base::HistogramTester histogram_tester;
  content::TestNavigationObserver nav_observer(GetActiveWebContents(), 1);
  ClickOnDialogViewWithIdAndWait(DialogViewId::CANCEL_BUTTON);
  // The bubble should be closed.
  // (Must wait for page navigation to complete before checking.)
  nav_observer.Wait();
  EXPECT_FALSE(GetSaveCardBubbleViews());
  // UMA should have recorded bubble rejection.
  histogram_tester.ExpectUniqueSample(
      "Autofill.SaveCreditCardPrompt.Upload.FirstShow",
      AutofillMetrics::SAVE_CARD_PROMPT_END_DENIED, 1);
}

// Tests the upload save bubble. Ensures that the Harmony version of the bubble
// does not have a [No thanks] button (it has an [X] Close button instead.)
IN_PROC_BROWSER_TEST_F(SaveCardBubbleViewsFullFormBrowserTest,
                       Upload_ShouldNotHaveNoThanksButtonIfSecondaryUiMdExpOn) {
  // Enable the SecondaryUiMd experiment.
  scoped_feature_list_.InitAndEnableFeature(features::kSecondaryUiMd);

  // Set up the Payments RPC.
  SetUploadDetailsRpcPaymentsAccepts();

  // Submitting the form should show the upload save bubble and legal footer.
  // (Must wait for response from Payments before accessing the controller.)
  ResetEventWaiterForSequence(
      {DialogEvent::REQUESTED_UPLOAD_SAVE,
       DialogEvent::RECEIVED_GET_UPLOAD_DETAILS_RESPONSE});
  FillAndSubmitForm();
  WaitForObservedEvent();
  EXPECT_TRUE(
      FindViewInBubbleById(DialogViewId::MAIN_CONTENT_VIEW_UPLOAD)->visible());
  EXPECT_TRUE(FindViewInBubbleById(DialogViewId::FOOTNOTE_VIEW)->visible());

  // Assert that the cancel button cannot be found.
  EXPECT_FALSE(FindViewInBubbleById(DialogViewId::CANCEL_BUTTON));
}

// Tests the upload save bubble. Ensures that clicking the top-right [X] close
// button successfully causes the bubble to go away.
IN_PROC_BROWSER_TEST_F(SaveCardBubbleViewsFullFormBrowserTest,
                       Upload_ClickingCloseClosesBubbleIfSecondaryUiMdExpOn) {
  // Enable the SecondaryUiMd experiment.
  scoped_feature_list_.InitAndEnableFeature(features::kSecondaryUiMd);

  // Set up the Payments RPC.
  SetUploadDetailsRpcPaymentsAccepts();

  // Submitting the form should show the upload save bubble and legal footer.
  // (Must wait for response from Payments before accessing the controller.)
  ResetEventWaiterForSequence(
      {DialogEvent::REQUESTED_UPLOAD_SAVE,
       DialogEvent::RECEIVED_GET_UPLOAD_DETAILS_RESPONSE});
  FillAndSubmitForm();
  WaitForObservedEvent();
  EXPECT_TRUE(
      FindViewInBubbleById(DialogViewId::MAIN_CONTENT_VIEW_UPLOAD)->visible());
  EXPECT_TRUE(FindViewInBubbleById(DialogViewId::FOOTNOTE_VIEW)->visible());

  // Clicking the [X] close button should dismiss the bubble.
  base::HistogramTester histogram_tester;
  content::TestNavigationObserver nav_observer(GetActiveWebContents(), 1);
  ClickOnDialogView(
      GetSaveCardBubbleViews()->GetBubbleFrameView()->GetCloseButtonForTest());
  // The bubble should be closed.
  // (Must wait for page navigation to complete before checking.)
  nav_observer.Wait();
  EXPECT_FALSE(GetSaveCardBubbleViews());
}

// Tests the upload save bubble. Ensures that the upload save version of the
// bubble does not have a [Learn more] link.
IN_PROC_BROWSER_TEST_F(SaveCardBubbleViewsFullFormBrowserTest,
                       Upload_ShouldNotHaveLearnMoreLink) {
  // Set up the Payments RPC.
  SetUploadDetailsRpcPaymentsAccepts();

  // Submitting the form should show the upload save bubble and legal footer.
  // (Must wait for response from Payments before accessing the controller.)
  ResetEventWaiterForSequence(
      {DialogEvent::REQUESTED_UPLOAD_SAVE,
       DialogEvent::RECEIVED_GET_UPLOAD_DETAILS_RESPONSE});
  FillAndSubmitForm();
  WaitForObservedEvent();
  EXPECT_TRUE(
      FindViewInBubbleById(DialogViewId::MAIN_CONTENT_VIEW_UPLOAD)->visible());
  EXPECT_TRUE(FindViewInBubbleById(DialogViewId::FOOTNOTE_VIEW)->visible());

  // Assert that the Learn more link cannot be found.
  EXPECT_FALSE(FindViewInBubbleById(DialogViewId::LEARN_MORE_LINK));
}

// TODO(jsaul): Only *part* of the legal message StyledLabel is clickable, and
//              the NOTREACHED() in SaveCardBubbleViews::StyledLabelLinkClicked
//              prevents us from being able to click it unless we know the exact
//              gfx::Range of the link. When/if that can be worked around,
//              create an Upload_ClickingTosLinkClosesBubble test.

// Tests the upload save logic. Ensures that Chrome delegates the offer-to-save
// call to Payments when the detected values experiment is on, and offers to
// upload save the card if Payments allows it.
IN_PROC_BROWSER_TEST_F(
    SaveCardBubbleViewsFullFormBrowserTest,
    Logic_DetectedValuesOn_CanOfferToSaveEvenIfNothingFoundIfPaymentsAccepts) {
  // Enable the SendDetectedValues experiment.
  scoped_feature_list_.InitAndEnableFeature(
      kAutofillUpstreamSendDetectedValues);

  // Set up the Payments RPC.
  SetUploadDetailsRpcPaymentsAccepts();

  // Submitting the form, even with only card number and expiration date, should
  // start the flow of asking Payments if Chrome should offer to save the card
  // to Google. In this case, Payments says yes, and the offer to save bubble
  // should be shown.
  ResetEventWaiterForSequence(
      {DialogEvent::REQUESTED_UPLOAD_SAVE,
       DialogEvent::RECEIVED_GET_UPLOAD_DETAILS_RESPONSE});
  FillAndSubmitFormWithCardDetailsOnly();
  WaitForObservedEvent();
  EXPECT_TRUE(
      FindViewInBubbleById(DialogViewId::MAIN_CONTENT_VIEW_UPLOAD)->visible());
  EXPECT_TRUE(FindViewInBubbleById(DialogViewId::FOOTNOTE_VIEW)->visible());
}

// Tests the upload save logic. Ensures that Chrome delegates the offer-to-save
// call to Payments when the detected values experiment is on, and still does
// not surface the offer to upload save dialog if Payments declines it.
IN_PROC_BROWSER_TEST_F(
    SaveCardBubbleViewsFullFormBrowserTest,
    Logic_DetectedValuesOn_ShouldNotOfferToSaveIfNothingFoundAndPaymentsDeclines) {
  // Enable the SendDetectedValues experiment.
  scoped_feature_list_.InitAndEnableFeature(
      kAutofillUpstreamSendDetectedValues);

  // Set up the Payments RPC.
  SetUploadDetailsRpcPaymentsDeclines();

  // Submitting the form, even with only card number and expiration date, should
  // start the flow of asking Payments if Chrome should offer to save the card
  // to Google. In this case, Payments says no, so the offer to save bubble
  // should not be shown.
  ResetEventWaiterForSequence(
      {DialogEvent::REQUESTED_UPLOAD_SAVE,
       DialogEvent::RECEIVED_GET_UPLOAD_DETAILS_RESPONSE});
  FillAndSubmitFormWithCardDetailsOnly();
  WaitForObservedEvent();
  EXPECT_FALSE(GetSaveCardBubbleViews());
}

// Tests the upload save logic. Ensures that credit card upload is offered if
// name, address, and CVC are detected.
IN_PROC_BROWSER_TEST_F(
    SaveCardBubbleViewsFullFormBrowserTest,
    Logic_DetectedValuesOff_ShouldAttemptToOfferToSaveIfEverythingFound) {
  // Disable the SendDetectedValues experiment.
  scoped_feature_list_.InitAndDisableFeature(
      kAutofillUpstreamSendDetectedValues);

  // Submitting the form should start the flow of asking Payments if Chrome
  // should offer to save the card to Google.
  ResetEventWaiterForSequence({DialogEvent::REQUESTED_UPLOAD_SAVE});
  FillAndSubmitForm();
  WaitForObservedEvent();
}

// Tests the upload save logic. Ensures that credit card upload is offered even
// if street addresses conflict, as long as their postal codes are the same.
IN_PROC_BROWSER_TEST_F(
    SaveCardBubbleViewsFullFormWithShippingBrowserTest,
    Logic_DetectedValuesOff_ShouldAttemptToOfferToSaveIfStreetAddressesConflict) {
  // Disable the SendDetectedValues experiment.
  scoped_feature_list_.InitAndDisableFeature(
      kAutofillUpstreamSendDetectedValues);

  // Submit first shipping address form with a conflicting street address.
  content::TestNavigationObserver shipping_form_nav_observer(
      GetActiveWebContents(), 1);
  FillAndSubmitFormWithConflictingStreetAddress();
  shipping_form_nav_observer.Wait();

  // Submitting the second form should start the flow of asking Payments if
  // Chrome should offer to save the Google, because conflicting street
  // addresses with the same postal code are allowable.
  ResetEventWaiterForSequence({DialogEvent::REQUESTED_UPLOAD_SAVE});
  FillAndSubmitForm();
  WaitForObservedEvent();
}

// Tests the upload save logic. Ensures that credit card upload is not offered
// if CVC is not detected.
IN_PROC_BROWSER_TEST_F(
    SaveCardBubbleViewsFullFormBrowserTest,
    Logic_DetectedValuesOff_ShouldNotOfferToSaveIfCvcNotFound) {
  // Disable the SendDetectedValues experiment.
  scoped_feature_list_.InitAndDisableFeature(
      kAutofillUpstreamSendDetectedValues);

  // Submitting the form should not show the upload save bubble because CVC is
  // missing.
  ResetEventWaiterForSequence({DialogEvent::DID_NOT_REQUEST_UPLOAD_SAVE});
  FillAndSubmitFormWithoutCvc();
  WaitForObservedEvent();
}

// Tests the upload save logic. Ensures that Chrome lets Payments decide whether
// upload save should be offered if the detected values experiment is on, even
// if CVC is not detected.
IN_PROC_BROWSER_TEST_F(
    SaveCardBubbleViewsFullFormBrowserTest,
    Logic_DetectedValuesOn_ShouldAttemptToOfferToSaveIfCvcNotFound) {
  // Enable the SendDetectedValues experiment.
  scoped_feature_list_.InitAndEnableFeature(
      kAutofillUpstreamSendDetectedValues);

  // Submitting the form should still start the flow of asking Payments if
  // Chrome should offer to save the card to Google, even though CVC is missing.
  ResetEventWaiterForSequence({DialogEvent::REQUESTED_UPLOAD_SAVE});
  FillAndSubmitFormWithoutCvc();
  WaitForObservedEvent();
}

// Tests the upload save logic. Ensures that credit card upload is not offered
// if an invalid CVC is detected.
IN_PROC_BROWSER_TEST_F(
    SaveCardBubbleViewsFullFormBrowserTest,
    Logic_DetectedValuesOff_ShouldNotOfferToSaveIfInvalidCvcFound) {
  // Disable the SendDetectedValues experiment.
  scoped_feature_list_.InitAndDisableFeature(
      kAutofillUpstreamSendDetectedValues);

  // Submitting the form should not show the upload save bubble because CVC is
  // invalid.
  ResetEventWaiterForSequence({DialogEvent::DID_NOT_REQUEST_UPLOAD_SAVE});
  FillAndSubmitFormWithInvalidCvc();
  WaitForObservedEvent();
}

// Tests the upload save logic. Ensures that Chrome lets Payments decide whether
// upload save should be offered if the detected values experiment is on, even
// if the detected CVC is invalid.
IN_PROC_BROWSER_TEST_F(
    SaveCardBubbleViewsFullFormBrowserTest,
    Logic_DetectedValuesOn_ShouldAttemptToOfferToSaveIfInvalidCvcFound) {
  // Enable the SendDetectedValues experiment.
  scoped_feature_list_.InitAndEnableFeature(
      kAutofillUpstreamSendDetectedValues);

  // Submitting the form should still start the flow of asking Payments if
  // Chrome should offer to save the card to Google, even though the provided
  // CVC is invalid.
  ResetEventWaiterForSequence({DialogEvent::REQUESTED_UPLOAD_SAVE});
  FillAndSubmitFormWithInvalidCvc();
  WaitForObservedEvent();
}

// Tests the upload save logic. Ensures that credit card upload is not offered
// if address/cardholder name is not detected.
IN_PROC_BROWSER_TEST_F(
    SaveCardBubbleViewsFullFormBrowserTest,
    Logic_DetectedValuesOff_ShouldNotOfferToSaveIfNameNotFound) {
  // Disable the SendDetectedValues experiment.
  scoped_feature_list_.InitAndDisableFeature(
      kAutofillUpstreamSendDetectedValues);

  // Submitting the form should not show the upload save bubble because name is
  // missing.
  ResetEventWaiterForSequence({DialogEvent::DID_NOT_REQUEST_UPLOAD_SAVE});
  FillAndSubmitFormWithoutName();
  WaitForObservedEvent();
}

// Tests the upload save logic. Ensures that Chrome lets Payments decide whether
// upload save should be offered if the detected values experiment is on, even
// if address/cardholder name is not detected.
IN_PROC_BROWSER_TEST_F(
    SaveCardBubbleViewsFullFormBrowserTest,
    Logic_DetectedValuesOn_ShouldAttemptToOfferToSaveIfNameNotFound) {
  // Enable the SendDetectedValues experiment.
  scoped_feature_list_.InitAndEnableFeature(
      kAutofillUpstreamSendDetectedValues);

  // Submitting the form should still start the flow of asking Payments if
  // Chrome should offer to save the card to Google, even though name is
  // missing.
  ResetEventWaiterForSequence({DialogEvent::REQUESTED_UPLOAD_SAVE});
  FillAndSubmitFormWithoutName();
  WaitForObservedEvent();
}

// Tests the upload save logic. Ensures that credit card upload is not offered
// if multiple conflicting names are detected.
IN_PROC_BROWSER_TEST_F(
    SaveCardBubbleViewsFullFormWithShippingBrowserTest,
    Logic_DetectedValuesOff_ShouldNotOfferToSaveIfNamesConflict) {
  // Disable the SendDetectedValues experiment.
  scoped_feature_list_.InitAndDisableFeature(
      kAutofillUpstreamSendDetectedValues);

  // Submit first shipping address form with a conflicting name.
  content::TestNavigationObserver shipping_form_nav_observer(
      GetActiveWebContents(), 1);
  FillAndSubmitFormWithConflictingName();
  shipping_form_nav_observer.Wait();

  // Submitting the second form should not show the upload save bubble because
  // the name conflicts with the previous form.
  ResetEventWaiterForSequence({DialogEvent::DID_NOT_REQUEST_UPLOAD_SAVE});
  FillAndSubmitForm();
  WaitForObservedEvent();
}

// Tests the upload save logic. Ensures that Chrome lets Payments decide whether
// upload save should be offered if the detected values experiment is on, even
// if multiple conflicting names are detected.
IN_PROC_BROWSER_TEST_F(
    SaveCardBubbleViewsFullFormWithShippingBrowserTest,
    Logic_DetectedValuesOn_ShouldAttemptToOfferToSaveIfNamesConflict) {
  // Enable the SendDetectedValues experiment.
  scoped_feature_list_.InitAndEnableFeature(
      kAutofillUpstreamSendDetectedValues);

  // Submit first shipping address form with a conflicting name.
  content::TestNavigationObserver shipping_form_nav_observer(
      GetActiveWebContents(), 1);
  FillAndSubmitFormWithConflictingName();
  shipping_form_nav_observer.Wait();

  // Submitting the form should still start the flow of asking Payments if
  // Chrome should offer to save the card to Google, even though the name
  // conflicts with the previous form.
  ResetEventWaiterForSequence({DialogEvent::REQUESTED_UPLOAD_SAVE});
  FillAndSubmitForm();
  WaitForObservedEvent();
}

// Tests the upload save logic. Ensures that credit card upload is not offered
// if billing address is not detected.
IN_PROC_BROWSER_TEST_F(
    SaveCardBubbleViewsFullFormBrowserTest,
    Logic_DetectedValuesOff_ShouldNotOfferToSaveIfAddressNotFound) {
  // Disable the SendDetectedValues experiment.
  scoped_feature_list_.InitAndDisableFeature(
      kAutofillUpstreamSendDetectedValues);

  // Submitting the form should not show the upload save bubble because address
  // is missing.
  ResetEventWaiterForSequence({DialogEvent::DID_NOT_REQUEST_UPLOAD_SAVE});
  FillAndSubmitFormWithoutAddress();
  WaitForObservedEvent();
}

// Tests the upload save logic. Ensures that Chrome lets Payments decide whether
// upload save should be offered if the detected values experiment is on, even
// if billing address is not detected.
IN_PROC_BROWSER_TEST_F(
    SaveCardBubbleViewsFullFormBrowserTest,
    Logic_DetectedValuesOn_ShouldAttemptToOfferToSaveIfAddressNotFound) {
  // Enable the SendDetectedValues experiment.
  scoped_feature_list_.InitAndEnableFeature(
      kAutofillUpstreamSendDetectedValues);

  // Submitting the form should still start the flow of asking Payments if
  // Chrome should offer to save the card to Google, even though billing address
  // is missing.
  ResetEventWaiterForSequence({DialogEvent::REQUESTED_UPLOAD_SAVE});
  FillAndSubmitFormWithoutAddress();
  WaitForObservedEvent();
}

// Tests the upload save logic. Ensures that credit card upload is not offered
// if multiple conflicting billing address postal codes are detected.
IN_PROC_BROWSER_TEST_F(
    SaveCardBubbleViewsFullFormWithShippingBrowserTest,
    Logic_DetectedValuesOff_ShouldNotOfferToSaveIfPostalCodesConflict) {
  // Disable the SendDetectedValues experiment.
  scoped_feature_list_.InitAndDisableFeature(
      kAutofillUpstreamSendDetectedValues);

  // Submit first shipping address form with a conflicting postal code.
  content::TestNavigationObserver shipping_form_nav_observer(
      GetActiveWebContents(), 1);
  FillAndSubmitFormWithConflictingPostalCode();
  shipping_form_nav_observer.Wait();

  // Submitting the second form should not show the upload save bubble because
  // the postal code conflicts with the previous form.
  ResetEventWaiterForSequence({DialogEvent::DID_NOT_REQUEST_UPLOAD_SAVE});
  FillAndSubmitForm();
  WaitForObservedEvent();
}

// Tests the upload save logic. Ensures that Chrome lets Payments decide whether
// upload save should be offered if the detected values experiment is on, even
// if multiple conflicting billing address postal codes are detected.
IN_PROC_BROWSER_TEST_F(
    SaveCardBubbleViewsFullFormWithShippingBrowserTest,
    Logic_DetectedValuesOn_ShouldAttemptToOfferToSaveIfPostalCodesConflict) {
  // Enable the SendDetectedValues experiment.
  scoped_feature_list_.InitAndEnableFeature(
      kAutofillUpstreamSendDetectedValues);

  // Submit first shipping address form with a conflicting postal code.
  content::TestNavigationObserver shipping_form_nav_observer(
      GetActiveWebContents(), 1);
  FillAndSubmitFormWithConflictingPostalCode();
  shipping_form_nav_observer.Wait();

  // Submitting the form should still start the flow of asking Payments if
  // Chrome should offer to save the card to Google, even though the postal code
  // conflicts with the previous form.
  ResetEventWaiterForSequence({DialogEvent::REQUESTED_UPLOAD_SAVE});
  FillAndSubmitForm();
  WaitForObservedEvent();
}

// Tests UMA logging for the upload save bubble. Ensures that if the user
// declines upload, Autofill.UploadAcceptedCardOrigin is not logged.
IN_PROC_BROWSER_TEST_F(
    SaveCardBubbleViewsFullFormBrowserTest,
    Upload_DecliningUploadDoesNotLogUserAcceptedCardOriginUMA) {
  // Enable the SecondaryUiMd experiment (required for clicking the Close
  // button).
  scoped_feature_list_.InitAndEnableFeature(features::kSecondaryUiMd);

  // Set up the Payments RPC.
  SetUploadDetailsRpcPaymentsAccepts();

  // Submitting the form should show the upload save bubble and legal footer.
  // (Must wait for response from Payments before accessing the controller.)
  base::HistogramTester histogram_tester;
  ResetEventWaiterForSequence(
      {DialogEvent::REQUESTED_UPLOAD_SAVE,
       DialogEvent::RECEIVED_GET_UPLOAD_DETAILS_RESPONSE});
  FillAndSubmitForm();
  WaitForObservedEvent();
  EXPECT_TRUE(
      FindViewInBubbleById(DialogViewId::MAIN_CONTENT_VIEW_UPLOAD)->visible());
  EXPECT_TRUE(FindViewInBubbleById(DialogViewId::FOOTNOTE_VIEW)->visible());

  // Clicking the [X] close button should dismiss the bubble.
  content::TestNavigationObserver nav_observer(GetActiveWebContents(), 1);
  ClickOnDialogView(
      GetSaveCardBubbleViews()->GetBubbleFrameView()->GetCloseButtonForTest());

  // Ensure that UMA was logged correctly.
  histogram_tester.ExpectUniqueSample(
      "Autofill.UploadOfferedCardOrigin",
      AutofillMetrics::OFFERING_UPLOAD_OF_NEW_CARD, 1);
  histogram_tester.ExpectTotalCount("Autofill.UploadAcceptedCardOrigin", 0);
}

}  // namespace autofill
