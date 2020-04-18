// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ssl/insecure_sensitive_input_driver.h"

#include <memory>
#include <string>

#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "components/security_state/content/ssl_status_input_event_data.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/ssl_status.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/navigation_simulator.h"
#include "content/public/test/web_contents_tester.h"

namespace {

class InsecureSensitiveInputDriverTest
    : public ChromeRenderViewHostTestHarness {
 protected:
  InsecureSensitiveInputDriverTest() {}

  ~InsecureSensitiveInputDriverTest() override {}

  security_state::InsecureInputEventData GetInputEvents(
      content::NavigationEntry* entry) {
    security_state::SSLStatusInputEventData* input_events =
        static_cast<security_state::SSLStatusInputEventData*>(
            entry->GetSSL().user_data.get());
    if (input_events)
      return *input_events->input_events();

    return security_state::InsecureInputEventData();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(InsecureSensitiveInputDriverTest);
};

// Tests that password visibility notifications are forwarded.
TEST_F(InsecureSensitiveInputDriverTest, PasswordVisibility) {
  std::unique_ptr<InsecureSensitiveInputDriver> driver(
      new InsecureSensitiveInputDriver(main_rfh()));

  // Do a mock navigation so that there is a navigation entry on which
  // password visibility gets recorded.
  GURL url("http://example.test");
  NavigateAndCommit(url);
  content::NavigationEntry* entry =
      web_contents()->GetController().GetVisibleEntry();
  ASSERT_TRUE(entry);
  EXPECT_EQ(url, entry->GetURL());
  EXPECT_FALSE(GetInputEvents(entry).password_field_shown);

  driver->PasswordFieldVisibleInInsecureContext();

  // Check that the password visibility notification was passed on.
  entry = web_contents()->GetController().GetVisibleEntry();
  ASSERT_TRUE(entry);
  EXPECT_EQ(url, entry->GetURL());
  EXPECT_TRUE(GetInputEvents(entry).password_field_shown);

  // If the password field becomes hidden, then the flag should be unset
  // on the SSLStatus.
  driver->AllPasswordFieldsInInsecureContextInvisible();
  EXPECT_FALSE(GetInputEvents(entry).password_field_shown);
}

// Tests that password visibility notifications from subframes are
// recorded correctly.
TEST_F(InsecureSensitiveInputDriverTest, PasswordVisibilityWithSubframe) {
  // Do a mock navigation so that there is a navigation entry on which
  // password visibility gets recorded.
  GURL url("http://example.test");
  NavigateAndCommit(url);

  // Create a subframe with a password field and check that
  // notifications for it are handled properly.
  content::RenderFrameHost* subframe =
      content::RenderFrameHostTester::For(main_rfh())->AppendChild("child");
  subframe = content::NavigationSimulator::NavigateAndCommitFromDocument(
      GURL("http://example2.test"), subframe);
  auto subframe_driver =
      std::make_unique<InsecureSensitiveInputDriver>(subframe);
  subframe_driver->PasswordFieldVisibleInInsecureContext();

  content::NavigationEntry* entry =
      web_contents()->GetController().GetVisibleEntry();
  ASSERT_TRUE(entry);
  EXPECT_EQ(url, entry->GetURL());
  EXPECT_TRUE(GetInputEvents(entry).password_field_shown);

  subframe_driver->AllPasswordFieldsInInsecureContextInvisible();
  EXPECT_FALSE(GetInputEvents(entry).password_field_shown);
}

// Tests that password visibility notifications are recorded correctly
// when there is a password field in both the main frame and a subframe.
TEST_F(InsecureSensitiveInputDriverTest,
       PasswordVisibilityWithMainFrameAndSubframe) {
  std::unique_ptr<InsecureSensitiveInputDriver> driver(
      new InsecureSensitiveInputDriver(main_rfh()));
  // Do a mock navigation so that there is a navigation entry on which
  // password visibility gets recorded.
  GURL url("http://example.test");
  NavigateAndCommit(url);

  driver->PasswordFieldVisibleInInsecureContext();
  content::NavigationEntry* entry =
      web_contents()->GetController().GetVisibleEntry();
  ASSERT_TRUE(entry);
  EXPECT_EQ(url, entry->GetURL());
  EXPECT_TRUE(GetInputEvents(entry).password_field_shown);

  // Create a subframe with a password field and check that
  // notifications for it are handled properly.
  content::RenderFrameHost* subframe =
      content::RenderFrameHostTester::For(main_rfh())->AppendChild("child");
  subframe = content::NavigationSimulator::NavigateAndCommitFromDocument(
      GURL("http://example2.test"), subframe);
  auto subframe_driver =
      std::make_unique<InsecureSensitiveInputDriver>(subframe);
  subframe_driver->PasswordFieldVisibleInInsecureContext();

  entry = web_contents()->GetController().GetVisibleEntry();
  ASSERT_TRUE(entry);
  EXPECT_EQ(url, entry->GetURL());
  EXPECT_TRUE(GetInputEvents(entry).password_field_shown);

  subframe_driver->AllPasswordFieldsInInsecureContextInvisible();
  EXPECT_TRUE(GetInputEvents(entry).password_field_shown);

  driver->AllPasswordFieldsInInsecureContextInvisible();
  EXPECT_FALSE(GetInputEvents(entry).password_field_shown);
}

// Tests that when a frame is deleted, its password visibility flag gets
// unset.
TEST_F(InsecureSensitiveInputDriverTest,
       PasswordVisibilityWithSubframeDeleted) {
  // Do a mock navigation so that there is a navigation entry on which
  // password visibility gets recorded.
  GURL url("http://example.test");
  NavigateAndCommit(url);

  // Create a subframe with a password field.
  content::RenderFrameHost* subframe =
      content::RenderFrameHostTester::For(main_rfh())->AppendChild("child");
  subframe = content::NavigationSimulator::NavigateAndCommitFromDocument(
      GURL("http://example2.test"), subframe);
  auto subframe_driver =
      std::make_unique<InsecureSensitiveInputDriver>(subframe);
  content::RenderFrameHostTester* subframe_tester =
      content::RenderFrameHostTester::For(subframe);
  subframe_driver->PasswordFieldVisibleInInsecureContext();

  content::NavigationEntry* entry =
      web_contents()->GetController().GetVisibleEntry();
  ASSERT_TRUE(entry);
  EXPECT_EQ(url, entry->GetURL());
  EXPECT_TRUE(GetInputEvents(entry).password_field_shown);

  subframe_tester->Detach();
  EXPECT_FALSE(GetInputEvents(entry).password_field_shown);

  // Check that the subframe's flag isn't hanging around preventing the
  // warning from being removed.
  std::unique_ptr<InsecureSensitiveInputDriver> driver(
      new InsecureSensitiveInputDriver(main_rfh()));
  driver->PasswordFieldVisibleInInsecureContext();
  EXPECT_TRUE(GetInputEvents(entry).password_field_shown);
  driver->AllPasswordFieldsInInsecureContextInvisible();
  EXPECT_FALSE(GetInputEvents(entry).password_field_shown);
}

// Tests that a cross-process navigation does not remove the password
// field flag when the RenderFrameHost for the original process goes
// away. Regression test for https://crbug.com/664674
TEST_F(InsecureSensitiveInputDriverTest,
       RenderFrameHostDeletionOnCrossProcessNavigation) {
  NavigateAndCommit(GURL("http://example.test"));

  content::RenderFrameHostTester* old_rfh_tester =
      content::RenderFrameHostTester::For(web_contents()->GetMainFrame());
  content::WebContentsTester* tester =
      content::WebContentsTester::For(web_contents());

  controller().LoadURL(GURL("http://example2.test"), content::Referrer(),
                       ui::PAGE_TRANSITION_TYPED, std::string());
  content::RenderFrameHost* pending_rfh = pending_main_rfh();
  ASSERT_TRUE(pending_rfh);
  int entry_id = controller().GetPendingEntry()->GetUniqueID();
  tester->TestDidNavigate(pending_rfh, entry_id, true,
                          GURL("http://example2.test"),
                          ui::PAGE_TRANSITION_TYPED);

  auto driver = std::make_unique<InsecureSensitiveInputDriver>(main_rfh());
  driver->PasswordFieldVisibleInInsecureContext();
  content::NavigationEntry* entry =
      web_contents()->GetController().GetVisibleEntry();
  ASSERT_TRUE(entry);
  EXPECT_TRUE(GetInputEvents(entry).password_field_shown);

  // After the old RenderFrameHost is deleted, the password flag should still be
  // set.
  old_rfh_tester->SimulateSwapOutACK();
  EXPECT_TRUE(GetInputEvents(entry).password_field_shown);
}

// Tests that field edit notifications are forwarded.
TEST_F(InsecureSensitiveInputDriverTest, FieldEdit) {
  std::unique_ptr<InsecureSensitiveInputDriver> driver(
      new InsecureSensitiveInputDriver(main_rfh()));

  // Do a mock navigation so that there is a navigation entry on which the
  // field edit gets recorded.
  GURL url("http://example.test");
  NavigateAndCommit(url);
  content::NavigationEntry* entry =
      web_contents()->GetController().GetVisibleEntry();
  ASSERT_TRUE(entry);
  EXPECT_FALSE(GetInputEvents(entry).insecure_field_edited);

  driver->DidEditFieldInInsecureContext();

  // Check that the field edit notification was passed on.
  entry = web_contents()->GetController().GetVisibleEntry();
  ASSERT_TRUE(entry);
  EXPECT_EQ(url, entry->GetURL());
  EXPECT_TRUE(GetInputEvents(entry).insecure_field_edited);
}

// Tests that field edit notifications from subframes are
// recorded correctly.
TEST_F(InsecureSensitiveInputDriverTest, FieldEditWithSubframe) {
  // Do a mock navigation so that there is a navigation entry on which
  // password visibility gets recorded.
  GURL url("http://example.test");
  NavigateAndCommit(url);
  content::NavigationEntry* entry =
      web_contents()->GetController().GetVisibleEntry();
  ASSERT_TRUE(entry);
  EXPECT_FALSE(GetInputEvents(entry).insecure_field_edited);

  // Create a subframe and check that notifications for field edits
  // are handled properly.
  content::RenderFrameHost* subframe =
      content::RenderFrameHostTester::For(main_rfh())->AppendChild("child");
  subframe = content::NavigationSimulator::NavigateAndCommitFromDocument(
      GURL("http://example2.test"), subframe);
  auto subframe_driver =
      std::make_unique<InsecureSensitiveInputDriver>(subframe);
  subframe_driver->DidEditFieldInInsecureContext();

  EXPECT_EQ(url, entry->GetURL());
  EXPECT_TRUE(GetInputEvents(entry).insecure_field_edited);
}

}  // namespace
