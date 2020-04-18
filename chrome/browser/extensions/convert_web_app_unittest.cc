// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/convert_web_app.h"

#include <stddef.h>

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "base/version.h"
#include "chrome/browser/extensions/bookmark_app_helper.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/extensions/manifest_handlers/app_launch_info.h"
#include "chrome/common/web_application_info.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_icon_set.h"
#include "extensions/common/extension_resource.h"
#include "extensions/common/manifest_constants.h"
#include "extensions/common/manifest_handlers/icons_handler.h"
#include "extensions/common/permissions/permission_set.h"
#include "extensions/common/permissions/permissions_data.h"
#include "extensions/common/url_pattern.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/codec/png_codec.h"
#include "url/gurl.h"

namespace extensions {

namespace keys = manifest_keys;

namespace {

// Returns an icon info corresponding to a canned icon.
WebApplicationInfo::IconInfo GetIconInfo(const GURL& url, int size) {
  WebApplicationInfo::IconInfo result;

  base::FilePath icon_file;
  if (!base::PathService::Get(chrome::DIR_TEST_DATA, &icon_file)) {
    ADD_FAILURE() << "Could not get test data directory.";
    return result;
  }

  icon_file = icon_file.AppendASCII("extensions")
                       .AppendASCII("convert_web_app")
                       .AppendASCII(base::StringPrintf("%i.png", size));

  result.url = url;
  result.width = size;
  result.height = size;

  std::string icon_data;
  if (!base::ReadFileToString(icon_file, &icon_data)) {
    ADD_FAILURE() << "Could not read test icon.";
    return result;
  }

  if (!gfx::PNGCodec::Decode(
        reinterpret_cast<const unsigned char*>(icon_data.c_str()),
        icon_data.size(), &result.data)) {
    ADD_FAILURE() << "Could not decode test icon.";
    return result;
  }

  return result;
}

base::Time GetTestTime(int year, int month, int day, int hour, int minute,
                       int second, int millisecond) {
  base::Time::Exploded exploded = {0};
  exploded.year = year;
  exploded.month = month;
  exploded.day_of_month = day;
  exploded.hour = hour;
  exploded.minute = minute;
  exploded.second = second;
  exploded.millisecond = millisecond;
  base::Time out_time;
  EXPECT_TRUE(base::Time::FromUTCExploded(exploded, &out_time));
  return out_time;
}

}  // namespace

TEST(ExtensionFromWebApp, GetScopeURLFromBookmarkApp) {
  base::ScopedTempDir extensions_dir;
  ASSERT_TRUE(extensions_dir.CreateUniqueTempDir());

  base::DictionaryValue manifest;
  manifest.SetString(keys::kName, "Test App");
  manifest.SetString(keys::kVersion, "0");
  manifest.SetString(keys::kLaunchWebURL, "http://aaronboodman.com/gearpad/");

  // Create a "url_handlers" dictionary with one URL handler generated from
  // the scope.
  // {
  //   "scope": {
  //     "matches": [ "http://aaronboodman.com/gearpad/*" ],
  //     "title": "Test App"
  //   },
  // }
  GURL scope_url = GURL("http://aaronboodman.com/gearpad/");
  manifest.SetDictionary(keys::kUrlHandlers,
                         CreateURLHandlersForBookmarkApp(
                             scope_url, base::ASCIIToUTF16("Test App")));

  std::string error;
  scoped_refptr<Extension> bookmark_app =
      Extension::Create(extensions_dir.GetPath(), Manifest::INTERNAL, manifest,
                        Extension::FROM_BOOKMARK, &error);
  ASSERT_TRUE(bookmark_app.get());

  EXPECT_EQ(scope_url, GetScopeURLFromBookmarkApp(bookmark_app.get()));
}

TEST(ExtensionFromWebApp, GetScopeURLFromBookmarkApp_NoURLHandlers) {
  base::ScopedTempDir extensions_dir;
  ASSERT_TRUE(extensions_dir.CreateUniqueTempDir());

  base::DictionaryValue manifest;
  manifest.SetString(keys::kName, "Test App");
  manifest.SetString(keys::kVersion, "0");
  manifest.SetString(keys::kLaunchWebURL, "http://aaronboodman.com/gearpad/");
  manifest.SetDictionary(keys::kUrlHandlers,
                         std::make_unique<base::DictionaryValue>());

  std::string error;
  scoped_refptr<Extension> bookmark_app =
      Extension::Create(extensions_dir.GetPath(), Manifest::INTERNAL, manifest,
                        Extension::FROM_BOOKMARK, &error);
  ASSERT_TRUE(bookmark_app.get());

  EXPECT_EQ(GURL(), GetScopeURLFromBookmarkApp(bookmark_app.get()));
}

TEST(ExtensionFromWebApp, GetScopeURLFromBookmarkApp_WrongURLHandler) {
  base::ScopedTempDir extensions_dir;
  ASSERT_TRUE(extensions_dir.CreateUniqueTempDir());

  base::DictionaryValue manifest;
  manifest.SetString(keys::kName, "Test App");
  manifest.SetString(keys::kVersion, "0");
  manifest.SetString(keys::kLaunchWebURL, "http://aaronboodman.com/gearpad/");

  // Create a "url_handlers" dictionary with one URL handler not generated
  // from the scope.
  // {
  //   "test_url_handler": {
  //     "matches": [ "http://*.aaronboodman.com/" ],
  //     "title": "test handler"
  //   }
  // }
  auto test_matches = std::make_unique<base::ListValue>();
  test_matches->AppendString("http://*.aaronboodman.com/");

  auto test_handler = std::make_unique<base::DictionaryValue>();
  test_handler->SetList(keys::kMatches, std::move(test_matches));
  test_handler->SetString(keys::kUrlHandlerTitle, "test handler");

  auto url_handlers = std::make_unique<base::DictionaryValue>();
  url_handlers->SetDictionary("test_url_handler", std::move(test_handler));
  manifest.SetDictionary(keys::kUrlHandlers, std::move(url_handlers));

  std::string error;
  scoped_refptr<Extension> bookmark_app =
      Extension::Create(extensions_dir.GetPath(), Manifest::INTERNAL, manifest,
                        Extension::FROM_BOOKMARK, &error);
  ASSERT_TRUE(bookmark_app.get());

  EXPECT_EQ(GURL(), GetScopeURLFromBookmarkApp(bookmark_app.get()));
}

TEST(ExtensionFromWebApp, GetScopeURLFromBookmarkApp_ExtraURLHandler) {
  base::ScopedTempDir extensions_dir;
  ASSERT_TRUE(extensions_dir.CreateUniqueTempDir());

  base::DictionaryValue manifest;
  manifest.SetString(keys::kName, "Test App");
  manifest.SetString(keys::kVersion, "0");
  manifest.SetString(keys::kLaunchWebURL, "http://aaronboodman.com/gearpad/");

  // Create a "url_handlers" dictionary with two URL handlers. One for
  // the scope and and extra one for testing.
  // {
  //   "scope": {
  //     "matches": [ "http://aaronboodman.com/gearpad/*" ],
  //     "title": "Test App"
  //   },
  //   "test_url_handler": {
  //     "matches": [ "http://*.aaronboodman.com/" ],
  //     "title": "test handler"
  //   }
  // }
  GURL scope_url = GURL("http://aaronboodman.com/gearpad/");
  std::unique_ptr<base::DictionaryValue> url_handlers =
      CreateURLHandlersForBookmarkApp(scope_url,
                                      base::ASCIIToUTF16("Test App"));

  auto test_matches = std::make_unique<base::ListValue>();
  test_matches->AppendString("http://*.aaronboodman.com/");

  auto test_handler = std::make_unique<base::DictionaryValue>();
  test_handler->SetList(keys::kMatches, std::move(test_matches));
  test_handler->SetString(keys::kUrlHandlerTitle, "test handler");

  url_handlers->SetDictionary("test_url_handler", std::move(test_handler));
  manifest.SetDictionary(keys::kUrlHandlers, std::move(url_handlers));

  std::string error;
  scoped_refptr<Extension> bookmark_app =
      Extension::Create(extensions_dir.GetPath(), Manifest::INTERNAL, manifest,
                        Extension::FROM_BOOKMARK, &error);
  ASSERT_TRUE(bookmark_app.get());

  // Check that we can retrieve the scope even if there is an extra
  // url handler.
  EXPECT_EQ(scope_url, GetScopeURLFromBookmarkApp(bookmark_app.get()));
}

TEST(ExtensionFromWebApp, GenerateVersion) {
  EXPECT_EQ("2010.1.1.0",
            ConvertTimeToExtensionVersion(
                GetTestTime(2010, 1, 1, 0, 0, 0, 0)));
  EXPECT_EQ("2010.12.31.22111",
            ConvertTimeToExtensionVersion(
                GetTestTime(2010, 12, 31, 8, 5, 50, 500)));
  EXPECT_EQ("2010.10.1.65535",
            ConvertTimeToExtensionVersion(
                GetTestTime(2010, 10, 1, 23, 59, 59, 999)));
}

TEST(ExtensionFromWebApp, Basic) {
  base::ScopedTempDir extensions_dir;
  ASSERT_TRUE(extensions_dir.CreateUniqueTempDir());

  WebApplicationInfo web_app;
  web_app.title = base::ASCIIToUTF16("Gearpad");
  web_app.description =
      base::ASCIIToUTF16("The best text editor in the universe!");
  web_app.app_url = GURL("http://aaronboodman.com/gearpad/");
  web_app.scope = GURL("http://aaronboodman.com/gearpad/");

  const int sizes[] = {16, 48, 128};
  for (size_t i = 0; i < arraysize(sizes); ++i) {
    GURL icon_url(
        web_app.app_url.Resolve(base::StringPrintf("%i.png", sizes[i])));
    web_app.icons.push_back(GetIconInfo(icon_url, sizes[i]));
  }

  scoped_refptr<Extension> extension = ConvertWebAppToExtension(
      web_app, GetTestTime(1978, 12, 11, 0, 0, 0, 0), extensions_dir.GetPath());
  ASSERT_TRUE(extension.get());

  base::ScopedTempDir extension_dir;
  EXPECT_TRUE(extension_dir.Set(extension->path()));

  EXPECT_TRUE(extension->is_app());
  EXPECT_TRUE(extension->is_hosted_app());
  EXPECT_FALSE(extension->is_legacy_packaged_app());

  EXPECT_EQ("zVvdNZy3Mp7CFU8JVSyXNlDuHdVLbP7fDO3TGVzj/0w=",
            extension->public_key());
  EXPECT_EQ("oplhagaaipaimkjlbekcdjkffijdockj", extension->id());
  EXPECT_EQ("1978.12.11.0", extension->version().GetString());
  EXPECT_EQ(base::UTF16ToUTF8(web_app.title), extension->name());
  EXPECT_EQ(base::UTF16ToUTF8(web_app.description), extension->description());
  EXPECT_EQ(web_app.app_url, AppLaunchInfo::GetFullLaunchURL(extension.get()));
  EXPECT_EQ(web_app.scope, GetScopeURLFromBookmarkApp(extension.get()));
  EXPECT_EQ(0u,
            extension->permissions_data()->active_permissions().apis().size());
  ASSERT_EQ(0u, extension->web_extent().patterns().size());

  EXPECT_EQ(web_app.icons.size(),
            IconsInfo::GetIcons(extension.get()).map().size());
  for (size_t i = 0; i < web_app.icons.size(); ++i) {
    EXPECT_EQ(base::StringPrintf("icons/%i.png", web_app.icons[i].width),
              IconsInfo::GetIcons(extension.get()).Get(
                  web_app.icons[i].width, ExtensionIconSet::MATCH_EXACTLY));
    ExtensionResource resource =
        IconsInfo::GetIconResource(extension.get(),
                                   web_app.icons[i].width,
                                   ExtensionIconSet::MATCH_EXACTLY);
    ASSERT_TRUE(!resource.empty());
    EXPECT_TRUE(base::PathExists(resource.GetFilePath()));
  }
}

TEST(ExtensionFromWebApp, Minimal) {
  base::ScopedTempDir extensions_dir;
  ASSERT_TRUE(extensions_dir.CreateUniqueTempDir());

  WebApplicationInfo web_app;
  web_app.title = base::ASCIIToUTF16("Gearpad");
  web_app.app_url = GURL("http://aaronboodman.com/gearpad/");

  scoped_refptr<Extension> extension = ConvertWebAppToExtension(
      web_app, GetTestTime(1978, 12, 11, 0, 0, 0, 0), extensions_dir.GetPath());
  ASSERT_TRUE(extension.get());

  base::ScopedTempDir extension_dir;
  EXPECT_TRUE(extension_dir.Set(extension->path()));

  EXPECT_TRUE(extension->is_app());
  EXPECT_TRUE(extension->is_hosted_app());
  EXPECT_FALSE(extension->is_legacy_packaged_app());

  EXPECT_EQ("zVvdNZy3Mp7CFU8JVSyXNlDuHdVLbP7fDO3TGVzj/0w=",
            extension->public_key());
  EXPECT_EQ("oplhagaaipaimkjlbekcdjkffijdockj", extension->id());
  EXPECT_EQ("1978.12.11.0", extension->version().GetString());
  EXPECT_EQ(base::UTF16ToUTF8(web_app.title), extension->name());
  EXPECT_EQ("", extension->description());
  EXPECT_EQ(web_app.app_url, AppLaunchInfo::GetFullLaunchURL(extension.get()));
  EXPECT_TRUE(GetScopeURLFromBookmarkApp(extension.get()).is_empty());
  EXPECT_EQ(0u, IconsInfo::GetIcons(extension.get()).map().size());
  EXPECT_EQ(0u,
            extension->permissions_data()->active_permissions().apis().size());
  ASSERT_EQ(0u, extension->web_extent().patterns().size());
}

// Tests that a scope not ending in "/" works correctly.
// The tested behavior is unexpected but is working correctly according
// to the Web Manifest spec. https://github.com/w3c/manifest/issues/554
TEST(ExtensionFromWebApp, ScopeDoesNotEndInSlash) {
  base::ScopedTempDir extensions_dir;
  ASSERT_TRUE(extensions_dir.CreateUniqueTempDir());

  WebApplicationInfo web_app;
  web_app.title = base::ASCIIToUTF16("Gearpad");
  web_app.description =
      base::ASCIIToUTF16("The best text editor in the universe!");
  web_app.app_url = GURL("http://aaronboodman.com/gearpad/");
  web_app.scope = GURL("http://aaronboodman.com/gear");

  scoped_refptr<Extension> extension = ConvertWebAppToExtension(
      web_app, GetTestTime(1978, 12, 11, 0, 0, 0, 0), extensions_dir.GetPath());
  ASSERT_TRUE(extension.get());
  EXPECT_EQ(web_app.scope, GetScopeURLFromBookmarkApp(extension.get()));
}

}  // namespace extensions
