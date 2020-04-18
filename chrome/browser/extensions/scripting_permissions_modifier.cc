// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/scripting_permissions_modifier.h"

#include "base/feature_list.h"
#include "chrome/browser/extensions/extension_sync_service.h"
#include "chrome/browser/extensions/permissions_updater.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_features.h"
#include "extensions/common/manifest_handlers/permissions_parser.h"
#include "extensions/common/permissions/permission_set.h"
#include "extensions/common/permissions/permissions_data.h"
#include "extensions/common/user_script.h"

namespace extensions {

namespace {

// The entry into the ExtensionPrefs for allowing an extension to script on
// all urls without explicit permission.
const char kExtensionAllowedOnAllUrlsPrefName[] =
    "extension_can_script_all_urls";

URLPatternSet FilterImpliedAllHostsPatterns(const URLPatternSet& patterns) {
  URLPatternSet result;
  for (const URLPattern& pattern : patterns) {
    if (pattern.MatchesEffectiveTld())
      result.AddPattern(pattern);
  }
  return result;
}

// Returns true if the extension must be allowed to access all urls.
bool ExtensionMustBeAllowedOnAllUrls(const Extension& extension) {
  // Some extensions must retain privilege to execute on all urls. Specifically,
  // extensions that don't show up in chrome:extensions (where withheld
  // permissions couldn't be granted), extensions that are part of chrome or
  // corporate policy, and extensions that are whitelisted to script everywhere
  // must always have permission to run on a page.
  return !extension.ShouldDisplayInExtensionSettings() ||
         Manifest::IsPolicyLocation(extension.location()) ||
         Manifest::IsComponentLocation(extension.location()) ||
         PermissionsData::CanExecuteScriptEverywhere(extension.id(),
                                                     extension.location());
}

// Partitions |permissions| into two sets of permissions, placing any
// all-hosts-like permissions into |withheld_permissions_out| and the rest
// into |granted_permissions_out|.
void PartitionPermissions(
    const PermissionSet& permissions,
    std::unique_ptr<const PermissionSet>* granted_permissions_out,
    std::unique_ptr<const PermissionSet>* withheld_permissions_out) {
  auto segregate_url_permissions = [](const URLPatternSet& patterns,
                                      URLPatternSet* granted,
                                      URLPatternSet* withheld) {
    for (const URLPattern& pattern : patterns) {
      if (pattern.MatchesEffectiveTld())
        withheld->AddPattern(pattern);
      else
        granted->AddPattern(pattern);
    }
  };

  URLPatternSet granted_explicit_hosts;
  URLPatternSet withheld_explicit_hosts;
  URLPatternSet granted_scriptable_hosts;
  URLPatternSet withheld_scriptable_hosts;
  segregate_url_permissions(permissions.explicit_hosts(),
                            &granted_explicit_hosts, &withheld_explicit_hosts);
  segregate_url_permissions(permissions.scriptable_hosts(),
                            &granted_scriptable_hosts,
                            &withheld_scriptable_hosts);

  granted_permissions_out->reset(
      new PermissionSet(permissions.apis(), permissions.manifest_permissions(),
                        granted_explicit_hosts, granted_scriptable_hosts));
  withheld_permissions_out->reset(
      new PermissionSet(APIPermissionSet(), ManifestPermissionSet(),
                        withheld_explicit_hosts, withheld_scriptable_hosts));
}

// Returns true if the extension should even be considered for being affected
// by the runtime host permissions experiment.
bool ShouldConsiderExtension(const Extension& extension) {
  // No extensions are affected if the experiment is disabled.
  if (!base::FeatureList::IsEnabled(features::kRuntimeHostPermissions))
    return false;

  // Certain extensions are always exempt from having permissions withheld.
  if (ExtensionMustBeAllowedOnAllUrls(extension))
    return false;

  return true;
}

base::Optional<bool> GetPrefValue(const ExtensionPrefs& prefs,
                                  const ExtensionId& id) {
  bool allowed = false;
  if (!prefs.ReadPrefAsBoolean(id, kExtensionAllowedOnAllUrlsPrefName,
                               &allowed)) {
    return base::nullopt;
  }
  return allowed;
}

}  // namespace

ScriptingPermissionsModifier::ScriptingPermissionsModifier(
    content::BrowserContext* browser_context,
    const scoped_refptr<const Extension>& extension)
    : browser_context_(browser_context),
      extension_(extension),
      extension_prefs_(ExtensionPrefs::Get(browser_context_)) {
  DCHECK(extension_);
}

ScriptingPermissionsModifier::~ScriptingPermissionsModifier() {}

void ScriptingPermissionsModifier::SetAllowedOnAllUrls(bool allowed) {
  DCHECK(CanAffectExtension());

  if (IsAllowedOnAllUrls() == allowed)
    return;

  extension_prefs_->UpdateExtensionPref(extension_->id(),
                                        kExtensionAllowedOnAllUrlsPrefName,
                                        std::make_unique<base::Value>(allowed));
  if (allowed)
    GrantWithheldImpliedAllHosts();
  else
    WithholdImpliedAllHosts();

  // If this was an update to permissions, we also need to sync the change.
  // TODO(devlin): This isn't currently necessary. We should remove it and add
  // it back.
  ExtensionSyncService* sync_service =
      ExtensionSyncService::Get(browser_context_);
  if (sync_service)  // |sync_service| can be null in unittests.
    sync_service->SyncExtensionChangeIfNeeded(*extension_);
}

bool ScriptingPermissionsModifier::IsAllowedOnAllUrls() const {
  DCHECK(CanAffectExtension());

  base::Optional<bool> pref_value =
      GetPrefValue(*extension_prefs_, extension_->id());
  if (!pref_value.has_value()) {
    // If there is no value present, default to true.
    return true;
  }
  return *pref_value;
}

bool ScriptingPermissionsModifier::CanAffectExtension() const {
  if (!ShouldConsiderExtension(*extension_))
    return false;

  // The extension can be affected if it currently has all-hosts-style
  // permissions, or if it did and they are actively withheld.
  return extension_->permissions_data()
             ->active_permissions()
             .ShouldWarnAllHosts() ||
         extension_->permissions_data()->HasWithheldImpliedAllHosts();
}

void ScriptingPermissionsModifier::GrantHostPermission(const GURL& url) {
  DCHECK(CanAffectExtension());

  GURL origin = url.GetOrigin();
  URLPatternSet new_explicit_hosts;
  URLPatternSet new_scriptable_hosts;

  const PermissionSet& withheld_permissions =
      extension_->permissions_data()->withheld_permissions();
  if (withheld_permissions.explicit_hosts().MatchesURL(url)) {
    new_explicit_hosts.AddOrigin(UserScript::ValidUserScriptSchemes(), origin);
  }
  if (withheld_permissions.scriptable_hosts().MatchesURL(url)) {
    new_scriptable_hosts.AddOrigin(UserScript::ValidUserScriptSchemes(),
                                   origin);
  }

  PermissionsUpdater(browser_context_)
      .AddPermissions(extension_.get(),
                      PermissionSet(APIPermissionSet(), ManifestPermissionSet(),
                                    new_explicit_hosts, new_scriptable_hosts));
}

bool ScriptingPermissionsModifier::HasGrantedHostPermission(
    const GURL& url) const {
  DCHECK(CanAffectExtension());

  GURL origin = url.GetOrigin();
  // If the extension doesn't have access to the host, it clearly hasn't been
  // granted permission.
  if (!extension_->permissions_data()
           ->active_permissions()
           .effective_hosts()
           .MatchesURL(origin)) {
    return false;
  }

  // Check which permissions would have been withheld. If access to the host
  // would have otherwise been withheld, then we know that access has been
  // explicitly granted.
  // TODO(devlin): This seems wrong, and won't work with trying to grant or
  // withhold e.g. optional permissions. It's also overly expensive.
  const PermissionSet& required_permissions =
      PermissionsParser::GetRequiredPermissions(extension_.get());
  std::unique_ptr<const PermissionSet> granted_permissions;
  std::unique_ptr<const PermissionSet> withheld_permissions;
  PartitionPermissions(required_permissions, &granted_permissions,
                       &withheld_permissions);
  if (!granted_permissions->effective_hosts().MatchesURL(origin) &&
      withheld_permissions->effective_hosts().MatchesURL(origin)) {
    return true;
  }

  return false;
}

void ScriptingPermissionsModifier::RemoveGrantedHostPermission(
    const GURL& url) {
  DCHECK(CanAffectExtension());
  DCHECK(HasGrantedHostPermission(url));

  GURL origin = url.GetOrigin();
  URLPatternSet explicit_hosts;
  URLPatternSet scriptable_hosts;
  const PermissionSet& active_permissions =
      extension_->permissions_data()->active_permissions();

  // We know the host permission was granted, but it may only be requested in
  // either explicit or scriptable hosts. Only remove it if it is already
  // present.
  if (active_permissions.explicit_hosts().MatchesURL(url))
    explicit_hosts.AddOrigin(UserScript::ValidUserScriptSchemes(), origin);
  if (active_permissions.scriptable_hosts().MatchesURL(url))
    scriptable_hosts.AddOrigin(UserScript::ValidUserScriptSchemes(), origin);

  PermissionsUpdater(browser_context_)
      .RemovePermissions(
          extension_.get(),
          PermissionSet(APIPermissionSet(), ManifestPermissionSet(),
                        explicit_hosts, scriptable_hosts),
          PermissionsUpdater::REMOVE_HARD);
}

// static
void ScriptingPermissionsModifier::WithholdPermissionsIfNecessary(
    const Extension& extension,
    const ExtensionPrefs& extension_prefs,
    const PermissionSet& permissions,
    std::unique_ptr<const PermissionSet>* granted_permissions_out,
    std::unique_ptr<const PermissionSet>* withheld_permissions_out) {
  bool should_withhold = false;
  if (ShouldConsiderExtension(extension)) {
    base::Optional<bool> pref_value =
        GetPrefValue(extension_prefs, extension.id());
    should_withhold = pref_value.has_value() && pref_value.value() == false;
  }

  should_withhold &= permissions.ShouldWarnAllHosts();
  if (!should_withhold) {
    *granted_permissions_out = permissions.Clone();
    withheld_permissions_out->reset(new PermissionSet());
    return;
  }

  PartitionPermissions(permissions, granted_permissions_out,
                       withheld_permissions_out);
}

std::unique_ptr<const PermissionSet>
ScriptingPermissionsModifier::GetRevokablePermissions() const {
  // No extra revokable permissions if the extension couldn't ever be affected.
  if (!ShouldConsiderExtension(*extension_))
    return nullptr;

  std::unique_ptr<const PermissionSet> granted_permissions;
  std::unique_ptr<const PermissionSet> withheld_permissions;
  PartitionPermissions(extension_->permissions_data()->active_permissions(),
                       &granted_permissions, &withheld_permissions);

  return withheld_permissions;
}

void ScriptingPermissionsModifier::GrantWithheldImpliedAllHosts() {
  const PermissionSet& withheld =
      extension_->permissions_data()->withheld_permissions();

  PermissionSet permissions(
      APIPermissionSet(), ManifestPermissionSet(),
      FilterImpliedAllHostsPatterns(withheld.explicit_hosts()),
      FilterImpliedAllHostsPatterns(withheld.scriptable_hosts()));
  PermissionsUpdater(browser_context_)
      .AddPermissions(extension_.get(), permissions);
}

void ScriptingPermissionsModifier::WithholdImpliedAllHosts() {
  const PermissionSet& active =
      extension_->permissions_data()->active_permissions();
  PermissionSet permissions(
      APIPermissionSet(), ManifestPermissionSet(),
      FilterImpliedAllHostsPatterns(active.explicit_hosts()),
      FilterImpliedAllHostsPatterns(active.scriptable_hosts()));
  PermissionsUpdater(browser_context_)
      .RemovePermissions(extension_.get(), permissions,
                         PermissionsUpdater::REMOVE_HARD);
}

}  // namespace extensions
