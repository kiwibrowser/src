// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/autofill/save_card_bubble_views_browsertest_base.h"

#include <list>
#include <memory>
#include <string>

#include "chrome/browser/ui/autofill/chrome_autofill_client.h"
#include "chrome/browser/ui/autofill/save_card_bubble_controller_impl.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/autofill/dialog_event_waiter.h"
#include "chrome/browser/ui/views/autofill/dialog_view_ids.h"
#include "chrome/browser/ui/views/autofill/save_card_bubble_views.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/autofill/content/browser/content_autofill_driver.h"
#include "components/autofill/core/browser/credit_card_save_manager.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/test/browser_test_utils.h"
#include "device/geolocation/public/cpp/scoped_geolocation_overrider.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "ui/events/base_event_utils.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/window/dialog_client_view.h"

namespace autofill {

namespace {

const char kURLGetUploadDetailsRequest[] =
    "https://payments.google.com/payments/apis/chromepaymentsservice/"
    "getdetailsforsavecard";
const char kResponseGetUploadDetailsSuccess[] =
    "{\"legal_message\":{\"line\":[{\"template\":\"Legal message template with "
    "link: "
    "{0}.\",\"template_parameter\":[{\"display_text\":\"Link\",\"url\":\"https:"
    "//www.example.com/\"}]}]},\"context_token\":\"dummy_context_token\"}";
const char kResponseGetUploadDetailsFailure[] =
    "{\"error\":{\"code\":\"FAILED_PRECONDITION\",\"user_error_message\":\"An "
    "unexpected error has occurred. Please try again later.\"}}";

const double kFakeGeolocationLatitude = 1.23;
const double kFakeGeolocationLongitude = 4.56;

}  // namespace

SaveCardBubbleViewsBrowserTestBase::SaveCardBubbleViewsBrowserTestBase(
    const std::string& test_file_path)
    : test_file_path_(test_file_path) {}

SaveCardBubbleViewsBrowserTestBase::~SaveCardBubbleViewsBrowserTestBase() {}

void SaveCardBubbleViewsBrowserTestBase::SetUpOnMainThread() {
  // Set up the HTTPS server (uses the embedded_test_server).
  ASSERT_TRUE(embedded_test_server()->InitializeAndListen());
  embedded_test_server()->ServeFilesFromSourceDirectory(
      "components/test/data/autofill");
  embedded_test_server()->StartAcceptingConnections();

  // Set up the URL fetcher. By providing an Impl, unexpected calls (sync, etc.)
  // won't cause the test to crash.
  url_fetcher_factory_ = std::make_unique<net::FakeURLFetcherFactory>(
      new net::URLFetcherImplFactory());

  // Set up this class as the ObserverForTest implementation.
  CreditCardSaveManager* credit_card_save_manager =
      ContentAutofillDriver::GetForRenderFrameHost(
          GetActiveWebContents()->GetMainFrame())
          ->autofill_manager()
          ->form_data_importer_.get()
          ->credit_card_save_manager_.get();
  credit_card_save_manager->SetEventObserverForTesting(this);

  // Set up the fake geolocation data.
  geolocation_overrider_ = std::make_unique<device::ScopedGeolocationOverrider>(
      kFakeGeolocationLatitude, kFakeGeolocationLongitude);

  NavigateTo(test_file_path_);
}

void SaveCardBubbleViewsBrowserTestBase::NavigateTo(
    const std::string& file_path) {
  if (file_path.find("data:") == 0U) {
    ui_test_utils::NavigateToURL(browser(), GURL(file_path));
  } else {
    ui_test_utils::NavigateToURL(browser(),
                                 embedded_test_server()->GetURL(file_path));
  }
}

void SaveCardBubbleViewsBrowserTestBase::OnOfferLocalSave() {
  if (event_waiter_)
    event_waiter_->OnEvent(DialogEvent::OFFERED_LOCAL_SAVE);
}

void SaveCardBubbleViewsBrowserTestBase::OnDecideToRequestUploadSave() {
  if (event_waiter_)
    event_waiter_->OnEvent(DialogEvent::REQUESTED_UPLOAD_SAVE);
}

void SaveCardBubbleViewsBrowserTestBase::OnDecideToNotRequestUploadSave() {
  if (event_waiter_)
    event_waiter_->OnEvent(DialogEvent::DID_NOT_REQUEST_UPLOAD_SAVE);
}

void SaveCardBubbleViewsBrowserTestBase::OnReceivedGetUploadDetailsResponse() {
  if (event_waiter_)
    event_waiter_->OnEvent(DialogEvent::RECEIVED_GET_UPLOAD_DETAILS_RESPONSE);
}

void SaveCardBubbleViewsBrowserTestBase::OnSentUploadCardRequest() {
  if (event_waiter_)
    event_waiter_->OnEvent(DialogEvent::SENT_UPLOAD_CARD_REQUEST);
}

// Should be called for credit_card_upload_form_address_and_cc.html.
void SaveCardBubbleViewsBrowserTestBase::FillAndSubmitForm() {
  content::WebContents* web_contents = GetActiveWebContents();
  const std::string click_fill_button_js =
      "(function() { document.getElementById('fill_form').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_fill_button_js));

  const std::string click_submit_button_js =
      "(function() { document.getElementById('submit').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_submit_button_js));
}

void SaveCardBubbleViewsBrowserTestBase::
    FillAndSubmitFormWithCardDetailsOnly() {
  content::WebContents* web_contents = GetActiveWebContents();
  const std::string click_fill_card_button_js =
      "(function() { document.getElementById('fill_card_only').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_fill_card_button_js));

  const std::string click_submit_button_js =
      "(function() { document.getElementById('submit').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_submit_button_js));
}

// Should be called for credit_card_upload_form_address_and_cc.html.
void SaveCardBubbleViewsBrowserTestBase::FillAndSubmitFormWithoutCvc() {
  content::WebContents* web_contents = GetActiveWebContents();
  const std::string click_fill_button_js =
      "(function() { document.getElementById('fill_form').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_fill_button_js));

  const std::string click_clear_cvc_button_js =
      "(function() { document.getElementById('clear_cvc').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_clear_cvc_button_js));

  const std::string click_submit_button_js =
      "(function() { document.getElementById('submit').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_submit_button_js));
}

// Should be called for credit_card_upload_form_address_and_cc.html.
void SaveCardBubbleViewsBrowserTestBase::FillAndSubmitFormWithInvalidCvc() {
  content::WebContents* web_contents = GetActiveWebContents();
  const std::string click_fill_button_js =
      "(function() { document.getElementById('fill_form').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_fill_button_js));

  const std::string click_fill_invalid_cvc_button_js =
      "(function() { document.getElementById('fill_invalid_cvc').click(); "
      "})();";
  ASSERT_TRUE(
      content::ExecuteScript(web_contents, click_fill_invalid_cvc_button_js));

  const std::string click_submit_button_js =
      "(function() { document.getElementById('submit').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_submit_button_js));
}

// Should be called for credit_card_upload_form_address_and_cc.html.
void SaveCardBubbleViewsBrowserTestBase::FillAndSubmitFormWithoutName() {
  content::WebContents* web_contents = GetActiveWebContents();
  const std::string click_fill_button_js =
      "(function() { document.getElementById('fill_form').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_fill_button_js));

  const std::string click_clear_name_button_js =
      "(function() { document.getElementById('clear_name').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_clear_name_button_js));

  const std::string click_submit_button_js =
      "(function() { document.getElementById('submit').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_submit_button_js));
}

// Should be called for credit_card_upload_form_address_and_cc.html.
void SaveCardBubbleViewsBrowserTestBase::FillAndSubmitFormWithoutAddress() {
  content::WebContents* web_contents = GetActiveWebContents();
  const std::string click_fill_button_js =
      "(function() { document.getElementById('fill_form').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_fill_button_js));

  const std::string click_clear_name_button_js =
      "(function() { document.getElementById('clear_address').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_clear_name_button_js));

  const std::string click_submit_button_js =
      "(function() { document.getElementById('submit').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_submit_button_js));
}

// Should be called for credit_card_upload_form_shipping_address.html.
void SaveCardBubbleViewsBrowserTestBase::
    FillAndSubmitFormWithConflictingName() {
  content::WebContents* web_contents = GetActiveWebContents();
  const std::string click_fill_button_js =
      "(function() { document.getElementById('fill_form').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_fill_button_js));

  const std::string click_conflicting_name_button_js =
      "(function() { document.getElementById('conflicting_name').click(); "
      "})();";
  ASSERT_TRUE(
      content::ExecuteScript(web_contents, click_conflicting_name_button_js));

  const std::string click_submit_button_js =
      "(function() { document.getElementById('submit').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_submit_button_js));
}

// Should be called for credit_card_upload_form_shipping_address.html.
void SaveCardBubbleViewsBrowserTestBase::
    FillAndSubmitFormWithConflictingStreetAddress() {
  content::WebContents* web_contents = GetActiveWebContents();
  const std::string click_fill_button_js =
      "(function() { document.getElementById('fill_form').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_fill_button_js));

  const std::string click_conflicting_street_address_button_js =
      "(function() { "
      "document.getElementById('conflicting_street_address').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(
      web_contents, click_conflicting_street_address_button_js));

  const std::string click_submit_button_js =
      "(function() { document.getElementById('submit').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_submit_button_js));
}

// Should be called for credit_card_upload_form_shipping_address.html.
void SaveCardBubbleViewsBrowserTestBase::
    FillAndSubmitFormWithConflictingPostalCode() {
  content::WebContents* web_contents = GetActiveWebContents();
  const std::string click_fill_button_js =
      "(function() { document.getElementById('fill_form').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_fill_button_js));

  const std::string click_conflicting_postal_code_button_js =
      "(function() { "
      "document.getElementById('conflicting_postal_code').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents,
                                     click_conflicting_postal_code_button_js));

  const std::string click_submit_button_js =
      "(function() { document.getElementById('submit').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_submit_button_js));
}

void SaveCardBubbleViewsBrowserTestBase::SetUploadDetailsRpcPaymentsAccepts() {
  url_fetcher_factory_->SetFakeResponse(
      GURL(kURLGetUploadDetailsRequest), kResponseGetUploadDetailsSuccess,
      net::HTTP_OK, net::URLRequestStatus::SUCCESS);
}

void SaveCardBubbleViewsBrowserTestBase::SetUploadDetailsRpcPaymentsDeclines() {
  url_fetcher_factory_->SetFakeResponse(
      GURL(kURLGetUploadDetailsRequest), kResponseGetUploadDetailsFailure,
      net::HTTP_OK, net::URLRequestStatus::SUCCESS);
}

void SaveCardBubbleViewsBrowserTestBase::SetUploadDetailsRpcServerError() {
  url_fetcher_factory_->SetFakeResponse(
      GURL(kURLGetUploadDetailsRequest), kResponseGetUploadDetailsSuccess,
      net::HTTP_INTERNAL_SERVER_ERROR, net::URLRequestStatus::FAILED);
}

void SaveCardBubbleViewsBrowserTestBase::ClickOnDialogView(views::View* view) {
  DCHECK(view);
  ui::MouseEvent pressed(ui::ET_MOUSE_PRESSED, gfx::Point(), gfx::Point(),
                         ui::EventTimeForNow(), ui::EF_LEFT_MOUSE_BUTTON,
                         ui::EF_LEFT_MOUSE_BUTTON);
  view->OnMousePressed(pressed);
  ui::MouseEvent released_event = ui::MouseEvent(
      ui::ET_MOUSE_RELEASED, gfx::Point(), gfx::Point(), ui::EventTimeForNow(),
      ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON);
  view->OnMouseReleased(released_event);
}

void SaveCardBubbleViewsBrowserTestBase::ClickOnDialogViewWithIdAndWait(
    DialogViewId view_id) {
  ClickOnDialogView(FindViewInBubbleById(view_id));
  WaitForObservedEvent();
}

views::View* SaveCardBubbleViewsBrowserTestBase::FindViewInBubbleById(
    DialogViewId view_id) {
  SaveCardBubbleViews* save_card_bubble_views = GetSaveCardBubbleViews();
  DCHECK(save_card_bubble_views);

  views::View* specified_view =
      save_card_bubble_views->GetViewByID(static_cast<int>(view_id));
  if (!specified_view) {
    // Many of the save card bubble's inner Views are not child views but rather
    // contained by its DialogClientView. If we didn't find what we were looking
    // for, check there as well.
    specified_view = save_card_bubble_views->GetDialogClientView()->GetViewByID(
        static_cast<int>(view_id));
  }
  if (!specified_view) {
    // Additionally, the save card bubble's footnote view is not part of its
    // main bubble, and contains elements such as the legal message links.
    // If we didn't find what we were looking for, check there as well.
    views::View* footnote_view =
        save_card_bubble_views->GetFootnoteViewForTesting();
    if (footnote_view) {
      specified_view = footnote_view->GetViewByID(static_cast<int>(view_id));
    }
  }
  return specified_view;
}

SaveCardBubbleViews*
SaveCardBubbleViewsBrowserTestBase::GetSaveCardBubbleViews() {
  SaveCardBubbleControllerImpl* save_card_bubble_controller_impl =
      SaveCardBubbleControllerImpl::FromWebContents(GetActiveWebContents());
  if (!save_card_bubble_controller_impl)
    return nullptr;
  SaveCardBubbleView* save_card_bubble_view =
      save_card_bubble_controller_impl->save_card_bubble_view();
  if (!save_card_bubble_view)
    return nullptr;
  return static_cast<SaveCardBubbleViews*>(save_card_bubble_view);
}

content::WebContents*
SaveCardBubbleViewsBrowserTestBase::GetActiveWebContents() {
  return browser()->tab_strip_model()->GetActiveWebContents();
}

void SaveCardBubbleViewsBrowserTestBase::ResetEventWaiterForSequence(
    std::list<DialogEvent> event_sequence) {
  event_waiter_ = std::make_unique<DialogEventWaiter<DialogEvent>>(
      std::move(event_sequence));
}

void SaveCardBubbleViewsBrowserTestBase::WaitForObservedEvent() {
  event_waiter_->Wait();
}

}  // namespace autofill
