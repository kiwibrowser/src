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

#ifndef TESTS_UNITTESTS_VALIDATIONTEST_H_
#define TESTS_UNITTESTS_VALIDATIONTEST_H_

#include "dawn/webgpu_cpp.h"
#include "dawn_native/DawnNative.h"
#include "gtest/gtest.h"

#define ASSERT_DEVICE_ERROR(statement)                          \
    StartExpectDeviceError();                                   \
    statement;                                                  \
    if (!EndExpectDeviceError()) {                              \
        FAIL() << "Expected device error in:\n " << #statement; \
    }                                                           \
    do {                                                        \
    } while (0)

class ValidationTest : public testing::Test {
  public:
    ValidationTest();
    ~ValidationTest() override;

    wgpu::Device CreateDeviceFromAdapter(dawn_native::Adapter adapter,
                                         const std::vector<const char*>& requiredExtensions);

    void TearDown() override;

    void StartExpectDeviceError();
    bool EndExpectDeviceError();
    std::string GetLastDeviceErrorMessage() const;

    void WaitForAllOperations(const wgpu::Device& device) const;

    // Helper functions to create objects to test validation.

    struct DummyRenderPass : public wgpu::RenderPassDescriptor {
      public:
        DummyRenderPass(const wgpu::Device& device);
        wgpu::Texture attachment;
        wgpu::TextureFormat attachmentFormat;
        uint32_t width;
        uint32_t height;

      private:
        wgpu::RenderPassColorAttachmentDescriptor mColorAttachment;
    };

  protected:
    wgpu::Device device;
    dawn_native::Adapter adapter;
    std::unique_ptr<dawn_native::Instance> instance;

  private:
    static void OnDeviceError(WGPUErrorType type, const char* message, void* userdata);
    std::string mDeviceErrorMessage;
    bool mExpectError = false;
    bool mError = false;
};

#endif  // TESTS_UNITTESTS_VALIDATIONTEST_H_
