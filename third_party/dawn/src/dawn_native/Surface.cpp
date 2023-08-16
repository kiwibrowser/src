// Copyright 2020 the Dawn Authors
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

#include "dawn_native/Surface.h"

#include "common/Platform.h"
#include "dawn_native/Instance.h"
#include "dawn_native/SwapChain.h"

#if defined(DAWN_PLATFORM_WINDOWS)
#    include "common/windows_with_undefs.h"
#endif  // DAWN_PLATFORM_WINDOWS

#if defined(DAWN_USE_X11)
#    include "common/xlib_with_undefs.h"
#endif  // defined(DAWN_USE_X11)

namespace dawn_native {

#if defined(DAWN_ENABLE_BACKEND_METAL)
    bool InheritsFromCAMetalLayer(void* obj);
#endif  // defined(DAWN_ENABLE_BACKEND_METAL)

    MaybeError ValidateSurfaceDescriptor(const InstanceBase* instance,
                                         const SurfaceDescriptor* descriptor) {
        // TODO(cwallez@chromium.org): Have some type of helper to iterate over all the chained
        // structures.
        if (descriptor->nextInChain == nullptr) {
            return DAWN_VALIDATION_ERROR("Surface cannot be created with just the base descriptor");
        }

        const ChainedStruct* chainedDescriptor = descriptor->nextInChain;
        if (chainedDescriptor->nextInChain != nullptr) {
            return DAWN_VALIDATION_ERROR("Cannot specify two windows for a single surface");
        }

        switch (chainedDescriptor->sType) {
#if defined(DAWN_ENABLE_BACKEND_METAL)
            case wgpu::SType::SurfaceDescriptorFromMetalLayer: {
                const SurfaceDescriptorFromMetalLayer* metalDesc =
                    static_cast<const SurfaceDescriptorFromMetalLayer*>(chainedDescriptor);

                // Check that the layer is a CAMetalLayer (or a derived class).
                if (!InheritsFromCAMetalLayer(metalDesc->layer)) {
                    return DAWN_VALIDATION_ERROR("layer must be a CAMetalLayer");
                }
                break;
            }
#endif  // defined(DAWN_ENABLE_BACKEND_METAL)

#if defined(DAWN_PLATFORM_WINDOWS)
            case wgpu::SType::SurfaceDescriptorFromWindowsHWND: {
                const SurfaceDescriptorFromWindowsHWND* hwndDesc =
                    static_cast<const SurfaceDescriptorFromWindowsHWND*>(chainedDescriptor);

                // It is not possible to validate an HINSTANCE.

                // Validate the hwnd using the windows.h IsWindow function.
                if (IsWindow(static_cast<HWND>(hwndDesc->hwnd)) == 0) {
                    return DAWN_VALIDATION_ERROR("Invalid HWND");
                }
                break;
            }
#endif  // defined(DAWN_PLATFORM_WINDOWS)

#if defined(DAWN_USE_X11)
            case wgpu::SType::SurfaceDescriptorFromXlib: {
                const SurfaceDescriptorFromXlib* xDesc =
                    static_cast<const SurfaceDescriptorFromXlib*>(chainedDescriptor);

                // It is not possible to validate an X Display.

                // Check the validity of the window by calling a getter function on the window that
                // returns a status code. If the window is bad the call return a status of zero. We
                // need to set a temporary X11 error handler while doing this because the default
                // X11 error handler exits the program on any error.
                XErrorHandler oldErrorHandler =
                    XSetErrorHandler([](Display*, XErrorEvent*) { return 0; });
                XWindowAttributes attributes;
                int status = XGetWindowAttributes(reinterpret_cast<Display*>(xDesc->display),
                                                  xDesc->window, &attributes);
                XSetErrorHandler(oldErrorHandler);

                if (status == 0) {
                    return DAWN_VALIDATION_ERROR("Invalid X Window");
                }
                break;
            }
#endif  // defined(DAWN_USE_X11)

            case wgpu::SType::SurfaceDescriptorFromCanvasHTMLSelector:
            default:
                return DAWN_VALIDATION_ERROR("Unsupported sType");
        }

        return {};
    }

    Surface::Surface(InstanceBase* instance, const SurfaceDescriptor* descriptor)
        : mInstance(instance) {
        ASSERT(descriptor->nextInChain != nullptr);
        const ChainedStruct* chainedDescriptor = descriptor->nextInChain;

        switch (chainedDescriptor->sType) {
            case wgpu::SType::SurfaceDescriptorFromMetalLayer: {
                const SurfaceDescriptorFromMetalLayer* metalDesc =
                    static_cast<const SurfaceDescriptorFromMetalLayer*>(chainedDescriptor);
                mType = Type::MetalLayer;
                mMetalLayer = metalDesc->layer;
                break;
            }

            case wgpu::SType::SurfaceDescriptorFromWindowsHWND: {
                const SurfaceDescriptorFromWindowsHWND* hwndDesc =
                    static_cast<const SurfaceDescriptorFromWindowsHWND*>(chainedDescriptor);
                mType = Type::WindowsHWND;
                mHInstance = hwndDesc->hinstance;
                mHWND = hwndDesc->hwnd;
                break;
            }

            case wgpu::SType::SurfaceDescriptorFromXlib: {
                const SurfaceDescriptorFromXlib* xDesc =
                    static_cast<const SurfaceDescriptorFromXlib*>(chainedDescriptor);
                mType = Type::Xlib;
                mXDisplay = xDesc->display;
                mXWindow = xDesc->window;
                break;
            }

            default:
                UNREACHABLE();
        }
    }

    Surface::~Surface() {
        if (mSwapChain != nullptr) {
            mSwapChain->DetachFromSurface();
            mSwapChain = nullptr;
        }
    }

    NewSwapChainBase* Surface::GetAttachedSwapChain() const {
        return mSwapChain;
    }

    void Surface::SetAttachedSwapChain(NewSwapChainBase* swapChain) {
        mSwapChain = swapChain;
    }

    InstanceBase* Surface::GetInstance() {
        return mInstance.Get();
    }

    Surface::Type Surface::GetType() const {
        return mType;
    }

    void* Surface::GetMetalLayer() const {
        ASSERT(mType == Type::MetalLayer);
        return mMetalLayer;
    }

    void* Surface::GetHInstance() const {
        ASSERT(mType == Type::WindowsHWND);
        return mHInstance;
    }
    void* Surface::GetHWND() const {
        ASSERT(mType == Type::WindowsHWND);
        return mHWND;
    }

    void* Surface::GetXDisplay() const {
        ASSERT(mType == Type::Xlib);
        return mXDisplay;
    }
    uint32_t Surface::GetXWindow() const {
        ASSERT(mType == Type::Xlib);
        return mXWindow;
    }

}  // namespace dawn_native
