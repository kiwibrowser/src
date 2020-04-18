// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_WORKER_THREAD_MESSAGE_FILTER_H_
#define CONTENT_RENDERER_WORKER_THREAD_MESSAGE_FILTER_H_

#include "base/macros.h"
#include "content/renderer/child_message_filter.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace content {

class ThreadSafeSender;

// A base class for filtering IPC messages targeted for worker threads.
class WorkerThreadMessageFilter : public ChildMessageFilter {
 public:
  WorkerThreadMessageFilter(
      ThreadSafeSender* thread_safe_sender,
      scoped_refptr<base::SingleThreadTaskRunner> main_thread_task_runner);

 protected:
  ~WorkerThreadMessageFilter() override;

  base::SingleThreadTaskRunner* main_thread_task_runner() {
    return main_thread_task_runner_.get();
  }
  ThreadSafeSender* thread_safe_sender() { return thread_safe_sender_.get(); }

 private:
  // Returns whether this filter should process |msg|.
  virtual bool ShouldHandleMessage(const IPC::Message& msg) const = 0;

  // Processes the IPC message in the worker thread, if the filter could extract
  // its thread id. Otherwise, runs in the main thread. It only receives a
  // message if ShouldHandleMessage() returns true for it.
  virtual void OnFilteredMessageReceived(const IPC::Message& msg) = 0;

  // Attempts to extract the thread-id of the worker-thread that should process
  // the IPC message. Returns whether the thread-id could be determined and set
  // in |ipc_thread_id|.
  virtual bool GetWorkerThreadIdForMessage(const IPC::Message& msg,
                                           int* ipc_thread_id) = 0;

  // ChildMessageFilter implementation:
  base::TaskRunner* OverrideTaskRunnerForMessage(const IPC::Message& msg) final;
  bool OnMessageReceived(const IPC::Message& msg) final;

  scoped_refptr<base::SingleThreadTaskRunner> main_thread_task_runner_;
  scoped_refptr<ThreadSafeSender> thread_safe_sender_;

  DISALLOW_COPY_AND_ASSIGN(WorkerThreadMessageFilter);
};

}  // namespace content

#endif  // CONTENT_RENDERER_WORKER_THREAD_MESSAGE_FILTER_H_
