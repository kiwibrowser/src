// Copyright 2020 The Dawn Authors
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

#ifndef DAWNNATIVE_SURFACE_H_
#define DAWNNATIVE_SURFACE_H_

#include "common/RefCounted.h"
#include "dawn_native/Error.h"
#include "dawn_native/Forward.h"

#include "dawn_native/dawn_platform.h"

namespace dawn_native {

    MaybeError ValidateSurfaceDescriptor(const InstanceBase* instance,
                                         const SurfaceDescriptor* descriptor);

    // A surface is a sum types of all the kind of windows Dawn supports. The OS-specific types
    // aren't used because they would cause compilation errors on other OSes (or require
    // ObjectiveC).
    // The surface is also used to store the current swapchain so that we can detach it when it is
    // replaced.
    class Surface final : public RefCounted {
      public:
        Surface(InstanceBase* instance, const SurfaceDescriptor* descriptor);

        void SetAttachedSwapChain(NewSwapChainBase* swapChain);
        NewSwapChainBase* GetAttachedSwapChain() const;

        // These are valid to call on all Surfaces.
        enum class Type { MetalLayer, WindowsHWND, Xlib };
        Type GetType() const;
        InstanceBase* GetInstance();

        // Valid to call if the type is MetalLayer
        void* GetMetalLayer() const;

        // Valid to call if the type is WindowsHWND
        void* GetHInstance() const;
        void* GetHWND() const;

        // Valid to call if the type is WindowsXlib
        void* GetXDisplay() const;
        uint32_t GetXWindow() const;

      private:
        ~Surface() override;

        Ref<InstanceBase> mInstance;
        Type mType;

        // The swapchain will set this to null when it is destroyed.
        NewSwapChainBase* mSwapChain = nullptr;

        // MetalLayer
        void* mMetalLayer = nullptr;

        // WindowsHwnd
        void* mHInstance = nullptr;
        void* mHWND = nullptr;

        // Xlib
        void* mXDisplay = nullptr;
        uint32_t mXWindow = 0;
    };

}  // namespace dawn_native

#endif  // DAWNNATIVE_SURFACE_H_
