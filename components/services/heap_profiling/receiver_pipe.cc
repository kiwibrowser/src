// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/services/heap_profiling/receiver_pipe.h"

#include "base/bind.h"
#include "base/task_runner.h"
#include "components/services/heap_profiling/stream_receiver.h"

namespace heap_profiling {

ReceiverPipeBase::ReceiverPipeBase(
    mojo::edk::ScopedInternalPlatformHandle handle)
    : handle_(std::move(handle)) {}

ReceiverPipeBase::~ReceiverPipeBase() = default;

void ReceiverPipeBase::SetReceiver(scoped_refptr<base::TaskRunner> task_runner,
                                   scoped_refptr<StreamReceiver> receiver) {
  receiver_task_runner_ = std::move(task_runner);
  receiver_ = receiver;
}

void ReceiverPipeBase::ReportError() {
  handle_.reset();
}

void ReceiverPipeBase::OnStreamDataThunk(
    scoped_refptr<base::TaskRunner> pipe_task_runner,
    std::unique_ptr<char[]> data,
    size_t size) {
  if (!receiver_->OnStreamData(std::move(data), size)) {
    pipe_task_runner->PostTask(
        FROM_HERE, base::BindOnce(&ReceiverPipeBase::ReportError, this));
  }
}

}  // namespace heap_profiling
