// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/intent_helper/arc_navigation_throttle.h"

#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "chrome/browser/chromeos/apps/intent_helper/apps_navigation_throttle.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/arc/arc_service_manager.h"
#include "components/arc/intent_helper/arc_intent_helper_bridge.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"

namespace arc {

namespace {

// Searches for a preferred app in |app_candidates| and returns its index. If
// not found, returns |app_candidates.size()|.
size_t FindPreferredApp(
    const std::vector<mojom::IntentHandlerInfoPtr>& app_candidates,
    const GURL& url_for_logging) {
  for (size_t i = 0; i < app_candidates.size(); ++i) {
    if (!app_candidates[i]->is_preferred)
      continue;
    if (ArcIntentHelperBridge::IsIntentHelperPackage(
            app_candidates[i]->package_name)) {
      // If Chrome browser was selected as the preferred app, we shouldn't
      // create a throttle.
      DVLOG(1)
          << "Chrome browser is selected as the preferred app for this URL: "
          << url_for_logging;
    }
    return i;
  }
  return app_candidates.size();  // not found
}

}  // namespace

// static
void ArcNavigationThrottle::GetArcAppsForPicker(
    content::WebContents* web_contents,
    const GURL& url,
    chromeos::GetAppsCallback callback) {
  arc::ArcServiceManager* arc_service_manager = arc::ArcServiceManager::Get();
  if (!arc_service_manager) {
    DVLOG(1) << "Cannot get an instance of ArcServiceManager";
    std::move(callback).Run({});
    return;
  }

  auto* instance = ARC_GET_INSTANCE_FOR_METHOD(
      arc_service_manager->arc_bridge_service()->intent_helper(),
      RequestUrlHandlerList);
  if (!instance) {
    DVLOG(1) << "Cannot get access to RequestUrlHandlerList";
    std::move(callback).Run({});
    return;
  }

  // |throttle| will delete itself when it is finished.
  ArcNavigationThrottle* throttle = new ArcNavigationThrottle(web_contents);
  throttle->GetArcAppsForPicker(instance, url, std::move(callback));
}

// static
bool ArcNavigationThrottle::WillGetArcAppsForNavigation(
    content::NavigationHandle* handle,
    chromeos::AppsNavigationCallback callback) {
  ArcServiceManager* arc_service_manager = ArcServiceManager::Get();
  if (!arc_service_manager)
    return false;

  content::WebContents* web_contents = handle->GetWebContents();
  auto* intent_helper_bridge = ArcIntentHelperBridge::GetForBrowserContext(
      web_contents->GetBrowserContext());
  if (!intent_helper_bridge)
    return false;

  const GURL& url = handle->GetURL();
  if (intent_helper_bridge->ShouldChromeHandleUrl(url)) {
    // Allow navigation to proceed if there isn't an android app that handles
    // the given URL.
    return false;
  }

  mojom::IntentHelperInstance* instance = ARC_GET_INSTANCE_FOR_METHOD(
      arc_service_manager->arc_bridge_service()->intent_helper(),
      RequestUrlHandlerList);
  if (!instance)
    return false;

  // |throttle| will delete itself when it is finished.
  ArcNavigationThrottle* throttle = new ArcNavigationThrottle(web_contents);

  // Return true to defer the navigation until we asynchronously hear back from
  // ARC whether a preferred app should be launched. This makes it safe to bind
  // |handle| as a raw pointer argument. We will either resume or cancel the
  // navigation as soon as the callback is run. If the WebContents is destroyed
  // prior to this asynchronous method finishing, it is safe to not run
  // |callback| since it will not matter what we do with the deferred navigation
  // for a now-closed tab.
  throttle->GetArcAppsForNavigation(instance, url, std::move(callback));
  return true;
}

// static
bool ArcNavigationThrottle::MaybeLaunchOrPersistArcApp(
    const GURL& url,
    const std::string& package_name,
    bool should_launch,
    bool should_persist) {
  auto* arc_service_manager = arc::ArcServiceManager::Get();
  arc::mojom::IntentHelperInstance* instance = nullptr;
  if (arc_service_manager) {
    instance = ARC_GET_INSTANCE_FOR_METHOD(
        arc_service_manager->arc_bridge_service()->intent_helper(), HandleUrl);
  }

  // With no instance, or if we neither want to launch an ARC app or persist the
  // preference to the container, return early.
  if (!instance || !(should_launch || should_persist))
    return false;

  if (should_persist) {
    DCHECK(arc_service_manager);
    if (ARC_GET_INSTANCE_FOR_METHOD(
            arc_service_manager->arc_bridge_service()->intent_helper(),
            AddPreferredPackage)) {
      instance->AddPreferredPackage(package_name);
    }
  }

  if (should_launch) {
    instance->HandleUrl(url.spec(), package_name);
    return true;
  }

  return false;
}

// static
size_t ArcNavigationThrottle::GetAppIndex(
    const std::vector<mojom::IntentHandlerInfoPtr>& app_candidates,
    const std::string& selected_app_package) {
  for (size_t i = 0; i < app_candidates.size(); ++i) {
    if (app_candidates[i]->package_name == selected_app_package)
      return i;
  }
  return app_candidates.size();
}

// static
bool ArcNavigationThrottle::IsAppAvailable(
    const std::vector<mojom::IntentHandlerInfoPtr>& app_candidates) {
  return app_candidates.size() > 1 ||
         (app_candidates.size() == 1 &&
          !ArcIntentHelperBridge::IsIntentHelperPackage(
              app_candidates[0]->package_name));
}

// static
bool ArcNavigationThrottle::IsAppAvailableForTesting(
    const std::vector<mojom::IntentHandlerInfoPtr>& app_candidates) {
  return IsAppAvailable(app_candidates);
}

// static
size_t ArcNavigationThrottle::FindPreferredAppForTesting(
    const std::vector<mojom::IntentHandlerInfoPtr>& app_candidates) {
  return FindPreferredApp(app_candidates, GURL());
}

ArcNavigationThrottle::~ArcNavigationThrottle() = default;

ArcNavigationThrottle::ArcNavigationThrottle(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents), weak_ptr_factory_(this) {}

void ArcNavigationThrottle::GetArcAppsForNavigation(
    mojom::IntentHelperInstance* instance,
    const GURL& url,
    chromeos::AppsNavigationCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  instance->RequestUrlHandlerList(
      url.spec(),
      base::BindOnce(
          &ArcNavigationThrottle::OnAppCandidatesReceivedForNavigation,
          weak_ptr_factory_.GetWeakPtr(), url, std::move(callback)));
}

void ArcNavigationThrottle::GetArcAppsForPicker(
    mojom::IntentHelperInstance* instance,
    const GURL& url,
    chromeos::GetAppsCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  instance->RequestUrlHandlerList(
      url.spec(),
      base::BindOnce(&ArcNavigationThrottle::OnAppCandidatesReceivedForPicker,
                     weak_ptr_factory_.GetWeakPtr(), url, std::move(callback)));
}

void ArcNavigationThrottle::OnAppCandidatesReceivedForNavigation(
    const GURL& url,
    chromeos::AppsNavigationCallback callback,
    std::vector<mojom::IntentHandlerInfoPtr> app_candidates) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  std::unique_ptr<ArcNavigationThrottle> deleter(this);
  if (!IsAppAvailable(app_candidates)) {
    // This scenario shouldn't be accessed as ArcNavigationThrottle is created
    // iff there are ARC apps which can actually handle the given URL.
    DVLOG(1) << "There are no app candidates for this URL: " << url;
    chromeos::AppsNavigationThrottle::RecordUma(
        std::string(), chromeos::AppType::INVALID,
        chromeos::IntentPickerCloseReason::ERROR, /*should_persist=*/false);
    std::move(callback).Run(chromeos::AppsNavigationAction::RESUME, {});
    return;
  }

  // If one of the apps is marked as preferred, launch it immediately.
  chromeos::PreferredPlatform pref_platform =
      DidLaunchPreferredArcApp(url, app_candidates);

  switch (pref_platform) {
    case chromeos::PreferredPlatform::ARC:
      std::move(callback).Run(chromeos::AppsNavigationAction::CANCEL, {});
      return;
    case chromeos::PreferredPlatform::NATIVE_CHROME:
      std::move(callback).Run(chromeos::AppsNavigationAction::RESUME, {});
      return;
    case chromeos::PreferredPlatform::PWA:
      NOTREACHED();
      break;
    case chromeos::PreferredPlatform::NONE:
      break;  // Do nothing.
  }

  // We are always going to resume navigation at this point, and possibly show
  // the intent picker bubble to prompt the user to choose if they would like to
  // use an ARC app to open the URL.
  deleter.release();
  GetArcAppIcons(url, std::move(app_candidates),
                 base::BindOnce(std::move(callback),
                                chromeos::AppsNavigationAction::RESUME));
}

void ArcNavigationThrottle::OnAppCandidatesReceivedForPicker(
    const GURL& url,
    chromeos::GetAppsCallback callback,
    std::vector<arc::mojom::IntentHandlerInfoPtr> app_candidates) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  std::unique_ptr<ArcNavigationThrottle> deleter(this);
  if (!IsAppAvailable(app_candidates)) {
    DVLOG(1) << "There are no app candidates for this URL";
    std::move(callback).Run({});
    return;
  }

  deleter.release();
  GetArcAppIcons(url, std::move(app_candidates), std::move(callback));
}

chromeos::PreferredPlatform ArcNavigationThrottle::DidLaunchPreferredArcApp(
    const GURL& url,
    const std::vector<mojom::IntentHandlerInfoPtr>& app_candidates) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  chromeos::PreferredPlatform preferred_platform =
      chromeos::PreferredPlatform::NONE;
  chromeos::AppType app_type = chromeos::AppType::INVALID;
  const size_t index = FindPreferredApp(app_candidates, url);

  if (index != app_candidates.size()) {
    auto close_reason = chromeos::IntentPickerCloseReason::PREFERRED_APP_FOUND;
    const std::string& package_name = app_candidates[index]->package_name;

    // Make sure that the instance at least supports HandleUrl.
    auto* arc_service_manager = ArcServiceManager::Get();
    mojom::IntentHelperInstance* instance = nullptr;
    if (arc_service_manager) {
      instance = ARC_GET_INSTANCE_FOR_METHOD(
          arc_service_manager->arc_bridge_service()->intent_helper(),
          HandleUrl);
    }

    if (!instance) {
      close_reason = chromeos::IntentPickerCloseReason::ERROR;
    } else if (ArcIntentHelperBridge::IsIntentHelperPackage(package_name)) {
      Browser* browser = chrome::FindBrowserWithWebContents(web_contents());
      if (browser)
        browser->window()->SetIntentPickerViewVisibility(/*visible=*/true);
      preferred_platform = chromeos::PreferredPlatform::NATIVE_CHROME;
    } else {
      instance->HandleUrl(url.spec(), package_name);
      preferred_platform = chromeos::PreferredPlatform::ARC;
      app_type = chromeos::AppType::ARC;
    }
    chromeos::AppsNavigationThrottle::RecordUma(
        package_name, app_type, close_reason, /*should_persist=*/false);
  }

  return preferred_platform;
}

void ArcNavigationThrottle::GetArcAppIcons(
    const GURL& url,
    std::vector<mojom::IntentHandlerInfoPtr> app_candidates,
    chromeos::GetAppsCallback callback) {
  std::unique_ptr<ArcNavigationThrottle> deleter(this);
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  auto* intent_helper_bridge = ArcIntentHelperBridge::GetForBrowserContext(
      web_contents()->GetBrowserContext());
  if (!intent_helper_bridge) {
    LOG(ERROR) << "Cannot get an instance of ArcIntentHelperBridge";
    chromeos::AppsNavigationThrottle::RecordUma(
        std::string(), chromeos::AppType::INVALID,
        chromeos::IntentPickerCloseReason::ERROR, /*should_persist=*/false);
    std::move(callback).Run({});
    return;
  }
  std::vector<ArcIntentHelperBridge::ActivityName> activities;
  for (const auto& candidate : app_candidates)
    activities.emplace_back(candidate->package_name, candidate->activity_name);

  deleter.release();
  intent_helper_bridge->GetActivityIcons(
      activities,
      base::BindOnce(&ArcNavigationThrottle::OnAppIconsReceived,
                     weak_ptr_factory_.GetWeakPtr(), url,
                     std::move(app_candidates), std::move(callback)));
}

void ArcNavigationThrottle::OnAppIconsReceived(
    const GURL& url,
    std::vector<arc::mojom::IntentHandlerInfoPtr> app_candidates,
    chromeos::GetAppsCallback callback,
    std::unique_ptr<arc::ArcIntentHelperBridge::ActivityToIconsMap> icons) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  std::unique_ptr<ArcNavigationThrottle> deleter(this);
  std::vector<chromeos::IntentPickerAppInfo> app_info;

  for (const auto& candidate : app_candidates) {
    gfx::Image icon;
    const arc::ArcIntentHelperBridge::ActivityName activity(
        candidate->package_name, candidate->activity_name);
    const auto it = icons->find(activity);

    app_info.emplace_back(chromeos::AppType::ARC,
                          it != icons->end() ? it->second.icon16 : gfx::Image(),
                          candidate->package_name, candidate->name);
  }

  // After running the callback, |this| is always deleted by |deleter| going out
  // of scope.
  std::move(callback).Run(std::move(app_info));
}

void ArcNavigationThrottle::WebContentsDestroyed() {
  delete this;
}

}  // namespace arc
