// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/common/cast_extensions_client.h"

#include <memory>
#include <string>

#include "base/logging.h"
#include "base/macros.h"
#include "base/no_destructor.h"
#include "chromecast/common/extensions_api/cast_aliases.h"
#include "chromecast/common/extensions_api/cast_api_features.h"
#include "chromecast/common/extensions_api/cast_api_permissions.h"
#include "chromecast/common/extensions_api/cast_behavior_features.h"
#include "chromecast/common/extensions_api/cast_manifest_features.h"
#include "chromecast/common/extensions_api/cast_permission_features.h"
#include "chromecast/common/extensions_api/generated_schemas.h"
#include "chromecast/common/extensions_api/tts/tts_engine_manifest_handler.h"
#include "components/version_info/version_info.h"
#include "content/public/common/user_agent.h"
#include "extensions/common/api/generated_schemas.h"
#include "extensions/common/common_manifest_handlers.h"
#include "extensions/common/extension_urls.h"
#include "extensions/common/extensions_aliases.h"
#include "extensions/common/features/json_feature_provider_source.h"
#include "extensions/common/features/manifest_feature.h"
#include "extensions/common/features/simple_feature.h"
#include "extensions/common/manifest_handler.h"
#include "extensions/common/manifest_handlers/automation.h"
#include "extensions/common/manifest_handlers/content_scripts_handler.h"
#include "extensions/common/permissions/permission_message_provider.h"
#include "extensions/common/permissions/permissions_info.h"
#include "extensions/common/permissions/permissions_provider.h"
#include "extensions/common/url_pattern_set.h"
#include "extensions/grit/extensions_resources.h"
#include "extensions/shell/grit/app_shell_resources.h"

namespace extensions {

namespace {

void RegisterCastManifestHandlers() {
  DCHECK(!ManifestHandler::IsRegistrationFinalized());
  (new AutomationHandler)->Register();  // TODO(crbug/837773) De-dupe later.
  (new ContentScriptsHandler)->Register();
  (new TtsEngineManifestHandler)->Register();
}

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

}  // namespace

CastExtensionsClient::CastExtensionsClient()
    : extensions_api_permissions_(ExtensionsAPIPermissions()),
      webstore_base_url_(extension_urls::kChromeWebstoreBaseURL),
      webstore_update_url_(extension_urls::kChromeWebstoreUpdateURL) {}

CastExtensionsClient::~CastExtensionsClient() {}

void CastExtensionsClient::Initialize() {
  RegisterCommonManifestHandlers();
  RegisterCastManifestHandlers();
  ManifestHandler::FinalizeRegistration();
  // TODO(jamescook): Do we need to whitelist any extensions?

  PermissionsInfo::GetInstance()->AddProvider(cast_api_permissions_,
                                              GetCastPermissionAliases());
  PermissionsInfo::GetInstance()->AddProvider(extensions_api_permissions_,
                                              GetExtensionsPermissionAliases());
}

void CastExtensionsClient::InitializeWebStoreUrls(
    base::CommandLine* command_line) {}

const PermissionMessageProvider&
CastExtensionsClient::GetPermissionMessageProvider() const {
  NOTIMPLEMENTED();
  static base::NoDestructor<ShellPermissionMessageProvider>
      g_permission_message_provider;
  return *g_permission_message_provider;
}

const std::string CastExtensionsClient::GetProductName() {
  return "cast_shell";
}

std::unique_ptr<FeatureProvider> CastExtensionsClient::CreateFeatureProvider(
    const std::string& name) const {
  std::unique_ptr<FeatureProvider> provider;
  if (name == "api") {
    provider = std::make_unique<CastAPIFeatureProvider>();
  } else if (name == "manifest") {
    provider = std::make_unique<CastManifestFeatureProvider>();
  } else if (name == "permission") {
    provider = std::make_unique<CastPermissionFeatureProvider>();
  } else if (name == "behavior") {
    provider = std::make_unique<CastBehaviorFeatureProvider>();
  } else {
    NOTREACHED();
  }
  return provider;
}

std::unique_ptr<JSONFeatureProviderSource>
CastExtensionsClient::CreateAPIFeatureSource() const {
  std::unique_ptr<JSONFeatureProviderSource> source(
      new JSONFeatureProviderSource("api"));
  source->LoadJSON(IDR_EXTENSION_API_FEATURES);
  source->LoadJSON(IDR_SHELL_EXTENSION_API_FEATURES);
  return source;
}

void CastExtensionsClient::FilterHostPermissions(
    const URLPatternSet& hosts,
    URLPatternSet* new_hosts,
    PermissionIDSet* permissions) const {
  NOTIMPLEMENTED();
}

void CastExtensionsClient::SetScriptingWhitelist(
    const ScriptingWhitelist& whitelist) {
  scripting_whitelist_ = whitelist;
}

const ExtensionsClient::ScriptingWhitelist&
CastExtensionsClient::GetScriptingWhitelist() const {
  // TODO(jamescook): Real whitelist.
  return scripting_whitelist_;
}

URLPatternSet CastExtensionsClient::GetPermittedChromeSchemeHosts(
    const Extension* extension,
    const APIPermissionSet& api_permissions) const {
  NOTIMPLEMENTED();
  return URLPatternSet();
}

bool CastExtensionsClient::IsScriptableURL(const GURL& url,
                                           std::string* error) const {
  NOTIMPLEMENTED();
  return true;
}

bool CastExtensionsClient::IsAPISchemaGenerated(const std::string& name) const {
  return api::GeneratedSchemas::IsGenerated(name) ||
         cast::api::CastGeneratedSchemas::IsGenerated(name);
}

base::StringPiece CastExtensionsClient::GetAPISchema(
    const std::string& name) const {
  // Schema for cast_shell-only APIs.
  if (cast::api::CastGeneratedSchemas::IsGenerated(name))
    return cast::api::CastGeneratedSchemas::Get(name);

  // Core extensions APIs.
  return api::GeneratedSchemas::Get(name);
}

bool CastExtensionsClient::ShouldSuppressFatalErrors() const {
  return true;
}

void CastExtensionsClient::RecordDidSuppressFatalError() {}

const GURL& CastExtensionsClient::GetWebstoreBaseURL() const {
  return webstore_base_url_;
}

const GURL& CastExtensionsClient::GetWebstoreUpdateURL() const {
  return webstore_update_url_;
}

bool CastExtensionsClient::IsBlacklistUpdateURL(const GURL& url) const {
  return true;
}

std::string CastExtensionsClient::GetUserAgent() const {
  return content::BuildUserAgentFromProduct(
      version_info::GetProductNameAndVersionForUserAgent());
}

}  // namespace extensions
