// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/capture/video/shared_memory_handle_provider.h"

namespace media {

SharedMemoryHandleProvider::SharedMemoryHandleProvider() {
#if DCHECK_IS_ON()
  map_ref_count_ = 0;
#endif
}

SharedMemoryHandleProvider::~SharedMemoryHandleProvider() {
  base::AutoLock lock(mapping_lock_);

  // If the tracker is being destroyed, there must be no outstanding
  // Handles. If this DCHECK() triggers, it means that either there is a logic
  // flaw in VideoCaptureBufferPoolImpl, or a client did not delete all of its
  // owned VideoCaptureBufferHandles before calling Pool::ReliquishXYZ().
#if DCHECK_IS_ON()
  DCHECK_EQ(map_ref_count_, 0);
#endif

  if (shared_memory_ && shared_memory_->memory()) {
    DVLOG(3) << __func__ << ": Unmapping memory for in-process access @"
             << shared_memory_->memory() << '.';
    CHECK(shared_memory_->Unmap());
  }
}

bool SharedMemoryHandleProvider::InitForSize(size_t size) {
#if DCHECK_IS_ON()
  DCHECK_EQ(map_ref_count_, 0);
#endif
  DCHECK(!shared_memory_);
  shared_memory_.emplace();
  if (shared_memory_->CreateAnonymous(size)) {
    mapped_size_ = size;
    read_only_flag_ = false;
    return true;
  }
  return false;
}

bool SharedMemoryHandleProvider::InitFromMojoHandle(
    mojo::ScopedSharedBufferHandle buffer_handle) {
#if DCHECK_IS_ON()
  DCHECK_EQ(map_ref_count_, 0);
#endif
  DCHECK(!shared_memory_);

  base::SharedMemoryHandle memory_handle;
  mojo::UnwrappedSharedMemoryHandleProtection protection;
  const MojoResult result = mojo::UnwrapSharedMemoryHandle(
      std::move(buffer_handle), &memory_handle, &mapped_size_, &protection);
  if (result != MOJO_RESULT_OK)
    return false;
  read_only_flag_ =
      protection == mojo::UnwrappedSharedMemoryHandleProtection::kReadOnly;
  shared_memory_.emplace(memory_handle, read_only_flag_);
  return true;
}

mojo::ScopedSharedBufferHandle
SharedMemoryHandleProvider::GetHandleForInterProcessTransit(bool read_only) {
  if (read_only_flag_ && !read_only) {
    // Wanted read-write access, but read-only access is all that is available.
    NOTREACHED();
    return mojo::ScopedSharedBufferHandle();
  }
  // TODO(https://crbug.com/803136): This does not actually obey |read_only| in
  // any capacity because it uses DuplicateHandle. In order to properly obey
  // |read_only| (when true), we need to use |SharedMemory::GetReadOnlyHandle()|
  // but that is not possible. With the base::SharedMemory API and this
  // SharedMemoryHandleProvider API as they are today, it isn't possible to know
  // whether |shared_memory_| even supports read-only duplication. Note that
  // changing |kReadWrite| to |kReadOnly| does NOT affect the ability to map
  // the handle read-write.
  return mojo::WrapSharedMemoryHandle(
      base::SharedMemory::DuplicateHandle(shared_memory_->handle()),
      mapped_size_, mojo::UnwrappedSharedMemoryHandleProtection::kReadWrite);
}

base::SharedMemoryHandle
SharedMemoryHandleProvider::GetNonOwnedSharedMemoryHandleForLegacyIPC() {
  return shared_memory_->handle();
}

std::unique_ptr<VideoCaptureBufferHandle>
SharedMemoryHandleProvider::GetHandleForInProcessAccess() {
  {
    base::AutoLock lock(mapping_lock_);
#if DCHECK_IS_ON()
    DCHECK_GE(map_ref_count_, 0);
    ++map_ref_count_;
#endif
    if (!shared_memory_->memory()) {
      CHECK(shared_memory_->Map(mapped_size_));
      DVLOG(3) << __func__ << ": Mapped memory for in-process access @"
               << shared_memory_->memory() << '.';
    }
  }

  return std::make_unique<Handle>(this);
}

#if DCHECK_IS_ON()
void SharedMemoryHandleProvider::OnHandleDestroyed() {
  base::AutoLock lock(mapping_lock_);
  DCHECK_GT(map_ref_count_, 0);
  --map_ref_count_;
}
#endif

SharedMemoryHandleProvider::Handle::Handle(SharedMemoryHandleProvider* owner)
    : owner_(owner) {}

SharedMemoryHandleProvider::Handle::~Handle() {
#if DCHECK_IS_ON()
  owner_->OnHandleDestroyed();
#endif
}

size_t SharedMemoryHandleProvider::Handle::mapped_size() const {
  return owner_->mapped_size_;
}

uint8_t* SharedMemoryHandleProvider::Handle::data() const {
  return static_cast<uint8_t*>(owner_->shared_memory_->memory());
}

const uint8_t* SharedMemoryHandleProvider::Handle::const_data() const {
  return static_cast<const uint8_t*>(owner_->shared_memory_->memory());
}

}  // namespace media
