// Copyright 2018 The Dawn Authors
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

#ifndef DAWNNATIVE_METALBACKEND_H_
#define DAWNNATIVE_METALBACKEND_H_

#include <dawn/dawn_wsi.h>
#include <dawn_native/DawnNative.h>

// The specifics of the Metal backend expose types in function signatures that might not be
// available in dependent's minimum supported SDK version. Suppress all availability errors using
// clang's pragmas. Dependents using the types without guarded availability will still get errors
// when using the types.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunguarded-availability"

struct __IOSurface;
typedef __IOSurface* IOSurfaceRef;

#ifdef __OBJC__
#    import <Metal/Metal.h>
#endif  //__OBJC__

namespace dawn_native { namespace metal {
    struct DAWN_NATIVE_EXPORT ExternalImageDescriptorIOSurface : ExternalImageDescriptor {
      public:
        ExternalImageDescriptorIOSurface();

        IOSurfaceRef ioSurface;
        uint32_t plane;
    };

    DAWN_NATIVE_EXPORT WGPUTexture
    WrapIOSurface(WGPUDevice device, const ExternalImageDescriptorIOSurface* descriptor);

    // When making Metal interop with other APIs, we need to be careful that QueueSubmit doesn't
    // mean that the operations will be visible to other APIs/Metal devices right away. macOS
    // does have a global queue of graphics operations, but the command buffers are inserted there
    // when they are "scheduled". Submitting other operations before the command buffer is
    // scheduled could lead to races in who gets scheduled first and incorrect rendering.
    DAWN_NATIVE_EXPORT void WaitForCommandsToBeScheduled(WGPUDevice device);
}}  // namespace dawn_native::metal

#ifdef __OBJC__
namespace dawn_native { namespace metal {
    DAWN_NATIVE_EXPORT id<MTLDevice> GetMetalDevice(WGPUDevice device);
}}      // namespace dawn_native::metal
#endif  // __OBJC__

#pragma clang diagnostic pop

#endif  // DAWNNATIVE_METALBACKEND_H_
