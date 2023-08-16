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

#include "common/Assert.h"
#include "tests/unittests/validation/ValidationTest.h"
#include "utils/ComboRenderPipelineDescriptor.h"
#include "utils/TextureFormatUtils.h"
#include "utils/WGPUHelpers.h"

class StorageTextureValidationTests : public ValidationTest {
  protected:
    static const char* GetGLSLFloatImageTypeDeclaration(wgpu::TextureViewDimension dimension) {
        switch (dimension) {
            case wgpu::TextureViewDimension::e1D:
                return "image1D";
            case wgpu::TextureViewDimension::e2D:
                return "image2D";
            case wgpu::TextureViewDimension::e2DArray:
                return "image2DArray";
            case wgpu::TextureViewDimension::Cube:
                return "imageCube";
            case wgpu::TextureViewDimension::CubeArray:
                return "imageCubeArray";
            case wgpu::TextureViewDimension::e3D:
                return "image3D";
            case wgpu::TextureViewDimension::Undefined:
            default:
                UNREACHABLE();
                return "";
        }
    }

    static std::string CreateComputeShaderWithStorageTexture(
        wgpu::BindingType storageTextureBindingType,
        wgpu::TextureFormat textureFormat,
        wgpu::TextureViewDimension textureViewDimension = wgpu::TextureViewDimension::e2D) {
        const char* glslImageFormatQualifier = utils::GetGLSLImageFormatQualifier(textureFormat);
        const char* textureComponentTypePrefix =
            utils::GetColorTextureComponentTypePrefix(textureFormat);
        return CreateComputeShaderWithStorageTexture(
            storageTextureBindingType, glslImageFormatQualifier, textureComponentTypePrefix,
            GetGLSLFloatImageTypeDeclaration(textureViewDimension));
    }

    static std::string CreateComputeShaderWithStorageTexture(
        wgpu::BindingType storageTextureBindingType,
        const char* glslImageFormatQualifier,
        const char* textureComponentTypePrefix,
        const char* glslImageTypeDeclaration = "image2D") {
        const char* memoryQualifier = "";
        switch (storageTextureBindingType) {
            case wgpu::BindingType::ReadonlyStorageTexture:
                memoryQualifier = "readonly";
                break;
            case wgpu::BindingType::WriteonlyStorageTexture:
                memoryQualifier = "writeonly";
                break;
            default:
                UNREACHABLE();
                break;
        }

        std::ostringstream ostream;
        ostream << "#version 450\n"
                   "layout (set = 0, binding = 0, "
                << glslImageFormatQualifier << ") uniform " << memoryQualifier << " "
                << textureComponentTypePrefix << glslImageTypeDeclaration
                << " image0;\n"
                   "void main() {\n"
                   "}\n";

        return ostream.str();
    }

    wgpu::Texture CreateTexture(wgpu::TextureUsage usage,
                                wgpu::TextureFormat format,
                                uint32_t sampleCount = 1,
                                uint32_t arrayLayerCount = 1) {
        wgpu::TextureDescriptor descriptor;
        descriptor.dimension = wgpu::TextureDimension::e2D;
        descriptor.size = {16, 16, arrayLayerCount};
        descriptor.sampleCount = sampleCount;
        descriptor.format = format;
        descriptor.mipLevelCount = 1;
        descriptor.usage = usage;
        return device.CreateTexture(&descriptor);
    }

    const wgpu::ShaderModule mDefaultVSModule =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.f, 0.f, 0.f, 1.f);
        })");
    const wgpu::ShaderModule mDefaultFSModule =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, R"(
        #version 450
        layout(location = 0) out vec4 fragColor;
        void main() {
            fragColor = vec4(1.f, 0.f, 0.f, 1.f);
        })");

    const std::array<wgpu::BindingType, 2> kSupportedStorageTextureBindingTypes = {
        wgpu::BindingType::ReadonlyStorageTexture, wgpu::BindingType::WriteonlyStorageTexture};
};

// Validate read-only storage textures can be declared in vertex and fragment shaders, while
// writeonly storage textures cannot be used in vertex shaders.
TEST_F(StorageTextureValidationTests, RenderPipeline) {
    // Readonly storage texture can be declared in a vertex shader.
    {
        wgpu::ShaderModule vsModule =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
            #version 450
            layout(set = 0, binding = 0, rgba8) uniform readonly image2D image0;
            void main() {
                gl_Position = imageLoad(image0, ivec2(gl_VertexIndex, 0));
            })");

        utils::ComboRenderPipelineDescriptor descriptor(device);
        descriptor.layout = nullptr;
        descriptor.vertexStage.module = vsModule;
        descriptor.cFragmentStage.module = mDefaultFSModule;
        device.CreateRenderPipeline(&descriptor);
    }

    // Read-only storage textures can be declared in a fragment shader.
    {
        wgpu::ShaderModule fsModule =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, R"(
            #version 450
            layout(set = 0, binding = 0, rgba8) uniform readonly image2D image0;
            layout(location = 0) out vec4 fragColor;
            void main() {
                fragColor = imageLoad(image0, ivec2(gl_FragCoord.xy));
            })");

        utils::ComboRenderPipelineDescriptor descriptor(device);
        descriptor.layout = nullptr;
        descriptor.vertexStage.module = mDefaultVSModule;
        descriptor.cFragmentStage.module = fsModule;
        device.CreateRenderPipeline(&descriptor);
    }

    // Write-only storage textures cannot be declared in a vertex shader.
    {
        wgpu::ShaderModule vsModule =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
            #version 450
            layout(set = 0, binding = 0, rgba8) uniform writeonly image2D image0;
            void main() {
                imageStore(image0, ivec2(gl_VertexIndex, 0), vec4(1.f, 0.f, 0.f, 1.f));
            })");

        utils::ComboRenderPipelineDescriptor descriptor(device);
        descriptor.layout = nullptr;
        descriptor.vertexStage.module = vsModule;
        descriptor.cFragmentStage.module = mDefaultFSModule;
        ASSERT_DEVICE_ERROR(device.CreateRenderPipeline(&descriptor));
    }

    // Write-only storage textures can be declared in a fragment shader.
    {
        wgpu::ShaderModule fsModule =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, R"(
            #version 450
            layout(set = 0, binding = 0, rgba8) uniform writeonly image2D image0;
            void main() {
                imageStore(image0, ivec2(gl_FragCoord.xy), vec4(1.f, 0.f, 0.f, 1.f));
            })");

        utils::ComboRenderPipelineDescriptor descriptor(device);
        descriptor.layout = nullptr;
        descriptor.vertexStage.module = mDefaultVSModule;
        descriptor.cFragmentStage.module = fsModule;
        device.CreateRenderPipeline(&descriptor);
    }
}

// Validate both read-only and write-only storage textures can be declared in
// compute shaders.
TEST_F(StorageTextureValidationTests, ComputePipeline) {
    // Read-only storage textures can be declared in a compute shader.
    {
        wgpu::ShaderModule csModule =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Compute, R"(
            #version 450
            layout(set = 0, binding = 0, rgba8) uniform readonly image2D image0;
            layout(std430, set = 0, binding = 1) buffer Buf { uint buf; };
            void main() {
                vec4 pixel = imageLoad(image0, ivec2(gl_LocalInvocationID.xy));
                buf = uint(pixel.x);
            })");

        wgpu::ComputePipelineDescriptor descriptor;
        descriptor.layout = nullptr;
        descriptor.computeStage.module = csModule;
        descriptor.computeStage.entryPoint = "main";

        device.CreateComputePipeline(&descriptor);
    }

    // Write-only storage textures can be declared in a compute shader.
    {
        wgpu::ShaderModule csModule =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Compute, R"(
            #version 450
            layout(set = 0, binding = 0, rgba8) uniform writeonly image2D image0;
            void main() {
                imageStore(image0, ivec2(gl_LocalInvocationID.xy), vec4(0.f, 0.f, 0.f, 0.f));
            })");

        wgpu::ComputePipelineDescriptor descriptor;
        descriptor.layout = nullptr;
        descriptor.computeStage.module = csModule;
        descriptor.computeStage.entryPoint = "main";

        device.CreateComputePipeline(&descriptor);
    }
}

// Validate read-write storage textures have not been supported yet.
TEST_F(StorageTextureValidationTests, ReadWriteStorageTexture) {
    // Read-write storage textures cannot be declared in a vertex shader by default.
    {
        wgpu::ShaderModule vsModule =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
            #version 450
            layout(set = 0, binding = 0, rgba8) uniform image2D image0;
            void main() {
                vec4 pixel = imageLoad(image0, ivec2(gl_VertexIndex, 0));
                imageStore(image0, ivec2(gl_VertexIndex, 0), pixel * 2);
            })");

        utils::ComboRenderPipelineDescriptor descriptor(device);
        descriptor.layout = nullptr;
        descriptor.vertexStage.module = vsModule;
        descriptor.cFragmentStage.module = mDefaultFSModule;
        ASSERT_DEVICE_ERROR(device.CreateRenderPipeline(&descriptor));
    }

    // Read-write storage textures cannot be declared in a fragment shader by default.
    {
        wgpu::ShaderModule fsModule =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, R"(
            #version 450
            layout(set = 0, binding = 0, rgba8) uniform image2D image0;
            void main() {
                vec4 pixel = imageLoad(image0, ivec2(gl_FragCoord.xy));
                imageStore(image0, ivec2(gl_FragCoord.xy), pixel * 2);
            })");

        utils::ComboRenderPipelineDescriptor descriptor(device);
        descriptor.layout = nullptr;
        descriptor.vertexStage.module = mDefaultVSModule;
        descriptor.cFragmentStage.module = fsModule;
        ASSERT_DEVICE_ERROR(device.CreateRenderPipeline(&descriptor));
    }

    // Read-write storage textures cannot be declared in a compute shader by default.
    {
        wgpu::ShaderModule csModule =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Compute, R"(
            #version 450
            layout(set = 0, binding = 0, rgba8) uniform image2D image0;
            void main() {
                vec4 pixel = imageLoad(image0, ivec2(gl_LocalInvocationID.xy));
                imageStore(image0, ivec2(gl_LocalInvocationID.xy), pixel * 2);
            })");

        wgpu::ComputePipelineDescriptor descriptor;
        descriptor.layout = nullptr;
        descriptor.computeStage.module = csModule;
        descriptor.computeStage.entryPoint = "main";

        ASSERT_DEVICE_ERROR(device.CreateComputePipeline(&descriptor));
    }
}

// Test that using read-only storage texture and write-only storage texture in
// BindGroupLayout is valid, while using read-write storage texture is not allowed now.
TEST_F(StorageTextureValidationTests, BindGroupLayoutWithStorageTextureBindingType) {
    struct TestSpec {
        wgpu::ShaderStage stage;
        wgpu::BindingType type;
        bool valid;
    };
    constexpr std::array<TestSpec, 9> kTestSpecs = {
        {{wgpu::ShaderStage::Vertex, wgpu::BindingType::ReadonlyStorageTexture, true},
         {wgpu::ShaderStage::Vertex, wgpu::BindingType::WriteonlyStorageTexture, false},
         {wgpu::ShaderStage::Vertex, wgpu::BindingType::StorageTexture, false},
         {wgpu::ShaderStage::Fragment, wgpu::BindingType::ReadonlyStorageTexture, true},
         {wgpu::ShaderStage::Fragment, wgpu::BindingType::WriteonlyStorageTexture, true},
         {wgpu::ShaderStage::Fragment, wgpu::BindingType::StorageTexture, false},
         {wgpu::ShaderStage::Compute, wgpu::BindingType::ReadonlyStorageTexture, true},
         {wgpu::ShaderStage::Compute, wgpu::BindingType::WriteonlyStorageTexture, true},
         {wgpu::ShaderStage::Compute, wgpu::BindingType::StorageTexture, false}}};

    for (const auto& testSpec : kTestSpecs) {
        wgpu::BindGroupLayoutEntry entry = {0, testSpec.stage, testSpec.type};
        entry.storageTextureFormat = wgpu::TextureFormat::R32Uint;

        wgpu::BindGroupLayoutDescriptor descriptor;
        descriptor.entryCount = 1;
        descriptor.entries = &entry;

        if (testSpec.valid) {
            device.CreateBindGroupLayout(&descriptor);
        } else {
            ASSERT_DEVICE_ERROR(device.CreateBindGroupLayout(&descriptor));
        }
    }
}

// Validate it is an error to declare a read-only or write-only storage texture in shaders with any
// format that doesn't support TextureUsage::Storage texture usages.
TEST_F(StorageTextureValidationTests, StorageTextureFormatInShaders) {
    // Not include RGBA8UnormSrgb, BGRA8Unorm, BGRA8UnormSrgb because they are not related to any
    // SPIR-V Image Formats.
    constexpr std::array<wgpu::TextureFormat, 32> kWGPUTextureFormatSupportedAsSPIRVImageFormats = {
        wgpu::TextureFormat::R32Uint,      wgpu::TextureFormat::R32Sint,
        wgpu::TextureFormat::R32Float,     wgpu::TextureFormat::RGBA8Unorm,
        wgpu::TextureFormat::RGBA8Snorm,   wgpu::TextureFormat::RGBA8Uint,
        wgpu::TextureFormat::RGBA8Sint,    wgpu::TextureFormat::RG32Uint,
        wgpu::TextureFormat::RG32Sint,     wgpu::TextureFormat::RG32Float,
        wgpu::TextureFormat::RGBA16Uint,   wgpu::TextureFormat::RGBA16Sint,
        wgpu::TextureFormat::RGBA16Float,  wgpu::TextureFormat::RGBA32Uint,
        wgpu::TextureFormat::RGBA32Sint,   wgpu::TextureFormat::RGBA32Float,
        wgpu::TextureFormat::R8Unorm,      wgpu::TextureFormat::R8Snorm,
        wgpu::TextureFormat::R8Uint,       wgpu::TextureFormat::R8Sint,
        wgpu::TextureFormat::R16Uint,      wgpu::TextureFormat::R16Sint,
        wgpu::TextureFormat::R16Float,     wgpu::TextureFormat::RG8Unorm,
        wgpu::TextureFormat::RG8Snorm,     wgpu::TextureFormat::RG8Uint,
        wgpu::TextureFormat::RG8Sint,      wgpu::TextureFormat::RG16Uint,
        wgpu::TextureFormat::RG16Sint,     wgpu::TextureFormat::RG16Float,
        wgpu::TextureFormat::RGB10A2Unorm, wgpu::TextureFormat::RG11B10Float};

    for (wgpu::BindingType storageTextureBindingType : kSupportedStorageTextureBindingTypes) {
        for (wgpu::TextureFormat format : kWGPUTextureFormatSupportedAsSPIRVImageFormats) {
            std::string computeShader =
                CreateComputeShaderWithStorageTexture(storageTextureBindingType, format);
            if (utils::TextureFormatSupportsStorageTexture(format)) {
                utils::CreateShaderModule(device, utils::SingleShaderStage::Compute,
                                          computeShader.c_str());
            } else {
                ASSERT_DEVICE_ERROR(utils::CreateShaderModule(
                    device, utils::SingleShaderStage::Compute, computeShader.c_str()));
            }
        }
    }
}

// Verify that declaring a storage texture format that is not supported in WebGPU causes validation
// error.
TEST_F(StorageTextureValidationTests, UnsupportedSPIRVStorageTextureFormat) {
    struct TextureFormatInfo {
        const char* name;
        const char* componentTypePrefix;
    };

    constexpr std::array<TextureFormatInfo, 7> kUnsupportedTextureFormats = {{{"rgba16", ""},
                                                                              {"rg16", ""},
                                                                              {"r16", ""},
                                                                              {"rgba16_snorm", ""},
                                                                              {"rg16_snorm", ""},
                                                                              {"r16_snorm", ""},
                                                                              {"rgb10_a2ui", "u"}}};

    for (wgpu::BindingType bindingType : kSupportedStorageTextureBindingTypes) {
        for (const TextureFormatInfo& formatInfo : kUnsupportedTextureFormats) {
            std::string computeShader = CreateComputeShaderWithStorageTexture(
                bindingType, formatInfo.name, formatInfo.componentTypePrefix);
            ASSERT_DEVICE_ERROR(utils::CreateShaderModule(device, utils::SingleShaderStage::Compute,
                                                          computeShader.c_str()));
        }
    }
}

// Verify that declaring a storage texture dimension that is not supported in WebGPU in shader
// causes validation error at the creation of PSO. WebGPU doesn't support using cube map texture
// views and cube map array texture views as storage textures.
TEST_F(StorageTextureValidationTests, UnsupportedTextureViewDimensionInShader) {
    constexpr std::array<wgpu::TextureViewDimension, 2> kUnsupportedTextureViewDimensions = {
        wgpu::TextureViewDimension::Cube, wgpu::TextureViewDimension::CubeArray};
    constexpr wgpu::TextureFormat kFormat = wgpu::TextureFormat::R32Float;

    for (wgpu::BindingType bindingType : kSupportedStorageTextureBindingTypes) {
        for (wgpu::TextureViewDimension dimension : kUnsupportedTextureViewDimensions) {
            std::string computeShader =
                CreateComputeShaderWithStorageTexture(bindingType, kFormat, dimension);
            wgpu::ShaderModule csModule = utils::CreateShaderModule(
                device, utils::SingleShaderStage::Compute, computeShader.c_str());

            wgpu::ComputePipelineDescriptor computePipelineDescriptor;
            computePipelineDescriptor.computeStage.module = csModule;
            computePipelineDescriptor.computeStage.entryPoint = "main";
            computePipelineDescriptor.layout = nullptr;
            ASSERT_DEVICE_ERROR(device.CreateComputePipeline(&computePipelineDescriptor));
        }
    }
}

// Verify that declaring a texture view dimension that is not supported to be used as storage
// textures in WebGPU in bind group layout causes validation error. WebGPU doesn't support using
// cube map texture views and cube map array texture views as storage textures.
TEST_F(StorageTextureValidationTests, UnsupportedTextureViewDimensionInBindGroupLayout) {
    constexpr std::array<wgpu::TextureViewDimension, 2> kUnsupportedTextureViewDimensions = {
        wgpu::TextureViewDimension::Cube, wgpu::TextureViewDimension::CubeArray};
    constexpr wgpu::TextureFormat kFormat = wgpu::TextureFormat::R32Float;

    for (wgpu::BindingType bindingType : kSupportedStorageTextureBindingTypes) {
        for (wgpu::TextureViewDimension dimension : kUnsupportedTextureViewDimensions) {
            ASSERT_DEVICE_ERROR(utils::MakeBindGroupLayout(
                device, {{0, wgpu::ShaderStage::Compute, bindingType, false, 0, false, dimension,
                          wgpu::TextureComponentType::Float, kFormat}}));
        }
    }
}

// Verify when we create and use a bind group layout with storage textures in the creation of
// render and compute pipeline, the binding type in the bind group layout must match the
// declaration in the shader.
TEST_F(StorageTextureValidationTests, BindGroupLayoutEntryTypeMatchesShaderDeclaration) {
    constexpr std::array<wgpu::BindingType, 7> kSupportedBindingTypes = {
        wgpu::BindingType::UniformBuffer,          wgpu::BindingType::StorageBuffer,
        wgpu::BindingType::ReadonlyStorageBuffer,  wgpu::BindingType::Sampler,
        wgpu::BindingType::SampledTexture,         wgpu::BindingType::ReadonlyStorageTexture,
        wgpu::BindingType::WriteonlyStorageTexture};
    constexpr wgpu::TextureFormat kStorageTextureFormat = wgpu::TextureFormat::R32Float;

    for (wgpu::BindingType bindingTypeInShader : kSupportedStorageTextureBindingTypes) {
        // Create the compute shader with the given binding type.
        std::string computeShader =
            CreateComputeShaderWithStorageTexture(bindingTypeInShader, kStorageTextureFormat);
        wgpu::ShaderModule csModule = utils::CreateShaderModule(
            device, utils::SingleShaderStage::Compute, computeShader.c_str());

        // Set common fields of compute pipeline descriptor.
        wgpu::ComputePipelineDescriptor defaultComputePipelineDescriptor;
        defaultComputePipelineDescriptor.computeStage.module = csModule;
        defaultComputePipelineDescriptor.computeStage.entryPoint = "main";

        // Set common fileds of bind group layout binding.
        wgpu::BindGroupLayoutEntry defaultBindGroupLayoutEntry;
        defaultBindGroupLayoutEntry.binding = 0;
        defaultBindGroupLayoutEntry.visibility = wgpu::ShaderStage::Compute;
        defaultBindGroupLayoutEntry.storageTextureFormat = kStorageTextureFormat;

        for (wgpu::BindingType bindingTypeInBindgroupLayout : kSupportedBindingTypes) {
            wgpu::ComputePipelineDescriptor computePipelineDescriptor =
                defaultComputePipelineDescriptor;

            // Create bind group layout with different binding types.
            wgpu::BindGroupLayoutEntry bindGroupLayoutBinding = defaultBindGroupLayoutEntry;
            bindGroupLayoutBinding.type = bindingTypeInBindgroupLayout;
            wgpu::BindGroupLayout bindGroupLayout =
                utils::MakeBindGroupLayout(device, {bindGroupLayoutBinding});
            computePipelineDescriptor.layout =
                utils::MakeBasicPipelineLayout(device, &bindGroupLayout);

            // The binding type in the bind group layout must the same as the related image object
            // declared in shader.
            if (bindingTypeInBindgroupLayout == bindingTypeInShader) {
                device.CreateComputePipeline(&computePipelineDescriptor);
            } else {
                ASSERT_DEVICE_ERROR(device.CreateComputePipeline(&computePipelineDescriptor));
            }
        }
    }
}

// Verify it is invalid not to set a valid texture format in a bind group layout when the binding
// type is read-only or write-only storage texture.
TEST_F(StorageTextureValidationTests, UndefinedStorageTextureFormatInBindGroupLayout) {
    wgpu::BindGroupLayoutEntry errorBindGroupLayoutEntry;
    errorBindGroupLayoutEntry.binding = 0;
    errorBindGroupLayoutEntry.visibility = wgpu::ShaderStage::Compute;
    errorBindGroupLayoutEntry.storageTextureFormat = wgpu::TextureFormat::Undefined;

    for (wgpu::BindingType bindingType : kSupportedStorageTextureBindingTypes) {
        errorBindGroupLayoutEntry.type = bindingType;
        ASSERT_DEVICE_ERROR(utils::MakeBindGroupLayout(device, {errorBindGroupLayoutEntry}));
    }
}

// Verify it is invalid to create a bind group layout with storage textures and an unsupported
// storage texture format.
TEST_F(StorageTextureValidationTests, StorageTextureFormatInBindGroupLayout) {
    wgpu::BindGroupLayoutEntry defaultBindGroupLayoutEntry;
    defaultBindGroupLayoutEntry.binding = 0;
    defaultBindGroupLayoutEntry.visibility = wgpu::ShaderStage::Compute;

    for (wgpu::BindingType bindingType : kSupportedStorageTextureBindingTypes) {
        for (wgpu::TextureFormat textureFormat : utils::kAllTextureFormats) {
            wgpu::BindGroupLayoutEntry bindGroupLayoutBinding = defaultBindGroupLayoutEntry;
            bindGroupLayoutBinding.type = bindingType;
            bindGroupLayoutBinding.storageTextureFormat = textureFormat;
            if (utils::TextureFormatSupportsStorageTexture(textureFormat)) {
                utils::MakeBindGroupLayout(device, {bindGroupLayoutBinding});
            } else {
                ASSERT_DEVICE_ERROR(utils::MakeBindGroupLayout(device, {bindGroupLayoutBinding}));
            }
        }
    }
}

// Verify the storage texture format in the bind group layout must match the declaration in shader.
TEST_F(StorageTextureValidationTests, BindGroupLayoutStorageTextureFormatMatchesShaderDeclaration) {
    for (wgpu::BindingType bindingType : kSupportedStorageTextureBindingTypes) {
        for (wgpu::TextureFormat storageTextureFormatInShader : utils::kAllTextureFormats) {
            if (!utils::TextureFormatSupportsStorageTexture(storageTextureFormatInShader)) {
                continue;
            }

            // Create the compute shader module with the given binding type and storage texture
            // format.
            std::string computeShader =
                CreateComputeShaderWithStorageTexture(bindingType, storageTextureFormatInShader);
            wgpu::ShaderModule csModule = utils::CreateShaderModule(
                device, utils::SingleShaderStage::Compute, computeShader.c_str());

            // Set common fields of compute pipeline descriptor.
            wgpu::ComputePipelineDescriptor defaultComputePipelineDescriptor;
            defaultComputePipelineDescriptor.computeStage.module = csModule;
            defaultComputePipelineDescriptor.computeStage.entryPoint = "main";

            // Set common fileds of bind group layout binding.
            wgpu::BindGroupLayoutEntry defaultBindGroupLayoutEntry = {0, wgpu::ShaderStage::Compute,
                                                                      bindingType};

            for (wgpu::TextureFormat storageTextureFormatInBindGroupLayout :
                 utils::kAllTextureFormats) {
                if (!utils::TextureFormatSupportsStorageTexture(
                        storageTextureFormatInBindGroupLayout)) {
                    continue;
                }

                // Create the bind group layout with the given storage texture format.
                wgpu::BindGroupLayoutEntry bindGroupLayoutBinding = defaultBindGroupLayoutEntry;
                bindGroupLayoutBinding.storageTextureFormat = storageTextureFormatInBindGroupLayout;
                wgpu::BindGroupLayout bindGroupLayout =
                    utils::MakeBindGroupLayout(device, {bindGroupLayoutBinding});

                // Create the compute pipeline with the bind group layout.
                wgpu::ComputePipelineDescriptor computePipelineDescriptor =
                    defaultComputePipelineDescriptor;
                computePipelineDescriptor.layout =
                    utils::MakeBasicPipelineLayout(device, &bindGroupLayout);

                // The storage texture format in the bind group layout must be the same as the one
                // declared in the shader.
                if (storageTextureFormatInShader == storageTextureFormatInBindGroupLayout) {
                    device.CreateComputePipeline(&computePipelineDescriptor);
                } else {
                    ASSERT_DEVICE_ERROR(device.CreateComputePipeline(&computePipelineDescriptor));
                }
            }
        }
    }
}

// Verify the dimension of the bind group layout with storage textures must match the one declared
// in shader.
TEST_F(StorageTextureValidationTests, BindGroupLayoutViewDimensionMatchesShaderDeclaration) {
    constexpr std::array<wgpu::TextureViewDimension, 4> kSupportedDimensions = {
        wgpu::TextureViewDimension::e1D, wgpu::TextureViewDimension::e2D,
        wgpu::TextureViewDimension::e2DArray, wgpu::TextureViewDimension::e3D};
    constexpr wgpu::TextureFormat kStorageTextureFormat = wgpu::TextureFormat::R32Float;

    for (wgpu::BindingType bindingType : kSupportedStorageTextureBindingTypes) {
        for (wgpu::TextureViewDimension dimensionInShader : kSupportedDimensions) {
            // Create the compute shader with the given texture view dimension.
            std::string computeShader = CreateComputeShaderWithStorageTexture(
                bindingType, kStorageTextureFormat, dimensionInShader);
            wgpu::ShaderModule csModule = utils::CreateShaderModule(
                device, utils::SingleShaderStage::Compute, computeShader.c_str());

            // Set common fields of compute pipeline descriptor.
            wgpu::ComputePipelineDescriptor defaultComputePipelineDescriptor;
            defaultComputePipelineDescriptor.computeStage.module = csModule;
            defaultComputePipelineDescriptor.computeStage.entryPoint = "main";

            // Set common fileds of bind group layout binding.
            wgpu::BindGroupLayoutEntry defaultBindGroupLayoutEntry = {0, wgpu::ShaderStage::Compute,
                                                                      bindingType};
            defaultBindGroupLayoutEntry.storageTextureFormat = kStorageTextureFormat;

            for (wgpu::TextureViewDimension dimensionInBindGroupLayout : kSupportedDimensions) {
                // Create the bind group layout with the given texture view dimension.
                wgpu::BindGroupLayoutEntry bindGroupLayoutBinding = defaultBindGroupLayoutEntry;
                bindGroupLayoutBinding.viewDimension = dimensionInBindGroupLayout;
                wgpu::BindGroupLayout bindGroupLayout =
                    utils::MakeBindGroupLayout(device, {bindGroupLayoutBinding});

                // Create the compute pipeline with the bind group layout.
                wgpu::ComputePipelineDescriptor computePipelineDescriptor =
                    defaultComputePipelineDescriptor;
                computePipelineDescriptor.layout =
                    utils::MakeBasicPipelineLayout(device, &bindGroupLayout);

                // The texture dimension in the bind group layout must be the same as the one
                // declared in the shader.
                if (dimensionInShader == dimensionInBindGroupLayout) {
                    device.CreateComputePipeline(&computePipelineDescriptor);
                } else {
                    ASSERT_DEVICE_ERROR(device.CreateComputePipeline(&computePipelineDescriptor));
                }
            }
        }
    }
}

// Verify that in a bind group layout binding neither read-only nor write-only storage textures
// are allowed to have dynamic offsets.
TEST_F(StorageTextureValidationTests, StorageTextureCannotHaveDynamicOffsets) {
    for (wgpu::BindingType storageBindingType : kSupportedStorageTextureBindingTypes) {
        wgpu::BindGroupLayoutEntry bindGroupLayoutBinding;
        bindGroupLayoutBinding.binding = 0;
        bindGroupLayoutBinding.visibility = wgpu::ShaderStage::Compute;
        bindGroupLayoutBinding.type = storageBindingType;
        bindGroupLayoutBinding.storageTextureFormat = wgpu::TextureFormat::R32Float;

        bindGroupLayoutBinding.hasDynamicOffset = true;
        ASSERT_DEVICE_ERROR(utils::MakeBindGroupLayout(device, {bindGroupLayoutBinding}));
    }
}

// Verify that only a texture view can be used as a read-only or write-only storage texture in a
// bind group.
TEST_F(StorageTextureValidationTests, StorageTextureBindingTypeInBindGroup) {
    constexpr wgpu::TextureFormat kStorageTextureFormat = wgpu::TextureFormat::R32Float;
    for (wgpu::BindingType storageBindingType : kSupportedStorageTextureBindingTypes) {
        // Create a bind group layout.
        wgpu::BindGroupLayoutEntry bindGroupLayoutBinding;
        bindGroupLayoutBinding.binding = 0;
        bindGroupLayoutBinding.visibility = wgpu::ShaderStage::Compute;
        bindGroupLayoutBinding.type = storageBindingType;
        bindGroupLayoutBinding.storageTextureFormat = kStorageTextureFormat;
        wgpu::BindGroupLayout bindGroupLayout =
            utils::MakeBindGroupLayout(device, {bindGroupLayoutBinding});

        // Buffers are not allowed to be used as storage textures in a bind group.
        {
            wgpu::BufferDescriptor descriptor;
            descriptor.size = 1024;
            descriptor.usage = wgpu::BufferUsage::Uniform;
            wgpu::Buffer buffer = device.CreateBuffer(&descriptor);
            ASSERT_DEVICE_ERROR(utils::MakeBindGroup(device, bindGroupLayout, {{0, buffer}}));
        }

        // Samplers are not allowed to be used as storage textures in a bind group.
        {
            wgpu::SamplerDescriptor descriptor = utils::GetDefaultSamplerDescriptor();
            wgpu::Sampler sampler = device.CreateSampler(&descriptor);
            ASSERT_DEVICE_ERROR(utils::MakeBindGroup(device, bindGroupLayout, {{0, sampler}}));
        }

        // Texture views are allowed to be used as storage textures in a bind group.
        {
            wgpu::TextureView textureView =
                CreateTexture(wgpu::TextureUsage::Storage, kStorageTextureFormat).CreateView();
            utils::MakeBindGroup(device, bindGroupLayout, {{0, textureView}});
        }
    }
}

// Verify that a texture used as read-only or write-only storage texture in a bind group must be
// created with the texture usage wgpu::TextureUsage::STORAGE.
TEST_F(StorageTextureValidationTests, StorageTextureUsageInBindGroup) {
    constexpr wgpu::TextureFormat kStorageTextureFormat = wgpu::TextureFormat::R32Float;
    constexpr std::array<wgpu::TextureUsage, 6> kTextureUsages = {
        wgpu::TextureUsage::CopySrc,          wgpu::TextureUsage::CopyDst,
        wgpu::TextureUsage::Sampled,          wgpu::TextureUsage::Storage,
        wgpu::TextureUsage::OutputAttachment, wgpu::TextureUsage::Present};

    for (wgpu::BindingType storageBindingType : kSupportedStorageTextureBindingTypes) {
        // Create a bind group layout.
        wgpu::BindGroupLayoutEntry bindGroupLayoutBinding;
        bindGroupLayoutBinding.binding = 0;
        bindGroupLayoutBinding.visibility = wgpu::ShaderStage::Compute;
        bindGroupLayoutBinding.type = storageBindingType;
        bindGroupLayoutBinding.storageTextureFormat = wgpu::TextureFormat::R32Float;
        wgpu::BindGroupLayout bindGroupLayout =
            utils::MakeBindGroupLayout(device, {bindGroupLayoutBinding});

        for (wgpu::TextureUsage usage : kTextureUsages) {
            // Create texture views with different texture usages
            wgpu::TextureView textureView =
                CreateTexture(usage, kStorageTextureFormat).CreateView();

            // Verify that the texture used as storage texture must be created with the texture
            // usage wgpu::TextureUsage::STORAGE.
            if (usage & wgpu::TextureUsage::Storage) {
                utils::MakeBindGroup(device, bindGroupLayout, {{0, textureView}});
            } else {
                ASSERT_DEVICE_ERROR(
                    utils::MakeBindGroup(device, bindGroupLayout, {{0, textureView}}));
            }
        }
    }
}

// Verify that the format of a texture used as read-only or write-only storage texture in a bind
// group must match the corresponding bind group binding.
TEST_F(StorageTextureValidationTests, StorageTextureFormatInBindGroup) {
    for (wgpu::BindingType storageBindingType : kSupportedStorageTextureBindingTypes) {
        wgpu::BindGroupLayoutEntry defaultBindGroupLayoutEntry;
        defaultBindGroupLayoutEntry.binding = 0;
        defaultBindGroupLayoutEntry.visibility = wgpu::ShaderStage::Compute;
        defaultBindGroupLayoutEntry.type = storageBindingType;

        for (wgpu::TextureFormat formatInBindGroupLayout : utils::kAllTextureFormats) {
            if (!utils::TextureFormatSupportsStorageTexture(formatInBindGroupLayout)) {
                continue;
            }

            // Create a bind group layout with given storage texture format.
            wgpu::BindGroupLayoutEntry bindGroupLayoutBinding = defaultBindGroupLayoutEntry;
            bindGroupLayoutBinding.storageTextureFormat = formatInBindGroupLayout;
            wgpu::BindGroupLayout bindGroupLayout =
                utils::MakeBindGroupLayout(device, {bindGroupLayoutBinding});

            for (wgpu::TextureFormat textureViewFormat : utils::kAllTextureFormats) {
                if (!utils::TextureFormatSupportsStorageTexture(textureViewFormat)) {
                    continue;
                }

                // Create texture views with different texture formats.
                wgpu::TextureView storageTextureView =
                    CreateTexture(wgpu::TextureUsage::Storage, textureViewFormat).CreateView();

                // Verify that the format of the texture view used as storage texture in a bind
                // group must match the storage texture format declaration in the bind group layout.
                if (textureViewFormat == formatInBindGroupLayout) {
                    utils::MakeBindGroup(device, bindGroupLayout, {{0, storageTextureView}});
                } else {
                    ASSERT_DEVICE_ERROR(
                        utils::MakeBindGroup(device, bindGroupLayout, {{0, storageTextureView}}));
                }
            }
        }
    }
}

// Verify that the dimension of a texture view used as read-only or write-only storage texture in a
// bind group must match the corresponding bind group binding.
TEST_F(StorageTextureValidationTests, StorageTextureViewDimensionInBindGroup) {
    constexpr wgpu::TextureFormat kStorageTextureFormat = wgpu::TextureFormat::R32Float;
    constexpr uint32_t kArrayLayerCount = 6u;

    // Currently we only support creating 2D-compatible texture view dimensions.
    // TODO(jiawei.shao@intel.com): test the use of 1D and 3D texture view dimensions when they are
    // supported in Dawn.
    constexpr std::array<wgpu::TextureViewDimension, 2> kSupportedDimensions = {
        wgpu::TextureViewDimension::e2D, wgpu::TextureViewDimension::e2DArray};

    wgpu::Texture texture =
        CreateTexture(wgpu::TextureUsage::Storage, kStorageTextureFormat, 1, kArrayLayerCount);

    wgpu::TextureViewDescriptor kDefaultTextureViewDescriptor;
    kDefaultTextureViewDescriptor.format = kStorageTextureFormat;
    kDefaultTextureViewDescriptor.baseMipLevel = 0;
    kDefaultTextureViewDescriptor.mipLevelCount = 1;
    kDefaultTextureViewDescriptor.baseArrayLayer = 0;
    kDefaultTextureViewDescriptor.arrayLayerCount = 1u;

    for (wgpu::BindingType storageBindingType : kSupportedStorageTextureBindingTypes) {
        wgpu::BindGroupLayoutEntry defaultBindGroupLayoutEntry;
        defaultBindGroupLayoutEntry.binding = 0;
        defaultBindGroupLayoutEntry.visibility = wgpu::ShaderStage::Compute;
        defaultBindGroupLayoutEntry.type = storageBindingType;
        defaultBindGroupLayoutEntry.storageTextureFormat = kStorageTextureFormat;

        for (wgpu::TextureViewDimension dimensionInBindGroupLayout : kSupportedDimensions) {
            // Create a bind group layout with given texture view dimension.
            wgpu::BindGroupLayoutEntry bindGroupLayoutBinding = defaultBindGroupLayoutEntry;
            bindGroupLayoutBinding.viewDimension = dimensionInBindGroupLayout;
            wgpu::BindGroupLayout bindGroupLayout =
                utils::MakeBindGroupLayout(device, {bindGroupLayoutBinding});

            for (wgpu::TextureViewDimension dimensionOfTextureView : kSupportedDimensions) {
                // Create a texture view with given texture view dimension.
                wgpu::TextureViewDescriptor textureViewDescriptor = kDefaultTextureViewDescriptor;
                textureViewDescriptor.dimension = dimensionOfTextureView;
                wgpu::TextureView storageTextureView = texture.CreateView(&textureViewDescriptor);

                // Verify that the dimension of the texture view used as storage texture in a bind
                // group must match the texture view dimension declaration in the bind group layout.
                if (dimensionInBindGroupLayout == dimensionOfTextureView) {
                    utils::MakeBindGroup(device, bindGroupLayout, {{0, storageTextureView}});
                } else {
                    ASSERT_DEVICE_ERROR(
                        utils::MakeBindGroup(device, bindGroupLayout, {{0, storageTextureView}}));
                }
            }
        }
    }
}

// Verify multisampled storage textures cannot be supported now.
TEST_F(StorageTextureValidationTests, MultisampledStorageTexture) {
    for (wgpu::BindingType bindingType : kSupportedStorageTextureBindingTypes) {
        std::string computeShader =
            CreateComputeShaderWithStorageTexture(bindingType, "rgba8", "", "image2DMS");
        wgpu::ShaderModule csModule = utils::CreateShaderModule(
            device, utils::SingleShaderStage::Compute, computeShader.c_str());

        wgpu::ComputePipelineDescriptor descriptor;
        descriptor.layout = nullptr;
        descriptor.computeStage.module = csModule;
        descriptor.computeStage.entryPoint = "main";

        ASSERT_DEVICE_ERROR(device.CreateComputePipeline(&descriptor));
    }
}

// Verify it is valid to use a texture as either read-only storage texture or write-only storage
// texture in a render pass.
TEST_F(StorageTextureValidationTests, StorageTextureInRenderPass) {
    constexpr wgpu::TextureFormat kFormat = wgpu::TextureFormat::RGBA8Unorm;
    wgpu::Texture storageTexture = CreateTexture(wgpu::TextureUsage::Storage, kFormat);

    wgpu::Texture outputAttachment = CreateTexture(wgpu::TextureUsage::OutputAttachment, kFormat);
    utils::ComboRenderPassDescriptor renderPassDescriptor({outputAttachment.CreateView()});

    for (wgpu::BindingType storageTextureType : kSupportedStorageTextureBindingTypes) {
        // Create a bind group that contains a storage texture.
        wgpu::BindGroupLayout bindGroupLayout = utils::MakeBindGroupLayout(
            device,
            {{0, wgpu::ShaderStage::Fragment, storageTextureType, false, 0, false,
              wgpu::TextureViewDimension::Undefined, wgpu::TextureComponentType::Float, kFormat}});

        wgpu::BindGroup bindGroupWithStorageTexture =
            utils::MakeBindGroup(device, bindGroupLayout, {{0, storageTexture.CreateView()}});

        // It is valid to use a texture as read-only or write-only storage texture in the render
        // pass.
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::RenderPassEncoder renderPassEncoder = encoder.BeginRenderPass(&renderPassDescriptor);
        renderPassEncoder.SetBindGroup(0, bindGroupWithStorageTexture);
        renderPassEncoder.EndPass();
        encoder.Finish();
    }
}

// Verify it is valid to use a a texture as both read-only storage texture and sampled texture in
// one render pass, while it is invalid to use a texture as both write-only storage texture and
// sampled texture in one render pass.
TEST_F(StorageTextureValidationTests, StorageTextureAndSampledTextureInOneRenderPass) {
    constexpr wgpu::TextureFormat kFormat = wgpu::TextureFormat::RGBA8Unorm;
    wgpu::Texture storageTexture =
        CreateTexture(wgpu::TextureUsage::Storage | wgpu::TextureUsage::Sampled, kFormat);

    wgpu::Texture outputAttachment = CreateTexture(wgpu::TextureUsage::OutputAttachment, kFormat);
    utils::ComboRenderPassDescriptor renderPassDescriptor({outputAttachment.CreateView()});

    // Create a bind group that contains a storage texture and a sampled texture.
    for (wgpu::BindingType storageTextureType : kSupportedStorageTextureBindingTypes) {
        // Create a bind group that binds the same texture as both storage texture and sampled
        // texture.
        wgpu::BindGroupLayout bindGroupLayout = utils::MakeBindGroupLayout(
            device,
            {{0, wgpu::ShaderStage::Fragment, storageTextureType, false, 0, false,
              wgpu::TextureViewDimension::Undefined, wgpu::TextureComponentType::Float, kFormat},
             {1, wgpu::ShaderStage::Fragment, wgpu::BindingType::SampledTexture, false, 0, false,
              wgpu::TextureViewDimension::Undefined, wgpu::TextureComponentType::Float, kFormat}});
        wgpu::BindGroup bindGroup = utils::MakeBindGroup(
            device, bindGroupLayout,
            {{0, storageTexture.CreateView()}, {1, storageTexture.CreateView()}});

        // It is valid to use a a texture as both read-only storage texture and sampled texture in
        // one render pass, while it is invalid to use a texture as both write-only storage
        // texture an sampled texture in one render pass.
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::RenderPassEncoder renderPassEncoder = encoder.BeginRenderPass(&renderPassDescriptor);
        renderPassEncoder.SetBindGroup(0, bindGroup);
        renderPassEncoder.EndPass();
        switch (storageTextureType) {
            case wgpu::BindingType::ReadonlyStorageTexture:
                encoder.Finish();
                break;
            case wgpu::BindingType::WriteonlyStorageTexture:
                ASSERT_DEVICE_ERROR(encoder.Finish());
                break;
            default:
                UNREACHABLE();
                break;
        }
    }
}

// Verify it is invalid to use a a texture as both storage texture (either read-only or write-only)
// and output attachment in one render pass.
TEST_F(StorageTextureValidationTests, StorageTextureAndOutputAttachmentInOneRenderPass) {
    constexpr wgpu::TextureFormat kFormat = wgpu::TextureFormat::RGBA8Unorm;
    wgpu::Texture storageTexture =
        CreateTexture(wgpu::TextureUsage::Storage | wgpu::TextureUsage::OutputAttachment, kFormat);
    utils::ComboRenderPassDescriptor renderPassDescriptor({storageTexture.CreateView()});

    for (wgpu::BindingType storageTextureType : kSupportedStorageTextureBindingTypes) {
        // Create a bind group that contains a storage texture.
        wgpu::BindGroupLayout bindGroupLayout = utils::MakeBindGroupLayout(
            device,
            {{0, wgpu::ShaderStage::Fragment, storageTextureType, false, 0, false,
              wgpu::TextureViewDimension::Undefined, wgpu::TextureComponentType::Float, kFormat}});
        wgpu::BindGroup bindGroupWithStorageTexture =
            utils::MakeBindGroup(device, bindGroupLayout, {{0, storageTexture.CreateView()}});

        // It is invalid to use a texture as both storage texture and output attachment in one
        // render pass.
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::RenderPassEncoder renderPassEncoder = encoder.BeginRenderPass(&renderPassDescriptor);
        renderPassEncoder.SetBindGroup(0, bindGroupWithStorageTexture);
        renderPassEncoder.EndPass();
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }
}

// Verify it is invalid to use a a texture as both read-only storage texture and write-only storage
// texture in one render pass.
TEST_F(StorageTextureValidationTests, ReadOnlyAndWriteOnlyStorageTextureInOneRenderPass) {
    constexpr wgpu::TextureFormat kFormat = wgpu::TextureFormat::RGBA8Unorm;
    wgpu::Texture storageTexture = CreateTexture(wgpu::TextureUsage::Storage, kFormat);

    // Create a bind group that uses the same texture as both read-only and write-only storage
    // texture.
    wgpu::BindGroupLayout bindGroupLayout = utils::MakeBindGroupLayout(
        device,
        {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::ReadonlyStorageTexture, false, 0,
          false, wgpu::TextureViewDimension::Undefined, wgpu::TextureComponentType::Float, kFormat},
         {1, wgpu::ShaderStage::Fragment, wgpu::BindingType::WriteonlyStorageTexture, false, 0,
          false, wgpu::TextureViewDimension::Undefined, wgpu::TextureComponentType::Float,
          kFormat}});
    wgpu::BindGroup bindGroup =
        utils::MakeBindGroup(device, bindGroupLayout,
                             {{0, storageTexture.CreateView()}, {1, storageTexture.CreateView()}});

    // It is invalid to use a texture as both read-only storage texture and write-only storage
    // texture in one render pass.
    wgpu::Texture outputAttachment = CreateTexture(wgpu::TextureUsage::OutputAttachment, kFormat);
    utils::ComboRenderPassDescriptor renderPassDescriptor({outputAttachment.CreateView()});
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::RenderPassEncoder renderPassEncoder = encoder.BeginRenderPass(&renderPassDescriptor);
    renderPassEncoder.SetBindGroup(0, bindGroup);
    renderPassEncoder.EndPass();
    ASSERT_DEVICE_ERROR(encoder.Finish());
}

// Verify it is valid to use a texture as both storage texture (read-only or write-only) and
// sampled texture in one compute pass.
TEST_F(StorageTextureValidationTests, StorageTextureAndSampledTextureInOneComputePass) {
    constexpr wgpu::TextureFormat kFormat = wgpu::TextureFormat::RGBA8Unorm;
    wgpu::Texture storageTexture =
        CreateTexture(wgpu::TextureUsage::Storage | wgpu::TextureUsage::Sampled, kFormat);

    for (wgpu::BindingType storageTextureType : kSupportedStorageTextureBindingTypes) {
        // Create a bind group that binds the same texture as both storage texture and sampled
        // texture.
        wgpu::BindGroupLayout bindGroupLayout = utils::MakeBindGroupLayout(
            device,
            {{0, wgpu::ShaderStage::Compute, storageTextureType, false, 0, false,
              wgpu::TextureViewDimension::Undefined, wgpu::TextureComponentType::Float, kFormat},
             {1, wgpu::ShaderStage::Compute, wgpu::BindingType::SampledTexture, false, 0, false,
              wgpu::TextureViewDimension::Undefined, wgpu::TextureComponentType::Float, kFormat}});
        wgpu::BindGroup bindGroup = utils::MakeBindGroup(
            device, bindGroupLayout,
            {{0, storageTexture.CreateView()}, {1, storageTexture.CreateView()}});

        // It is valid to use a a texture as both storage texture (read-only or write-only) and
        // sampled texture in one compute pass.
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::ComputePassEncoder computePassEncoder = encoder.BeginComputePass();
        computePassEncoder.SetBindGroup(0, bindGroup);
        computePassEncoder.EndPass();
        encoder.Finish();
    }
}

// Verify it is valid to use a texture as both read-only storage texture and write-only storage
// texture in one compute pass.
TEST_F(StorageTextureValidationTests, ReadOnlyAndWriteOnlyStorageTextureInOneComputePass) {
    constexpr wgpu::TextureFormat kFormat = wgpu::TextureFormat::RGBA8Unorm;
    wgpu::Texture storageTexture = CreateTexture(wgpu::TextureUsage::Storage, kFormat);

    // Create a bind group that uses the same texture as both read-only and write-only storage
    // texture.
    wgpu::BindGroupLayout bindGroupLayout = utils::MakeBindGroupLayout(
        device,
        {{0, wgpu::ShaderStage::Compute, wgpu::BindingType::ReadonlyStorageTexture, false, 0, false,
          wgpu::TextureViewDimension::Undefined, wgpu::TextureComponentType::Float, kFormat},
         {1, wgpu::ShaderStage::Compute, wgpu::BindingType::WriteonlyStorageTexture, false, 0,
          false, wgpu::TextureViewDimension::Undefined, wgpu::TextureComponentType::Float,
          kFormat}});
    wgpu::BindGroup bindGroup =
        utils::MakeBindGroup(device, bindGroupLayout,
                             {{0, storageTexture.CreateView()}, {1, storageTexture.CreateView()}});

    // It is valid to use a texture as both read-only storage texture and write-only storage
    // texture in one compute pass.
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::ComputePassEncoder computePassEncoder = encoder.BeginComputePass();
    computePassEncoder.SetBindGroup(0, bindGroup);
    computePassEncoder.EndPass();
    encoder.Finish();
}
