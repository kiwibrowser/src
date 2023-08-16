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

#include "tests/DawnTest.h"

#include "dawn_native/MetalBackend.h"
#include "utils/ComboRenderPipelineDescriptor.h"
#include "utils/WGPUHelpers.h"

#include <CoreFoundation/CoreFoundation.h>
#include <CoreVideo/CVPixelBuffer.h>
#include <IOSurface/IOSurface.h>

namespace {

    void AddIntegerValue(CFMutableDictionaryRef dictionary, const CFStringRef key, int32_t value) {
        CFNumberRef number = CFNumberCreate(nullptr, kCFNumberSInt32Type, &value);
        CFDictionaryAddValue(dictionary, key, number);
        CFRelease(number);
    }

    class ScopedIOSurfaceRef {
      public:
        ScopedIOSurfaceRef() : mSurface(nullptr) {
        }
        explicit ScopedIOSurfaceRef(IOSurfaceRef surface) : mSurface(surface) {
        }

        ~ScopedIOSurfaceRef() {
            if (mSurface != nullptr) {
                CFRelease(mSurface);
                mSurface = nullptr;
            }
        }

        IOSurfaceRef get() const {
            return mSurface;
        }

        ScopedIOSurfaceRef(ScopedIOSurfaceRef&& other) {
            if (mSurface != nullptr) {
                CFRelease(mSurface);
            }
            mSurface = other.mSurface;
            other.mSurface = nullptr;
        }

        ScopedIOSurfaceRef& operator=(ScopedIOSurfaceRef&& other) {
            if (mSurface != nullptr) {
                CFRelease(mSurface);
            }
            mSurface = other.mSurface;
            other.mSurface = nullptr;

            return *this;
        }

        ScopedIOSurfaceRef(const ScopedIOSurfaceRef&) = delete;
        ScopedIOSurfaceRef& operator=(const ScopedIOSurfaceRef&) = delete;

      private:
        IOSurfaceRef mSurface = nullptr;
    };

    ScopedIOSurfaceRef CreateSinglePlaneIOSurface(uint32_t width,
                                                  uint32_t height,
                                                  uint32_t format,
                                                  uint32_t bytesPerElement) {
        CFMutableDictionaryRef dict =
            CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
                                      &kCFTypeDictionaryValueCallBacks);
        AddIntegerValue(dict, kIOSurfaceWidth, width);
        AddIntegerValue(dict, kIOSurfaceHeight, height);
        AddIntegerValue(dict, kIOSurfacePixelFormat, format);
        AddIntegerValue(dict, kIOSurfaceBytesPerElement, bytesPerElement);

        IOSurfaceRef ioSurface = IOSurfaceCreate(dict);
        EXPECT_NE(nullptr, ioSurface);
        CFRelease(dict);

        return ScopedIOSurfaceRef(ioSurface);
    }

    class IOSurfaceTestBase : public DawnTest {
      public:
        wgpu::Texture WrapIOSurface(const wgpu::TextureDescriptor* descriptor,
                                    IOSurfaceRef ioSurface,
                                    uint32_t plane,
                                    bool isCleared = true) {
            dawn_native::metal::ExternalImageDescriptorIOSurface externDesc;
            externDesc.cTextureDescriptor =
                reinterpret_cast<const WGPUTextureDescriptor*>(descriptor);
            externDesc.ioSurface = ioSurface;
            externDesc.plane = plane;
            externDesc.isCleared = isCleared;
            WGPUTexture texture = dawn_native::metal::WrapIOSurface(device.Get(), &externDesc);
            return wgpu::Texture::Acquire(texture);
        }
    };

}  // anonymous namespace

// A small fixture used to initialize default data for the IOSurface validation tests.
// These tests are skipped if the harness is using the wire.
class IOSurfaceValidationTests : public IOSurfaceTestBase {
  public:
    IOSurfaceValidationTests() {
        defaultIOSurface = CreateSinglePlaneIOSurface(10, 10, kCVPixelFormatType_32BGRA, 4);

        descriptor.dimension = wgpu::TextureDimension::e2D;
        descriptor.format = wgpu::TextureFormat::BGRA8Unorm;
        descriptor.size = {10, 10, 1};
        descriptor.sampleCount = 1;
        descriptor.mipLevelCount = 1;
        descriptor.usage = wgpu::TextureUsage::OutputAttachment;
    }

  protected:
    wgpu::TextureDescriptor descriptor;
    ScopedIOSurfaceRef defaultIOSurface;
};

// Test a successful wrapping of an IOSurface in a texture
TEST_P(IOSurfaceValidationTests, Success) {
    DAWN_SKIP_TEST_IF(UsesWire());
    wgpu::Texture texture = WrapIOSurface(&descriptor, defaultIOSurface.get(), 0);
    ASSERT_NE(texture.Get(), nullptr);
}

// Test an error occurs if the texture descriptor is invalid
TEST_P(IOSurfaceValidationTests, InvalidTextureDescriptor) {
    DAWN_SKIP_TEST_IF(UsesWire());

    wgpu::ChainedStruct chainedDescriptor;
    descriptor.nextInChain = &chainedDescriptor;

    ASSERT_DEVICE_ERROR(wgpu::Texture texture =
                            WrapIOSurface(&descriptor, defaultIOSurface.get(), 0));
    ASSERT_EQ(texture.Get(), nullptr);
}

// Test an error occurs if the plane is too large
TEST_P(IOSurfaceValidationTests, PlaneTooLarge) {
    DAWN_SKIP_TEST_IF(UsesWire());
    ASSERT_DEVICE_ERROR(wgpu::Texture texture =
                            WrapIOSurface(&descriptor, defaultIOSurface.get(), 1));
    ASSERT_EQ(texture.Get(), nullptr);
}

// Test an error occurs if the descriptor dimension isn't 2D
// TODO(cwallez@chromium.org): Reenable when 1D or 3D textures are implemented
TEST_P(IOSurfaceValidationTests, DISABLED_InvalidTextureDimension) {
    DAWN_SKIP_TEST_IF(UsesWire());
    descriptor.dimension = wgpu::TextureDimension::e2D;

    ASSERT_DEVICE_ERROR(wgpu::Texture texture =
                            WrapIOSurface(&descriptor, defaultIOSurface.get(), 0));
    ASSERT_EQ(texture.Get(), nullptr);
}

// Test an error occurs if the descriptor mip level count isn't 1
TEST_P(IOSurfaceValidationTests, InvalidMipLevelCount) {
    DAWN_SKIP_TEST_IF(UsesWire());
    descriptor.mipLevelCount = 2;

    ASSERT_DEVICE_ERROR(wgpu::Texture texture =
                            WrapIOSurface(&descriptor, defaultIOSurface.get(), 0));
    ASSERT_EQ(texture.Get(), nullptr);
}

// Test an error occurs if the descriptor depth isn't 1
TEST_P(IOSurfaceValidationTests, InvalidDepth) {
    DAWN_SKIP_TEST_IF(UsesWire());
    descriptor.size.depth = 2;

    ASSERT_DEVICE_ERROR(wgpu::Texture texture =
                            WrapIOSurface(&descriptor, defaultIOSurface.get(), 0));
    ASSERT_EQ(texture.Get(), nullptr);
}

// Test an error occurs if the descriptor sample count isn't 1
TEST_P(IOSurfaceValidationTests, InvalidSampleCount) {
    DAWN_SKIP_TEST_IF(UsesWire());
    descriptor.sampleCount = 4;

    ASSERT_DEVICE_ERROR(wgpu::Texture texture =
                            WrapIOSurface(&descriptor, defaultIOSurface.get(), 0));
    ASSERT_EQ(texture.Get(), nullptr);
}

// Test an error occurs if the descriptor width doesn't match the surface's
TEST_P(IOSurfaceValidationTests, InvalidWidth) {
    DAWN_SKIP_TEST_IF(UsesWire());
    descriptor.size.width = 11;

    ASSERT_DEVICE_ERROR(wgpu::Texture texture =
                            WrapIOSurface(&descriptor, defaultIOSurface.get(), 0));
    ASSERT_EQ(texture.Get(), nullptr);
}

// Test an error occurs if the descriptor height doesn't match the surface's
TEST_P(IOSurfaceValidationTests, InvalidHeight) {
    DAWN_SKIP_TEST_IF(UsesWire());
    descriptor.size.height = 11;

    ASSERT_DEVICE_ERROR(wgpu::Texture texture =
                            WrapIOSurface(&descriptor, defaultIOSurface.get(), 0));
    ASSERT_EQ(texture.Get(), nullptr);
}

// Test an error occurs if the descriptor format isn't compatible with the IOSurface's
TEST_P(IOSurfaceValidationTests, InvalidFormat) {
    DAWN_SKIP_TEST_IF(UsesWire());
    descriptor.format = wgpu::TextureFormat::R8Unorm;

    ASSERT_DEVICE_ERROR(wgpu::Texture texture =
                            WrapIOSurface(&descriptor, defaultIOSurface.get(), 0));
    ASSERT_EQ(texture.Get(), nullptr);
}

// Fixture to test using IOSurfaces through different usages.
// These tests are skipped if the harness is using the wire.
class IOSurfaceUsageTests : public IOSurfaceTestBase {
  public:
    // Test that sampling a 1x1 works.
    void DoSampleTest(IOSurfaceRef ioSurface,
                      wgpu::TextureFormat format,
                      void* data,
                      size_t dataSize,
                      RGBA8 expectedColor) {
        // Write the data to the IOSurface
        IOSurfaceLock(ioSurface, 0, nullptr);
        memcpy(IOSurfaceGetBaseAddress(ioSurface), data, dataSize);
        IOSurfaceUnlock(ioSurface, 0, nullptr);

        // The simplest texture sampling pipeline.
        wgpu::RenderPipeline pipeline;
        {
            wgpu::ShaderModule vs =
                utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
                #version 450
                layout (location = 0) out vec2 o_texCoord;
                void main() {
                    const vec2 pos[6] = vec2[6](vec2(-2.f, -2.f),
                                                vec2(-2.f,  2.f),
                                                vec2( 2.f, -2.f),
                                                vec2(-2.f,  2.f),
                                                vec2( 2.f, -2.f),
                                                vec2( 2.f,  2.f));
                    const vec2 texCoord[6] = vec2[6](vec2(0.f, 0.f),
                                                     vec2(0.f, 1.f),
                                                     vec2(1.f, 0.f),
                                                     vec2(0.f, 1.f),
                                                     vec2(1.f, 0.f),
                                                     vec2(1.f, 1.f));
                    gl_Position = vec4(pos[gl_VertexIndex], 0.f, 1.f);
                    o_texCoord = texCoord[gl_VertexIndex];
                }
            )");
            wgpu::ShaderModule fs =
                utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, R"(
                #version 450
                layout(set = 0, binding = 0) uniform sampler sampler0;
                layout(set = 0, binding = 1) uniform texture2D texture0;
                layout(location = 0) in vec2 texCoord;
                layout(location = 0) out vec4 fragColor;

                void main() {
                    fragColor = texture(sampler2D(texture0, sampler0), texCoord);
                }
            )");

            utils::ComboRenderPipelineDescriptor descriptor(device);
            descriptor.vertexStage.module = vs;
            descriptor.cFragmentStage.module = fs;
            descriptor.cColorStates[0].format = wgpu::TextureFormat::RGBA8Unorm;

            pipeline = device.CreateRenderPipeline(&descriptor);
        }

        // The bindgroup containing the texture view for the ioSurface as well as the sampler.
        wgpu::BindGroup bindGroup;
        {
            wgpu::TextureDescriptor textureDescriptor;
            textureDescriptor.dimension = wgpu::TextureDimension::e2D;
            textureDescriptor.format = format;
            textureDescriptor.size = {1, 1, 1};
            textureDescriptor.sampleCount = 1;
            textureDescriptor.mipLevelCount = 1;
            textureDescriptor.usage = wgpu::TextureUsage::Sampled;
            wgpu::Texture wrappingTexture = WrapIOSurface(&textureDescriptor, ioSurface, 0);

            wgpu::TextureView textureView = wrappingTexture.CreateView();

            wgpu::SamplerDescriptor samplerDescriptor = utils::GetDefaultSamplerDescriptor();
            wgpu::Sampler sampler = device.CreateSampler(&samplerDescriptor);

            bindGroup = utils::MakeBindGroup(device, pipeline.GetBindGroupLayout(0),
                                             {{0, sampler}, {1, textureView}});
        }

        // Submit commands samping from the ioSurface and writing the result to renderPass.color
        utils::BasicRenderPass renderPass = utils::CreateBasicRenderPass(device, 1, 1);
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        {
            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
            pass.SetPipeline(pipeline);
            pass.SetBindGroup(0, bindGroup);
            pass.Draw(6);
            pass.EndPass();
        }

        wgpu::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);

        EXPECT_PIXEL_RGBA8_EQ(expectedColor, renderPass.color, 0, 0);
    }

    // Test that clearing using BeginRenderPass writes correct data in the ioSurface.
    void DoClearTest(IOSurfaceRef ioSurface,
                     wgpu::TextureFormat format,
                     void* data,
                     size_t dataSize) {
        // Get a texture view for the ioSurface
        wgpu::TextureDescriptor textureDescriptor;
        textureDescriptor.dimension = wgpu::TextureDimension::e2D;
        textureDescriptor.format = format;
        textureDescriptor.size = {1, 1, 1};
        textureDescriptor.sampleCount = 1;
        textureDescriptor.mipLevelCount = 1;
        textureDescriptor.usage = wgpu::TextureUsage::OutputAttachment;
        wgpu::Texture ioSurfaceTexture = WrapIOSurface(&textureDescriptor, ioSurface, 0);

        wgpu::TextureView ioSurfaceView = ioSurfaceTexture.CreateView();

        utils::ComboRenderPassDescriptor renderPassDescriptor({ioSurfaceView}, {});
        renderPassDescriptor.cColorAttachments[0].clearColor = {1 / 255.0f, 2 / 255.0f, 3 / 255.0f,
                                                                4 / 255.0f};

        // Execute commands to clear the ioSurface
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDescriptor);
        pass.EndPass();

        wgpu::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);

        // Wait for the commands touching the IOSurface to be scheduled
        dawn_native::metal::WaitForCommandsToBeScheduled(device.Get());

        // Check the correct data was written
        IOSurfaceLock(ioSurface, kIOSurfaceLockReadOnly, nullptr);
        ASSERT_EQ(0, memcmp(IOSurfaceGetBaseAddress(ioSurface), data, dataSize));
        IOSurfaceUnlock(ioSurface, kIOSurfaceLockReadOnly, nullptr);
    }
};

// Test sampling from a R8 IOSurface
TEST_P(IOSurfaceUsageTests, SampleFromR8IOSurface) {
    DAWN_SKIP_TEST_IF(UsesWire());
    ScopedIOSurfaceRef ioSurface =
        CreateSinglePlaneIOSurface(1, 1, kCVPixelFormatType_OneComponent8, 1);

    uint8_t data = 0x01;
    DoSampleTest(ioSurface.get(), wgpu::TextureFormat::R8Unorm, &data, sizeof(data),
                 RGBA8(1, 0, 0, 255));
}

// Test clearing a R8 IOSurface
TEST_P(IOSurfaceUsageTests, ClearR8IOSurface) {
    DAWN_SKIP_TEST_IF(UsesWire());
    ScopedIOSurfaceRef ioSurface =
        CreateSinglePlaneIOSurface(1, 1, kCVPixelFormatType_OneComponent8, 1);

    uint8_t data = 0x01;
    DoClearTest(ioSurface.get(), wgpu::TextureFormat::R8Unorm, &data, sizeof(data));
}

// Test sampling from a RG8 IOSurface
TEST_P(IOSurfaceUsageTests, SampleFromRG8IOSurface) {
    DAWN_SKIP_TEST_IF(UsesWire());
    ScopedIOSurfaceRef ioSurface =
        CreateSinglePlaneIOSurface(1, 1, kCVPixelFormatType_TwoComponent8, 2);

    uint16_t data = 0x0102;  // Stored as (G, R)
    DoSampleTest(ioSurface.get(), wgpu::TextureFormat::RG8Unorm, &data, sizeof(data),
                 RGBA8(2, 1, 0, 255));
}

// Test clearing a RG8 IOSurface
TEST_P(IOSurfaceUsageTests, ClearRG8IOSurface) {
    DAWN_SKIP_TEST_IF(UsesWire());
    ScopedIOSurfaceRef ioSurface =
        CreateSinglePlaneIOSurface(1, 1, kCVPixelFormatType_TwoComponent8, 2);

    uint16_t data = 0x0201;
    DoClearTest(ioSurface.get(), wgpu::TextureFormat::RG8Unorm, &data, sizeof(data));
}

// Test sampling from a BGRA8 IOSurface
TEST_P(IOSurfaceUsageTests, SampleFromBGRA8IOSurface) {
    DAWN_SKIP_TEST_IF(UsesWire());
    ScopedIOSurfaceRef ioSurface = CreateSinglePlaneIOSurface(1, 1, kCVPixelFormatType_32BGRA, 4);

    uint32_t data = 0x01020304;  // Stored as (A, R, G, B)
    DoSampleTest(ioSurface.get(), wgpu::TextureFormat::BGRA8Unorm, &data, sizeof(data),
                 RGBA8(2, 3, 4, 1));
}

// Test clearing a BGRA8 IOSurface
TEST_P(IOSurfaceUsageTests, ClearBGRA8IOSurface) {
    DAWN_SKIP_TEST_IF(UsesWire());
    ScopedIOSurfaceRef ioSurface = CreateSinglePlaneIOSurface(1, 1, kCVPixelFormatType_32BGRA, 4);

    uint32_t data = 0x04010203;
    DoClearTest(ioSurface.get(), wgpu::TextureFormat::BGRA8Unorm, &data, sizeof(data));
}

// Test sampling from an RGBA8 IOSurface
TEST_P(IOSurfaceUsageTests, SampleFromRGBA8IOSurface) {
    DAWN_SKIP_TEST_IF(UsesWire());
    ScopedIOSurfaceRef ioSurface = CreateSinglePlaneIOSurface(1, 1, kCVPixelFormatType_32RGBA, 4);

    uint32_t data = 0x01020304;  // Stored as (A, B, G, R)
    DoSampleTest(ioSurface.get(), wgpu::TextureFormat::RGBA8Unorm, &data, sizeof(data),
                 RGBA8(4, 3, 2, 1));
}

// Test clearing an RGBA8 IOSurface
TEST_P(IOSurfaceUsageTests, ClearRGBA8IOSurface) {
    DAWN_SKIP_TEST_IF(UsesWire());
    ScopedIOSurfaceRef ioSurface = CreateSinglePlaneIOSurface(1, 1, kCVPixelFormatType_32RGBA, 4);

    uint32_t data = 0x04030201;
    DoClearTest(ioSurface.get(), wgpu::TextureFormat::RGBA8Unorm, &data, sizeof(data));
}

// Test that texture with color is cleared when isCleared = false
TEST_P(IOSurfaceUsageTests, UnclearedTextureIsCleared) {
    DAWN_SKIP_TEST_IF(UsesWire());

    ScopedIOSurfaceRef ioSurface = CreateSinglePlaneIOSurface(1, 1, kCVPixelFormatType_32RGBA, 4);
    uint32_t data = 0x04030201;

    IOSurfaceLock(ioSurface.get(), 0, nullptr);
    memcpy(IOSurfaceGetBaseAddress(ioSurface.get()), &data, sizeof(data));
    IOSurfaceUnlock(ioSurface.get(), 0, nullptr);

    wgpu::TextureDescriptor textureDescriptor;
    textureDescriptor.dimension = wgpu::TextureDimension::e2D;
    textureDescriptor.format = wgpu::TextureFormat::RGBA8Unorm;
    textureDescriptor.size = {1, 1, 1};
    textureDescriptor.sampleCount = 1;
    textureDescriptor.mipLevelCount = 1;
    textureDescriptor.usage = wgpu::TextureUsage::OutputAttachment | wgpu::TextureUsage::CopySrc;

    // wrap ioSurface and ensure color is not visible when isCleared set to false
    wgpu::Texture ioSurfaceTexture = WrapIOSurface(&textureDescriptor, ioSurface.get(), 0, false);
    EXPECT_PIXEL_RGBA8_EQ(RGBA8(0, 0, 0, 0), ioSurfaceTexture, 0, 0);
}

DAWN_INSTANTIATE_TEST(IOSurfaceValidationTests, MetalBackend());
DAWN_INSTANTIATE_TEST(IOSurfaceUsageTests, MetalBackend());
