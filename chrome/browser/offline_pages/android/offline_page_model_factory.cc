// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/offline_pages/offline_page_model_factory.h"

#include <utility>

#include "base/files/file_path.h"
#include "base/memory/singleton.h"
#include "base/path_service.h"
#include "base/sequenced_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "base/time/default_clock.h"
#include "chrome/browser/download/download_prefs.h"
#include "chrome/browser/offline_pages/android/cct_origin_observer.h"
#include "chrome/browser/offline_pages/android/offline_pages_download_manager_bridge.h"
#include "chrome/browser/offline_pages/download_archive_manager.h"
#include "chrome/browser/offline_pages/fresh_offline_content_observer.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_constants.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/offline_pages/core/model/offline_page_model_taskified.h"
#include "components/offline_pages/core/offline_page_metadata_store_sql.h"

namespace offline_pages {

OfflinePageModelFactory::OfflinePageModelFactory()
    : BrowserContextKeyedServiceFactory(
          "OfflinePageModel",
          BrowserContextDependencyManager::GetInstance()) {}

// static
OfflinePageModelFactory* OfflinePageModelFactory::GetInstance() {
  return base::Singleton<OfflinePageModelFactory>::get();
}

// static
OfflinePageModel* OfflinePageModelFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<OfflinePageModel*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

KeyedService* OfflinePageModelFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  scoped_refptr<base::SequencedTaskRunner> background_task_runner =
      base::CreateSequencedTaskRunnerWithTraits({base::MayBlock()});

  base::FilePath store_path =
      profile->GetPath().Append(chrome::kOfflinePageMetadataDirname);
  std::unique_ptr<OfflinePageMetadataStoreSQL> metadata_store(
      new OfflinePageMetadataStoreSQL(background_task_runner, store_path));

  base::FilePath persistent_archives_dir =
      profile->GetPath().Append(chrome::kOfflinePageArchivesDirname);
  // If base::PathService::Get returns false, the temporary_archives_dir will be
  // empty, and no temporary pages will be saved during this chrome lifecycle.
  base::FilePath temporary_archives_dir;
  if (base::PathService::Get(base::DIR_CACHE, &temporary_archives_dir)) {
    temporary_archives_dir =
        temporary_archives_dir.Append(chrome::kOfflinePageArchivesDirname);
  }
  std::unique_ptr<ArchiveManager> archive_manager(new DownloadArchiveManager(
      temporary_archives_dir, persistent_archives_dir,
      DownloadPrefs::GetDefaultDownloadDirectory(), background_task_runner,
      profile));
  auto clock = std::make_unique<base::DefaultClock>();

  std::unique_ptr<SystemDownloadManager> download_manager(
      new android::OfflinePagesDownloadManagerBridge());

  OfflinePageModelTaskified* model = new OfflinePageModelTaskified(
      std::move(metadata_store), std::move(archive_manager),
      std::move(download_manager), background_task_runner,
      base::DefaultClock::GetInstance());

  CctOriginObserver::AttachToOfflinePageModel(model);

  FreshOfflineContentObserver::AttachToOfflinePageModel(model);

  return model;
}

}  // namespace offline_pages
