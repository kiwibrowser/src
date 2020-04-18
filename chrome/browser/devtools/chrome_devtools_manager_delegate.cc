// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/devtools/chrome_devtools_manager_delegate.h"

#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/devtools/chrome_devtools_session.h"
#include "chrome/browser/devtools/device/android_device_manager.h"
#include "chrome/browser/devtools/device/tcp_device_provider.h"
#include "chrome/browser/devtools/devtools_window.h"
#include "chrome/browser/devtools/protocol/target_handler.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/browser/ui/tab_contents/tab_contents_iterator.h"
#include "chrome/grit/browser_resources.h"
#include "components/guest_view/browser/guest_view_base.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_host.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/process_manager.h"
#include "ui/base/resource/resource_bundle.h"

using content::DevToolsAgentHost;

const char ChromeDevToolsManagerDelegate::kTypeApp[] = "app";
const char ChromeDevToolsManagerDelegate::kTypeBackgroundPage[] =
    "background_page";

namespace {

bool GetExtensionInfo(content::WebContents* wc,
                      std::string* name,
                      std::string* type) {
  Profile* profile = Profile::FromBrowserContext(wc->GetBrowserContext());
  if (!profile)
    return false;
  const extensions::Extension* extension =
      extensions::ProcessManager::Get(profile)->GetExtensionForWebContents(wc);
  if (!extension)
    return false;
  extensions::ExtensionHost* extension_host =
      extensions::ProcessManager::Get(profile)->GetBackgroundHostForExtension(
          extension->id());
  if (extension_host && extension_host->host_contents() == wc) {
    *name = extension->name();
    *type = ChromeDevToolsManagerDelegate::kTypeBackgroundPage;
    return true;
  }
  if (extension->is_hosted_app() || extension->is_legacy_packaged_app() ||
      extension->is_platform_app()) {
    *name = extension->name();
    *type = ChromeDevToolsManagerDelegate::kTypeApp;
    return true;
  }
  return false;
}

ChromeDevToolsManagerDelegate* g_instance;

}  // namespace

// static
ChromeDevToolsManagerDelegate* ChromeDevToolsManagerDelegate::GetInstance() {
  return g_instance;
}

ChromeDevToolsManagerDelegate::ChromeDevToolsManagerDelegate() {
  DCHECK(!g_instance);
  g_instance = this;
}

ChromeDevToolsManagerDelegate::~ChromeDevToolsManagerDelegate() {
  DCHECK(g_instance == this);
  g_instance = nullptr;
}

void ChromeDevToolsManagerDelegate::Inspect(
    content::DevToolsAgentHost* agent_host) {
  DevToolsWindow::OpenDevToolsWindow(agent_host, nullptr);
}

bool ChromeDevToolsManagerDelegate::HandleCommand(
    DevToolsAgentHost* agent_host,
    content::DevToolsAgentHostClient* client,
    base::DictionaryValue* command_dict) {
  DCHECK(sessions_.find(client) != sessions_.end());
  auto response = sessions_[client]->dispatcher()->dispatch(
      protocol::toProtocolValue(command_dict, 1000));
  return response != protocol::DispatchResponse::Status::kFallThrough;
}

std::string ChromeDevToolsManagerDelegate::GetTargetType(
    content::WebContents* web_contents) {
  auto& all_tabs = AllTabContentses();
  auto it = std::find(all_tabs.begin(), all_tabs.end(), web_contents);
  if (it != all_tabs.end())
    return DevToolsAgentHost::kTypePage;

  std::string extension_name;
  std::string extension_type;
  if (!GetExtensionInfo(web_contents, &extension_name, &extension_type))
    return DevToolsAgentHost::kTypeOther;
  return extension_type;
}

std::string ChromeDevToolsManagerDelegate::GetTargetTitle(
    content::WebContents* web_contents) {
  std::string extension_name;
  std::string extension_type;
  if (!GetExtensionInfo(web_contents, &extension_name, &extension_type))
    return std::string();
  return extension_name;
}

void ChromeDevToolsManagerDelegate::ClientAttached(
    content::DevToolsAgentHost* agent_host,
    content::DevToolsAgentHostClient* client) {
  DCHECK(sessions_.find(client) == sessions_.end());
  sessions_[client] =
      std::make_unique<ChromeDevToolsSession>(agent_host, client);
}

void ChromeDevToolsManagerDelegate::ClientDetached(
    content::DevToolsAgentHost* agent_host,
    content::DevToolsAgentHostClient* client) {
  sessions_.erase(client);
}

scoped_refptr<DevToolsAgentHost>
ChromeDevToolsManagerDelegate::CreateNewTarget(const GURL& url) {
  NavigateParams params(ProfileManager::GetLastUsedProfile(), url,
                        ui::PAGE_TRANSITION_AUTO_TOPLEVEL);
  params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  Navigate(&params);
  if (!params.navigated_or_inserted_contents)
    return nullptr;
  return DevToolsAgentHost::GetOrCreateFor(
      params.navigated_or_inserted_contents);
}

std::string ChromeDevToolsManagerDelegate::GetDiscoveryPageHTML() {
  return ui::ResourceBundle::GetSharedInstance()
      .GetRawDataResource(IDR_DEVTOOLS_DISCOVERY_PAGE_HTML)
      .as_string();
}

bool ChromeDevToolsManagerDelegate::HasBundledFrontendResources() {
  return true;
}

void ChromeDevToolsManagerDelegate::DevicesAvailable(
    const DevToolsDeviceDiscovery::CompleteDevices& devices) {
  DevToolsAgentHost::List remote_targets;
  for (const auto& complete : devices) {
    for (const auto& browser : complete.second->browsers()) {
      for (const auto& page : browser->pages())
        remote_targets.push_back(page->CreateTarget());
    }
  }
  remote_agent_hosts_.swap(remote_targets);
}

void ChromeDevToolsManagerDelegate::UpdateDeviceDiscovery() {
  RemoteLocations remote_locations;
  for (const auto& it : sessions_) {
    TargetHandler* target_handler = it.second->target_handler();
    if (!target_handler)
      continue;
    RemoteLocations& locations = target_handler->remote_locations();
    remote_locations.insert(locations.begin(), locations.end());
  }

  bool equals = remote_locations.size() == remote_locations_.size();
  if (equals) {
    RemoteLocations::iterator it1 = remote_locations.begin();
    RemoteLocations::iterator it2 = remote_locations_.begin();
    while (it1 != remote_locations.end()) {
      DCHECK(it2 != remote_locations_.end());
      if (!(*it1).Equals(*it2))
        equals = false;
      ++it1;
      ++it2;
    }
    DCHECK(it2 == remote_locations_.end());
  }

  if (equals)
    return;

  if (remote_locations.empty()) {
    device_discovery_.reset();
    remote_agent_hosts_.clear();
  } else {
    if (!device_manager_)
      device_manager_ = AndroidDeviceManager::Create();

    AndroidDeviceManager::DeviceProviders providers;
    providers.push_back(new TCPDeviceProvider(remote_locations));
    device_manager_->SetDeviceProviders(providers);

    device_discovery_.reset(new DevToolsDeviceDiscovery(device_manager_.get(),
        base::Bind(&ChromeDevToolsManagerDelegate::DevicesAvailable,
                   base::Unretained(this))));
  }
  remote_locations_.swap(remote_locations);
}
