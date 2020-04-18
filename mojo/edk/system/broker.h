// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_EDK_SYSTEM_BROKER_H_
#define MOJO_EDK_SYSTEM_BROKER_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/writable_shared_memory_region.h"
#include "base/synchronization/lock.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"

namespace mojo {
namespace edk {

// The Broker is a channel to the broker process, which allows synchronous IPCs
// to fulfill shared memory allocation requests on some platforms.
class Broker {
 public:
  // Note: This is blocking, and will wait for the first message over
  // |platform_handle|.
  explicit Broker(ScopedInternalPlatformHandle platform_handle);
  ~Broker();

  // Returns the platform handle that should be used to establish a NodeChannel
  // to the process which is inviting us to join its network. This is the first
  // handle read off the Broker channel upon construction.
  ScopedInternalPlatformHandle GetInviterInternalPlatformHandle();

  // Request a shared buffer from the broker process. Blocks the current thread.
  base::WritableSharedMemoryRegion GetWritableSharedMemoryRegion(
      size_t num_bytes);

 private:
  // Handle to the broker process, used for synchronous IPCs.
  ScopedInternalPlatformHandle sync_channel_;

  // Handle to the inviter process which is recieved in the first first message
  // over |sync_channel_|.
  ScopedInternalPlatformHandle inviter_channel_;

  // Lock to only allow one sync message at a time. This avoids having to deal
  // with message ordering since we can only have one request at a time
  // in-flight.
  base::Lock lock_;

  DISALLOW_COPY_AND_ASSIGN(Broker);
};

}  // namespace edk
}  // namespace mojo

#endif  // MOJO_EDK_SYSTEM_BROKER_H_
