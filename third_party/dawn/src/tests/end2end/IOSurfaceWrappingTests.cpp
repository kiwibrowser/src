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
#include "utils/DawnHelpers.h"

#include <CoreFoundation/CoreFoundation.h>
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
        dawn::Texture WrapIOSurface(const dawn::TextureDescriptor* descriptor,
                                    IOSurfaceRef ioSurface,
                                    uint32_t plane) {
            DawnTexture texture = dawn_native::metal::WrapIOSurface(
                device.Get(), reinterpret_cast<const DawnTextureDescriptor*>(descriptor), ioSurface,
                plane);
            return dawn::Texture::Acquire(texture);
        }
    };

}  // anonymous namespace

// A small fixture used to initialize default data for the IOSurface validation tests.
// These tests are skipped if the harness is using the wire.
class IOSurfaceValidationTests : public IOSurfaceTestBase {
  public:
    IOSurfaceValidationTests() {
        defaultIOSurface = CreateSinglePlaneIOSurface(10, 10, 'BGRA', 4);

        descriptor.dimension = dawn::TextureDimension::e2D;
        descriptor.format = dawn::TextureFormat::B8G8R8A8Unorm;
        descriptor.size = {10, 10, 1};
        descriptor.sampleCount = 1;
        descriptor.arrayLayerCount = 1;
        descriptor.mipLevelCount = 1;
        descriptor.usage = dawn::TextureUsageBit::OutputAttachment;
    }

  protected:
    dawn::TextureDescriptor descriptor;
    ScopedIOSurfaceRef defaultIOSurface;
};

// Test a successful wrapping of an IOSurface in a texture
TEST_P(IOSurfaceValidationTests, Success) {
    DAWN_SKIP_TEST_IF(UsesWire());
    dawn::Texture texture = WrapIOSurface(&descriptor, defaultIOSurface.get(), 0);
    ASSERT_NE(texture.Get(), nullptr);
}

// Test an error occurs if the texture descriptor is invalid
TEST_P(IOSurfaceValidationTests, InvalidTextureDescriptor) {
    DAWN_SKIP_TEST_IF(UsesWire());
    descriptor.nextInChain = this;

    ASSERT_DEVICE_ERROR(dawn::Texture texture =
                            WrapIOSurface(&descriptor, defaultIOSurface.get(), 0));
    ASSERT_EQ(texture.Get(), nullptr);
}

// Test an error occurs if the plane is too large
TEST_P(IOSurfaceValidationTests, PlaneTooLarge) {
    DAWN_SKIP_TEST_IF(UsesWire());
    ASSERT_DEVICE_ERROR(dawn::Texture texture =
                            WrapIOSurface(&descriptor, defaultIOSurface.get(), 1));
    ASSERT_EQ(texture.Get(), nullptr);
}

// Test an error occurs if the descriptor dimension isn't 2D
// TODO(cwallez@chromium.org): Reenable when 1D or 3D textures are implemented
TEST_P(IOSurfaceValidationTests, DISABLED_InvalidTextureDimension) {
    DAWN_SKIP_TEST_IF(UsesWire());
    descriptor.dimension = dawn::TextureDimension::e2D;

    ASSERT_DEVICE_ERROR(dawn::Texture texture =
                            WrapIOSurface(&descriptor, defaultIOSurface.get(), 0));
    ASSERT_EQ(texture.Get(), nullptr);
}

// Test an error occurs if the descriptor mip level count isn't 1
TEST_P(IOSurfaceValidationTests, InvalidMipLevelCount) {
    DAWN_SKIP_TEST_IF(UsesWire());
    descriptor.mipLevelCount = 2;

    ASSERT_DEVICE_ERROR(dawn::Texture texture =
                            WrapIOSurface(&descriptor, defaultIOSurface.get(), 0));
    ASSERT_EQ(texture.Get(), nullptr);
}

// Test an error occurs if the descriptor array layer count isn't 1
TEST_P(IOSurfaceValidationTests, InvalidArrayLayerCount) {
    DAWN_SKIP_TEST_IF(UsesWire());
    descriptor.arrayLayerCount = 2;

    ASSERT_DEVICE_ERROR(dawn::Texture texture =
                            WrapIOSurface(&descriptor, defaultIOSurface.get(), 0));
    ASSERT_EQ(texture.Get(), nullptr);
}

// Test an error occurs if the descriptor sample count isn't 1
TEST_P(IOSurfaceValidationTests, InvalidSampleCount) {
    DAWN_SKIP_TEST_IF(UsesWire());
    descriptor.sampleCount = 4;

    ASSERT_DEVICE_ERROR(dawn::Texture texture =
                            WrapIOSurface(&descriptor, defaultIOSurface.get(), 0));
    ASSERT_EQ(texture.Get(), nullptr);
}

// Test an error occurs if the descriptor width doesn't match the surface's
TEST_P(IOSurfaceValidationTests, InvalidWidth) {
    DAWN_SKIP_TEST_IF(UsesWire());
    descriptor.size.width = 11;

    ASSERT_DEVICE_ERROR(dawn::Texture texture =
                            WrapIOSurface(&descriptor, defaultIOSurface.get(), 0));
    ASSERT_EQ(texture.Get(), nullptr);
}

// Test an error occurs if the descriptor height doesn't match the surface's
TEST_P(IOSurfaceValidationTests, InvalidHeight) {
    DAWN_SKIP_TEST_IF(UsesWire());
    descriptor.size.height = 11;

    ASSERT_DEVICE_ERROR(dawn::Texture texture =
                            WrapIOSurface(&descriptor, defaultIOSurface.get(), 0));
    ASSERT_EQ(texture.Get(), nullptr);
}

// Test an error occurs if the descriptor format isn't compatible with the IOSurface's
TEST_P(IOSurfaceValidationTests, InvalidFormat) {
    DAWN_SKIP_TEST_IF(UsesWire());
    descriptor.format = dawn::TextureFormat::R8Unorm;

    ASSERT_DEVICE_ERROR(dawn::Texture texture =
                            WrapIOSurface(&descriptor, defaultIOSurface.get(), 0));
    ASSERT_EQ(texture.Get(), nullptr);
}

// Fixture to test using IOSurfaces through different usages.
// These tests are skipped if the harness is using the wire.
class IOSurfaceUsageTests : public IOSurfaceTestBase {
  public:
    // Test that sampling a 1x1 works.
    void DoSampleTest(IOSurfaceRef ioSurface,
                      dawn::TextureFormat format,
                      void* data,
                      size_t dataSize,
                      RGBA8 expectedColor) {
        // Write the data to the IOSurface
        IOSurfaceLock(ioSurface, 0, nullptr);
        memcpy(IOSurfaceGetBaseAddress(ioSurface), data, dataSize);
        IOSurfaceUnlock(ioSurface, 0, nullptr);

        // The bindgroup containing the texture view for the ioSurface as well as the sampler.
        dawn::BindGroupLayout bgl;
        dawn::BindGroup bindGroup;
        {
            dawn::TextureDescriptor textureDescriptor;
            textureDescriptor.dimension = dawn::TextureDimension::e2D;
            textureDescriptor.format = format;
            textureDescriptor.size = {1, 1, 1};
            textureDescriptor.sampleCount = 1;
            textureDescriptor.arrayLayerCount = 1;
            textureDescriptor.mipLevelCount = 1;
            textureDescriptor.usage = dawn::TextureUsageBit::Sampled;
            dawn::Texture wrappingTexture = WrapIOSurface(&textureDescriptor, ioSurface, 0);

            dawn::TextureView textureView = wrappingTexture.CreateDefaultView();

            dawn::SamplerDescriptor samplerDescriptor = utils::GetDefaultSamplerDescriptor();
            dawn::Sampler sampler = device.CreateSampler(&samplerDescriptor);

            bgl = utils::MakeBindGroupLayout(
                device, {
                            {0, dawn::ShaderStageBit::Fragment, dawn::BindingType::Sampler},
                            {1, dawn::ShaderStageBit::Fragment, dawn::BindingType::SampledTexture},
                        });

            bindGroup = utils::MakeBindGroup(device, bgl, {{0, sampler}, {1, textureView}});
        }

        // The simplest texture sampling pipeline.
        dawn::RenderPipeline pipeline;
        {
            dawn::ShaderModule vs = utils::CreateShaderModule(device, dawn::ShaderStage::Vertex, R"(
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
            dawn::ShaderModule fs =
                utils::CreateShaderModule(device, dawn::ShaderStage::Fragment, R"(
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
            descriptor.cVertexStage.module = vs;
            descriptor.cFragmentStage.module = fs;
            descriptor.layout = utils::MakeBasicPipelineLayout(device, &bgl);
            descriptor.cColorStates[0]->format = dawn::TextureFormat::R8G8B8A8Unorm;

            pipeline = device.CreateRenderPipeline(&descriptor);
        }

        // Submit commands samping from the ioSurface and writing the result to renderPass.color
        utils::BasicRenderPass renderPass = utils::CreateBasicRenderPass(device, 1, 1);
        dawn::CommandEncoder encoder = device.CreateCommandEncoder();
        {
            dawn::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
            pass.SetPipeline(pipeline);
            pass.SetBindGroup(0, bindGroup, 0, nullptr);
            pass.Draw(6, 1, 0, 0);
            pass.EndPass();
        }

        dawn::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);

        EXPECT_PIXEL_RGBA8_EQ(expectedColor, renderPass.color, 0, 0);
    }

    // Test that clearing using BeginRenderPass writes correct data in the ioSurface.
    void DoClearTest(IOSurfaceRef ioSurface,
                     dawn::TextureFormat format,
                     void* data,
                     size_t dataSize) {
        // Get a texture view for the ioSurface
        dawn::TextureDescriptor textureDescriptor;
        textureDescriptor.dimension = dawn::TextureDimension::e2D;
        textureDescriptor.format = format;
        textureDescriptor.size = {1, 1, 1};
        textureDescriptor.sampleCount = 1;
        textureDescriptor.arrayLayerCount = 1;
        textureDescriptor.mipLevelCount = 1;
        textureDescriptor.usage = dawn::TextureUsageBit::OutputAttachment;
        dawn::Texture ioSurfaceTexture = WrapIOSurface(&textureDescriptor, ioSurface, 0);

        dawn::TextureView ioSurfaceView = ioSurfaceTexture.CreateDefaultView();

        utils::ComboRenderPassDescriptor renderPassDescriptor({ioSurfaceView}, {});
        renderPassDescriptor.cColorAttachmentsInfoPtr[0]->clearColor = {1 / 255.0f, 2 / 255.0f,
                                                                        3 / 255.0f, 4 / 255.0f};

        // Execute commands to clear the ioSurface
        dawn::CommandEncoder encoder = device.CreateCommandEncoder();
        dawn::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDescriptor);
        pass.EndPass();

        dawn::CommandBuffer commands = encoder.Finish();
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
    ScopedIOSurfaceRef ioSurface = CreateSinglePlaneIOSurface(1, 1, 'L008', 1);

    uint8_t data = 0x01;
    DoSampleTest(ioSurface.get(), dawn::TextureFormat::R8Unorm, &data, sizeof(data),
                 RGBA8(1, 0, 0, 255));
}

// Test clearing a R8 IOSurface
TEST_P(IOSurfaceUsageTests, ClearR8IOSurface) {
    DAWN_SKIP_TEST_IF(UsesWire());
    ScopedIOSurfaceRef ioSurface = CreateSinglePlaneIOSurface(1, 1, 'L008', 1);

    uint8_t data = 0x01;
    DoClearTest(ioSurface.get(), dawn::TextureFormat::R8Unorm, &data, sizeof(data));
}

// Test sampling from a RG8 IOSurface
TEST_P(IOSurfaceUsageTests, SampleFromRG8IOSurface) {
    DAWN_SKIP_TEST_IF(UsesWire());
    ScopedIOSurfaceRef ioSurface = CreateSinglePlaneIOSurface(1, 1, '2C08', 2);

    uint16_t data = 0x0102;  // Stored as (G, R)
    DoSampleTest(ioSurface.get(), dawn::TextureFormat::R8G8Unorm, &data, sizeof(data),
                 RGBA8(2, 1, 0, 255));
}

// Test clearing a RG8 IOSurface
TEST_P(IOSurfaceUsageTests, ClearRG8IOSurface) {
    DAWN_SKIP_TEST_IF(UsesWire());
    ScopedIOSurfaceRef ioSurface = CreateSinglePlaneIOSurface(1, 1, '2C08', 2);

    uint16_t data = 0x0201;
    DoClearTest(ioSurface.get(), dawn::TextureFormat::R8G8Unorm, &data, sizeof(data));
}

// Test sampling from a BGRA8 IOSurface
TEST_P(IOSurfaceUsageTests, SampleFromBGRA8888IOSurface) {
    DAWN_SKIP_TEST_IF(UsesWire());
    ScopedIOSurfaceRef ioSurface = CreateSinglePlaneIOSurface(1, 1, 'BGRA', 4);

    uint32_t data = 0x01020304;  // Stored as (A, R, G, B)
    DoSampleTest(ioSurface.get(), dawn::TextureFormat::B8G8R8A8Unorm, &data, sizeof(data),
                 RGBA8(2, 3, 4, 1));
}

// Test clearing a BGRA8 IOSurface
TEST_P(IOSurfaceUsageTests, ClearBGRA8IOSurface) {
    DAWN_SKIP_TEST_IF(UsesWire());
    ScopedIOSurfaceRef ioSurface = CreateSinglePlaneIOSurface(1, 1, 'BGRA', 4);

    uint32_t data = 0x04010203;
    DoClearTest(ioSurface.get(), dawn::TextureFormat::B8G8R8A8Unorm, &data, sizeof(data));
}

DAWN_INSTANTIATE_TEST(IOSurfaceValidationTests, MetalBackend);
DAWN_INSTANTIATE_TEST(IOSurfaceUsageTests, MetalBackend);
