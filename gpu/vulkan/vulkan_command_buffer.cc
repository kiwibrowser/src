// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/vulkan/vulkan_command_buffer.h"

#include "base/logging.h"
#include "gpu/vulkan/vulkan_command_pool.h"
#include "gpu/vulkan/vulkan_device_queue.h"

namespace gpu {

VulkanCommandBuffer::VulkanCommandBuffer(VulkanDeviceQueue* device_queue,
                                         VulkanCommandPool* command_pool,
                                         bool primary)
    : primary_(primary),
      device_queue_(device_queue),
      command_pool_(command_pool) {
  command_pool_->IncrementCommandBufferCount();
}

VulkanCommandBuffer::~VulkanCommandBuffer() {
  DCHECK_EQ(static_cast<VkCommandBuffer>(VK_NULL_HANDLE), command_buffer_);
  DCHECK_EQ(static_cast<VkFence>(VK_NULL_HANDLE), submission_fence_);
  DCHECK(!recording_);
  command_pool_->DecrementCommandBufferCount();
}

bool VulkanCommandBuffer::Initialize() {
  VkResult result = VK_SUCCESS;
  VkDevice device = device_queue_->GetVulkanDevice();

  VkCommandBufferAllocateInfo command_buffer_info = {};
  command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  command_buffer_info.commandPool = command_pool_->handle();
  command_buffer_info.level = primary_ ? VK_COMMAND_BUFFER_LEVEL_PRIMARY
                                       : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
  command_buffer_info.commandBufferCount = 1;

  result =
      vkAllocateCommandBuffers(device, &command_buffer_info, &command_buffer_);
  if (VK_SUCCESS != result) {
    DLOG(ERROR) << "vkAllocateCommandBuffers() failed: " << result;
    return false;
  }

  VkFenceCreateInfo fence_create_info = {};
  fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  result =
      vkCreateFence(device, &fence_create_info, nullptr, &submission_fence_);
  if (VK_SUCCESS != result) {
    DLOG(ERROR) << "vkCreateFence(submission) failed: " << result;
    return false;
  }

  record_type_ = RECORD_TYPE_EMPTY;
  return true;
}

void VulkanCommandBuffer::Destroy() {
  VkDevice device = device_queue_->GetVulkanDevice();
  if (VK_NULL_HANDLE != submission_fence_) {
    DCHECK(SubmissionFinished());
    vkDestroyFence(device, submission_fence_, nullptr);
    submission_fence_ = VK_NULL_HANDLE;
  }

  if (VK_NULL_HANDLE != command_buffer_) {
    vkFreeCommandBuffers(device, command_pool_->handle(), 1, &command_buffer_);
    command_buffer_ = VK_NULL_HANDLE;
  }
}

bool VulkanCommandBuffer::Submit(uint32_t num_wait_semaphores,
                                 VkSemaphore* wait_semaphores,
                                 uint32_t num_signal_semaphores,
                                 VkSemaphore* signal_semaphores) {
  VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

  DCHECK(primary_);
  VkSubmitInfo submit_info = {};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffer_;
  submit_info.waitSemaphoreCount = num_wait_semaphores;
  submit_info.pWaitSemaphores = wait_semaphores;
  submit_info.pWaitDstStageMask = &wait_dst_stage_mask;
  submit_info.signalSemaphoreCount = num_signal_semaphores;
  submit_info.pSignalSemaphores = signal_semaphores;

  VkResult result = VK_SUCCESS;

  result =
      vkResetFences(device_queue_->GetVulkanDevice(), 1, &submission_fence_);
  if (VK_SUCCESS != result) {
    DLOG(ERROR) << "vkResetFences() failed: " << result;
    return false;
  }

  result = vkQueueSubmit(device_queue_->GetVulkanQueue(), 1, &submit_info,
                         submission_fence_);

  PostExecution();
  if (VK_SUCCESS != result) {
    DLOG(ERROR) << "vkQueueSubmit() failed: " << result;
    return false;
  }

  return true;
}

void VulkanCommandBuffer::Enqueue(VkCommandBuffer primary_command_buffer) {
  DCHECK(!primary_);
  vkCmdExecuteCommands(primary_command_buffer, 1, &command_buffer_);
  PostExecution();
}

void VulkanCommandBuffer::Clear() {
  // Mark to reset upon next use.
  if (record_type_ != RECORD_TYPE_EMPTY)
    record_type_ = RECORD_TYPE_DIRTY;
}

void VulkanCommandBuffer::Wait(uint64_t timeout) {
  VkDevice device = device_queue_->GetVulkanDevice();
  vkWaitForFences(device, 1, &submission_fence_, true, timeout);
}

bool VulkanCommandBuffer::SubmissionFinished() {
  VkDevice device = device_queue_->GetVulkanDevice();
  return VK_SUCCESS == vkGetFenceStatus(device, submission_fence_);
}

void VulkanCommandBuffer::PostExecution() {
  if (record_type_ == RECORD_TYPE_SINGLE_USE) {
    // Clear upon next use.
    record_type_ = RECORD_TYPE_DIRTY;
  } else if (record_type_ == RECORD_TYPE_MULTI_USE) {
    // Can no longer record new items unless marked as clear.
    record_type_ = RECORD_TYPE_RECORDED;
  }
}

void VulkanCommandBuffer::ResetIfDirty() {
  DCHECK(!recording_);
  if (record_type_ == RECORD_TYPE_DIRTY) {
    // Block if command buffer is still in use. This can be externally avoided
    // using the asynchronous SubmissionFinished() function.
    VkDevice device = device_queue_->GetVulkanDevice();
    vkWaitForFences(device, 1, &submission_fence_, true, UINT64_MAX);
    VkResult result = vkResetCommandBuffer(command_buffer_, 0);
    if (VK_SUCCESS != result) {
      DLOG(ERROR) << "vkResetCommandBuffer() failed: " << result;
    } else {
      record_type_ = RECORD_TYPE_EMPTY;
    }
  }
}

CommandBufferRecorderBase::~CommandBufferRecorderBase() {
  VkResult result = vkEndCommandBuffer(handle_);
  if (VK_SUCCESS != result) {
    DLOG(ERROR) << "vkEndCommandBuffer() failed: " << result;
  }
};

ScopedMultiUseCommandBufferRecorder::ScopedMultiUseCommandBufferRecorder(
    VulkanCommandBuffer& command_buffer)
    : CommandBufferRecorderBase(command_buffer) {
  ValidateMultiUse(command_buffer);
  VkCommandBufferBeginInfo begin_info = {};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  VkResult result = vkBeginCommandBuffer(handle_, &begin_info);

  if (VK_SUCCESS != result) {
    DLOG(ERROR) << "vkBeginCommandBuffer() failed: " << result;
  }
}

ScopedSingleUseCommandBufferRecorder::ScopedSingleUseCommandBufferRecorder(
    VulkanCommandBuffer& command_buffer)
    : CommandBufferRecorderBase(command_buffer) {
  ValidateSingleUse(command_buffer);
  VkCommandBufferBeginInfo begin_info = {};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  VkResult result = vkBeginCommandBuffer(handle_, &begin_info);

  if (VK_SUCCESS != result) {
    DLOG(ERROR) << "vkBeginCommandBuffer() failed: " << result;
  }
}

}  // namespace gpu
