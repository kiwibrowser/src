// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/driver/model_type_controller.h"

#include <utility>

#include "base/bind.h"
#include "base/location.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "components/sync/base/bind_to_task_runner.h"
#include "components/sync/base/data_type_histogram.h"
#include "components/sync/driver/sync_client.h"
#include "components/sync/engine/activation_context.h"
#include "components/sync/engine/model_type_configurer.h"
#include "components/sync/model/data_type_error_handler_impl.h"
#include "components/sync/model/model_type_change_processor.h"
#include "components/sync/model/sync_merge_result.h"

namespace syncer {

using DelegateProvider = ModelTypeController::DelegateProvider;
using ModelTask = ModelTypeController::ModelTask;

namespace {

void OnSyncStartingHelperOnModelThread(
    const ModelErrorHandler& error_handler,
    ModelTypeControllerDelegate::StartCallback callback_bound_to_ui_thread,
    base::WeakPtr<ModelTypeControllerDelegate> delegate) {
  delegate->OnSyncStarting(std::move(error_handler),
                           std::move(callback_bound_to_ui_thread));
}

void GetAllNodesForDebuggingHelperOnModelThread(
    const DataTypeController::AllNodesCallback& callback_bound_to_ui_thread,
    base::WeakPtr<ModelTypeControllerDelegate> delegate) {
  delegate->GetAllNodesForDebugging(callback_bound_to_ui_thread);
}

void GetStatusCountersForDebuggingHelperOnModelThread(
    const DataTypeController::StatusCountersCallback&
        callback_bound_to_ui_thread,
    base::WeakPtr<ModelTypeControllerDelegate> delegate) {
  delegate->GetStatusCountersForDebugging(callback_bound_to_ui_thread);
}

void RecordMemoryUsageHistogramHelperOnModelThread(
    base::WeakPtr<ModelTypeControllerDelegate> delegate) {
  delegate->RecordMemoryUsageHistogram();
}

void DisableSyncHelperOnModelThread(
    base::WeakPtr<ModelTypeControllerDelegate> delegate) {
  delegate->DisableSync();
}

void ReportError(ModelType model_type,
                 scoped_refptr<base::SequencedTaskRunner> ui_thread,
                 const ModelErrorHandler& error_handler,
                 const ModelError& error) {
  // TODO(wychen): enum uma should be strongly typed. crbug.com/661401
  UMA_HISTOGRAM_ENUMERATION("Sync.DataTypeRunFailures",
                            ModelTypeToHistogramInt(model_type),
                            static_cast<int>(MODEL_TYPE_COUNT));
  ui_thread->PostTask(error.location(), base::Bind(error_handler, error));
}

// This function allows us to return a Callback using Bind that returns the
// given |arg|. This function itself does nothing.
base::WeakPtr<ModelTypeControllerDelegate> ReturnCapturedDelegate(
    base::WeakPtr<ModelTypeControllerDelegate> arg) {
  return arg;
}

void RunModelTask(DelegateProvider delegate_provider, ModelTask task) {
  base::WeakPtr<ModelTypeControllerDelegate> delegate =
      std::move(delegate_provider).Run();
  if (delegate.get())
    std::move(task).Run(delegate);
}

}  // namespace

ModelTypeController::ModelTypeController(
    ModelType type,
    SyncClient* sync_client,
    const scoped_refptr<base::SingleThreadTaskRunner>& model_thread)
    : DataTypeController(type),
      sync_client_(sync_client),
      model_thread_(model_thread),
      sync_prefs_(sync_client->GetPrefService()),
      state_(NOT_RUNNING) {}

ModelTypeController::~ModelTypeController() {}

bool ModelTypeController::ShouldLoadModelBeforeConfigure() const {
  // USS datatypes require loading models because model controls storage where
  // data type context and progress marker are persisted.
  return true;
}

void ModelTypeController::LoadModels(
    const ModelLoadCallback& model_load_callback) {
  DCHECK(CalledOnValidThread());
  DCHECK(!model_load_callback.is_null());
  model_load_callback_ = model_load_callback;

  if (state() != NOT_RUNNING) {
    LoadModelsDone(RUNTIME_ERROR,
                   SyncError(FROM_HERE, SyncError::DATATYPE_ERROR,
                             "Model already running", type()));
    return;
  }

  state_ = MODEL_STARTING;

  // Callback that posts back to the UI thread.
  ModelTypeControllerDelegate::StartCallback callback_bound_to_ui_thread =
      BindToCurrentSequence(base::BindOnce(
          &ModelTypeController::OnProcessorStarted, base::AsWeakPtr(this)));

  ModelErrorHandler error_handler = base::BindRepeating(
      &ReportError, type(), base::SequencedTaskRunnerHandle::Get(),
      base::Bind(&ModelTypeController::ReportModelError,
                 base::AsWeakPtr(this)));

  // Start the type processor on the model thread.
  PostModelTask(FROM_HERE,
                base::BindOnce(&OnSyncStartingHelperOnModelThread,
                               std::move(error_handler),
                               std::move(callback_bound_to_ui_thread)));
}

void ModelTypeController::BeforeLoadModels(ModelTypeConfigurer* configurer) {}

void ModelTypeController::LoadModelsDone(ConfigureResult result,
                                         const SyncError& error) {
  DCHECK(CalledOnValidThread());

  if (state_ == NOT_RUNNING) {
    // The callback arrived on the UI thread after the type has been already
    // stopped.
    RecordStartFailure(ABORTED);
    return;
  }

  if (IsSuccessfulResult(result)) {
    DCHECK_EQ(MODEL_STARTING, state_);
    state_ = MODEL_LOADED;
  } else {
    RecordStartFailure(result);
  }

  if (!model_load_callback_.is_null()) {
    model_load_callback_.Run(type(), error);
  }
}

void ModelTypeController::OnProcessorStarted(
    std::unique_ptr<ActivationContext> activation_context) {
  DCHECK(CalledOnValidThread());
  // Hold on to the activation context until ActivateDataType is called.
  if (state_ == MODEL_STARTING) {
    activation_context_ = std::move(activation_context);
  }
  LoadModelsDone(OK, SyncError());
}

void ModelTypeController::RegisterWithBackend(
    base::Callback<void(bool)> set_downloaded,
    ModelTypeConfigurer* configurer) {
  DCHECK(CalledOnValidThread());
  if (activated_)
    return;
  DCHECK(configurer);
  DCHECK(activation_context_);
  DCHECK_EQ(MODEL_LOADED, state_);
  // Inform the DataTypeManager whether our initial download is complete.
  set_downloaded.Run(activation_context_->model_type_state.initial_sync_done());
  // Pass activation context to ModelTypeRegistry, where ModelTypeWorker gets
  // created and connected with ModelTypeProcessor.
  configurer->ActivateNonBlockingDataType(type(),
                                          std::move(activation_context_));
  activated_ = true;
}

void ModelTypeController::StartAssociating(
    const StartCallback& start_callback) {
  DCHECK(CalledOnValidThread());
  DCHECK(!start_callback.is_null());
  DCHECK_EQ(MODEL_LOADED, state_);

  state_ = RUNNING;

  // There is no association, just call back promptly.
  SyncMergeResult merge_result(type());
  start_callback.Run(OK, merge_result, merge_result);
}

void ModelTypeController::ActivateDataType(ModelTypeConfigurer* configurer) {
  DCHECK(CalledOnValidThread());
  DCHECK(configurer);
  DCHECK_EQ(RUNNING, state_);
  // In contrast with directory datatypes, non-blocking data types should be
  // activated in RegisterWithBackend. activation_context_ should be passed
  // to backend before call to ActivateDataType.
  DCHECK(!activation_context_);
}

void ModelTypeController::DeactivateDataType(ModelTypeConfigurer* configurer) {
  DCHECK(CalledOnValidThread());
  DCHECK(configurer);
  if (activated_) {
    configurer->DeactivateNonBlockingDataType(type());
    activated_ = false;
  }
}

void ModelTypeController::Stop() {
  DCHECK(CalledOnValidThread());

  if (state() == NOT_RUNNING)
    return;

  // Check preferences if datatype is not in preferred datatypes. Only call
  // DisableSync if the delegate is ready to handle it (controller is in loaded
  // state).
  ModelTypeSet preferred_types =
      sync_prefs_.GetPreferredDataTypes(ModelTypeSet(type()));
  if ((state() == MODEL_LOADED || state() == RUNNING) &&
      (!sync_prefs_.IsFirstSetupComplete() || !preferred_types.Has(type()))) {
    PostModelTask(FROM_HERE, base::BindOnce(&DisableSyncHelperOnModelThread));
  }

  state_ = NOT_RUNNING;
}

DataTypeController::State ModelTypeController::state() const {
  return state_;
}

void ModelTypeController::GetAllNodes(const AllNodesCallback& callback) {
  PostModelTask(FROM_HERE,
                base::BindOnce(&GetAllNodesForDebuggingHelperOnModelThread,
                               BindToCurrentSequence(callback)));
}

void ModelTypeController::GetStatusCounters(
    const StatusCountersCallback& callback) {
  PostModelTask(
      FROM_HERE,
      base::BindOnce(&GetStatusCountersForDebuggingHelperOnModelThread,
                     BindToCurrentSequence(callback)));
}

void ModelTypeController::RecordMemoryUsageHistogram() {
  PostModelTask(FROM_HERE,
                base::BindOnce(&RecordMemoryUsageHistogramHelperOnModelThread));
}

void ModelTypeController::ReportModelError(const ModelError& error) {
  DCHECK(CalledOnValidThread());
  LoadModelsDone(UNRECOVERABLE_ERROR,
                 SyncError(error.location(), SyncError::DATATYPE_ERROR,
                           error.message(), type()));
}

void ModelTypeController::RecordStartFailure(ConfigureResult result) const {
  DCHECK(CalledOnValidThread());
  // TODO(wychen): enum uma should be strongly typed. crbug.com/661401
  UMA_HISTOGRAM_ENUMERATION("Sync.DataTypeStartFailures",
                            ModelTypeToHistogramInt(type()),
                            static_cast<int>(MODEL_TYPE_COUNT));
#define PER_DATA_TYPE_MACRO(type_str)                                    \
  UMA_HISTOGRAM_ENUMERATION("Sync." type_str "ConfigureFailure", result, \
                            MAX_CONFIGURE_RESULT);
  SYNC_DATA_TYPE_HISTOGRAM(type());
#undef PER_DATA_TYPE_MACRO
}

DelegateProvider ModelTypeController::GetDelegateProvider() {
  // Get the delegate eagerly, and capture the weak pointer.
  base::WeakPtr<ModelTypeControllerDelegate> delegate =
      sync_client_->GetControllerDelegateForModelType(type());
  return base::Bind(&ReturnCapturedDelegate, delegate);
}

void ModelTypeController::PostModelTask(const base::Location& location,
                                        ModelTask task) {
  DCHECK(model_thread_);
  model_thread_->PostTask(
      location,
      base::BindOnce(&RunModelTask, GetDelegateProvider(), std::move(task)));
}

}  // namespace syncer
