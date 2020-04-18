// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/permissions/permissions_api_helpers.h"

#include <stddef.h>

#include <memory>
#include <utility>

#include "base/macros.h"
#include "base/values.h"
#include "chrome/common/extensions/api/permissions.h"
#include "extensions/common/permissions/permission_set.h"
#include "extensions/common/url_pattern_set.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using extensions::api::permissions::Permissions;
using extensions::permissions_api_helpers::PackPermissionSet;
using extensions::permissions_api_helpers::UnpackPermissionSet;

namespace extensions {

namespace {

static void AddPattern(URLPatternSet* extent, const std::string& pattern) {
  int schemes = URLPattern::SCHEME_ALL;
  extent->AddPattern(URLPattern(schemes, pattern));
}

}  // namespace

// Tests that we can convert PermissionSets to and from values.
TEST(ExtensionPermissionsAPIHelpers, Pack) {
  APIPermissionSet apis;
  apis.insert(APIPermission::kTab);
  apis.insert(APIPermission::kFileBrowserHandler);
  // Note: kFileBrowserHandler implies kFileBrowserHandlerInternal.
  URLPatternSet hosts;
  AddPattern(&hosts, "http://a.com/*");
  AddPattern(&hosts, "http://b.com/*");

  PermissionSet permission_set(apis, ManifestPermissionSet(), hosts,
                               URLPatternSet());

  // Pack the permission set to value and verify its contents.
  std::unique_ptr<Permissions> permissions(PackPermissionSet(permission_set));
  std::unique_ptr<base::DictionaryValue> value(permissions->ToValue());
  base::ListValue* api_list = NULL;
  base::ListValue* origin_list = NULL;
  EXPECT_TRUE(value->GetList("permissions", &api_list));
  EXPECT_TRUE(value->GetList("origins", &origin_list));

  EXPECT_EQ(3u, api_list->GetSize());
  EXPECT_EQ(2u, origin_list->GetSize());

  std::string expected_apis[] = {"tabs", "fileBrowserHandler",
                                 "fileBrowserHandlerInternal"};
  for (size_t i = 0; i < arraysize(expected_apis); ++i) {
    std::unique_ptr<base::Value> value(new base::Value(expected_apis[i]));
    EXPECT_NE(api_list->end(), api_list->Find(*value));
  }

  std::string expected_origins[] = { "http://a.com/*", "http://b.com/*" };
  for (size_t i = 0; i < arraysize(expected_origins); ++i) {
    std::unique_ptr<base::Value> value(new base::Value(expected_origins[i]));
    EXPECT_NE(origin_list->end(), origin_list->Find(*value));
  }

  // Unpack the value back to a permission set and make sure its equal to the
  // original one.
  std::string error;
  Permissions permissions_object;
  EXPECT_TRUE(Permissions::Populate(*value, &permissions_object));
  std::unique_ptr<const PermissionSet> from_value =
      UnpackPermissionSet(permissions_object, true, &error);
  EXPECT_TRUE(error.empty());

  EXPECT_EQ(permission_set, *from_value);
}

// Tests various error conditions and edge cases when unpacking values
// into PermissionSets.
TEST(ExtensionPermissionsAPIHelpers, Unpack) {
  std::unique_ptr<base::ListValue> apis(new base::ListValue());
  apis->AppendString("tabs");
  std::unique_ptr<base::ListValue> origins(new base::ListValue());
  origins->AppendString("http://a.com/*");

  std::unique_ptr<base::DictionaryValue> value(new base::DictionaryValue());
  std::unique_ptr<const PermissionSet> permissions;
  std::string error;

  // Origins shouldn't have to be present.
  {
    Permissions permissions_object;
    value->Set("permissions", apis->CreateDeepCopy());
    EXPECT_TRUE(Permissions::Populate(*value, &permissions_object));
    permissions = UnpackPermissionSet(permissions_object, true, &error);
    EXPECT_TRUE(permissions->HasAPIPermission(APIPermission::kTab));
    EXPECT_TRUE(permissions.get());
    EXPECT_TRUE(error.empty());
  }

  // The api permissions don't need to be present either.
  {
    Permissions permissions_object;
    value->Clear();
    value->Set("origins", origins->CreateDeepCopy());
    EXPECT_TRUE(Permissions::Populate(*value, &permissions_object));
    permissions = UnpackPermissionSet(permissions_object, true, &error);
    EXPECT_TRUE(permissions.get());
    EXPECT_TRUE(error.empty());
    EXPECT_TRUE(permissions->HasExplicitAccessToOrigin(GURL("http://a.com/")));
  }

  // Throw errors for non-string API permissions.
  {
    Permissions permissions_object;
    value->Clear();
    std::unique_ptr<base::ListValue> invalid_apis = apis->CreateDeepCopy();
    invalid_apis->AppendInteger(3);
    value->Set("permissions", std::move(invalid_apis));
    EXPECT_FALSE(Permissions::Populate(*value, &permissions_object));
  }

  // Throw errors for non-string origins.
  {
    Permissions permissions_object;
    value->Clear();
    std::unique_ptr<base::ListValue> invalid_origins =
        origins->CreateDeepCopy();
    invalid_origins->AppendInteger(3);
    value->Set("origins", std::move(invalid_origins));
    EXPECT_FALSE(Permissions::Populate(*value, &permissions_object));
  }

  // Throw errors when "origins" or "permissions" are not list values.
  {
    Permissions permissions_object;
    value->Clear();
    value->Set("origins", std::make_unique<base::Value>(2));
    EXPECT_FALSE(Permissions::Populate(*value, &permissions_object));
  }

  {
    Permissions permissions_object;
    value->Clear();
    value->Set("permissions", std::make_unique<base::Value>(2));
    EXPECT_FALSE(Permissions::Populate(*value, &permissions_object));
  }

  // Additional fields should be allowed.
  {
    Permissions permissions_object;
    value->Clear();
    value->Set("origins", origins->CreateDeepCopy());
    value->Set("random", std::make_unique<base::Value>(3));
    EXPECT_TRUE(Permissions::Populate(*value, &permissions_object));
    permissions = UnpackPermissionSet(permissions_object, true, &error);
    EXPECT_TRUE(permissions.get());
    EXPECT_TRUE(error.empty());
    EXPECT_TRUE(permissions->HasExplicitAccessToOrigin(GURL("http://a.com/")));
  }

  // Unknown permissions should throw an error.
  {
    Permissions permissions_object;
    value->Clear();
    std::unique_ptr<base::ListValue> invalid_apis = apis->CreateDeepCopy();
    invalid_apis->AppendString("unknown_permission");
    value->Set("permissions", std::move(invalid_apis));
    EXPECT_TRUE(Permissions::Populate(*value, &permissions_object));
    permissions = UnpackPermissionSet(permissions_object, true, &error);
    EXPECT_FALSE(permissions.get());
    EXPECT_FALSE(error.empty());
    EXPECT_EQ(error, "'unknown_permission' is not a recognized permission.");
  }
}

}  // namespace extensions
