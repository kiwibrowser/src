// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAPTURE_VIDEO_SHARED_MEMORY_HANDLE_PROVIDER_H_
#define MEDIA_CAPTURE_VIDEO_SHARED_MEMORY_HANDLE_PROVIDER_H_

#include <memory>

#include "base/logging.h"
#include "base/memory/shared_memory.h"
#include "base/optional.h"
#include "media/capture/capture_export.h"
#include "media/capture/video/video_capture_buffer_handle.h"
#include "media/capture/video/video_capture_device.h"
#include "mojo/public/cpp/system/platform_handle.h"

namespace media {

// Provides handles from a single, owned base::SharedMemory instance.
class CAPTURE_EXPORT SharedMemoryHandleProvider
    : public VideoCaptureDevice::Client::Buffer::HandleProvider {
 public:
  // Note: One of the two InitXYZ() methods must be called before using any of
  // the HandleProvider methods.
  SharedMemoryHandleProvider();

  ~SharedMemoryHandleProvider() override;

  // Initialize by creating anonymous shared memory of the given |size|. Returns
  // false if the operation failed.
  bool InitForSize(size_t size);

  // Initialize by duplicating an existing mojo |buffer_handle|. Returns false
  // if the operation failed.
  bool InitFromMojoHandle(mojo::ScopedSharedBufferHandle buffer_handle);

  // Implementation of Buffer::HandleProvider:
  mojo::ScopedSharedBufferHandle GetHandleForInterProcessTransit(
      bool read_only) override;
  base::SharedMemoryHandle GetNonOwnedSharedMemoryHandleForLegacyIPC() override;
  std::unique_ptr<VideoCaptureBufferHandle> GetHandleForInProcessAccess()
      override;

 private:
  // Accessor to mapped memory. When the first of these is created, the shared
  // memory is mapped. The unmapping, however, does not occur until the
  // SharedMemoryHandleProvider is destroyed. Therefore, the provider must
  // outlive all of its Handles.
  class Handle : public VideoCaptureBufferHandle {
   public:
    explicit Handle(SharedMemoryHandleProvider* owner);
    ~Handle() final;

    size_t mapped_size() const final;
    uint8_t* data() const final;
    const uint8_t* const_data() const final;

   private:
    SharedMemoryHandleProvider* const owner_;
  };

#if DCHECK_IS_ON()
  // Called by Handle to decrement |map_ref_count_|. This is thread-safe.
  void OnHandleDestroyed();
#endif

  // These are set by one of the InitXYZ() methods.
  base::Optional<base::SharedMemory> shared_memory_;
  size_t mapped_size_;
  bool read_only_flag_;

  // Synchronizes changes to |map_ref_count_| and Map() and Unmap() operations
  // on |shared_memory_|. This is because the thread that calls
  // GetHandleForInProcessAccess() may pass ownership of the returned Handle to
  // code that runs on a diffrent thread.
  base::Lock mapping_lock_;

#if DCHECK_IS_ON()
  // The number of Handle instances that are referencing the mapped memory. This
  // is only used while DCHECKs are turned on, as a sanity-check that the object
  // graph/lifetimes have not changed in a bad way.
  int map_ref_count_;
#endif

  DISALLOW_COPY_AND_ASSIGN(SharedMemoryHandleProvider);
};

}  // namespace media

#endif  // MEDIA_CAPTURE_VIDEO_SHARED_MEMORY_HANDLE_PROVIDER_H_
