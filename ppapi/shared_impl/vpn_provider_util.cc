// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/shared_impl/vpn_provider_util.h"

namespace ppapi {

VpnProviderSharedBuffer::VpnProviderSharedBuffer(
    uint32_t capacity,
    uint32_t packet_size,
    std::unique_ptr<base::SharedMemory> shm)
    : capacity_(capacity),
      max_packet_size_(packet_size),
      shm_(std::move(shm)),
      available_(capacity, true) {
  DCHECK(this->shm_);
}

VpnProviderSharedBuffer::~VpnProviderSharedBuffer() {}

bool VpnProviderSharedBuffer::GetAvailable(uint32_t* id) {
  for (uint32_t i = 0; i < capacity_; i++) {
    if (available_[i]) {
      if (id) {
        *id = i;
      }
      return true;
    }
  }
  return false;
}

void VpnProviderSharedBuffer::SetAvailable(uint32_t id, bool value) {
  if (id >= capacity_) {
    NOTREACHED();
    return;
  }
  available_[id] = value;
}

void* VpnProviderSharedBuffer::GetBuffer(uint32_t id) {
  if (id >= capacity_) {
    NOTREACHED();
    return nullptr;
  }
  return reinterpret_cast<char*>(shm_->memory()) + max_packet_size_ * id;
}

base::SharedMemoryHandle VpnProviderSharedBuffer::GetHandle() {
  return shm_->handle();
}

}  // namespace ppapi
