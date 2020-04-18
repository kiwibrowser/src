// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ui/webui/sync_internals/sync_internals_message_handler.h"

#include <utility>
#include <vector>

#include "base/logging.h"
#include "base/values.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/sync/base/weak_handle.h"
#include "components/sync/driver/about_sync_util.h"
#include "components/sync/driver/sync_service.h"
#include "components/sync/engine/cycle/commit_counters.h"
#include "components/sync/engine/cycle/status_counters.h"
#include "components/sync/engine/cycle/update_counters.h"
#include "components/sync/engine/events/protocol_event.h"
#include "components/sync/js/js_event_details.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/sync/ios_chrome_profile_sync_service_factory.h"
#include "ios/chrome/common/channel_info.h"
#include "ios/web/public/web_thread.h"
#include "ios/web/public/webui/web_ui_ios.h"

using syncer::JsEventDetails;
using syncer::ModelTypeSet;
using syncer::WeakHandle;

SyncInternalsMessageHandler::SyncInternalsMessageHandler()
    : is_registered_(false),
      is_registered_for_counters_(false),
      weak_ptr_factory_(this) {}

SyncInternalsMessageHandler::~SyncInternalsMessageHandler() {
  if (js_controller_)
    js_controller_->RemoveJsEventHandler(this);

  syncer::SyncService* service = GetSyncService();
  if (service && service->HasObserver(this)) {
    service->RemoveObserver(this);
    service->RemoveProtocolEventObserver(this);
  }

  if (service && is_registered_for_counters_) {
    service->RemoveTypeDebugInfoObserver(this);
  }
}

void SyncInternalsMessageHandler::RegisterMessages() {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);

  web_ui()->RegisterMessageCallback(
      syncer::sync_ui_util::kRegisterForEvents,
      base::BindRepeating(&SyncInternalsMessageHandler::HandleRegisterForEvents,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      syncer::sync_ui_util::kRegisterForPerTypeCounters,
      base::BindRepeating(
          &SyncInternalsMessageHandler::HandleRegisterForPerTypeCounters,
          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      syncer::sync_ui_util::kRequestUpdatedAboutInfo,
      base::BindRepeating(
          &SyncInternalsMessageHandler::HandleRequestUpdatedAboutInfo,
          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      syncer::sync_ui_util::kRequestListOfTypes,
      base::BindRepeating(
          &SyncInternalsMessageHandler::HandleRequestListOfTypes,
          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      syncer::sync_ui_util::kGetAllNodes,
      base::BindRepeating(&SyncInternalsMessageHandler::HandleGetAllNodes,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      syncer::sync_ui_util::kSetIncludeSpecifics,
      base::BindRepeating(
          &SyncInternalsMessageHandler::HandleSetIncludeSpecifics,
          base::Unretained(this)));
}

void SyncInternalsMessageHandler::HandleRegisterForEvents(
    const base::ListValue* args) {
  DCHECK(args->empty());

  // is_registered_ flag protects us from double-registering.  This could
  // happen on a page refresh, where the JavaScript gets re-run but the
  // message handler remains unchanged.
  syncer::SyncService* service = GetSyncService();
  if (service && !is_registered_) {
    service->AddObserver(this);
    service->AddProtocolEventObserver(this);
    js_controller_ = service->GetJsController();
    js_controller_->AddJsEventHandler(this);
    is_registered_ = true;
  }
}

void SyncInternalsMessageHandler::HandleRegisterForPerTypeCounters(
    const base::ListValue* args) {
  DCHECK(args->empty());

  if (syncer::SyncService* service = GetSyncService()) {
    if (!is_registered_for_counters_) {
      service->AddTypeDebugInfoObserver(this);
      is_registered_for_counters_ = true;
    } else {
      // Re-register to ensure counters get re-emitted.
      service->RemoveTypeDebugInfoObserver(this);
      service->AddTypeDebugInfoObserver(this);
    }
  }
}

void SyncInternalsMessageHandler::HandleRequestUpdatedAboutInfo(
    const base::ListValue* args) {
  DCHECK(args->empty());
  SendAboutInfo();
}

void SyncInternalsMessageHandler::HandleRequestListOfTypes(
    const base::ListValue* args) {
  DCHECK(args->empty());
  base::DictionaryValue event_details;
  std::unique_ptr<base::ListValue> type_list(new base::ListValue());
  ModelTypeSet protocol_types = syncer::ProtocolTypes();
  for (ModelTypeSet::Iterator it = protocol_types.First(); it.Good();
       it.Inc()) {
    type_list->AppendString(ModelTypeToString(it.Get()));
  }
  event_details.Set(syncer::sync_ui_util::kTypes, std::move(type_list));
  web_ui()->CallJavascriptFunction(
      syncer::sync_ui_util::kDispatchEvent,
      base::Value(syncer::sync_ui_util::kOnReceivedListOfTypes), event_details);
}

void SyncInternalsMessageHandler::HandleGetAllNodes(
    const base::ListValue* args) {
  DCHECK_EQ(1U, args->GetSize());
  int request_id = 0;
  bool success = ExtractIntegerValue(args, &request_id);
  DCHECK(success);

  syncer::SyncService* service = GetSyncService();
  if (service) {
    service->GetAllNodes(
        base::Bind(&SyncInternalsMessageHandler::OnReceivedAllNodes,
                   weak_ptr_factory_.GetWeakPtr(), request_id));
  }
}

void SyncInternalsMessageHandler::HandleSetIncludeSpecifics(
    const base::ListValue* args) {
  DCHECK_EQ(1U, args->GetSize());
  include_specifics_ = args->GetList()[0].GetBool();
}

void SyncInternalsMessageHandler::OnReceivedAllNodes(
    int request_id,
    std::unique_ptr<base::ListValue> nodes) {
  base::Value id(request_id);
  web_ui()->CallJavascriptFunction(syncer::sync_ui_util::kGetAllNodesCallback,
                                   id, *nodes);
}

void SyncInternalsMessageHandler::OnStateChanged(syncer::SyncService* sync) {
  SendAboutInfo();
}

void SyncInternalsMessageHandler::OnProtocolEvent(
    const syncer::ProtocolEvent& event) {
  std::unique_ptr<base::DictionaryValue> value(
      syncer::ProtocolEvent::ToValue(event, include_specifics_));
  web_ui()->CallJavascriptFunction(
      syncer::sync_ui_util::kDispatchEvent,
      base::Value(syncer::sync_ui_util::kOnProtocolEvent), *value);
}

void SyncInternalsMessageHandler::OnCommitCountersUpdated(
    syncer::ModelType type,
    const syncer::CommitCounters& counters) {
  EmitCounterUpdate(type, syncer::sync_ui_util::kCommit, counters.ToValue());
}

void SyncInternalsMessageHandler::OnUpdateCountersUpdated(
    syncer::ModelType type,
    const syncer::UpdateCounters& counters) {
  EmitCounterUpdate(type, syncer::sync_ui_util::kUpdate, counters.ToValue());
}

void SyncInternalsMessageHandler::OnStatusCountersUpdated(
    syncer::ModelType type,
    const syncer::StatusCounters& counters) {
  EmitCounterUpdate(type, syncer::sync_ui_util::kStatus, counters.ToValue());
}

void SyncInternalsMessageHandler::EmitCounterUpdate(
    syncer::ModelType type,
    const std::string& counter_type,
    std::unique_ptr<base::DictionaryValue> value) {
  std::unique_ptr<base::DictionaryValue> details(new base::DictionaryValue());
  details->SetString(syncer::sync_ui_util::kModelType, ModelTypeToString(type));
  details->SetString(syncer::sync_ui_util::kCounterType, counter_type);
  details->Set(syncer::sync_ui_util::kCounters, std::move(value));
  web_ui()->CallJavascriptFunction(
      syncer::sync_ui_util::kDispatchEvent,
      base::Value(syncer::sync_ui_util::kOnCountersUpdated), *details);
}

void SyncInternalsMessageHandler::HandleJsEvent(const std::string& name,
                                                const JsEventDetails& details) {
  DVLOG(1) << "Handling event: " << name << " with details "
           << details.ToString();
  web_ui()->CallJavascriptFunction(syncer::sync_ui_util::kDispatchEvent,
                                   base::Value(name), details.Get());
}

void SyncInternalsMessageHandler::SendAboutInfo() {
  syncer::SyncService* sync_service = GetSyncService();
  std::unique_ptr<base::DictionaryValue> value =
      syncer::sync_ui_util::ConstructAboutInformation(sync_service,
                                                      GetChannel());
  web_ui()->CallJavascriptFunction(
      syncer::sync_ui_util::kDispatchEvent,
      base::Value(syncer::sync_ui_util::kOnAboutInfoUpdated), *value);
}

// Gets the SyncService of the underlying original profile. May return null.
syncer::SyncService* SyncInternalsMessageHandler::GetSyncService() {
  ios::ChromeBrowserState* browser_state =
      ios::ChromeBrowserState::FromWebUIIOS(web_ui());
  return IOSChromeProfileSyncServiceFactory::GetForBrowserState(
      browser_state->GetOriginalChromeBrowserState());
}
