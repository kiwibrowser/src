// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/command_buffer_direct.h"

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "gpu/command_buffer/service/sync_point_manager.h"
#include "gpu/command_buffer/service/transfer_buffer_manager.h"

namespace gpu {

namespace {

uint64_t g_next_command_buffer_id = 1;

}  // anonymous namespace

CommandBufferDirect::CommandBufferDirect(
    TransferBufferManager* transfer_buffer_manager)
    : CommandBufferDirect(transfer_buffer_manager, nullptr) {}

CommandBufferDirect::CommandBufferDirect(
    TransferBufferManager* transfer_buffer_manager,
    SyncPointManager* sync_point_manager)
    : service_(this, transfer_buffer_manager),
      sync_point_manager_(sync_point_manager),
      command_buffer_id_(
          CommandBufferId::FromUnsafeValue(g_next_command_buffer_id++)) {
  if (sync_point_manager_) {
    sync_point_order_data_ = sync_point_manager_->CreateSyncPointOrderData();
    sync_point_client_state_ = sync_point_manager_->CreateSyncPointClientState(
        GetNamespaceID(), GetCommandBufferID(),
        sync_point_order_data_->sequence_id());
  } else {
    sync_point_order_data_ = nullptr;
    sync_point_client_state_ = nullptr;
  }
}

CommandBufferDirect::~CommandBufferDirect() {
  sync_point_manager_ = nullptr;
  if (sync_point_order_data_) {
    sync_point_order_data_->Destroy();
    sync_point_order_data_ = nullptr;
  }
  if (sync_point_client_state_) {
    sync_point_client_state_->Destroy();
    sync_point_client_state_ = nullptr;
  }
}

CommandBuffer::State CommandBufferDirect::GetLastState() {
  service_.UpdateState();
  return service_.GetState();
}

CommandBuffer::State CommandBufferDirect::WaitForTokenInRange(int32_t start,
                                                              int32_t end) {
  State state = GetLastState();
  DCHECK(state.error != error::kNoError || InRange(start, end, state.token));
  return state;
}

CommandBuffer::State CommandBufferDirect::WaitForGetOffsetInRange(
    uint32_t set_get_buffer_count,
    int32_t start,
    int32_t end) {
  State state = GetLastState();
  DCHECK(state.error != error::kNoError ||
         (InRange(start, end, state.get_offset) &&
          (set_get_buffer_count == state.set_get_buffer_count)));
  return state;
}

void CommandBufferDirect::Flush(int32_t put_offset) {
  DCHECK(handler_);
  uint32_t order_num = 0;
  if (sync_point_manager_) {
    // If sync point manager is supported, assign order numbers to commands.
    if (paused_order_num_) {
      // Was previous paused, continue to process the order number.
      order_num = paused_order_num_;
      paused_order_num_ = 0;
    } else {
      order_num = sync_point_order_data_->GenerateUnprocessedOrderNumber();
    }
    sync_point_order_data_->BeginProcessingOrderNumber(order_num);
  }

  if (pause_commands_) {
    // Do not process commands, simply store the current order number.
    paused_order_num_ = order_num;

    sync_point_order_data_->PauseProcessingOrderNumber(order_num);
    return;
  }

  service_.Flush(put_offset, handler_);
  if (sync_point_manager_) {
    // Finish processing order number here.
    sync_point_order_data_->FinishProcessingOrderNumber(order_num);
  }
}

void CommandBufferDirect::OrderingBarrier(int32_t put_offset) {
  Flush(put_offset);
}

void CommandBufferDirect::SetGetBuffer(int32_t transfer_buffer_id) {
  service_.SetGetBuffer(transfer_buffer_id);
}

scoped_refptr<Buffer> CommandBufferDirect::CreateTransferBuffer(size_t size,
                                                                int32_t* id) {
  return service_.CreateTransferBuffer(size, id);
}

void CommandBufferDirect::DestroyTransferBuffer(int32_t id) {
  service_.DestroyTransferBuffer(id);
}

CommandBufferServiceClient::CommandBatchProcessedResult
CommandBufferDirect::OnCommandBatchProcessed() {
  return kContinueExecution;
}

void CommandBufferDirect::OnParseError() {}

void CommandBufferDirect::OnConsoleMessage(int32_t id,
                                           const std::string& message) {}

void CommandBufferDirect::CacheShader(const std::string& key,
                                      const std::string& shader) {}

void CommandBufferDirect::OnFenceSyncRelease(uint64_t release) {
  DCHECK(sync_point_client_state_);
  service_.SetReleaseCount(release);
  sync_point_client_state_->ReleaseFenceSync(release);
}

bool CommandBufferDirect::OnWaitSyncToken(const gpu::SyncToken& sync_token) {
  DCHECK(sync_point_manager_);
  if (sync_point_manager_->IsSyncTokenReleased(sync_token))
    return false;
  service_.SetScheduled(false);
  return true;
}

void CommandBufferDirect::OnDescheduleUntilFinished() {
  service_.SetScheduled(false);
}

void CommandBufferDirect::OnRescheduleAfterFinished() {
  service_.SetScheduled(true);
}

void CommandBufferDirect::OnSwapBuffers(uint64_t swap_id, uint32_t flags) {}

gpu::CommandBufferNamespace CommandBufferDirect::GetNamespaceID() const {
  return gpu::CommandBufferNamespace::IN_PROCESS;
}

CommandBufferId CommandBufferDirect::GetCommandBufferID() const {
  return command_buffer_id_;
}

void CommandBufferDirect::SetCommandsPaused(bool paused) {
  pause_commands_ = paused;
}

void CommandBufferDirect::SignalSyncToken(const gpu::SyncToken& sync_token,
                                          base::OnceClosure callback) {
  if (sync_point_manager_) {
    DCHECK(!paused_order_num_);
    uint32_t order_num =
        sync_point_order_data_->GenerateUnprocessedOrderNumber();
    sync_point_order_data_->BeginProcessingOrderNumber(order_num);
    base::RepeatingClosure maybe_pass_callback =
        base::AdaptCallbackForRepeating(std::move(callback));
    if (!sync_point_client_state_->Wait(sync_token, maybe_pass_callback))
      maybe_pass_callback.Run();
    sync_point_order_data_->FinishProcessingOrderNumber(order_num);
  } else {
    std::move(callback).Run();
  }
}

scoped_refptr<Buffer> CommandBufferDirect::CreateTransferBufferWithId(
    size_t size,
    int32_t id) {
  return service_.CreateTransferBufferWithId(size, id);
}

}  // namespace gpu
