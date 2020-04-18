// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/sync/ios_user_event_service_factory.h"

#include <memory>
#include <utility>

#include "base/memory/singleton.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "components/sync/base/model_type.h"
#include "components/sync/base/report_unrecoverable_error.h"
#include "components/sync/model_impl/client_tag_based_model_type_processor.h"
#include "components/sync/user_events/no_op_user_event_service.h"
#include "components/sync/user_events/user_event_service_impl.h"
#include "components/sync/user_events/user_event_sync_bridge.h"
#include "ios/chrome/browser/browser_state/browser_state_otr_helper.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/sync/ios_chrome_profile_sync_service_factory.h"
#include "ios/chrome/common/channel_info.h"
#include "ios/web/public/browser_state.h"

// static
syncer::UserEventService* IOSUserEventServiceFactory::GetForBrowserState(
    ios::ChromeBrowserState* browser_state) {
  return static_cast<syncer::UserEventService*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
IOSUserEventServiceFactory* IOSUserEventServiceFactory::GetInstance() {
  return base::Singleton<IOSUserEventServiceFactory>::get();
}

IOSUserEventServiceFactory::IOSUserEventServiceFactory()
    : BrowserStateKeyedServiceFactory(
          "UserEventService",
          BrowserStateDependencyManager::GetInstance()) {}

IOSUserEventServiceFactory::~IOSUserEventServiceFactory() {}

std::unique_ptr<KeyedService>
IOSUserEventServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* browser_state) const {
  syncer::SyncService* sync_service =
      IOSChromeProfileSyncServiceFactory::GetForBrowserState(
          ios::ChromeBrowserState::FromBrowserState(browser_state));
  if (!syncer::UserEventServiceImpl::MightRecordEvents(
          browser_state->IsOffTheRecord(), sync_service)) {
    return std::make_unique<syncer::NoOpUserEventService>();
  }

  syncer::OnceModelTypeStoreFactory store_factory =
      browser_sync::ProfileSyncService::GetModelTypeStoreFactory(
          browser_state->GetStatePath());
  auto bridge = std::make_unique<syncer::UserEventSyncBridge>(
      std::move(store_factory),
      std::make_unique<syncer::ClientTagBasedModelTypeProcessor>(
          syncer::USER_EVENTS, /*dump_stack=*/base::BindRepeating(
              &syncer::ReportUnrecoverableError, ::GetChannel())),
      sync_service->GetGlobalIdMapper(), sync_service);
  return std::make_unique<syncer::UserEventServiceImpl>(sync_service,
                                                        std::move(bridge));
}

web::BrowserState* IOSUserEventServiceFactory::GetBrowserStateToUse(
    web::BrowserState* context) const {
  return GetBrowserStateOwnInstanceInIncognito(context);
}
