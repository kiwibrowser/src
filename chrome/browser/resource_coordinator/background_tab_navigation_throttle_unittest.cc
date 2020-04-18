// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/background_tab_navigation_throttle.h"

#include <tuple>

#include "base/macros.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/resource_coordinator/tab_manager_features.h"
#include "chrome/browser/ui/tab_ui_helper.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "components/variations/variations_associated_data.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/navigation_throttle.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/web_contents_tester.h"
#include "url/gurl.h"

namespace resource_coordinator {

namespace {

enum ExpectInstantiationResult {
  EXPECT_INSTANTIATION,
  EXPECT_NO_INSTANTIATION
};

const char kTestUrl[] = "http://www.example.com";

}  // namespace

class BackgroundTabNavigationThrottleTest
    : public ChromeRenderViewHostTestHarness,
      public testing::WithParamInterface<
          std::tuple<ExpectInstantiationResult,
                     bool,  // enable_feature
                     bool,  // is_in_main_frame
                     bool,  // is_background_tab
                     bool,  // no_opener
                     bool,  // is_initial_navigation
                     bool,  // has_tab_ui_helper
                     GURL>> {
 public:
  BackgroundTabNavigationThrottleTest() {}

  void SetUp() override {
    std::tie(expected_instantiation_result_, enable_feature_, is_in_main_frame_,
             is_background_tab_, no_opener_, is_initial_navigation_,
             has_tab_ui_helper_, url_) = GetParam();

    if (enable_feature_) {
      scoped_feature_list_.InitAndEnableFeature(
          features::kStaggeredBackgroundTabOpening);
    }

    ChromeRenderViewHostTestHarness::SetUp();
    content::RenderFrameHostTester::For(main_rfh())
        ->InitializeRenderFrameIfNeeded();
  }

  void TearDown() override {
    ChromeRenderViewHostTestHarness::TearDown();
    variations::testing::ClearAllVariationParams();
  }

 protected:
  ExpectInstantiationResult expected_instantiation_result_;
  bool enable_feature_;
  bool is_in_main_frame_;
  bool is_background_tab_;
  bool no_opener_;
  bool is_initial_navigation_;
  bool has_tab_ui_helper_;
  GURL url_;

 private:
  base::test::ScopedFeatureList scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(BackgroundTabNavigationThrottleTest);
};

TEST_P(BackgroundTabNavigationThrottleTest, Instantiate) {
  if (has_tab_ui_helper_)
    TabUIHelper::CreateForWebContents(web_contents());

  if (!is_initial_navigation_)
    NavigateAndCommit(GURL(kTestUrl));

  if (is_background_tab_)
    web_contents()->WasHidden();
  else
    web_contents()->WasShown();

  std::unique_ptr<content::WebContents> opener;
  if (!no_opener_) {
    opener = CreateTestWebContents();
    content::WebContentsTester::For(web_contents())->SetOpener(opener.get());
  }

  content::RenderFrameHost* rfh;
  if (is_in_main_frame_)
    rfh = main_rfh();
  else
    rfh = content::RenderFrameHostTester::For(main_rfh())->AppendChild("child");

  DCHECK(rfh);
  std::unique_ptr<content::NavigationHandle> handle =
      content::NavigationHandle::CreateNavigationHandleForTesting(url_, rfh);
  std::unique_ptr<BackgroundTabNavigationThrottle> throttle =
      BackgroundTabNavigationThrottle::MaybeCreateThrottleFor(handle.get());

  const bool expect_instantiation =
      expected_instantiation_result_ == EXPECT_INSTANTIATION;
  EXPECT_EQ(expect_instantiation, throttle != nullptr);
}

INSTANTIATE_TEST_CASE_P(
    InstantiateThrottle,
    BackgroundTabNavigationThrottleTest,
    ::testing::Values(std::make_tuple(EXPECT_INSTANTIATION,
                                      true,  // Enable feature
                                      true,  // Is in main frame
                                      true,  // Is background tab
                                      true,  // No opener
                                      true,  // Is initial navigation
                                      true,  // Has tab UI helper
                                      GURL(kTestUrl)),
                      std::make_tuple(EXPECT_NO_INSTANTIATION,
                                      false,  // Disable feature
                                      true,
                                      true,
                                      true,
                                      true,
                                      true,
                                      GURL(kTestUrl)),
                      std::make_tuple(EXPECT_NO_INSTANTIATION,
                                      true,
                                      false,  // Is in child frame
                                      true,
                                      true,
                                      true,
                                      true,
                                      GURL(kTestUrl)),
                      std::make_tuple(EXPECT_NO_INSTANTIATION,
                                      true,
                                      true,
                                      false,  // Is foreground tab
                                      true,
                                      true,
                                      true,
                                      GURL(kTestUrl)),
                      std::make_tuple(EXPECT_NO_INSTANTIATION,
                                      true,
                                      true,
                                      true,
                                      false,  // Has opener
                                      true,
                                      true,
                                      GURL(kTestUrl)),
                      std::make_tuple(EXPECT_NO_INSTANTIATION,
                                      true,
                                      true,
                                      true,
                                      true,
                                      false,  // Is not initial navigation
                                      true,
                                      GURL(kTestUrl)),
                      std::make_tuple(EXPECT_NO_INSTANTIATION,
                                      true,
                                      true,
                                      true,
                                      true,
                                      true,
                                      false,  // Do not have tab UI helper
                                      GURL(kTestUrl))));

}  // namespace resource_coordinator
