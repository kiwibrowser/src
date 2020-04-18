// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/engine/ui_model_worker.h"

#include <utility>

namespace syncer {

UIModelWorker::UIModelWorker(
    scoped_refptr<base::SingleThreadTaskRunner> ui_thread)
    : ui_thread_(std::move(ui_thread)) {}

ModelSafeGroup UIModelWorker::GetModelSafeGroup() {
  return GROUP_UI;
}

bool UIModelWorker::IsOnModelSequence() {
  return ui_thread_->BelongsToCurrentThread();
}

UIModelWorker::~UIModelWorker() {}

void UIModelWorker::ScheduleWork(base::OnceClosure work) {
  ui_thread_->PostTask(FROM_HERE, std::move(work));
}

}  // namespace syncer
