// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/gamepad/gamepad_shared_buffer.h"

namespace device {

GamepadSharedBuffer::GamepadSharedBuffer() {
  base::SharedMemoryCreateOptions options;
  options.size = sizeof(GamepadHardwareBuffer);
  options.share_read_only = true;
  bool res = shared_memory_.Create(options) && shared_memory_.Map(options.size);
  CHECK(res);

  void* mem = shared_memory_.memory();
  DCHECK(mem);
  hardware_buffer_ = new (mem) GamepadHardwareBuffer();
  memset(&(hardware_buffer_->data), 0, sizeof(Gamepads));
}

GamepadSharedBuffer::~GamepadSharedBuffer() = default;

base::SharedMemory* GamepadSharedBuffer::shared_memory() {
  return &shared_memory_;
}

Gamepads* GamepadSharedBuffer::buffer() {
  return &(hardware_buffer()->data);
}

GamepadHardwareBuffer* GamepadSharedBuffer::hardware_buffer() {
  return hardware_buffer_;
}

void GamepadSharedBuffer::WriteBegin() {
  hardware_buffer_->seqlock.WriteBegin();
}

void GamepadSharedBuffer::WriteEnd() {
  hardware_buffer_->seqlock.WriteEnd();
}

}  // namespace device
