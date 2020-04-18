// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/download/browser_download_service.h"

#include <vector>

#include "base/macros.h"
#include "base/test/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "ios/chrome/browser/browser_state/test_chrome_browser_state.h"
#import "ios/chrome/browser/download/download_manager_tab_helper.h"
#include "ios/chrome/browser/download/pass_kit_mime_type.h"
#import "ios/chrome/browser/download/pass_kit_tab_helper.h"
#import "ios/web/public/download/download_controller.h"
#import "ios/web/public/download/download_task.h"
#include "ios/web/public/features.h"
#import "ios/web/public/test/fakes/fake_download_task.h"
#import "ios/web/public/test/fakes/test_web_state.h"
#include "ios/web/public/test/test_web_thread_bundle.h"
#include "testing/platform_test.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
char kUrl[] = "https://test.test/";

// Substitutes real TabHelper for testing.
template <class TabHelper>
class StubTabHelper : public TabHelper {
 public:
  static void CreateForWebState(web::WebState* web_state) {
    web_state->SetUserData(TabHelper::UserDataKey(),
                           base::WrapUnique(new StubTabHelper(web_state)));
  }

  // Adds the given task to tasks() lists.
  void Download(std::unique_ptr<web::DownloadTask> task) override {
    tasks_.push_back(std::move(task));
  }

  // Tasks added via Download() call.
  using DownloadTasks = std::vector<std::unique_ptr<web::DownloadTask>>;
  const DownloadTasks& tasks() const { return tasks_; }

 private:
  StubTabHelper(web::WebState* web_state)
      : TabHelper(web_state, /*delegate=*/nil) {}

  DownloadTasks tasks_;

  DISALLOW_COPY_AND_ASSIGN(StubTabHelper);
};

}  // namespace

// Test fixture for testing BrowserDownloadService class.
class BrowserDownloadServiceTest : public PlatformTest {
 protected:
  BrowserDownloadServiceTest()
      : browser_state_(browser_state_builder_.Build()) {
    feature_list_.InitAndEnableFeature(web::features::kNewFileDownload);

    StubTabHelper<PassKitTabHelper>::CreateForWebState(&web_state_);
    StubTabHelper<DownloadManagerTabHelper>::CreateForWebState(&web_state_);

    // BrowserDownloadServiceFactory sets its service as
    // DownloadControllerDelegate. These test use separate
    // BrowserDownloadService, not created by factory. So delegate
    // is temporary removed for these tests to avoid DCHECKs.
    previous_delegate_ = download_controller()->GetDelegate();
    download_controller()->SetDelegate(nullptr);
    service_ = std::make_unique<BrowserDownloadService>(download_controller());
  }

  ~BrowserDownloadServiceTest() override {
    service_.reset();
    // Return back the original delegate so service created by service factory
    // can be destructed without DCHECKs.
    download_controller()->SetDelegate(previous_delegate_);
  }

  web::DownloadController* download_controller() {
    return web::DownloadController::FromBrowserState(browser_state_.get());
  }

  StubTabHelper<PassKitTabHelper>* pass_kit_tab_helper() {
    return static_cast<StubTabHelper<PassKitTabHelper>*>(
        PassKitTabHelper::FromWebState(&web_state_));
  }

  StubTabHelper<DownloadManagerTabHelper>* download_manager_tab_helper() {
    return static_cast<StubTabHelper<DownloadManagerTabHelper>*>(
        DownloadManagerTabHelper::FromWebState(&web_state_));
  }

  web::DownloadControllerDelegate* previous_delegate_;
  web::TestWebThreadBundle thread_bundle_;
  TestChromeBrowserState::Builder browser_state_builder_;
  std::unique_ptr<TestChromeBrowserState> browser_state_;
  std::unique_ptr<BrowserDownloadService> service_;
  web::TestWebState web_state_;
  base::test::ScopedFeatureList feature_list_;
  base::HistogramTester histogram_tester_;
};

// Tests that BrowserDownloadService downloads the task using
// PassKitTabHelper.
TEST_F(BrowserDownloadServiceTest, PkPassMimeType) {
  ASSERT_TRUE(download_controller()->GetDelegate());
  auto task =
      std::make_unique<web::FakeDownloadTask>(GURL(kUrl), kPkPassMimeType);
  web::DownloadTask* task_ptr = task.get();
  download_controller()->GetDelegate()->OnDownloadCreated(
      download_controller(), &web_state_, std::move(task));
  ASSERT_EQ(1U, pass_kit_tab_helper()->tasks().size());
  EXPECT_EQ(task_ptr, pass_kit_tab_helper()->tasks()[0].get());
  ASSERT_TRUE(download_manager_tab_helper()->tasks().empty());
  histogram_tester_.ExpectUniqueSample(
      "Download.IOSDownloadMimeType",
      static_cast<base::HistogramBase::Sample>(DownloadMimeTypeResult::PkPass),
      1);
}

// Tests that BrowserDownloadService uses DownloadManagerTabHelper for PDF Mime
// Type.
TEST_F(BrowserDownloadServiceTest, PdfMimeType) {
  ASSERT_TRUE(download_controller()->GetDelegate());
  auto task =
      std::make_unique<web::FakeDownloadTask>(GURL(kUrl), "application/pdf");
  web::DownloadTask* task_ptr = task.get();
  download_controller()->GetDelegate()->OnDownloadCreated(
      download_controller(), &web_state_, std::move(task));
  ASSERT_TRUE(pass_kit_tab_helper()->tasks().empty());
  ASSERT_EQ(1U, download_manager_tab_helper()->tasks().size());
  EXPECT_EQ(task_ptr, download_manager_tab_helper()->tasks()[0].get());
  histogram_tester_.ExpectUniqueSample(
      "Download.IOSDownloadMimeType",
      static_cast<base::HistogramBase::Sample>(DownloadMimeTypeResult::Other),
      1);
}

// Tests that BrowserDownloadService uses DownloadManagerTabHelper for Mobile
// Config Mime Type.
TEST_F(BrowserDownloadServiceTest, iOSMobileConfigMimeType) {
  ASSERT_TRUE(download_controller()->GetDelegate());
  auto task = std::make_unique<web::FakeDownloadTask>(
      GURL(kUrl), "application/x-apple-aspen-config");
  web::DownloadTask* task_ptr = task.get();
  download_controller()->GetDelegate()->OnDownloadCreated(
      download_controller(), &web_state_, std::move(task));
  ASSERT_TRUE(pass_kit_tab_helper()->tasks().empty());
  ASSERT_EQ(1U, download_manager_tab_helper()->tasks().size());
  EXPECT_EQ(task_ptr, download_manager_tab_helper()->tasks()[0].get());
  histogram_tester_.ExpectUniqueSample(
      "Download.IOSDownloadMimeType",
      static_cast<base::HistogramBase::Sample>(
          DownloadMimeTypeResult::iOSMobileConfig),
      1);
}
