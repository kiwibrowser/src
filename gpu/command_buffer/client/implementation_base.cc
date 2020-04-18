// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/client/implementation_base.h"

#include <algorithm>

#include "base/strings/stringprintf.h"
#include "base/trace_event/memory_dump_manager.h"
#include "base/trace_event/trace_event.h"
#include "gpu/command_buffer/client/cmd_buffer_helper.h"
#include "gpu/command_buffer/client/gpu_control.h"
#include "gpu/command_buffer/client/mapped_memory.h"
#include "gpu/command_buffer/client/query_tracker.h"
#include "gpu/command_buffer/client/shared_memory_limits.h"
#include "gpu/command_buffer/common/sync_token.h"

namespace gpu {

#if !defined(_MSC_VER)
const size_t ImplementationBase::kMaxSizeOfSimpleResult;
const unsigned int ImplementationBase::kStartingOffset;
#endif

ImplementationBase::ImplementationBase(CommandBufferHelper* helper,
                                       TransferBufferInterface* transfer_buffer,
                                       GpuControl* gpu_control)
    : transfer_buffer_(transfer_buffer),
      gpu_control_(gpu_control),
      capabilities_(gpu_control->GetCapabilities()),
      helper_(helper),
      weak_ptr_factory_(this) {}

ImplementationBase::~ImplementationBase() {
  // The gpu_control_ outlives this class, so clear the client on it before we
  // self-destruct.
  gpu_control_->SetGpuControlClient(nullptr);
}

void ImplementationBase::FreeUnusedSharedMemory() {
  mapped_memory_->FreeUnused();
}

void ImplementationBase::FreeEverything() {
  query_tracker_->Shrink(helper_);
  FreeUnusedSharedMemory();
  transfer_buffer_->Free();
  helper_->FreeRingBuffer();
}

void ImplementationBase::SetLostContextCallback(base::OnceClosure callback) {
  lost_context_callback_ = std::move(callback);
}

void ImplementationBase::FlushPendingWork() {
  gpu_control_->FlushPendingWork();
}

void ImplementationBase::SignalSyncToken(const SyncToken& sync_token,
                                         base::OnceClosure callback) {
  SyncToken verified_sync_token;
  if (sync_token.HasData() &&
      GetVerifiedSyncTokenForIPC(sync_token, &verified_sync_token)) {
    // We can only send verified sync tokens across IPC.
    gpu_control_->SignalSyncToken(
        verified_sync_token,
        base::BindOnce(&ImplementationBase::RunIfContextNotLost,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  } else {
    // Invalid sync token, just call the callback immediately.
    std::move(callback).Run();
  }
}

// This may be called from any thread. It's safe to access gpu_control_ without
// the lock because it is const.
bool ImplementationBase::IsSyncTokenSignaled(const SyncToken& sync_token) {
  // Check that the sync token belongs to this context.
  DCHECK_EQ(gpu_control_->GetNamespaceID(), sync_token.namespace_id());
  DCHECK_EQ(gpu_control_->GetCommandBufferID(), sync_token.command_buffer_id());
  return gpu_control_->IsFenceSyncReleased(sync_token.release_count());
}

void ImplementationBase::SignalQuery(uint32_t query,
                                     base::OnceClosure callback) {
  // Flush previously entered commands to ensure ordering with any
  // glBeginQueryEXT() calls that may have been put into the context.
  IssueShallowFlush();
  gpu_control_->SignalQuery(
      query,
      base::BindOnce(&ImplementationBase::RunIfContextNotLost,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void ImplementationBase::GetGpuFence(
    uint32_t gpu_fence_id,
    base::OnceCallback<void(std::unique_ptr<gfx::GpuFence>)> callback) {
  // This ShallowFlush is required to ensure that the GetGpuFence
  // call is processed after the preceding CreateGpuFenceCHROMIUM call.
  IssueShallowFlush();
  gpu_control_->GetGpuFence(gpu_fence_id, std::move(callback));
}

bool ImplementationBase::OnMemoryDump(
    const base::trace_event::MemoryDumpArgs& args,
    base::trace_event::ProcessMemoryDump* pmd) {
  using base::trace_event::MemoryAllocatorDump;
  using base::trace_event::MemoryDumpLevelOfDetail;

  // Dump owned MappedMemoryManager memory as well.
  mapped_memory_->OnMemoryDump(args, pmd);

  if (!transfer_buffer_->HaveBuffer())
    return true;

  const uint64_t tracing_process_id =
      base::trace_event::MemoryDumpManager::GetInstance()
          ->GetTracingProcessId();

  MemoryAllocatorDump* dump = pmd->CreateAllocatorDump(base::StringPrintf(
      "gpu/transfer_buffer_memory/buffer_%d", transfer_buffer_->GetShmId()));
  dump->AddScalar(MemoryAllocatorDump::kNameSize,
                  MemoryAllocatorDump::kUnitsBytes,
                  transfer_buffer_->GetSize());

  if (args.level_of_detail != MemoryDumpLevelOfDetail::BACKGROUND) {
    dump->AddScalar("free_size", MemoryAllocatorDump::kUnitsBytes,
                    transfer_buffer_->GetFragmentedFreeSize());
    auto shared_memory_guid =
        transfer_buffer_->shared_memory_handle().GetGUID();
    const int kImportance = 2;
    if (!shared_memory_guid.is_empty()) {
      pmd->CreateSharedMemoryOwnershipEdge(dump->guid(), shared_memory_guid,
                                           kImportance);
    } else {
      auto guid = GetBufferGUIDForTracing(tracing_process_id,
                                          transfer_buffer_->GetShmId());
      pmd->CreateSharedGlobalAllocatorDump(guid);
      pmd->AddOwnershipEdge(dump->guid(), guid, kImportance);
    }
  }

  return true;
}

gpu::ContextResult ImplementationBase::Initialize(
    const SharedMemoryLimits& limits) {
  TRACE_EVENT0("gpu", "ImplementationBase::Initialize");
  DCHECK_GE(limits.start_transfer_buffer_size, limits.min_transfer_buffer_size);
  DCHECK_LE(limits.start_transfer_buffer_size, limits.max_transfer_buffer_size);
  DCHECK_GE(limits.min_transfer_buffer_size, kStartingOffset);

  gpu_control_->SetGpuControlClient(this);

  if (!transfer_buffer_->Initialize(
          limits.start_transfer_buffer_size, kStartingOffset,
          limits.min_transfer_buffer_size, limits.max_transfer_buffer_size,
          kAlignment, kSizeToFlush)) {
    // TransferBuffer::Initialize doesn't fail for transient reasons such as if
    // the context was lost. See http://crrev.com/c/720269
    LOG(ERROR) << "ContextResult::kFatalFailure: "
               << "TransferBuffer::Initialize() failed";
    return gpu::ContextResult::kFatalFailure;
  }

  mapped_memory_ = std::make_unique<MappedMemoryManager>(
      helper_, limits.mapped_memory_reclaim_limit);
  mapped_memory_->set_chunk_size_multiple(limits.mapped_memory_chunk_size);
  query_tracker_ = std::make_unique<gles2::QueryTracker>(mapped_memory_.get());

  return gpu::ContextResult::kSuccess;
}

void ImplementationBase::WaitForCmd() {
  TRACE_EVENT0("gpu", "ImplementationBase::WaitForCmd");
  helper_->Finish();
}

void* ImplementationBase::GetResultBuffer() {
  return transfer_buffer_->GetResultBuffer();
}

int32_t ImplementationBase::GetResultShmId() {
  return transfer_buffer_->GetShmId();
}

uint32_t ImplementationBase::GetResultShmOffset() {
  return transfer_buffer_->GetResultOffset();
}

bool ImplementationBase::GetBucketContents(uint32_t bucket_id,
                                           std::vector<int8_t>* data) {
  TRACE_EVENT0("gpu", "ImplementationBase::GetBucketContents");
  DCHECK(data);
  const uint32_t kStartSize = 32 * 1024;
  ScopedTransferBufferPtr buffer(kStartSize, helper_, transfer_buffer_);
  if (!buffer.valid()) {
    return false;
  }
  typedef cmd::GetBucketStart::Result Result;
  Result* result = GetResultAs<Result*>();
  if (!result) {
    return false;
  }
  *result = 0;
  helper_->GetBucketStart(bucket_id, GetResultShmId(), GetResultShmOffset(),
                          buffer.size(), buffer.shm_id(), buffer.offset());
  WaitForCmd();
  uint32_t size = *result;
  data->resize(size);
  if (size > 0u) {
    uint32_t offset = 0;
    while (size) {
      if (!buffer.valid()) {
        buffer.Reset(size);
        if (!buffer.valid()) {
          return false;
        }
        helper_->GetBucketData(bucket_id, offset, buffer.size(),
                               buffer.shm_id(), buffer.offset());
        WaitForCmd();
      }
      uint32_t size_to_copy = std::min(size, buffer.size());
      memcpy(&(*data)[offset], buffer.address(), size_to_copy);
      offset += size_to_copy;
      size -= size_to_copy;
      buffer.Release();
    }
    // Free the bucket. This is not required but it does free up the memory.
    // and we don't have to wait for the result so from the client's perspective
    // it's cheap.
    helper_->SetBucketSize(bucket_id, 0);
  }
  return true;
}

void ImplementationBase::SetBucketContents(uint32_t bucket_id,
                                           const void* data,
                                           size_t size) {
  DCHECK(data);
  helper_->SetBucketSize(bucket_id, size);
  if (size > 0u) {
    uint32_t offset = 0;
    while (size) {
      ScopedTransferBufferPtr buffer(size, helper_, transfer_buffer_);
      if (!buffer.valid()) {
        return;
      }
      memcpy(buffer.address(), static_cast<const int8_t*>(data) + offset,
             buffer.size());
      helper_->SetBucketData(bucket_id, offset, buffer.size(), buffer.shm_id(),
                             buffer.offset());
      offset += buffer.size();
      size -= buffer.size();
    }
  }
}

void ImplementationBase::SetBucketAsCString(uint32_t bucket_id,
                                            const char* str) {
  // NOTE: strings are passed NULL terminated. That means the empty
  // string will have a size of 1 and no-string will have a size of 0
  if (str) {
    SetBucketContents(bucket_id, str, strlen(str) + 1);
  } else {
    helper_->SetBucketSize(bucket_id, 0);
  }
}

bool ImplementationBase::GetBucketAsString(uint32_t bucket_id,
                                           std::string* str) {
  DCHECK(str);
  std::vector<int8_t> data;
  // NOTE: strings are passed NULL terminated. That means the empty
  // string will have a size of 1 and no-string will have a size of 0
  if (!GetBucketContents(bucket_id, &data)) {
    return false;
  }
  if (data.empty()) {
    return false;
  }
  str->assign(&data[0], &data[0] + data.size() - 1);
  return true;
}

void ImplementationBase::SetBucketAsString(uint32_t bucket_id,
                                           const std::string& str) {
  // NOTE: strings are passed NULL terminated. That means the empty
  // string will have a size of 1 and no-string will have a size of 0
  SetBucketContents(bucket_id, str.c_str(), str.size() + 1);
}

bool ImplementationBase::GetVerifiedSyncTokenForIPC(
    const SyncToken& sync_token,
    SyncToken* verified_sync_token) {
  DCHECK(sync_token.HasData());
  DCHECK(verified_sync_token);

  if (!sync_token.verified_flush() &&
      !gpu_control_->CanWaitUnverifiedSyncToken(sync_token))
    return false;

  *verified_sync_token = sync_token;
  verified_sync_token->SetVerifyFlush();
  return true;
}

void ImplementationBase::RunIfContextNotLost(base::OnceClosure callback) {
  if (!lost_context_callback_run_) {
    std::move(callback).Run();
  }
}

void ImplementationBase::SetGrContext(GrContext* gr) {}

bool ImplementationBase::HasGrContextSupport() const {
  return false;
}

void ImplementationBase::WillCallGLFromSkia() {
  // Should only be called on subclasses that have GrContextSupport
  NOTREACHED();
}

void ImplementationBase::DidCallGLFromSkia() {
  // Should only be called on subclasses that have GrContextSupport
  NOTREACHED();
}

}  // namespace gpu
