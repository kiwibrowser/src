// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/system/platform_handle.h"

#include "base/memory/platform_shared_memory_region.h"
#include "base/numerics/safe_conversions.h"
#include "build/build_config.h"

#if defined(OS_MACOSX) && !defined(OS_IOS)
#include <mach/mach.h>
#include "base/mac/mach_logging.h"
#endif

namespace mojo {

namespace {

uint64_t PlatformHandleValueFromPlatformFile(base::PlatformFile file) {
#if defined(OS_WIN)
  return reinterpret_cast<uint64_t>(file);
#else
  return static_cast<uint64_t>(file);
#endif
}

base::PlatformFile PlatformFileFromPlatformHandleValue(uint64_t value) {
#if defined(OS_WIN)
  return reinterpret_cast<base::PlatformFile>(value);
#else
  return static_cast<base::PlatformFile>(value);
#endif
}

ScopedSharedBufferHandle WrapPlatformSharedMemoryRegion(
    base::subtle::PlatformSharedMemoryRegion region) {
  if (!region.IsValid())
    return ScopedSharedBufferHandle();

  MojoPlatformSharedMemoryRegionAccessMode access_mode;
  switch (region.GetMode()) {
    case base::subtle::PlatformSharedMemoryRegion::Mode::kReadOnly:
      access_mode = MOJO_PLATFORM_SHARED_MEMORY_REGION_ACCESS_MODE_READ_ONLY;
      break;
    case base::subtle::PlatformSharedMemoryRegion::Mode::kWritable:
      access_mode = MOJO_PLATFORM_SHARED_MEMORY_REGION_ACCESS_MODE_WRITABLE;
      break;
    case base::subtle::PlatformSharedMemoryRegion::Mode::kUnsafe:
      access_mode = MOJO_PLATFORM_SHARED_MEMORY_REGION_ACCESS_MODE_UNSAFE;
      break;
    default:
      NOTREACHED();
      return ScopedSharedBufferHandle();
  }

  base::subtle::PlatformSharedMemoryRegion::ScopedPlatformHandle handle =
      region.PassPlatformHandle();
  MojoPlatformHandle platform_handles[2];
  uint32_t num_platform_handles = 1;
  platform_handles[0].struct_size = sizeof(platform_handles[0]);
#if defined(OS_WIN)
  platform_handles[0].type = MOJO_PLATFORM_HANDLE_TYPE_WINDOWS_HANDLE;
  platform_handles[0].value = reinterpret_cast<uint64_t>(handle.Take());
#elif defined(OS_FUCHSIA)
  platform_handles[0].type = MOJO_PLATFORM_HANDLE_TYPE_FUCHSIA_HANDLE;
  platform_handles[0].value = static_cast<uint64_t>(handle.release());
#elif defined(OS_MACOSX) && !defined(OS_IOS)
  platform_handles[0].type = MOJO_PLATFORM_HANDLE_TYPE_MACH_PORT;
  platform_handles[0].value = static_cast<uint64_t>(handle.release());
#elif defined(OS_ANDROID)
  platform_handles[0].type = MOJO_PLATFORM_HANDLE_TYPE_FILE_DESCRIPTOR;
  platform_handles[0].value = static_cast<uint64_t>(handle.release());
#else
  platform_handles[0].type = MOJO_PLATFORM_HANDLE_TYPE_FILE_DESCRIPTOR;
  platform_handles[0].value = static_cast<uint64_t>(handle.fd.release());

  if (region.GetMode() ==
      base::subtle::PlatformSharedMemoryRegion::Mode::kWritable) {
    num_platform_handles = 2;
    platform_handles[1].struct_size = sizeof(platform_handles[1]);
    platform_handles[1].type = MOJO_PLATFORM_HANDLE_TYPE_FILE_DESCRIPTOR;
    platform_handles[1].value =
        static_cast<uint64_t>(handle.readonly_fd.release());
  }
#endif
  const auto& guid = region.GetGUID();
  MojoSharedBufferGuid mojo_guid = {guid.GetHighForSerialization(),
                                    guid.GetLowForSerialization()};
  MojoHandle mojo_handle;
  MojoResult result = MojoWrapPlatformSharedMemoryRegion(
      platform_handles, num_platform_handles, region.GetSize(), &mojo_guid,
      access_mode, nullptr, &mojo_handle);
  if (result != MOJO_RESULT_OK)
    return ScopedSharedBufferHandle();
  return ScopedSharedBufferHandle(SharedBufferHandle(mojo_handle));
}

base::subtle::PlatformSharedMemoryRegion UnwrapPlatformSharedMemoryRegion(
    ScopedSharedBufferHandle mojo_handle) {
  if (!mojo_handle.is_valid())
    return base::subtle::PlatformSharedMemoryRegion();

  MojoPlatformHandle platform_handles[2];
  platform_handles[0].struct_size = sizeof(platform_handles[0]);
  platform_handles[1].struct_size = sizeof(platform_handles[1]);
  uint32_t num_platform_handles = 2;
  uint64_t size;
  MojoSharedBufferGuid mojo_guid;
  MojoPlatformSharedMemoryRegionAccessMode access_mode;
  MojoResult result = MojoUnwrapPlatformSharedMemoryRegion(
      mojo_handle.release().value(), nullptr, platform_handles,
      &num_platform_handles, &size, &mojo_guid, &access_mode);
  if (result != MOJO_RESULT_OK)
    return base::subtle::PlatformSharedMemoryRegion();

  base::subtle::PlatformSharedMemoryRegion::ScopedPlatformHandle region_handle;
#if defined(OS_WIN)
  if (num_platform_handles != 1)
    return base::subtle::PlatformSharedMemoryRegion();
  if (platform_handles[0].type != MOJO_PLATFORM_HANDLE_TYPE_WINDOWS_HANDLE)
    return base::subtle::PlatformSharedMemoryRegion();
  region_handle.Set(reinterpret_cast<HANDLE>(platform_handles[0].value));
#elif defined(OS_FUCHSIA)
  if (num_platform_handles != 1)
    return base::subtle::PlatformSharedMemoryRegion();
  if (platform_handles[0].type != MOJO_PLATFORM_HANDLE_TYPE_FUCHSIA_HANDLE)
    return base::subtle::PlatformSharedMemoryRegion();
  region_handle.reset(static_cast<zx_handle_t>(platform_handles[0].value));
#elif defined(OS_MACOSX) && !defined(OS_IOS)
  if (num_platform_handles != 1)
    return base::subtle::PlatformSharedMemoryRegion();
  if (platform_handles[0].type != MOJO_PLATFORM_HANDLE_TYPE_MACH_PORT)
    return base::subtle::PlatformSharedMemoryRegion();
  region_handle.reset(static_cast<mach_port_t>(platform_handles[0].value));
#elif defined(OS_ANDROID)
  if (num_platform_handles != 1)
    return base::subtle::PlatformSharedMemoryRegion();
  if (platform_handles[0].type != MOJO_PLATFORM_HANDLE_TYPE_FILE_DESCRIPTOR)
    return base::subtle::PlatformSharedMemoryRegion();
  region_handle.reset(static_cast<int>(platform_handles[0].value));
#else
  if (access_mode == MOJO_PLATFORM_SHARED_MEMORY_REGION_ACCESS_MODE_WRITABLE) {
    if (num_platform_handles != 2)
      return base::subtle::PlatformSharedMemoryRegion();
  } else if (num_platform_handles != 1) {
    return base::subtle::PlatformSharedMemoryRegion();
  }
  if (platform_handles[0].type != MOJO_PLATFORM_HANDLE_TYPE_FILE_DESCRIPTOR)
    return base::subtle::PlatformSharedMemoryRegion();
  region_handle.fd.reset(static_cast<int>(platform_handles[0].value));
  if (num_platform_handles == 2) {
    if (platform_handles[1].type != MOJO_PLATFORM_HANDLE_TYPE_FILE_DESCRIPTOR)
      return base::subtle::PlatformSharedMemoryRegion();
    region_handle.readonly_fd.reset(
        static_cast<int>(platform_handles[1].value));
  }
#endif

  base::subtle::PlatformSharedMemoryRegion::Mode mode;
  switch (access_mode) {
    case MOJO_PLATFORM_SHARED_MEMORY_REGION_ACCESS_MODE_READ_ONLY:
      mode = base::subtle::PlatformSharedMemoryRegion::Mode::kReadOnly;
      break;
    case MOJO_PLATFORM_SHARED_MEMORY_REGION_ACCESS_MODE_WRITABLE:
      mode = base::subtle::PlatformSharedMemoryRegion::Mode::kWritable;
      break;
    case MOJO_PLATFORM_SHARED_MEMORY_REGION_ACCESS_MODE_UNSAFE:
      mode = base::subtle::PlatformSharedMemoryRegion::Mode::kUnsafe;
      break;
    default:
      return base::subtle::PlatformSharedMemoryRegion();
  }

  return base::subtle::PlatformSharedMemoryRegion::Take(
      std::move(region_handle), mode, size,
      base::UnguessableToken::Deserialize(mojo_guid.high, mojo_guid.low));
}

}  // namespace

void PlatformHandleToMojoPlatformHandle(PlatformHandle handle,
                                        MojoPlatformHandle* out_handle) {
  DCHECK(out_handle);
  out_handle->struct_size = sizeof(MojoPlatformHandle);
  if (!handle.is_valid()) {
    out_handle->type = MOJO_PLATFORM_HANDLE_TYPE_INVALID;
    out_handle->value = 0;
    return;
  }

  do {
#if defined(OS_WIN)
    out_handle->type = MOJO_PLATFORM_HANDLE_TYPE_WINDOWS_HANDLE;
    out_handle->value =
        static_cast<uint64_t>(HandleToLong(handle.TakeHandle().Take()));
    break;
#elif defined(OS_FUCHSIA)
    if (handle.is_valid_handle()) {
      out_handle->type = MOJO_PLATFORM_HANDLE_TYPE_FUCHSIA_HANDLE;
      out_handle->value = handle.TakeHandle().release();
      break;
    }
#elif defined(OS_MACOSX) && !defined(OS_IOS)
    if (handle.is_valid_mach_port()) {
      out_handle->type = MOJO_PLATFORM_HANDLE_TYPE_MACH_PORT;
      out_handle->value =
          static_cast<uint64_t>(handle.TakeMachPort().release());
      break;
    }
#endif

#if defined(OS_POSIX) || defined(OS_FUCHSIA)
    DCHECK(handle.is_valid_fd());
    out_handle->type = MOJO_PLATFORM_HANDLE_TYPE_FILE_DESCRIPTOR;
    out_handle->value = static_cast<uint64_t>(handle.TakeFD().release());
#endif
  } while (false);

  // One of the above cases must take ownership of |handle|.
  DCHECK(!handle.is_valid());
}

PlatformHandle MojoPlatformHandleToPlatformHandle(
    const MojoPlatformHandle* handle) {
  if (handle->struct_size < sizeof(*handle) ||
      handle->type == MOJO_PLATFORM_HANDLE_TYPE_INVALID) {
    return PlatformHandle();
  }

#if defined(OS_WIN)
  if (handle->type != MOJO_PLATFORM_HANDLE_TYPE_WINDOWS_HANDLE)
    return PlatformHandle();
  return PlatformHandle(
      base::win::ScopedHandle(LongToHandle(static_cast<long>(handle->value))));
#elif defined(OS_FUCHSIA)
  if (handle->type == MOJO_PLATFORM_HANDLE_TYPE_FUCHSIA_HANDLE)
    return PlatformHandle(base::ScopedZxHandle(handle->value));
#elif defined(OS_MACOSX) && !defined(OS_IOS)
  if (handle->type == MOJO_PLATFORM_HANDLE_TYPE_MACH_PORT) {
    return PlatformHandle(base::mac::ScopedMachSendRight(
        static_cast<mach_port_t>(handle->value)));
  }
#endif

#if defined(OS_POSIX) || defined(OS_FUCHSIA)
  if (handle->type != MOJO_PLATFORM_HANDLE_TYPE_FILE_DESCRIPTOR)
    return PlatformHandle();
  return PlatformHandle(base::ScopedFD(static_cast<int>(handle->value)));
#endif
}

// Wraps a PlatformFile as a Mojo handle. Takes ownership of the file object.
ScopedHandle WrapPlatformFile(base::PlatformFile platform_file) {
  MojoPlatformHandle platform_handle;
  platform_handle.struct_size = sizeof(MojoPlatformHandle);
  platform_handle.type = kPlatformFileHandleType;
  platform_handle.value = PlatformHandleValueFromPlatformFile(platform_file);

  MojoHandle mojo_handle;
  MojoResult result =
      MojoWrapPlatformHandle(&platform_handle, nullptr, &mojo_handle);
  CHECK_EQ(result, MOJO_RESULT_OK);

  return ScopedHandle(Handle(mojo_handle));
}

MojoResult UnwrapPlatformFile(ScopedHandle handle, base::PlatformFile* file) {
  MojoPlatformHandle platform_handle;
  platform_handle.struct_size = sizeof(MojoPlatformHandle);
  MojoResult result = MojoUnwrapPlatformHandle(handle.release().value(),
                                               nullptr, &platform_handle);
  if (result != MOJO_RESULT_OK)
    return result;

  if (platform_handle.type == MOJO_PLATFORM_HANDLE_TYPE_INVALID) {
    *file = base::kInvalidPlatformFile;
  } else {
    CHECK_EQ(platform_handle.type, kPlatformFileHandleType);
    *file = PlatformFileFromPlatformHandleValue(platform_handle.value);
  }

  return MOJO_RESULT_OK;
}

ScopedSharedBufferHandle WrapSharedMemoryHandle(
    const base::SharedMemoryHandle& memory_handle,
    size_t size,
    UnwrappedSharedMemoryHandleProtection protection) {
  if (!memory_handle.IsValid())
    return ScopedSharedBufferHandle();
  MojoPlatformHandle platform_handle;
  platform_handle.struct_size = sizeof(MojoPlatformHandle);
  platform_handle.type = kPlatformSharedBufferHandleType;
#if defined(OS_MACOSX) && !defined(OS_IOS)
  platform_handle.value =
      static_cast<uint64_t>(memory_handle.GetMemoryObject());
#else
  platform_handle.value =
      PlatformHandleValueFromPlatformFile(memory_handle.GetHandle());
#endif

  MojoPlatformSharedMemoryRegionAccessMode access_mode =
      MOJO_PLATFORM_SHARED_MEMORY_REGION_ACCESS_MODE_UNSAFE;
  if (protection == UnwrappedSharedMemoryHandleProtection::kReadOnly) {
    access_mode = MOJO_PLATFORM_SHARED_MEMORY_REGION_ACCESS_MODE_READ_ONLY;

#if defined(OS_ANDROID)
    // Many callers assume that base::SharedMemory::GetReadOnlyHandle() gives
    // them a handle which is actually read-only. This assumption is invalid on
    // Android. As a precursor to migrating all base::SharedMemory usage --
    // including Mojo internals -- to the new base shared memory API, we ensure
    // that regions are set to read-only if any of their handles are wrapped
    // read-only. This relies on existing usages not attempting to map the
    // region writable any time after this call.
    if (!memory_handle.IsRegionReadOnly())
      memory_handle.SetRegionReadOnly();
#endif
  }

  MojoSharedBufferGuid guid;
  guid.high = memory_handle.GetGUID().GetHighForSerialization();
  guid.low = memory_handle.GetGUID().GetLowForSerialization();
  MojoHandle mojo_handle;
  MojoResult result = MojoWrapPlatformSharedMemoryRegion(
      &platform_handle, 1, size, &guid, access_mode, nullptr, &mojo_handle);
  CHECK_EQ(result, MOJO_RESULT_OK);

  return ScopedSharedBufferHandle(SharedBufferHandle(mojo_handle));
}

MojoResult UnwrapSharedMemoryHandle(
    ScopedSharedBufferHandle handle,
    base::SharedMemoryHandle* memory_handle,
    size_t* size,
    UnwrappedSharedMemoryHandleProtection* protection) {
  if (!handle.is_valid())
    return MOJO_RESULT_INVALID_ARGUMENT;
  MojoPlatformHandle platform_handles[2];
  platform_handles[0].struct_size = sizeof(platform_handles[0]);
  platform_handles[1].struct_size = sizeof(platform_handles[1]);

  uint32_t num_platform_handles = 2;
  uint64_t num_bytes;
  MojoSharedBufferGuid mojo_guid;
  MojoPlatformSharedMemoryRegionAccessMode access_mode;
  MojoResult result = MojoUnwrapPlatformSharedMemoryRegion(
      handle.release().value(), nullptr, platform_handles,
      &num_platform_handles, &num_bytes, &mojo_guid, &access_mode);
  if (result != MOJO_RESULT_OK)
    return result;

  if (size) {
    DCHECK(base::IsValueInRangeForNumericType<size_t>(num_bytes));
    *size = static_cast<size_t>(num_bytes);
  }

  if (protection) {
    *protection =
        access_mode == MOJO_PLATFORM_SHARED_MEMORY_REGION_ACCESS_MODE_READ_ONLY
            ? UnwrappedSharedMemoryHandleProtection::kReadOnly
            : UnwrappedSharedMemoryHandleProtection::kReadWrite;
  }

  base::UnguessableToken guid =
      base::UnguessableToken::Deserialize(mojo_guid.high, mojo_guid.low);
#if defined(OS_MACOSX) && !defined(OS_IOS)
  DCHECK_EQ(platform_handles[0].type, MOJO_PLATFORM_HANDLE_TYPE_MACH_PORT);
  DCHECK_EQ(num_platform_handles, 1u);
  *memory_handle = base::SharedMemoryHandle(
      static_cast<mach_port_t>(platform_handles[0].value), num_bytes, guid);
#elif defined(OS_FUCHSIA)
  DCHECK_EQ(platform_handles[0].type, MOJO_PLATFORM_HANDLE_TYPE_FUCHSIA_HANDLE);
  DCHECK_EQ(num_platform_handles, 1u);
  *memory_handle = base::SharedMemoryHandle(
      static_cast<zx_handle_t>(platform_handles[0].value), num_bytes, guid);
#elif defined(OS_POSIX)
  DCHECK_EQ(platform_handles[0].type,
            MOJO_PLATFORM_HANDLE_TYPE_FILE_DESCRIPTOR);
  *memory_handle = base::SharedMemoryHandle(
      base::FileDescriptor(static_cast<int>(platform_handles[0].value), false),
      num_bytes, guid);
#if !defined(OS_ANDROID)
  if (access_mode == MOJO_PLATFORM_SHARED_MEMORY_REGION_ACCESS_MODE_WRITABLE) {
    DCHECK_EQ(num_platform_handles, 2u);
    // When unwrapping as a base::SharedMemoryHandle, make sure to discard the
    // extra file descriptor if the region is writable. base::SharedMemoryHandle
    // effectively only supports read-only and unsafe usage modes when wrapping
    // or unwrapping to and from Mojo handles.
    base::ScopedFD discarded_readonly_fd(
        static_cast<int>(platform_handles[1].value));
  } else {
    DCHECK_EQ(num_platform_handles, 1u);
  }
#else   // !defined(OS_ANDROID)
  DCHECK_EQ(num_platform_handles, 1u);
#endif  // !defined(OS_ANDROID)
#elif defined(OS_WIN)
  DCHECK_EQ(platform_handles[0].type, MOJO_PLATFORM_HANDLE_TYPE_WINDOWS_HANDLE);
  DCHECK_EQ(num_platform_handles, 1u);
  *memory_handle = base::SharedMemoryHandle(
      reinterpret_cast<HANDLE>(platform_handles[0].value), num_bytes, guid);
#endif

  return MOJO_RESULT_OK;
}

ScopedSharedBufferHandle WrapReadOnlySharedMemoryRegion(
    base::ReadOnlySharedMemoryRegion region) {
  return WrapPlatformSharedMemoryRegion(
      base::ReadOnlySharedMemoryRegion::TakeHandleForSerialization(
          std::move(region)));
}

ScopedSharedBufferHandle WrapUnsafeSharedMemoryRegion(
    base::UnsafeSharedMemoryRegion region) {
  return WrapPlatformSharedMemoryRegion(
      base::UnsafeSharedMemoryRegion::TakeHandleForSerialization(
          std::move(region)));
}

ScopedSharedBufferHandle WrapWritableSharedMemoryRegion(
    base::WritableSharedMemoryRegion region) {
  return WrapPlatformSharedMemoryRegion(
      base::WritableSharedMemoryRegion::TakeHandleForSerialization(
          std::move(region)));
}

base::ReadOnlySharedMemoryRegion UnwrapReadOnlySharedMemoryRegion(
    ScopedSharedBufferHandle handle) {
  return base::ReadOnlySharedMemoryRegion::Deserialize(
      UnwrapPlatformSharedMemoryRegion(std::move(handle)));
}

base::UnsafeSharedMemoryRegion UnwrapUnsafeSharedMemoryRegion(
    ScopedSharedBufferHandle handle) {
  return base::UnsafeSharedMemoryRegion::Deserialize(
      UnwrapPlatformSharedMemoryRegion(std::move(handle)));
}

base::WritableSharedMemoryRegion UnwrapWritableSharedMemoryRegion(
    ScopedSharedBufferHandle handle) {
  return base::WritableSharedMemoryRegion::Deserialize(
      UnwrapPlatformSharedMemoryRegion(std::move(handle)));
}

#if defined(OS_MACOSX) && !defined(OS_IOS)
ScopedHandle WrapMachPort(mach_port_t port) {
  kern_return_t kr =
      mach_port_mod_refs(mach_task_self(), port, MACH_PORT_RIGHT_SEND, 1);
  MACH_LOG_IF(ERROR, kr != KERN_SUCCESS, kr)
      << "MachPortAttachmentMac mach_port_mod_refs";
  if (kr != KERN_SUCCESS)
    return ScopedHandle();

  MojoPlatformHandle platform_handle;
  platform_handle.struct_size = sizeof(MojoPlatformHandle);
  platform_handle.type = MOJO_PLATFORM_HANDLE_TYPE_MACH_PORT;
  platform_handle.value = static_cast<uint64_t>(port);

  MojoHandle mojo_handle;
  MojoResult result =
      MojoWrapPlatformHandle(&platform_handle, nullptr, &mojo_handle);
  CHECK_EQ(result, MOJO_RESULT_OK);

  return ScopedHandle(Handle(mojo_handle));
}

MojoResult UnwrapMachPort(ScopedHandle handle, mach_port_t* port) {
  MojoPlatformHandle platform_handle;
  platform_handle.struct_size = sizeof(MojoPlatformHandle);
  MojoResult result = MojoUnwrapPlatformHandle(handle.release().value(),
                                               nullptr, &platform_handle);
  if (result != MOJO_RESULT_OK)
    return result;

  CHECK(platform_handle.type == MOJO_PLATFORM_HANDLE_TYPE_MACH_PORT ||
        platform_handle.type == MOJO_PLATFORM_HANDLE_TYPE_INVALID);
  *port = static_cast<mach_port_t>(platform_handle.value);
  return MOJO_RESULT_OK;
}
#endif  // defined(OS_MACOSX) && !defined(OS_IOS)

}  // namespace mojo
