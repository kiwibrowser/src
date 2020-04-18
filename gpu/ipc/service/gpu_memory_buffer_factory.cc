// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/ipc/service/gpu_memory_buffer_factory.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "build/build_config.h"

#if defined(OS_MACOSX)
#include "gpu/ipc/service/gpu_memory_buffer_factory_io_surface.h"
#endif

#if defined(OS_LINUX)
#include "gpu/ipc/service/gpu_memory_buffer_factory_native_pixmap.h"
#endif

#if defined(OS_WIN)
#include "gpu/ipc/service/gpu_memory_buffer_factory_dxgi.h"
#endif

#if defined(OS_ANDROID)
#include "gpu/ipc/service/gpu_memory_buffer_factory_android_hardware_buffer.h"
#endif

namespace gpu {

// static
std::unique_ptr<GpuMemoryBufferFactory>
GpuMemoryBufferFactory::CreateNativeType() {
#if defined(OS_MACOSX)
  return base::WrapUnique(new GpuMemoryBufferFactoryIOSurface);
#elif defined(OS_ANDROID)
  return base::WrapUnique(new GpuMemoryBufferFactoryAndroidHardwareBuffer);
#elif defined(OS_LINUX)
  return base::WrapUnique(new GpuMemoryBufferFactoryNativePixmap);
#elif defined(OS_WIN)
  return base::WrapUnique(new GpuMemoryBufferFactoryDXGI);
#else
  return nullptr;
#endif
}

}  // namespace gpu
