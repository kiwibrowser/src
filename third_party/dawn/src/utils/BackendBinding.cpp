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
    BackendBinding* CreateD3D12Binding(GLFWwindow* window, DawnDevice device);
#endif
#if defined(DAWN_ENABLE_BACKEND_METAL)
    BackendBinding* CreateMetalBinding(GLFWwindow* window, DawnDevice device);
#endif
#if defined(DAWN_ENABLE_BACKEND_NULL)
    BackendBinding* CreateNullBinding(GLFWwindow* window, DawnDevice device);
#endif
#if defined(DAWN_ENABLE_BACKEND_OPENGL)
    BackendBinding* CreateOpenGLBinding(GLFWwindow* window, DawnDevice device);
#endif
#if defined(DAWN_ENABLE_BACKEND_VULKAN)
    BackendBinding* CreateVulkanBinding(GLFWwindow* window, DawnDevice device);
#endif

    BackendBinding::BackendBinding(GLFWwindow* window, DawnDevice device)
        : mWindow(window), mDevice(device) {
    }

    void SetupGLFWWindowHintsForBackend(dawn_native::BackendType type) {
        if (type == dawn_native::BackendType::OpenGL) {
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        } else {
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        }
    }

    void DiscoverAdapter(dawn_native::Instance* instance,
                         GLFWwindow* window,
                         dawn_native::BackendType type) {
        DAWN_UNUSED(type);
        DAWN_UNUSED(window);

        if (type == dawn_native::BackendType::OpenGL) {
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

    BackendBinding* CreateBinding(dawn_native::BackendType type,
                                  GLFWwindow* window,
                                  DawnDevice device) {
        switch (type) {
#if defined(DAWN_ENABLE_BACKEND_D3D12)
            case dawn_native::BackendType::D3D12:
                return CreateD3D12Binding(window, device);
#endif

#if defined(DAWN_ENABLE_BACKEND_METAL)
            case dawn_native::BackendType::Metal:
                return CreateMetalBinding(window, device);
#endif

#if defined(DAWN_ENABLE_BACKEND_NULL)
            case dawn_native::BackendType::Null:
                return CreateNullBinding(window, device);
#endif

#if defined(DAWN_ENABLE_BACKEND_OPENGL)
            case dawn_native::BackendType::OpenGL:
                return CreateOpenGLBinding(window, device);
#endif

#if defined(DAWN_ENABLE_BACKEND_VULKAN)
            case dawn_native::BackendType::Vulkan:
                return CreateVulkanBinding(window, device);
#endif

            default:
                return nullptr;
        }
    }

}  // namespace utils
