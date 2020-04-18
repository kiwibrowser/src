// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/worker_thread_message_filter.h"

#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/child/thread_safe_sender.h"
#include "content/renderer/worker_thread_registry.h"
#include "ipc/ipc_message_macros.h"

namespace content {

WorkerThreadMessageFilter::WorkerThreadMessageFilter(
    ThreadSafeSender* thread_safe_sender,
    scoped_refptr<base::SingleThreadTaskRunner> main_thread_task_runner)
    : main_thread_task_runner_(std::move(main_thread_task_runner)),
      thread_safe_sender_(thread_safe_sender) {}

WorkerThreadMessageFilter::~WorkerThreadMessageFilter() {
}

base::TaskRunner* WorkerThreadMessageFilter::OverrideTaskRunnerForMessage(
    const IPC::Message& msg) {
  if (!ShouldHandleMessage(msg))
    return nullptr;
  int ipc_thread_id = 0;
  const bool success = GetWorkerThreadIdForMessage(msg, &ipc_thread_id);
  DCHECK(success);
  if (!ipc_thread_id)
    return main_thread_task_runner_.get();
  return WorkerThreadRegistry::Instance()->GetTaskRunnerFor(ipc_thread_id);
}

bool WorkerThreadMessageFilter::OnMessageReceived(const IPC::Message& msg) {
  if (!ShouldHandleMessage(msg))
    return false;
  // If the IPC message is received in a worker thread, but it has already been
  // stopped, then drop the message.
  if (!main_thread_task_runner_->BelongsToCurrentThread() &&
      !WorkerThread::GetCurrentId()) {
    return false;
  }
  OnFilteredMessageReceived(msg);
  return true;
}

}  // namespace content
