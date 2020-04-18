// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/bookmark_app_helper.h"

#include "base/macros.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/banners/app_banner_settings_helper.h"
#include "chrome/browser/extensions/convert_web_app.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_service_test_base.h"
#include "chrome/browser/extensions/favicon_downloader.h"
#include "chrome/browser/extensions/launch_util.h"
#include "chrome/browser/installable/installable_data.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/extensions/manifest_handlers/app_launch_info.h"
#include "chrome/common/extensions/manifest_handlers/app_theme_color_info.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/test_utils.h"
#include "content/public/test/web_contents_tester.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension_icon_set.h"
#include "extensions/common/manifest_handlers/icons_handler.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/skia_util.h"

namespace extensions {

namespace {

using ForInstallableSite = BookmarkAppHelper::ForInstallableSite;

const char kManifestUrl[] = "http://www.chromium.org/manifest.json";
const char kAppUrl[] = "http://www.chromium.org/index.html";
const char kAlternativeAppUrl[] = "http://www.notchromium.org";
const char kAppScope[] = "http://www.chromium.org/scope/";
const char kAppAlternativeScope[] = "http://www.chromium.org/new/";
const char kAppDefaultScope[] = "http://www.chromium.org/";
const char kAppTitle[] = "Test title";
const char kAppShortName[] = "Test short name";
const char kAlternativeAppTitle[] = "Different test title";
const char kAppDescription[] = "Test description";
const char kAppIcon1[] = "fav1.png";
const char kAppIcon2[] = "fav2.png";
const char kAppIcon3[] = "fav3.png";
const char kAppIconURL1[] = "http://foo.com/1.png";
const char kAppIconURL2[] = "http://foo.com/2.png";
const char kAppIconURL3[] = "http://foo.com/3.png";
const char kAppIconURL4[] = "http://foo.com/4.png";

const int kIconSizeTiny = extension_misc::EXTENSION_ICON_BITTY;
const int kIconSizeSmall = extension_misc::EXTENSION_ICON_SMALL;
const int kIconSizeMedium = extension_misc::EXTENSION_ICON_MEDIUM;
const int kIconSizeLarge = extension_misc::EXTENSION_ICON_LARGE;
const int kIconSizeGigantor = extension_misc::EXTENSION_ICON_GIGANTOR;
const int kIconSizeUnsupported = 123;

const int kIconSizeSmallBetweenMediumAndLarge = 63;
const int kIconSizeLargeBetweenMediumAndLarge = 96;

class BookmarkAppHelperTest : public testing::Test {
 public:
  BookmarkAppHelperTest() {}
  ~BookmarkAppHelperTest() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(BookmarkAppHelperTest);
};

class BookmarkAppHelperExtensionServiceTest
    : public extensions::ExtensionServiceTestBase {
 public:
  BookmarkAppHelperExtensionServiceTest() {}
  ~BookmarkAppHelperExtensionServiceTest() override {}

  void SetUp() override {
    extensions::ExtensionServiceTestBase::SetUp();
    InitializeEmptyExtensionService();
    service_->Init();
    EXPECT_EQ(0u, registry()->enabled_extensions().size());
  }

  void TearDown() override {
    ExtensionServiceTestBase::TearDown();
    for (content::RenderProcessHost::iterator i(
             content::RenderProcessHost::AllHostsIterator());
         !i.IsAtEnd();
         i.Advance()) {
      content::RenderProcessHost* host = i.GetCurrentValue();
      if (Profile::FromBrowserContext(host->GetBrowserContext()) ==
          profile_.get())
        host->Cleanup();
    }
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(BookmarkAppHelperExtensionServiceTest);
};

SkBitmap CreateSquareBitmapWithColor(int size, SkColor color) {
  SkBitmap bitmap;
  bitmap.allocN32Pixels(size, size);
  bitmap.eraseColor(color);
  return bitmap;
}

BookmarkAppHelper::BitmapAndSource CreateSquareBitmapAndSourceWithColor(
    int size,
    SkColor color) {
  return BookmarkAppHelper::BitmapAndSource(
      GURL(), CreateSquareBitmapWithColor(size, color));
}

void ValidateBitmapSizeAndColor(SkBitmap bitmap, int size, SkColor color) {
  // Obtain pixel lock to access pixels.
  EXPECT_EQ(color, bitmap.getColor(0, 0));
  EXPECT_EQ(size, bitmap.width());
  EXPECT_EQ(size, bitmap.height());
}

WebApplicationInfo::IconInfo CreateIconInfoWithBitmap(int size, SkColor color) {
  WebApplicationInfo::IconInfo icon_info;
  icon_info.width = size;
  icon_info.height = size;
  icon_info.data = CreateSquareBitmapWithColor(size, color);
  return icon_info;
}

std::set<int> TestSizesToGenerate() {
  const int kIconSizesToGenerate[] = {
      extension_misc::EXTENSION_ICON_SMALL,
      extension_misc::EXTENSION_ICON_MEDIUM,
      extension_misc::EXTENSION_ICON_LARGE,
  };
  return std::set<int>(kIconSizesToGenerate,
                       kIconSizesToGenerate + arraysize(kIconSizesToGenerate));
}

void ValidateAllIconsWithURLsArePresent(const WebApplicationInfo& info_to_check,
                                        const WebApplicationInfo& other_info) {
  for (const auto& icon : info_to_check.icons) {
    if (!icon.url.is_empty()) {
      bool found = false;
      for (const auto& other_icon : info_to_check.icons) {
        if (other_icon.url == icon.url && other_icon.width == icon.width) {
          found = true;
          break;
        }
      }
      EXPECT_TRUE(found);
    }
  }
}

std::vector<BookmarkAppHelper::BitmapAndSource>::const_iterator
FindLargestBitmapAndSourceVector(
    const std::vector<BookmarkAppHelper::BitmapAndSource>& bitmap_vector) {
  auto result = bitmap_vector.end();
  int largest = -1;
  for (std::vector<BookmarkAppHelper::BitmapAndSource>::const_iterator it =
           bitmap_vector.begin();
       it != bitmap_vector.end(); ++it) {
    if (it->bitmap.width() > largest) {
      result = it;
    }
  }
  return result;
}

std::vector<BookmarkAppHelper::BitmapAndSource>::const_iterator
FindMatchingBitmapAndSourceVector(
    const std::vector<BookmarkAppHelper::BitmapAndSource>& bitmap_vector,
    int size) {
  for (std::vector<BookmarkAppHelper::BitmapAndSource>::const_iterator it =
           bitmap_vector.begin();
       it != bitmap_vector.end(); ++it) {
    if (it->bitmap.width() == size) {
      return it;
    }
  }
  return bitmap_vector.end();
}

std::vector<BookmarkAppHelper::BitmapAndSource>::const_iterator
FindEqualOrLargerBitmapAndSourceVector(
    const std::vector<BookmarkAppHelper::BitmapAndSource>& bitmap_vector,
    int size) {
  for (std::vector<BookmarkAppHelper::BitmapAndSource>::const_iterator it =
           bitmap_vector.begin();
       it != bitmap_vector.end(); ++it) {
    if (it->bitmap.width() >= size) {
      return it;
    }
  }
  return bitmap_vector.end();
}

void ValidateIconsGeneratedAndResizedCorrectly(
    std::vector<BookmarkAppHelper::BitmapAndSource> downloaded,
    std::map<int, BookmarkAppHelper::BitmapAndSource> size_map,
    std::set<int> sizes_to_generate,
    int expected_generated,
    int expected_resized) {
  GURL empty_url("");
  int number_generated = 0;
  int number_resized = 0;

  auto icon_largest = FindLargestBitmapAndSourceVector(downloaded);
  for (const auto& size : sizes_to_generate) {
    auto icon_downloaded = FindMatchingBitmapAndSourceVector(downloaded, size);
    auto icon_larger = FindEqualOrLargerBitmapAndSourceVector(downloaded, size);
    if (icon_downloaded == downloaded.end()) {
      auto icon_resized = size_map.find(size);
      if (icon_largest == downloaded.end()) {
        // There are no downloaded icons. Expect an icon to be generated.
        EXPECT_NE(size_map.end(), icon_resized);
        EXPECT_EQ(size, icon_resized->second.bitmap.width());
        EXPECT_EQ(size, icon_resized->second.bitmap.height());
        EXPECT_EQ(size, icon_resized->second.bitmap.height());
        EXPECT_EQ(empty_url, icon_resized->second.source_url);
        ++number_generated;
      } else {
        // If there is a larger downloaded icon, it should be resized. Otherwise
        // the largest downloaded icon should be resized.
        auto icon_to_resize = icon_largest;
        if (icon_larger != downloaded.end())
          icon_to_resize = icon_larger;
        EXPECT_NE(size_map.end(), icon_resized);
        EXPECT_EQ(size, icon_resized->second.bitmap.width());
        EXPECT_EQ(size, icon_resized->second.bitmap.height());
        EXPECT_EQ(size, icon_resized->second.bitmap.height());
        EXPECT_EQ(icon_to_resize->source_url, icon_resized->second.source_url);
        ++number_resized;
      }
    } else {
      // There is an icon of exactly this size downloaded. Expect no icon to be
      // generated, and the existing downloaded icon to be used.
      auto icon_resized = size_map.find(size);
      EXPECT_NE(size_map.end(), icon_resized);
      EXPECT_EQ(size, icon_resized->second.bitmap.width());
      EXPECT_EQ(size, icon_resized->second.bitmap.height());
      EXPECT_EQ(size, icon_downloaded->bitmap.width());
      EXPECT_EQ(size, icon_downloaded->bitmap.height());
      EXPECT_EQ(icon_downloaded->source_url, icon_resized->second.source_url);
    }
  }
  EXPECT_EQ(expected_generated, number_generated);
  EXPECT_EQ(expected_resized, number_resized);
}

void TestIconGeneration(int icon_size,
                        int expected_generated,
                        int expected_resized) {
  std::vector<BookmarkAppHelper::BitmapAndSource> downloaded;

  // Add an icon with a URL and bitmap. 'Download' it.
  WebApplicationInfo::IconInfo icon_info =
      CreateIconInfoWithBitmap(icon_size, SK_ColorRED);
  icon_info.url = GURL(kAppIconURL1);
  downloaded.push_back(BookmarkAppHelper::BitmapAndSource(
        icon_info.url, icon_info.data));

  // Now run the resizing/generation and validation.
  WebApplicationInfo web_app_info;
  std::map<int, BookmarkAppHelper::BitmapAndSource> size_map =
      BookmarkAppHelper::ResizeIconsAndGenerateMissing(
          downloaded, TestSizesToGenerate(), &web_app_info);

  ValidateIconsGeneratedAndResizedCorrectly(downloaded, size_map,
                                            TestSizesToGenerate(),
                                            expected_generated,
                                            expected_resized);
}

class TestBookmarkAppHelper : public BookmarkAppHelper {
 public:
  TestBookmarkAppHelper(ExtensionService* service,
                        WebApplicationInfo web_app_info,
                        content::WebContents* contents)
      : BookmarkAppHelper(service->profile(),
                          web_app_info,
                          contents,
                          WebappInstallSource::MENU_BROWSER_TAB),
        bitmap_(CreateSquareBitmapWithColor(32, SK_ColorRED)) {}

  ~TestBookmarkAppHelper() override {}

  void CreationComplete(const extensions::Extension* extension,
                        const WebApplicationInfo& web_app_info) {
    extension_ = extension;
  }

  void CompleteInstallableCheck(const char* manifest_url,
                                const blink::Manifest& manifest,
                                ForInstallableSite for_installable_site) {
    bool installable = for_installable_site == ForInstallableSite::kYes;
    InstallableData data = {
        installable ? NO_ERROR_DETECTED : MANIFEST_DISPLAY_NOT_SUPPORTED,
        GURL(manifest_url),
        &manifest,
        GURL(kAppIconURL1),
        &bitmap_,
        GURL(),
        nullptr,
        installable,
        installable,
    };
    BookmarkAppHelper::OnDidPerformInstallableCheck(data);
  }

  void CompleteIconDownload(
      bool success,
      const std::map<GURL, std::vector<SkBitmap> >& bitmaps) {
    BookmarkAppHelper::OnIconsDownloaded(success, bitmaps);
  }

  const Extension* extension() { return extension_; }

  const FaviconDownloader* favicon_downloader() {
    return favicon_downloader_.get();
  }

 private:
  const Extension* extension_;
  SkBitmap bitmap_;

  DISALLOW_COPY_AND_ASSIGN(TestBookmarkAppHelper);
};

}  // namespace

TEST_F(BookmarkAppHelperExtensionServiceTest, CreateBookmarkApp) {
  WebApplicationInfo web_app_info;
  web_app_info.app_url = GURL(kAppUrl);
  web_app_info.title = base::UTF8ToUTF16(kAppTitle);
  web_app_info.description = base::UTF8ToUTF16(kAppDescription);

  std::unique_ptr<content::WebContents> contents(
      content::WebContentsTester::CreateTestWebContents(profile(), nullptr));
  TestBookmarkAppHelper helper(service_, web_app_info, contents.get());
  helper.Create(base::Bind(&TestBookmarkAppHelper::CreationComplete,
                           base::Unretained(&helper)));

  helper.CompleteInstallableCheck(kManifestUrl, blink::Manifest(),
                                  ForInstallableSite::kNo);

  std::map<GURL, std::vector<SkBitmap> > icon_map;
  icon_map[GURL(kAppUrl)].push_back(
      CreateSquareBitmapWithColor(kIconSizeSmall, SK_ColorRED));
  helper.CompleteIconDownload(true, icon_map);

  content::RunAllTasksUntilIdle();
  EXPECT_TRUE(helper.extension());
  const Extension* extension =
      service_->GetInstalledExtension(helper.extension()->id());
  EXPECT_TRUE(extension);
  EXPECT_EQ(1u, registry()->enabled_extensions().size());
  EXPECT_TRUE(extension->from_bookmark());
  EXPECT_EQ(kAppTitle, extension->name());
  EXPECT_EQ(kAppDescription, extension->description());
  EXPECT_EQ(GURL(kAppUrl), AppLaunchInfo::GetLaunchWebURL(extension));
  EXPECT_FALSE(
      IconsInfo::GetIconResource(
          extension, kIconSizeSmall, ExtensionIconSet::MATCH_EXACTLY).empty());
  EXPECT_FALSE(
      AppBannerSettingsHelper::GetSingleBannerEvent(
          contents.get(), web_app_info.app_url, web_app_info.app_url.spec(),
          AppBannerSettingsHelper::APP_BANNER_EVENT_DID_ADD_TO_HOMESCREEN)
          .is_null());
}

class BookmarkAppHelperExtensionServiceInstallableSiteTest
    : public BookmarkAppHelperExtensionServiceTest,
      public ::testing::WithParamInterface<ForInstallableSite> {
 public:
  BookmarkAppHelperExtensionServiceInstallableSiteTest() {}
  ~BookmarkAppHelperExtensionServiceInstallableSiteTest() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(
      BookmarkAppHelperExtensionServiceInstallableSiteTest);
};

TEST_P(BookmarkAppHelperExtensionServiceInstallableSiteTest,
       CreateBookmarkAppWithManifest) {
  WebApplicationInfo web_app_info;

  std::unique_ptr<content::WebContents> contents(
      content::WebContentsTester::CreateTestWebContents(profile(), nullptr));
  TestBookmarkAppHelper helper(service_, web_app_info, contents.get());
  helper.Create(base::Bind(&TestBookmarkAppHelper::CreationComplete,
                           base::Unretained(&helper)));

  blink::Manifest manifest;
  manifest.start_url = GURL(kAppUrl);
  manifest.name = base::NullableString16(base::UTF8ToUTF16(kAppTitle), false);
  manifest.scope = GURL(kAppScope);
  manifest.theme_color = SK_ColorBLUE;
  helper.CompleteInstallableCheck(kManifestUrl, manifest, GetParam());

  std::map<GURL, std::vector<SkBitmap> > icon_map;
  helper.CompleteIconDownload(true, icon_map);

  content::RunAllTasksUntilIdle();
  EXPECT_TRUE(helper.extension());
  const Extension* extension =
      service_->GetInstalledExtension(helper.extension()->id());
  EXPECT_TRUE(extension);
  EXPECT_EQ(1u, registry()->enabled_extensions().size());
  EXPECT_TRUE(extension->from_bookmark());
  EXPECT_EQ(kAppTitle, extension->name());
  EXPECT_EQ(GURL(kAppUrl), AppLaunchInfo::GetLaunchWebURL(extension));
  EXPECT_EQ(SK_ColorBLUE, AppThemeColorInfo::GetThemeColor(extension).value());
  EXPECT_FALSE(
      AppBannerSettingsHelper::GetSingleBannerEvent(
          contents.get(), manifest.start_url, manifest.start_url.spec(),
          AppBannerSettingsHelper::APP_BANNER_EVENT_DID_ADD_TO_HOMESCREEN)
          .is_null());

  if (GetParam() == ForInstallableSite::kYes) {
    EXPECT_EQ(GURL(kAppScope), GetScopeURLFromBookmarkApp(extension));
  } else {
    EXPECT_EQ(GURL(), GetScopeURLFromBookmarkApp(extension));
  }
}

TEST_P(BookmarkAppHelperExtensionServiceInstallableSiteTest,
       CreateBookmarkAppWithManifestIcons) {
  WebApplicationInfo web_app_info;
  std::unique_ptr<content::WebContents> contents(
      content::WebContentsTester::CreateTestWebContents(profile(), nullptr));
  TestBookmarkAppHelper helper(service_, web_app_info, contents.get());
  helper.Create(base::Bind(&TestBookmarkAppHelper::CreationComplete,
                           base::Unretained(&helper)));

  blink::Manifest manifest;
  manifest.start_url = GURL(kAppUrl);
  manifest.name = base::NullableString16(base::UTF8ToUTF16(kAppTitle), false);
  manifest.scope = GURL(kAppScope);
  blink::Manifest::Icon icon;
  icon.src = GURL(kAppIconURL1);
  manifest.icons.push_back(icon);
  icon.src = GURL(kAppIconURL2);
  manifest.icons.push_back(icon);
  helper.CompleteInstallableCheck(kManifestUrl, manifest, GetParam());

  // Favicon URLs are ignored because the site has a manifest with icons.
  EXPECT_FALSE(helper.favicon_downloader()->need_favicon_urls_);

  // Only 1 icon should be downloading since the other was provided by the
  // InstallableManager.
  EXPECT_EQ(1u, helper.favicon_downloader()->in_progress_requests_.size());

  std::map<GURL, std::vector<SkBitmap>> icon_map;
  icon_map[GURL(kAppIconURL2)].push_back(
      CreateSquareBitmapWithColor(kIconSizeSmall, SK_ColorRED));
  helper.CompleteIconDownload(true, icon_map);

  content::RunAllTasksUntilIdle();
  EXPECT_TRUE(helper.extension());
  const Extension* extension =
      service_->GetInstalledExtension(helper.extension()->id());
  EXPECT_TRUE(extension);
  EXPECT_EQ(1u, registry()->enabled_extensions().size());
  EXPECT_TRUE(extension->from_bookmark());
  EXPECT_EQ(kAppTitle, extension->name());
  EXPECT_EQ(GURL(kAppUrl), AppLaunchInfo::GetLaunchWebURL(extension));

  if (GetParam() == ForInstallableSite::kYes) {
    EXPECT_EQ(GURL(kAppScope), GetScopeURLFromBookmarkApp(extension));
  } else {
    EXPECT_EQ(GURL(), GetScopeURLFromBookmarkApp(extension));
  }
}

TEST_P(BookmarkAppHelperExtensionServiceInstallableSiteTest,
       CreateBookmarkAppWithManifestNoScope) {
  WebApplicationInfo web_app_info;

  std::unique_ptr<content::WebContents> contents(
      content::WebContentsTester::CreateTestWebContents(profile(), nullptr));
  TestBookmarkAppHelper helper(service_, web_app_info, contents.get());
  helper.Create(base::Bind(&TestBookmarkAppHelper::CreationComplete,
                           base::Unretained(&helper)));

  blink::Manifest manifest;
  manifest.start_url = GURL(kAppUrl);
  manifest.name = base::NullableString16(base::UTF8ToUTF16(kAppTitle), false);
  helper.CompleteInstallableCheck(kManifestUrl, manifest, GetParam());

  std::map<GURL, std::vector<SkBitmap>> icon_map;
  helper.CompleteIconDownload(true, icon_map);
  content::RunAllTasksUntilIdle();
  EXPECT_TRUE(helper.extension());

  const Extension* extension =
      service_->GetInstalledExtension(helper.extension()->id());
  if (GetParam() == ForInstallableSite::kYes) {
    EXPECT_EQ(GURL(kAppDefaultScope), GetScopeURLFromBookmarkApp(extension));
  } else {
    EXPECT_EQ(GURL(), GetScopeURLFromBookmarkApp(extension));
  }
}

INSTANTIATE_TEST_CASE_P(/* no prefix */,
                        BookmarkAppHelperExtensionServiceInstallableSiteTest,
                        ::testing::Values(ForInstallableSite::kNo,
                                          ForInstallableSite::kYes));

TEST_F(BookmarkAppHelperExtensionServiceTest,
       CreateBookmarkAppDefaultLauncherContainers) {
  auto scoped_feature_list = std::make_unique<base::test::ScopedFeatureList>();
  scoped_feature_list->InitAndEnableFeature(features::kDesktopPWAWindowing);

  WebApplicationInfo web_app_info;
  std::map<GURL, std::vector<SkBitmap>> icon_map;

  std::unique_ptr<content::WebContents> contents(
      content::WebContentsTester::CreateTestWebContents(profile(), nullptr));
  blink::Manifest manifest;
  manifest.start_url = GURL(kAppUrl);
  manifest.name = base::NullableString16(base::UTF8ToUTF16(kAppTitle), false);
  manifest.scope = GURL(kAppScope);
  {
    TestBookmarkAppHelper helper(service_, web_app_info, contents.get());
    helper.Create(base::Bind(&TestBookmarkAppHelper::CreationComplete,
                             base::Unretained(&helper)));

    helper.CompleteInstallableCheck(kManifestUrl, manifest,
                                    ForInstallableSite::kYes);
    helper.CompleteIconDownload(true, icon_map);

    content::RunAllTasksUntilIdle();
    EXPECT_TRUE(helper.extension());
    const Extension* extension =
        service_->GetInstalledExtension(helper.extension()->id());
    EXPECT_TRUE(extension);
    EXPECT_EQ(LAUNCH_CONTAINER_WINDOW,
              GetLaunchContainer(ExtensionPrefs::Get(profile()), extension));
  }
  {
    TestBookmarkAppHelper helper(service_, web_app_info, contents.get());
    helper.Create(base::Bind(&TestBookmarkAppHelper::CreationComplete,
                             base::Unretained(&helper)));

    helper.CompleteInstallableCheck(kManifestUrl, manifest,
                                    ForInstallableSite::kNo);
    helper.CompleteIconDownload(true, icon_map);

    content::RunAllTasksUntilIdle();
    EXPECT_TRUE(helper.extension());
    const Extension* extension =
        service_->GetInstalledExtension(helper.extension()->id());
    EXPECT_TRUE(extension);
    EXPECT_EQ(LAUNCH_CONTAINER_TAB,
              GetLaunchContainer(ExtensionPrefs::Get(profile()), extension));
  }
}

TEST_F(BookmarkAppHelperExtensionServiceTest,
       CreateBookmarkAppWithoutManifest) {
  WebApplicationInfo web_app_info;
  web_app_info.title = base::UTF8ToUTF16(kAppTitle);
  web_app_info.description = base::UTF8ToUTF16(kAppDescription);
  web_app_info.app_url = GURL(kAppUrl);

  std::unique_ptr<content::WebContents> contents(
      content::WebContentsTester::CreateTestWebContents(profile(), nullptr));
  TestBookmarkAppHelper helper(service_, web_app_info, contents.get());
  helper.Create(base::Bind(&TestBookmarkAppHelper::CreationComplete,
                           base::Unretained(&helper)));

  helper.CompleteInstallableCheck(kManifestUrl, blink::Manifest(),
                                  ForInstallableSite::kNo);
  std::map<GURL, std::vector<SkBitmap>> icon_map;
  helper.CompleteIconDownload(true, icon_map);
  content::RunAllTasksUntilIdle();
  EXPECT_TRUE(helper.extension());

  const Extension* extension =
      service_->GetInstalledExtension(helper.extension()->id());
  EXPECT_TRUE(extension);
  EXPECT_EQ(kAppTitle, extension->name());
  EXPECT_EQ(kAppDescription, extension->description());
  EXPECT_EQ(GURL(kAppUrl), AppLaunchInfo::GetLaunchWebURL(extension));
  EXPECT_EQ(GURL(), GetScopeURLFromBookmarkApp(extension));
  EXPECT_FALSE(AppThemeColorInfo::GetThemeColor(extension));
}

TEST_F(BookmarkAppHelperExtensionServiceTest, CreateBookmarkAppNoContents) {
  WebApplicationInfo web_app_info;
  web_app_info.app_url = GURL(kAppUrl);
  web_app_info.title = base::UTF8ToUTF16(kAppTitle);
  web_app_info.description = base::UTF8ToUTF16(kAppDescription);
  web_app_info.scope = GURL(kAppScope);
  web_app_info.icons.push_back(
      CreateIconInfoWithBitmap(kIconSizeTiny, SK_ColorRED));

  TestBookmarkAppHelper helper(service_, web_app_info, NULL);
  helper.Create(base::Bind(&TestBookmarkAppHelper::CreationComplete,
                           base::Unretained(&helper)));

  content::RunAllTasksUntilIdle();
  EXPECT_TRUE(helper.extension());
  const Extension* extension =
      service_->GetInstalledExtension(helper.extension()->id());
  EXPECT_TRUE(extension);
  EXPECT_EQ(1u, registry()->enabled_extensions().size());
  EXPECT_TRUE(extension->from_bookmark());
  EXPECT_EQ(kAppTitle, extension->name());
  EXPECT_EQ(kAppDescription, extension->description());
  EXPECT_EQ(GURL(kAppUrl), AppLaunchInfo::GetLaunchWebURL(extension));
  EXPECT_EQ(GURL(kAppScope), GetScopeURLFromBookmarkApp(extension));
  EXPECT_FALSE(
      IconsInfo::GetIconResource(extension, kIconSizeTiny,
                                 ExtensionIconSet::MATCH_EXACTLY).empty());
  EXPECT_FALSE(
      IconsInfo::GetIconResource(
          extension, kIconSizeSmall, ExtensionIconSet::MATCH_EXACTLY).empty());
  EXPECT_FALSE(
      IconsInfo::GetIconResource(extension,
                                 kIconSizeSmall * 2,
                                 ExtensionIconSet::MATCH_EXACTLY).empty());
  EXPECT_FALSE(
      IconsInfo::GetIconResource(
          extension, kIconSizeMedium, ExtensionIconSet::MATCH_EXACTLY).empty());
  EXPECT_FALSE(
      IconsInfo::GetIconResource(extension,
                                 kIconSizeMedium * 2,
                                 ExtensionIconSet::MATCH_EXACTLY).empty());
}

TEST_F(BookmarkAppHelperExtensionServiceTest, CreateAndUpdateBookmarkApp) {
  EXPECT_EQ(0u, registry()->enabled_extensions().size());
  WebApplicationInfo web_app_info;
  web_app_info.app_url = GURL(kAppUrl);
  web_app_info.title = base::UTF8ToUTF16(kAppTitle);
  web_app_info.description = base::UTF8ToUTF16(kAppDescription);
  web_app_info.scope = GURL(kAppScope);
  web_app_info.icons.push_back(
      CreateIconInfoWithBitmap(kIconSizeSmall, SK_ColorRED));

  extensions::CreateOrUpdateBookmarkApp(service_, &web_app_info);
  content::RunAllTasksUntilIdle();

  {
    EXPECT_EQ(1u, registry()->enabled_extensions().size());
    const Extension* extension =
        registry()->enabled_extensions().begin()->get();
    EXPECT_TRUE(extension->from_bookmark());
    EXPECT_EQ(kAppTitle, extension->name());
    EXPECT_EQ(kAppDescription, extension->description());
    EXPECT_EQ(GURL(kAppUrl), AppLaunchInfo::GetLaunchWebURL(extension));
    EXPECT_EQ(GURL(kAppScope), GetScopeURLFromBookmarkApp(extension));
    EXPECT_FALSE(extensions::IconsInfo::GetIconResource(
                     extension, kIconSizeSmall, ExtensionIconSet::MATCH_EXACTLY)
                     .empty());
  }

  web_app_info.title = base::UTF8ToUTF16(kAlternativeAppTitle);
  web_app_info.icons[0] = CreateIconInfoWithBitmap(kIconSizeLarge, SK_ColorRED);
  web_app_info.scope = GURL(kAppAlternativeScope);

  extensions::CreateOrUpdateBookmarkApp(service_, &web_app_info);
  content::RunAllTasksUntilIdle();

  {
    EXPECT_EQ(1u, registry()->enabled_extensions().size());
    const Extension* extension =
        registry()->enabled_extensions().begin()->get();
    EXPECT_TRUE(extension->from_bookmark());
    EXPECT_EQ(kAlternativeAppTitle, extension->name());
    EXPECT_EQ(kAppDescription, extension->description());
    EXPECT_EQ(GURL(kAppUrl), AppLaunchInfo::GetLaunchWebURL(extension));
    EXPECT_EQ(GURL(kAppAlternativeScope),
              GetScopeURLFromBookmarkApp(extension));
    EXPECT_FALSE(extensions::IconsInfo::GetIconResource(
                     extension, kIconSizeSmall, ExtensionIconSet::MATCH_EXACTLY)
                     .empty());
    EXPECT_FALSE(extensions::IconsInfo::GetIconResource(
                     extension, kIconSizeLarge, ExtensionIconSet::MATCH_EXACTLY)
                     .empty());
  }
}

TEST_F(BookmarkAppHelperExtensionServiceTest, LinkedAppIconsAreNotChanged) {
  WebApplicationInfo web_app_info;

  // Add two icons with a URL and bitmap, two icons with just a bitmap, an
  // icon with just URL and an icon in an unsupported size with just a URL.
  WebApplicationInfo::IconInfo icon_info =
      CreateIconInfoWithBitmap(kIconSizeSmall, SK_ColorRED);
  icon_info.url = GURL(kAppIconURL1);
  web_app_info.icons.push_back(icon_info);

  icon_info = CreateIconInfoWithBitmap(kIconSizeMedium, SK_ColorRED);
  icon_info.url = GURL(kAppIconURL2);
  web_app_info.icons.push_back(icon_info);

  icon_info.data = SkBitmap();
  icon_info.url = GURL(kAppIconURL3);
  icon_info.width = 0;
  icon_info.height = 0;
  web_app_info.icons.push_back(icon_info);

  icon_info.url = GURL(kAppIconURL4);
  web_app_info.icons.push_back(icon_info);

  icon_info = CreateIconInfoWithBitmap(kIconSizeLarge, SK_ColorRED);
  web_app_info.icons.push_back(icon_info);

  icon_info = CreateIconInfoWithBitmap(kIconSizeUnsupported, SK_ColorRED);
  web_app_info.icons.push_back(icon_info);

  // 'Download' one of the icons without a size or bitmap.
  std::vector<BookmarkAppHelper::BitmapAndSource> downloaded;
  downloaded.push_back(BookmarkAppHelper::BitmapAndSource(
      GURL(kAppIconURL3),
      CreateSquareBitmapWithColor(kIconSizeLarge, SK_ColorBLACK)));

  // Now run the resizing and generation into a new web app info.
  WebApplicationInfo new_web_app_info;
  std::map<int, BookmarkAppHelper::BitmapAndSource> size_map =
      BookmarkAppHelper::ResizeIconsAndGenerateMissing(
          downloaded, TestSizesToGenerate(), &new_web_app_info);

  // Now check that the linked app icons (i.e. those with URLs) are matching in
  // both lists.
  ValidateAllIconsWithURLsArePresent(web_app_info, new_web_app_info);
  ValidateAllIconsWithURLsArePresent(new_web_app_info, web_app_info);
}

TEST_F(BookmarkAppHelperTest, UpdateWebAppInfoFromManifest) {
  WebApplicationInfo web_app_info;
  web_app_info.title = base::UTF8ToUTF16(kAlternativeAppTitle);
  web_app_info.app_url = GURL(kAlternativeAppUrl);
  WebApplicationInfo::IconInfo info;
  info.url = GURL(kAppIcon1);
  web_app_info.icons.push_back(info);

  blink::Manifest manifest;
  manifest.start_url = GURL(kAppUrl);
  manifest.short_name = base::NullableString16(base::UTF8ToUTF16(kAppShortName),
                                               false);

  BookmarkAppHelper::UpdateWebAppInfoFromManifest(manifest, &web_app_info,
                                                  ForInstallableSite::kNo);
  EXPECT_EQ(base::UTF8ToUTF16(kAppShortName), web_app_info.title);
  EXPECT_EQ(GURL(kAppUrl), web_app_info.app_url);

  // The icon info from |web_app_info| should be left as is, since the manifest
  // doesn't have any icon information.
  EXPECT_EQ(1u, web_app_info.icons.size());
  EXPECT_EQ(GURL(kAppIcon1), web_app_info.icons[0].url);

  // Test that |manifest.name| takes priority over |manifest.short_name|, and
  // that icons provided by the manifest replace icons in |web_app_info|.
  manifest.name = base::NullableString16(base::UTF8ToUTF16(kAppTitle), false);

  blink::Manifest::Icon icon;
  icon.src = GURL(kAppIcon2);
  manifest.icons.push_back(icon);
  icon.src = GURL(kAppIcon3);
  manifest.icons.push_back(icon);

  BookmarkAppHelper::UpdateWebAppInfoFromManifest(
      manifest, &web_app_info, BookmarkAppHelper::ForInstallableSite::kNo);
  EXPECT_EQ(base::UTF8ToUTF16(kAppTitle), web_app_info.title);

  EXPECT_EQ(2u, web_app_info.icons.size());
  EXPECT_EQ(GURL(kAppIcon2), web_app_info.icons[0].url);
  EXPECT_EQ(GURL(kAppIcon3), web_app_info.icons[1].url);
}

// Tests "scope" is only set for installable sites.
TEST_F(BookmarkAppHelperTest, UpdateWebAppInfoFromManifestInstallableSite) {
  {
    blink::Manifest manifest;
    manifest.start_url = GURL(kAppUrl);
    WebApplicationInfo web_app_info;
    BookmarkAppHelper::UpdateWebAppInfoFromManifest(
        manifest, &web_app_info,
        BookmarkAppHelper::ForInstallableSite::kUnknown);
    EXPECT_EQ(GURL(), web_app_info.scope);
  }

  {
    blink::Manifest manifest;
    manifest.start_url = GURL(kAppUrl);
    WebApplicationInfo web_app_info;
    BookmarkAppHelper::UpdateWebAppInfoFromManifest(
        manifest, &web_app_info, BookmarkAppHelper::ForInstallableSite::kNo);
    EXPECT_EQ(GURL(), web_app_info.scope);
  }

  {
    blink::Manifest manifest;
    manifest.start_url = GURL(kAppUrl);
    WebApplicationInfo web_app_info;
    BookmarkAppHelper::UpdateWebAppInfoFromManifest(
        manifest, &web_app_info, BookmarkAppHelper::ForInstallableSite::kYes);

    EXPECT_NE(GURL(), web_app_info.scope);
  }
}

TEST_F(BookmarkAppHelperTest, ConstrainBitmapsToSizes) {
  std::set<int> desired_sizes;
  desired_sizes.insert(16);
  desired_sizes.insert(32);
  desired_sizes.insert(48);
  desired_sizes.insert(96);
  desired_sizes.insert(128);
  desired_sizes.insert(256);

  {
    std::vector<BookmarkAppHelper::BitmapAndSource> bitmaps;
    bitmaps.push_back(CreateSquareBitmapAndSourceWithColor(16, SK_ColorRED));
    bitmaps.push_back(CreateSquareBitmapAndSourceWithColor(32, SK_ColorGREEN));
    bitmaps.push_back(
        CreateSquareBitmapAndSourceWithColor(144, SK_ColorYELLOW));

    std::map<int, BookmarkAppHelper::BitmapAndSource> results(
        BookmarkAppHelper::ConstrainBitmapsToSizes(bitmaps, desired_sizes));

    EXPECT_EQ(6u, results.size());
    ValidateBitmapSizeAndColor(results[16].bitmap, 16, SK_ColorRED);
    ValidateBitmapSizeAndColor(results[32].bitmap, 32, SK_ColorGREEN);
    ValidateBitmapSizeAndColor(results[48].bitmap, 48, SK_ColorYELLOW);
    ValidateBitmapSizeAndColor(results[96].bitmap, 96, SK_ColorYELLOW);
    ValidateBitmapSizeAndColor(results[128].bitmap, 128, SK_ColorYELLOW);
    ValidateBitmapSizeAndColor(results[256].bitmap, 256, SK_ColorYELLOW);
  }
  {
    std::vector<BookmarkAppHelper::BitmapAndSource> bitmaps;
    bitmaps.push_back(CreateSquareBitmapAndSourceWithColor(512, SK_ColorRED));
    bitmaps.push_back(CreateSquareBitmapAndSourceWithColor(18, SK_ColorGREEN));
    bitmaps.push_back(CreateSquareBitmapAndSourceWithColor(33, SK_ColorBLUE));
    bitmaps.push_back(CreateSquareBitmapAndSourceWithColor(17, SK_ColorYELLOW));

    std::map<int, BookmarkAppHelper::BitmapAndSource> results(
        BookmarkAppHelper::ConstrainBitmapsToSizes(bitmaps, desired_sizes));

    EXPECT_EQ(6u, results.size());
    ValidateBitmapSizeAndColor(results[16].bitmap, 16, SK_ColorYELLOW);
    ValidateBitmapSizeAndColor(results[32].bitmap, 32, SK_ColorBLUE);
    ValidateBitmapSizeAndColor(results[48].bitmap, 48, SK_ColorRED);
    ValidateBitmapSizeAndColor(results[96].bitmap, 96, SK_ColorRED);
    ValidateBitmapSizeAndColor(results[128].bitmap, 128, SK_ColorRED);
    ValidateBitmapSizeAndColor(results[256].bitmap, 256, SK_ColorRED);
  }
}

TEST_F(BookmarkAppHelperTest, IsValidBookmarkAppUrl) {
  EXPECT_TRUE(IsValidBookmarkAppUrl(GURL("https://chromium.org")));
  EXPECT_TRUE(IsValidBookmarkAppUrl(GURL("https://www.chromium.org")));
  EXPECT_TRUE(IsValidBookmarkAppUrl(
      GURL("https://www.chromium.org/path/to/page.html")));
  EXPECT_TRUE(IsValidBookmarkAppUrl(GURL("http://chromium.org")));
  EXPECT_TRUE(IsValidBookmarkAppUrl(GURL("http://www.chromium.org")));
  EXPECT_TRUE(
      IsValidBookmarkAppUrl(GURL("http://www.chromium.org/path/to/page.html")));
  EXPECT_TRUE(IsValidBookmarkAppUrl(
      GURL("chrome-extension://oafaagfgbdpldilgjjfjocjglfbolmac")));

  EXPECT_FALSE(IsValidBookmarkAppUrl(GURL("ftp://www.chromium.org")));
  EXPECT_FALSE(IsValidBookmarkAppUrl(GURL("chrome://flags")));
  EXPECT_FALSE(IsValidBookmarkAppUrl(GURL("about:blank")));
  EXPECT_FALSE(IsValidBookmarkAppUrl(
      GURL("file://mhjfbmdgcfjbbpaeojofohoefgiehjai")));
  EXPECT_FALSE(IsValidBookmarkAppUrl(GURL("chrome://extensions")));
}

TEST_F(BookmarkAppHelperTest, IconsResizedFromOddSizes) {
  std::vector<BookmarkAppHelper::BitmapAndSource> downloaded;

  // Add three icons with a URL and bitmap. 'Download' each of them.
  WebApplicationInfo::IconInfo icon_info =
      CreateIconInfoWithBitmap(kIconSizeSmall, SK_ColorRED);
  icon_info.url = GURL(kAppIconURL1);
  downloaded.push_back(BookmarkAppHelper::BitmapAndSource(
        icon_info.url, icon_info.data));

  icon_info = CreateIconInfoWithBitmap(kIconSizeSmallBetweenMediumAndLarge,
                                       SK_ColorRED);
  icon_info.url = GURL(kAppIconURL2);
  downloaded.push_back(BookmarkAppHelper::BitmapAndSource(
        icon_info.url, icon_info.data));

  icon_info = CreateIconInfoWithBitmap(kIconSizeLargeBetweenMediumAndLarge,
                                       SK_ColorRED);
  icon_info.url = GURL(kAppIconURL3);
  downloaded.push_back(BookmarkAppHelper::BitmapAndSource(
        icon_info.url, icon_info.data));

  // Now run the resizing and generation.
  WebApplicationInfo web_app_info;
  std::map<int, BookmarkAppHelper::BitmapAndSource> size_map =
      BookmarkAppHelper::ResizeIconsAndGenerateMissing(
          downloaded, TestSizesToGenerate(), &web_app_info);

  // No icons should be generated. The LARGE and MEDIUM sizes should be resized.
  ValidateIconsGeneratedAndResizedCorrectly(
      downloaded, size_map, TestSizesToGenerate(), 0, 2);
}

TEST_F(BookmarkAppHelperTest, IconsResizedFromLarger) {
  std::vector<BookmarkAppHelper::BitmapAndSource> downloaded;

  // Add three icons with a URL and bitmap. 'Download' two of them and pretend
  // the third failed to download.
  WebApplicationInfo::IconInfo icon_info =
      CreateIconInfoWithBitmap(kIconSizeSmall, SK_ColorRED);
  icon_info.url = GURL(kAppIconURL1);
  downloaded.push_back(BookmarkAppHelper::BitmapAndSource(
        icon_info.url, icon_info.data));

  icon_info = CreateIconInfoWithBitmap(kIconSizeLarge, SK_ColorBLUE);
  icon_info.url = GURL(kAppIconURL2);

  icon_info = CreateIconInfoWithBitmap(kIconSizeGigantor, SK_ColorBLACK);
  icon_info.url = GURL(kAppIconURL3);
  downloaded.push_back(BookmarkAppHelper::BitmapAndSource(
        icon_info.url, icon_info.data));

  // Now run the resizing and generation.
  WebApplicationInfo web_app_info;
  std::map<int, BookmarkAppHelper::BitmapAndSource> size_map =
      BookmarkAppHelper::ResizeIconsAndGenerateMissing(
          downloaded, TestSizesToGenerate(), &web_app_info);

  // Expect icon for MEDIUM and LARGE to be resized from the gigantor icon
  // as it was not downloaded.
  ValidateIconsGeneratedAndResizedCorrectly(
      downloaded, size_map, TestSizesToGenerate(), 0, 2);
}

TEST_F(BookmarkAppHelperTest, AllIconsGeneratedWhenNotDownloaded) {
  std::vector<BookmarkAppHelper::BitmapAndSource> downloaded;

  // Add three icons with a URL and bitmap. 'Download' none of them.
  WebApplicationInfo::IconInfo icon_info =
      CreateIconInfoWithBitmap(kIconSizeSmall, SK_ColorRED);
  icon_info.url = GURL(kAppIconURL1);

  icon_info = CreateIconInfoWithBitmap(kIconSizeLarge, SK_ColorBLUE);
  icon_info.url = GURL(kAppIconURL2);

  icon_info = CreateIconInfoWithBitmap(kIconSizeGigantor, SK_ColorBLACK);
  icon_info.url = GURL(kAppIconURL3);

  // Now run the resizing and generation.
  WebApplicationInfo web_app_info;
  std::map<int, BookmarkAppHelper::BitmapAndSource> size_map =
      BookmarkAppHelper::ResizeIconsAndGenerateMissing(
          downloaded, TestSizesToGenerate(), &web_app_info);

  // Expect all icons to be generated.
  ValidateIconsGeneratedAndResizedCorrectly(
      downloaded, size_map, TestSizesToGenerate(), 3, 0);
}


TEST_F(BookmarkAppHelperTest, IconResizedFromLargerAndSmaller) {
  std::vector<BookmarkAppHelper::BitmapAndSource> downloaded;

  // Pretend the huge icon wasn't downloaded but two smaller ones were.
  WebApplicationInfo::IconInfo icon_info =
      CreateIconInfoWithBitmap(kIconSizeTiny, SK_ColorRED);
  icon_info.url = GURL(kAppIconURL1);
  downloaded.push_back(BookmarkAppHelper::BitmapAndSource(
        icon_info.url, icon_info.data));

  icon_info = CreateIconInfoWithBitmap(kIconSizeMedium, SK_ColorBLUE);
  icon_info.url = GURL(kAppIconURL2);
  downloaded.push_back(BookmarkAppHelper::BitmapAndSource(
        icon_info.url, icon_info.data));

  icon_info = CreateIconInfoWithBitmap(kIconSizeGigantor, SK_ColorBLACK);
  icon_info.url = GURL(kAppIconURL3);

  // Now run the resizing and generation.
  WebApplicationInfo web_app_info;
  std::map<int, BookmarkAppHelper::BitmapAndSource> size_map =
      BookmarkAppHelper::ResizeIconsAndGenerateMissing(
          downloaded, TestSizesToGenerate(), &web_app_info);

  // Expect no icons to be generated, but the LARGE and SMALL icons to be
  // resized from the MEDIUM icon.
  ValidateIconsGeneratedAndResizedCorrectly(
      downloaded, size_map, TestSizesToGenerate(), 0, 2);

  // Verify specifically that the LARGE icons was resized from the medium icon.
  const auto it = size_map.find(kIconSizeLarge);
  EXPECT_NE(size_map.end(), it);
  EXPECT_EQ(GURL(kAppIconURL2), it->second.source_url);
}

TEST_F(BookmarkAppHelperTest, IconsResizedWhenOnlyATinyOneIsProvided) {
  // When only a tiny icon is downloaded (smaller than the three desired
  // sizes), 3 icons should be resized.
  TestIconGeneration(kIconSizeTiny, 0, 3);
}

TEST_F(BookmarkAppHelperTest, IconsResizedWhenOnlyAGigantorOneIsProvided) {
  // When an enormous icon is provided, each desired icon size should be resized
  // from it, and no icons should be generated.
  TestIconGeneration(kIconSizeGigantor, 0, 3);
}

}  // namespace extensions
