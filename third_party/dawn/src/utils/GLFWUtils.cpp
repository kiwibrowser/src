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

#include "utils/GLFWUtils.h"

#include "GLFW/glfw3.h"
#include "common/Platform.h"

#include <cstdlib>

#if defined(DAWN_PLATFORM_WINDOWS)
#    define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(DAWN_USE_X11)
#    define GLFW_EXPOSE_NATIVE_X11
#endif
#include "GLFW/glfw3native.h"

namespace utils {

    void SetupGLFWWindowHintsForBackend(wgpu::BackendType type) {
        if (type == wgpu::BackendType::OpenGL) {
            // Ask for OpenGL 4.4 which is what the GL backend requires for compute shaders and
            // texture views.
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        } else {
            // Without this GLFW will initialize a GL context on the window, which prevents using
            // the window with other APIs (by crashing in weird ways).
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        }
    }

    wgpu::Surface CreateSurfaceForWindow(wgpu::Instance instance, GLFWwindow* window) {
        std::unique_ptr<wgpu::ChainedStruct> chainedDescriptor =
            SetupWindowAndGetSurfaceDescriptorForTesting(window);

        wgpu::SurfaceDescriptor descriptor;
        descriptor.nextInChain = chainedDescriptor.get();
        wgpu::Surface surface = instance.CreateSurface(&descriptor);

        return surface;
    }

#if defined(DAWN_PLATFORM_WINDOWS)
    std::unique_ptr<wgpu::ChainedStruct> SetupWindowAndGetSurfaceDescriptorForTesting(
        GLFWwindow* window) {
        std::unique_ptr<wgpu::SurfaceDescriptorFromWindowsHWND> desc =
            std::make_unique<wgpu::SurfaceDescriptorFromWindowsHWND>();
        desc->hwnd = glfwGetWin32Window(window);
        desc->hinstance = GetModuleHandle(nullptr);
        return std::move(desc);
    }
#elif defined(DAWN_USE_X11)
    std::unique_ptr<wgpu::ChainedStruct> SetupWindowAndGetSurfaceDescriptorForTesting(
        GLFWwindow* window) {
        std::unique_ptr<wgpu::SurfaceDescriptorFromXlib> desc =
            std::make_unique<wgpu::SurfaceDescriptorFromXlib>();
        desc->display = glfwGetX11Display();
        desc->window = glfwGetX11Window(window);
        return std::move(desc);
    }
#elif defined(DAWN_ENABLE_BACKEND_METAL)
    // SetupWindowAndGetSurfaceDescriptorForTesting defined in GLFWUtils_metal.mm
#else
    std::unique_ptr<wgpu::ChainedStruct> SetupWindowAndGetSurfaceDescriptorForTesting(GLFWwindow*) {
        return nullptr;
    }
#endif

}  // namespace utils
