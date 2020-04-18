// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/offline_pages/test_offline_page_model_builder.h"

#include <utility>

#include "base/memory/ref_counted.h"
#include "base/path_service.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/default_clock.h"
#include "chrome/common/chrome_constants.h"
#include "components/offline_pages/core/archive_manager.h"
#include "components/offline_pages/core/model/offline_page_model_taskified.h"
#include "components/offline_pages/core/offline_page_metadata_store_sql.h"
#include "components/offline_pages/core/stub_system_download_manager.h"
#include "content/public/browser/browser_context.h"

namespace {
const int64_t kDownloadId = 42LL;
}  // namespace

namespace offline_pages {

std::unique_ptr<KeyedService> BuildTestOfflinePageModel(
    content::BrowserContext* context) {
  scoped_refptr<base::SingleThreadTaskRunner> task_runner =
      base::ThreadTaskRunnerHandle::Get();

  base::FilePath store_path =
      context->GetPath().Append(chrome::kOfflinePageMetadataDirname);
  std::unique_ptr<OfflinePageMetadataStoreSQL> metadata_store(
      new OfflinePageMetadataStoreSQL(task_runner, store_path));

  base::FilePath private_archives_dir =
      context->GetPath().Append(chrome::kOfflinePageArchivesDirname);
  base::FilePath public_archives_dir("/sdcard/Download");
  // If base::PathService::Get returns false, the temporary_archives_dir will be
  // empty, and no temporary pages will be saved during this chrome lifecycle.
  base::FilePath temporary_archives_dir;
  if (base::PathService::Get(base::DIR_CACHE, &temporary_archives_dir)) {
    temporary_archives_dir =
        temporary_archives_dir.Append(chrome::kOfflinePageArchivesDirname);
  }
  std::unique_ptr<ArchiveManager> archive_manager(
      new ArchiveManager(temporary_archives_dir, private_archives_dir,
                         public_archives_dir, task_runner));
  std::unique_ptr<SystemDownloadManager> stub_download_manager(
      new StubSystemDownloadManager(kDownloadId, true));

  return std::unique_ptr<KeyedService>(new OfflinePageModelTaskified(
      std::move(metadata_store), std::move(archive_manager),
      std::move(stub_download_manager), task_runner,
      base::DefaultClock::GetInstance()));
}

}  // namespace offline_pages
