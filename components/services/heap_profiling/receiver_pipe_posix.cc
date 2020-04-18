// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/services/heap_profiling/receiver_pipe_posix.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/message_loop/message_loop_current.h"
#include "base/posix/eintr_wrapper.h"
#include "base/threading/thread.h"
#include "build/build_config.h"
#include "components/services/heap_profiling/public/cpp/sender_pipe.h"
#include "components/services/heap_profiling/receiver_pipe.h"
#include "components/services/heap_profiling/stream_receiver.h"
#include "mojo/edk/embedder/platform_channel_utils_posix.h"
#include "mojo/edk/embedder/platform_handle.h"

namespace heap_profiling {

ReceiverPipe::ReceiverPipe(mojo::edk::ScopedInternalPlatformHandle handle)
    : ReceiverPipeBase(std::move(handle)),
      controller_(FROM_HERE),
      read_buffer_(new char[SenderPipe::kPipeSize]) {}

ReceiverPipe::~ReceiverPipe() {}

void ReceiverPipe::StartReadingOnIOThread() {
  base::MessageLoopCurrentForIO::Get()->WatchFileDescriptor(
      handle_.get().handle, true, base::MessagePumpForIO::WATCH_READ,
      &controller_, this);
  OnFileCanReadWithoutBlocking(handle_.get().handle);
}

void ReceiverPipe::OnFileCanReadWithoutBlocking(int fd) {
  ssize_t bytes_read = 0;
  do {
    base::circular_deque<mojo::edk::InternalPlatformHandle> dummy_for_receive;

    bytes_read = HANDLE_EINTR(
        read(handle_.get().handle, read_buffer_.get(), SenderPipe::kPipeSize));
    if (bytes_read > 0) {
      receiver_task_runner_->PostTask(
          FROM_HERE,
          base::BindOnce(&ReceiverPipe::OnStreamDataThunk, this,
                         base::MessageLoopCurrent::Get()->task_runner(),
                         std::move(read_buffer_),
                         static_cast<size_t>(bytes_read)));
      read_buffer_.reset(new char[SenderPipe::kPipeSize]);
      return;
    } else if (bytes_read == 0) {
      // Other end closed the pipe.
      controller_.StopWatchingFileDescriptor();
      DCHECK(receiver_task_runner_);
      receiver_task_runner_->PostTask(
          FROM_HERE,
          base::BindOnce(&StreamReceiver::OnStreamComplete, receiver_));
      return;
    } else {
      if (errno != EAGAIN && errno != EWOULDBLOCK) {
        controller_.StopWatchingFileDescriptor();
        PLOG(ERROR) << "Problem reading socket.";
        DCHECK(receiver_task_runner_);
        receiver_task_runner_->PostTask(
            FROM_HERE,
            base::BindOnce(&StreamReceiver::OnStreamComplete, receiver_));
      }
    }
  } while (bytes_read > 0);
}

void ReceiverPipe::OnFileCanWriteWithoutBlocking(int fd) {
  NOTREACHED();
}

}  // namespace heap_profiling
