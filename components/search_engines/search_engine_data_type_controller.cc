// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/search_engines/search_engine_data_type_controller.h"

#include "base/threading/thread_task_runner_handle.h"

namespace browser_sync {

SearchEngineDataTypeController::SearchEngineDataTypeController(
    const base::Closure& dump_stack,
    syncer::SyncClient* sync_client,
    TemplateURLService* template_url_service)
    : AsyncDirectoryTypeController(syncer::SEARCH_ENGINES,
                                   dump_stack,
                                   sync_client,
                                   syncer::GROUP_UI,
                                   base::ThreadTaskRunnerHandle::Get()),
      template_url_service_(template_url_service) {}

TemplateURLService::Subscription*
SearchEngineDataTypeController::GetSubscriptionForTesting() {
  return template_url_subscription_.get();
}

SearchEngineDataTypeController::~SearchEngineDataTypeController() {}

// We want to start the TemplateURLService before we begin associating.
bool SearchEngineDataTypeController::StartModels() {
  DCHECK(CalledOnValidThread());
  // If the TemplateURLService is loaded, continue with association. We force
  // a load here to prevent the rest of Sync from waiting on
  // TemplateURLService's lazy load.
  DCHECK(template_url_service_);
  template_url_service_->Load();
  if (template_url_service_->loaded()) {
    return true;  // Continue to Associate().
  }

  // Register a callback and continue when the TemplateURLService is loaded.
  template_url_subscription_ = template_url_service_->RegisterOnLoadedCallback(
      base::Bind(&SearchEngineDataTypeController::OnTemplateURLServiceLoaded,
                 base::AsWeakPtr(this)));

  return false;  // Don't continue Start.
}

void SearchEngineDataTypeController::StopModels() {
  DCHECK(CalledOnValidThread());
  template_url_subscription_.reset();
}

void SearchEngineDataTypeController::OnTemplateURLServiceLoaded() {
  DCHECK(CalledOnValidThread());
  DCHECK_EQ(MODEL_STARTING, state());
  template_url_subscription_.reset();
  OnModelLoaded();
}

}  // namespace browser_sync
