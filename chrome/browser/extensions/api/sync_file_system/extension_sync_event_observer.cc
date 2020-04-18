// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/sync_file_system/extension_sync_event_observer.h"

#include <utility>

#include "base/lazy_instance.h"
#include "chrome/browser/extensions/api/sync_file_system/sync_file_system_api_helpers.h"
#include "chrome/browser/sync_file_system/sync_event_observer.h"
#include "chrome/browser/sync_file_system/sync_file_system_service.h"
#include "chrome/browser/sync_file_system/sync_file_system_service_factory.h"
#include "chrome/browser/sync_file_system/syncable_file_system_util.h"
#include "chrome/common/extensions/api/sync_file_system.h"
#include "content/public/browser/browser_context.h"
#include "extensions/browser/event_router.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system_provider.h"
#include "extensions/browser/extensions_browser_client.h"
#include "extensions/common/extension_set.h"
#include "storage/browser/fileapi/file_system_url.h"
#include "storage/common/fileapi/file_system_util.h"

using sync_file_system::SyncEventObserver;

namespace extensions {

static base::LazyInstance<
    BrowserContextKeyedAPIFactory<ExtensionSyncEventObserver>>::DestructorAtExit
    g_extension_sync_event_observer_factory = LAZY_INSTANCE_INITIALIZER;

// static
BrowserContextKeyedAPIFactory<ExtensionSyncEventObserver>*
ExtensionSyncEventObserver::GetFactoryInstance() {
  return g_extension_sync_event_observer_factory.Pointer();
}

ExtensionSyncEventObserver::ExtensionSyncEventObserver(
    content::BrowserContext* context)
    : browser_context_(context), sync_service_(NULL) {}

void ExtensionSyncEventObserver::InitializeForService(
    sync_file_system::SyncFileSystemService* sync_service) {
  DCHECK(sync_service);
  if (sync_service_ != NULL) {
    DCHECK_EQ(sync_service_, sync_service);
    return;
  }
  sync_service_ = sync_service;
  sync_service_->AddSyncEventObserver(this);
}

ExtensionSyncEventObserver::~ExtensionSyncEventObserver() {}

void ExtensionSyncEventObserver::Shutdown() {
  if (sync_service_ != NULL)
    sync_service_->RemoveSyncEventObserver(this);
}

std::string ExtensionSyncEventObserver::GetExtensionId(
    const GURL& app_origin) {
  const Extension* app = ExtensionRegistry::Get(browser_context_)
      ->enabled_extensions().GetAppByURL(app_origin);
  if (!app) {
    // The app is uninstalled or disabled.
    return std::string();
  }
  return app->id();
}

void ExtensionSyncEventObserver::OnSyncStateUpdated(
    const GURL& app_origin,
    sync_file_system::SyncServiceState state,
    const std::string& description) {
  // Convert state and description into SyncState Object.
  api::sync_file_system::ServiceInfo service_info;
  service_info.state = SyncServiceStateToExtensionEnum(state);
  service_info.description = description;
  std::unique_ptr<base::ListValue> params(
      api::sync_file_system::OnServiceStatusChanged::Create(service_info));

  BroadcastOrDispatchEvent(
      app_origin, events::SYNC_FILE_SYSTEM_ON_SERVICE_STATUS_CHANGED,
      api::sync_file_system::OnServiceStatusChanged::kEventName,
      std::move(params));
}

void ExtensionSyncEventObserver::OnFileSynced(
    const storage::FileSystemURL& url,
    sync_file_system::SyncFileType file_type,
    sync_file_system::SyncFileStatus status,
    sync_file_system::SyncAction action,
    sync_file_system::SyncDirection direction) {
  std::unique_ptr<base::ListValue> params(new base::ListValue());

  std::unique_ptr<base::DictionaryValue> entry =
      CreateDictionaryValueForFileSystemEntry(url, file_type);
  if (!entry)
    return;
  params->Append(std::move(entry));

  // Status, SyncAction and any optional notes to go here.
  api::sync_file_system::FileStatus status_enum =
      SyncFileStatusToExtensionEnum(status);
  api::sync_file_system::SyncAction action_enum =
      SyncActionToExtensionEnum(action);
  api::sync_file_system::SyncDirection direction_enum =
      SyncDirectionToExtensionEnum(direction);
  params->AppendString(api::sync_file_system::ToString(status_enum));
  params->AppendString(api::sync_file_system::ToString(action_enum));
  params->AppendString(api::sync_file_system::ToString(direction_enum));

  BroadcastOrDispatchEvent(
      url.origin(), events::SYNC_FILE_SYSTEM_ON_FILE_STATUS_CHANGED,
      api::sync_file_system::OnFileStatusChanged::kEventName,
      std::move(params));
}

void ExtensionSyncEventObserver::BroadcastOrDispatchEvent(
    const GURL& app_origin,
    events::HistogramValue histogram_value,
    const std::string& event_name,
    std::unique_ptr<base::ListValue> values) {
  // Check to see whether the event should be broadcasted to all listening
  // extensions or sent to a specific extension ID.
  bool broadcast_mode = app_origin.is_empty();
  EventRouter* event_router = EventRouter::Get(browser_context_);
  DCHECK(event_router);

  auto event = std::make_unique<Event>(histogram_value, event_name,
                                       std::move(values), browser_context_);

  // No app_origin, broadcast to all listening extensions for this event name.
  if (broadcast_mode) {
    event_router->BroadcastEvent(std::move(event));
    return;
  }

  // Dispatch to single extension ID.
  const std::string extension_id = GetExtensionId(app_origin);
  if (extension_id.empty())
    return;
  event_router->DispatchEventToExtension(extension_id, std::move(event));
}

template <>
void BrowserContextKeyedAPIFactory<
    ExtensionSyncEventObserver>::DeclareFactoryDependencies() {
  DependsOn(sync_file_system::SyncFileSystemServiceFactory::GetInstance());
  DependsOn(ExtensionsBrowserClient::Get()->GetExtensionSystemFactory());
}

}  // namespace extensions
