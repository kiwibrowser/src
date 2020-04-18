// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/chrome_extensions_browser_client.h"

#include <utility>

#include "base/command_line.h"
#include "base/logging.h"
#include "base/version.h"
#include "build/build_config.h"
#include "chrome/browser/app_mode/app_mode_utils.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/extensions/activity_log/activity_log.h"
#include "chrome/browser/extensions/api/chrome_extensions_api_client.h"
#include "chrome/browser/extensions/api/content_settings/content_settings_service.h"
#include "chrome/browser/extensions/api/generated_api_registration.h"
#include "chrome/browser/extensions/api/preference/preference_api.h"
#include "chrome/browser/extensions/api/runtime/chrome_runtime_api_delegate.h"
#include "chrome/browser/extensions/chrome_component_extension_resource_manager.h"
#include "chrome/browser/extensions/chrome_extension_api_frame_id_map_helper.h"
#include "chrome/browser/extensions/chrome_extension_host_delegate.h"
#include "chrome/browser/extensions/chrome_extension_web_contents_observer.h"
#include "chrome/browser/extensions/chrome_extensions_interface_registration.h"
#include "chrome/browser/extensions/chrome_kiosk_delegate.h"
#include "chrome/browser/extensions/chrome_process_manager_delegate.h"
#include "chrome/browser/extensions/chrome_url_request_util.h"
#include "chrome/browser/extensions/error_console/error_console.h"
#include "chrome/browser/extensions/event_router_forwarder.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_system_factory.h"
#include "chrome/browser/extensions/extension_util.h"
#include "chrome/browser/extensions/menu_manager.h"
#include "chrome/browser/extensions/updater/chrome_update_client_config.h"
#include "chrome/browser/external_protocol/external_protocol_handler.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/renderer_host/chrome_navigation_ui_data.h"
#include "chrome/browser/sessions/session_tab_helper.h"
#include "chrome/browser/task_manager/web_contents_tags.h"
#include "chrome/browser/ui/bluetooth/chrome_extension_bluetooth_chooser.h"
#include "chrome/common/channel_info.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "components/net_log/chrome_net_log.h"
#include "components/update_client/update_client.h"
#include "components/version_info/version_info.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/common/content_switches.h"
#include "extensions/browser/api/generated_api_registration.h"
#include "extensions/browser/extension_function_registry.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/extension_util.h"
#include "extensions/browser/mojo/interface_registration.h"
#include "extensions/browser/pref_names.h"
#include "extensions/browser/url_request_util.h"
#include "extensions/common/features/feature_channel.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/extensions/updater/chromeos_extension_cache_delegate.h"
#include "chrome/browser/extensions/updater/extension_cache_impl.h"
#include "chromeos/chromeos_switches.h"
#include "components/user_manager/user_manager.h"
#else
#include "extensions/browser/updater/null_extension_cache.h"
#endif

namespace extensions {

namespace {

// If true, the extensions client will behave as though there is always a
// new chrome update.
bool g_did_chrome_update_for_testing = false;

bool ExtensionsDisabled(const base::CommandLine& command_line) {
  return command_line.HasSwitch(switches::kDisableExtensions) ||
         command_line.HasSwitch(switches::kDisableExtensionsExcept);
}

}  // namespace

ChromeExtensionsBrowserClient::ChromeExtensionsBrowserClient() {
  process_manager_delegate_.reset(new ChromeProcessManagerDelegate);
  api_client_.reset(new ChromeExtensionsAPIClient);
  SetCurrentChannel(chrome::GetChannel());
  resource_manager_.reset(new ChromeComponentExtensionResourceManager());
}

ChromeExtensionsBrowserClient::~ChromeExtensionsBrowserClient() {}

bool ChromeExtensionsBrowserClient::IsShuttingDown() {
  return g_browser_process->IsShuttingDown();
}

bool ChromeExtensionsBrowserClient::AreExtensionsDisabled(
    const base::CommandLine& command_line,
    content::BrowserContext* context) {
  Profile* profile = static_cast<Profile*>(context);
  return ExtensionsDisabled(command_line) ||
         profile->GetPrefs()->GetBoolean(prefs::kDisableExtensions);
}

bool ChromeExtensionsBrowserClient::IsValidContext(
    content::BrowserContext* context) {
  Profile* profile = static_cast<Profile*>(context);
  return g_browser_process->profile_manager() &&
         g_browser_process->profile_manager()->IsValidProfile(profile);
}

bool ChromeExtensionsBrowserClient::IsSameContext(
    content::BrowserContext* first,
    content::BrowserContext* second) {
  Profile* first_profile = Profile::FromBrowserContext(first);
  Profile* second_profile = Profile::FromBrowserContext(second);
  // TODO(crbug.com/727487): We need to check both ways because of offscreen
  // presentation profiles, which are not registered with the original profile.
  // This can be reverted to check just first->IsSameProfile(second) when Bug
  // 727487 is fixed and presentations have a proper profile type.  See Bug
  // 664351 for background.
  return first_profile->IsSameProfile(second_profile) ||
         second_profile->IsSameProfile(first_profile);
}

bool ChromeExtensionsBrowserClient::HasOffTheRecordContext(
    content::BrowserContext* context) {
  return static_cast<Profile*>(context)->HasOffTheRecordProfile();
}

content::BrowserContext* ChromeExtensionsBrowserClient::GetOffTheRecordContext(
    content::BrowserContext* context) {
  return static_cast<Profile*>(context)->GetOffTheRecordProfile();
}

content::BrowserContext* ChromeExtensionsBrowserClient::GetOriginalContext(
    content::BrowserContext* context) {
  DCHECK(context);
  return static_cast<Profile*>(context)->GetOriginalProfile();
}

#if defined(OS_CHROMEOS)
std::string ChromeExtensionsBrowserClient::GetUserIdHashFromContext(
    content::BrowserContext* context) {
  return chromeos::ProfileHelper::GetUserIdHashFromProfile(
      static_cast<Profile*>(context));
}
#endif

bool ChromeExtensionsBrowserClient::IsGuestSession(
    content::BrowserContext* context) const {
  return static_cast<Profile*>(context)->IsGuestSession();
}

bool ChromeExtensionsBrowserClient::IsExtensionIncognitoEnabled(
    const std::string& extension_id,
    content::BrowserContext* context) const {
  return IsGuestSession(context)
      || util::IsIncognitoEnabled(extension_id, context);
}

bool ChromeExtensionsBrowserClient::CanExtensionCrossIncognito(
    const Extension* extension,
    content::BrowserContext* context) const {
  return IsGuestSession(context)
      || util::CanCrossIncognito(extension, context);
}

net::URLRequestJob*
ChromeExtensionsBrowserClient::MaybeCreateResourceBundleRequestJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate,
    const base::FilePath& directory_path,
    const std::string& content_security_policy,
    bool send_cors_header) {
  return chrome_url_request_util::MaybeCreateURLRequestResourceBundleJob(
      request,
      network_delegate,
      directory_path,
      content_security_policy,
      send_cors_header);
}

base::FilePath ChromeExtensionsBrowserClient::GetBundleResourcePath(
    const network::ResourceRequest& request,
    const base::FilePath& extension_resources_path,
    int* resource_id) const {
  return chrome_url_request_util::GetBundleResourcePath(
      request, extension_resources_path, resource_id);
}

void ChromeExtensionsBrowserClient::LoadResourceFromResourceBundle(
    const network::ResourceRequest& request,
    network::mojom::URLLoaderRequest loader,
    const base::FilePath& resource_relative_path,
    int resource_id,
    const std::string& content_security_policy,
    network::mojom::URLLoaderClientPtr client,
    bool send_cors_header) {
  chrome_url_request_util::LoadResourceFromResourceBundle(
      request, std::move(loader), resource_relative_path, resource_id,
      content_security_policy, std::move(client), send_cors_header);
}

bool ChromeExtensionsBrowserClient::AllowCrossRendererResourceLoad(
    const GURL& url,
    content::ResourceType resource_type,
    ui::PageTransition page_transition,
    int child_id,
    bool is_incognito,
    const Extension* extension,
    const ExtensionSet& extensions,
    const ProcessMap& process_map) {
  bool allowed = false;
  if (chrome_url_request_util::AllowCrossRendererResourceLoad(
          url, resource_type, page_transition, child_id, is_incognito,
          extension, extensions, process_map, &allowed)) {
    return allowed;
  }

  // Couldn't determine if resource is allowed. Block the load.
  return false;
}

PrefService* ChromeExtensionsBrowserClient::GetPrefServiceForContext(
    content::BrowserContext* context) {
  return static_cast<Profile*>(context)->GetPrefs();
}

void ChromeExtensionsBrowserClient::GetEarlyExtensionPrefsObservers(
    content::BrowserContext* context,
    std::vector<ExtensionPrefsObserver*>* observers) const {
  observers->push_back(ContentSettingsService::Get(context));
}

ProcessManagerDelegate*
ChromeExtensionsBrowserClient::GetProcessManagerDelegate() const {
  return process_manager_delegate_.get();
}

std::unique_ptr<ExtensionHostDelegate>
ChromeExtensionsBrowserClient::CreateExtensionHostDelegate() {
  return std::unique_ptr<ExtensionHostDelegate>(
      new ChromeExtensionHostDelegate);
}

bool ChromeExtensionsBrowserClient::DidVersionUpdate(
    content::BrowserContext* context) {
  Profile* profile = static_cast<Profile*>(context);

  // Unit tests may not provide prefs; assume everything is up to date.
  ExtensionPrefs* extension_prefs = ExtensionPrefs::Get(profile);
  if (!extension_prefs)
    return false;

  if (g_did_chrome_update_for_testing)
    return true;

  // If we're inside a browser test, then assume prefs are all up to date.
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(switches::kTestType))
    return false;

  PrefService* pref_service = extension_prefs->pref_service();
  base::Version last_version;
  if (pref_service->HasPrefPath(pref_names::kLastChromeVersion)) {
    std::string last_version_str =
        pref_service->GetString(pref_names::kLastChromeVersion);
    last_version = base::Version(last_version_str);
  }

  std::string current_version_str = version_info::GetVersionNumber();
  const base::Version& current_version = version_info::GetVersion();
  pref_service->SetString(pref_names::kLastChromeVersion, current_version_str);

  // If there was no version string in prefs, assume we're out of date.
  if (!last_version.IsValid())
    return true;
  // If the current version string is invalid, assume we didn't update.
  if (!current_version.IsValid())
    return false;

  return last_version < current_version;
}

void ChromeExtensionsBrowserClient::PermitExternalProtocolHandler() {
  ExternalProtocolHandler::PermitLaunchUrl();
}

bool ChromeExtensionsBrowserClient::IsRunningInForcedAppMode() {
  return chrome::IsRunningInForcedAppMode();
}

bool ChromeExtensionsBrowserClient::IsAppModeForcedForApp(
    const ExtensionId& extension_id) {
  return chrome::IsRunningInForcedAppModeForApp(extension_id);
}

bool ChromeExtensionsBrowserClient::IsLoggedInAsPublicAccount() {
#if defined(OS_CHROMEOS)
  return user_manager::UserManager::Get()->IsLoggedInAsPublicAccount();
#else
  return false;
#endif
}

ExtensionSystemProvider*
ChromeExtensionsBrowserClient::GetExtensionSystemFactory() {
  return ExtensionSystemFactory::GetInstance();
}

void ChromeExtensionsBrowserClient::RegisterExtensionFunctions(
    ExtensionFunctionRegistry* registry) const {
  // Preferences.
  registry->RegisterFunction<GetPreferenceFunction>();
  registry->RegisterFunction<SetPreferenceFunction>();
  registry->RegisterFunction<ClearPreferenceFunction>();

  // Generated APIs from lower-level modules.
  api::GeneratedFunctionRegistry::RegisterAll(registry);

  // Generated APIs from Chrome.
  api::ChromeGeneratedFunctionRegistry::RegisterAll(registry);
}

void ChromeExtensionsBrowserClient::RegisterExtensionInterfaces(
    service_manager::BinderRegistryWithArgs<content::RenderFrameHost*>*
        registry,
    content::RenderFrameHost* render_frame_host,
    const Extension* extension) const {
  RegisterInterfacesForExtension(registry, render_frame_host, extension);
  RegisterChromeInterfacesForExtension(registry, render_frame_host, extension);
}

std::unique_ptr<RuntimeAPIDelegate>
ChromeExtensionsBrowserClient::CreateRuntimeAPIDelegate(
    content::BrowserContext* context) const {
  return std::unique_ptr<RuntimeAPIDelegate>(
      new ChromeRuntimeAPIDelegate(context));
}

const ComponentExtensionResourceManager*
ChromeExtensionsBrowserClient::GetComponentExtensionResourceManager() {
  return resource_manager_.get();
}

void ChromeExtensionsBrowserClient::BroadcastEventToRenderers(
    events::HistogramValue histogram_value,
    const std::string& event_name,
    std::unique_ptr<base::ListValue> args) {
  g_browser_process->extension_event_router_forwarder()
      ->BroadcastEventToRenderers(histogram_value, event_name, std::move(args),
                                  GURL());
}

net::NetLog* ChromeExtensionsBrowserClient::GetNetLog() {
  return g_browser_process->net_log();
}

ExtensionCache* ChromeExtensionsBrowserClient::GetExtensionCache() {
  if (!extension_cache_.get()) {
#if defined(OS_CHROMEOS)
    extension_cache_.reset(new ExtensionCacheImpl(
        std::make_unique<ChromeOSExtensionCacheDelegate>()));
#else
    extension_cache_.reset(new NullExtensionCache());
#endif
  }
  return extension_cache_.get();
}

bool ChromeExtensionsBrowserClient::IsBackgroundUpdateAllowed() {
  return !base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kDisableBackgroundNetworking);
}

bool ChromeExtensionsBrowserClient::IsMinBrowserVersionSupported(
    const std::string& min_version) {
  const base::Version& browser_version = version_info::GetVersion();
  base::Version browser_min_version(min_version);
  return !browser_version.IsValid() || !browser_min_version.IsValid() ||
         browser_min_version.CompareTo(browser_version) <= 0;
}

ExtensionWebContentsObserver*
ChromeExtensionsBrowserClient::GetExtensionWebContentsObserver(
    content::WebContents* web_contents) {
  return ChromeExtensionWebContentsObserver::FromWebContents(web_contents);
}

void ChromeExtensionsBrowserClient::ReportError(
    content::BrowserContext* context,
    std::unique_ptr<ExtensionError> error) {
  ErrorConsole::Get(context)->ReportError(std::move(error));
}

void ChromeExtensionsBrowserClient::CleanUpWebView(
    content::BrowserContext* browser_context,
    int embedder_process_id,
    int view_instance_id) {
  // Clean up context menus for the WebView.
  auto* menu_manager =
      MenuManager::Get(Profile::FromBrowserContext(browser_context));
  menu_manager->RemoveAllContextItems(
      MenuItem::ExtensionKey("", embedder_process_id, view_instance_id));
}

void ChromeExtensionsBrowserClient::AttachExtensionTaskManagerTag(
    content::WebContents* web_contents,
    ViewType view_type) {
  switch (view_type) {
    case VIEW_TYPE_APP_WINDOW:
    case VIEW_TYPE_COMPONENT:
    case VIEW_TYPE_EXTENSION_BACKGROUND_PAGE:
    case VIEW_TYPE_EXTENSION_DIALOG:
    case VIEW_TYPE_EXTENSION_POPUP:
      // These are the only types that are tracked by the ExtensionTag.
      task_manager::WebContentsTags::CreateForExtension(web_contents,
                                                        view_type);
      return;

    case VIEW_TYPE_BACKGROUND_CONTENTS:
    case VIEW_TYPE_EXTENSION_GUEST:
    case VIEW_TYPE_PANEL:
    case VIEW_TYPE_TAB_CONTENTS:
      // Those types are tracked by other tags:
      // BACKGROUND_CONTENTS --> task_manager::BackgroundContentsTag.
      // GUEST --> extensions::ChromeGuestViewManagerDelegate.
      // PANEL --> task_manager::PanelTag.
      // TAB_CONTENTS --> task_manager::TabContentsTag.
      // These tags are created and attached to the web_contents in other
      // locations, and they must be ignored here.
      return;

    case VIEW_TYPE_INVALID:
      NOTREACHED();
      return;
  }
}

scoped_refptr<update_client::UpdateClient>
ChromeExtensionsBrowserClient::CreateUpdateClient(
    content::BrowserContext* context) {
  return update_client::UpdateClientFactory(
      ChromeUpdateClientConfig::Create(context));
}

std::unique_ptr<ExtensionApiFrameIdMapHelper>
ChromeExtensionsBrowserClient::CreateExtensionApiFrameIdMapHelper(
    ExtensionApiFrameIdMap* map) {
  return std::make_unique<ChromeExtensionApiFrameIdMapHelper>(map);
}

std::unique_ptr<content::BluetoothChooser>
ChromeExtensionsBrowserClient::CreateBluetoothChooser(
    content::RenderFrameHost* frame,
    const content::BluetoothChooser::EventHandler& event_handler) {
  return std::make_unique<ChromeExtensionBluetoothChooser>(frame,
                                                           event_handler);
}

bool ChromeExtensionsBrowserClient::IsActivityLoggingEnabled(
    content::BrowserContext* context) {
  ActivityLog* activity_log = ActivityLog::GetInstance(context);
  return activity_log && activity_log->is_active();
}

ExtensionNavigationUIData*
ChromeExtensionsBrowserClient::GetExtensionNavigationUIData(
    net::URLRequest* request) {
  const content::ResourceRequestInfo* info =
      content::ResourceRequestInfo::ForRequest(request);
  if (!info)
    return nullptr;
  ChromeNavigationUIData* navigation_data =
      static_cast<ChromeNavigationUIData*>(info->GetNavigationUIData());
  if (!navigation_data)
    return nullptr;
  return navigation_data->GetExtensionNavigationUIData();
}

void ChromeExtensionsBrowserClient::GetTabAndWindowIdForWebContents(
    content::WebContents* web_contents,
    int* tab_id,
    int* window_id) {
  SessionTabHelper* session_tab_helper =
      SessionTabHelper::FromWebContents(web_contents);
  if (session_tab_helper) {
    *tab_id = session_tab_helper->session_id().id();
    *window_id = session_tab_helper->window_id().id();
  } else {
    *tab_id = -1;
    *window_id = -1;
  }
}

KioskDelegate* ChromeExtensionsBrowserClient::GetKioskDelegate() {
  if (!kiosk_delegate_)
    kiosk_delegate_.reset(new ChromeKioskDelegate());
  return kiosk_delegate_.get();
}

bool ChromeExtensionsBrowserClient::IsLockScreenContext(
    content::BrowserContext* context) {
#if defined(OS_CHROMEOS)
  return chromeos::ProfileHelper::IsLockScreenAppProfile(
      Profile::FromBrowserContext(context));
#else
  return false;
#endif
}

std::string ChromeExtensionsBrowserClient::GetApplicationLocale() {
  return g_browser_process->GetApplicationLocale();
}

bool ChromeExtensionsBrowserClient::IsExtensionEnabled(
    const std::string& extension_id,
    content::BrowserContext* context) const {
  return ExtensionSystem::Get(context)->extension_service()->IsExtensionEnabled(
      extension_id);
}

// static
void ChromeExtensionsBrowserClient::set_did_chrome_update_for_testing(
    bool did_update) {
  g_did_chrome_update_for_testing = did_update;
}

}  // namespace extensions
