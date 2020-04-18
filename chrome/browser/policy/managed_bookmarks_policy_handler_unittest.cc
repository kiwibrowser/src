// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/policy/managed_bookmarks_policy_handler.h"

#include <memory>
#include <utility>

#include "base/json/json_reader.h"
#include "base/memory/ptr_util.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "components/policy/core/browser/configuration_policy_pref_store.h"
#include "components/policy/core/browser/configuration_policy_pref_store_test.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_types.h"
#include "components/policy/core/common/schema.h"
#include "components/policy/policy_constants.h"
#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "extensions/common/value_builder.h"
#endif

namespace policy {

class ManagedBookmarksPolicyHandlerTest
    : public ConfigurationPolicyPrefStoreTest {
  void SetUp() override {
    Schema chrome_schema = Schema::Wrap(GetChromeSchemaData());
    handler_list_.AddHandler(base::WrapUnique<ConfigurationPolicyHandler>(
        new ManagedBookmarksPolicyHandler(chrome_schema)));
  }
};

#if BUILDFLAG(ENABLE_EXTENSIONS)
TEST_F(ManagedBookmarksPolicyHandlerTest, ApplyPolicySettings) {
  EXPECT_FALSE(store_->GetValue(bookmarks::prefs::kManagedBookmarks, NULL));

  PolicyMap policy;
  policy.Set(key::kManagedBookmarks, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
             POLICY_SOURCE_CLOUD,
             base::JSONReader::Read("["
                                    // The following gets filtered out from the
                                    // JSON string when parsed.
                                    "  {"
                                    "    \"toplevel_name\": \"abc 123\""
                                    "  },"
                                    "  {"
                                    "    \"name\": \"Google\","
                                    "    \"url\": \"google.com\""
                                    "  },"
                                    "  {"
                                    "    \"name\": \"Empty Folder\","
                                    "    \"children\": []"
                                    "  },"
                                    "  {"
                                    "    \"name\": \"Big Folder\","
                                    "    \"children\": ["
                                    "      {"
                                    "        \"name\": \"Youtube\","
                                    "        \"url\": \"youtube.com\""
                                    "      },"
                                    "      {"
                                    "        \"name\": \"Chromium\","
                                    "        \"url\": \"chromium.org\""
                                    "      },"
                                    "      {"
                                    "        \"name\": \"More Stuff\","
                                    "        \"children\": ["
                                    "          {"
                                    "            \"name\": \"Bugs\","
                                    "            \"url\": \"crbug.com\""
                                    "          }"
                                    "        ]"
                                    "      }"
                                    "    ]"
                                    "  }"
                                    "]"),
             nullptr);
  UpdateProviderPolicy(policy);
  const base::Value* pref_value = NULL;
  EXPECT_TRUE(
      store_->GetValue(bookmarks::prefs::kManagedBookmarks, &pref_value));
  ASSERT_TRUE(pref_value);

  // Make sure the kManagedBookmarksFolderName pref is set correctly.
  const base::Value* folder_value = NULL;
  std::string folder_name;
  EXPECT_TRUE(store_->GetValue(bookmarks::prefs::kManagedBookmarksFolderName,
                               &folder_value));
  ASSERT_TRUE(folder_value);
  ASSERT_TRUE(folder_value->GetAsString(&folder_name));
  EXPECT_EQ("abc 123", folder_name);

  std::unique_ptr<base::Value> expected(
      extensions::ListBuilder()
          .Append(extensions::DictionaryBuilder()
                      .Set("name", "Google")
                      .Set("url", "http://google.com/")
                      .Build())
          .Append(extensions::DictionaryBuilder()
                      .Set("name", "Empty Folder")
                      .Set("children", extensions::ListBuilder().Build())
                      .Build())
          .Append(
              extensions::DictionaryBuilder()
                  .Set("name", "Big Folder")
                  .Set("children",
                       extensions::ListBuilder()
                           .Append(extensions::DictionaryBuilder()
                                       .Set("name", "Youtube")
                                       .Set("url", "http://youtube.com/")
                                       .Build())
                           .Append(extensions::DictionaryBuilder()
                                       .Set("name", "Chromium")
                                       .Set("url", "http://chromium.org/")
                                       .Build())
                           .Append(
                               extensions::DictionaryBuilder()
                                   .Set("name", "More Stuff")
                                   .Set("children",
                                        extensions::ListBuilder()
                                            .Append(
                                                extensions::DictionaryBuilder()
                                                    .Set("name", "Bugs")
                                                    .Set("url",
                                                         "http://"
                                                         "crbug."
                                                         "com"
                                                         "/")
                                                    .Build())
                                            .Build())
                                   .Build())
                           .Build())
                  .Build())
          .Build());
  EXPECT_TRUE(pref_value->Equals(expected.get()));
}
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

#if BUILDFLAG(ENABLE_EXTENSIONS)
TEST_F(ManagedBookmarksPolicyHandlerTest, ApplyPolicySettingsNoTitle) {
  EXPECT_FALSE(store_->GetValue(bookmarks::prefs::kManagedBookmarks, NULL));

  PolicyMap policy;
  policy.Set(key::kManagedBookmarks, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
             POLICY_SOURCE_CLOUD,
             base::JSONReader::Read("["
                                    "  {"
                                    "    \"name\": \"Google\","
                                    "    \"url\": \"google.com\""
                                    "  }"
                                    "]"),
             nullptr);
  UpdateProviderPolicy(policy);
  const base::Value* pref_value = NULL;
  EXPECT_TRUE(
      store_->GetValue(bookmarks::prefs::kManagedBookmarks, &pref_value));
  ASSERT_TRUE(pref_value);

  // Make sure the kManagedBookmarksFolderName pref is set correctly.
  const base::Value* folder_value = NULL;
  std::string folder_name;
  EXPECT_TRUE(store_->GetValue(bookmarks::prefs::kManagedBookmarksFolderName,
                               &folder_value));
  ASSERT_TRUE(folder_value);
  ASSERT_TRUE(folder_value->GetAsString(&folder_name));
  EXPECT_EQ("", folder_name);

  std::unique_ptr<base::Value> expected(
      extensions::ListBuilder()
          .Append(extensions::DictionaryBuilder()
                      .Set("name", "Google")
                      .Set("url", "http://google.com/")
                      .Build())
          .Build());
  EXPECT_TRUE(pref_value->Equals(expected.get()));
}
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

TEST_F(ManagedBookmarksPolicyHandlerTest, WrongPolicyType) {
  PolicyMap policy;
  // The expected type is base::ListValue, but this policy sets it as an
  // unparsed base::Value. Any type other than ListValue should fail.
  policy.Set(key::kManagedBookmarks, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
             POLICY_SOURCE_CLOUD,
             std::make_unique<base::Value>("["
                                           "  {"
                                           "    \"name\": \"Google\","
                                           "    \"url\": \"google.com\""
                                           "  },"
                                           "]"),
             nullptr);
  UpdateProviderPolicy(policy);
  EXPECT_FALSE(store_->GetValue(bookmarks::prefs::kManagedBookmarks, NULL));
}

#if BUILDFLAG(ENABLE_EXTENSIONS)
TEST_F(ManagedBookmarksPolicyHandlerTest, UnknownKeys) {
  PolicyMap policy;
  policy.Set(key::kManagedBookmarks, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
             POLICY_SOURCE_CLOUD,
             base::JSONReader::Read("["
                                    "  {"
                                    "    \"name\": \"Google\","
                                    "    \"unknown\": \"should be ignored\","
                                    "    \"url\": \"google.com\""
                                    "  }"
                                    "]"),
             nullptr);
  UpdateProviderPolicy(policy);
  const base::Value* pref_value = NULL;
  EXPECT_TRUE(
      store_->GetValue(bookmarks::prefs::kManagedBookmarks, &pref_value));
  ASSERT_TRUE(pref_value);

  std::unique_ptr<base::Value> expected(
      extensions::ListBuilder()
          .Append(extensions::DictionaryBuilder()
                      .Set("name", "Google")
                      .Set("url", "http://google.com/")
                      .Build())
          .Build());
  EXPECT_TRUE(pref_value->Equals(expected.get()));
}
#endif

#if BUILDFLAG(ENABLE_EXTENSIONS)
TEST_F(ManagedBookmarksPolicyHandlerTest, BadBookmark) {
  PolicyMap policy;
  policy.Set(key::kManagedBookmarks, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
             POLICY_SOURCE_CLOUD,
             base::JSONReader::Read("["
                                    "  {"
                                    "    \"name\": \"Empty\","
                                    "    \"url\": \"\""
                                    "  },"
                                    "  {"
                                    "    \"name\": \"Invalid type\","
                                    "    \"url\": 4"
                                    "  },"
                                    "  {"
                                    "    \"name\": \"Invalid URL\","
                                    "    \"url\": \"?\""
                                    "  },"
                                    "  {"
                                    "    \"name\": \"Google\","
                                    "    \"url\": \"google.com\""
                                    "  }"
                                    "]"),
             nullptr);
  UpdateProviderPolicy(policy);
  const base::Value* pref_value = NULL;
  EXPECT_TRUE(
      store_->GetValue(bookmarks::prefs::kManagedBookmarks, &pref_value));
  ASSERT_TRUE(pref_value);

  std::unique_ptr<base::Value> expected(
      extensions::ListBuilder()
          .Append(extensions::DictionaryBuilder()
                      .Set("name", "Google")
                      .Set("url", "http://google.com/")
                      .Build())
          .Build());
  EXPECT_TRUE(pref_value->Equals(expected.get()));
}
#endif

}  // namespace policy
