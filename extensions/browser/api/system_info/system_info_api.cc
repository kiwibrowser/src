// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/system_info/system_info_api.h"

#include <stdint.h>

#include <memory>
#include <set>
#include <utility>

#include "base/bind.h"
#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/memory/singleton.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "components/storage_monitor/removable_storage_observer.h"
#include "components/storage_monitor/storage_info.h"
#include "components/storage_monitor/storage_monitor.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/browser/api/system_storage/storage_info_provider.h"
#include "extensions/browser/extensions_browser_client.h"
#include "extensions/common/api/system_display.h"
#include "extensions/common/api/system_storage.h"
#include "ui/display/display_observer.h"
#include "ui/display/screen.h"

namespace extensions {

using api::system_storage::StorageUnitInfo;
using content::BrowserThread;
using storage_monitor::StorageMonitor;

namespace system_display = api::system_display;
namespace system_storage = api::system_storage;

namespace {

bool IsDisplayChangedEvent(const std::string& event_name) {
  return event_name == system_display::OnDisplayChanged::kEventName;
}

bool IsSystemStorageEvent(const std::string& event_name) {
  return (event_name == system_storage::OnAttached::kEventName ||
          event_name == system_storage::OnDetached::kEventName);
}

// Event router for systemInfo API. It is a singleton instance shared by
// multiple profiles.
class SystemInfoEventRouter : public display::DisplayObserver,
                              public storage_monitor::RemovableStorageObserver {
 public:
  static SystemInfoEventRouter* GetInstance();

  SystemInfoEventRouter();
  ~SystemInfoEventRouter() override;

  // Add/remove event listener for the |event_name| event.
  void AddEventListener(const std::string& event_name);
  void RemoveEventListener(const std::string& event_name);

 private:
  // display::DisplayObserver:
  void OnDisplayAdded(const display::Display& new_display) override;
  void OnDisplayRemoved(const display::Display& old_display) override;
  void OnDisplayMetricsChanged(const display::Display& display,
                               uint32_t metrics) override;

  // RemovableStorageObserver implementation.
  void OnRemovableStorageAttached(
      const storage_monitor::StorageInfo& info) override;
  void OnRemovableStorageDetached(
      const storage_monitor::StorageInfo& info) override;

  // Called from any thread to dispatch the systemInfo event to all extension
  // processes cross multiple profiles.
  void DispatchEvent(events::HistogramValue histogram_value,
                     const std::string& event_name,
                     std::unique_ptr<base::ListValue> args);

  // Called to dispatch the systemInfo.display.onDisplayChanged event.
  void OnDisplayChanged();

  // Used to record the event names being watched.
  std::multiset<std::string> watching_event_set_;

  bool has_storage_monitor_observer_;

  DISALLOW_COPY_AND_ASSIGN(SystemInfoEventRouter);
};

static base::LazyInstance<SystemInfoEventRouter>::Leaky
    g_system_info_event_router = LAZY_INSTANCE_INITIALIZER;

// static
SystemInfoEventRouter* SystemInfoEventRouter::GetInstance() {
  return g_system_info_event_router.Pointer();
}

SystemInfoEventRouter::SystemInfoEventRouter()
    : has_storage_monitor_observer_(false) {
}

SystemInfoEventRouter::~SystemInfoEventRouter() {
  if (has_storage_monitor_observer_) {
    StorageMonitor* storage_monitor = StorageMonitor::GetInstance();
    if (storage_monitor)
      storage_monitor->RemoveObserver(this);
  }
}

void SystemInfoEventRouter::AddEventListener(const std::string& event_name) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  watching_event_set_.insert(event_name);
  if (watching_event_set_.count(event_name) > 1)
    return;

  if (IsDisplayChangedEvent(event_name)) {
    display::Screen* screen = display::Screen::GetScreen();
    if (screen)
      screen->AddObserver(this);
  }

  if (IsSystemStorageEvent(event_name)) {
    if (!has_storage_monitor_observer_) {
      has_storage_monitor_observer_ = true;
      DCHECK(StorageMonitor::GetInstance()->IsInitialized());
      StorageMonitor::GetInstance()->AddObserver(this);
    }
  }
}

void SystemInfoEventRouter::RemoveEventListener(const std::string& event_name) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  std::multiset<std::string>::iterator it =
      watching_event_set_.find(event_name);
  if (it != watching_event_set_.end()) {
    watching_event_set_.erase(it);
    if (watching_event_set_.count(event_name) > 0)
      return;
  }

  if (IsDisplayChangedEvent(event_name)) {
    display::Screen* screen = display::Screen::GetScreen();
    if (screen)
      screen->RemoveObserver(this);
  }

  if (IsSystemStorageEvent(event_name)) {
    const std::string& other_event_name =
        (event_name == system_storage::OnDetached::kEventName)
            ? system_storage::OnAttached::kEventName
            : system_storage::OnDetached::kEventName;
    if (watching_event_set_.count(other_event_name) == 0) {
      StorageMonitor::GetInstance()->RemoveObserver(this);
      has_storage_monitor_observer_ = false;
    }
  }
}

void SystemInfoEventRouter::OnRemovableStorageAttached(
    const storage_monitor::StorageInfo& info) {
  StorageUnitInfo unit;
  systeminfo::BuildStorageUnitInfo(info, &unit);
  std::unique_ptr<base::ListValue> args(new base::ListValue);
  args->Append(unit.ToValue());
  DispatchEvent(events::SYSTEM_STORAGE_ON_ATTACHED,
                system_storage::OnAttached::kEventName, std::move(args));
}

void SystemInfoEventRouter::OnRemovableStorageDetached(
    const storage_monitor::StorageInfo& info) {
  std::unique_ptr<base::ListValue> args(new base::ListValue);
  std::string transient_id =
      StorageMonitor::GetInstance()->GetTransientIdForDeviceId(
          info.device_id());
  args->AppendString(transient_id);

  DispatchEvent(events::SYSTEM_STORAGE_ON_DETACHED,
                system_storage::OnDetached::kEventName, std::move(args));
}

void SystemInfoEventRouter::OnDisplayAdded(
    const display::Display& new_display) {
  OnDisplayChanged();
}

void SystemInfoEventRouter::OnDisplayRemoved(
    const display::Display& old_display) {
  OnDisplayChanged();
}

void SystemInfoEventRouter::OnDisplayMetricsChanged(
    const display::Display& display,
    uint32_t metrics) {
  OnDisplayChanged();
}

void SystemInfoEventRouter::OnDisplayChanged() {
  std::unique_ptr<base::ListValue> args(new base::ListValue());
  DispatchEvent(events::SYSTEM_DISPLAY_ON_DISPLAY_CHANGED,
                system_display::OnDisplayChanged::kEventName, std::move(args));
}

void SystemInfoEventRouter::DispatchEvent(
    events::HistogramValue histogram_value,
    const std::string& event_name,
    std::unique_ptr<base::ListValue> args) {
  ExtensionsBrowserClient::Get()->BroadcastEventToRenderers(
      histogram_value, event_name, std::move(args));
}

void AddEventListener(const std::string& event_name) {
  SystemInfoEventRouter::GetInstance()->AddEventListener(event_name);
}

void RemoveEventListener(const std::string& event_name) {
  SystemInfoEventRouter::GetInstance()->RemoveEventListener(event_name);
}

}  // namespace

static base::LazyInstance<
    BrowserContextKeyedAPIFactory<SystemInfoAPI>>::DestructorAtExit g_factory =
    LAZY_INSTANCE_INITIALIZER;

// static
BrowserContextKeyedAPIFactory<SystemInfoAPI>*
SystemInfoAPI::GetFactoryInstance() {
  return g_factory.Pointer();
}

SystemInfoAPI::SystemInfoAPI(content::BrowserContext* context)
    : browser_context_(context) {
  EventRouter* router = EventRouter::Get(browser_context_);
  router->RegisterObserver(this, system_storage::OnAttached::kEventName);
  router->RegisterObserver(this, system_storage::OnDetached::kEventName);
  router->RegisterObserver(this, system_display::OnDisplayChanged::kEventName);
}

SystemInfoAPI::~SystemInfoAPI() {
}

void SystemInfoAPI::Shutdown() {
  EventRouter::Get(browser_context_)->UnregisterObserver(this);
}

void SystemInfoAPI::OnListenerAdded(const EventListenerInfo& details) {
  if (IsSystemStorageEvent(details.event_name)) {
    StorageMonitor::GetInstance()->EnsureInitialized(
        base::Bind(&AddEventListener, details.event_name));
  } else {
    AddEventListener(details.event_name);
  }
}

void SystemInfoAPI::OnListenerRemoved(const EventListenerInfo& details) {
  if (IsSystemStorageEvent(details.event_name)) {
    StorageMonitor::GetInstance()->EnsureInitialized(
        base::Bind(&RemoveEventListener, details.event_name));
  } else {
    RemoveEventListener(details.event_name);
  }
}

}  // namespace extensions
