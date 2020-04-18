// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <windows.h>

#include <limits>
#include <utility>

#include "base/debug/alias.h"
#include "base/memory/platform_shared_memory_region.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/string_piece.h"
#include "mojo/edk/embedder/named_platform_handle.h"
#include "mojo/edk/embedder/named_platform_handle_utils.h"
#include "mojo/edk/embedder/platform_handle.h"
#include "mojo/edk/embedder/platform_handle_utils.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"
#include "mojo/edk/system/broker.h"
#include "mojo/edk/system/broker_messages.h"
#include "mojo/edk/system/channel.h"

namespace mojo {
namespace edk {

namespace {

// 256 bytes should be enough for anyone!
const size_t kMaxBrokerMessageSize = 256;

bool TakeHandlesFromBrokerMessage(Channel::Message* message,
                                  size_t num_handles,
                                  ScopedInternalPlatformHandle* out_handles) {
  if (message->num_handles() != num_handles) {
    DLOG(ERROR) << "Received unexpected number of handles in broker message";
    return false;
  }

  std::vector<ScopedInternalPlatformHandle> handles = message->TakeHandles();
  DCHECK_EQ(handles.size(), num_handles);
  DCHECK(out_handles);

  for (size_t i = 0; i < num_handles; ++i)
    out_handles[i] = std::move(handles[i]);
  return true;
}

Channel::MessagePtr WaitForBrokerMessage(InternalPlatformHandle platform_handle,
                                         BrokerMessageType expected_type) {
  char buffer[kMaxBrokerMessageSize];
  DWORD bytes_read = 0;
  BOOL result = ::ReadFile(platform_handle.handle, buffer,
                           kMaxBrokerMessageSize, &bytes_read, nullptr);
  if (!result) {
    // The pipe may be broken if the browser side has been closed, e.g. during
    // browser shutdown. In that case the ReadFile call will fail and we
    // shouldn't continue waiting.
    PLOG(ERROR) << "Error reading broker pipe";
    return nullptr;
  }

  Channel::MessagePtr message =
      Channel::Message::Deserialize(buffer, static_cast<size_t>(bytes_read));
  if (!message || message->payload_size() < sizeof(BrokerMessageHeader)) {
    LOG(ERROR) << "Invalid broker message";

    base::debug::Alias(&buffer[0]);
    base::debug::Alias(&bytes_read);
    CHECK(false);
    return nullptr;
  }

  const BrokerMessageHeader* header =
      reinterpret_cast<const BrokerMessageHeader*>(message->payload());
  if (header->type != expected_type) {
    LOG(ERROR) << "Unexpected broker message type";

    base::debug::Alias(&buffer[0]);
    base::debug::Alias(&bytes_read);
    CHECK(false);
    return nullptr;
  }

  return message;
}

}  // namespace

Broker::Broker(ScopedInternalPlatformHandle handle)
    : sync_channel_(std::move(handle)) {
  CHECK(sync_channel_.is_valid());
  Channel::MessagePtr message =
      WaitForBrokerMessage(sync_channel_.get(), BrokerMessageType::INIT);

  // If we fail to read a message (broken pipe), just return early. The inviter
  // handle will be null and callers must handle this gracefully.
  if (!message)
    return;

  if (!TakeHandlesFromBrokerMessage(message.get(), 1, &inviter_channel_)) {
    // If the message has no handles, we expect it to carry pipe name instead.
    const BrokerMessageHeader* header =
        static_cast<const BrokerMessageHeader*>(message->payload());
    CHECK_GE(message->payload_size(),
             sizeof(BrokerMessageHeader) + sizeof(InitData));
    const InitData* data = reinterpret_cast<const InitData*>(header + 1);
    CHECK_EQ(message->payload_size(),
             sizeof(BrokerMessageHeader) + sizeof(InitData) +
                 data->pipe_name_length * sizeof(base::char16));
    const base::char16* name_data =
        reinterpret_cast<const base::char16*>(data + 1);
    CHECK(data->pipe_name_length);
    inviter_channel_ = CreateClientHandle(NamedPlatformHandle(
        base::StringPiece16(name_data, data->pipe_name_length)));
  }
}

Broker::~Broker() {}

ScopedInternalPlatformHandle Broker::GetInviterInternalPlatformHandle() {
  return std::move(inviter_channel_);
}

base::WritableSharedMemoryRegion Broker::GetWritableSharedMemoryRegion(
    size_t num_bytes) {
  base::AutoLock lock(lock_);
  BufferRequestData* buffer_request;
  Channel::MessagePtr out_message = CreateBrokerMessage(
      BrokerMessageType::BUFFER_REQUEST, 0, 0, &buffer_request);
  buffer_request->size = base::checked_cast<uint32_t>(num_bytes);
  DWORD bytes_written = 0;
  BOOL result = ::WriteFile(sync_channel_.get().handle, out_message->data(),
                            static_cast<DWORD>(out_message->data_num_bytes()),
                            &bytes_written, nullptr);
  if (!result ||
      static_cast<size_t>(bytes_written) != out_message->data_num_bytes()) {
    PLOG(ERROR) << "Error sending sync broker message";
    return base::WritableSharedMemoryRegion();
  }

  ScopedInternalPlatformHandle handle;
  Channel::MessagePtr response = WaitForBrokerMessage(
      sync_channel_.get(), BrokerMessageType::BUFFER_RESPONSE);
  if (response && TakeHandlesFromBrokerMessage(response.get(), 1, &handle)) {
    BufferResponseData* data;
    if (!GetBrokerMessageData(response.get(), &data))
      return base::WritableSharedMemoryRegion();
    return base::WritableSharedMemoryRegion::Deserialize(
        base::subtle::PlatformSharedMemoryRegion::Take(
            CreateSharedMemoryRegionHandleFromInternalPlatformHandles(
                std::move(handle), ScopedInternalPlatformHandle()),
            base::subtle::PlatformSharedMemoryRegion::Mode::kWritable,
            num_bytes,
            base::UnguessableToken::Deserialize(data->guid_high,
                                                data->guid_low)));
  }

  return base::WritableSharedMemoryRegion();
}

}  // namespace edk
}  // namespace mojo
