// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_AUDIO_USER_INPUT_MONITOR_H_
#define SERVICES_AUDIO_USER_INPUT_MONITOR_H_

#include <memory>

#include "base/memory/shared_memory_mapping.h"
#include "media/base/user_input_monitor.h"
#include "mojo/public/cpp/system/buffer.h"

namespace audio {

// TODO(https://crbug.com/836226) remove inheritance after switching to audio
// service input streams.
class UserInputMonitor : public media::UserInputMonitor {
 public:
  explicit UserInputMonitor(base::ReadOnlySharedMemoryMapping memory_mapping);
  ~UserInputMonitor() override;

  // Returns nullptr for invalid handle.
  static std::unique_ptr<UserInputMonitor> Create(
      mojo::ScopedSharedBufferHandle keypress_count_buffer);

  void EnableKeyPressMonitoring() override;
  void DisableKeyPressMonitoring() override;
  uint32_t GetKeyPressCount() const override;

 private:
  base::ReadOnlySharedMemoryMapping key_press_count_mapping_;

  DISALLOW_COPY_AND_ASSIGN(UserInputMonitor);
};

}  // namespace audio

#endif  // SERVICES_AUDIO_USER_INPUT_MONITOR_H_
