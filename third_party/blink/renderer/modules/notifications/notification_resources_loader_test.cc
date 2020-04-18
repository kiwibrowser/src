// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/notifications/notification_resources_loader.h"

#include <memory>
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/modules/notifications/web_notification_constants.h"
#include "third_party/blink/public/platform/modules/notifications/web_notification_data.h"
#include "third_party/blink/public/platform/modules/notifications/web_notification_resources.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/platform/web_url_loader_mock_factory.h"
#include "third_party/blink/public/platform/web_url_response.h"
#include "third_party/blink/renderer/core/testing/page_test_base.h"
#include "third_party/blink/renderer/platform/heap/heap.h"
#include "third_party/blink/renderer/platform/loader/fetch/memory_cache.h"
#include "third_party/blink/renderer/platform/testing/testing_platform_support.h"
#include "third_party/blink/renderer/platform/testing/unit_test_helpers.h"
#include "third_party/blink/renderer/platform/testing/url_test_helpers.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {
namespace {

constexpr char kResourcesLoaderBaseUrl[] = "http://test.com/";
constexpr char kResourcesLoaderBaseDir[] = "notifications/";
constexpr char kResourcesLoaderIcon48x48[] = "48x48.png";
constexpr char kResourcesLoaderIcon100x100[] = "100x100.png";
constexpr char kResourcesLoaderIcon110x110[] = "110x110.png";
constexpr char kResourcesLoaderIcon120x120[] = "120x120.png";
constexpr char kResourcesLoaderIcon500x500[] = "500x500.png";
constexpr char kResourcesLoaderIcon3000x1000[] = "3000x1000.png";
constexpr char kResourcesLoaderIcon3000x2000[] = "3000x2000.png";

class NotificationResourcesLoaderTest : public PageTestBase {
 public:
  NotificationResourcesLoaderTest()
      : loader_(new NotificationResourcesLoader(
            Bind(&NotificationResourcesLoaderTest::DidFetchResources,
                 WTF::Unretained(this)))) {}

  ~NotificationResourcesLoaderTest() override {
    loader_->Stop();
    platform_->GetURLLoaderMockFactory()
        ->UnregisterAllURLsAndClearMemoryCache();
  }

  void SetUp() override { PageTestBase::SetUp(IntSize()); }

 protected:
  ExecutionContext* GetExecutionContext() const { return &GetDocument(); }

  NotificationResourcesLoader* Loader() const { return loader_.Get(); }

  WebNotificationResources* Resources() const { return resources_.get(); }

  void DidFetchResources(NotificationResourcesLoader* loader) {
    resources_ = loader->GetResources();
  }

  // Registers a mocked url. When fetched, |fileName| will be loaded from the
  // test data directory.
  WebURL RegisterMockedURL(const String& file_name) {
    WebURL registered_url = URLTestHelpers::RegisterMockedURLLoadFromBase(
        kResourcesLoaderBaseUrl,
        test::CoreTestDataPath(kResourcesLoaderBaseDir), file_name,
        "image/png");
    return registered_url;
  }

  // Registers a mocked url that will fail to be fetched, with a 404 error.
  WebURL RegisterMockedErrorURL(const String& file_name) {
    WebURL url(KURL(kResourcesLoaderBaseUrl + file_name));
    URLTestHelpers::RegisterMockedErrorURLLoad(url);
    return url;
  }

  ScopedTestingPlatformSupport<TestingPlatformSupport> platform_;

 private:
  Persistent<NotificationResourcesLoader> loader_;
  std::unique_ptr<WebNotificationResources> resources_;
};

TEST_F(NotificationResourcesLoaderTest, LoadMultipleResources) {
  WebNotificationData notification_data;
  notification_data.image = RegisterMockedURL(kResourcesLoaderIcon500x500);
  notification_data.icon = RegisterMockedURL(kResourcesLoaderIcon100x100);
  notification_data.badge = RegisterMockedURL(kResourcesLoaderIcon48x48);
  notification_data.actions =
      WebVector<WebNotificationAction>(static_cast<size_t>(2));
  notification_data.actions[0].icon =
      RegisterMockedURL(kResourcesLoaderIcon110x110);
  notification_data.actions[1].icon =
      RegisterMockedURL(kResourcesLoaderIcon120x120);

  ASSERT_FALSE(Resources());

  Loader()->Start(GetExecutionContext(), notification_data);
  platform_->GetURLLoaderMockFactory()->ServeAsynchronousRequests();

  ASSERT_TRUE(Resources());

  ASSERT_FALSE(Resources()->image.drawsNothing());
  ASSERT_EQ(500, Resources()->image.width());
  ASSERT_EQ(500, Resources()->image.height());

  ASSERT_FALSE(Resources()->icon.drawsNothing());
  ASSERT_EQ(100, Resources()->icon.width());

  ASSERT_FALSE(Resources()->badge.drawsNothing());
  ASSERT_EQ(48, Resources()->badge.width());

  ASSERT_EQ(2u, Resources()->action_icons.size());
  ASSERT_FALSE(Resources()->action_icons[0].drawsNothing());
  ASSERT_EQ(110, Resources()->action_icons[0].width());
  ASSERT_FALSE(Resources()->action_icons[1].drawsNothing());
  ASSERT_EQ(120, Resources()->action_icons[1].width());
}

TEST_F(NotificationResourcesLoaderTest, LargeIconsAreScaledDown) {
  WebNotificationData notification_data;
  notification_data.icon = RegisterMockedURL(kResourcesLoaderIcon500x500);
  notification_data.badge = notification_data.icon;
  notification_data.actions =
      WebVector<WebNotificationAction>(static_cast<size_t>(1));
  notification_data.actions[0].icon = notification_data.icon;

  ASSERT_FALSE(Resources());

  Loader()->Start(GetExecutionContext(), notification_data);
  platform_->GetURLLoaderMockFactory()->ServeAsynchronousRequests();

  ASSERT_TRUE(Resources());

  ASSERT_FALSE(Resources()->icon.drawsNothing());
  ASSERT_EQ(kWebNotificationMaxIconSizePx, Resources()->icon.width());
  ASSERT_EQ(kWebNotificationMaxIconSizePx, Resources()->icon.height());

  ASSERT_FALSE(Resources()->badge.drawsNothing());
  ASSERT_EQ(kWebNotificationMaxBadgeSizePx, Resources()->badge.width());
  ASSERT_EQ(kWebNotificationMaxBadgeSizePx, Resources()->badge.height());

  ASSERT_EQ(1u, Resources()->action_icons.size());
  ASSERT_FALSE(Resources()->action_icons[0].drawsNothing());
  ASSERT_EQ(kWebNotificationMaxActionIconSizePx,
            Resources()->action_icons[0].width());
  ASSERT_EQ(kWebNotificationMaxActionIconSizePx,
            Resources()->action_icons[0].height());
}

TEST_F(NotificationResourcesLoaderTest, DownscalingPreserves3_1AspectRatio) {
  WebNotificationData notification_data;
  notification_data.image = RegisterMockedURL(kResourcesLoaderIcon3000x1000);

  ASSERT_FALSE(Resources());

  Loader()->Start(GetExecutionContext(), notification_data);
  platform_->GetURLLoaderMockFactory()->ServeAsynchronousRequests();

  ASSERT_TRUE(Resources());

  ASSERT_FALSE(Resources()->image.drawsNothing());
  ASSERT_EQ(kWebNotificationMaxImageWidthPx, Resources()->image.width());
  ASSERT_EQ(kWebNotificationMaxImageWidthPx / 3, Resources()->image.height());
}

TEST_F(NotificationResourcesLoaderTest, DownscalingPreserves3_2AspectRatio) {
  WebNotificationData notification_data;
  notification_data.image = RegisterMockedURL(kResourcesLoaderIcon3000x2000);

  ASSERT_FALSE(Resources());

  Loader()->Start(GetExecutionContext(), notification_data);
  platform_->GetURLLoaderMockFactory()->ServeAsynchronousRequests();

  ASSERT_TRUE(Resources());

  ASSERT_FALSE(Resources()->image.drawsNothing());
  ASSERT_EQ(kWebNotificationMaxImageHeightPx * 3 / 2,
            Resources()->image.width());
  ASSERT_EQ(kWebNotificationMaxImageHeightPx, Resources()->image.height());
}

TEST_F(NotificationResourcesLoaderTest, EmptyDataYieldsEmptyResources) {
  WebNotificationData notification_data;

  ASSERT_FALSE(Resources());

  Loader()->Start(GetExecutionContext(), notification_data);
  platform_->GetURLLoaderMockFactory()->ServeAsynchronousRequests();

  ASSERT_TRUE(Resources());

  ASSERT_TRUE(Resources()->image.drawsNothing());
  ASSERT_TRUE(Resources()->icon.drawsNothing());
  ASSERT_TRUE(Resources()->badge.drawsNothing());
  ASSERT_EQ(0u, Resources()->action_icons.size());
}

TEST_F(NotificationResourcesLoaderTest, EmptyResourcesIfAllImagesFailToLoad) {
  WebNotificationData notification_data;
  notification_data.image = notification_data.icon;
  notification_data.icon = RegisterMockedErrorURL(kResourcesLoaderIcon100x100);
  notification_data.badge = notification_data.icon;
  notification_data.actions =
      WebVector<WebNotificationAction>(static_cast<size_t>(1));
  notification_data.actions[0].icon = notification_data.icon;

  ASSERT_FALSE(Resources());

  Loader()->Start(GetExecutionContext(), notification_data);
  platform_->GetURLLoaderMockFactory()->ServeAsynchronousRequests();

  ASSERT_TRUE(Resources());

  // The test received resources but they are all empty. This ensures that a
  // notification can still be shown even if the images fail to load.
  ASSERT_TRUE(Resources()->image.drawsNothing());
  ASSERT_TRUE(Resources()->icon.drawsNothing());
  ASSERT_TRUE(Resources()->badge.drawsNothing());
  ASSERT_EQ(1u, Resources()->action_icons.size());
  ASSERT_TRUE(Resources()->action_icons[0].drawsNothing());
}

TEST_F(NotificationResourcesLoaderTest, OneImageFailsToLoad) {
  WebNotificationData notification_data;
  notification_data.icon = RegisterMockedURL(kResourcesLoaderIcon100x100);
  notification_data.badge = RegisterMockedErrorURL(kResourcesLoaderIcon48x48);

  ASSERT_FALSE(Resources());

  Loader()->Start(GetExecutionContext(), notification_data);
  platform_->GetURLLoaderMockFactory()->ServeAsynchronousRequests();

  ASSERT_TRUE(Resources());

  // The test received resources even though one image failed to load. This
  // ensures that a notification can still be shown, though slightly degraded.
  ASSERT_TRUE(Resources()->image.drawsNothing());
  ASSERT_FALSE(Resources()->icon.drawsNothing());
  ASSERT_EQ(100, Resources()->icon.width());
  ASSERT_TRUE(Resources()->badge.drawsNothing());
  ASSERT_EQ(0u, Resources()->action_icons.size());
}

TEST_F(NotificationResourcesLoaderTest, StopYieldsNoResources) {
  WebNotificationData notification_data;
  notification_data.image = RegisterMockedURL(kResourcesLoaderIcon500x500);
  notification_data.icon = RegisterMockedURL(kResourcesLoaderIcon100x100);
  notification_data.badge = RegisterMockedURL(kResourcesLoaderIcon48x48);
  notification_data.actions =
      WebVector<WebNotificationAction>(static_cast<size_t>(2));
  notification_data.actions[0].icon =
      RegisterMockedURL(kResourcesLoaderIcon110x110);
  notification_data.actions[1].icon =
      RegisterMockedURL(kResourcesLoaderIcon120x120);

  ASSERT_FALSE(Resources());

  Loader()->Start(GetExecutionContext(), notification_data);

  // Check that starting the loader did not synchronously fail, providing
  // empty resources. The requests should be pending now.
  ASSERT_FALSE(Resources());

  // The loader would stop e.g. when the execution context is destroyed or
  // when the loader is about to be destroyed, as a pre-finalizer.
  Loader()->Stop();
  platform_->GetURLLoaderMockFactory()->ServeAsynchronousRequests();

  // Loading should have been cancelled when |stop| was called so no resources
  // should have been received by the test even though
  // |serveAsynchronousRequests| was called.
  ASSERT_FALSE(Resources());
}

}  // namespace
}  // namespace blink
