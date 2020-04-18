// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SERVICES_HEAP_PROFILING_RECEIVER_PIPE_POSIX_H_
#define COMPONENTS_SERVICES_HEAP_PROFILING_RECEIVER_PIPE_POSIX_H_

#include <string>

#include "base/files/platform_file.h"
#include "base/macros.h"
#include "base/message_loop/message_pump_for_io.h"
#include "build/build_config.h"
#include "components/services/heap_profiling/receiver_pipe.h"

namespace heap_profiling {

class ReceiverPipe : public ReceiverPipeBase,
                     public base::MessagePumpForIO::FdWatcher {
 public:
  explicit ReceiverPipe(mojo::edk::ScopedInternalPlatformHandle handle);

  // Must be called on the IO thread.
  void StartReadingOnIOThread();

 private:
  ~ReceiverPipe() override;

  // MessagePumpForIO::FdWatcher implementation.
  void OnFileCanReadWithoutBlocking(int fd) override;
  void OnFileCanWriteWithoutBlocking(int fd) override;

  base::MessagePumpForIO::FdWatchController controller_;
  std::unique_ptr<char[]> read_buffer_;

  DISALLOW_COPY_AND_ASSIGN(ReceiverPipe);
};

}  // namespace heap_profiling

#endif  // COMPONENTS_SERVICES_HEAP_PROFILING_RECEIVER_PIPE_POSIX_H_
