#ifndef DAWNNATIVE_VULKAN_EXTERNALHANDLE_H_
#define DAWNNATIVE_VULKAN_EXTERNALHANDLE_H_

#include "common/vulkan_platform.h"

namespace dawn_native { namespace vulkan {

#if DAWN_PLATFORM_LINUX
    // File descriptor
    using ExternalMemoryHandle = int;
    // File descriptor
    using ExternalSemaphoreHandle = int;
#elif DAWN_PLATFORM_FUCHSIA
    // Really a Zircon vmo handle.
    using ExternalMemoryHandle = zx_handle_t;
    // Really a Zircon event handle.
    using ExternalSemaphoreHandle = zx_handle_t;
#else
    // Generic types so that the Null service can compile, not used for real handles
    using ExternalMemoryHandle = void*;
    using ExternalSemaphoreHandle = void*;
#endif

}}  // namespace dawn_native::vulkan

#endif  // DAWNNATIVE_VULKAN_EXTERNALHANDLE_H_
