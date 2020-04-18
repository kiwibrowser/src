// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ssl/security_state_tab_helper.h"

#include <string>

#include "base/command_line.h"
#include "base/test/histogram_tester.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "components/security_state/content/ssl_status_input_event_data.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const char kHTTPBadNavigationHistogram[] =
    "Security.HTTPBad.NavigationStartedAfterUserWarnedAboutSensitiveInput";
const char kHTTPBadWebContentsDestroyedHistogram[] =
    "Security.HTTPBad.WebContentsDestroyedAfterUserWarnedAboutSensitiveInput";
const char kFormSubmissionSecurityLevelHistogram[] =
    "Security.SecurityLevel.FormSubmission";

// Gets the Insecure Input Events from the entry's SSLStatus user data.
security_state::InsecureInputEventData GetInputEvents(
    content::NavigationEntry* entry) {
  security_state::SSLStatusInputEventData* input_events =
      static_cast<security_state::SSLStatusInputEventData*>(
          entry->GetSSL().user_data.get());
  if (input_events)
    return *input_events->input_events();

  return security_state::InsecureInputEventData();
}

// Stores the Insecure Input Events to the entry's SSLStatus user data.
void SetInputEvents(content::NavigationEntry* entry,
                    security_state::InsecureInputEventData events) {
  security_state::SSLStatus& ssl = entry->GetSSL();
  security_state::SSLStatusInputEventData* input_events =
      static_cast<security_state::SSLStatusInputEventData*>(
          ssl.user_data.get());
  if (!input_events) {
    ssl.user_data =
        std::make_unique<security_state::SSLStatusInputEventData>(events);
  } else {
    *input_events->input_events() = events;
  }
}

class SecurityStateTabHelperHistogramTest
    : public ChromeRenderViewHostTestHarness,
      public testing::WithParamInterface<bool> {
 public:
  SecurityStateTabHelperHistogramTest() : helper_(nullptr) {}
  ~SecurityStateTabHelperHistogramTest() override {}

  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();

    SecurityStateTabHelper::CreateForWebContents(web_contents());
    helper_ = SecurityStateTabHelper::FromWebContents(web_contents());
    NavigateToHTTP();
  }

 protected:
  void SignalSensitiveInput() {
    content::NavigationEntry* entry =
        web_contents()->GetController().GetVisibleEntry();
    security_state::InsecureInputEventData input_events = GetInputEvents(entry);
    if (GetParam())
      input_events.password_field_shown = true;
    else
      input_events.credit_card_field_edited = true;
    SetInputEvents(entry, input_events);
    helper_->DidChangeVisibleSecurityState();
  }

  void ClearInputEvents() {
    content::NavigationEntry* entry =
        web_contents()->GetController().GetVisibleEntry();
    SetInputEvents(entry, security_state::InsecureInputEventData());
    helper_->DidChangeVisibleSecurityState();
  }

  const std::string HistogramName() {
    if (GetParam())
      return "Security.HTTPBad.UserWarnedAboutSensitiveInput.Password";
    else
      return "Security.HTTPBad.UserWarnedAboutSensitiveInput.CreditCard";
  }

  void StartFormSubmissionNavigation() {
    std::unique_ptr<content::NavigationHandle> handle =
        content::NavigationHandle::CreateNavigationHandleForTesting(
            GURL("http://example.test"), web_contents()->GetMainFrame(), true,
            net::OK, false, false, ui::PAGE_TRANSITION_LINK, true);
  }

  void NavigateToHTTP() { NavigateAndCommit(GURL("http://example.test")); }

  void NavigateToDifferentHTTPPage() {
    NavigateAndCommit(GURL("http://example2.test"));
  }

 private:
  SecurityStateTabHelper* helper_;
  DISALLOW_COPY_AND_ASSIGN(SecurityStateTabHelperHistogramTest);
};

// Tests that an UMA histogram is recorded after setting the security
// level to HTTP_SHOW_WARNING and navigating away.
TEST_P(SecurityStateTabHelperHistogramTest,
       HTTPOmniboxWarningNavigationHistogram) {
  base::HistogramTester histograms;
  SignalSensitiveInput();
  // Make sure that if the omnibox warning gets dynamically hidden, the
  // histogram still gets recorded.
  NavigateToDifferentHTTPPage();
  if (GetParam()) {
    ClearInputEvents();
  }
  // Destroy the WebContents to simulate the tab being closed after a
  // navigation.
  SetContents(nullptr);
  histograms.ExpectTotalCount(kHTTPBadNavigationHistogram, 1);
  histograms.ExpectTotalCount(kHTTPBadWebContentsDestroyedHistogram, 0);
}

// Tests that an UMA histogram is recorded after setting the security
// level to HTTP_SHOW_WARNING and closing the tab.
TEST_P(SecurityStateTabHelperHistogramTest,
       HTTPOmniboxWarningTabClosedHistogram) {
  base::HistogramTester histograms;
  SignalSensitiveInput();
  // Destroy the WebContents to simulate the tab being closed.
  SetContents(nullptr);
  histograms.ExpectTotalCount(kHTTPBadNavigationHistogram, 0);
  histograms.ExpectTotalCount(kHTTPBadWebContentsDestroyedHistogram, 1);
}

TEST_P(SecurityStateTabHelperHistogramTest, FormSubmissionHistogram) {
  base::HistogramTester histograms;
  StartFormSubmissionNavigation();
  histograms.ExpectUniqueSample(kFormSubmissionSecurityLevelHistogram,
                                security_state::HTTP_SHOW_WARNING, 1);
}

// Tests that UMA logs the omnibox warning when security level is
// HTTP_SHOW_WARNING.
TEST_P(SecurityStateTabHelperHistogramTest, HTTPOmniboxWarningHistogram) {
  base::HistogramTester histograms;
  SignalSensitiveInput();
  histograms.ExpectUniqueSample(HistogramName(), true, 1);

  // Fire again and ensure no sample is recorded.
  SignalSensitiveInput();
  histograms.ExpectUniqueSample(HistogramName(), true, 1);

  // Navigate to a new page and ensure a sample is recorded.
  NavigateToDifferentHTTPPage();
  histograms.ExpectUniqueSample(HistogramName(), true, 1);
  SignalSensitiveInput();
  histograms.ExpectUniqueSample(HistogramName(), true, 2);
}

INSTANTIATE_TEST_CASE_P(SecurityStateTabHelperHistogramTest,
                        SecurityStateTabHelperHistogramTest,
                        // Here 'true' to test password field triggered
                        // histogram and 'false' to test credit card field.
                        testing::Bool());

}  // namespace
