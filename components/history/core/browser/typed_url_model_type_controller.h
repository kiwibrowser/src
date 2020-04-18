// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_HISTORY_CORE_BROWSER_TYPED_URL_MODEL_TYPE_CONTROLLER_H__
#define COMPONENTS_HISTORY_CORE_BROWSER_TYPED_URL_MODEL_TYPE_CONTROLLER_H__

#include "base/task/cancelable_task_tracker.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/sync/driver/model_type_controller.h"

namespace history {

class TypedURLModelTypeController : public syncer::ModelTypeController {
 public:
  TypedURLModelTypeController(syncer::SyncClient* sync_client,
                              const char* history_disabled_pref_name);

  ~TypedURLModelTypeController() override;

  // syncer::DataTypeController implementation.
  bool ReadyForStart() const override;

 private:
  // syncer::ModelTypeController implementation.
  void PostModelTask(const base::Location& location, ModelTask task) override;

  void OnSavingBrowserHistoryDisabledChanged();

  // Name of the pref that indicates whether saving history is disabled.
  const char* history_disabled_pref_name_;

  PrefChangeRegistrar pref_registrar_;

  // Helper object to make sure we don't leave tasks running on the history
  // thread.
  base::CancelableTaskTracker task_tracker_;

  DISALLOW_COPY_AND_ASSIGN(TypedURLModelTypeController);
};

}  // namespace history

#endif  // COMPONENTS_HISTORY_CORE_BROWSER_TYPED_URL_MODEL_TYPE_CONTROLLER_H__
