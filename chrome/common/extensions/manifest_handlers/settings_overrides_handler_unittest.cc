// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/extensions/manifest_handlers/settings_overrides_handler.h"

#include <memory>

#include "base/json/json_string_value_serializer.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "components/version_info/version_info.h"
#include "extensions/common/error_utils.h"
#include "extensions/common/extension.h"
#include "extensions/common/features/feature_channel.h"
#include "extensions/common/manifest_constants.h"
#include "extensions/common/manifest_url_handlers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const char kManifest[] =
    "{"
    " \"version\" : \"1.0.0.0\","
    " \"manifest_version\" : 2,"
    " \"name\" : \"Test\","
    " \"chrome_settings_overrides\" : {"
    "   \"homepage\" : \"http://www.homepage.com\","
    "   \"search_provider\" : {"
    "        \"name\" : \"first\","
    "        \"keyword\" : \"firstkey\","
    "        \"search_url\" : \"http://www.foo.com/s?q={searchTerms}\","
    "        \"favicon_url\" : \"http://www.foo.com/favicon.ico\","
    "        \"suggest_url\" : \"http://www.foo.com/s?q={searchTerms}\","
    "        \"encoding\" : \"UTF-8\","
    "        \"is_default\" : true"
    "    },"
    "   \"startup_pages\" : [\"http://www.startup.com\"]"
    "  }"
    "}";

const char kPrepopulatedManifest[] =
    "{"
    " \"version\" : \"1.0.0.0\","
    " \"manifest_version\" : 2,"
    " \"name\" : \"Test\","
    " \"chrome_settings_overrides\" : {"
    "   \"search_provider\" : {"
    "        \"search_url\" : \"http://www.foo.com/s?q={searchTerms}\","
    "        \"prepopulated_id\" : 3,"
    "        \"is_default\" : true"
    "    }"
    "  }"
    "}";

const char kBrokenManifest[] =
    "{"
    " \"version\" : \"1.0.0.0\","
    " \"manifest_version\" : 2,"
    " \"name\" : \"Test\","
    " \"chrome_settings_overrides\" : {"
    "   \"homepage\" : \"{invalid}\","
    "   \"search_provider\" : {"
    "        \"name\" : \"first\","
    "        \"keyword\" : \"firstkey\","
    "        \"search_url\" : \"{invalid}/s?q={searchTerms}\","
    "        \"favicon_url\" : \"{invalid}/favicon.ico\","
    "        \"encoding\" : \"UTF-8\","
    "        \"is_default\" : true"
    "    },"
    "   \"startup_pages\" : [\"{invalid}\"]"
    "  }"
    "}";

using extensions::api::manifest_types::ChromeSettingsOverrides;
using extensions::Extension;
using extensions::Manifest;
using extensions::SettingsOverrides;
namespace manifest_keys = extensions::manifest_keys;

TEST(OverrideSettingsTest, ParseManifest) {
#if defined(OS_MACOSX)
  // On Mac, this API is limited to trunk.
  extensions::ScopedCurrentChannel scoped_channel(
      version_info::Channel::UNKNOWN);
#endif  // OS_MACOSX

  std::string manifest(kManifest);
  JSONStringValueDeserializer json(manifest);
  std::string error;
  std::unique_ptr<base::Value> root(json.Deserialize(NULL, &error));
  ASSERT_TRUE(root);
  ASSERT_TRUE(root->is_dict());
  scoped_refptr<Extension> extension = Extension::Create(
      base::FilePath(FILE_PATH_LITERAL("//nonexistent")),
      Manifest::INVALID_LOCATION,
      *static_cast<base::DictionaryValue*>(root.get()),
      Extension::NO_FLAGS,
      &error);
  ASSERT_TRUE(extension.get());
#if defined(OS_WIN) || defined(OS_MACOSX)
  ASSERT_TRUE(extension->manifest()->HasPath(manifest_keys::kSettingsOverride));

  SettingsOverrides* settings_override = static_cast<SettingsOverrides*>(
      extension->GetManifestData(manifest_keys::kSettingsOverride));
  ASSERT_TRUE(settings_override);
  ASSERT_TRUE(settings_override->search_engine);
  EXPECT_TRUE(settings_override->search_engine->is_default);
  const ChromeSettingsOverrides::Search_provider* search_engine =
      settings_override->search_engine.get();
  EXPECT_EQ("first", *search_engine->name);
  EXPECT_EQ("firstkey", *search_engine->keyword);
  EXPECT_EQ("http://www.foo.com/s?q={searchTerms}", search_engine->search_url);
  EXPECT_EQ("http://www.foo.com/favicon.ico", *search_engine->favicon_url);
  EXPECT_EQ("http://www.foo.com/s?q={searchTerms}",
            *search_engine->suggest_url);
  EXPECT_EQ("UTF-8", *search_engine->encoding);

  EXPECT_EQ(std::vector<GURL>(1, GURL("http://www.startup.com")),
            settings_override->startup_pages);

  ASSERT_TRUE(settings_override->homepage);
  EXPECT_EQ(GURL("http://www.homepage.com"), *settings_override->homepage);
#else
  EXPECT_FALSE(
      extension->manifest()->HasPath(manifest_keys::kSettingsOverride));
#endif
}

TEST(OverrideSettingsTest, ParsePrepopulatedId) {
#if defined(OS_MACOSX)
  // On Mac, this API is limited to trunk.
  extensions::ScopedCurrentChannel scoped_channel(
      version_info::Channel::UNKNOWN);
#endif  // OS_MACOSX

  std::string manifest(kPrepopulatedManifest);
  JSONStringValueDeserializer json(manifest);
  std::string error;
  std::unique_ptr<base::Value> root(json.Deserialize(NULL, &error));
  ASSERT_TRUE(root);
  ASSERT_TRUE(root->is_dict());
  scoped_refptr<Extension> extension =
      Extension::Create(base::FilePath(FILE_PATH_LITERAL("//nonexistent")),
                        Manifest::INVALID_LOCATION,
                        *static_cast<base::DictionaryValue*>(root.get()),
                        Extension::NO_FLAGS,
                        &error);
  ASSERT_TRUE(extension.get());
#if defined(OS_WIN) || defined(OS_MACOSX)
  ASSERT_TRUE(extension->manifest()->HasPath(manifest_keys::kSettingsOverride));

  SettingsOverrides* settings_override = static_cast<SettingsOverrides*>(
      extension->GetManifestData(manifest_keys::kSettingsOverride));
  ASSERT_TRUE(settings_override);
  ASSERT_TRUE(settings_override->search_engine);
  EXPECT_TRUE(settings_override->search_engine->is_default);
  const ChromeSettingsOverrides::Search_provider* search_engine =
      settings_override->search_engine.get();
  ASSERT_TRUE(search_engine->prepopulated_id);
  EXPECT_EQ("http://www.foo.com/s?q={searchTerms}", search_engine->search_url);
  EXPECT_EQ(3, *search_engine->prepopulated_id);
#else
  EXPECT_FALSE(
      extension->manifest()->HasPath(manifest_keys::kSettingsOverride));
#endif
}

TEST(OverrideSettingsTest, ParseBrokenManifest) {
#if defined(OS_MACOSX)
  // On Mac, this API is limited to trunk.
  extensions::ScopedCurrentChannel scoped_channel(
      version_info::Channel::UNKNOWN);
#endif  // OS_MACOSX

  std::string manifest(kBrokenManifest);
  JSONStringValueDeserializer json(manifest);
  std::string error;
  std::unique_ptr<base::Value> root(json.Deserialize(NULL, &error));
  ASSERT_TRUE(root);
  ASSERT_TRUE(root->is_dict());
  scoped_refptr<Extension> extension = Extension::Create(
      base::FilePath(FILE_PATH_LITERAL("//nonexistent")),
      Manifest::INVALID_LOCATION,
      *static_cast<base::DictionaryValue*>(root.get()),
      Extension::NO_FLAGS,
      &error);
#if defined(OS_WIN) || defined(OS_MACOSX)
  EXPECT_FALSE(extension.get());
  EXPECT_EQ(
      extensions::ErrorUtils::FormatErrorMessage(
          extensions::manifest_errors::kInvalidEmptyDictionary,
          extensions::manifest_keys::kSettingsOverride),
      error);
#else
  EXPECT_TRUE(extension.get());
  EXPECT_FALSE(
      extension->manifest()->HasPath(manifest_keys::kSettingsOverride));
#endif
}

}  // namespace
