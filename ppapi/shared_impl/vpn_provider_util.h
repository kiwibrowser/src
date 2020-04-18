// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_SHAERD_IMPL_VPN_PROVIDER_UTIL_H_
#define PPAPI_SHAERD_IMPL_VPN_PROVIDER_UTIL_H_

#include <memory>

#include "base/memory/shared_memory.h"
#include "ppapi/shared_impl/ppapi_shared_export.h"

namespace ppapi {

class PPAPI_SHARED_EXPORT VpnProviderSharedBuffer {
 public:
  VpnProviderSharedBuffer(uint32_t capacity,
                          uint32_t packet_size,
                          std::unique_ptr<base::SharedMemory> shm);
  ~VpnProviderSharedBuffer();

  bool GetAvailable(uint32_t* id);
  void SetAvailable(uint32_t id, bool value);
  void* GetBuffer(uint32_t id);
  base::SharedMemoryHandle GetHandle();
  uint32_t max_packet_size() { return max_packet_size_; }

 private:
  uint32_t capacity_;
  uint32_t max_packet_size_;
  std::unique_ptr<base::SharedMemory> shm_;
  std::vector<bool> available_;

  DISALLOW_COPY_AND_ASSIGN(VpnProviderSharedBuffer);
};

}  // namespace ppapi

#endif  // PPAPI_SHAERD_IMPL_VPN_PROVIDER_UTIL_H_
