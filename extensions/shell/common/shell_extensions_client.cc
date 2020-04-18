// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/shell/common/shell_extensions_client.h"

#include <memory>
#include <string>

#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/macros.h"
#include "components/version_info/version_info.h"
#include "content/public/common/user_agent.h"
#include "extensions/common/api/generated_schemas.h"
#include "extensions/common/common_manifest_handlers.h"
#include "extensions/common/extension_urls.h"
#include "extensions/common/extensions_aliases.h"
#include "extensions/common/features/json_feature_provider_source.h"
#include "extensions/common/features/simple_feature.h"
#include "extensions/common/manifest_handler.h"
#include "extensions/common/permissions/permission_message_provider.h"
#include "extensions/common/permissions/permissions_info.h"
#include "extensions/common/permissions/permissions_provider.h"
#include "extensions/common/url_pattern_set.h"
#include "extensions/grit/extensions_resources.h"
#include "extensions/shell/common/api/generated_schemas.h"
#include "extensions/shell/common/api/shell_api_features.h"
#include "extensions/shell/common/api/shell_behavior_features.h"
#include "extensions/shell/common/api/shell_manifest_features.h"
#include "extensions/shell/common/api/shell_permission_features.h"
#include "extensions/shell/grit/app_shell_resources.h"

namespace extensions {

namespace {

// TODO(jamescook): Refactor ChromePermissionsMessageProvider so we can share
// code. For now, this implementation does nothing.
class ShellPermissionMessageProvider : public PermissionMessageProvider {
 public:
  ShellPermissionMessageProvider() {}
  ~ShellPermissionMessageProvider() override {}

  // PermissionMessageProvider implementation.
  PermissionMessages GetPermissionMessages(
      const PermissionIDSet& permissions) const override {
    return PermissionMessages();
  }

  bool IsPrivilegeIncrease(const PermissionSet& granted_permissions,
                           const PermissionSet& requested_permissions,
                           Manifest::Type extension_type) const override {
    // Ensure we implement this before shipping.
    CHECK(false);
    return false;
  }

  PermissionIDSet GetAllPermissionIDs(
      const PermissionSet& permissions,
      Manifest::Type extension_type) const override {
    return PermissionIDSet();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ShellPermissionMessageProvider);
};

base::LazyInstance<ShellPermissionMessageProvider>::DestructorAtExit
    g_permission_message_provider = LAZY_INSTANCE_INITIALIZER;

}  // namespace

ShellExtensionsClient::ShellExtensionsClient()
    : extensions_api_permissions_(ExtensionsAPIPermissions()),
      webstore_base_url_(extension_urls::kChromeWebstoreBaseURL),
      webstore_update_url_(extension_urls::kChromeWebstoreUpdateURL) {}

ShellExtensionsClient::~ShellExtensionsClient() {
}

void ShellExtensionsClient::Initialize() {
  RegisterCommonManifestHandlers();
  ManifestHandler::FinalizeRegistration();
  // TODO(jamescook): Do we need to whitelist any extensions?

  PermissionsInfo::GetInstance()->AddProvider(extensions_api_permissions_,
                                              GetExtensionsPermissionAliases());
}

void ShellExtensionsClient::InitializeWebStoreUrls(
    base::CommandLine* command_line) {}

const PermissionMessageProvider&
ShellExtensionsClient::GetPermissionMessageProvider() const {
  NOTIMPLEMENTED();
  return g_permission_message_provider.Get();
}

const std::string ShellExtensionsClient::GetProductName() {
  return "app_shell";
}

std::unique_ptr<FeatureProvider> ShellExtensionsClient::CreateFeatureProvider(
    const std::string& name) const {
  std::unique_ptr<FeatureProvider> provider;
  if (name == "api") {
    provider.reset(new ShellAPIFeatureProvider());
  } else if (name == "manifest") {
    provider.reset(new ShellManifestFeatureProvider());
  } else if (name == "permission") {
    provider.reset(new ShellPermissionFeatureProvider());
  } else if (name == "behavior") {
    provider.reset(new ShellBehaviorFeatureProvider());
  } else {
    NOTREACHED();
  }
  return provider;
}

std::unique_ptr<JSONFeatureProviderSource>
ShellExtensionsClient::CreateAPIFeatureSource() const {
  std::unique_ptr<JSONFeatureProviderSource> source(
      new JSONFeatureProviderSource("api"));
  source->LoadJSON(IDR_EXTENSION_API_FEATURES);
  source->LoadJSON(IDR_SHELL_EXTENSION_API_FEATURES);
  return source;
}

void ShellExtensionsClient::FilterHostPermissions(
    const URLPatternSet& hosts,
    URLPatternSet* new_hosts,
    PermissionIDSet* permissions) const {
  NOTIMPLEMENTED();
}

void ShellExtensionsClient::SetScriptingWhitelist(
    const ScriptingWhitelist& whitelist) {
  scripting_whitelist_ = whitelist;
}

const ExtensionsClient::ScriptingWhitelist&
ShellExtensionsClient::GetScriptingWhitelist() const {
  // TODO(jamescook): Real whitelist.
  return scripting_whitelist_;
}

URLPatternSet ShellExtensionsClient::GetPermittedChromeSchemeHosts(
    const Extension* extension,
    const APIPermissionSet& api_permissions) const {
  NOTIMPLEMENTED();
  return URLPatternSet();
}

bool ShellExtensionsClient::IsScriptableURL(const GURL& url,
                                            std::string* error) const {
  // No restrictions on URLs.
  return true;
}

bool ShellExtensionsClient::IsAPISchemaGenerated(
    const std::string& name) const {
  return api::GeneratedSchemas::IsGenerated(name) ||
         shell::api::ShellGeneratedSchemas::IsGenerated(name);
}

base::StringPiece ShellExtensionsClient::GetAPISchema(
    const std::string& name) const {
  // Schema for app_shell-only APIs.
  if (shell::api::ShellGeneratedSchemas::IsGenerated(name))
    return shell::api::ShellGeneratedSchemas::Get(name);

  // Core extensions APIs.
  return api::GeneratedSchemas::Get(name);
}

bool ShellExtensionsClient::ShouldSuppressFatalErrors() const {
  return true;
}

void ShellExtensionsClient::RecordDidSuppressFatalError() {
}

const GURL& ShellExtensionsClient::GetWebstoreBaseURL() const {
  return webstore_base_url_;
}

const GURL& ShellExtensionsClient::GetWebstoreUpdateURL() const {
  return webstore_update_url_;
}

bool ShellExtensionsClient::IsBlacklistUpdateURL(const GURL& url) const {
  // TODO(rockot): Maybe we want to do something else here. For now we accept
  // any URL as a blacklist URL because we don't really care.
  return true;
}

std::string ShellExtensionsClient::GetUserAgent() const {
  return content::BuildUserAgentFromProduct(
      version_info::GetProductNameAndVersionForUserAgent());
}

}  // namespace extensions
