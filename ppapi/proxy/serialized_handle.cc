// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/serialized_handle.h"

#include "base/files/file.h"
#include "base/memory/shared_memory.h"
#include "base/pickle.h"
#include "build/build_config.h"
#include "ipc/ipc_platform_file.h"

#if defined(OS_NACL)
#include <unistd.h>
#endif

namespace ppapi {
namespace proxy {

SerializedHandle::SerializedHandle()
    : type_(INVALID),
      size_(0),
      descriptor_(IPC::InvalidPlatformFileForTransit()),
      open_flags_(0),
      file_io_(0) {
}

SerializedHandle::SerializedHandle(Type type_param)
    : type_(type_param),
      size_(0),
      descriptor_(IPC::InvalidPlatformFileForTransit()),
      open_flags_(0),
      file_io_(0) {
}

SerializedHandle::SerializedHandle(const base::SharedMemoryHandle& handle,
                                   uint32_t size)
    : type_(SHARED_MEMORY),
      shm_handle_(handle),
      size_(size),
      descriptor_(IPC::InvalidPlatformFileForTransit()),
      open_flags_(0),
      file_io_(0) {}

SerializedHandle::SerializedHandle(
    Type type,
    const IPC::PlatformFileForTransit& socket_descriptor)
    : type_(type),
      size_(0),
      descriptor_(socket_descriptor),
      open_flags_(0),
      file_io_(0) {
}

bool SerializedHandle::IsHandleValid() const {
  switch (type_) {
    case SHARED_MEMORY:
      return base::SharedMemory::IsHandleValid(shm_handle_);
    case SOCKET:
    case FILE:
      return !(IPC::InvalidPlatformFileForTransit() == descriptor_);
    case INVALID:
      return false;
    // No default so the compiler will warn us if a new type is added.
  }
  return false;
}

void SerializedHandle::Close() {
  if (IsHandleValid()) {
    switch (type_) {
      case INVALID:
        NOTREACHED();
        break;
      case SHARED_MEMORY:
        base::SharedMemory::CloseHandle(shm_handle_);
        break;
      case SOCKET:
      case FILE:
        base::File file_closer = IPC::PlatformFileForTransitToFile(descriptor_);
        break;
      // No default so the compiler will warn us if a new type is added.
    }
  }
  *this = SerializedHandle();
}

// static
void SerializedHandle::WriteHeader(const Header& hdr, base::Pickle* pickle) {
  pickle->WriteInt(hdr.type);
  if (hdr.type == SHARED_MEMORY) {
    pickle->WriteUInt32(hdr.size);
  } else if (hdr.type == FILE) {
    pickle->WriteInt(hdr.open_flags);
    pickle->WriteInt(hdr.file_io);
  }
}

// static
bool SerializedHandle::ReadHeader(base::PickleIterator* iter, Header* hdr) {
  *hdr = Header(INVALID, 0, 0, 0);
  int type = 0;
  if (!iter->ReadInt(&type))
    return false;
  bool valid_type = false;
  switch (type) {
    case SHARED_MEMORY: {
      uint32_t size = 0;
      if (!iter->ReadUInt32(&size))
        return false;
      hdr->size = size;
      valid_type = true;
      break;
    }
    case FILE: {
      int open_flags = 0;
      PP_Resource file_io = 0;
      if (!iter->ReadInt(&open_flags) || !iter->ReadInt(&file_io))
        return false;
      hdr->open_flags = open_flags;
      hdr->file_io = file_io;
      valid_type = true;
      break;
    }
    case SOCKET:
    case INVALID:
      valid_type = true;
      break;
    // No default so the compiler will warn us if a new type is added.
  }
  if (valid_type)
    hdr->type = Type(type);
  return valid_type;
}

}  // namespace proxy
}  // namespace ppapi
