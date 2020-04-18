// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/prefs/persistent_pref_store.h"

#include <utility>

#include "base/threading/sequenced_task_runner_handle.h"

void PersistentPrefStore::CommitPendingWrite(base::OnceClosure done_callback) {
  // Default behavior for PersistentPrefStore implementation that don't issue
  // disk operations: schedule the callback immediately.
  if (done_callback) {
    base::SequencedTaskRunnerHandle::Get()->PostTask(FROM_HERE,
                                                     std::move(done_callback));
  }
}
