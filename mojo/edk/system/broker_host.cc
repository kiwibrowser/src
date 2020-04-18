// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/system/broker_host.h"

#include <utility>

#include "base/logging.h"
#include "base/memory/platform_shared_memory_region.h"
#include "base/memory/ref_counted.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "mojo/edk/embedder/named_platform_channel_pair.h"
#include "mojo/edk/embedder/named_platform_handle.h"
#include "mojo/edk/embedder/platform_handle_utils.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"
#include "mojo/edk/system/broker_messages.h"

namespace mojo {
namespace edk {

BrokerHost::BrokerHost(base::ProcessHandle client_process,
                       ScopedInternalPlatformHandle platform_handle,
                       const ProcessErrorCallback& process_error_callback)
    : process_error_callback_(process_error_callback)
#if defined(OS_WIN)
      ,
      client_process_(ScopedProcessHandle::CloneFrom(client_process))
#endif
{
  CHECK(platform_handle.is_valid());

  base::MessageLoopCurrent::Get()->AddDestructionObserver(this);

  channel_ = Channel::Create(
      this,
      ConnectionParams(TransportProtocol::kLegacy, std::move(platform_handle)),
      base::ThreadTaskRunnerHandle::Get());
  channel_->Start();
}

BrokerHost::~BrokerHost() {
  // We're always destroyed on the creation thread, which is the IO thread.
  base::MessageLoopCurrent::Get()->RemoveDestructionObserver(this);

  if (channel_)
    channel_->ShutDown();
}

bool BrokerHost::PrepareHandlesForClient(
    std::vector<ScopedInternalPlatformHandle>* handles) {
#if defined(OS_WIN)
  if (!Channel::Message::RewriteHandles(base::GetCurrentProcessHandle(),
                                        client_process_.get(), handles)) {
    // NOTE: We only log an error here. We do not signal a logical error or
    // prevent any message from being sent. The client should handle unexpected
    // invalid handles appropriately.
    DLOG(ERROR) << "Failed to rewrite one or more handles to broker client.";
    return false;
  }
#endif
  return true;
}

bool BrokerHost::SendChannel(ScopedInternalPlatformHandle handle) {
  CHECK(handle.is_valid());
  CHECK(channel_);

#if defined(OS_WIN)
  InitData* data;
  Channel::MessagePtr message =
      CreateBrokerMessage(BrokerMessageType::INIT, 1, 0, &data);
  data->pipe_name_length = 0;
#else
  Channel::MessagePtr message =
      CreateBrokerMessage(BrokerMessageType::INIT, 1, nullptr);
#endif
  std::vector<ScopedInternalPlatformHandle> handles(1);
  handles[0] = std::move(handle);

  // This may legitimately fail on Windows if the client process is in another
  // session, e.g., is an elevated process.
  if (!PrepareHandlesForClient(&handles))
    return false;

  message->SetHandles(std::move(handles));
  channel_->Write(std::move(message));
  return true;
}

#if defined(OS_WIN)

void BrokerHost::SendNamedChannel(const base::StringPiece16& pipe_name) {
  InitData* data;
  base::char16* name_data;
  Channel::MessagePtr message = CreateBrokerMessage(
      BrokerMessageType::INIT, 0, sizeof(*name_data) * pipe_name.length(),
      &data, reinterpret_cast<void**>(&name_data));
  data->pipe_name_length = static_cast<uint32_t>(pipe_name.length());
  std::copy(pipe_name.begin(), pipe_name.end(), name_data);
  channel_->Write(std::move(message));
}

#endif  // defined(OS_WIN)

void BrokerHost::OnBufferRequest(uint32_t num_bytes) {
  base::subtle::PlatformSharedMemoryRegion region =
      base::subtle::PlatformSharedMemoryRegion::CreateWritable(num_bytes);

  std::vector<ScopedInternalPlatformHandle> handles(2);
  if (region.IsValid()) {
    ExtractInternalPlatformHandlesFromSharedMemoryRegionHandle(
        region.PassPlatformHandle(), &handles[0], &handles[1]);
#if !defined(OS_POSIX) || defined(OS_ANDROID) || defined(OS_FUCHSIA) || \
    (defined(OS_MACOSX) && !defined(OS_IOS))
    // Non-POSIX systems, as well as Android, Fuchsia, and non-iOS Mac, only use
    // a single handle to represent a writable region.
    DCHECK(!handles[1].is_valid());
    handles.resize(1);
#else
    DCHECK(handles[1].is_valid());
#endif
  }

  BufferResponseData* response;
  Channel::MessagePtr message = CreateBrokerMessage(
      BrokerMessageType::BUFFER_RESPONSE, handles.size(), 0, &response);
  if (!handles.empty()) {
    base::UnguessableToken guid = region.GetGUID();
    response->guid_high = guid.GetHighForSerialization();
    response->guid_low = guid.GetLowForSerialization();
    PrepareHandlesForClient(&handles);
    message->SetHandles(std::move(handles));
  }

  channel_->Write(std::move(message));
}

void BrokerHost::OnChannelMessage(
    const void* payload,
    size_t payload_size,
    std::vector<ScopedInternalPlatformHandle> handles) {
  if (payload_size < sizeof(BrokerMessageHeader))
    return;

  const BrokerMessageHeader* header =
      static_cast<const BrokerMessageHeader*>(payload);
  switch (header->type) {
    case BrokerMessageType::BUFFER_REQUEST:
      if (payload_size ==
          sizeof(BrokerMessageHeader) + sizeof(BufferRequestData)) {
        const BufferRequestData* request =
            reinterpret_cast<const BufferRequestData*>(header + 1);
        OnBufferRequest(request->size);
      }
      break;

    default:
      DLOG(ERROR) << "Unexpected broker message type: " << header->type;
      break;
  }
}

void BrokerHost::OnChannelError(Channel::Error error) {
  if (process_error_callback_ &&
      error == Channel::Error::kReceivedMalformedData) {
    process_error_callback_.Run("Broker host received malformed message");
  }

  delete this;
}

void BrokerHost::WillDestroyCurrentMessageLoop() {
  delete this;
}

}  // namespace edk
}  // namespace mojo
