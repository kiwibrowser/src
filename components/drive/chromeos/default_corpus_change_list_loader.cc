// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/chromeos/default_corpus_change_list_loader.h"

#include <memory>

#include "components/drive/chromeos/about_resource_root_folder_id_loader.h"
#include "components/drive/file_system_core_util.h"

namespace drive {
namespace internal {

DefaultCorpusChangeListLoader::DefaultCorpusChangeListLoader(
    EventLogger* logger,
    base::SequencedTaskRunner* blocking_task_runner,
    ResourceMetadata* resource_metadata,
    JobScheduler* scheduler,
    AboutResourceLoader* about_resource_loader,
    LoaderController* apply_task_controller)
    : logger_(logger),
      blocking_task_runner_(blocking_task_runner),
      resource_metadata_(resource_metadata),
      scheduler_(scheduler),
      loader_controller_(apply_task_controller) {
  root_folder_id_loader_ =
      std::make_unique<AboutResourceRootFolderIdLoader>(about_resource_loader);

  start_page_token_loader_ = std::make_unique<StartPageTokenLoader>(
      util::kTeamDriveIdDefaultCorpus, scheduler_);

  change_list_loader_ = std::make_unique<ChangeListLoader>(
      logger_, blocking_task_runner_.get(), resource_metadata_, scheduler_,
      root_folder_id_loader_.get(), start_page_token_loader_.get(),
      loader_controller_);

  directory_loader_ = std::make_unique<DirectoryLoader>(
      logger_, blocking_task_runner_.get(), resource_metadata_, scheduler_,
      root_folder_id_loader_.get(), start_page_token_loader_.get(),
      loader_controller_);
}

DefaultCorpusChangeListLoader::~DefaultCorpusChangeListLoader() = default;

void DefaultCorpusChangeListLoader::AddObserver(
    ChangeListLoaderObserver* observer) {
  change_list_loader_->AddObserver(observer);
  directory_loader_->AddObserver(observer);
}

void DefaultCorpusChangeListLoader::RemoveObserver(
    ChangeListLoaderObserver* observer) {
  change_list_loader_->RemoveObserver(observer);
  directory_loader_->RemoveObserver(observer);
}

bool DefaultCorpusChangeListLoader::IsRefreshing() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  return change_list_loader_->IsRefreshing();
}

void DefaultCorpusChangeListLoader::LoadIfNeeded(
    const FileOperationCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  change_list_loader_->LoadIfNeeded(callback);
}

void DefaultCorpusChangeListLoader::ReadDirectory(
    const base::FilePath& directory_path,
    const ReadDirectoryEntriesCallback& entries_callback,
    const FileOperationCallback& completion_callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  directory_loader_->ReadDirectory(directory_path, entries_callback,
                                   completion_callback);

  // Also start loading all of the user's contents.
  change_list_loader_->LoadIfNeeded(base::DoNothing());
}

void DefaultCorpusChangeListLoader::CheckForUpdates(
    const FileOperationCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  change_list_loader_->CheckForUpdates(callback);
}

}  // namespace internal
}  // namespace drive
