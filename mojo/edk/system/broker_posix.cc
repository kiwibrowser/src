// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/system/broker.h"

#include <fcntl.h>
#include <unistd.h>

#include <utility>

#include "base/logging.h"
#include "base/memory/platform_shared_memory_region.h"
#include "build/build_config.h"
#include "mojo/edk/embedder/platform_channel_utils_posix.h"
#include "mojo/edk/embedder/platform_handle_utils.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"
#include "mojo/edk/system/broker_messages.h"
#include "mojo/edk/system/channel.h"

namespace mojo {
namespace edk {

namespace {

Channel::MessagePtr WaitForBrokerMessage(
    const ScopedInternalPlatformHandle& platform_handle,
    BrokerMessageType expected_type,
    size_t expected_num_handles,
    size_t expected_data_size,
    std::vector<ScopedInternalPlatformHandle>* incoming_handles) {
  Channel::MessagePtr message(new Channel::Message(
      sizeof(BrokerMessageHeader) + expected_data_size, expected_num_handles));
  base::circular_deque<ScopedInternalPlatformHandle> incoming_platform_handles;
  ssize_t read_result = PlatformChannelRecvmsg(
      platform_handle, const_cast<void*>(message->data()),
      message->data_num_bytes(), &incoming_platform_handles, true /* block */);
  bool error = false;
  if (read_result < 0) {
    PLOG(ERROR) << "Recvmsg error";
    error = true;
  } else if (static_cast<size_t>(read_result) != message->data_num_bytes()) {
    LOG(ERROR) << "Invalid node channel message";
    error = true;
  } else if (incoming_platform_handles.size() != expected_num_handles) {
    LOG(ERROR) << "Received unexpected number of handles";
    error = true;
  }

  if (error)
    return nullptr;

  const BrokerMessageHeader* header =
      reinterpret_cast<const BrokerMessageHeader*>(message->payload());
  if (header->type != expected_type) {
    LOG(ERROR) << "Unexpected message";
    return nullptr;
  }

  incoming_handles->resize(incoming_platform_handles.size());
  std::move(incoming_platform_handles.begin(), incoming_platform_handles.end(),
            incoming_handles->begin());

  return message;
}

}  // namespace

Broker::Broker(ScopedInternalPlatformHandle platform_handle)
    : sync_channel_(std::move(platform_handle)) {
  CHECK(sync_channel_.is_valid());

  // Mark the channel as blocking.
  int flags = fcntl(sync_channel_.get().handle, F_GETFL);
  PCHECK(flags != -1);
  flags = fcntl(sync_channel_.get().handle, F_SETFL, flags & ~O_NONBLOCK);
  PCHECK(flags != -1);

  // Wait for the first message, which should contain a handle.
  std::vector<ScopedInternalPlatformHandle> incoming_platform_handles;
  if (WaitForBrokerMessage(sync_channel_, BrokerMessageType::INIT, 1, 0,
                           &incoming_platform_handles)) {
    inviter_channel_ = std::move(incoming_platform_handles[0]);
  }
}

Broker::~Broker() = default;

ScopedInternalPlatformHandle Broker::GetInviterInternalPlatformHandle() {
  return std::move(inviter_channel_);
}

base::WritableSharedMemoryRegion Broker::GetWritableSharedMemoryRegion(
    size_t num_bytes) {
  base::AutoLock lock(lock_);

  BufferRequestData* buffer_request;
  Channel::MessagePtr out_message = CreateBrokerMessage(
      BrokerMessageType::BUFFER_REQUEST, 0, 0, &buffer_request);
  buffer_request->size = num_bytes;
  ssize_t write_result = PlatformChannelWrite(
      sync_channel_, out_message->data(), out_message->data_num_bytes());
  if (write_result < 0) {
    PLOG(ERROR) << "Error sending sync broker message";
    return base::WritableSharedMemoryRegion();
  } else if (static_cast<size_t>(write_result) !=
             out_message->data_num_bytes()) {
    LOG(ERROR) << "Error sending complete broker message";
    return base::WritableSharedMemoryRegion();
  }

#if !defined(OS_POSIX) || defined(OS_ANDROID) || defined(OS_FUCHSIA) || \
    (defined(OS_MACOSX) && !defined(OS_IOS))
  // Non-POSIX systems, as well as Android, Fuchsia, and non-iOS Mac, only use
  // a single handle to represent a writable region.
  constexpr size_t kNumExpectedHandles = 1;
#else
  constexpr size_t kNumExpectedHandles = 2;
#endif

  std::vector<ScopedInternalPlatformHandle> incoming_platform_handles;
  Channel::MessagePtr message = WaitForBrokerMessage(
      sync_channel_, BrokerMessageType::BUFFER_RESPONSE, kNumExpectedHandles,
      sizeof(BufferResponseData), &incoming_platform_handles);
  if (message) {
    const BufferResponseData* data;
    if (!GetBrokerMessageData(message.get(), &data))
      return base::WritableSharedMemoryRegion();

    if (incoming_platform_handles.size() == 1)
      incoming_platform_handles.emplace_back();
    return base::WritableSharedMemoryRegion::Deserialize(
        base::subtle::PlatformSharedMemoryRegion::Take(
            CreateSharedMemoryRegionHandleFromInternalPlatformHandles(
                std::move(incoming_platform_handles[0]),
                std::move(incoming_platform_handles[1])),
            base::subtle::PlatformSharedMemoryRegion::Mode::kWritable,
            num_bytes,
            base::UnguessableToken::Deserialize(data->guid_high,
                                                data->guid_low)));
  }

  return base::WritableSharedMemoryRegion();
}

}  // namespace edk
}  // namespace mojo
