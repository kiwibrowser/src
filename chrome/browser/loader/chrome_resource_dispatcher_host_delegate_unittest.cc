// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/loader/chrome_resource_dispatcher_host_delegate.h"

#include <memory>

#include "chrome/browser/loader/chrome_navigation_data.h"
#include "chrome/browser/loader/chrome_resource_dispatcher_host_delegate.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_data.h"
#include "components/previews/core/test_previews_decider.h"
#include "content/public/browser/navigation_data.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "net/base/request_priority.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_context.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

class ChromeResourceDispatcherHostDelegateTest : public testing::Test {
 public:
  ChromeResourceDispatcherHostDelegateTest()
      : profile_manager_(
            new TestingProfileManager(TestingBrowserProcess::GetGlobal())) {}
  ~ChromeResourceDispatcherHostDelegateTest() override {}

  void SetUp() override { ASSERT_TRUE(profile_manager_->SetUp()); }

  // Exposes private static method for tests.
  static content::PreviewsState DetermineCommittedPreviews(
      const net::URLRequest* request,
      const previews::PreviewsDecider* previews_decider,
      content::PreviewsState initial_state) {
    return ChromeResourceDispatcherHostDelegate::DetermineCommittedPreviews(
        request, previews_decider, initial_state);
  }

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  // Set up TestingProfileManager for extensions::UserScriptListener.
  std::unique_ptr<TestingProfileManager> profile_manager_;
};

TEST_F(ChromeResourceDispatcherHostDelegateTest,
       GetNavigationDataWithDataReductionProxyData) {
  std::unique_ptr<net::URLRequestContext> context =
      std::make_unique<net::URLRequestContext>();
  std::unique_ptr<net::URLRequest> fake_request(
      context->CreateRequest(GURL("google.com"), net::RequestPriority::IDLE,
                             nullptr, TRAFFIC_ANNOTATION_FOR_TESTS));
  // Add DataReductionProxyData to URLRequest
  data_reduction_proxy::DataReductionProxyData* data_reduction_proxy_data =
      data_reduction_proxy::DataReductionProxyData::GetDataAndCreateIfNecessary(
          fake_request.get());
  data_reduction_proxy_data->set_used_data_reduction_proxy(true);
  std::unique_ptr<ChromeResourceDispatcherHostDelegate> delegate =
      std::make_unique<ChromeResourceDispatcherHostDelegate>();
  ChromeNavigationData* chrome_navigation_data =
      static_cast<ChromeNavigationData*>(
          delegate->GetNavigationData(fake_request.get()));
  data_reduction_proxy::DataReductionProxyData* data_reduction_proxy_data_copy =
      chrome_navigation_data->GetDataReductionProxyData();
  // The DataReductionProxyData should be a copy of the one on URLRequest
  EXPECT_NE(data_reduction_proxy_data_copy, data_reduction_proxy_data);
  // Make sure DataReductionProxyData was copied.
  EXPECT_TRUE(data_reduction_proxy_data_copy->used_data_reduction_proxy());
  EXPECT_EQ(
      chrome_navigation_data,
      ChromeNavigationData::GetDataAndCreateIfNecessary(fake_request.get()));
}

TEST_F(ChromeResourceDispatcherHostDelegateTest,
       GetNavigationDataWithoutDataReductionProxyData) {
  std::unique_ptr<net::URLRequestContext> context =
      std::make_unique<net::URLRequestContext>();
  std::unique_ptr<net::URLRequest> fake_request(
      context->CreateRequest(GURL("google.com"), net::RequestPriority::IDLE,
                             nullptr, TRAFFIC_ANNOTATION_FOR_TESTS));
  std::unique_ptr<ChromeResourceDispatcherHostDelegate> delegate =
      std::make_unique<ChromeResourceDispatcherHostDelegate>();
  ChromeNavigationData* chrome_navigation_data =
      static_cast<ChromeNavigationData*>(
          delegate->GetNavigationData(fake_request.get()));
  EXPECT_FALSE(chrome_navigation_data->GetDataReductionProxyData());
}

TEST_F(ChromeResourceDispatcherHostDelegateTest,
       DetermineCommittedPreviewsForServerPreview) {
  content::PreviewsState enabled_previews =
      content::SERVER_LITE_PAGE_ON | content::SERVER_LOFI_ON |
      content::CLIENT_LOFI_ON | content::NOSCRIPT_ON;
  std::unique_ptr<net::URLRequestContext> context =
      std::make_unique<net::URLRequestContext>();
  std::unique_ptr<net::URLRequest> fake_request(
      context->CreateRequest(GURL("google.com"), net::RequestPriority::IDLE,
                             nullptr, TRAFFIC_ANNOTATION_FOR_TESTS));
  // Add ResourceType to URLRequest.
  content::ResourceRequestInfo::AllocateForTesting(
      fake_request.get(), content::RESOURCE_TYPE_MAIN_FRAME, nullptr, -1, -1,
      -1,
      true,   // is_main_frame
      false,  // allow_download
      false,  // is_async
      enabled_previews,
      nullptr);  // navigation_ui_data
  // Add DataReductionProxyData to URLRequest.
  data_reduction_proxy::DataReductionProxyData* data_reduction_proxy_data =
      data_reduction_proxy::DataReductionProxyData::GetDataAndCreateIfNecessary(
          fake_request.get());
  data_reduction_proxy_data->set_used_data_reduction_proxy(true);
  data_reduction_proxy_data->set_lite_page_received(true);
  std::unique_ptr<ChromeResourceDispatcherHostDelegate> delegate =
      std::make_unique<ChromeResourceDispatcherHostDelegate>();
  std::unique_ptr<previews::TestPreviewsDecider> previews_decider =
      std::make_unique<previews::TestPreviewsDecider>(true);
  EXPECT_EQ(
      content::SERVER_LITE_PAGE_ON,
      ChromeResourceDispatcherHostDelegateTest::DetermineCommittedPreviews(
          fake_request.get(), previews_decider.get(), enabled_previews));
}

TEST_F(ChromeResourceDispatcherHostDelegateTest,
       DetermineCommittedPreviewsForClientPreview) {
  content::PreviewsState enabled_previews =
      content::SERVER_LITE_PAGE_ON | content::SERVER_LOFI_ON |
      content::CLIENT_LOFI_ON | content::NOSCRIPT_ON;
  std::unique_ptr<net::URLRequestContext> context =
      std::make_unique<net::URLRequestContext>();
  std::unique_ptr<ChromeResourceDispatcherHostDelegate> delegate =
      std::make_unique<ChromeResourceDispatcherHostDelegate>();
  std::unique_ptr<previews::TestPreviewsDecider> previews_decider =
      std::make_unique<previews::TestPreviewsDecider>(true);
  std::unique_ptr<net::URLRequest> fake_request(context->CreateRequest(
      GURL("https://google.com"), net::RequestPriority::IDLE, nullptr,
      TRAFFIC_ANNOTATION_FOR_TESTS));
  // Add ResourceType to URLRequest.
  content::ResourceRequestInfo::AllocateForTesting(
      fake_request.get(), content::RESOURCE_TYPE_MAIN_FRAME, nullptr, -1, -1,
      -1,
      true,   // is_main_frame
      false,  // allow_download
      false,  // is_async
      enabled_previews,
      nullptr);  // navigation_ui_data
  EXPECT_EQ(
      content::NOSCRIPT_ON,
      ChromeResourceDispatcherHostDelegateTest::DetermineCommittedPreviews(
          fake_request.get(), previews_decider.get(), enabled_previews));

  // Now ensure that the no transform directive honored for NoScript.
  std::unique_ptr<previews::TestPreviewsDecider> negative_previews_decider =
      std::make_unique<previews::TestPreviewsDecider>(false);
  previews::PreviewsUserData::Create(fake_request.get(), 1);
  previews::PreviewsUserData::GetData(*fake_request.get())
      ->SetCacheControlNoTransformDirective();
  EXPECT_EQ(
      content::PREVIEWS_OFF,
      ChromeResourceDispatcherHostDelegateTest::DetermineCommittedPreviews(
          fake_request.get(), negative_previews_decider.get(),
          enabled_previews));
}
