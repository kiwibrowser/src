// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/system/channel.h"

#include <fdio/limits.h>
#include <fdio/util.h>
#include <zircon/processargs.h>
#include <zircon/status.h>
#include <zircon/syscalls.h>
#include <algorithm>

#include "base/bind.h"
#include "base/containers/circular_deque.h"
#include "base/files/scoped_file.h"
#include "base/fuchsia/scoped_zx_handle.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop_current.h"
#include "base/message_loop/message_pump_for_io.h"
#include "base/synchronization/lock.h"
#include "base/task_runner.h"

namespace mojo {
namespace edk {

namespace {

const size_t kMaxBatchReadCapacity = 256 * 1024;

bool UnwrapInternalPlatformHandle(
    ScopedInternalPlatformHandle handle,
    Channel::Message::HandleInfoEntry* info_out,
    std::vector<ScopedInternalPlatformHandle>* handles_out) {
  DCHECK(handle.get().is_valid());

  if (!handle.get().is_valid_fd()) {
    *info_out = {0u, 0u};
    handles_out->emplace_back(std::move(handle));
    return true;
  }

  // Each FDIO file descriptor is implemented using one or more native resources
  // and can be un-wrapped into a set of |handle| and |info| pairs, with |info|
  // consisting of an FDIO-defined type & arguments (see zircon/processargs.h).
  //
  // We try to transfer the FD, but if that fails (for example if the file has
  // already been dup()d into another FD) we may need to clone.
  zx_handle_t handles[FDIO_MAX_HANDLES] = {};
  uint32_t info[FDIO_MAX_HANDLES] = {};
  zx_status_t result = fdio_transfer_fd(handle.get().as_fd(), 0, handles, info);
  if (result > 0) {
    // On success, the fd in |handle| has been transferred and is no longer
    // valid. Release from the ScopedInternalPlatformHandle to avoid close()ing
    // an invalid handle.
    ignore_result(handle.release());
  } else if (result == ZX_ERR_UNAVAILABLE) {
    // No luck, try cloning instead.
    result = fdio_clone_fd(handle.get().as_fd(), 0, handles, info);
  }

  if (result <= 0) {
    DLOG(ERROR) << "fdio_transfer_fd(" << handle.get().as_fd()
                << "): " << zx_status_get_string(result);
    return false;
  }
  DCHECK_LE(result, FDIO_MAX_HANDLES);

  // We assume here that only the |PA_HND_TYPE| of the |info| really matters,
  // and that that is the same for all the underlying handles.
  *info_out = {PA_HND_TYPE(info[0]), result};
  for (int i = 0; i < result; ++i) {
    DCHECK_EQ(PA_HND_TYPE(info[0]), PA_HND_TYPE(info[i]));
    DCHECK_EQ(0u, PA_HND_SUBTYPE(info[i]));
    handles_out->emplace_back(InternalPlatformHandle::ForHandle(handles[i]));
  }

  return true;
}

ScopedInternalPlatformHandle WrapInternalPlatformHandles(
    Channel::Message::HandleInfoEntry info,
    base::circular_deque<base::ScopedZxHandle>* handles) {
  ScopedInternalPlatformHandle out_handle;
  if (!info.type) {
    out_handle.reset(
        InternalPlatformHandle::ForHandle(handles->front().release()));
    handles->pop_front();
  } else {
    if (info.count > FDIO_MAX_HANDLES)
      return ScopedInternalPlatformHandle();

    // Fetch the required number of handles from |handles| and set up type info.
    zx_handle_t fd_handles[FDIO_MAX_HANDLES] = {};
    uint32_t fd_infos[FDIO_MAX_HANDLES] = {};
    for (int i = 0; i < info.count; ++i) {
      fd_handles[i] = (*handles)[i].get();
      fd_infos[i] = PA_HND(info.type, 0);
    }

    // Try to wrap the handles into an FDIO file descriptor.
    base::ScopedFD out_fd;
    zx_status_t result =
        fdio_create_fd(fd_handles, fd_infos, info.count, out_fd.receive());
    if (result != ZX_OK) {
      DLOG(ERROR) << "fdio_create_fd: " << zx_status_get_string(result);
      return ScopedInternalPlatformHandle();
    }

    // The handles are owned by FDIO now, so |release()| them before removing
    // the entries from |handles|.
    for (int i = 0; i < info.count; ++i) {
      ignore_result(handles->front().release());
      handles->pop_front();
    }

    out_handle.reset(InternalPlatformHandle::ForFd(out_fd.release()));
  }
  return out_handle;
}

// A view over a Channel::Message object. The write queue uses these since
// large messages may need to be sent in chunks.
class MessageView {
 public:
  // Owns |message|. |offset| indexes the first unsent byte in the message.
  MessageView(Channel::MessagePtr message, size_t offset)
      : message_(std::move(message)),
        offset_(offset),
        handles_(message_->TakeHandlesForTransport()) {
    DCHECK_GT(message_->data_num_bytes(), offset_);
  }

  MessageView(MessageView&& other) { *this = std::move(other); }

  MessageView& operator=(MessageView&& other) {
    message_ = std::move(other.message_);
    offset_ = other.offset_;
    handles_ = std::move(other.handles_);
    return *this;
  }

  ~MessageView() {}

  const void* data() const {
    return static_cast<const char*>(message_->data()) + offset_;
  }

  size_t data_num_bytes() const { return message_->data_num_bytes() - offset_; }

  size_t data_offset() const { return offset_; }
  void advance_data_offset(size_t num_bytes) {
    DCHECK_GT(message_->data_num_bytes(), offset_ + num_bytes);
    offset_ += num_bytes;
  }

  std::vector<ScopedInternalPlatformHandle> TakeHandles() {
    if (handles_.empty())
      return std::vector<ScopedInternalPlatformHandle>();

    // We can only pass Fuchsia handles via IPC, so unwrap any FDIO file-
    // descriptors in |handles_| into the underlying handles, and serialize the
    // metadata, if any, into the extra header.
    auto* handles_info = reinterpret_cast<Channel::Message::HandleInfoEntry*>(
        message_->mutable_extra_header());
    memset(handles_info, 0, message_->extra_header_size());

    std::vector<ScopedInternalPlatformHandle> in_handles = std::move(handles_);
    handles_.reserve(in_handles.size());
    for (size_t i = 0; i < in_handles.size(); i++) {
      if (!UnwrapInternalPlatformHandle(std::move(in_handles[i]),
                                        &handles_info[i], &handles_))
        return std::vector<ScopedInternalPlatformHandle>();
    }
    return std::move(handles_);
  }

 private:
  Channel::MessagePtr message_;
  size_t offset_;
  std::vector<ScopedInternalPlatformHandle> handles_;

  DISALLOW_COPY_AND_ASSIGN(MessageView);
};

class ChannelFuchsia : public Channel,
                       public base::MessageLoopCurrent::DestructionObserver,
                       public base::MessagePumpForIO::ZxHandleWatcher {
 public:
  ChannelFuchsia(Delegate* delegate,
                 ConnectionParams connection_params,
                 scoped_refptr<base::TaskRunner> io_task_runner)
      : Channel(delegate),
        self_(this),
        handle_(connection_params.TakeChannelHandle()),
        io_task_runner_(io_task_runner) {
    CHECK(handle_.is_valid());
  }

  void Start() override {
    if (io_task_runner_->RunsTasksInCurrentSequence()) {
      StartOnIOThread();
    } else {
      io_task_runner_->PostTask(
          FROM_HERE, base::BindOnce(&ChannelFuchsia::StartOnIOThread, this));
    }
  }

  void ShutDownImpl() override {
    // Always shut down asynchronously when called through the public interface.
    io_task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&ChannelFuchsia::ShutDownOnIOThread, this));
  }

  void Write(MessagePtr message) override {
    bool write_error = false;
    {
      base::AutoLock lock(write_lock_);
      if (reject_writes_)
        return;
      if (!WriteNoLock(MessageView(std::move(message), 0)))
        reject_writes_ = write_error = true;
    }
    if (write_error) {
      // Do not synchronously invoke OnWriteError(). Write() may have been
      // called by the delegate and we don't want to re-enter it.
      io_task_runner_->PostTask(
          FROM_HERE, base::BindOnce(&ChannelFuchsia::OnWriteError, this,
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
    DCHECK(io_task_runner_->RunsTasksInCurrentSequence());
    if (num_handles > std::numeric_limits<uint16_t>::max())
      return false;

    // Locate the handle info and verify there is enough of it.
    if (!extra_header)
      return false;
    const auto* handles_info =
        reinterpret_cast<const Channel::Message::HandleInfoEntry*>(
            extra_header);
    size_t handles_info_size = sizeof(handles_info[0]) * num_handles;
    if (handles_info_size > extra_header_size)
      return false;

    // Some caller-supplied handles may be FDIO file-descriptors, which were
    // un-wrapped to more than one native platform resource handle for transfer.
    // We may therefore need to expect more than |num_handles| handles to have
    // been accumulated in |incoming_handles_|, based on the handle info.
    size_t num_raw_handles = 0u;
    for (size_t i = 0; i < num_handles; ++i)
      num_raw_handles += handles_info[i].type ? handles_info[i].count : 1;

    // If there are too few handles then we're not ready yet, so return true
    // indicating things are OK, but leave |handles| empty.
    if (incoming_handles_.size() < num_raw_handles)
      return true;

    handles->reserve(num_handles);
    for (size_t i = 0; i < num_handles; ++i) {
      handles->emplace_back(
          WrapInternalPlatformHandles(handles_info[i], &incoming_handles_)
              .release());
    }
    return true;
  }

 private:
  ~ChannelFuchsia() override {
    DCHECK(!read_watch_);
  }

  void StartOnIOThread() {
    DCHECK(!read_watch_);

    base::MessageLoopCurrent::Get()->AddDestructionObserver(this);

    read_watch_.reset(
        new base::MessagePumpForIO::ZxHandleWatchController(FROM_HERE));
    base::MessageLoopCurrentForIO::Get()->WatchZxHandle(
        handle_.get().as_handle(), true /* persistent */,
        ZX_CHANNEL_READABLE | ZX_CHANNEL_PEER_CLOSED, read_watch_.get(), this);
  }

  void ShutDownOnIOThread() {
    base::MessageLoopCurrent::Get()->RemoveDestructionObserver(this);

    read_watch_.reset();
    if (leak_handle_)
      ignore_result(handle_.release());
    handle_.reset();

    // May destroy the |this| if it was the last reference.
    self_ = nullptr;
  }

  // base::MessageLoopCurrent::DestructionObserver:
  void WillDestroyCurrentMessageLoop() override {
    DCHECK(io_task_runner_->RunsTasksInCurrentSequence());
    if (self_)
      ShutDownOnIOThread();
  }

  // base::MessagePumpForIO::ZxHandleWatcher:
  void OnZxHandleSignalled(zx_handle_t handle, zx_signals_t signals) override {
    DCHECK(io_task_runner_->RunsTasksInCurrentSequence());
    CHECK_EQ(handle, handle_.get().as_handle());
    DCHECK((ZX_CHANNEL_READABLE | ZX_CHANNEL_PEER_CLOSED) & signals);

    // We always try to read message(s), even if ZX_CHANNEL_PEER_CLOSED, since
    // the peer may have closed while messages were still unread, in the pipe.

    bool validation_error = false;
    bool read_error = false;
    size_t next_read_size = 0;
    size_t buffer_capacity = 0;
    size_t total_bytes_read = 0;
    do {
      buffer_capacity = next_read_size;
      char* buffer = GetReadBuffer(&buffer_capacity);
      DCHECK_GT(buffer_capacity, 0u);

      uint32_t bytes_read = 0;
      uint32_t handles_read = 0;
      zx_handle_t handles[ZX_CHANNEL_MAX_MSG_HANDLES] = {};

      zx_status_t read_result = zx_channel_read(
          handle_.get().as_handle(), 0, buffer, handles, buffer_capacity,
          arraysize(handles), &bytes_read, &handles_read);
      if (read_result == ZX_OK) {
        for (size_t i = 0; i < handles_read; ++i) {
          incoming_handles_.push_back(base::ScopedZxHandle(handles[i]));
        }
        total_bytes_read += bytes_read;
        if (!OnReadComplete(bytes_read, &next_read_size)) {
          read_error = true;
          validation_error = true;
          break;
        }
      } else if (read_result == ZX_ERR_BUFFER_TOO_SMALL) {
        DCHECK_LE(handles_read, arraysize(handles));
        next_read_size = bytes_read;
      } else if (read_result == ZX_ERR_SHOULD_WAIT) {
        break;
      } else {
        DLOG_IF(ERROR, read_result != ZX_ERR_PEER_CLOSED)
            << "zx_channel_read: " << zx_status_get_string(read_result);
        read_error = true;
        break;
      }
    } while (total_bytes_read < kMaxBatchReadCapacity && next_read_size > 0);
    if (read_error) {
      // Stop receiving read notifications.
      read_watch_.reset();
      if (validation_error)
        OnError(Error::kReceivedMalformedData);
      else
        OnError(Error::kDisconnected);
    }
  }

  // Attempts to write a message directly to the channel. If the full message
  // cannot be written, it's queued and a wait is initiated to write the message
  // ASAP on the I/O thread.
  bool WriteNoLock(MessageView message_view) {
    uint32_t write_bytes = 0;
    do {
      message_view.advance_data_offset(write_bytes);

      std::vector<ScopedInternalPlatformHandle> outgoing_handles =
          message_view.TakeHandles();
      zx_handle_t handles[ZX_CHANNEL_MAX_MSG_HANDLES] = {};
      size_t handles_count = outgoing_handles.size();

      DCHECK_LE(handles_count, arraysize(handles));
      for (size_t i = 0; i < handles_count; ++i) {
        DCHECK(outgoing_handles[i].is_valid());
        handles[i] = outgoing_handles[i].get().as_handle();
      }

      write_bytes = std::min(message_view.data_num_bytes(),
                             static_cast<size_t>(ZX_CHANNEL_MAX_MSG_BYTES));
      zx_status_t result =
          zx_channel_write(handle_.get().as_handle(), 0, message_view.data(),
                           write_bytes, handles, handles_count);
      if (result != ZX_OK) {
        // TODO(fuchsia): Handle ZX_ERR_SHOULD_WAIT flow-control errors, once
        // the platform starts generating them. See crbug.com/754084.
        DLOG_IF(ERROR, result != ZX_ERR_PEER_CLOSED)
            << "WriteNoLock(zx_channel_write): "
            << zx_status_get_string(result);
        return false;
      }

      // |handles| have been transferred to the peer process, so release()
      // them, to avoid them being double-closed.
      for (auto& outgoing_handle : outgoing_handles)
        ignore_result(outgoing_handle.release());
    } while (write_bytes < message_view.data_num_bytes());

    return true;
  }

  void OnWriteError(Error error) {
    DCHECK(io_task_runner_->RunsTasksInCurrentSequence());
    DCHECK(reject_writes_);

    if (error == Error::kDisconnected) {
      // If we can't write because the pipe is disconnected then continue
      // reading to fetch any in-flight messages, relying on end-of-stream to
      // signal the actual disconnection.
      if (read_watch_) {
        // TODO: When we add flow-control for writes, we also need to reset the
        // write-watcher here.
        return;
      }
    }

    OnError(error);
  }

  // Keeps the Channel alive at least until explicit shutdown on the IO thread.
  scoped_refptr<Channel> self_;

  ScopedInternalPlatformHandle handle_;
  scoped_refptr<base::TaskRunner> io_task_runner_;

  // These members are only used on the IO thread.
  std::unique_ptr<base::MessagePumpForIO::ZxHandleWatchController> read_watch_;
  base::circular_deque<base::ScopedZxHandle> incoming_handles_;
  bool leak_handle_ = false;

  base::Lock write_lock_;
  bool reject_writes_ = false;

  DISALLOW_COPY_AND_ASSIGN(ChannelFuchsia);
};

}  // namespace

// static
scoped_refptr<Channel> Channel::Create(
    Delegate* delegate,
    ConnectionParams connection_params,
    scoped_refptr<base::TaskRunner> io_task_runner) {
  return new ChannelFuchsia(delegate, std::move(connection_params),
                            std::move(io_task_runner));
}

}  // namespace edk
}  // namespace mojo
