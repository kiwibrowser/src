// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_BROWSER_EXTENSIONS_CAST_EXTENSIONS_BROWSER_CLIENT_H_
#define CHROMECAST_BROWSER_EXTENSIONS_CAST_EXTENSIONS_BROWSER_CLIENT_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "extensions/browser/extensions_browser_client.h"
#include "extensions/browser/kiosk/kiosk_delegate.h"

class PrefService;

namespace extensions {

class ExtensionsAPIClient;

// An ExtensionsBrowserClient that supports a single content::BrowserContent
// with no related incognito context.
class CastExtensionsBrowserClient : public ExtensionsBrowserClient {
 public:
  // |context| is the single BrowserContext used for IsValidContext() below.
  // |pref_service| is used for GetPrefServiceForContext() below.
  CastExtensionsBrowserClient(content::BrowserContext* context,
                              PrefService* pref_service);
  ~CastExtensionsBrowserClient() override;

  // ExtensionsBrowserClient overrides:
  bool IsShuttingDown() override;
  bool AreExtensionsDisabled(const base::CommandLine& command_line,
                             content::BrowserContext* context) override;
  bool IsValidContext(content::BrowserContext* context) override;
  bool IsSameContext(content::BrowserContext* first,
                     content::BrowserContext* second) override;
  bool HasOffTheRecordContext(content::BrowserContext* context) override;
  content::BrowserContext* GetOffTheRecordContext(
      content::BrowserContext* context) override;
  content::BrowserContext* GetOriginalContext(
      content::BrowserContext* context) override;
  bool IsGuestSession(content::BrowserContext* context) const override;
  bool IsExtensionIncognitoEnabled(
      const std::string& extension_id,
      content::BrowserContext* context) const override;
  bool CanExtensionCrossIncognito(
      const Extension* extension,
      content::BrowserContext* context) const override;
  net::URLRequestJob* MaybeCreateResourceBundleRequestJob(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate,
      const base::FilePath& directory_path,
      const std::string& content_security_policy,
      bool send_cors_header) override;
  base::FilePath GetBundleResourcePath(
      const network::ResourceRequest& request,
      const base::FilePath& extension_resources_path,
      int* resource_id) const override;
  void LoadResourceFromResourceBundle(
      const network::ResourceRequest& request,
      network::mojom::URLLoaderRequest loader,
      const base::FilePath& resource_relative_path,
      int resource_id,
      const std::string& content_security_policy,
      network::mojom::URLLoaderClientPtr client,
      bool send_cors_header) override;
  bool AllowCrossRendererResourceLoad(const GURL& url,
                                      content::ResourceType resource_type,
                                      ui::PageTransition page_transition,
                                      int child_id,
                                      bool is_incognito,
                                      const Extension* extension,
                                      const ExtensionSet& extensions,
                                      const ProcessMap& process_map) override;
  PrefService* GetPrefServiceForContext(
      content::BrowserContext* context) override;
  void GetEarlyExtensionPrefsObservers(
      content::BrowserContext* context,
      std::vector<ExtensionPrefsObserver*>* observers) const override;
  ProcessManagerDelegate* GetProcessManagerDelegate() const override;
  std::unique_ptr<ExtensionHostDelegate> CreateExtensionHostDelegate() override;
  bool DidVersionUpdate(content::BrowserContext* context) override;
  void PermitExternalProtocolHandler() override;
  bool IsRunningInForcedAppMode() override;
  bool IsLoggedInAsPublicAccount() override;
  ExtensionSystemProvider* GetExtensionSystemFactory() override;
  void RegisterExtensionFunctions(
      ExtensionFunctionRegistry* registry) const override;
  void RegisterExtensionInterfaces(service_manager::BinderRegistryWithArgs<
                                       content::RenderFrameHost*>* registry,
                                   content::RenderFrameHost* render_frame_host,
                                   const Extension* extension) const override;
  std::unique_ptr<RuntimeAPIDelegate> CreateRuntimeAPIDelegate(
      content::BrowserContext* context) const override;
  const ComponentExtensionResourceManager*
  GetComponentExtensionResourceManager() override;
  void BroadcastEventToRenderers(
      events::HistogramValue histogram_value,
      const std::string& event_name,
      std::unique_ptr<base::ListValue> args) override;
  net::NetLog* GetNetLog() override;
  ExtensionCache* GetExtensionCache() override;
  bool IsBackgroundUpdateAllowed() override;
  bool IsMinBrowserVersionSupported(const std::string& min_version) override;
  ExtensionWebContentsObserver* GetExtensionWebContentsObserver(
      content::WebContents* web_contents) override;
  ExtensionNavigationUIData* GetExtensionNavigationUIData(
      net::URLRequest* request) override;
  KioskDelegate* GetKioskDelegate() override;
  bool IsLockScreenContext(content::BrowserContext* context) override;
  std::string GetApplicationLocale() override;
  bool IsAppModeForcedForApp(const ExtensionId& id) override;

  // Sets the API client.
  void SetAPIClientForTest(ExtensionsAPIClient* api_client);

 private:
  // The single BrowserContext for cast_shell. Not owned.
  content::BrowserContext* browser_context_;

  // The PrefService for |browser_context_|. Not owned.
  PrefService* pref_service_;

  // Support for extension APIs.
  std::unique_ptr<ExtensionsAPIClient> api_client_;

  DISALLOW_COPY_AND_ASSIGN(CastExtensionsBrowserClient);
};

}  // namespace extensions

#endif  // CHROMECAST_BROWSER_EXTENSIONS_CAST_EXTENSIONS_BROWSER_CLIENT_H_
