// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/capture/video/video_capture_buffer_pool_impl.h"

#include <memory>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "build/build_config.h"
#include "media/capture/video/video_capture_buffer_handle.h"
#include "media/capture/video/video_capture_buffer_tracker.h"
#include "ui/gfx/buffer_format_util.h"

namespace media {

VideoCaptureBufferPoolImpl::VideoCaptureBufferPoolImpl(
    std::unique_ptr<VideoCaptureBufferTrackerFactory> buffer_tracker_factory,
    int count)
    : count_(count),
      next_buffer_id_(0),
      last_relinquished_buffer_id_(kInvalidId),
      buffer_tracker_factory_(std::move(buffer_tracker_factory)) {
  DCHECK_GT(count, 0);
}

VideoCaptureBufferPoolImpl::~VideoCaptureBufferPoolImpl() = default;

mojo::ScopedSharedBufferHandle
VideoCaptureBufferPoolImpl::GetHandleForInterProcessTransit(int buffer_id,
                                                            bool read_only) {
  base::AutoLock lock(lock_);

  VideoCaptureBufferTracker* tracker = GetTracker(buffer_id);
  if (!tracker) {
    NOTREACHED() << "Invalid buffer_id.";
    return mojo::ScopedSharedBufferHandle();
  }
  return tracker->GetHandleForTransit(read_only);
}

base::SharedMemoryHandle
VideoCaptureBufferPoolImpl::GetNonOwnedSharedMemoryHandleForLegacyIPC(
    int buffer_id) {
  base::AutoLock lock(lock_);

  VideoCaptureBufferTracker* tracker = GetTracker(buffer_id);
  if (!tracker) {
    NOTREACHED() << "Invalid buffer_id.";
    return base::SharedMemoryHandle();
  }
  return tracker->GetNonOwnedSharedMemoryHandleForLegacyIPC();
}

std::unique_ptr<VideoCaptureBufferHandle>
VideoCaptureBufferPoolImpl::GetHandleForInProcessAccess(int buffer_id) {
  base::AutoLock lock(lock_);

  VideoCaptureBufferTracker* tracker = GetTracker(buffer_id);
  if (!tracker) {
    NOTREACHED() << "Invalid buffer_id.";
    return nullptr;
  }

  return tracker->GetMemoryMappedAccess();
}

int VideoCaptureBufferPoolImpl::ReserveForProducer(const gfx::Size& dimensions,
                                                   VideoPixelFormat format,
                                                   int frame_feedback_id,
                                                   int* buffer_id_to_drop) {
  base::AutoLock lock(lock_);
  return ReserveForProducerInternal(dimensions, format, frame_feedback_id,
                                    buffer_id_to_drop);
}

void VideoCaptureBufferPoolImpl::RelinquishProducerReservation(int buffer_id) {
  base::AutoLock lock(lock_);
  VideoCaptureBufferTracker* tracker = GetTracker(buffer_id);
  if (!tracker) {
    NOTREACHED() << "Invalid buffer_id.";
    return;
  }
  DCHECK(tracker->held_by_producer());
  tracker->set_held_by_producer(false);
  last_relinquished_buffer_id_ = buffer_id;
}

void VideoCaptureBufferPoolImpl::HoldForConsumers(int buffer_id,
                                                  int num_clients) {
  base::AutoLock lock(lock_);
  VideoCaptureBufferTracker* tracker = GetTracker(buffer_id);
  if (!tracker) {
    NOTREACHED() << "Invalid buffer_id.";
    return;
  }
  DCHECK(tracker->held_by_producer());
  DCHECK(!tracker->consumer_hold_count());

  tracker->set_consumer_hold_count(num_clients);
  // Note: |held_by_producer()| will stay true until
  // RelinquishProducerReservation() (usually called by destructor of the object
  // wrapping this tracker, e.g. a VideoFrame).
}

void VideoCaptureBufferPoolImpl::RelinquishConsumerHold(int buffer_id,
                                                        int num_clients) {
  base::AutoLock lock(lock_);
  VideoCaptureBufferTracker* tracker = GetTracker(buffer_id);
  if (!tracker) {
    NOTREACHED() << "Invalid buffer_id.";
    return;
  }
  DCHECK_GE(tracker->consumer_hold_count(), num_clients);

  tracker->set_consumer_hold_count(tracker->consumer_hold_count() -
                                   num_clients);
}

int VideoCaptureBufferPoolImpl::ResurrectLastForProducer(
    const gfx::Size& dimensions,
    VideoPixelFormat format) {
  base::AutoLock lock(lock_);

  // Return early if the last relinquished buffer has been re-used already.
  if (last_relinquished_buffer_id_ == kInvalidId)
    return kInvalidId;

  // If there are no consumers reading from this buffer, then it's safe to
  // provide this buffer back to the producer (because the producer may
  // potentially modify the content). Check that the expected dimensions,
  // and format match.
  auto it = trackers_.find(last_relinquished_buffer_id_);
  DCHECK(it != trackers_.end());
  DCHECK(!it->second->held_by_producer());
  if (it->second->consumer_hold_count() == 0 &&
      it->second->dimensions() == dimensions &&
      it->second->pixel_format() == format) {
    it->second->set_held_by_producer(true);
    const int resurrected_buffer_id = last_relinquished_buffer_id_;
    last_relinquished_buffer_id_ = kInvalidId;
    return resurrected_buffer_id;
  }

  return kInvalidId;
}

double VideoCaptureBufferPoolImpl::GetBufferPoolUtilization() const {
  base::AutoLock lock(lock_);
  int num_buffers_held = 0;
  for (const auto& entry : trackers_) {
    VideoCaptureBufferTracker* const tracker = entry.second.get();
    if (tracker->held_by_producer() || tracker->consumer_hold_count() > 0)
      ++num_buffers_held;
  }
  return static_cast<double>(num_buffers_held) / count_;
}

int VideoCaptureBufferPoolImpl::ReserveForProducerInternal(
    const gfx::Size& dimensions,
    VideoPixelFormat pixel_format,
    int frame_feedback_id,
    int* buffer_id_to_drop) {
  lock_.AssertAcquired();

  const size_t size_in_pixels = dimensions.GetArea();
  // Look for a tracker that's allocated, big enough, and not in use. Track the
  // largest one that's not big enough, in case we have to reallocate a tracker.
  *buffer_id_to_drop = kInvalidId;
  size_t largest_size_in_pixels = 0;
  auto tracker_of_last_resort = trackers_.end();
  auto tracker_to_drop = trackers_.end();
  for (auto it = trackers_.begin(); it != trackers_.end(); ++it) {
    VideoCaptureBufferTracker* const tracker = it->second.get();
    if (!tracker->consumer_hold_count() && !tracker->held_by_producer()) {
      if (tracker->max_pixel_count() >= size_in_pixels &&
          (tracker->pixel_format() == pixel_format)) {
        if (it->first == last_relinquished_buffer_id_) {
          // This buffer would do just fine, but avoid returning it because the
          // client may want to resurrect it. It will be returned perforce if
          // the pool has reached it's maximum limit (see code below).
          tracker_of_last_resort = it;
          continue;
        }
        // Existing tracker is big enough and has correct format. Reuse it.
        tracker->set_dimensions(dimensions);
        tracker->set_held_by_producer(true);
        tracker->set_frame_feedback_id(frame_feedback_id);
        return it->first;
      }
      if (tracker->max_pixel_count() > largest_size_in_pixels) {
        largest_size_in_pixels = tracker->max_pixel_count();
        tracker_to_drop = it;
      }
    }
  }

  // Preferably grow the pool by creating a new tracker. If we're at maximum
  // size, then try using |tracker_of_last_resort| or reallocate by deleting an
  // existing one instead.
  if (trackers_.size() == static_cast<size_t>(count_)) {
    if (tracker_of_last_resort != trackers_.end()) {
      last_relinquished_buffer_id_ = kInvalidId;
      tracker_of_last_resort->second->set_dimensions(dimensions);
      tracker_of_last_resort->second->set_held_by_producer(true);
      tracker_of_last_resort->second->set_frame_feedback_id(frame_feedback_id);
      return tracker_of_last_resort->first;
    }
    if (tracker_to_drop == trackers_.end()) {
      // We're out of space, and can't find an unused tracker to reallocate.
      return kInvalidId;
    }
    if (tracker_to_drop->first == last_relinquished_buffer_id_)
      last_relinquished_buffer_id_ = kInvalidId;
    *buffer_id_to_drop = tracker_to_drop->first;
    trackers_.erase(tracker_to_drop);
  }

  // Create the new tracker.
  const int buffer_id = next_buffer_id_++;

  std::unique_ptr<VideoCaptureBufferTracker> tracker =
      buffer_tracker_factory_->CreateTracker();
  if (!tracker->Init(dimensions, pixel_format)) {
    DLOG(ERROR) << "Error initializing VideoCaptureBufferTracker";
    return kInvalidId;
  }

  tracker->set_held_by_producer(true);
  tracker->set_frame_feedback_id(frame_feedback_id);
  trackers_[buffer_id] = std::move(tracker);

  return buffer_id;
}

VideoCaptureBufferTracker* VideoCaptureBufferPoolImpl::GetTracker(
    int buffer_id) {
  auto it = trackers_.find(buffer_id);
  return (it == trackers_.end()) ? nullptr : it->second.get();
}

}  // namespace media
