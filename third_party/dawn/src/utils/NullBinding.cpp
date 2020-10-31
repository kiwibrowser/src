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
#include "dawn_native/NullBackend.h"

#include <memory>

namespace utils {

    class NullBinding : public BackendBinding {
      public:
        NullBinding(GLFWwindow* window, WGPUDevice device) : BackendBinding(window, device) {
        }

        uint64_t GetSwapChainImplementation() override {
            if (mSwapchainImpl.userData == nullptr) {
                mSwapchainImpl = dawn_native::null::CreateNativeSwapChainImpl();
            }
            return reinterpret_cast<uint64_t>(&mSwapchainImpl);
        }
        WGPUTextureFormat GetPreferredSwapChainTextureFormat() override {
            return WGPUTextureFormat_RGBA8Unorm;
        }

      private:
        DawnSwapChainImplementation mSwapchainImpl = {};
    };

    BackendBinding* CreateNullBinding(GLFWwindow* window, WGPUDevice device) {
        return new NullBinding(window, device);
    }

}  // namespace utils
