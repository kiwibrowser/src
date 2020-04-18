// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/user_input_monitor.h"

#include <utility>

#include "mojo/public/cpp/system/platform_handle.h"

namespace audio {

// static
std::unique_ptr<UserInputMonitor> UserInputMonitor::Create(
    mojo::ScopedSharedBufferHandle handle) {
  base::ReadOnlySharedMemoryRegion memory =
      mojo::UnwrapReadOnlySharedMemoryRegion(std::move(handle));
  if (memory.IsValid())
    return std::make_unique<UserInputMonitor>(memory.Map());

  return nullptr;
}

UserInputMonitor::UserInputMonitor(
    base::ReadOnlySharedMemoryMapping memory_mapping)
    : key_press_count_mapping_(std::move(memory_mapping)) {}

UserInputMonitor::~UserInputMonitor() = default;

void UserInputMonitor::EnableKeyPressMonitoring() {}

void UserInputMonitor::DisableKeyPressMonitoring() {}

uint32_t UserInputMonitor::GetKeyPressCount() const {
  return media::ReadKeyPressMonitorCount(key_press_count_mapping_);
}

}  // namespace audio
