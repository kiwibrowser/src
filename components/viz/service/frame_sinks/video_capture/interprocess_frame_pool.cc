// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/frame_sinks/video_capture/interprocess_frame_pool.h"

#include <algorithm>

#include "base/bind.h"
#include "base/bind_helpers.h"

using media::VideoFrame;
using media::VideoPixelFormat;

namespace viz {

// static
constexpr base::TimeDelta InterprocessFramePool::kMinLoggingPeriod;

InterprocessFramePool::InterprocessFramePool(int capacity)
    : capacity_(std::max(capacity, 0)), weak_factory_(this) {
  DCHECK_GT(capacity_, 0u);
}

InterprocessFramePool::~InterprocessFramePool() = default;

scoped_refptr<VideoFrame> InterprocessFramePool::ReserveVideoFrame(
    VideoPixelFormat format,
    const gfx::Size& size) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Calling this method is a signal that there is no intention of resurrecting
  // the last frame.
  resurrectable_handle_ = MOJO_HANDLE_INVALID;

  const size_t bytes_required = VideoFrame::AllocationSize(format, size);

  // Look for an available buffer that's large enough. If one is found, wrap it
  // in a VideoFrame and return it.
  for (auto it = available_buffers_.rbegin(); it != available_buffers_.rend();
       ++it) {
    if (it->bytes_allocated < bytes_required) {
      continue;
    }
    PooledBuffer taken = std::move(*it);
    available_buffers_.erase(it.base() - 1);
    return WrapBuffer(std::move(taken), format, size);
  }

  // Look for the largest available buffer, reallocate it, wrap it in a
  // VideoFrame and return it.
  while (!available_buffers_.empty()) {
    const auto it =
        std::max_element(available_buffers_.rbegin(), available_buffers_.rend(),
                         [this](const PooledBuffer& a, const PooledBuffer& b) {
                           return a.bytes_allocated < b.bytes_allocated;
                         });
    available_buffers_.erase(it.base() - 1);  // Release before allocating more.
    PooledBuffer reallocated;
    reallocated.buffer = mojo::SharedBufferHandle::Create(bytes_required);
    if (!reallocated.buffer.is_valid()) {
      LOG_IF(WARNING, CanLogSharedMemoryFailure())
          << "Failed to re-allocate " << bytes_required << " bytes.";
      continue;  // Try again after freeing the next-largest buffer.
    }
    reallocated.bytes_allocated = bytes_required;
    return WrapBuffer(std::move(reallocated), format, size);
  }

  // There are no available buffers. If the pool is at max capacity, punt.
  // Otherwise, allocate a new buffer, wrap it in a VideoFrame and return it.
  if (utilized_buffers_.size() >= capacity_) {
    return nullptr;
  }
  PooledBuffer additional;
  additional.buffer = mojo::SharedBufferHandle::Create(bytes_required);
  if (!additional.buffer.is_valid()) {
    LOG_IF(WARNING, CanLogSharedMemoryFailure())
        << "Failed to allocate " << bytes_required << " bytes.";
    return nullptr;
  }
  additional.bytes_allocated = bytes_required;
  return WrapBuffer(std::move(additional), format, size);
}

scoped_refptr<VideoFrame> InterprocessFramePool::ResurrectLastVideoFrame(
    VideoPixelFormat expected_format,
    const gfx::Size& expected_size) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Find the tracking entry for the resurrectable buffer. If it is still being
  // used, or is not of the expected format and size, punt.
  if (resurrectable_handle_ == MOJO_HANDLE_INVALID ||
      last_delivered_format_ != expected_format ||
      last_delivered_size_ != expected_size) {
    return nullptr;
  }
  const auto it = std::find_if(
      available_buffers_.rbegin(), available_buffers_.rend(),
      [this](const PooledBuffer& candidate) {
        return candidate.buffer.get().value() == resurrectable_handle_;
      });
  if (it == available_buffers_.rend()) {
    return nullptr;
  }

  // Wrap the buffer in a VideoFrame and return it.
  PooledBuffer resurrected = std::move(*it);
  available_buffers_.erase(it.base() - 1);
  return WrapBuffer(std::move(resurrected), expected_format, expected_size);
}

InterprocessFramePool::BufferAndSize
InterprocessFramePool::CloneHandleForDelivery(const VideoFrame* frame) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Record that this frame is the last-delivered one, for possible future calls
  // to ResurrectLastVideoFrame().
  const auto it = utilized_buffers_.find(frame);
  DCHECK(it != utilized_buffers_.end());
  resurrectable_handle_ = it->second.buffer.get().value();
  last_delivered_format_ = frame->format();
  last_delivered_size_ = frame->coded_size();

  return BufferAndSize(it->second.buffer->Clone(
                           mojo::SharedBufferHandle::AccessMode::READ_WRITE),
                       it->second.bytes_allocated);
}

float InterprocessFramePool::GetUtilization() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  return static_cast<float>(utilized_buffers_.size()) / capacity_;
}

scoped_refptr<VideoFrame> InterprocessFramePool::WrapBuffer(
    PooledBuffer pooled_buffer,
    VideoPixelFormat format,
    const gfx::Size& size) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(pooled_buffer.buffer.is_valid());

  // Map the shared memory into the current process, or move an existing
  // "scoped mapping" into the local variable. The mapping will be owned by the
  // VideoFrame wrapper (below) until the VideoFrame goes out-of-scope; and this
  // might be after InterprocessFramePool is destroyed.
  mojo::ScopedSharedBufferMapping mapping;
  if (pooled_buffer.mapping) {
    mapping = std::move(pooled_buffer.mapping);
  } else {
    mapping = pooled_buffer.buffer->Map(pooled_buffer.bytes_allocated);
    if (!mapping) {
      LOG_IF(WARNING, CanLogSharedMemoryFailure())
          << "Failed to map shared memory to back the VideoFrame ("
          << pooled_buffer.bytes_allocated << " bytes).";
      // The shared memory will be freed when pooled_buffer goes out-of-scope
      // here:
      return nullptr;
    }
  }

  // Create the VideoFrame wrapper, add tracking in |utilized_buffers_|, and add
  // a destruction observer to return the buffer to the pool once the VideoFrame
  // goes out-of-scope.
  //
  // The VideoFrame could be held, externally, beyond the lifetime of this
  // InterprocessFramePool. However, this is safe because 1) the use of a
  // WeakPtr cancels the callback that would return the buffer back to the pool,
  // and 2) the mapped memory remains valid until the ScopedSharedBufferMapping
  // goes out-of-scope (when the OnceClosure is destroyed).
  scoped_refptr<VideoFrame> frame = VideoFrame::WrapExternalData(
      format, size, gfx::Rect(size), size, static_cast<uint8_t*>(mapping.get()),
      pooled_buffer.bytes_allocated, base::TimeDelta());
  DCHECK(frame);
  utilized_buffers_.emplace(frame.get(), std::move(pooled_buffer));
  frame->AddDestructionObserver(
      base::BindOnce(&InterprocessFramePool::OnFrameWrapperDestroyed,
                     weak_factory_.GetWeakPtr(), base::Unretained(frame.get()),
                     std::move(mapping)));
  return frame;
}

void InterprocessFramePool::OnFrameWrapperDestroyed(
    const VideoFrame* frame,
    mojo::ScopedSharedBufferMapping mapping) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Return the buffer to the pool by moving the PooledBuffer back into
  // |available_buffers_|.
  const auto it = utilized_buffers_.find(frame);
  DCHECK(it != utilized_buffers_.end());
  available_buffers_.emplace_back(std::move(it->second));
  available_buffers_.back().mapping = std::move(mapping);
  utilized_buffers_.erase(it);
  DCHECK_LE(available_buffers_.size() + utilized_buffers_.size(), capacity_);
}

bool InterprocessFramePool::CanLogSharedMemoryFailure() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const base::TimeTicks now = base::TimeTicks::Now();
  if ((now - last_fail_log_time_) >= kMinLoggingPeriod) {
    last_fail_log_time_ = now;
    return true;
  }
  return false;
}

InterprocessFramePool::PooledBuffer::PooledBuffer() = default;
InterprocessFramePool::PooledBuffer::PooledBuffer(
    PooledBuffer&& other) noexcept = default;
InterprocessFramePool::PooledBuffer& InterprocessFramePool::PooledBuffer::
operator=(PooledBuffer&& other) noexcept = default;
InterprocessFramePool::PooledBuffer::~PooledBuffer() = default;

}  // namespace viz
