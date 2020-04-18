// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/permissions_updater.h"

#include <utility>

#include "base/files/file_path.h"
#include "base/json/json_file_value_serializer.h"
#include "base/memory/ref_counted.h"
#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "base/values.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_service_test_base.h"
#include "chrome/browser/extensions/extension_util.h"
#include "chrome/browser/extensions/scripting_permissions_modifier.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/extensions/extension_test_util.h"
#include "chrome/test/base/testing_profile.h"
#include "components/crx_file/id_util.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/notification_service.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_builder.h"
#include "extensions/common/extension_features.h"
#include "extensions/common/permissions/permission_set.h"
#include "extensions/common/permissions/permissions_data.h"
#include "extensions/common/value_builder.h"
#include "testing/gtest/include/gtest/gtest.h"

using extension_test_util::LoadManifest;

namespace extensions {

namespace {

scoped_refptr<const Extension> CreateExtensionWithOptionalPermissions(
    std::unique_ptr<base::Value> optional_permissions,
    std::unique_ptr<base::Value> permissions,
    const std::string& name) {
  return ExtensionBuilder()
      .SetLocation(Manifest::INTERNAL)
      .SetManifest(
          DictionaryBuilder()
              .Set("name", name)
              .Set("description", "foo")
              .Set("manifest_version", 2)
              .Set("version", "0.1.2.3")
              .Set("permissions", std::move(permissions))
              .Set("optional_permissions", std::move(optional_permissions))
              .Build())
      .SetID(crx_file::id_util::GenerateId(name))
      .Build();
}

// A helper class that listens for NOTIFICATION_EXTENSION_PERMISSIONS_UPDATED.
class PermissionsUpdaterListener : public content::NotificationObserver {
 public:
  PermissionsUpdaterListener()
      : received_notification_(false), waiting_(false) {
    registrar_.Add(this,
                   extensions::NOTIFICATION_EXTENSION_PERMISSIONS_UPDATED,
                   content::NotificationService::AllSources());
  }

  void Reset() {
    received_notification_ = false;
    waiting_ = false;
    extension_ = NULL;
    permissions_ = NULL;
  }

  void Wait() {
    if (received_notification_)
      return;

    waiting_ = true;
    base::RunLoop run_loop;
    run_loop.Run();
  }

  bool received_notification() const { return received_notification_; }
  const Extension* extension() const { return extension_.get(); }
  const PermissionSet* permissions() const { return permissions_.get(); }
  UpdatedExtensionPermissionsInfo::Reason reason() const { return reason_; }

 private:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override {
    received_notification_ = true;
    UpdatedExtensionPermissionsInfo* info =
        content::Details<UpdatedExtensionPermissionsInfo>(details).ptr();

    extension_ = info->extension;
    permissions_ = info->permissions.Clone();
    reason_ = info->reason;

    if (waiting_) {
      waiting_ = false;
      base::RunLoop::QuitCurrentWhenIdleDeprecated();
    }
  }

  bool received_notification_;
  bool waiting_;
  content::NotificationRegistrar registrar_;
  scoped_refptr<const Extension> extension_;
  std::unique_ptr<const PermissionSet> permissions_;
  UpdatedExtensionPermissionsInfo::Reason reason_;
};

class PermissionsUpdaterTest : public ExtensionServiceTestBase {
};

scoped_refptr<Extension> LoadOurManifest() {
  base::FilePath path;
  path = path.AppendASCII("api_test")
      .AppendASCII("permissions")
      .AppendASCII("optional");
  return LoadManifest(path.AsUTF8Unsafe(),
                      "manifest.json",
                      Manifest::INTERNAL,
                      Extension::NO_FLAGS);
}

void AddPattern(URLPatternSet* extent, const std::string& pattern) {
  int schemes = URLPattern::SCHEME_ALL;
  extent->AddPattern(URLPattern(schemes, pattern));
}

class PermissionsUpdaterTestDelegate : public PermissionsUpdater::Delegate {
 public:
  PermissionsUpdaterTestDelegate() {}
  ~PermissionsUpdaterTestDelegate() override {}

  // PermissionsUpdater::Delegate
  void InitializePermissions(
      const Extension* extension,
      std::unique_ptr<const PermissionSet>* granted_permissions) override {
    // Remove the cookie permission.
    APIPermissionSet api_permission_set((*granted_permissions)->apis());
    api_permission_set.erase(APIPermission::kCookie);
    granted_permissions->reset(
        new PermissionSet(api_permission_set, ManifestPermissionSet(),
                          URLPatternSet(), URLPatternSet()));
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(PermissionsUpdaterTestDelegate);
};

}  // namespace

// Test that the PermissionUpdater can correctly add and remove active
// permissions. This tests all of PermissionsUpdater's public methods because
// GrantActivePermissions and SetPermissions are used by AddPermissions.
TEST_F(PermissionsUpdaterTest, AddAndRemovePermissions) {
  InitializeEmptyExtensionService();

  // Load the test extension.
  scoped_refptr<Extension> extension = LoadOurManifest();
  ASSERT_TRUE(extension.get());

  APIPermissionSet default_apis;
  default_apis.insert(APIPermission::kManagement);
  ManifestPermissionSet empty_manifest_permissions;

  URLPatternSet default_hosts;
  AddPattern(&default_hosts, "http://a.com/*");
  PermissionSet default_permissions(default_apis, empty_manifest_permissions,
                                    default_hosts, URLPatternSet());

  // Make sure it loaded properly.
  ASSERT_EQ(default_permissions,
            extension->permissions_data()->active_permissions());

  ExtensionPrefs* prefs = ExtensionPrefs::Get(profile_.get());
  std::unique_ptr<const PermissionSet> active_permissions;
  std::unique_ptr<const PermissionSet> granted_permissions;

  // Add a few permissions.
  APIPermissionSet apis;
  apis.insert(APIPermission::kNotifications);
  URLPatternSet hosts;
  AddPattern(&hosts, "http://*.c.com/*");
  URLPatternSet scriptable_hosts;
  AddPattern(&scriptable_hosts, "http://*.example.com/*");

  {
    PermissionSet delta(apis, empty_manifest_permissions, hosts,
                        scriptable_hosts);

    PermissionsUpdaterListener listener;
    PermissionsUpdater(profile_.get()).AddPermissions(extension.get(), delta);

    listener.Wait();

    // Verify that the permission notification was sent correctly.
    ASSERT_TRUE(listener.received_notification());
    ASSERT_EQ(extension.get(), listener.extension());
    ASSERT_EQ(UpdatedExtensionPermissionsInfo::ADDED, listener.reason());
    ASSERT_EQ(delta, *listener.permissions());

    // Make sure the extension's active permissions reflect the change.
    active_permissions = PermissionSet::CreateUnion(default_permissions, delta);
    ASSERT_EQ(*active_permissions,
              extension->permissions_data()->active_permissions());

    // Verify that the new granted and active permissions were also stored
    // in the extension preferences. In this case, the granted permissions
    // should be equal to the active permissions.
    ASSERT_EQ(*active_permissions,
              *prefs->GetActivePermissions(extension->id()));
    granted_permissions = active_permissions->Clone();
    ASSERT_EQ(*granted_permissions,
              *prefs->GetGrantedPermissions(extension->id()));
  }

  {
  // In the second part of the test, we'll remove the permissions that we
  // just added except for 'notifications'.
  apis.erase(APIPermission::kNotifications);
  PermissionSet delta(apis, empty_manifest_permissions, hosts,
                      scriptable_hosts);

  PermissionsUpdaterListener listener;
  PermissionsUpdater(profile_.get())
      .RemovePermissions(extension.get(), delta,
                         PermissionsUpdater::REMOVE_SOFT);
  listener.Wait();

  // Verify that the notification was correct.
  ASSERT_TRUE(listener.received_notification());
  ASSERT_EQ(extension.get(), listener.extension());
  ASSERT_EQ(UpdatedExtensionPermissionsInfo::REMOVED, listener.reason());
  ASSERT_EQ(delta, *listener.permissions());

  // Make sure the extension's active permissions reflect the change.
  active_permissions =
      PermissionSet::CreateDifference(*active_permissions, delta);
  ASSERT_EQ(*active_permissions,
            extension->permissions_data()->active_permissions());

  // Verify that the extension prefs hold the new active permissions and the
  // same granted permissions.
  ASSERT_EQ(*active_permissions, *prefs->GetActivePermissions(extension->id()));

  ASSERT_EQ(*granted_permissions,
            *prefs->GetGrantedPermissions(extension->id()));
  }
}

TEST_F(PermissionsUpdaterTest, RevokingPermissions) {
  InitializeEmptyExtensionService();

  ExtensionPrefs* prefs = ExtensionPrefs::Get(profile());

  auto api_permission_set = [](APIPermission::ID id) {
    APIPermissionSet apis;
    apis.insert(id);
    return std::make_unique<PermissionSet>(apis, ManifestPermissionSet(),
                                           URLPatternSet(), URLPatternSet());
  };

  auto url_permission_set = [](const GURL& url) {
    URLPatternSet set;
    URLPattern pattern(URLPattern::SCHEME_ALL, url.spec());
    set.AddPattern(pattern);
    return std::make_unique<PermissionSet>(
        APIPermissionSet(), ManifestPermissionSet(), set, URLPatternSet());
  };

  auto can_access_page =
      [](scoped_refptr<const extensions::Extension> extension,
         const GURL& document_url) -> bool {
    PermissionsData::PageAccess access =
        extension->permissions_data()->GetPageAccess(document_url, -1, nullptr);
    return access == PermissionsData::PageAccess::kAllowed;
  };

  {
    // Test revoking optional permissions.
    ListBuilder optional_permissions;
    optional_permissions.Append("tabs").Append("cookies").Append("management");
    ListBuilder required_permissions;
    required_permissions.Append("topSites");
    scoped_refptr<const Extension> extension =
        CreateExtensionWithOptionalPermissions(optional_permissions.Build(),
                                               required_permissions.Build(),
                                               "My Extension");

    PermissionsUpdater updater(profile());
    EXPECT_TRUE(updater.GetRevokablePermissions(extension.get())->IsEmpty());

    // Add the optional "cookies" permission.
    updater.AddPermissions(extension.get(),
                           *api_permission_set(APIPermission::kCookie));
    const PermissionsData* permissions = extension->permissions_data();
    // The extension should have the permission in its active permissions and
    // its granted permissions (stored in prefs). And, the permission should
    // be revokable.
    EXPECT_TRUE(permissions->HasAPIPermission(APIPermission::kCookie));
    std::unique_ptr<const PermissionSet> granted_permissions =
        prefs->GetGrantedPermissions(extension->id());
    EXPECT_TRUE(granted_permissions->HasAPIPermission(APIPermission::kCookie));
    EXPECT_TRUE(updater.GetRevokablePermissions(extension.get())
                    ->HasAPIPermission(APIPermission::kCookie));

    // Repeat with "tabs".
    updater.AddPermissions(extension.get(),
                           *api_permission_set(APIPermission::kTab));
    EXPECT_TRUE(permissions->HasAPIPermission(APIPermission::kTab));
    granted_permissions = prefs->GetGrantedPermissions(extension->id());
    EXPECT_TRUE(granted_permissions->HasAPIPermission(APIPermission::kTab));
    EXPECT_TRUE(updater.GetRevokablePermissions(extension.get())
                    ->HasAPIPermission(APIPermission::kTab));

    // Remove the "tabs" permission. The extension should no longer have it
    // in its active or granted permissions, and it shouldn't be revokable.
    // The extension should still have the "cookies" permission.
    updater.RemovePermissions(extension.get(),
                              *api_permission_set(APIPermission::kTab),
                              PermissionsUpdater::REMOVE_HARD);
    EXPECT_FALSE(permissions->HasAPIPermission(APIPermission::kTab));
    granted_permissions = prefs->GetGrantedPermissions(extension->id());
    EXPECT_FALSE(granted_permissions->HasAPIPermission(APIPermission::kTab));
    EXPECT_FALSE(updater.GetRevokablePermissions(extension.get())
                     ->HasAPIPermission(APIPermission::kTab));
    EXPECT_TRUE(permissions->HasAPIPermission(APIPermission::kCookie));
    granted_permissions = prefs->GetGrantedPermissions(extension->id());
    EXPECT_TRUE(granted_permissions->HasAPIPermission(APIPermission::kCookie));
    EXPECT_TRUE(updater.GetRevokablePermissions(extension.get())
                    ->HasAPIPermission(APIPermission::kCookie));
  }

  {
    // Test revoking non-optional host permissions with click-to-script.
    base::test::ScopedFeatureList scoped_feature_list;
    scoped_feature_list.InitAndEnableFeature(features::kRuntimeHostPermissions);
    ListBuilder optional_permissions;
    optional_permissions.Append("tabs");
    ListBuilder required_permissions;
    required_permissions.Append("topSites")
        .Append("http://*/*")
        .Append("http://*.google.com/*");
    scoped_refptr<const Extension> extension =
        CreateExtensionWithOptionalPermissions(optional_permissions.Build(),
                                               required_permissions.Build(),
                                               "My Extension");
    PermissionsUpdater updater(profile());
    updater.InitializePermissions(extension.get());

    ScriptingPermissionsModifier(profile(), extension)
        .SetAllowedOnAllUrls(false);

    // All-hosts was withheld, so the extension shouldn't have access to any
    // site (like foo.com).
    const GURL kOrigin("http://foo.com");

    EXPECT_FALSE(extension->permissions_data()
                     ->active_permissions()
                     .HasExplicitAccessToOrigin(kOrigin));
    EXPECT_TRUE(extension->permissions_data()
                    ->withheld_permissions()
                    .HasExplicitAccessToOrigin(kOrigin));

    const GURL kRequiredOrigin("http://www.google.com/");
    EXPECT_TRUE(extension->permissions_data()
                    ->active_permissions()
                    .HasExplicitAccessToOrigin(kRequiredOrigin));
    EXPECT_FALSE(updater.GetRevokablePermissions(extension.get())
                     ->HasExplicitAccessToOrigin(kRequiredOrigin));

    // Give the extension access to foo.com. Now, the foo.com permission should
    // be revokable.
    updater.AddPermissions(extension.get(), *url_permission_set(kOrigin));
    EXPECT_TRUE(extension->permissions_data()
                    ->active_permissions()
                    .HasExplicitAccessToOrigin(kOrigin));
    EXPECT_TRUE(updater.GetRevokablePermissions(extension.get())
                    ->HasExplicitAccessToOrigin(kOrigin));

    // Revoke the foo.com permission. The extension should no longer have
    // access to foo.com, and the revokable permissions should be empty.
    updater.RemovePermissions(extension.get(), *url_permission_set(kOrigin),
                              PermissionsUpdater::REMOVE_HARD);
    EXPECT_FALSE(extension->permissions_data()
                     ->active_permissions()
                     .HasExplicitAccessToOrigin(kOrigin));
    EXPECT_TRUE(extension->permissions_data()
                    ->withheld_permissions()
                    .HasExplicitAccessToOrigin(kOrigin));
    EXPECT_TRUE(updater.GetRevokablePermissions(extension.get())->IsEmpty());
  }

  {
    // Make sure policy restriction updates update permission data.
    URLPatternSet default_policy_blocked_hosts;
    URLPatternSet default_policy_allowed_hosts;
    URLPatternSet policy_blocked_hosts;
    URLPatternSet policy_allowed_hosts;
    ListBuilder optional_permissions;
    ListBuilder required_permissions;
    required_permissions.Append("tabs").Append("http://*/*");
    scoped_refptr<const Extension> extension =
        CreateExtensionWithOptionalPermissions(optional_permissions.Build(),
                                               required_permissions.Build(),
                                               "ExtensionSettings");
    AddPattern(&default_policy_blocked_hosts, "http://*.google.com/*");
    PermissionsUpdater updater(profile());
    updater.InitializePermissions(extension.get());
    extension->permissions_data()->SetDefaultPolicyHostRestrictions(
        default_policy_blocked_hosts, default_policy_allowed_hosts);

    // By default, all subdomains of google.com should be blocked.
    const GURL kOrigin("http://foo.com");
    const GURL kGoogle("http://www.google.com");
    const GURL kExampleGoogle("http://example.google.com");
    EXPECT_TRUE(
        extension->permissions_data()->UsesDefaultPolicyHostRestrictions());
    EXPECT_TRUE(can_access_page(extension, kOrigin));
    EXPECT_FALSE(can_access_page(extension, kGoogle));
    EXPECT_FALSE(can_access_page(extension, kExampleGoogle));

    AddPattern(&default_policy_allowed_hosts, "http://example.google.com/*");
    // Give the extension access to example.google.com. Now the
    // example.google.com should not be a runtime blocked host.
    updater.SetDefaultPolicyHostRestrictions(default_policy_blocked_hosts,
                                             default_policy_allowed_hosts);

    EXPECT_TRUE(
        extension->permissions_data()->UsesDefaultPolicyHostRestrictions());
    EXPECT_TRUE(can_access_page(extension, kOrigin));
    EXPECT_FALSE(can_access_page(extension, kGoogle));
    EXPECT_TRUE(can_access_page(extension, kExampleGoogle));

    // Revoke extension access to foo.com. Now, foo.com should be a runtime
    // blocked host.
    AddPattern(&default_policy_blocked_hosts, "*://*.foo.com/");
    updater.SetDefaultPolicyHostRestrictions(default_policy_blocked_hosts,
                                             default_policy_allowed_hosts);
    EXPECT_TRUE(
        extension->permissions_data()->UsesDefaultPolicyHostRestrictions());
    EXPECT_FALSE(can_access_page(extension, kOrigin));
    EXPECT_FALSE(can_access_page(extension, kGoogle));
    EXPECT_TRUE(can_access_page(extension, kExampleGoogle));

    // Remove foo.com from blocked hosts. The extension should no longer have
    // be a runtime blocked host.
    default_policy_blocked_hosts.ClearPatterns();
    AddPattern(&default_policy_blocked_hosts, "*://*.foo.com/");
    updater.SetDefaultPolicyHostRestrictions(default_policy_blocked_hosts,
                                             default_policy_allowed_hosts);
    EXPECT_TRUE(
        extension->permissions_data()->UsesDefaultPolicyHostRestrictions());
    EXPECT_FALSE(can_access_page(extension, kOrigin));
    EXPECT_TRUE(can_access_page(extension, kGoogle));
    EXPECT_TRUE(can_access_page(extension, kExampleGoogle));

    // Set an empty individual policy, should not affect default policy.
    updater.SetPolicyHostRestrictions(extension.get(), policy_blocked_hosts,
                                      policy_allowed_hosts);
    EXPECT_FALSE(
        extension->permissions_data()->UsesDefaultPolicyHostRestrictions());
    EXPECT_TRUE(can_access_page(extension, kOrigin));
    EXPECT_TRUE(can_access_page(extension, kGoogle));
    EXPECT_TRUE(can_access_page(extension, kExampleGoogle));

    // Block google.com for the Individual scope.
    // Whitelist example.google.com for the Indiviaul scope.
    // Leave google.com and example.google.com off both the whitelist and
    // blacklist for Default scope.
    AddPattern(&policy_blocked_hosts, "*://*.google.com/*");
    AddPattern(&policy_allowed_hosts, "*://example.google.com/*");
    updater.SetPolicyHostRestrictions(extension.get(), policy_blocked_hosts,
                                      policy_allowed_hosts);
    EXPECT_FALSE(
        extension->permissions_data()->UsesDefaultPolicyHostRestrictions());
    EXPECT_TRUE(can_access_page(extension, kOrigin));
    EXPECT_FALSE(can_access_page(extension, kGoogle));
    EXPECT_TRUE(can_access_page(extension, kExampleGoogle));

    // Switch back to default scope for extension.
    updater.SetUsesDefaultHostRestrictions(extension.get());
    EXPECT_TRUE(
        extension->permissions_data()->UsesDefaultPolicyHostRestrictions());
    default_policy_blocked_hosts.ClearPatterns();
    default_policy_allowed_hosts.ClearPatterns();
    updater.SetDefaultPolicyHostRestrictions(default_policy_blocked_hosts,
                                             default_policy_allowed_hosts);
  }
}

// Test that the permissions updater delegate works - in this test it removes
// the cookies permission.
TEST_F(PermissionsUpdaterTest, Delegate) {
  InitializeEmptyExtensionService();

  ListBuilder required_permissions;
  required_permissions.Append("tabs").Append("management").Append("cookies");
  scoped_refptr<const Extension> extension =
      CreateExtensionWithOptionalPermissions(
          std::make_unique<base::ListValue>(), required_permissions.Build(),
          "My Extension");

  auto test_delegate = std::make_unique<PermissionsUpdaterTestDelegate>();
  PermissionsUpdater::SetPlatformDelegate(test_delegate.get());
  PermissionsUpdater updater(profile());
  updater.InitializePermissions(extension.get());

  EXPECT_TRUE(extension->permissions_data()->HasAPIPermission(
      APIPermission::kTab));
  EXPECT_TRUE(extension->permissions_data()->HasAPIPermission(
      APIPermission::kManagement));
  EXPECT_FALSE(extension->permissions_data()->HasAPIPermission(
      APIPermission::kCookie));

  // Unset the delegate.
  PermissionsUpdater::SetPlatformDelegate(nullptr);
}

}  // namespace extensions
