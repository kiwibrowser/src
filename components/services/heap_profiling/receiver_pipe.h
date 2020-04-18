// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SERVICES_HEAP_PROFILING_RECEIVER_PIPE_H_
#define COMPONENTS_SERVICES_HEAP_PROFILING_RECEIVER_PIPE_H_

#include "base/memory/ref_counted.h"
#include "build/build_config.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"

namespace base {
class TaskRunner;
}

namespace heap_profiling {

class StreamReceiver;

// Base class for the platform-specific receiver pipes. Since there is only
// ever one actual implementation of this in the system, those implementations
// are called "ReceiverPipe" and the common functions are not
// virtual. This class is just for the shared implementation.
class ReceiverPipeBase : public base::RefCountedThreadSafe<ReceiverPipeBase> {
 public:
  void SetReceiver(scoped_refptr<base::TaskRunner> task_runner,
                   scoped_refptr<StreamReceiver> receiver);

 protected:
  friend class base::RefCountedThreadSafe<ReceiverPipeBase>;

  explicit ReceiverPipeBase(mojo::edk::ScopedInternalPlatformHandle handle);
  virtual ~ReceiverPipeBase();

  // Callback that indicates an error has occurred and the connection should
  // be closed. May be called more than once in an error condition.
  void ReportError();

  // Called on the receiver task runner's thread to call the OnStreamData
  // function and post the error back to the pipe on the correct thread if one
  // occurs.
  void OnStreamDataThunk(scoped_refptr<base::TaskRunner> pipe_task_runner,
                         std::unique_ptr<char[]> data,
                         size_t size);

  scoped_refptr<base::TaskRunner> receiver_task_runner_;
  scoped_refptr<StreamReceiver> receiver_;

  mojo::edk::ScopedInternalPlatformHandle handle_;
};

}  // namespace heap_profiling

// Define the platform-specific specialization.
#if defined(OS_WIN)
#include "components/services/heap_profiling/receiver_pipe_win.h"
#else
#include "components/services/heap_profiling/receiver_pipe_posix.h"
#endif

#endif  // COMPONENTS_SERVICES_HEAP_PROFILING_RECEIVER_PIPE_H_
