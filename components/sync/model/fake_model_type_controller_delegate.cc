// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/model/fake_model_type_controller_delegate.h"

#include <utility>

#include "base/callback.h"
#include "components/sync/engine/cycle/status_counters.h"

namespace syncer {

FakeModelTypeControllerDelegate::FakeModelTypeControllerDelegate(ModelType type)
    : type_(type), weak_ptr_factory_(this) {}

FakeModelTypeControllerDelegate::~FakeModelTypeControllerDelegate() {}

void FakeModelTypeControllerDelegate::OnSyncStarting(
    const ModelErrorHandler& error_handler,
    StartCallback callback) {
  if (!callback.is_null()) {
    std::move(callback).Run(nullptr);
  }
}

void FakeModelTypeControllerDelegate::DisableSync() {}

void FakeModelTypeControllerDelegate::GetAllNodesForDebugging(
    ModelTypeControllerDelegate::AllNodesCallback callback) {
  std::move(callback).Run(type_, std::make_unique<base::ListValue>());
}

void FakeModelTypeControllerDelegate::GetStatusCountersForDebugging(
    StatusCountersCallback callback) {
  std::move(callback).Run(type_, StatusCounters());
}

void FakeModelTypeControllerDelegate::RecordMemoryUsageHistogram() {}

base::WeakPtr<ModelTypeControllerDelegate>
FakeModelTypeControllerDelegate::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

}  // namespace syncer
