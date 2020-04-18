// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_MODEL_FAKE_MODEL_TYPE_CONTROLLER_DELEGATE_H_
#define COMPONENTS_SYNC_MODEL_FAKE_MODEL_TYPE_CONTROLLER_DELEGATE_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "components/sync/base/model_type.h"
#include "components/sync/model/model_error.h"
#include "components/sync/model/model_type_controller_delegate.h"

namespace syncer {

// A ModelTypeChangeProcessor implementation for tests.
class FakeModelTypeControllerDelegate : public ModelTypeControllerDelegate {
 public:
  explicit FakeModelTypeControllerDelegate(ModelType type);
  ~FakeModelTypeControllerDelegate() override;

  // ModelTypeControllerDelegate overrides
  void OnSyncStarting(const ModelErrorHandler& error_handler,
                      StartCallback callback) override;
  void DisableSync() override;
  void GetAllNodesForDebugging(AllNodesCallback callback) override;
  void GetStatusCountersForDebugging(StatusCountersCallback callback) override;
  void RecordMemoryUsageHistogram() override;

  base::WeakPtr<ModelTypeControllerDelegate> GetWeakPtr();

 private:
  const ModelType type_;
  base::WeakPtrFactory<FakeModelTypeControllerDelegate> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(FakeModelTypeControllerDelegate);
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_MODEL_FAKE_MODEL_TYPE_CONTROLLER_DELEGATE_H_
