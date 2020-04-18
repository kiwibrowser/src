// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sessions/tab_loader.h"

#include <algorithm>
#include <string>

#include "base/memory/memory_coordinator_client_registry.h"
#include "base/memory/memory_coordinator_proxy.h"
#include "base/memory/memory_pressure_monitor.h"
#include "base/metrics/histogram_macros.h"
#include "build/build_config.h"
#include "chrome/browser/sessions/session_restore_stats_collector.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/favicon/content/content_favicon_driver.h"
#include "components/variations/variations_associated_data.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_features.h"
#include "services/resource_coordinator/public/cpp/resource_coordinator_features.h"

using content::NavigationController;
using content::RenderWidgetHost;
using content::WebContents;

namespace {

size_t g_max_loaded_tab_count_for_testing = 0;

}  // namespace

void TabLoader::Observe(int type,
                        const content::NotificationSource& source,
                        const content::NotificationDetails& details) {
  switch (type) {
    case content::NOTIFICATION_WEB_CONTENTS_DESTROYED: {
      WebContents* web_contents = content::Source<WebContents>(source).ptr();
      HandleTabClosedOrLoaded(&web_contents->GetController());
      break;
    }
    case content::NOTIFICATION_LOAD_STOP: {
      DCHECK(!resource_coordinator::IsPageAlmostIdleSignalEnabled());
      NavigationController* controller =
          content::Source<NavigationController>(source).ptr();
      HandleTabClosedOrLoaded(controller);
      break;
    }
    default:
      NOTREACHED() << "Unknown notification received:" << type;
  }
  // Delete ourselves when we are done.
  if (tabs_loading_.empty() && tabs_to_load_.empty())
    this_retainer_ = nullptr;
}

void TabLoader::OnPageAlmostIdle(content::WebContents* web_contents) {
  auto* controller = &web_contents->GetController();
  // The |web_contents| is not managed by TabLoader.
  if (tabs_loading_.find(controller) == tabs_loading_.end() &&
      !base::ContainsValue(tabs_to_load_, controller)) {
    return;
  }
  HandleTabClosedOrLoaded(controller);
}

void TabLoader::SetTabLoadingEnabled(bool enable_tab_loading) {
  // TODO(chrisha): Make the SessionRestoreStatsCollector aware that tab loading
  // was explicitly stopped or restarted. This can make be used to invalidate
  // various metrics.
  if (enable_tab_loading == loading_enabled_)
    return;
  loading_enabled_ = enable_tab_loading;
  if (loading_enabled_) {
    LoadNextTab();
  } else {
    force_load_timer_.Stop();
  }
}

// static
void TabLoader::RestoreTabs(const std::vector<RestoredTab>& tabs,
                            const base::TimeTicks& restore_started) {
  if (!shared_tab_loader_)
    shared_tab_loader_ = new TabLoader(restore_started);

  shared_tab_loader_->stats_collector_->TrackTabs(tabs);
  shared_tab_loader_->StartLoading(tabs);
}

// static
void TabLoader::SetMaxLoadedTabCountForTest(size_t value) {
  g_max_loaded_tab_count_for_testing = value;
}

TabLoader::TabLoader(base::TimeTicks restore_started)
    : memory_pressure_listener_(
          base::Bind(&TabLoader::OnMemoryPressure, base::Unretained(this))),
      force_load_delay_multiplier_(1),
      loading_enabled_(true),
      started_to_load_count_(0),
      restore_started_(restore_started) {
  stats_collector_ = new SessionRestoreStatsCollector(
      restore_started,
      std::make_unique<
          SessionRestoreStatsCollector::UmaStatsReportingDelegate>());
  shared_tab_loader_ = this;
  this_retainer_ = this;
  base::MemoryCoordinatorClientRegistry::GetInstance()->Register(this);
  if (auto* page_signal_receiver =
          resource_coordinator::PageSignalReceiver::GetInstance()) {
    page_signal_receiver->AddObserver(this);
  }
}

TabLoader::~TabLoader() {
  DCHECK(tabs_loading_.empty() && tabs_to_load_.empty());
  DCHECK(shared_tab_loader_ == this);
  shared_tab_loader_ = nullptr;
  if (auto* page_signal_receiver =
          resource_coordinator::PageSignalReceiver::GetInstance()) {
    page_signal_receiver->RemoveObserver(this);
  }
  base::MemoryCoordinatorClientRegistry::GetInstance()->Unregister(this);
  SessionRestore::OnTabLoaderFinishedLoadingTabs();
}

void TabLoader::StartLoading(const std::vector<RestoredTab>& tabs) {
  // Add the tabs to the list of tabs loading/to load and register them for
  // notifications. Also, restore the favicons of the background tabs (the title
  // has already been set by now).This avoids having blank icons in case the
  // restore is halted due to memory pressure. Also, when multiple tabs are
  // restored to a single window, the title may not appear, and the user will
  // have no way of finding out which tabs corresponds to which page if the icon
  // is a generic grey one.
  for (auto& restored_tab : tabs) {
    if (!restored_tab.is_active()) {
      tabs_to_load_.push_back(&restored_tab.contents()->GetController());
      favicon::ContentFaviconDriver* favicon_driver =
          favicon::ContentFaviconDriver::FromWebContents(
              restored_tab.contents());
      // |favicon_driver| might be null when testing.
      if (favicon_driver) {
        favicon_driver->FetchFavicon(favicon_driver->GetActiveURL(),
                                     /*is_same_document=*/false);
      }
    } else {
      ++started_to_load_count_;
      tabs_loading_.insert(&restored_tab.contents()->GetController());
    }
    RegisterForNotifications(&restored_tab.contents()->GetController());
  }

  // When multiple profiles are using the same TabLoader, another profile might
  // already have started loading. In that case, the tabs scheduled for loading
  // by this profile are already in the loading queue, and they will get loaded
  // eventually.
  if (delegate_)
    return;

  // Create a TabLoaderDelegate which will allow OS specific behavior for tab
  // loading.
  if (!delegate_) {
    delegate_ = TabLoaderDelegate::Create(this);
    // There is already at least one tab loading (the active tab). As such we
    // only have to start the timeout timer here. But, don't restore background
    // tabs if the system is under memory pressure.
    if (ShouldStopLoadingTabs()) {
      StopLoadingTabs();
      return;
    }

    StartFirstTimer();
  }
}

void TabLoader::LoadNextTab() {
  // LoadNextTab should only get called after we have started the tab
  // loading.
  CHECK(delegate_);

  // Abort if loading is not enabled.
  if (!loading_enabled_)
    return;

  if (!tabs_to_load_.empty()) {
    // Check the memory pressure before restoring the next tab, and abort if
    // there is pressure. This is important on the Mac because of the sometimes
    // large delay between a memory pressure event and receiving a notification
    // of that event (in that case tab restore can trigger memory pressure but
    // will complete before the notification arrives).
    if (ShouldStopLoadingTabs()) {
      StopLoadingTabs();
      return;
    }

    stats_collector_->OnWillLoadNextTab(!force_load_timer_.IsRunning());
    NavigationController* controller = tabs_to_load_.front();
    DCHECK(controller);
    ++started_to_load_count_;
    tabs_loading_.insert(controller);
    tabs_to_load_.pop_front();
    controller->LoadIfNecessary();
    content::WebContents* contents = controller->GetWebContents();
    if (contents) {
      Browser* browser = chrome::FindBrowserWithWebContents(contents);
      if (browser &&
          browser->tab_strip_model()->GetActiveWebContents() != contents) {
        // By default tabs are marked as visible. As only the active tab is
        // visible we need to explicitly tell non-active tabs they are hidden.
        // Without this call non-active tabs are not marked as backgrounded.
        //
        // NOTE: We need to do this here rather than when the tab is added to
        // the Browser as at that time not everything has been created, so that
        // the call would do nothing.
        contents->WasHidden();
      }
    }
  }

  if (!tabs_to_load_.empty())
    StartTimer();
}

void TabLoader::StartFirstTimer() {
  force_load_timer_.Stop();
  force_load_timer_.Start(FROM_HERE,
                          delegate_->GetFirstTabLoadingTimeout(),
                          this, &TabLoader::ForceLoadTimerFired);
}

void TabLoader::StartTimer() {
  force_load_timer_.Stop();
  force_load_timer_.Start(FROM_HERE,
                          delegate_->GetTimeoutBeforeLoadingNextTab() *
                              force_load_delay_multiplier_,
                          this, &TabLoader::ForceLoadTimerFired);
}

void TabLoader::RemoveTab(NavigationController* controller) {
  registrar_.Remove(this, content::NOTIFICATION_WEB_CONTENTS_DESTROYED,
                    content::Source<WebContents>(controller->GetWebContents()));
  if (!resource_coordinator::IsPageAlmostIdleSignalEnabled()) {
    registrar_.Remove(this, content::NOTIFICATION_LOAD_STOP,
                      content::Source<NavigationController>(controller));
  }

  TabsLoading::iterator i = tabs_loading_.find(controller);
  if (i != tabs_loading_.end())
    tabs_loading_.erase(i);

  TabsToLoad::iterator j =
      find(tabs_to_load_.begin(), tabs_to_load_.end(), controller);
  if (j != tabs_to_load_.end())
    tabs_to_load_.erase(j);
}

void TabLoader::ForceLoadTimerFired() {
  force_load_delay_multiplier_ *= 2;
  LoadNextTab();
}

void TabLoader::RegisterForNotifications(NavigationController* controller) {
  registrar_.Add(this, content::NOTIFICATION_WEB_CONTENTS_DESTROYED,
                 content::Source<WebContents>(controller->GetWebContents()));
  // When page almost idle signal is enabled, we don't use onload to help start
  // loading next tab, we use page almost idle signal instead.
  if (resource_coordinator::IsPageAlmostIdleSignalEnabled())
    return;
  registrar_.Add(this, content::NOTIFICATION_LOAD_STOP,
                 content::Source<NavigationController>(controller));
}

void TabLoader::HandleTabClosedOrLoaded(NavigationController* controller) {
  RemoveTab(controller);
  if (delegate_)
    LoadNextTab();
}

bool TabLoader::ShouldStopLoadingTabs() const {
  if (g_max_loaded_tab_count_for_testing != 0 &&
      started_to_load_count_ >= g_max_loaded_tab_count_for_testing)
    return true;
  if (base::FeatureList::IsEnabled(features::kMemoryCoordinator))
    return base::MemoryCoordinatorProxy::GetInstance()->GetCurrentMemoryState()
        != base::MemoryState::NORMAL;
  if (base::MemoryPressureMonitor::Get()) {
    return base::MemoryPressureMonitor::Get()->GetCurrentPressureLevel() !=
        base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_NONE;
  }
  return false;
}

void TabLoader::OnMemoryPressure(
    base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level) {
  if (ShouldStopLoadingTabs())
    StopLoadingTabs();
}

void TabLoader::OnMemoryStateChange(base::MemoryState state) {
  switch (state) {
    case base::MemoryState::NORMAL:
      break;
    case base::MemoryState::THROTTLED:
      StopLoadingTabs();
      break;
    case base::MemoryState::SUSPENDED:
      // Note that SUSPENDED never occurs in the main browser process so far.
      // Fall through.
    case base::MemoryState::UNKNOWN:
      NOTREACHED();
      break;
  }
}

void TabLoader::StopLoadingTabs() {
  // When receiving a resource pressure level warning, we stop pre-loading more
  // tabs since we are running in danger of loading more tabs by throwing out
  // old ones.
  if (tabs_to_load_.empty())
    return;
  // Stop the timer and suppress any tab loads while we clean the list.
  SetTabLoadingEnabled(false);
  while (!tabs_to_load_.empty()) {
    NavigationController* tab = tabs_to_load_.front();
    tabs_to_load_.pop_front();
    RemoveTab(tab);

    // Notify the stats collector that a tab's loading has been deferred due to
    // memory pressure.
    stats_collector_->DeferTab(tab);
  }
  // By calling |LoadNextTab| explicitly, we make sure that the
  // |NOTIFICATION_SESSION_RESTORE_DONE| event gets sent.
  LoadNextTab();
}

// static
TabLoader* TabLoader::shared_tab_loader_ = nullptr;
