// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LiICENSE file.

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/web_size.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/platform/web_url_loader_mock_factory.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/testing/page_test_base.h"
#include "third_party/blink/renderer/modules/background_fetch/background_fetch_icon_loader.h"
#include "third_party/blink/renderer/modules/background_fetch/icon_definition.h"
#include "third_party/blink/renderer/platform/heap/persistent.h"
#include "third_party/blink/renderer/platform/testing/testing_platform_support.h"
#include "third_party/blink/renderer/platform/testing/unit_test_helpers.h"
#include "third_party/blink/renderer/platform/testing/url_test_helpers.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"

namespace blink {
namespace {

enum class BackgroundFetchLoadState {
  kNotLoaded,
  kLoadFailed,
  kLoadSuccessful
};

constexpr char kBackgroundFetchImageLoaderBaseUrl[] = "http://test.com/";
constexpr char kBackgroundFetchImageLoaderBaseDir[] = "notifications/";
constexpr char kBackgroundFetchImageLoaderIcon500x500[] = "500x500.png";
constexpr char kBackgroundFetchImageLoaderIcon48x48[] = "48x48.png";
constexpr char kBackgroundFetchImageLoaderIcon3000x2000[] = "3000x2000.png";
constexpr char kBackgroundFetchImageLoaderIcon[] = "3000x2000.png";

}  // namespace

class BackgroundFetchIconLoaderTest : public PageTestBase {
 public:
  BackgroundFetchIconLoaderTest() : loader_(new BackgroundFetchIconLoader()) {}
  ~BackgroundFetchIconLoaderTest() override {
    loader_->Stop();
    platform_->GetURLLoaderMockFactory()
        ->UnregisterAllURLsAndClearMemoryCache();
  }

  void SetUp() override { PageTestBase::SetUp(IntSize()); }

  // Registers a mocked URL.
  WebURL RegisterMockedURL(const String& file_name) {
    WebURL registered_url = URLTestHelpers::RegisterMockedURLLoadFromBase(
        kBackgroundFetchImageLoaderBaseUrl,
        test::CoreTestDataPath(kBackgroundFetchImageLoaderBaseDir), file_name,
        "image/png");
    return registered_url;
  }

  // Callback for BackgroundFetchIconLoader. This will set up the state of the
  // load as either success or failed based on whether the bitmap is empty.
  void IconLoaded(const SkBitmap& bitmap) {
    if (!bitmap.empty())
      loaded_ = BackgroundFetchLoadState::kLoadSuccessful;
    else
      loaded_ = BackgroundFetchLoadState::kLoadFailed;
  }

  IconDefinition CreateTestIcon(const String& url_str, const String& size) {
    KURL url = RegisterMockedURL(url_str);
    IconDefinition icon;
    icon.setSrc(url.GetString());
    icon.setType("image/png");
    icon.setSizes(size);
    return icon;
  }

  int PickRightIcon(HeapVector<IconDefinition> icons,
                    const WebSize& ideal_display_size) {
    loader_->icons_ = std::move(icons);
    return loader_->PickBestIconForDisplay(GetContext(), ideal_display_size);
  }

  void LoadIcon(const KURL& url) {
    IconDefinition icon;
    icon.setSrc(url.GetString());
    icon.setType("image/png");
    icon.setSizes("500x500");
    HeapVector<IconDefinition> icons(1, icon);
    loader_->icons_ = std::move(icons);
    loader_->DidGetIconDisplaySizeIfSoLoadIcon(
        GetContext(),
        Bind(&BackgroundFetchIconLoaderTest::IconLoaded, WTF::Unretained(this)),
        WebSize(192, 192));
  }

  ExecutionContext* GetContext() const { return &GetDocument(); }

 protected:
  ScopedTestingPlatformSupport<TestingPlatformSupport> platform_;
  BackgroundFetchLoadState loaded_ = BackgroundFetchLoadState::kNotLoaded;

 private:
  Persistent<BackgroundFetchIconLoader> loader_;
};

TEST_F(BackgroundFetchIconLoaderTest, SuccessTest) {
  KURL url = RegisterMockedURL(kBackgroundFetchImageLoaderIcon500x500);
  LoadIcon(url);
  platform_->GetURLLoaderMockFactory()->ServeAsynchronousRequests();
  EXPECT_EQ(BackgroundFetchLoadState::kLoadSuccessful, loaded_);
}

TEST_F(BackgroundFetchIconLoaderTest, PickRightIconTest) {
  IconDefinition icon0 =
      CreateTestIcon(kBackgroundFetchImageLoaderIcon500x500, "500x500");
  IconDefinition icon1 =
      CreateTestIcon(kBackgroundFetchImageLoaderIcon48x48, "48x48");
  IconDefinition icon2 =
      CreateTestIcon(kBackgroundFetchImageLoaderIcon3000x2000, "3000x2000");

  HeapVector<IconDefinition> icons;
  icons.push_back(icon0);
  icons.push_back(icon1);
  icons.push_back(icon2);

  int index = PickRightIcon(std::move(icons), WebSize(50, 50));
  EXPECT_EQ(index, 1);
}

TEST_F(BackgroundFetchIconLoaderTest, PickRightIconGivenAnyTest) {
  IconDefinition icon0 =
      CreateTestIcon(kBackgroundFetchImageLoaderIcon500x500, "500x500");
  IconDefinition icon1 =
      CreateTestIcon(kBackgroundFetchImageLoaderIcon48x48, "48x48");
  IconDefinition icon2 = CreateTestIcon(kBackgroundFetchImageLoaderIcon, "any");

  HeapVector<IconDefinition> icons;
  icons.push_back(icon0);
  icons.push_back(icon1);
  icons.push_back(icon2);

  int index = PickRightIcon(std::move(icons), WebSize(50, 50));
  EXPECT_EQ(index, 2);
}

TEST_F(BackgroundFetchIconLoaderTest, PickRightIconWithTieBreakTest) {
  // Test that if two icons get the same score, the one declared last gets
  // picked.
  IconDefinition icon0 =
      CreateTestIcon(kBackgroundFetchImageLoaderIcon500x500, "500x500");
  IconDefinition icon1 =
      CreateTestIcon(kBackgroundFetchImageLoaderIcon48x48, "48x48");
  IconDefinition icon2 =
      CreateTestIcon(kBackgroundFetchImageLoaderIcon3000x2000, "48x48");
  HeapVector<IconDefinition> icons;
  icons.push_back(icon0);
  icons.push_back(icon1);
  icons.push_back(icon2);
  int index = PickRightIcon(std::move(icons), WebSize(50, 50));
  EXPECT_EQ(index, 2);
}

}  // namespace blink
