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

#include "common/Assert.h"
#include "dawn_native/D3D12Backend.h"

#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"

#include <memory>

namespace utils {

    class D3D12Binding : public BackendBinding {
      public:
        D3D12Binding(GLFWwindow* window, WGPUDevice device) : BackendBinding(window, device) {
        }

        uint64_t GetSwapChainImplementation() override {
            if (mSwapchainImpl.userData == nullptr) {
                HWND win32Window = glfwGetWin32Window(mWindow);
                mSwapchainImpl =
                    dawn_native::d3d12::CreateNativeSwapChainImpl(mDevice, win32Window);
            }
            return reinterpret_cast<uint64_t>(&mSwapchainImpl);
        }

        WGPUTextureFormat GetPreferredSwapChainTextureFormat() override {
            ASSERT(mSwapchainImpl.userData != nullptr);
            return dawn_native::d3d12::GetNativeSwapChainPreferredFormat(&mSwapchainImpl);
        }

      private:
        DawnSwapChainImplementation mSwapchainImpl = {};
    };

    BackendBinding* CreateD3D12Binding(GLFWwindow* window, WGPUDevice device) {
        return new D3D12Binding(window, device);
    }

}  // namespace utils
