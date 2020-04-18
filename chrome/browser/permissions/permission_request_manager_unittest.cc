// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/histogram_tester.h"
#include "build/build_config.h"
#include "chrome/browser/engagement/site_engagement_service.h"
#include "chrome/browser/permissions/mock_permission_request.h"
#include "chrome/browser/permissions/permission_request.h"
#include "chrome/browser/permissions/permission_request_manager.h"
#include "chrome/browser/permissions/permission_uma_util.h"
#include "chrome/browser/ui/permission_bubble/mock_permission_prompt_factory.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/testing_profile.h"
#include "testing/gtest/include/gtest/gtest.h"

const double kTestEngagementScore = 29;

class PermissionRequestManagerTest : public ChromeRenderViewHostTestHarness {
 public:
  PermissionRequestManagerTest()
      : ChromeRenderViewHostTestHarness(),
        request1_("test1",
                  PermissionRequestType::QUOTA,
                  PermissionRequestGestureType::GESTURE),
        request2_("test2",
                  PermissionRequestType::DOWNLOAD,
                  PermissionRequestGestureType::NO_GESTURE),
        request_mic_("mic",
                     PermissionRequestType::PERMISSION_MEDIASTREAM_MIC,
                     PermissionRequestGestureType::NO_GESTURE),
        request_camera_("cam",
                        PermissionRequestType::PERMISSION_MEDIASTREAM_CAMERA,
                        PermissionRequestGestureType::NO_GESTURE),
        iframe_request_same_domain_(
            "iframe",
            PermissionRequestType::PERMISSION_NOTIFICATIONS,
            GURL("http://www.google.com/some/url")),
        iframe_request_other_domain_(
            "iframe",
            PermissionRequestType::PERMISSION_GEOLOCATION,
            GURL("http://www.youtube.com")),
        iframe_request_mic_other_domain_(
            "iframe",
            PermissionRequestType::PERMISSION_MEDIASTREAM_MIC,
            GURL("http://www.youtube.com")) {}
  ~PermissionRequestManagerTest() override {}

  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();
    SetContents(CreateTestWebContents());
    url_ = GURL("http://www.google.com");
    NavigateAndCommit(url_);

    SiteEngagementService::Get(profile())->ResetBaseScoreForURL(
        url_, kTestEngagementScore);

    PermissionRequestManager::CreateForWebContents(web_contents());
    manager_ = PermissionRequestManager::FromWebContents(web_contents());
    prompt_factory_.reset(new MockPermissionPromptFactory(manager_));
  }

  void TearDown() override {
    prompt_factory_.reset();
    ChromeRenderViewHostTestHarness::TearDown();
  }

  void Accept() {
    manager_->Accept();
  }

  void Deny() {
    manager_->Deny();
  }

  void Closing() {
    manager_->Closing();
  }

  void WaitForFrameLoad() {
    // PermissionRequestManager ignores all parameters. Yay?
    manager_->DocumentLoadedInFrame(NULL);
    base::RunLoop().RunUntilIdle();
  }

  void WaitForBubbleToBeShown() {
    manager_->DocumentOnLoadCompletedInMainFrame();
    base::RunLoop().RunUntilIdle();
  }

  void MockTabSwitchAway() {
    manager_->OnVisibilityChanged(content::Visibility::HIDDEN);
  }

  void MockTabSwitchBack() {
    manager_->OnVisibilityChanged(content::Visibility::VISIBLE);
  }

  virtual void NavigationEntryCommitted(
      const content::LoadCommittedDetails& details) {
    manager_->NavigationEntryCommitted(details);
  }

 protected:
  GURL url_;
  MockPermissionRequest request1_;
  MockPermissionRequest request2_;
  MockPermissionRequest request_mic_;
  MockPermissionRequest request_camera_;
  MockPermissionRequest iframe_request_same_domain_;
  MockPermissionRequest iframe_request_other_domain_;
  MockPermissionRequest iframe_request_mic_other_domain_;
  PermissionRequestManager* manager_;
  std::unique_ptr<MockPermissionPromptFactory> prompt_factory_;
};

TEST_F(PermissionRequestManagerTest, SingleRequest) {
  manager_->AddRequest(&request1_);
  WaitForBubbleToBeShown();

  EXPECT_TRUE(prompt_factory_->is_visible());
  ASSERT_EQ(prompt_factory_->request_count(), 1);

  Accept();
  EXPECT_TRUE(request1_.granted());
}

TEST_F(PermissionRequestManagerTest, SingleRequestViewFirst) {
  manager_->AddRequest(&request1_);
  WaitForBubbleToBeShown();

  EXPECT_TRUE(prompt_factory_->is_visible());
  ASSERT_EQ(prompt_factory_->request_count(), 1);

  Accept();
  EXPECT_TRUE(request1_.granted());
}

// Most requests should never be grouped.
TEST_F(PermissionRequestManagerTest, TwoRequestsUngrouped) {
  manager_->AddRequest(&request1_);
  manager_->AddRequest(&request2_);

  WaitForBubbleToBeShown();
  EXPECT_TRUE(prompt_factory_->is_visible());
  ASSERT_EQ(prompt_factory_->request_count(), 1);
  Accept();
  EXPECT_TRUE(request1_.granted());

  WaitForBubbleToBeShown();
  EXPECT_TRUE(prompt_factory_->is_visible());
  ASSERT_EQ(prompt_factory_->request_count(), 1);
  Accept();
  EXPECT_TRUE(request2_.granted());
}

// Only mic/camera requests from the same origin should be grouped.
TEST_F(PermissionRequestManagerTest, MicCameraGrouped) {
  manager_->AddRequest(&request_mic_);
  manager_->AddRequest(&request_camera_);
  WaitForBubbleToBeShown();

  EXPECT_TRUE(prompt_factory_->is_visible());
  ASSERT_EQ(prompt_factory_->request_count(), 2);

  Accept();
  EXPECT_TRUE(request_mic_.granted());
  EXPECT_TRUE(request_camera_.granted());

  // If the requests come from different origins, they should not be grouped.
  manager_->AddRequest(&iframe_request_mic_other_domain_);
  manager_->AddRequest(&request_camera_);
  WaitForBubbleToBeShown();

  EXPECT_TRUE(prompt_factory_->is_visible());
  ASSERT_EQ(prompt_factory_->request_count(), 1);
}

TEST_F(PermissionRequestManagerTest, TwoRequestsTabSwitch) {
  manager_->AddRequest(&request_mic_);
  manager_->AddRequest(&request_camera_);
  WaitForBubbleToBeShown();

  EXPECT_TRUE(prompt_factory_->is_visible());
  ASSERT_EQ(prompt_factory_->request_count(), 2);

  MockTabSwitchAway();
#if defined(OS_ANDROID)
  EXPECT_TRUE(prompt_factory_->is_visible());
#else
  EXPECT_FALSE(prompt_factory_->is_visible());
#endif

  MockTabSwitchBack();
  WaitForBubbleToBeShown();
  EXPECT_TRUE(prompt_factory_->is_visible());
  ASSERT_EQ(prompt_factory_->request_count(), 2);

  Accept();
  EXPECT_TRUE(request_mic_.granted());
  EXPECT_TRUE(request_camera_.granted());
}

TEST_F(PermissionRequestManagerTest, NoRequests) {
  WaitForBubbleToBeShown();
  EXPECT_FALSE(prompt_factory_->is_visible());
}

TEST_F(PermissionRequestManagerTest, PermissionRequestWhileTabSwitchedAway) {
  MockTabSwitchAway();
  manager_->AddRequest(&request1_);
  WaitForBubbleToBeShown();
  EXPECT_FALSE(prompt_factory_->is_visible());

  MockTabSwitchBack();
  WaitForBubbleToBeShown();
  EXPECT_TRUE(prompt_factory_->is_visible());
}

TEST_F(PermissionRequestManagerTest, TwoRequestsDoNotCoalesce) {
  manager_->AddRequest(&request1_);
  WaitForBubbleToBeShown();
  manager_->AddRequest(&request2_);

  EXPECT_TRUE(prompt_factory_->is_visible());
  ASSERT_EQ(prompt_factory_->request_count(), 1);
}

TEST_F(PermissionRequestManagerTest, TwoRequestsShownInTwoBubbles) {
  manager_->AddRequest(&request1_);
  WaitForBubbleToBeShown();
  manager_->AddRequest(&request2_);

  EXPECT_TRUE(prompt_factory_->is_visible());
  ASSERT_EQ(prompt_factory_->request_count(), 1);

  Accept();
  WaitForBubbleToBeShown();

  EXPECT_TRUE(prompt_factory_->is_visible());
  ASSERT_EQ(prompt_factory_->request_count(), 1);
  ASSERT_EQ(prompt_factory_->show_count(), 2);
}

TEST_F(PermissionRequestManagerTest, TestAddDuplicateRequest) {
  manager_->AddRequest(&request1_);
  manager_->AddRequest(&request1_);

  WaitForBubbleToBeShown();
  EXPECT_TRUE(prompt_factory_->is_visible());
  ASSERT_EQ(prompt_factory_->request_count(), 1);
}

TEST_F(PermissionRequestManagerTest, SequentialRequests) {
  manager_->AddRequest(&request1_);
  WaitForBubbleToBeShown();
  EXPECT_TRUE(prompt_factory_->is_visible());

  Accept();
  EXPECT_TRUE(request1_.granted());

  EXPECT_FALSE(prompt_factory_->is_visible());

  manager_->AddRequest(&request2_);
  WaitForBubbleToBeShown();
  EXPECT_TRUE(prompt_factory_->is_visible());
  Accept();
  EXPECT_FALSE(prompt_factory_->is_visible());
  EXPECT_TRUE(request2_.granted());
}

TEST_F(PermissionRequestManagerTest, SameRequestRejected) {
  manager_->AddRequest(&request1_);
  manager_->AddRequest(&request1_);
  EXPECT_FALSE(request1_.finished());

  WaitForBubbleToBeShown();
  EXPECT_TRUE(prompt_factory_->is_visible());
  ASSERT_EQ(prompt_factory_->request_count(), 1);
}

TEST_F(PermissionRequestManagerTest, DuplicateQueuedRequest) {
  manager_->AddRequest(&request1_);
  WaitForBubbleToBeShown();
  manager_->AddRequest(&request2_);

  MockPermissionRequest dupe_request("test1");
  manager_->AddRequest(&dupe_request);
  EXPECT_FALSE(dupe_request.finished());
  EXPECT_FALSE(request1_.finished());

  MockPermissionRequest dupe_request2("test2");
  manager_->AddRequest(&dupe_request2);
  EXPECT_FALSE(dupe_request2.finished());
  EXPECT_FALSE(request2_.finished());

  Accept();
  EXPECT_TRUE(dupe_request.finished());
  EXPECT_TRUE(request1_.finished());

  WaitForBubbleToBeShown();
  Accept();
  EXPECT_TRUE(dupe_request2.finished());
  EXPECT_TRUE(request2_.finished());
}

TEST_F(PermissionRequestManagerTest, ForgetRequestsOnPageNavigation) {
  manager_->AddRequest(&request1_);
  WaitForBubbleToBeShown();
  manager_->AddRequest(&request2_);
  manager_->AddRequest(&iframe_request_other_domain_);

  EXPECT_TRUE(prompt_factory_->is_visible());
  ASSERT_EQ(prompt_factory_->request_count(), 1);

  NavigateAndCommit(GURL("http://www2.google.com/"));
  WaitForBubbleToBeShown();

  EXPECT_FALSE(prompt_factory_->is_visible());
  EXPECT_TRUE(request1_.finished());
  EXPECT_TRUE(request2_.finished());
  EXPECT_TRUE(iframe_request_other_domain_.finished());
}

TEST_F(PermissionRequestManagerTest, MainFrameNoRequestIFrameRequest) {
  manager_->AddRequest(&iframe_request_same_domain_);
  WaitForBubbleToBeShown();
  WaitForFrameLoad();

  EXPECT_TRUE(prompt_factory_->is_visible());
  Closing();
  EXPECT_TRUE(iframe_request_same_domain_.finished());
}

TEST_F(PermissionRequestManagerTest, MainFrameAndIFrameRequestSameDomain) {
  manager_->AddRequest(&request1_);
  manager_->AddRequest(&iframe_request_same_domain_);
  WaitForFrameLoad();
  WaitForBubbleToBeShown();

  EXPECT_TRUE(prompt_factory_->is_visible());
  ASSERT_EQ(1, prompt_factory_->request_count());
  Closing();
  EXPECT_TRUE(request1_.finished());
  EXPECT_FALSE(iframe_request_same_domain_.finished());
  WaitForBubbleToBeShown();
  EXPECT_TRUE(prompt_factory_->is_visible());
  ASSERT_EQ(1, prompt_factory_->request_count());
  Closing();
  EXPECT_FALSE(prompt_factory_->is_visible());
  EXPECT_TRUE(iframe_request_same_domain_.finished());
}

TEST_F(PermissionRequestManagerTest, MainFrameAndIFrameRequestOtherDomain) {
  manager_->AddRequest(&request1_);
  manager_->AddRequest(&iframe_request_other_domain_);
  WaitForFrameLoad();
  WaitForBubbleToBeShown();

  EXPECT_TRUE(prompt_factory_->is_visible());
  Closing();
  EXPECT_TRUE(request1_.finished());
  EXPECT_FALSE(iframe_request_other_domain_.finished());
  EXPECT_TRUE(prompt_factory_->is_visible());
  Closing();
  EXPECT_TRUE(iframe_request_other_domain_.finished());
}

TEST_F(PermissionRequestManagerTest, IFrameRequestWhenMainRequestVisible) {
  manager_->AddRequest(&request1_);
  WaitForBubbleToBeShown();
  EXPECT_TRUE(prompt_factory_->is_visible());

  manager_->AddRequest(&iframe_request_same_domain_);
  WaitForFrameLoad();
  ASSERT_EQ(prompt_factory_->request_count(), 1);
  Closing();
  EXPECT_TRUE(request1_.finished());
  EXPECT_FALSE(iframe_request_same_domain_.finished());
  EXPECT_TRUE(prompt_factory_->is_visible());
  ASSERT_EQ(prompt_factory_->request_count(), 1);
  Closing();
  EXPECT_TRUE(iframe_request_same_domain_.finished());
}

TEST_F(PermissionRequestManagerTest,
       IFrameRequestOtherDomainWhenMainRequestVisible) {
  manager_->AddRequest(&request1_);
  WaitForBubbleToBeShown();
  EXPECT_TRUE(prompt_factory_->is_visible());

  manager_->AddRequest(&iframe_request_other_domain_);
  WaitForFrameLoad();
  Closing();
  EXPECT_TRUE(request1_.finished());
  EXPECT_FALSE(iframe_request_other_domain_.finished());
  EXPECT_TRUE(prompt_factory_->is_visible());
  Closing();
  EXPECT_TRUE(iframe_request_other_domain_.finished());
}

TEST_F(PermissionRequestManagerTest, RequestsDontNeedUserGesture) {
  WaitForFrameLoad();
  WaitForBubbleToBeShown();
  manager_->AddRequest(&request1_);
  manager_->AddRequest(&iframe_request_other_domain_);
  manager_->AddRequest(&request2_);
  base::RunLoop().RunUntilIdle();

  EXPECT_TRUE(prompt_factory_->is_visible());
}

TEST_F(PermissionRequestManagerTest, UMAForSimpleAcceptedGestureBubble) {
  base::HistogramTester histograms;

  manager_->AddRequest(&request1_);
  WaitForBubbleToBeShown();
  histograms.ExpectUniqueSample(
      PermissionUmaUtil::kPermissionsPromptShown,
      static_cast<base::HistogramBase::Sample>(PermissionRequestType::QUOTA),
      1);
  histograms.ExpectUniqueSample(
      PermissionUmaUtil::kPermissionsPromptShownGesture,
      static_cast<base::HistogramBase::Sample>(PermissionRequestType::QUOTA),
      1);
  histograms.ExpectTotalCount(
      PermissionUmaUtil::kPermissionsPromptShownNoGesture, 0);
  histograms.ExpectTotalCount("Permissions.Engagement.Accepted.Quota", 0);

  Accept();
  histograms.ExpectUniqueSample(
      PermissionUmaUtil::kPermissionsPromptAccepted,
      static_cast<base::HistogramBase::Sample>(PermissionRequestType::QUOTA),
      1);
  histograms.ExpectTotalCount(
      PermissionUmaUtil::kPermissionsPromptDenied, 0);

  histograms.ExpectUniqueSample(
      PermissionUmaUtil::kPermissionsPromptAcceptedGesture,
      static_cast<base::HistogramBase::Sample>(PermissionRequestType::QUOTA),
      1);
  histograms.ExpectTotalCount(
      PermissionUmaUtil::kPermissionsPromptAcceptedNoGesture, 0);
  histograms.ExpectUniqueSample("Permissions.Engagement.Accepted.Quota",
                                kTestEngagementScore, 1);
}

TEST_F(PermissionRequestManagerTest, UMAForSimpleDeniedNoGestureBubble) {
  base::HistogramTester histograms;

  manager_->AddRequest(&request2_);
  WaitForBubbleToBeShown();

  histograms.ExpectTotalCount(
      PermissionUmaUtil::kPermissionsPromptShownGesture, 0);
  histograms.ExpectUniqueSample(
      PermissionUmaUtil::kPermissionsPromptShownNoGesture,
      static_cast<base::HistogramBase::Sample>(PermissionRequestType::DOWNLOAD),
      1);
  histograms.ExpectTotalCount("Permissions.Engagement.Denied.MultipleDownload",
                              0);
  // No need to test the other UMA for showing prompts again, they were tested
  // in UMAForSimpleAcceptedBubble.

  Deny();
  histograms.ExpectTotalCount(
      PermissionUmaUtil::kPermissionsPromptAccepted, 0);
  histograms.ExpectUniqueSample(
      PermissionUmaUtil::kPermissionsPromptDenied,
      static_cast<base::HistogramBase::Sample>(PermissionRequestType::DOWNLOAD),
      1);

  histograms.ExpectUniqueSample(
      PermissionUmaUtil::kPermissionsPromptDeniedNoGesture,
      static_cast<base::HistogramBase::Sample>(PermissionRequestType::DOWNLOAD),
      1);
  histograms.ExpectTotalCount(
      PermissionUmaUtil::kPermissionsPromptDeniedGesture, 0);
  histograms.ExpectUniqueSample(
      "Permissions.Engagement.Denied.MultipleDownload", kTestEngagementScore,
      1);
}

// This code path (calling Accept on a non-merged bubble, with no accepted
// permission) would never be used in actual Chrome, but its still tested for
// completeness.
TEST_F(PermissionRequestManagerTest, UMAForSimpleDeniedBubbleAlternatePath) {
  base::HistogramTester histograms;

  manager_->AddRequest(&request1_);
  WaitForBubbleToBeShown();
  // No need to test UMA for showing prompts again, they were tested in
  // UMAForSimpleAcceptedBubble.

  Deny();
  histograms.ExpectUniqueSample(
      PermissionUmaUtil::kPermissionsPromptDenied,
      static_cast<base::HistogramBase::Sample>(PermissionRequestType::QUOTA),
      1);
}

TEST_F(PermissionRequestManagerTest, UMAForMergedAcceptedBubble) {
  base::HistogramTester histograms;

  manager_->AddRequest(&request_mic_);
  manager_->AddRequest(&request_camera_);
  WaitForBubbleToBeShown();

  histograms.ExpectUniqueSample(
      PermissionUmaUtil::kPermissionsPromptShown,
      static_cast<base::HistogramBase::Sample>(PermissionRequestType::MULTIPLE),
      1);
  histograms.ExpectTotalCount(
      PermissionUmaUtil::kPermissionsPromptShownGesture, 0);
  histograms.ExpectTotalCount(
      PermissionUmaUtil::kPermissionsPromptShownNoGesture, 0);
  histograms.ExpectTotalCount(
      "Permissions.Engagement.Accepted.AudioAndVideoCapture", 0);

  Accept();

  histograms.ExpectUniqueSample(
      PermissionUmaUtil::kPermissionsPromptAccepted,
      static_cast<base::HistogramBase::Sample>(PermissionRequestType::MULTIPLE),
      1);
  histograms.ExpectUniqueSample(
      "Permissions.Engagement.Accepted.AudioAndVideoCapture",
      kTestEngagementScore, 1);
}

TEST_F(PermissionRequestManagerTest, UMAForMergedDeniedBubble) {
  base::HistogramTester histograms;

  manager_->AddRequest(&request_mic_);
  manager_->AddRequest(&request_camera_);
  WaitForBubbleToBeShown();
  histograms.ExpectTotalCount(
      "Permissions.Engagement.Denied.AudioAndVideoCapture", 0);
  // No need to test UMA for showing prompts again, they were tested in
  // UMAForMergedAcceptedBubble.

  Deny();

  histograms.ExpectUniqueSample(
      PermissionUmaUtil::kPermissionsPromptDenied,
      static_cast<base::HistogramBase::Sample>(PermissionRequestType::MULTIPLE),
      1);
  histograms.ExpectUniqueSample(
      "Permissions.Engagement.Denied.AudioAndVideoCapture",
      kTestEngagementScore, 1);
}

TEST_F(PermissionRequestManagerTest, UMAForIgnores) {
  base::HistogramTester histograms;

  manager_->AddRequest(&request1_);
  WaitForBubbleToBeShown();
  histograms.ExpectTotalCount("Permissions.Engagement.Ignored.Quota", 0);

  GURL youtube("http://www.youtube.com/");
  NavigateAndCommit(youtube);
  histograms.ExpectUniqueSample("Permissions.Engagement.Ignored.Quota",
                                kTestEngagementScore, 1);

  MockPermissionRequest youtube_request(
      "request2", PermissionRequestType::PERMISSION_GEOLOCATION, youtube);
  manager_->AddRequest(&youtube_request);
  WaitForBubbleToBeShown();

  NavigateAndCommit(GURL("http://www.google.com/"));
  histograms.ExpectUniqueSample("Permissions.Engagement.Ignored.Geolocation", 0,
                                1);
}

TEST_F(PermissionRequestManagerTest, UMAForTabSwitching) {
  base::HistogramTester histograms;

  manager_->AddRequest(&request1_);
  WaitForBubbleToBeShown();
  histograms.ExpectUniqueSample(
      PermissionUmaUtil::kPermissionsPromptShown,
      static_cast<base::HistogramBase::Sample>(PermissionRequestType::QUOTA),
      1);

  MockTabSwitchAway();
  MockTabSwitchBack();
  histograms.ExpectUniqueSample(
      PermissionUmaUtil::kPermissionsPromptShown,
      static_cast<base::HistogramBase::Sample>(PermissionRequestType::QUOTA),
      1);
}
