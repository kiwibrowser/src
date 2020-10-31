// Copyright 2019 The Dawn Authors
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

// A mock GLFW implementation that supports Fuchsia, but only implements
// the functions called from Dawn.

// NOTE: This must be included before GLFW/glfw3.h because the latter will
// include <vulkan/vulkan.h> and "common/vulkan_platform.h" wants to be
// the first header to do so for sanity reasons (e.g. undefining weird
// macros on Windows and Linux).
// clang-format off
#include "common/vulkan_platform.h"
#include "common/Assert.h"
#include <GLFW/glfw3.h>
// clang-format on

#include <dlfcn.h>

int glfwInit(void) {
    return GLFW_TRUE;
}

void glfwDefaultWindowHints(void) {
}

void glfwWindowHint(int hint, int value) {
    DAWN_UNUSED(hint);
    DAWN_UNUSED(value);
}

struct GLFWwindow {
    PFN_vkGetInstanceProcAddr GetInstanceProcAddress = nullptr;
    void* vulkan_loader = nullptr;

    GLFWwindow() {
        vulkan_loader = ::dlopen("libvulkan.so", RTLD_NOW);
        ASSERT(vulkan_loader != nullptr);
        GetInstanceProcAddress = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
            dlsym(vulkan_loader, "vkGetInstanceProcAddr"));
        ASSERT(GetInstanceProcAddress != nullptr);
    }

    ~GLFWwindow() {
        if (vulkan_loader) {
            ::dlclose(vulkan_loader);
        }
        vulkan_loader = nullptr;
    }
};

GLFWwindow* glfwCreateWindow(int width,
                             int height,
                             const char* title,
                             GLFWmonitor* monitor,
                             GLFWwindow* share) {
    ASSERT(monitor == nullptr);
    ASSERT(share == nullptr);
    DAWN_UNUSED(width);
    DAWN_UNUSED(height);
    DAWN_UNUSED(title);
    return new GLFWwindow();
}

VkResult glfwCreateWindowSurface(VkInstance instance,
                                 GLFWwindow* window,
                                 const VkAllocationCallbacks* allocator,
                                 VkSurfaceKHR* surface) {
    // IMPORTANT: This assumes that the VkInstance was created with a Fuchsia
    // swapchain layer enabled, as well as the corresponding extension that
    // is queried here to perform the surface creation. Dawn should do all
    // required steps in VulkanInfo.cpp, VulkanFunctions.cpp and BackendVk.cpp.

    auto vkCreateImagePipeSurfaceFUCHSIA = reinterpret_cast<PFN_vkCreateImagePipeSurfaceFUCHSIA>(
        window->GetInstanceProcAddress(instance, "vkCreateImagePipeSurfaceFUCHSIA"));
    ASSERT(vkCreateImagePipeSurfaceFUCHSIA != nullptr);
    if (!vkCreateImagePipeSurfaceFUCHSIA) {
        *surface = VK_NULL_HANDLE;
        return VK_ERROR_FEATURE_NOT_PRESENT;
    }

    const struct VkImagePipeSurfaceCreateInfoFUCHSIA create_info = {
        VK_STRUCTURE_TYPE_IMAGEPIPE_SURFACE_CREATE_INFO_FUCHSIA,
        nullptr,            // pNext
        0,                  // flags, ignored for now
        ZX_HANDLE_INVALID,  // imagePipeHandle, a null handle matches the framebuffer.
    };

    return vkCreateImagePipeSurfaceFUCHSIA(instance, &create_info, nullptr, surface);
}
