// Copyright 2017 The Dawn Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef COMMON_VULKANPLATFORM_H_
#define COMMON_VULKANPLATFORM_H_

#if !defined(DAWN_ENABLE_BACKEND_VULKAN)
#    error "vulkan_platform.h included without the Vulkan backend enabled"
#endif
#if defined(VULKAN_CORE_H_)
#    error "vulkan.h included before vulkan_platform.h"
#endif

#include "common/Platform.h"

#include <cstddef>
#include <cstdint>

// vulkan.h defines non-dispatchable handles to opaque pointers on 64bit architectures and uint64_t
// on 32bit architectures. This causes a problem in 32bit where the handles cannot be used to
// distinguish between overloads of the same function.
// Change the definition of non-dispatchable handles to be opaque structures containing a uint64_t
// and overload the comparison operators between themselves and VK_NULL_HANDLE (which will be
// redefined to be nullptr). This keeps the type-safety of having the handles be different types
// (like vulkan.h on 64 bit) but makes sure the types are different on 32 bit architectures.

#if defined(DAWN_PLATFORM_64_BIT)
#    define DAWN_DEFINE_NATIVE_NON_DISPATCHABLE_HANDLE(object) using object = struct object##_T*;
// This function is needed because MSVC doesn't accept reinterpret_cast from uint64_t from uint64_t
// TODO(cwallez@chromium.org): Remove this once we rework vulkan_platform.h
template <typename T>
T NativeNonDispatachableHandleFromU64(uint64_t u64) {
    return reinterpret_cast<T>(u64);
}
#elif defined(DAWN_PLATFORM_32_BIT)
#    define DAWN_DEFINE_NATIVE_NON_DISPATCHABLE_HANDLE(object) using object = uint64_t;
template <typename T>
T NativeNonDispatachableHandleFromU64(uint64_t u64) {
    return u64;
}
#else
#    error "Unsupported platform"
#endif

// Define a dummy Vulkan handle for use before we include vulkan.h
DAWN_DEFINE_NATIVE_NON_DISPATCHABLE_HANDLE(VkSomeHandle)

// Find out the alignment of native handles. Logically we would use alignof(VkSomeHandleNative) so
// why bother with the wrapper struct? It turns out that on Linux Intel x86 alignof(uint64_t) is 8
// but alignof(struct{uint64_t a;}) is 4. This is because this Intel ABI doesn't say anything about
// double-word alignment so for historical reasons compilers violated the standard and use an
// alignment of 4 for uint64_t (and double) inside structures.
// See https://stackoverflow.com/questions/44877185
// One way to get the alignment inside structures of a type is to look at the alignment of it
// wrapped in a structure. Hence VkSameHandleNativeWrappe

namespace dawn_native { namespace vulkan {

    namespace detail {
        template <typename T>
        struct WrapperStruct {
            T member;
        };

        template <typename T>
        static constexpr size_t AlignOfInStruct = alignof(WrapperStruct<T>);

        static constexpr size_t kNativeVkHandleAlignment = AlignOfInStruct<VkSomeHandle>;
        static constexpr size_t kUint64Alignment = AlignOfInStruct<uint64_t>;

        // Simple handle types that supports "nullptr_t" as a 0 value.
        template <typename Tag, typename HandleType>
        class alignas(detail::kNativeVkHandleAlignment) VkHandle {
          public:
            // Default constructor and assigning of VK_NULL_HANDLE
            VkHandle() = default;
            VkHandle(std::nullptr_t) {
            }

            // Use default copy constructor/assignment
            VkHandle(const VkHandle<Tag, HandleType>& other) = default;
            VkHandle& operator=(const VkHandle<Tag, HandleType>&) = default;

            // Comparisons between handles
            bool operator==(VkHandle<Tag, HandleType> other) const {
                return mHandle == other.mHandle;
            }
            bool operator!=(VkHandle<Tag, HandleType> other) const {
                return mHandle != other.mHandle;
            }

            // Comparisons between handles and VK_NULL_HANDLE
            bool operator==(std::nullptr_t) const {
                return mHandle == 0;
            }
            bool operator!=(std::nullptr_t) const {
                return mHandle != 0;
            }

            // Implicit conversion to real Vulkan types.
            operator HandleType() const {
                return GetHandle();
            }

            HandleType GetHandle() const {
                return mHandle;
            }

            HandleType& operator*() {
                return mHandle;
            }

            static VkHandle<Tag, HandleType> CreateFromHandle(HandleType handle) {
                return VkHandle{handle};
            }

          private:
            explicit VkHandle(HandleType handle) : mHandle(handle) {
            }

            HandleType mHandle = 0;
        };
    }  // namespace detail

    static constexpr std::nullptr_t VK_NULL_HANDLE = nullptr;

    template <typename Tag, typename HandleType>
    HandleType* AsVkArray(detail::VkHandle<Tag, HandleType>* handle) {
        return reinterpret_cast<HandleType*>(handle);
    }

}}  // namespace dawn_native::vulkan

#define VK_DEFINE_NON_DISPATCHABLE_HANDLE(object)                                   \
    DAWN_DEFINE_NATIVE_NON_DISPATCHABLE_HANDLE(object)                              \
    namespace dawn_native { namespace vulkan {                                      \
            using object = detail::VkHandle<struct VkTag##object, ::object>;        \
            static_assert(sizeof(object) == sizeof(uint64_t), "");                  \
            static_assert(alignof(object) == detail::kUint64Alignment, "");         \
            static_assert(sizeof(object) == sizeof(::object), "");                  \
            static_assert(alignof(object) == detail::kNativeVkHandleAlignment, ""); \
        }                                                                           \
    }  // namespace dawn_native::vulkan

// Import additional parts of Vulkan that are supported on our architecture and preemptively include
// headers that vulkan.h includes that we have "undefs" for.
#if defined(DAWN_PLATFORM_WINDOWS)
#    define VK_USE_PLATFORM_WIN32_KHR
#    include "common/windows_with_undefs.h"
#endif  // DAWN_PLATFORM_WINDOWS

#if defined(DAWN_USE_X11)
#    define VK_USE_PLATFORM_XLIB_KHR
#    include "common/xlib_with_undefs.h"
#endif  // defined(DAWN_USE_X11)

#if defined(DAWN_ENABLE_BACKEND_METAL)
#    define VK_USE_PLATFORM_METAL_EXT
#endif  // defined(DAWN_ENABLE_BACKEND_METAL)

#if defined(DAWN_PLATFORM_ANDROID)
#    define VK_USE_PLATFORM_ANDROID_KHR
#endif  // defined(DAWN_PLATFORM_ANDROID)

#if defined(DAWN_PLATFORM_FUCHSIA)
#    define VK_USE_PLATFORM_FUCHSIA
#endif  // defined(DAWN_PLATFORM_FUCHSIA)

// The actual inclusion of vulkan.h!
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

// Redefine VK_NULL_HANDLE for better type safety where possible.
#undef VK_NULL_HANDLE
#if defined(DAWN_PLATFORM_64_BIT)
static constexpr std::nullptr_t VK_NULL_HANDLE = nullptr;
#elif defined(DAWN_PLATFORM_32_BIT)
static constexpr uint64_t VK_NULL_HANDLE = 0;
#else
#    error "Unsupported platform"
#endif

// Include Fuchsia-specific definitions that are not upstreamed yet.
#if defined(DAWN_PLATFORM_FUCHSIA)
#    include <vulkan/vulkan_fuchsia_extras.h>
#endif  // defined(DAWN_PLATFORM_FUCHSIA)

#endif  // COMMON_VULKANPLATFORM_H_
