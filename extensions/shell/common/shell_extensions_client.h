// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_SHELL_COMMON_SHELL_EXTENSIONS_CLIENT_H_
#define EXTENSIONS_SHELL_COMMON_SHELL_EXTENSIONS_CLIENT_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "extensions/common/extensions_client.h"
#include "extensions/common/permissions/extensions_api_permissions.h"
#include "url/gurl.h"

namespace extensions {

// The app_shell implementation of ExtensionsClient.
class ShellExtensionsClient : public ExtensionsClient {
 public:
  ShellExtensionsClient();
  ~ShellExtensionsClient() override;

  // ExtensionsClient overrides:
  void Initialize() override;
  void InitializeWebStoreUrls(base::CommandLine* command_line) override;
  const PermissionMessageProvider& GetPermissionMessageProvider()
      const override;
  const std::string GetProductName() override;
  std::unique_ptr<FeatureProvider> CreateFeatureProvider(
      const std::string& name) const override;
  std::unique_ptr<JSONFeatureProviderSource> CreateAPIFeatureSource()
      const override;
  void FilterHostPermissions(const URLPatternSet& hosts,
                             URLPatternSet* new_hosts,
                             PermissionIDSet* permissions) const override;
  void SetScriptingWhitelist(const ScriptingWhitelist& whitelist) override;
  const ScriptingWhitelist& GetScriptingWhitelist() const override;
  URLPatternSet GetPermittedChromeSchemeHosts(
      const Extension* extension,
      const APIPermissionSet& api_permissions) const override;
  bool IsScriptableURL(const GURL& url, std::string* error) const override;
  bool IsAPISchemaGenerated(const std::string& name) const override;
  base::StringPiece GetAPISchema(const std::string& name) const override;
  bool ShouldSuppressFatalErrors() const override;
  void RecordDidSuppressFatalError() override;
  const GURL& GetWebstoreBaseURL() const override;
  const GURL& GetWebstoreUpdateURL() const override;
  bool IsBlacklistUpdateURL(const GURL& url) const override;
  std::string GetUserAgent() const override;

 private:
  const ExtensionsAPIPermissions extensions_api_permissions_;

  ScriptingWhitelist scripting_whitelist_;

  const GURL webstore_base_url_;
  const GURL webstore_update_url_;

  DISALLOW_COPY_AND_ASSIGN(ShellExtensionsClient);
};

}  // namespace extensions

#endif  // EXTENSIONS_SHELL_COMMON_SHELL_EXTENSIONS_CLIENT_H_
