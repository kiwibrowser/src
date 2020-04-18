/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/modules/quota/worker_navigator_storage_quota.h"

#include "third_party/blink/renderer/modules/quota/deprecated_storage_quota.h"
#include "third_party/blink/renderer/modules/quota/storage_manager.h"

namespace blink {

WorkerNavigatorStorageQuota::WorkerNavigatorStorageQuota() = default;

const char WorkerNavigatorStorageQuota::kSupplementName[] =
    "WorkerNavigatorStorageQuota";

WorkerNavigatorStorageQuota& WorkerNavigatorStorageQuota::From(
    WorkerNavigator& navigator) {
  WorkerNavigatorStorageQuota* supplement =
      Supplement<WorkerNavigator>::From<WorkerNavigatorStorageQuota>(navigator);
  if (!supplement) {
    supplement = new WorkerNavigatorStorageQuota();
    ProvideTo(navigator, supplement);
  }
  return *supplement;
}

StorageManager* WorkerNavigatorStorageQuota::storage(
    WorkerNavigator& navigator) {
  return WorkerNavigatorStorageQuota::From(navigator).storage();
}

StorageManager* WorkerNavigatorStorageQuota::storage() const {
  if (!storage_manager_)
    storage_manager_ = new StorageManager();
  return storage_manager_.Get();
}

void WorkerNavigatorStorageQuota::Trace(blink::Visitor* visitor) {
  visitor->Trace(storage_manager_);
  Supplement<WorkerNavigator>::Trace(visitor);
}

}  // namespace blink
