// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/binder/remote_object.h"

#include "base/location.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chromeos/binder/command_broker.h"

namespace binder {

RemoteObject::RemoteObject(CommandBroker* command_broker, int32_t handle)
    : release_closure_(command_broker->GetReleaseReferenceClosure(handle)),
      release_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      handle_(handle) {
  command_broker->AddReference(handle_);
}

RemoteObject::~RemoteObject() {
  if (release_task_runner_->BelongsToCurrentThread()) {
    release_closure_.Run();
  } else {
    release_task_runner_->PostTask(FROM_HERE, release_closure_);
  }
}

Object::Type RemoteObject::GetType() const {
  return TYPE_REMOTE;
}

bool RemoteObject::Transact(CommandBroker* command_broker,
                            const TransactionData& data,
                            std::unique_ptr<TransactionData>* reply) {
  return command_broker->Transact(handle_, data, reply);
}

}  // namespace binder
