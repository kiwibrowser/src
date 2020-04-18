// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/services/heap_profiling/receiver_pipe_win.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/message_loop/message_loop_current.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread.h"
#include "components/services/heap_profiling/public/cpp/sender_pipe.h"
#include "components/services/heap_profiling/receiver_pipe.h"
#include "components/services/heap_profiling/stream_receiver.h"

namespace heap_profiling {

ReceiverPipe::ReceiverPipe(mojo::edk::ScopedInternalPlatformHandle handle)
    : ReceiverPipeBase(std::move(handle)),
      read_buffer_(new char[SenderPipe::kPipeSize]) {
  ZeroOverlapped();
  base::MessageLoopCurrentForIO::Get()->RegisterIOHandler(handle_.get().handle,
                                                          this);
}

ReceiverPipe::~ReceiverPipe() {}

void ReceiverPipe::StartReadingOnIOThread() {
  ReadUntilBlocking();
}

void ReceiverPipe::ReadUntilBlocking() {
  // TODO(brettw) note that the IO completion callback will always be issued,
  // even for sync returns of ReadFile. If there is a lot of data ready to be
  // read, it would be nice to process them all in this loop rather than having
  // to go back to the message loop for each block, but that will require
  // different IOContext structures for each one.
  DWORD bytes_read = 0;
  ZeroOverlapped();

  DCHECK(!read_outstanding_);
  read_outstanding_ = this;
  if (!::ReadFile(handle_.get().handle, read_buffer_.get(),
                  SenderPipe::kPipeSize, &bytes_read, &context_.overlapped)) {
    if (GetLastError() == ERROR_IO_PENDING) {
      return;
    } else {
      if (receiver_) {
        receiver_task_runner_->PostTask(
            FROM_HERE,
            base::BindOnce(&StreamReceiver::OnStreamComplete, receiver_));
      }
      return;
    }
  }
}

void ReceiverPipe::ZeroOverlapped() {
  memset(&context_.overlapped, 0, sizeof(OVERLAPPED));
}

void ReceiverPipe::OnIOCompleted(base::MessagePumpForIO::IOContext* context,
                                 DWORD bytes_transfered,
                                 DWORD error) {
  // Note: any crashes with this on the stack are likely a result of destroying
  // a relevant class while there is I/O pending.
  DCHECK(read_outstanding_);
  // Clear |read_outstanding_| but retain the reference to keep ourself alive
  // until this function returns.
  scoped_refptr<ReceiverPipe> self(std::move(read_outstanding_));

  if (bytes_transfered && receiver_) {
    receiver_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&ReceiverPipe::OnStreamDataThunk, this,
                       base::MessageLoopCurrent::Get()->task_runner(),
                       std::move(read_buffer_),
                       static_cast<size_t>(bytes_transfered)));
    read_buffer_.reset(new char[SenderPipe::kPipeSize]);
  }
  ReadUntilBlocking();
}

}  // namespace heap_profiling
