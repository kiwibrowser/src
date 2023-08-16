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

#include "utils/BackendBinding.h"

#include "common/Compiler.h"

#include "GLFW/glfw3.h"

#if defined(DAWN_ENABLE_BACKEND_OPENGL)
#    include "dawn_native/OpenGLBackend.h"
#endif  // defined(DAWN_ENABLE_BACKEND_OPENGL)

namespace utils {

#if defined(DAWN_ENABLE_BACKEND_D3D12)
    BackendBinding* CreateD3D12Binding(GLFWwindow* window, WGPUDevice device);
#endif
#if defined(DAWN_ENABLE_BACKEND_METAL)
    BackendBinding* CreateMetalBinding(GLFWwindow* window, WGPUDevice device);
#endif
#if defined(DAWN_ENABLE_BACKEND_NULL)
    BackendBinding* CreateNullBinding(GLFWwindow* window, WGPUDevice device);
#endif
#if defined(DAWN_ENABLE_BACKEND_OPENGL)
    BackendBinding* CreateOpenGLBinding(GLFWwindow* window, WGPUDevice device);
#endif
#if defined(DAWN_ENABLE_BACKEND_VULKAN)
    BackendBinding* CreateVulkanBinding(GLFWwindow* window, WGPUDevice device);
#endif

    BackendBinding::BackendBinding(GLFWwindow* window, WGPUDevice device)
        : mWindow(window), mDevice(device) {
    }

    void DiscoverAdapter(dawn_native::Instance* instance,
                         GLFWwindow* window,
                         wgpu::BackendType type) {
        DAWN_UNUSED(type);
        DAWN_UNUSED(window);

        if (type == wgpu::BackendType::OpenGL) {
#if defined(DAWN_ENABLE_BACKEND_OPENGL)
            glfwMakeContextCurrent(window);
            dawn_native::opengl::AdapterDiscoveryOptions adapterOptions;
            adapterOptions.getProc = reinterpret_cast<void* (*)(const char*)>(glfwGetProcAddress);
            instance->DiscoverAdapters(&adapterOptions);
#endif  // defined(DAWN_ENABLE_BACKEND_OPENGL)
        } else {
            instance->DiscoverDefaultAdapters();
        }
    }

    BackendBinding* CreateBinding(wgpu::BackendType type, GLFWwindow* window, WGPUDevice device) {
        switch (type) {
#if defined(DAWN_ENABLE_BACKEND_D3D12)
            case wgpu::BackendType::D3D12:
                return CreateD3D12Binding(window, device);
#endif

#if defined(DAWN_ENABLE_BACKEND_METAL)
            case wgpu::BackendType::Metal:
                return CreateMetalBinding(window, device);
#endif

#if defined(DAWN_ENABLE_BACKEND_NULL)
            case wgpu::BackendType::Null:
                return CreateNullBinding(window, device);
#endif

#if defined(DAWN_ENABLE_BACKEND_OPENGL)
            case wgpu::BackendType::OpenGL:
                return CreateOpenGLBinding(window, device);
#endif

#if defined(DAWN_ENABLE_BACKEND_VULKAN)
            case wgpu::BackendType::Vulkan:
                return CreateVulkanBinding(window, device);
#endif

            default:
                return nullptr;
        }
    }

}  // namespace utils
