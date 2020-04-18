// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CRAZY_LINKER_MEMORY_MAPPING_H
#define CRAZY_LINKER_MEMORY_MAPPING_H

#include <errno.h>
#include <sys/mman.h>

#include "crazy_linker_debug.h"
#include "crazy_linker_error.h"

namespace crazy {

// Helper class for a memory mapping. This is _not_ scoped.
class MemoryMapping {
 public:
  enum Protection {
    CAN_READ = PROT_READ,
    CAN_WRITE = PROT_WRITE,
    CAN_READ_WRITE = PROT_READ | PROT_WRITE
  };
  MemoryMapping() : map_(NULL), size_(0) {}
  ~MemoryMapping() {}

  // Return current mapping address.
  void* Get() { return map_; }
  size_t GetSize() const { return size_; }

  // Allocate a new mapping.
  // |address| is either NULL or a fixed memory address.
  // |size| is the page-aligned size, must be > 0.
  // |prot| are the desired protection bit flags.
  // |fd| is -1 (for anonymous mappings), or a valid file descriptor.
  // on failure, return false and sets errno.
  bool Allocate(void* address, size_t size, Protection prot, int fd) {
    int flags = (fd >= 0) ? MAP_SHARED : MAP_ANONYMOUS;
    if (address)
      flags |= MAP_FIXED;

    size_ = size;
    map_ = ::mmap(address, size_, static_cast<int>(prot), flags, fd, 0);
    if (map_ == MAP_FAILED) {
      map_ = NULL;
      return false;
    }

    return true;
  }

  // Change the protection flags of the mapping.
  // On failure, return false and sets errno.
  bool SetProtection(Protection prot) {
    if (!map_ || ::mprotect(map_, size_, static_cast<int>(prot)) < 0)
      return false;
    return true;
  }

  // Deallocate an existing mapping, if any.
  void Deallocate() {
    if (map_) {
      ::munmap(map_, size_);
      map_ = NULL;
    }
  }

 protected:
  void* map_;
  size_t size_;
};

// Helper class for a memory mapping that is automatically
// unmapped on scope exit, unless its Release() method is called.
class ScopedMemoryMapping : public MemoryMapping {
 public:
  void* Release() {
    void* ret = map_;
    map_ = NULL;
    return ret;
  }

  ~ScopedMemoryMapping() { Deallocate(); }
};

}  // namespace crazy

#endif  // CRAZY_LINKER_MEMORY_MAPPING_H
