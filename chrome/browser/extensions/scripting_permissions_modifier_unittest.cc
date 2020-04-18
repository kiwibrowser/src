// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/strings/stringprintf.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/extensions/extension_service_test_base.h"
#include "chrome/browser/extensions/extension_util.h"
#include "chrome/browser/extensions/permissions_updater.h"
#include "chrome/browser/extensions/scripting_permissions_modifier.h"
#include "chrome/test/base/testing_profile.h"
#include "components/crx_file/id_util.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_builder.h"
#include "extensions/common/extension_features.h"
#include "extensions/common/permissions/permission_set.h"
#include "extensions/common/permissions/permissions_data.h"
#include "extensions/common/url_pattern.h"
#include "extensions/common/url_pattern_set.h"
#include "extensions/common/value_builder.h"

namespace extensions {

namespace {

scoped_refptr<const Extension> CreateExtensionWithPermissions(
    const std::set<URLPattern>& scriptable_hosts,
    const std::set<URLPattern>& explicit_hosts,
    Manifest::Location location,
    const std::string& name) {
  ListBuilder scriptable_host_list;
  for (std::set<URLPattern>::const_iterator pattern = scriptable_hosts.begin();
       pattern != scriptable_hosts.end(); ++pattern) {
    scriptable_host_list.Append(pattern->GetAsString());
  }

  ListBuilder explicit_host_list;
  for (std::set<URLPattern>::const_iterator pattern = explicit_hosts.begin();
       pattern != explicit_hosts.end(); ++pattern) {
    explicit_host_list.Append(pattern->GetAsString());
  }

  DictionaryBuilder script;
  script.Set("matches", scriptable_host_list.Build())
      .Set("js", ListBuilder().Append("foo.js").Build());

  return ExtensionBuilder()
      .SetLocation(location)
      .SetManifest(DictionaryBuilder()
                       .Set("name", name)
                       .Set("description", "foo")
                       .Set("manifest_version", 2)
                       .Set("version", "0.1.2.3")
                       .Set("content_scripts",
                            ListBuilder().Append(script.Build()).Build())
                       .Set("permissions", explicit_host_list.Build())
                       .Build())
      .SetID(crx_file::id_util::GenerateId(name))
      .Build();
}

testing::AssertionResult SetsAreEqual(const std::set<URLPattern>& set1,
                                      const std::set<URLPattern>& set2) {
  // Take the (set1 - set2) U (set2 - set1). This is then the set of all
  // elements which are in either set1 or set2, but not both.
  // If the sets are equal, this is none.
  std::set<URLPattern> difference = base::STLSetUnion<std::set<URLPattern>>(
      base::STLSetDifference<std::set<URLPattern>>(set1, set2),
      base::STLSetDifference<std::set<URLPattern>>(set2, set1));

  std::string error;
  for (std::set<URLPattern>::const_iterator iter = difference.begin();
       iter != difference.end(); ++iter) {
    if (iter->GetAsString() == "chrome://favicon/*")
      continue;  // Grr... This is auto-added for extensions with <all_urls>
    error = base::StringPrintf(
        "%s\n%s contains %s and the other does not.", error.c_str(),
        (set1.count(*iter) ? "Set1" : "Set2"), iter->GetAsString().c_str());
  }

  if (!error.empty())
    return testing::AssertionFailure() << error;
  return testing::AssertionSuccess();
}

class RuntimeHostPermissionsEnabledScope {
 public:
  RuntimeHostPermissionsEnabledScope() {
    feature_list_.InitAndEnableFeature(features::kRuntimeHostPermissions);
  }
  ~RuntimeHostPermissionsEnabledScope() {}

 private:
  base::test::ScopedFeatureList feature_list_;

  DISALLOW_COPY_AND_ASSIGN(RuntimeHostPermissionsEnabledScope);
};

using ScriptingPermissionsModifierUnitTest = ExtensionServiceTestBase;

}  // namespace

TEST_F(ScriptingPermissionsModifierUnitTest, WithholdAndGrantAllHosts) {
  InitializeEmptyExtensionService();

  // Permissions can only be withheld with the appropriate feature turned on.
  RuntimeHostPermissionsEnabledScope enabled_scope;

  URLPattern google(URLPattern::SCHEME_ALL, "http://www.google.com/*");
  URLPattern sub_google(URLPattern::SCHEME_ALL, "http://*.google.com/*");
  URLPattern all_http(URLPattern::SCHEME_ALL, "http://*/*");
  URLPattern all_hosts(URLPattern::SCHEME_ALL, "<all_urls>");
  URLPattern all_com(URLPattern::SCHEME_ALL, "http://*.com/*");

  std::set<URLPattern> all_host_patterns;
  std::set<URLPattern> safe_patterns;

  all_host_patterns.insert(all_http);
  all_host_patterns.insert(all_hosts);
  all_host_patterns.insert(all_com);

  safe_patterns.insert(google);
  safe_patterns.insert(sub_google);

  std::set<URLPattern> all_patterns =
      base::STLSetUnion<std::set<URLPattern>>(all_host_patterns, safe_patterns);

  scoped_refptr<const Extension> extension = CreateExtensionWithPermissions(
      all_patterns, all_patterns, Manifest::INTERNAL, "a");
  const PermissionsData* permissions_data = extension->permissions_data();
  PermissionsUpdater updater(profile());
  updater.InitializePermissions(extension.get());

  // By default, all permissions are granted.
  EXPECT_TRUE(SetsAreEqual(
      permissions_data->active_permissions().scriptable_hosts().patterns(),
      all_patterns));
  EXPECT_TRUE(SetsAreEqual(
      permissions_data->active_permissions().explicit_hosts().patterns(),
      all_patterns));
  EXPECT_TRUE(permissions_data->withheld_permissions()
                  .scriptable_hosts()
                  .patterns()
                  .empty());
  EXPECT_TRUE(permissions_data->withheld_permissions()
                  .explicit_hosts()
                  .patterns()
                  .empty());

  // Then, withhold the all-hosts permissions.
  ScriptingPermissionsModifier modifier(profile(), extension);
  modifier.SetAllowedOnAllUrls(false);

  EXPECT_TRUE(SetsAreEqual(
      permissions_data->active_permissions().scriptable_hosts().patterns(),
      safe_patterns));
  EXPECT_TRUE(SetsAreEqual(
      permissions_data->active_permissions().explicit_hosts().patterns(),
      safe_patterns));
  EXPECT_TRUE(SetsAreEqual(
      permissions_data->withheld_permissions().scriptable_hosts().patterns(),
      all_host_patterns));
  EXPECT_TRUE(SetsAreEqual(
      permissions_data->withheld_permissions().explicit_hosts().patterns(),
      all_host_patterns));

  // Finally, re-grant the withheld permissions.
  modifier.SetAllowedOnAllUrls(true);

  // We should be back to our initial state - all requested permissions are
  // granted.
  EXPECT_TRUE(SetsAreEqual(
      permissions_data->active_permissions().scriptable_hosts().patterns(),
      all_patterns));
  EXPECT_TRUE(SetsAreEqual(
      permissions_data->active_permissions().explicit_hosts().patterns(),
      all_patterns));
  EXPECT_TRUE(permissions_data->withheld_permissions()
                  .scriptable_hosts()
                  .patterns()
                  .empty());
  EXPECT_TRUE(permissions_data->withheld_permissions()
                  .explicit_hosts()
                  .patterns()
                  .empty());
}

TEST_F(ScriptingPermissionsModifierUnitTest, SwitchBehavior) {
  InitializeEmptyExtensionService();

  // Permissions can only be withheld with the appropriate feature turned on.
  auto enabled_scope = std::make_unique<RuntimeHostPermissionsEnabledScope>();

  URLPattern all_hosts(URLPattern::SCHEME_ALL, "<all_urls>");
  std::set<URLPattern> all_host_patterns;
  all_host_patterns.insert(all_hosts);

  scoped_refptr<const Extension> extension = CreateExtensionWithPermissions(
      all_host_patterns, all_host_patterns, Manifest::INTERNAL, "a");
  PermissionsUpdater updater(profile());
  updater.InitializePermissions(extension.get());
  const PermissionsData* permissions_data = extension->permissions_data();

  // By default, the extension should have all its permissions.
  EXPECT_TRUE(SetsAreEqual(
      permissions_data->active_permissions().scriptable_hosts().patterns(),
      all_host_patterns));
  EXPECT_TRUE(
      permissions_data->withheld_permissions().scriptable_hosts().is_empty());
  ScriptingPermissionsModifier modifier(profile(), extension);
  EXPECT_TRUE(modifier.IsAllowedOnAllUrls());

  // Revoke access.
  modifier.SetAllowedOnAllUrls(false);
  EXPECT_FALSE(modifier.IsAllowedOnAllUrls());
  EXPECT_TRUE(
      permissions_data->active_permissions().scriptable_hosts().is_empty());
  EXPECT_TRUE(SetsAreEqual(
      permissions_data->withheld_permissions().scriptable_hosts().patterns(),
      all_host_patterns));

  // Remove the switch. The extension should have permission again.
  enabled_scope.reset();
  updater.InitializePermissions(extension.get());
  EXPECT_FALSE(modifier.CanAffectExtension());
  EXPECT_TRUE(SetsAreEqual(
      permissions_data->active_permissions().scriptable_hosts().patterns(),
      all_host_patterns));
  EXPECT_TRUE(
      permissions_data->withheld_permissions().scriptable_hosts().is_empty());

  // Reapply the switch; the extension should go back to having permissions
  // withheld.
  enabled_scope = std::make_unique<RuntimeHostPermissionsEnabledScope>();
  updater.InitializePermissions(extension.get());
  EXPECT_FALSE(modifier.IsAllowedOnAllUrls());
  EXPECT_TRUE(
      permissions_data->active_permissions().scriptable_hosts().is_empty());
  EXPECT_TRUE(SetsAreEqual(
      permissions_data->withheld_permissions().scriptable_hosts().patterns(),
      all_host_patterns));
}

TEST_F(ScriptingPermissionsModifierUnitTest, GrantHostPermission) {
  InitializeEmptyExtensionService();

  // Permissions can only be withheld with the appropriate feature turned on.
  RuntimeHostPermissionsEnabledScope enabled_scope;

  URLPattern all_hosts(URLPattern::SCHEME_ALL, "<all_urls>");
  std::set<URLPattern> all_host_patterns;
  all_host_patterns.insert(all_hosts);

  scoped_refptr<const Extension> extension = CreateExtensionWithPermissions(
      all_host_patterns, all_host_patterns, Manifest::INTERNAL, "extension");
  PermissionsUpdater(profile()).InitializePermissions(extension.get());

  ScriptingPermissionsModifier modifier(profile(), extension);
  modifier.SetAllowedOnAllUrls(false);

  const GURL kUrl("https://www.google.com/");
  const GURL kUrl2("https://www.chromium.org/");
  EXPECT_FALSE(modifier.HasGrantedHostPermission(kUrl));
  EXPECT_FALSE(modifier.HasGrantedHostPermission(kUrl2));

  const PermissionsData* permissions = extension->permissions_data();
  auto get_page_access = [&permissions](const GURL& url) {
    return permissions->GetPageAccess(url, 0, nullptr);
  };

  EXPECT_EQ(PermissionsData::PageAccess::kWithheld, get_page_access(kUrl));
  EXPECT_EQ(PermissionsData::PageAccess::kWithheld, get_page_access(kUrl2));

  modifier.GrantHostPermission(kUrl);
  EXPECT_TRUE(modifier.HasGrantedHostPermission(kUrl));
  EXPECT_FALSE(modifier.HasGrantedHostPermission(kUrl2));
  EXPECT_EQ(PermissionsData::PageAccess::kAllowed, get_page_access(kUrl));
  EXPECT_EQ(PermissionsData::PageAccess::kWithheld, get_page_access(kUrl2));

  modifier.RemoveGrantedHostPermission(kUrl);
  EXPECT_FALSE(modifier.HasGrantedHostPermission(kUrl));
  EXPECT_FALSE(modifier.HasGrantedHostPermission(kUrl2));
  EXPECT_EQ(PermissionsData::PageAccess::kWithheld, get_page_access(kUrl));
  EXPECT_EQ(PermissionsData::PageAccess::kWithheld, get_page_access(kUrl2));
}

TEST_F(ScriptingPermissionsModifierUnitTest, CanAffectExtensionByLocation) {
  auto enabled_scope = std::make_unique<RuntimeHostPermissionsEnabledScope>();

  InitializeEmptyExtensionService();

  struct {
    Manifest::Location location;
    bool can_be_affected;
  } test_cases[] = {
      {Manifest::INTERNAL, true},   {Manifest::EXTERNAL_PREF, true},
      {Manifest::UNPACKED, true},   {Manifest::EXTERNAL_POLICY_DOWNLOAD, false},
      {Manifest::COMPONENT, false},
  };

  for (const auto& test_case : test_cases) {
    scoped_refptr<const Extension> extension =
        ExtensionBuilder("test")
            .SetLocation(test_case.location)
            .AddPermission("<all_urls>")
            .Build();
    EXPECT_EQ(test_case.can_be_affected,
              ScriptingPermissionsModifier(profile(), extension.get())
                  .CanAffectExtension())
        << test_case.location;
  }

  enabled_scope.reset();

  // With the feature disabled, no extension should be able to be affected.
  for (const auto& test_case : test_cases) {
    scoped_refptr<const Extension> extension =
        ExtensionBuilder("test")
            .SetLocation(test_case.location)
            .AddPermission("<all_urls>")
            .Build();
    EXPECT_FALSE(ScriptingPermissionsModifier(profile(), extension.get())
                     .CanAffectExtension())
        << test_case.location;
  }
}

}  // namespace extensions
