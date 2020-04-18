// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/system/channel.h"

#include <stdint.h>
#include <windows.h>

#include <algorithm>
#include <limits>
#include <memory>

#include "base/bind.h"
#include "base/containers/queue.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop_current.h"
#include "base/message_loop/message_pump_for_io.h"
#include "base/synchronization/lock.h"
#include "base/task_runner.h"
#include "base/win/win_util.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"

namespace mojo {
namespace edk {

namespace {

class ChannelWin : public Channel,
                   public base::MessageLoopCurrent::DestructionObserver,
                   public base::MessagePumpForIO::IOHandler {
 public:
  ChannelWin(Delegate* delegate,
             ScopedInternalPlatformHandle handle,
             scoped_refptr<base::TaskRunner> io_task_runner)
      : Channel(delegate),
        self_(this),
        handle_(std::move(handle)),
        io_task_runner_(io_task_runner) {
    CHECK(handle_.is_valid());
  }

  void Start() override {
    io_task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&ChannelWin::StartOnIOThread, this));
  }

  void ShutDownImpl() override {
    // Always shut down asynchronously when called through the public interface.
    io_task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&ChannelWin::ShutDownOnIOThread, this));
  }

  void Write(MessagePtr message) override {
    bool write_error = false;
    {
      base::AutoLock lock(write_lock_);
      if (reject_writes_)
        return;

      bool write_now = !delay_writes_ && outgoing_messages_.empty();
      outgoing_messages_.emplace_back(std::move(message));
      if (write_now && !WriteNoLock(outgoing_messages_.front()))
        reject_writes_ = write_error = true;
    }
    if (write_error) {
      // Do not synchronously invoke OnWriteError(). Write() may have been
      // called by the delegate and we don't want to re-enter it.
      io_task_runner_->PostTask(FROM_HERE,
                                base::BindOnce(&ChannelWin::OnWriteError, this,
                                               Error::kDisconnected));
    }
  }

  void LeakHandle() override {
    DCHECK(io_task_runner_->RunsTasksInCurrentSequence());
    leak_handle_ = true;
  }

  bool GetReadInternalPlatformHandles(
      size_t num_handles,
      const void* extra_header,
      size_t extra_header_size,
      std::vector<ScopedInternalPlatformHandle>* handles) override {
    DCHECK(extra_header);
    if (num_handles > std::numeric_limits<uint16_t>::max())
      return false;
    using HandleEntry = Channel::Message::HandleEntry;
    size_t handles_size = sizeof(HandleEntry) * num_handles;
    if (handles_size > extra_header_size)
      return false;
    handles->reserve(num_handles);
    const HandleEntry* extra_header_handles =
        reinterpret_cast<const HandleEntry*>(extra_header);
    for (size_t i = 0; i < num_handles; i++) {
      handles->emplace_back(ScopedInternalPlatformHandle(InternalPlatformHandle(
          base::win::Uint32ToHandle(extra_header_handles[i].handle))));
    }
    return true;
  }

 private:
  // May run on any thread.
  ~ChannelWin() override {}

  void StartOnIOThread() {
    base::MessageLoopCurrent::Get()->AddDestructionObserver(this);
    base::MessageLoopCurrentForIO::Get()->RegisterIOHandler(
        handle_.get().handle, this);

    if (handle_.get().needs_connection) {
      BOOL ok = ConnectNamedPipe(handle_.get().handle,
                                 &connect_context_.overlapped);
      if (ok) {
        PLOG(ERROR) << "Unexpected success while waiting for pipe connection";
        OnError(Error::kConnectionFailed);
        return;
      }

      const DWORD err = GetLastError();
      switch (err) {
        case ERROR_PIPE_CONNECTED:
          break;
        case ERROR_IO_PENDING:
          is_connect_pending_ = true;
          AddRef();
          return;
        case ERROR_NO_DATA:
        default:
          OnError(Error::kConnectionFailed);
          return;
      }
    }

    // Now that we have registered our IOHandler, we can start writing.
    {
      base::AutoLock lock(write_lock_);
      if (delay_writes_) {
        delay_writes_ = false;
        WriteNextNoLock();
      }
    }

    // Keep this alive in case we synchronously run shutdown, via OnError(),
    // as a result of a ReadFile() failure on the channel.
    scoped_refptr<ChannelWin> keep_alive(this);
    ReadMore(0);
  }

  void ShutDownOnIOThread() {
    base::MessageLoopCurrent::Get()->RemoveDestructionObserver(this);

    // BUG(crbug.com/583525): This function is expected to be called once, and
    // |handle_| should be valid at this point.
    CHECK(handle_.is_valid());
    CancelIo(handle_.get().handle);
    if (leak_handle_)
      ignore_result(handle_.release());
    handle_.reset();

    // Allow |this| to be destroyed as soon as no IO is pending.
    self_ = nullptr;
  }

  // base::MessageLoopCurrent::DestructionObserver:
  void WillDestroyCurrentMessageLoop() override {
    DCHECK(io_task_runner_->RunsTasksInCurrentSequence());
    if (self_)
      ShutDownOnIOThread();
  }

  // base::MessageLoop::IOHandler:
  void OnIOCompleted(base::MessagePumpForIO::IOContext* context,
                     DWORD bytes_transfered,
                     DWORD error) override {
    if (error != ERROR_SUCCESS) {
      if (context == &write_context_) {
        {
          base::AutoLock lock(write_lock_);
          reject_writes_ = true;
        }
        OnWriteError(Error::kDisconnected);
      } else {
        OnError(Error::kDisconnected);
      }
    } else if (context == &connect_context_) {
      DCHECK(is_connect_pending_);
      is_connect_pending_ = false;
      ReadMore(0);

      base::AutoLock lock(write_lock_);
      if (delay_writes_) {
        delay_writes_ = false;
        WriteNextNoLock();
      }
    } else if (context == &read_context_) {
      OnReadDone(static_cast<size_t>(bytes_transfered));
    } else {
      CHECK(context == &write_context_);
      OnWriteDone(static_cast<size_t>(bytes_transfered));
    }
    Release();
  }

  void OnReadDone(size_t bytes_read) {
    DCHECK(is_read_pending_);
    is_read_pending_ = false;

    if (bytes_read > 0) {
      size_t next_read_size = 0;
      if (OnReadComplete(bytes_read, &next_read_size)) {
        ReadMore(next_read_size);
      } else {
        OnError(Error::kReceivedMalformedData);
      }
    } else if (bytes_read == 0) {
      OnError(Error::kDisconnected);
    }
  }

  void OnWriteDone(size_t bytes_written) {
    if (bytes_written == 0)
      return;

    bool write_error = false;
    {
      base::AutoLock lock(write_lock_);

      DCHECK(is_write_pending_);
      is_write_pending_ = false;
      DCHECK(!outgoing_messages_.empty());

      Channel::MessagePtr message = std::move(outgoing_messages_.front());
      outgoing_messages_.pop_front();

      // Invalidate all the scoped handles so we don't attempt to close them.
      // Note that we don't simply release these objects because they also own
      // an internal process handle (in |owning_process|) which *does* need to
      // be closed.
      std::vector<ScopedInternalPlatformHandle> handles =
          message->TakeHandles();
      for (auto& handle : handles)
        handle.get().handle = INVALID_HANDLE_VALUE;

      // Overlapped WriteFile() to a pipe should always fully complete.
      if (message->data_num_bytes() != bytes_written)
        reject_writes_ = write_error = true;
      else if (!WriteNextNoLock())
        reject_writes_ = write_error = true;
    }
    if (write_error)
      OnWriteError(Error::kDisconnected);
  }

  void ReadMore(size_t next_read_size_hint) {
    DCHECK(!is_read_pending_);

    size_t buffer_capacity = next_read_size_hint;
    char* buffer = GetReadBuffer(&buffer_capacity);
    DCHECK_GT(buffer_capacity, 0u);

    BOOL ok = ReadFile(handle_.get().handle,
                       buffer,
                       static_cast<DWORD>(buffer_capacity),
                       NULL,
                       &read_context_.overlapped);

    if (ok || GetLastError() == ERROR_IO_PENDING) {
      is_read_pending_ = true;
      AddRef();
    } else {
      OnError(Error::kDisconnected);
    }
  }

  // Attempts to write a message directly to the channel. If the full message
  // cannot be written, it's queued and a wait is initiated to write the message
  // ASAP on the I/O thread.
  bool WriteNoLock(const Channel::MessagePtr& message) {
    BOOL ok = WriteFile(handle_.get().handle, message->data(),
                        static_cast<DWORD>(message->data_num_bytes()), NULL,
                        &write_context_.overlapped);

    if (ok || GetLastError() == ERROR_IO_PENDING) {
      is_write_pending_ = true;
      AddRef();
      return true;
    }
    return false;
  }

  bool WriteNextNoLock() {
    if (outgoing_messages_.empty())
      return true;
    return WriteNoLock(outgoing_messages_.front());
  }

  void OnWriteError(Error error) {
    DCHECK(io_task_runner_->RunsTasksInCurrentSequence());
    DCHECK(reject_writes_);

    if (error == Error::kDisconnected) {
      // If we can't write because the pipe is disconnected then continue
      // reading to fetch any in-flight messages, relying on end-of-stream to
      // signal the actual disconnection.
      if (is_read_pending_ || is_connect_pending_)
        return;
    }

    OnError(error);
  }

  // Keeps the Channel alive at least until explicit shutdown on the IO thread.
  scoped_refptr<Channel> self_;

  ScopedInternalPlatformHandle handle_;
  const scoped_refptr<base::TaskRunner> io_task_runner_;

  base::MessagePumpForIO::IOContext connect_context_;
  base::MessagePumpForIO::IOContext read_context_;
  bool is_connect_pending_ = false;
  bool is_read_pending_ = false;

  // Protects all fields potentially accessed on multiple threads via Write().
  base::Lock write_lock_;
  base::MessagePumpForIO::IOContext write_context_;
  base::circular_deque<Channel::MessagePtr> outgoing_messages_;
  bool delay_writes_ = true;
  bool reject_writes_ = false;
  bool is_write_pending_ = false;

  bool leak_handle_ = false;

  DISALLOW_COPY_AND_ASSIGN(ChannelWin);
};

}  // namespace

// static
scoped_refptr<Channel> Channel::Create(
    Delegate* delegate,
    ConnectionParams connection_params,
    scoped_refptr<base::TaskRunner> io_task_runner) {
  return new ChannelWin(delegate, connection_params.TakeChannelHandle(),
                        io_task_runner);
}

}  // namespace edk
}  // namespace mojo
