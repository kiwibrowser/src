// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/search/suggestions/suggestions_service_factory.h"

#include <memory>
#include <utility>

#include "base/sequenced_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "base/time/default_tick_clock.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search/suggestions/image_decoder_impl.h"
#include "chrome/browser/signin/identity_manager_factory.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/image_fetcher/core/image_fetcher.h"
#include "components/image_fetcher/core/image_fetcher_impl.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/leveldb_proto/proto_database.h"
#include "components/leveldb_proto/proto_database_impl.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/suggestions/blacklist_store.h"
#include "components/suggestions/image_manager.h"
#include "components/suggestions/proto/suggestions.pb.h"
#include "components/suggestions/suggestions_service_impl.h"
#include "components/suggestions/suggestions_store.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "services/identity/public/cpp/identity_manager.h"

using content::BrowserThread;
using image_fetcher::ImageFetcherImpl;

namespace suggestions {

// static
SuggestionsService* SuggestionsServiceFactory::GetForProfile(Profile* profile) {
  return static_cast<SuggestionsService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
SuggestionsServiceFactory* SuggestionsServiceFactory::GetInstance() {
  return base::Singleton<SuggestionsServiceFactory>::get();
}

SuggestionsServiceFactory::SuggestionsServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "SuggestionsService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(IdentityManagerFactory::GetInstance());
  DependsOn(ProfileSyncServiceFactory::GetInstance());
}

SuggestionsServiceFactory::~SuggestionsServiceFactory() {}

KeyedService* SuggestionsServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  scoped_refptr<base::SequencedTaskRunner> background_task_runner =
      base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::USER_VISIBLE});

  Profile* profile = static_cast<Profile*>(context);

  identity::IdentityManager* identity_manager =
      IdentityManagerFactory::GetForProfile(profile);
  browser_sync::ProfileSyncService* sync_service =
      ProfileSyncServiceFactory::GetForProfile(profile);

  std::unique_ptr<SuggestionsStore> suggestions_store(
      new SuggestionsStore(profile->GetPrefs()));
  std::unique_ptr<BlacklistStore> blacklist_store(
      new BlacklistStore(profile->GetPrefs()));

  std::unique_ptr<leveldb_proto::ProtoDatabaseImpl<ImageData>> db(
      new leveldb_proto::ProtoDatabaseImpl<ImageData>(background_task_runner));

  base::FilePath database_dir(
      profile->GetPath().Append(FILE_PATH_LITERAL("Thumbnails")));

  std::unique_ptr<ImageFetcherImpl> image_fetcher(
      new ImageFetcherImpl(std::make_unique<suggestions::ImageDecoderImpl>(),
                           profile->GetRequestContext()));
  std::unique_ptr<ImageManager> thumbnail_manager(
      new ImageManager(std::move(image_fetcher), std::move(db), database_dir));
  return new SuggestionsServiceImpl(
      identity_manager, sync_service, profile->GetRequestContext(),
      std::move(suggestions_store), std::move(thumbnail_manager),
      std::move(blacklist_store), base::DefaultTickClock::GetInstance());
}

void SuggestionsServiceFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  SuggestionsServiceImpl::RegisterProfilePrefs(registry);
}

}  // namespace suggestions
