// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/devtools/protocol/target_handler.h"

#include "chrome/browser/devtools/chrome_devtools_manager_delegate.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "content/public/browser/devtools_agent_host.h"

namespace {

class DevToolsBrowserContextManager : public BrowserListObserver {
 public:
  DevToolsBrowserContextManager();
  protocol::Response CreateBrowserContext(std::string* out_context_id);
  protocol::Response GetBrowserContexts(
      std::unique_ptr<protocol::Array<protocol::String>>* browser_context_ids);
  Profile* GetBrowserContext(const std::string& context_id);
  void DisposeBrowserContext(
      const std::string& context_id,
      std::unique_ptr<TargetHandler::DisposeBrowserContextCallback> callback);

 private:
  void OnOriginalProfileDestroyed(Profile* profile);

  void OnBrowserRemoved(Browser* browser) override;

  base::flat_map<
      std::string,
      std::unique_ptr<IndependentOTRProfileManager::OTRProfileRegistration>>
      registrations_;
  base::flat_map<std::string,
                 std::unique_ptr<TargetHandler::DisposeBrowserContextCallback>>
      pending_context_disposals_;

  base::WeakPtrFactory<DevToolsBrowserContextManager> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(DevToolsBrowserContextManager);
};

DevToolsBrowserContextManager::DevToolsBrowserContextManager()
    : weak_factory_(this) {}

Profile* DevToolsBrowserContextManager::GetBrowserContext(
    const std::string& context_id) {
  auto it = registrations_.find(context_id);
  return it == registrations_.end() ? nullptr : it->second->profile();
}

protocol::Response DevToolsBrowserContextManager::CreateBrowserContext(
    std::string* out_context_id) {
  Profile* original_profile =
      ProfileManager::GetActiveUserProfile()->GetOriginalProfile();

  auto registration =
      IndependentOTRProfileManager::GetInstance()->CreateFromOriginalProfile(
          original_profile,
          base::BindOnce(
              &DevToolsBrowserContextManager::OnOriginalProfileDestroyed,
              weak_factory_.GetWeakPtr()));
  *out_context_id = registration->profile()->UniqueId();
  registrations_[*out_context_id] = std::move(registration);
  return protocol::Response::OK();
}

protocol::Response DevToolsBrowserContextManager::GetBrowserContexts(
    std::unique_ptr<protocol::Array<protocol::String>>* browser_context_ids) {
  *browser_context_ids = std::make_unique<protocol::Array<protocol::String>>();
  for (const auto& registration_pair : registrations_) {
    (*browser_context_ids)
        ->addItem(registration_pair.second->profile()->UniqueId());
  }
  return protocol::Response::OK();
}

void DevToolsBrowserContextManager::DisposeBrowserContext(
    const std::string& context_id,
    std::unique_ptr<TargetHandler::DisposeBrowserContextCallback> callback) {
  if (pending_context_disposals_.find(context_id) !=
      pending_context_disposals_.end()) {
    callback->sendFailure(protocol::Response::Error(
        "Disposal of browser context " + context_id + " is already pending"));
    return;
  }
  auto it = registrations_.find(context_id);
  if (it == registrations_.end()) {
    callback->sendFailure(protocol::Response::InvalidParams(
        "Failed to find browser context with id " + context_id));
    return;
  }

  Profile* profile = it->second->profile();
  bool has_opened_browser = false;
  for (auto* opened_browser : *BrowserList::GetInstance()) {
    if (opened_browser->profile() == profile) {
      has_opened_browser = true;
      break;
    }
  }

  // If no browsers are opened - dispose right away.
  if (!has_opened_browser) {
    registrations_.erase(it);
    callback->sendSuccess();
    return;
  }

  if (pending_context_disposals_.empty())
    BrowserList::AddObserver(this);

  pending_context_disposals_[context_id] = std::move(callback);
  BrowserList::CloseAllBrowsersWithIncognitoProfile(
      profile, base::DoNothing(), base::DoNothing(),
      true /* skip_beforeunload */);
}

void DevToolsBrowserContextManager::OnOriginalProfileDestroyed(
    Profile* profile) {
  base::EraseIf(registrations_, [&profile](const auto& it) {
    return it.second->profile()->GetOriginalProfile() == profile;
  });
}

void DevToolsBrowserContextManager::OnBrowserRemoved(Browser* browser) {
  std::string context_id = browser->profile()->UniqueId();
  auto pending_disposal = pending_context_disposals_.find(context_id);
  if (pending_disposal == pending_context_disposals_.end())
    return;
  for (auto* opened_browser : *BrowserList::GetInstance()) {
    if (opened_browser->profile() == browser->profile())
      return;
  }
  auto it = registrations_.find(context_id);
  // We cannot delete immediately here: the profile might still be referenced
  // during the browser tier-down process.
  base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE,
                                                  it->second.release());
  registrations_.erase(it);
  pending_disposal->second->sendSuccess();
  pending_context_disposals_.erase(pending_disposal);
  if (pending_context_disposals_.empty())
    BrowserList::RemoveObserver(this);
}

base::LazyInstance<DevToolsBrowserContextManager>::Leaky
    g_devtools_browser_context_manager;

}  // namespace

TargetHandler::TargetHandler(protocol::UberDispatcher* dispatcher) {
  protocol::Target::Dispatcher::wire(dispatcher, this);
}

TargetHandler::~TargetHandler() {
  ChromeDevToolsManagerDelegate* delegate =
      ChromeDevToolsManagerDelegate::GetInstance();
  if (delegate)
    delegate->UpdateDeviceDiscovery();
}

protocol::Response TargetHandler::SetRemoteLocations(
    std::unique_ptr<protocol::Array<protocol::Target::RemoteLocation>>
        locations) {
  remote_locations_.clear();
  if (!locations)
    return protocol::Response::OK();

  for (size_t i = 0; i < locations->length(); ++i) {
    auto* item = locations->get(i);
    remote_locations_.insert(
        net::HostPortPair(item->GetHost(), item->GetPort()));
  }

  ChromeDevToolsManagerDelegate* delegate =
      ChromeDevToolsManagerDelegate::GetInstance();
  if (delegate)
    delegate->UpdateDeviceDiscovery();
  return protocol::Response::OK();
}

protocol::Response TargetHandler::CreateTarget(
    const std::string& url,
    protocol::Maybe<int> width,
    protocol::Maybe<int> height,
    protocol::Maybe<std::string> browser_context_id,
    protocol::Maybe<bool> enable_begin_frame_control,
    std::string* out_target_id) {
  Profile* profile = ProfileManager::GetActiveUserProfile();
  if (browser_context_id.isJust()) {
    std::string profile_id = browser_context_id.fromJust();
    profile =
        g_devtools_browser_context_manager.Get().GetBrowserContext(profile_id);
    if (!profile) {
      return protocol::Response::Error(
          "Failed to find browser context with id " + profile_id);
    }
  }

  NavigateParams params(profile, GURL(url), ui::PAGE_TRANSITION_AUTO_TOPLEVEL);
  Browser* target_browser = nullptr;
  // Find a browser to open a new tab.
  // We shouldn't use browser that is scheduled to close.
  for (auto* browser : *BrowserList::GetInstance()) {
    if (browser->profile() == profile &&
        !browser->IsAttemptingToCloseBrowser()) {
      target_browser = browser;
      break;
    }
  }
  if (target_browser) {
    params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
    params.browser = target_browser;
  } else {
    params.disposition = WindowOpenDisposition::NEW_WINDOW;
  }

  Navigate(&params);
  if (!params.navigated_or_inserted_contents)
    return protocol::Response::Error("Failed to open a new tab");

  *out_target_id = content::DevToolsAgentHost::GetOrCreateFor(
                       params.navigated_or_inserted_contents)
                       ->GetId();
  return protocol::Response::OK();
}

protocol::Response TargetHandler::CreateBrowserContext(
    std::string* out_context_id) {
  return g_devtools_browser_context_manager.Get().CreateBrowserContext(
      out_context_id);
}

protocol::Response TargetHandler::GetBrowserContexts(
    std::unique_ptr<protocol::Array<protocol::String>>* browser_context_ids) {
  return g_devtools_browser_context_manager.Get().GetBrowserContexts(
      browser_context_ids);
}

void TargetHandler::DisposeBrowserContext(
    const std::string& context_id,
    std::unique_ptr<DisposeBrowserContextCallback> callback) {
  g_devtools_browser_context_manager.Get().DisposeBrowserContext(
      context_id, std::move(callback));
}

