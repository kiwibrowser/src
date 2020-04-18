// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "base/values.h"
#include "chrome/browser/extensions/api/web_navigation/frame_navigation_state.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/testing_profile.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {

class FrameNavigationStateTest : public ChromeRenderViewHostTestHarness {
 public:
  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();

    content::RenderFrameHostTester::For(main_rfh())
        ->InitializeRenderFrameIfNeeded();
  }

 protected:
  FrameNavigationStateTest() {}
  ~FrameNavigationStateTest() override {}

  FrameNavigationState navigation_state_;

 private:
  DISALLOW_COPY_AND_ASSIGN(FrameNavigationStateTest);
};

// Test that a frame is correctly tracked, and removed once the tab contents
// goes away.
TEST_F(FrameNavigationStateTest, TrackFrame) {
  const GURL url1("http://www.google.com/");
  const GURL url2("http://mail.google.com/");

  // Create a main frame.
  EXPECT_FALSE(navigation_state_.CanSendEvents(main_rfh()));
  EXPECT_FALSE(navigation_state_.IsValidFrame(main_rfh()));
  navigation_state_.StartTrackingDocumentLoad(main_rfh(), url1, false, false);
  EXPECT_TRUE(navigation_state_.CanSendEvents(main_rfh()));
  EXPECT_TRUE(navigation_state_.IsValidFrame(main_rfh()));

  // Add a sub frame.
  content::RenderFrameHost* sub_frame =
      content::RenderFrameHostTester::For(main_rfh())->AppendChild("child");
  EXPECT_FALSE(navigation_state_.CanSendEvents(sub_frame));
  EXPECT_FALSE(navigation_state_.IsValidFrame(sub_frame));
  navigation_state_.StartTrackingDocumentLoad(sub_frame, url2, false, false);
  EXPECT_TRUE(navigation_state_.CanSendEvents(sub_frame));
  EXPECT_TRUE(navigation_state_.IsValidFrame(sub_frame));

  // Check frame state.
  EXPECT_EQ(url1, navigation_state_.GetUrl(main_rfh()));
  EXPECT_EQ(url2, navigation_state_.GetUrl(sub_frame));

  // Drop the frames.
  navigation_state_.FrameHostDeleted(sub_frame);
  EXPECT_FALSE(navigation_state_.CanSendEvents(sub_frame));
  EXPECT_FALSE(navigation_state_.IsValidFrame(sub_frame));

  navigation_state_.FrameHostDeleted(main_rfh());
  EXPECT_FALSE(navigation_state_.CanSendEvents(main_rfh()));
  EXPECT_FALSE(navigation_state_.IsValidFrame(main_rfh()));
}

// Test that no events can be sent for a frame after an error occurred, but
// before a new navigation happened in this frame.
TEST_F(FrameNavigationStateTest, ErrorState) {
  const GURL url("http://www.google.com/");

  navigation_state_.StartTrackingDocumentLoad(main_rfh(), url, false, false);
  EXPECT_TRUE(navigation_state_.CanSendEvents(main_rfh()));
  EXPECT_FALSE(navigation_state_.GetErrorOccurredInFrame(main_rfh()));

  // After an error occurred, no further events should be sent.
  navigation_state_.SetErrorOccurredInFrame(main_rfh());
  EXPECT_FALSE(navigation_state_.CanSendEvents(main_rfh()));
  EXPECT_TRUE(navigation_state_.GetErrorOccurredInFrame(main_rfh()));

  // Navigations to a network error page should be ignored.
  navigation_state_.StartTrackingDocumentLoad(main_rfh(), GURL(), false, true);
  EXPECT_FALSE(navigation_state_.CanSendEvents(main_rfh()));
  EXPECT_TRUE(navigation_state_.GetErrorOccurredInFrame(main_rfh()));

  // However, when the frame navigates again, it should send events again.
  navigation_state_.StartTrackingDocumentLoad(main_rfh(), url, false, false);
  EXPECT_TRUE(navigation_state_.CanSendEvents(main_rfh()));
  EXPECT_FALSE(navigation_state_.GetErrorOccurredInFrame(main_rfh()));
}

// Tests that for a sub frame, no events are send after an error occurred, but
// before a new navigation happened in this frame.
TEST_F(FrameNavigationStateTest, ErrorStateFrame) {
  const GURL url("http://www.google.com/");

  content::RenderFrameHost* sub_frame =
      content::RenderFrameHostTester::For(main_rfh())->AppendChild("child");
  navigation_state_.StartTrackingDocumentLoad(main_rfh(), url, false, false);
  navigation_state_.StartTrackingDocumentLoad(sub_frame, url, false, false);
  EXPECT_TRUE(navigation_state_.CanSendEvents(main_rfh()));
  EXPECT_TRUE(navigation_state_.CanSendEvents(sub_frame));

  // After an error occurred, no further events should be sent.
  navigation_state_.SetErrorOccurredInFrame(sub_frame);
  EXPECT_TRUE(navigation_state_.CanSendEvents(main_rfh()));
  EXPECT_FALSE(navigation_state_.CanSendEvents(sub_frame));

  // Navigations to a network error page should be ignored.
  navigation_state_.StartTrackingDocumentLoad(sub_frame, GURL(), false, true);
  EXPECT_TRUE(navigation_state_.CanSendEvents(main_rfh()));
  EXPECT_FALSE(navigation_state_.CanSendEvents(sub_frame));

  // However, when the frame navigates again, it should send events again.
  navigation_state_.StartTrackingDocumentLoad(sub_frame, url, false, false);
  EXPECT_TRUE(navigation_state_.CanSendEvents(main_rfh()));
  EXPECT_TRUE(navigation_state_.CanSendEvents(sub_frame));
}

// Tests that no events are send for a not web-safe scheme.
TEST_F(FrameNavigationStateTest, WebSafeScheme) {
  const GURL url("unsafe://www.google.com/");

  navigation_state_.StartTrackingDocumentLoad(main_rfh(), url, false, false);
  EXPECT_FALSE(navigation_state_.CanSendEvents(main_rfh()));
}

// Test for <iframe srcdoc=""> frames.
TEST_F(FrameNavigationStateTest, SrcDoc) {
  const GURL url("http://www.google.com/");
  const GURL blank("about:blank");
  const GURL srcdoc("about:srcdoc");

  content::RenderFrameHost* sub_frame =
      content::RenderFrameHostTester::For(main_rfh())->AppendChild("child");
  navigation_state_.StartTrackingDocumentLoad(main_rfh(), url, false, false);
  navigation_state_.StartTrackingDocumentLoad(sub_frame, srcdoc, false, false);

  EXPECT_TRUE(navigation_state_.CanSendEvents(main_rfh()));
  EXPECT_TRUE(navigation_state_.CanSendEvents(sub_frame));

  EXPECT_EQ(url, navigation_state_.GetUrl(main_rfh()));
  EXPECT_EQ(srcdoc, navigation_state_.GetUrl(sub_frame));

  EXPECT_TRUE(navigation_state_.IsValidUrl(srcdoc));
}

// Test that an individual frame can be detached.
TEST_F(FrameNavigationStateTest, DetachFrame) {
  const GURL url1("http://www.google.com/");
  const GURL url2("http://mail.google.com/");

  // Create a main frame.
  EXPECT_FALSE(navigation_state_.CanSendEvents(main_rfh()));
  EXPECT_FALSE(navigation_state_.IsValidFrame(main_rfh()));
  navigation_state_.StartTrackingDocumentLoad(main_rfh(), url1, false, false);
  EXPECT_TRUE(navigation_state_.CanSendEvents(main_rfh()));
  EXPECT_TRUE(navigation_state_.IsValidFrame(main_rfh()));

  // Add a sub frame.
  content::RenderFrameHost* sub_frame =
      content::RenderFrameHostTester::For(main_rfh())->AppendChild("child");
  EXPECT_FALSE(navigation_state_.CanSendEvents(sub_frame));
  EXPECT_FALSE(navigation_state_.IsValidFrame(sub_frame));
  navigation_state_.StartTrackingDocumentLoad(sub_frame, url2, false, false);
  EXPECT_TRUE(navigation_state_.CanSendEvents(sub_frame));
  EXPECT_TRUE(navigation_state_.IsValidFrame(sub_frame));

  // Check frame state.
  EXPECT_EQ(url1, navigation_state_.GetUrl(main_rfh()));
  EXPECT_EQ(url2, navigation_state_.GetUrl(sub_frame));

  // Drop one frame.
  navigation_state_.FrameHostDeleted(sub_frame);
  EXPECT_EQ(url1, navigation_state_.GetUrl(main_rfh()));
  EXPECT_FALSE(navigation_state_.CanSendEvents(sub_frame));
  EXPECT_FALSE(navigation_state_.IsValidFrame(sub_frame));
}

}  // namespace extensions
