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

#include "tests/unittests/validation/ValidationTest.h"

#include "common/Constants.h"
#include "utils/ComboRenderPipelineDescriptor.h"
#include "utils/WGPUHelpers.h"

#include <cmath>
#include <sstream>

class RenderPipelineValidationTest : public ValidationTest {
  protected:
    void SetUp() override {
        ValidationTest::SetUp();

        vsModule = utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
                #version 450
                void main() {
                    gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
                })");

        fsModule = utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, R"(
                #version 450
                layout(location = 0) out vec4 fragColor;
                void main() {
                    fragColor = vec4(0.0, 1.0, 0.0, 1.0);
                })");
    }

    wgpu::ShaderModule vsModule;
    wgpu::ShaderModule fsModule;
};

// Test cases where creation should succeed
TEST_F(RenderPipelineValidationTest, CreationSuccess) {
    {
        utils::ComboRenderPipelineDescriptor descriptor(device);
        descriptor.vertexStage.module = vsModule;
        descriptor.cFragmentStage.module = fsModule;

        device.CreateRenderPipeline(&descriptor);
    }
    {
        // Vertex input should be optional
        utils::ComboRenderPipelineDescriptor descriptor(device);
        descriptor.vertexStage.module = vsModule;
        descriptor.cFragmentStage.module = fsModule;
        descriptor.vertexState = nullptr;

        device.CreateRenderPipeline(&descriptor);
    }
    {
        // Rasterization state should be optional
        utils::ComboRenderPipelineDescriptor descriptor(device);
        descriptor.vertexStage.module = vsModule;
        descriptor.cFragmentStage.module = fsModule;
        descriptor.rasterizationState = nullptr;
        device.CreateRenderPipeline(&descriptor);
    }
}

// Tests that depth bias parameters must not be NaN.
TEST_F(RenderPipelineValidationTest, DepthBiasParameterNotBeNaN) {
    // Control case, depth bias parameters in ComboRenderPipeline default to 0 which is finite
    {
        utils::ComboRenderPipelineDescriptor descriptor(device);
        descriptor.vertexStage.module = vsModule;
        descriptor.cFragmentStage.module = fsModule;
        device.CreateRenderPipeline(&descriptor);
    }

    // Infinite depth bias clamp is valid
    {
        utils::ComboRenderPipelineDescriptor descriptor(device);
        descriptor.vertexStage.module = vsModule;
        descriptor.cFragmentStage.module = fsModule;
        descriptor.cRasterizationState.depthBiasClamp = INFINITY;
        device.CreateRenderPipeline(&descriptor);
    }
    // NAN depth bias clamp is invalid
    {
        utils::ComboRenderPipelineDescriptor descriptor(device);
        descriptor.vertexStage.module = vsModule;
        descriptor.cFragmentStage.module = fsModule;
        descriptor.cRasterizationState.depthBiasClamp = NAN;
        ASSERT_DEVICE_ERROR(device.CreateRenderPipeline(&descriptor));
    }

    // Infinite depth bias slope is valid
    {
        utils::ComboRenderPipelineDescriptor descriptor(device);
        descriptor.vertexStage.module = vsModule;
        descriptor.cFragmentStage.module = fsModule;
        descriptor.cRasterizationState.depthBiasSlopeScale = INFINITY;
        device.CreateRenderPipeline(&descriptor);
    }
    // NAN depth bias slope is invalid
    {
        utils::ComboRenderPipelineDescriptor descriptor(device);
        descriptor.vertexStage.module = vsModule;
        descriptor.cFragmentStage.module = fsModule;
        descriptor.cRasterizationState.depthBiasSlopeScale = NAN;
        ASSERT_DEVICE_ERROR(device.CreateRenderPipeline(&descriptor));
    }
}

// Tests that at least one color state is required.
TEST_F(RenderPipelineValidationTest, ColorStateRequired) {
    {
        // This one succeeds because attachment 0 is the color attachment
        utils::ComboRenderPipelineDescriptor descriptor(device);
        descriptor.vertexStage.module = vsModule;
        descriptor.cFragmentStage.module = fsModule;
        descriptor.colorStateCount = 1;

        device.CreateRenderPipeline(&descriptor);
    }

    {  // Fail because lack of color states (and depth/stencil state)
        utils::ComboRenderPipelineDescriptor descriptor(device);
        descriptor.vertexStage.module = vsModule;
        descriptor.cFragmentStage.module = fsModule;
        descriptor.colorStateCount = 0;

        ASSERT_DEVICE_ERROR(device.CreateRenderPipeline(&descriptor));
    }
}

// Tests that the color formats must be renderable.
TEST_F(RenderPipelineValidationTest, NonRenderableFormat) {
    {
        // Succeeds because RGBA8Unorm is renderable
        utils::ComboRenderPipelineDescriptor descriptor(device);
        descriptor.vertexStage.module = vsModule;
        descriptor.cFragmentStage.module = fsModule;
        descriptor.cColorStates[0].format = wgpu::TextureFormat::RGBA8Unorm;

        device.CreateRenderPipeline(&descriptor);
    }

    {
        // Fails because RG11B10Float is non-renderable
        utils::ComboRenderPipelineDescriptor descriptor(device);
        descriptor.vertexStage.module = vsModule;
        descriptor.cFragmentStage.module = fsModule;
        descriptor.cColorStates[0].format = wgpu::TextureFormat::RG11B10Float;

        ASSERT_DEVICE_ERROR(device.CreateRenderPipeline(&descriptor));
    }
}

// Tests that the format of the color state descriptor must match the output of the fragment shader.
TEST_F(RenderPipelineValidationTest, FragmentOutputFormatCompatibility) {
    constexpr uint32_t kNumTextureFormatBaseType = 3u;
    std::array<const char*, kNumTextureFormatBaseType> kVecPreFix = {{"", "i", "u"}};
    std::array<wgpu::TextureFormat, kNumTextureFormatBaseType> kColorFormats = {
        {wgpu::TextureFormat::RGBA8Unorm, wgpu::TextureFormat::RGBA8Sint,
         wgpu::TextureFormat::RGBA8Uint}};

    for (size_t i = 0; i < kNumTextureFormatBaseType; ++i) {
        for (size_t j = 0; j < kNumTextureFormatBaseType; ++j) {
            utils::ComboRenderPipelineDescriptor descriptor(device);
            descriptor.vertexStage.module = vsModule;
            descriptor.cColorStates[0].format = kColorFormats[j];

            std::ostringstream stream;
            stream << R"(
                #version 450
                layout(location = 0) out )"
                   << kVecPreFix[i] << R"(vec4 fragColor;
                void main() {
                })";
            descriptor.cFragmentStage.module = utils::CreateShaderModule(
                device, utils::SingleShaderStage::Fragment, stream.str().c_str());

            if (i == j) {
                device.CreateRenderPipeline(&descriptor);
            } else {
                ASSERT_DEVICE_ERROR(device.CreateRenderPipeline(&descriptor));
            }
        }
    }
}

/// Tests that the sample count of the render pipeline must be valid.
TEST_F(RenderPipelineValidationTest, SampleCount) {
    {
        utils::ComboRenderPipelineDescriptor descriptor(device);
        descriptor.vertexStage.module = vsModule;
        descriptor.cFragmentStage.module = fsModule;
        descriptor.sampleCount = 4;

        device.CreateRenderPipeline(&descriptor);
    }

    {
        utils::ComboRenderPipelineDescriptor descriptor(device);
        descriptor.vertexStage.module = vsModule;
        descriptor.cFragmentStage.module = fsModule;
        descriptor.sampleCount = 3;

        ASSERT_DEVICE_ERROR(device.CreateRenderPipeline(&descriptor));
    }
}

// Tests that the sample count of the render pipeline must be equal to the one of every attachments
// in the render pass.
TEST_F(RenderPipelineValidationTest, SampleCountCompatibilityWithRenderPass) {
    constexpr uint32_t kMultisampledCount = 4;
    constexpr wgpu::TextureFormat kColorFormat = wgpu::TextureFormat::RGBA8Unorm;
    constexpr wgpu::TextureFormat kDepthStencilFormat = wgpu::TextureFormat::Depth24PlusStencil8;

    wgpu::TextureDescriptor baseTextureDescriptor;
    baseTextureDescriptor.size.width = 4;
    baseTextureDescriptor.size.height = 4;
    baseTextureDescriptor.size.depth = 1;
    baseTextureDescriptor.mipLevelCount = 1;
    baseTextureDescriptor.dimension = wgpu::TextureDimension::e2D;
    baseTextureDescriptor.usage = wgpu::TextureUsage::OutputAttachment;

    utils::ComboRenderPipelineDescriptor nonMultisampledPipelineDescriptor(device);
    nonMultisampledPipelineDescriptor.sampleCount = 1;
    nonMultisampledPipelineDescriptor.vertexStage.module = vsModule;
    nonMultisampledPipelineDescriptor.cFragmentStage.module = fsModule;
    wgpu::RenderPipeline nonMultisampledPipeline =
        device.CreateRenderPipeline(&nonMultisampledPipelineDescriptor);

    nonMultisampledPipelineDescriptor.colorStateCount = 0;
    nonMultisampledPipelineDescriptor.depthStencilState =
        &nonMultisampledPipelineDescriptor.cDepthStencilState;
    wgpu::RenderPipeline nonMultisampledPipelineWithDepthStencilOnly =
        device.CreateRenderPipeline(&nonMultisampledPipelineDescriptor);

    utils::ComboRenderPipelineDescriptor multisampledPipelineDescriptor(device);
    multisampledPipelineDescriptor.sampleCount = kMultisampledCount;
    multisampledPipelineDescriptor.vertexStage.module = vsModule;
    multisampledPipelineDescriptor.cFragmentStage.module = fsModule;
    wgpu::RenderPipeline multisampledPipeline =
        device.CreateRenderPipeline(&multisampledPipelineDescriptor);

    multisampledPipelineDescriptor.colorStateCount = 0;
    multisampledPipelineDescriptor.depthStencilState =
        &multisampledPipelineDescriptor.cDepthStencilState;
    wgpu::RenderPipeline multisampledPipelineWithDepthStencilOnly =
        device.CreateRenderPipeline(&multisampledPipelineDescriptor);

    // It is not allowed to use multisampled render pass and non-multisampled render pipeline.
    {
        {
            wgpu::TextureDescriptor textureDescriptor = baseTextureDescriptor;
            textureDescriptor.format = kColorFormat;
            textureDescriptor.sampleCount = kMultisampledCount;
            wgpu::Texture multisampledColorTexture = device.CreateTexture(&textureDescriptor);
            utils::ComboRenderPassDescriptor renderPassDescriptor(
                {multisampledColorTexture.CreateView()});

            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            wgpu::RenderPassEncoder renderPass = encoder.BeginRenderPass(&renderPassDescriptor);
            renderPass.SetPipeline(nonMultisampledPipeline);
            renderPass.EndPass();

            ASSERT_DEVICE_ERROR(encoder.Finish());
        }

        {
            wgpu::TextureDescriptor textureDescriptor = baseTextureDescriptor;
            textureDescriptor.sampleCount = kMultisampledCount;
            textureDescriptor.format = kDepthStencilFormat;
            wgpu::Texture multisampledDepthStencilTexture =
                device.CreateTexture(&textureDescriptor);
            utils::ComboRenderPassDescriptor renderPassDescriptor(
                {}, multisampledDepthStencilTexture.CreateView());

            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            wgpu::RenderPassEncoder renderPass = encoder.BeginRenderPass(&renderPassDescriptor);
            renderPass.SetPipeline(nonMultisampledPipelineWithDepthStencilOnly);
            renderPass.EndPass();

            ASSERT_DEVICE_ERROR(encoder.Finish());
        }
    }

    // It is allowed to use multisampled render pass and multisampled render pipeline.
    {
        {
            wgpu::TextureDescriptor textureDescriptor = baseTextureDescriptor;
            textureDescriptor.format = kColorFormat;
            textureDescriptor.sampleCount = kMultisampledCount;
            wgpu::Texture multisampledColorTexture = device.CreateTexture(&textureDescriptor);
            utils::ComboRenderPassDescriptor renderPassDescriptor(
                {multisampledColorTexture.CreateView()});

            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            wgpu::RenderPassEncoder renderPass = encoder.BeginRenderPass(&renderPassDescriptor);
            renderPass.SetPipeline(multisampledPipeline);
            renderPass.EndPass();

            encoder.Finish();
        }

        {
            wgpu::TextureDescriptor textureDescriptor = baseTextureDescriptor;
            textureDescriptor.sampleCount = kMultisampledCount;
            textureDescriptor.format = kDepthStencilFormat;
            wgpu::Texture multisampledDepthStencilTexture =
                device.CreateTexture(&textureDescriptor);
            utils::ComboRenderPassDescriptor renderPassDescriptor(
                {}, multisampledDepthStencilTexture.CreateView());

            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            wgpu::RenderPassEncoder renderPass = encoder.BeginRenderPass(&renderPassDescriptor);
            renderPass.SetPipeline(multisampledPipelineWithDepthStencilOnly);
            renderPass.EndPass();

            encoder.Finish();
        }
    }

    // It is not allowed to use non-multisampled render pass and multisampled render pipeline.
    {
        {
            wgpu::TextureDescriptor textureDescriptor = baseTextureDescriptor;
            textureDescriptor.format = kColorFormat;
            textureDescriptor.sampleCount = 1;
            wgpu::Texture nonMultisampledColorTexture = device.CreateTexture(&textureDescriptor);
            utils::ComboRenderPassDescriptor nonMultisampledRenderPassDescriptor(
                {nonMultisampledColorTexture.CreateView()});

            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            wgpu::RenderPassEncoder renderPass =
                encoder.BeginRenderPass(&nonMultisampledRenderPassDescriptor);
            renderPass.SetPipeline(multisampledPipeline);
            renderPass.EndPass();

            ASSERT_DEVICE_ERROR(encoder.Finish());
        }

        {
            wgpu::TextureDescriptor textureDescriptor = baseTextureDescriptor;
            textureDescriptor.sampleCount = 1;
            textureDescriptor.format = kDepthStencilFormat;
            wgpu::Texture multisampledDepthStencilTexture =
                device.CreateTexture(&textureDescriptor);
            utils::ComboRenderPassDescriptor renderPassDescriptor(
                {}, multisampledDepthStencilTexture.CreateView());

            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            wgpu::RenderPassEncoder renderPass = encoder.BeginRenderPass(&renderPassDescriptor);
            renderPass.SetPipeline(multisampledPipelineWithDepthStencilOnly);
            renderPass.EndPass();

            ASSERT_DEVICE_ERROR(encoder.Finish());
        }
    }
}

// Tests that the texture component type in shader must match the bind group layout.
TEST_F(RenderPipelineValidationTest, TextureComponentTypeCompatibility) {
    constexpr uint32_t kNumTextureComponentType = 3u;
    std::array<const char*, kNumTextureComponentType> kTexturePrefix = {{"", "i", "u"}};
    std::array<wgpu::TextureComponentType, kNumTextureComponentType> kTextureComponentTypes = {{
        wgpu::TextureComponentType::Float,
        wgpu::TextureComponentType::Sint,
        wgpu::TextureComponentType::Uint,
    }};

    for (size_t i = 0; i < kNumTextureComponentType; ++i) {
        for (size_t j = 0; j < kNumTextureComponentType; ++j) {
            utils::ComboRenderPipelineDescriptor descriptor(device);
            descriptor.vertexStage.module = vsModule;

            std::ostringstream stream;
            stream << R"(
                #version 450
                layout(set = 0, binding = 0) uniform )"
                   << kTexturePrefix[i] << R"(texture2D tex;
                void main() {
                })";
            descriptor.cFragmentStage.module = utils::CreateShaderModule(
                device, utils::SingleShaderStage::Fragment, stream.str().c_str());

            wgpu::BindGroupLayout bgl = utils::MakeBindGroupLayout(
                device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::SampledTexture, false,
                          0, false, wgpu::TextureViewDimension::e2D, kTextureComponentTypes[j]}});
            descriptor.layout = utils::MakeBasicPipelineLayout(device, &bgl);

            if (i == j) {
                device.CreateRenderPipeline(&descriptor);
            } else {
                ASSERT_DEVICE_ERROR(device.CreateRenderPipeline(&descriptor));
            }
        }
    }
}

// Tests that the texture view dimension in shader must match the bind group layout.
TEST_F(RenderPipelineValidationTest, TextureViewDimensionCompatibility) {
    constexpr uint32_t kNumTextureViewDimensions = 6u;
    std::array<const char*, kNumTextureViewDimensions> kTextureKeywords = {{
        "texture1D",
        "texture2D",
        "texture2DArray",
        "textureCube",
        "textureCubeArray",
        "texture3D",
    }};

    std::array<wgpu::TextureViewDimension, kNumTextureViewDimensions> kTextureViewDimensions = {{
        wgpu::TextureViewDimension::e1D,
        wgpu::TextureViewDimension::e2D,
        wgpu::TextureViewDimension::e2DArray,
        wgpu::TextureViewDimension::Cube,
        wgpu::TextureViewDimension::CubeArray,
        wgpu::TextureViewDimension::e3D,
    }};

    for (size_t i = 0; i < kNumTextureViewDimensions; ++i) {
        for (size_t j = 0; j < kNumTextureViewDimensions; ++j) {
            utils::ComboRenderPipelineDescriptor descriptor(device);
            descriptor.vertexStage.module = vsModule;

            std::ostringstream stream;
            stream << R"(
                #version 450
                layout(set = 0, binding = 0) uniform )"
                   << kTextureKeywords[i] << R"( tex;
                void main() {
                })";
            descriptor.cFragmentStage.module = utils::CreateShaderModule(
                device, utils::SingleShaderStage::Fragment, stream.str().c_str());

            wgpu::BindGroupLayout bgl = utils::MakeBindGroupLayout(
                device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::SampledTexture, false,
                          0, false, kTextureViewDimensions[j], wgpu::TextureComponentType::Float}});
            descriptor.layout = utils::MakeBasicPipelineLayout(device, &bgl);

            if (i == j) {
                device.CreateRenderPipeline(&descriptor);
            } else {
                ASSERT_DEVICE_ERROR(device.CreateRenderPipeline(&descriptor));
            }
        }
    }
}

// Test that declaring a storage buffer in the vertex shader without setting pipeline layout won't
// cause crash.
TEST_F(RenderPipelineValidationTest, StorageBufferInVertexShaderNoLayout) {
    wgpu::ShaderModule vsModuleWithStorageBuffer =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
        #version 450
        #define kNumValues 100
        layout(std430, set = 0, binding = 0) buffer Dst { uint dst[kNumValues]; };
        void main() {
            uint index = gl_VertexIndex;
            dst[index] = 0x1234;
            gl_Position = vec4(1.f, 0.f, 0.f, 1.f);
        })");

    utils::ComboRenderPipelineDescriptor descriptor(device);
    descriptor.layout = nullptr;
    descriptor.vertexStage.module = vsModuleWithStorageBuffer;
    descriptor.cFragmentStage.module = fsModule;
    ASSERT_DEVICE_ERROR(device.CreateRenderPipeline(&descriptor));
}

// Test that a pipeline with defaulted layout may not have multisampled array textures
// TODO(enga): Also test multisampled cube, cube array, and 3D. These have no GLSL keywords.
TEST_F(RenderPipelineValidationTest, MultisampledTexture) {
    utils::ComboRenderPipelineDescriptor descriptor(device);
    descriptor.layout = nullptr;
    descriptor.cFragmentStage.module = fsModule;

    // Base case works.
    descriptor.vertexStage.module =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
        #version 450
        layout(set = 0, binding = 0) uniform texture2DMS texture;
        void main() {
        })");
    device.CreateRenderPipeline(&descriptor);

    // texture2DMSArray invalid
    descriptor.vertexStage.module =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
        #version 450
        layout(set = 0, binding = 0) uniform texture2DMSArray texture;
        void main() {
        })");
    ASSERT_DEVICE_ERROR(device.CreateRenderPipeline(&descriptor));
}
