// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/ipc/service/gpu_memory_manager.h"

#include <algorithm>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/process/process_handle.h"
#include "base/strings/string_number_conversions.h"
#include "base/trace_event/trace_event.h"
#include "gpu/command_buffer/common/gpu_memory_allocation.h"
#include "gpu/command_buffer/service/gpu_switches.h"
#include "gpu/ipc/common/memory_stats.h"
#include "gpu/ipc/service/gpu_channel_manager.h"
#include "gpu/ipc/service/gpu_channel_manager_delegate.h"
#include "gpu/ipc/service/gpu_memory_tracking.h"

namespace gpu {
namespace {

void TrackValueChanged(uint64_t old_size,
                       uint64_t new_size,
                       uint64_t* total_size) {
  DCHECK(new_size > old_size || *total_size >= (old_size - new_size));
  *total_size += (new_size - old_size);
}

}

GpuMemoryManager::GpuMemoryManager(GpuChannelManager* channel_manager)
    : channel_manager_(channel_manager),
      bytes_allocated_current_(0) {}

GpuMemoryManager::~GpuMemoryManager() {
  DCHECK(tracking_groups_.empty());
  DCHECK(!bytes_allocated_current_);
}

void GpuMemoryManager::TrackMemoryAllocatedChange(
    GpuMemoryTrackingGroup* tracking_group,
    uint64_t old_size,
    uint64_t new_size) {
  TrackValueChanged(old_size, new_size, &tracking_group->size_);
  TrackValueChanged(old_size, new_size, &bytes_allocated_current_);
}

bool GpuMemoryManager::EnsureGPUMemoryAvailable(uint64_t /* size_needed */) {
  // TODO: Check if there is enough space. Lose contexts until there is.
  return true;
}

uint64_t GpuMemoryManager::GetTrackerMemoryUsage(
    gles2::MemoryTracker* tracker) const {
  TrackingGroupMap::const_iterator tracking_group_it =
      tracking_groups_.find(tracker);
  DCHECK(tracking_group_it != tracking_groups_.end());
  return tracking_group_it->second->GetSize();
}

GpuMemoryTrackingGroup* GpuMemoryManager::CreateTrackingGroup(
    base::ProcessId pid, gles2::MemoryTracker* memory_tracker) {
  GpuMemoryTrackingGroup* tracking_group = new GpuMemoryTrackingGroup(
      pid, memory_tracker, this);
  DCHECK(!tracking_groups_.count(tracking_group->GetMemoryTracker()));
  tracking_groups_.insert(std::make_pair(tracking_group->GetMemoryTracker(),
                                         tracking_group));
  return tracking_group;
}

void GpuMemoryManager::OnDestroyTrackingGroup(
    GpuMemoryTrackingGroup* tracking_group) {
  DCHECK(tracking_groups_.count(tracking_group->GetMemoryTracker()));
  tracking_groups_.erase(tracking_group->GetMemoryTracker());
}

void GpuMemoryManager::GetVideoMemoryUsageStats(
    VideoMemoryUsageStats* video_memory_usage_stats) const {
  // For each context group, assign its memory usage to its PID
  video_memory_usage_stats->process_map.clear();
  for (TrackingGroupMap::const_iterator i =
       tracking_groups_.begin(); i != tracking_groups_.end(); ++i) {
    const GpuMemoryTrackingGroup* tracking_group = i->second;
    video_memory_usage_stats->process_map[
        tracking_group->GetPid()].video_memory += tracking_group->GetSize();
  }

  // Assign the total across all processes in the GPU process
  video_memory_usage_stats->process_map[
      base::GetCurrentProcId()].video_memory = GetCurrentUsage();
  video_memory_usage_stats->process_map[
      base::GetCurrentProcId()].has_duplicates = true;

  video_memory_usage_stats->bytes_allocated = GetCurrentUsage();
}

}  // namespace gpu
