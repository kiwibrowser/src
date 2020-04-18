// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_CHILD_MESSAGE_FILTER_H_
#define CONTENT_RENDERER_CHILD_MESSAGE_FILTER_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "ipc/ipc_sender.h"

namespace base {
class TaskRunner;
}

namespace IPC {
class MessageFilter;
}

namespace content {

class ThreadSafeSender;

// A base class for implementing IPC MessageFilter's that run on a different
// thread or TaskRunner than the main thread.
class ChildMessageFilter
    : public base::RefCountedThreadSafe<ChildMessageFilter>,
      public IPC::Sender {
 public:
  // IPC::Sender implementation. Can be called on any threads.
  bool Send(IPC::Message* message) override;

  // If implementers want to run OnMessageReceived on a different task
  // runner it should override this and return the TaskRunner for the message.
  // Returning NULL runs OnMessageReceived() on the current IPC thread.
  virtual base::TaskRunner* OverrideTaskRunnerForMessage(
      const IPC::Message& msg) = 0;

  // If OverrideTaskRunnerForMessage is overriden and returns non-null
  // this will be called on the returned TaskRunner.
  virtual bool OnMessageReceived(const IPC::Message& msg) = 0;

  // This method is called when WorkerTaskRunner::PostTask() returned false
  // for the target thread.  Note that there's still a little chance that
  // PostTask() returns true but OnMessageReceived() is never called on the
  // target thread.  By default this does nothing.
  virtual void OnStaleMessageReceived(const IPC::Message& msg) {}

 protected:
  ChildMessageFilter();
  ~ChildMessageFilter() override;

 private:
  class Internal;
  friend class ChildThreadImpl;
  friend class RenderThreadImpl;
  friend class WorkerThread;

  friend class base::RefCountedThreadSafe<ChildMessageFilter>;

  IPC::MessageFilter* GetFilter();

  // This implements IPC::MessageFilter to hide the actual filter methods from
  // child classes.
  Internal* internal_;

  scoped_refptr<ThreadSafeSender> thread_safe_sender_;

  DISALLOW_COPY_AND_ASSIGN(ChildMessageFilter);
};

}  // namespace content

#endif  // CONTENT_RENDERER_CHILD_MESSAGE_FILTER_H_
