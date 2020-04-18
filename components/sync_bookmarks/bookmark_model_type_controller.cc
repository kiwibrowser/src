// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync_bookmarks/bookmark_model_type_controller.h"

#include <utility>

#include "base/threading/thread_task_runner_handle.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/history/core/browser/history_service.h"
#include "components/sync/driver/sync_client.h"
#include "components/sync/driver/sync_service.h"
#include "components/sync/engine/model_type_configurer.h"
#include "components/sync/engine/model_type_processor_proxy.h"
#include "components/sync/model/sync_error.h"
#include "components/sync/protocol/model_type_state.pb.h"
#include "components/sync/protocol/sync.pb.h"
#include "components/sync/syncable/directory.h"
#include "components/sync/syncable/user_share.h"
#include "components/sync_bookmarks/bookmark_model_type_processor.h"

using syncer::SyncError;

namespace sync_bookmarks {

BookmarkModelTypeController::BookmarkModelTypeController(
    syncer::SyncClient* sync_client)
    : DataTypeController(syncer::BOOKMARKS),
      sync_client_(sync_client),
      state_(NOT_RUNNING) {}

BookmarkModelTypeController::~BookmarkModelTypeController() = default;

bool BookmarkModelTypeController::ShouldLoadModelBeforeConfigure() const {
  DCHECK(CalledOnValidThread());
  return true;
}

void BookmarkModelTypeController::BeforeLoadModels(
    syncer::ModelTypeConfigurer* configurer) {
  DCHECK(CalledOnValidThread());
}

void BookmarkModelTypeController::LoadModels(
    const ModelLoadCallback& model_load_callback) {
  DCHECK(CalledOnValidThread());
  if (state() != NOT_RUNNING) {
    model_load_callback.Run(type(),
                            SyncError(FROM_HERE, SyncError::DATATYPE_ERROR,
                                      "Model already running", type()));
    return;
  }

  state_ = MODEL_STARTING;

  if (DependenciesLoaded()) {
    state_ = MODEL_LOADED;
    model_load_callback.Run(type(), SyncError());
  } else {
    // TODO(pavely): Subscribe for BookmarkModel and HistoryService
    // notifications.
    NOTIMPLEMENTED();
  }
}

void BookmarkModelTypeController::RegisterWithBackend(
    base::Callback<void(bool)> set_downloaded,
    syncer::ModelTypeConfigurer* configurer) {
  DCHECK(CalledOnValidThread());
  if (activated_)
    return;
  DCHECK(configurer);
  std::unique_ptr<syncer::ActivationContext> activation_context =
      PrepareActivationContext();
  set_downloaded.Run(activation_context->model_type_state.initial_sync_done());
  configurer->ActivateNonBlockingDataType(type(),
                                          std::move(activation_context));
  activated_ = true;
}

void BookmarkModelTypeController::StartAssociating(
    const StartCallback& start_callback) {
  DCHECK(CalledOnValidThread());
  DCHECK(!start_callback.is_null());
  DCHECK_EQ(MODEL_LOADED, state_);

  state_ = RUNNING;

  // There is no association, just call back promptly.
  syncer::SyncMergeResult merge_result(type());
  start_callback.Run(OK, merge_result, merge_result);
}

void BookmarkModelTypeController::ActivateDataType(
    syncer::ModelTypeConfigurer* configurer) {
  DCHECK(CalledOnValidThread());
  DCHECK(configurer);
  DCHECK_EQ(RUNNING, state_);
}

void BookmarkModelTypeController::DeactivateDataType(
    syncer::ModelTypeConfigurer* configurer) {
  DCHECK(CalledOnValidThread());
  if (activated_) {
    configurer->DeactivateNonBlockingDataType(type());
    activated_ = false;
  }
}

void BookmarkModelTypeController::Stop() {
  DCHECK(CalledOnValidThread());
  NOTIMPLEMENTED();
}

syncer::DataTypeController::State BookmarkModelTypeController::state() const {
  DCHECK(CalledOnValidThread());
  return state_;
}

void BookmarkModelTypeController::GetAllNodes(
    const AllNodesCallback& callback) {
  DCHECK(CalledOnValidThread());
  NOTIMPLEMENTED();
}

void BookmarkModelTypeController::GetStatusCounters(
    const StatusCountersCallback& callback) {
  DCHECK(CalledOnValidThread());
  NOTIMPLEMENTED();
}

void BookmarkModelTypeController::RecordMemoryUsageHistogram() {
  DCHECK(CalledOnValidThread());
  NOTIMPLEMENTED();
}

bool BookmarkModelTypeController::DependenciesLoaded() {
  DCHECK(CalledOnValidThread());
  bookmarks::BookmarkModel* bookmark_model = sync_client_->GetBookmarkModel();
  if (!bookmark_model || !bookmark_model->loaded())
    return false;

  history::HistoryService* history_service = sync_client_->GetHistoryService();
  if (!history_service || !history_service->BackendLoaded())
    return false;

  return true;
}

std::unique_ptr<syncer::ActivationContext>
BookmarkModelTypeController::PrepareActivationContext() {
  DCHECK(!model_type_processor_);

  syncer::UserShare* user_share =
      sync_client_->GetSyncService()->GetUserShare();
  syncer::syncable::Directory* directory = user_share->directory.get();

  std::unique_ptr<syncer::ActivationContext> activation_context =
      std::make_unique<syncer::ActivationContext>();

  directory->GetDownloadProgress(
      type(), activation_context->model_type_state.mutable_progress_marker());
  activation_context->model_type_state.set_initial_sync_done(
      directory->InitialSyncEndedForType(type()));
  // TODO(pavely): Populate model_type_state.type_context.

  model_type_processor_ =
      std::make_unique<BookmarkModelTypeProcessor>(sync_client_);
  activation_context->type_processor =
      std::make_unique<syncer::ModelTypeProcessorProxy>(
          model_type_processor_->GetWeakPtr(),
          base::ThreadTaskRunnerHandle::Get());
  return activation_context;
}

}  // namespace sync_bookmarks
