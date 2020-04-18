// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_CHROME_EXTENSIONS_BROWSER_CLIENT_H_
#define CHROME_BROWSER_EXTENSIONS_CHROME_EXTENSIONS_BROWSER_CLIENT_H_

#include <memory>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "build/build_config.h"
#include "extensions/browser/extensions_browser_client.h"
#include "extensions/browser/kiosk/kiosk_delegate.h"

namespace base {
class CommandLine;
}

namespace content {
class BrowserContext;
}

namespace extensions {

class ChromeComponentExtensionResourceManager;
class ChromeExtensionsAPIClient;
class ChromeProcessManagerDelegate;

// Implementation of BrowserClient for Chrome, which includes
// knowledge of Profiles, BrowserContexts and incognito.
//
// NOTE: Methods that do not require knowledge of browser concepts should be
// implemented in ChromeExtensionsClient even if they are only used in the
// browser process (see chrome/common/extensions/chrome_extensions_client.h).
class ChromeExtensionsBrowserClient : public ExtensionsBrowserClient {
 public:
  ChromeExtensionsBrowserClient();
  ~ChromeExtensionsBrowserClient() override;

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
#if defined(OS_CHROMEOS)
  std::string GetUserIdHashFromContext(
      content::BrowserContext* context) override;
#endif
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
  bool IsAppModeForcedForApp(const ExtensionId& extension_id) override;
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
  void ReportError(content::BrowserContext* context,
                   std::unique_ptr<ExtensionError> error) override;
  void CleanUpWebView(content::BrowserContext* browser_context,
                      int embedder_process_id,
                      int view_instance_id) override;
  void AttachExtensionTaskManagerTag(content::WebContents* web_contents,
                                     ViewType view_type) override;
  scoped_refptr<update_client::UpdateClient> CreateUpdateClient(
      content::BrowserContext* context) override;
  std::unique_ptr<ExtensionApiFrameIdMapHelper>
  CreateExtensionApiFrameIdMapHelper(
      ExtensionApiFrameIdMap* map) override;
  std::unique_ptr<content::BluetoothChooser> CreateBluetoothChooser(
      content::RenderFrameHost* frame,
      const content::BluetoothChooser::EventHandler& event_handler) override;
  bool IsActivityLoggingEnabled(content::BrowserContext* context) override;
  extensions::ExtensionNavigationUIData* GetExtensionNavigationUIData(
      net::URLRequest* request) override;
  void GetTabAndWindowIdForWebContents(content::WebContents* web_contents,
                                       int* tab_id,
                                       int* window_id) override;
  KioskDelegate* GetKioskDelegate() override;
  bool IsLockScreenContext(content::BrowserContext* context) override;
  std::string GetApplicationLocale() override;
  bool IsExtensionEnabled(const std::string& extension_id,
                          content::BrowserContext* context) const override;

  static void set_did_chrome_update_for_testing(bool did_update);

 private:
  friend struct base::LazyInstanceTraitsBase<ChromeExtensionsBrowserClient>;

  // Support for ProcessManager.
  std::unique_ptr<ChromeProcessManagerDelegate> process_manager_delegate_;

  // Client for API implementations.
  std::unique_ptr<ChromeExtensionsAPIClient> api_client_;

  std::unique_ptr<ChromeComponentExtensionResourceManager> resource_manager_;

  std::unique_ptr<ExtensionCache> extension_cache_;

  std::unique_ptr<KioskDelegate> kiosk_delegate_;

  DISALLOW_COPY_AND_ASSIGN(ChromeExtensionsBrowserClient);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_CHROME_EXTENSIONS_BROWSER_CLIENT_H_
