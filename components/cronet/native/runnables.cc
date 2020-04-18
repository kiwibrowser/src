// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cronet/native/runnables.h"

#include <utility>

namespace cronet {

OnceClosureRunnable::OnceClosureRunnable(base::OnceClosure task)
    : task_(std::move(task)) {}

OnceClosureRunnable::~OnceClosureRunnable() = default;

void OnceClosureRunnable::Run() {
  std::move(task_).Run();
  // Runnable destroys itself after execution.

  // TODO(https://crbug.com/812334):
  // If an executor (which is implemented by a client)
  // decides to stop processing runnables, the runnables will never
  // be deleted causing a memory leak.
  // This issue should be addressed.
  delete this;
}

}  // namespace cronet
