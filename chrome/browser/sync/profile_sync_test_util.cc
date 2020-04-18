// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/profile_sync_test_util.h"

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/identity_manager_factory.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/sync/chrome_sync_client.h"
#include "chrome/common/channel_info.h"
#include "chrome/test/base/testing_profile.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/browser_sync/profile_sync_test_util.h"
#include "components/signin/core/browser/signin_manager.h"
#include "components/sync/driver/signin_manager_wrapper.h"
#include "components/sync/driver/startup_controller.h"
#include "components/sync/driver/sync_api_component_factory_mock.h"
#include "components/sync/model/model_type_store_test_util.h"

using browser_sync::ProfileSyncService;

ProfileSyncService::InitParams CreateProfileSyncServiceParamsForTest(
    Profile* profile) {
  auto sync_client = std::make_unique<browser_sync::ChromeSyncClient>(profile);

  sync_client->SetSyncApiComponentFactoryForTesting(
      std::make_unique<syncer::SyncApiComponentFactoryMock>());

  ProfileSyncService::InitParams init_params =
      CreateProfileSyncServiceParamsForTest(std::move(sync_client), profile);

  return init_params;
}

ProfileSyncService::InitParams CreateProfileSyncServiceParamsForTest(
    std::unique_ptr<syncer::SyncClient> sync_client,
    Profile* profile) {
  ProfileSyncService::InitParams init_params;

  init_params.signin_wrapper = std::make_unique<SigninManagerWrapper>(
      IdentityManagerFactory::GetForProfile(profile),
      SigninManagerFactory::GetForProfile(profile));
  init_params.signin_scoped_device_id_callback =
      base::BindRepeating([]() { return std::string(); });
  init_params.oauth2_token_service =
      ProfileOAuth2TokenServiceFactory::GetForProfile(profile);
  init_params.start_behavior = ProfileSyncService::MANUAL_START;
  init_params.sync_client = std::move(sync_client);
  init_params.network_time_update_callback = base::DoNothing();
  init_params.base_directory = profile->GetPath();
  init_params.url_request_context = profile->GetRequestContext();
  init_params.debug_identifier = profile->GetDebugName();
  init_params.channel = chrome::GetChannel();
  init_params.model_type_store_factory =
      syncer::ModelTypeStoreTestUtil::FactoryForInMemoryStoreForTest();

  return init_params;
}

std::unique_ptr<TestingProfile> MakeSignedInTestingProfile() {
  auto profile = std::make_unique<TestingProfile>();
  SigninManagerFactory::GetForProfile(profile.get())
      ->SetAuthenticatedAccountInfo("12345", "foo");
  return profile;
}

std::unique_ptr<KeyedService> BuildMockProfileSyncService(
    content::BrowserContext* context) {
  return std::make_unique<browser_sync::ProfileSyncServiceMock>(
      CreateProfileSyncServiceParamsForTest(
          Profile::FromBrowserContext(context)));
}
