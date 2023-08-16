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

#include "tests/unittests/validation/ValidationTest.h"

#include "utils/ComboRenderPipelineDescriptor.h"
#include "utils/WGPUHelpers.h"

class GetBindGroupLayoutTests : public ValidationTest {
  protected:
    static constexpr wgpu::ShaderStage kVisibilityAll =
        wgpu::ShaderStage::Compute | wgpu::ShaderStage::Fragment | wgpu::ShaderStage::Vertex;

    wgpu::RenderPipeline RenderPipelineFromFragmentShader(const char* shader) {
        wgpu::ShaderModule vsModule =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
        #version 450
        void main() {
        })");
        wgpu::ShaderModule fsModule =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, shader);

        utils::ComboRenderPipelineDescriptor descriptor(device);
        descriptor.layout = nullptr;
        descriptor.vertexStage.module = vsModule;
        descriptor.cFragmentStage.module = fsModule;

        return device.CreateRenderPipeline(&descriptor);
    }
};

// Test that GetBindGroupLayout returns the same object for the same index
// and for matching layouts.
TEST_F(GetBindGroupLayoutTests, SameObject) {
    wgpu::ShaderModule vsModule =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
        #version 450
        layout(set = 0, binding = 0) uniform UniformBuffer1 {
            vec4 pos1;
        };

        layout(set = 1, binding = 0) uniform UniformBuffer2 {
            vec4 pos2;
        };

        void main() {
        })");

    wgpu::ShaderModule fsModule =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, R"(
        #version 450
        layout(set = 2, binding = 0) uniform UniformBuffer3 {
            vec4 pos3;
        };

        layout(set = 3, binding = 0) buffer StorageBuffer {
            mat4 pos4;
        };

        void main() {
        })");

    utils::ComboRenderPipelineDescriptor descriptor(device);
    descriptor.layout = nullptr;
    descriptor.vertexStage.module = vsModule;
    descriptor.cFragmentStage.module = fsModule;

    wgpu::RenderPipeline pipeline = device.CreateRenderPipeline(&descriptor);

    EXPECT_EQ(pipeline.GetBindGroupLayout(0).Get(), pipeline.GetBindGroupLayout(0).Get());

    EXPECT_EQ(pipeline.GetBindGroupLayout(1).Get(), pipeline.GetBindGroupLayout(1).Get());

    EXPECT_EQ(pipeline.GetBindGroupLayout(0).Get(), pipeline.GetBindGroupLayout(1).Get());

    EXPECT_EQ(pipeline.GetBindGroupLayout(0).Get(), pipeline.GetBindGroupLayout(2).Get());

    EXPECT_NE(pipeline.GetBindGroupLayout(0).Get(), pipeline.GetBindGroupLayout(3).Get());
}

// Test that getBindGroupLayout defaults are correct
// - shader stage visibility is All
// - dynamic offsets is false
TEST_F(GetBindGroupLayoutTests, DefaultShaderStageAndDynamicOffsets) {
    wgpu::RenderPipeline pipeline = RenderPipelineFromFragmentShader(R"(
        #version 450
        layout(set = 0, binding = 0) uniform UniformBuffer {
            vec4 pos;
        };

        void main() {
        })");

    wgpu::BindGroupLayoutEntry binding = {};
    binding.binding = 0;
    binding.type = wgpu::BindingType::UniformBuffer;
    binding.multisampled = false;
    binding.minBufferBindingSize = 4 * sizeof(float);

    wgpu::BindGroupLayoutDescriptor desc = {};
    desc.entryCount = 1;
    desc.entries = &binding;

    // Check that visibility and dynamic offsets match
    binding.hasDynamicOffset = false;
    binding.visibility = kVisibilityAll;
    EXPECT_EQ(device.CreateBindGroupLayout(&desc).Get(), pipeline.GetBindGroupLayout(0).Get());

    // Check that any change in visibility doesn't match.
    binding.visibility = wgpu::ShaderStage::Vertex;
    EXPECT_NE(device.CreateBindGroupLayout(&desc).Get(), pipeline.GetBindGroupLayout(0).Get());

    binding.visibility = wgpu::ShaderStage::Fragment;
    EXPECT_NE(device.CreateBindGroupLayout(&desc).Get(), pipeline.GetBindGroupLayout(0).Get());

    binding.visibility = wgpu::ShaderStage::Compute;
    EXPECT_NE(device.CreateBindGroupLayout(&desc).Get(), pipeline.GetBindGroupLayout(0).Get());

    // Check that any change in hasDynamicOffsets doesn't match.
    binding.hasDynamicOffset = true;
    binding.visibility = kVisibilityAll;
    EXPECT_NE(device.CreateBindGroupLayout(&desc).Get(), pipeline.GetBindGroupLayout(0).Get());
}

// Test GetBindGroupLayout works with a compute pipeline
TEST_F(GetBindGroupLayoutTests, ComputePipeline) {
    wgpu::ShaderModule csModule =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Compute, R"(
        #version 450
        layout(set = 0, binding = 0) uniform UniformBuffer {
            vec4 pos;
        };
        void main() {
        })");

    wgpu::ComputePipelineDescriptor descriptor;
    descriptor.layout = nullptr;
    descriptor.computeStage.module = csModule;
    descriptor.computeStage.entryPoint = "main";

    wgpu::ComputePipeline pipeline = device.CreateComputePipeline(&descriptor);

    wgpu::BindGroupLayoutEntry binding = {};
    binding.binding = 0;
    binding.type = wgpu::BindingType::UniformBuffer;
    binding.visibility = kVisibilityAll;
    binding.hasDynamicOffset = false;
    binding.minBufferBindingSize = 4 * sizeof(float);

    wgpu::BindGroupLayoutDescriptor desc = {};
    desc.entryCount = 1;
    desc.entries = &binding;

    EXPECT_EQ(device.CreateBindGroupLayout(&desc).Get(), pipeline.GetBindGroupLayout(0).Get());
}

// Test that the binding type matches the shader.
TEST_F(GetBindGroupLayoutTests, BindingType) {
    wgpu::BindGroupLayoutEntry binding = {};
    binding.binding = 0;
    binding.hasDynamicOffset = false;
    binding.multisampled = false;
    binding.minBufferBindingSize = 4 * sizeof(float);

    wgpu::BindGroupLayoutDescriptor desc = {};
    desc.entryCount = 1;
    desc.entries = &binding;

    {
        // Storage buffer binding is not supported in vertex shader.
        binding.visibility = wgpu::ShaderStage::Compute | wgpu::ShaderStage::Fragment;
        binding.type = wgpu::BindingType::StorageBuffer;
        wgpu::RenderPipeline pipeline = RenderPipelineFromFragmentShader(R"(
        #version 450
        layout(set = 0, binding = 0) buffer Storage {
            vec4 pos;
        };

        void main() {})");
        EXPECT_EQ(device.CreateBindGroupLayout(&desc).Get(), pipeline.GetBindGroupLayout(0).Get());
    }

    binding.visibility = kVisibilityAll;
    {
        binding.type = wgpu::BindingType::UniformBuffer;
        wgpu::RenderPipeline pipeline = RenderPipelineFromFragmentShader(R"(
        #version 450
        layout(set = 0, binding = 0) uniform Buffer {
            vec4 pos;
        };

        void main() {})");
        EXPECT_EQ(device.CreateBindGroupLayout(&desc).Get(), pipeline.GetBindGroupLayout(0).Get());
    }

    {
        binding.type = wgpu::BindingType::ReadonlyStorageBuffer;
        wgpu::RenderPipeline pipeline = RenderPipelineFromFragmentShader(R"(
        #version 450
        layout(set = 0, binding = 0) readonly buffer Storage {
            vec4 pos;
        };

        void main() {})");
        EXPECT_EQ(device.CreateBindGroupLayout(&desc).Get(), pipeline.GetBindGroupLayout(0).Get());
    }

    binding.minBufferBindingSize = 0;
    {
        binding.type = wgpu::BindingType::SampledTexture;
        wgpu::RenderPipeline pipeline = RenderPipelineFromFragmentShader(R"(
        #version 450
        layout(set = 0, binding = 0) uniform texture2D tex;

        void main() {})");
        EXPECT_EQ(device.CreateBindGroupLayout(&desc).Get(), pipeline.GetBindGroupLayout(0).Get());
    }

    {
        binding.type = wgpu::BindingType::Sampler;
        wgpu::RenderPipeline pipeline = RenderPipelineFromFragmentShader(R"(
        #version 450
        layout(set = 0, binding = 0) uniform sampler samp;

        void main() {})");
        EXPECT_EQ(device.CreateBindGroupLayout(&desc).Get(), pipeline.GetBindGroupLayout(0).Get());
    }
}

// Test that multisampling matches the shader.
TEST_F(GetBindGroupLayoutTests, Multisampled) {
    wgpu::BindGroupLayoutEntry binding = {};
    binding.binding = 0;
    binding.type = wgpu::BindingType::SampledTexture;
    binding.visibility = kVisibilityAll;
    binding.hasDynamicOffset = false;

    wgpu::BindGroupLayoutDescriptor desc = {};
    desc.entryCount = 1;
    desc.entries = &binding;

    {
        binding.multisampled = false;
        wgpu::RenderPipeline pipeline = RenderPipelineFromFragmentShader(R"(
        #version 450
        layout(set = 0, binding = 0) uniform texture2D tex;

        void main() {})");
        EXPECT_EQ(device.CreateBindGroupLayout(&desc).Get(), pipeline.GetBindGroupLayout(0).Get());
    }

    {
        binding.multisampled = true;
        wgpu::RenderPipeline pipeline = RenderPipelineFromFragmentShader(R"(
        #version 450
        layout(set = 0, binding = 0) uniform texture2DMS tex;

        void main() {})");
        EXPECT_EQ(device.CreateBindGroupLayout(&desc).Get(), pipeline.GetBindGroupLayout(0).Get());
    }
}

// Test that texture view dimension matches the shader.
TEST_F(GetBindGroupLayoutTests, ViewDimension) {
    wgpu::BindGroupLayoutEntry binding = {};
    binding.binding = 0;
    binding.type = wgpu::BindingType::SampledTexture;
    binding.visibility = kVisibilityAll;
    binding.hasDynamicOffset = false;
    binding.multisampled = false;

    wgpu::BindGroupLayoutDescriptor desc = {};
    desc.entryCount = 1;
    desc.entries = &binding;

    {
        binding.viewDimension = wgpu::TextureViewDimension::e1D;
        wgpu::RenderPipeline pipeline = RenderPipelineFromFragmentShader(R"(
        #version 450
        layout(set = 0, binding = 0) uniform texture1D tex;

        void main() {})");
        EXPECT_EQ(device.CreateBindGroupLayout(&desc).Get(), pipeline.GetBindGroupLayout(0).Get());
    }

    {
        binding.viewDimension = wgpu::TextureViewDimension::e2D;
        wgpu::RenderPipeline pipeline = RenderPipelineFromFragmentShader(R"(
        #version 450
        layout(set = 0, binding = 0) uniform texture2D tex;

        void main() {})");
        EXPECT_EQ(device.CreateBindGroupLayout(&desc).Get(), pipeline.GetBindGroupLayout(0).Get());
    }

    {
        binding.viewDimension = wgpu::TextureViewDimension::e2DArray;
        wgpu::RenderPipeline pipeline = RenderPipelineFromFragmentShader(R"(
        #version 450
        layout(set = 0, binding = 0) uniform texture2DArray tex;

        void main() {})");
        EXPECT_EQ(device.CreateBindGroupLayout(&desc).Get(), pipeline.GetBindGroupLayout(0).Get());
    }

    {
        binding.viewDimension = wgpu::TextureViewDimension::e3D;
        wgpu::RenderPipeline pipeline = RenderPipelineFromFragmentShader(R"(
        #version 450
        layout(set = 0, binding = 0) uniform texture3D tex;

        void main() {})");
        EXPECT_EQ(device.CreateBindGroupLayout(&desc).Get(), pipeline.GetBindGroupLayout(0).Get());
    }

    {
        binding.viewDimension = wgpu::TextureViewDimension::Cube;
        wgpu::RenderPipeline pipeline = RenderPipelineFromFragmentShader(R"(
        #version 450
        layout(set = 0, binding = 0) uniform textureCube tex;

        void main() {})");
        EXPECT_EQ(device.CreateBindGroupLayout(&desc).Get(), pipeline.GetBindGroupLayout(0).Get());
    }

    {
        binding.viewDimension = wgpu::TextureViewDimension::CubeArray;
        wgpu::RenderPipeline pipeline = RenderPipelineFromFragmentShader(R"(
                #version 450
                layout(set = 0, binding = 0) uniform textureCubeArray tex;

                void main() {})");
        EXPECT_EQ(device.CreateBindGroupLayout(&desc).Get(), pipeline.GetBindGroupLayout(0).Get());
    }
}

// Test that texture component type matches the shader.
TEST_F(GetBindGroupLayoutTests, TextureComponentType) {
    wgpu::BindGroupLayoutEntry binding = {};
    binding.binding = 0;
    binding.type = wgpu::BindingType::SampledTexture;
    binding.visibility = kVisibilityAll;
    binding.hasDynamicOffset = false;
    binding.multisampled = false;

    wgpu::BindGroupLayoutDescriptor desc = {};
    desc.entryCount = 1;
    desc.entries = &binding;

    {
        binding.textureComponentType = wgpu::TextureComponentType::Float;
        wgpu::RenderPipeline pipeline = RenderPipelineFromFragmentShader(R"(
                #version 450
                layout(set = 0, binding = 0) uniform texture2D tex;

                void main() {})");
        EXPECT_EQ(device.CreateBindGroupLayout(&desc).Get(), pipeline.GetBindGroupLayout(0).Get());
    }

    {
        binding.textureComponentType = wgpu::TextureComponentType::Sint;
        wgpu::RenderPipeline pipeline = RenderPipelineFromFragmentShader(R"(
                #version 450
                layout(set = 0, binding = 0) uniform itexture2D tex;

                void main() {})");
        EXPECT_EQ(device.CreateBindGroupLayout(&desc).Get(), pipeline.GetBindGroupLayout(0).Get());
    }

    {
        binding.textureComponentType = wgpu::TextureComponentType::Uint;
        wgpu::RenderPipeline pipeline = RenderPipelineFromFragmentShader(R"(
                #version 450
                layout(set = 0, binding = 0) uniform utexture2D tex;

                void main() {})");
        EXPECT_EQ(device.CreateBindGroupLayout(&desc).Get(), pipeline.GetBindGroupLayout(0).Get());
    }
}

// Test that binding= indices match.
TEST_F(GetBindGroupLayoutTests, BindingIndices) {
    wgpu::BindGroupLayoutEntry binding = {};
    binding.type = wgpu::BindingType::UniformBuffer;
    binding.visibility = kVisibilityAll;
    binding.hasDynamicOffset = false;
    binding.multisampled = false;
    binding.minBufferBindingSize = 4 * sizeof(float);

    wgpu::BindGroupLayoutDescriptor desc = {};
    desc.entryCount = 1;
    desc.entries = &binding;

    {
        binding.binding = 0;
        wgpu::RenderPipeline pipeline = RenderPipelineFromFragmentShader(R"(
                #version 450
                layout(set = 0, binding = 0) uniform Buffer {
                    vec4 pos;
                };

                void main() {})");
        EXPECT_EQ(device.CreateBindGroupLayout(&desc).Get(), pipeline.GetBindGroupLayout(0).Get());
    }

    {
        binding.binding = 1;
        wgpu::RenderPipeline pipeline = RenderPipelineFromFragmentShader(R"(
                #version 450
                layout(set = 0, binding = 1) uniform Buffer {
                    vec4 pos;
                };

                void main() {})");
        EXPECT_EQ(device.CreateBindGroupLayout(&desc).Get(), pipeline.GetBindGroupLayout(0).Get());
    }

    {
        binding.binding = 2;
        wgpu::RenderPipeline pipeline = RenderPipelineFromFragmentShader(R"(
                #version 450
                layout(set = 0, binding = 1) uniform Buffer {
                    vec4 pos;
                };

                void main() {})");
        EXPECT_NE(device.CreateBindGroupLayout(&desc).Get(), pipeline.GetBindGroupLayout(0).Get());
    }
}

// Test it is valid to have duplicate bindings in the shaders.
TEST_F(GetBindGroupLayoutTests, DuplicateBinding) {
    wgpu::ShaderModule vsModule =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
        #version 450
        layout(set = 0, binding = 0) uniform UniformBuffer1 {
            vec4 pos1;
        };

        layout(set = 1, binding = 0) uniform UniformBuffer2 {
            vec4 pos2;
        };

        void main() {})");

    wgpu::ShaderModule fsModule =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, R"(
        #version 450
        layout(set = 1, binding = 0) uniform UniformBuffer3 {
            vec4 pos3;
        };

        void main() {})");

    utils::ComboRenderPipelineDescriptor descriptor(device);
    descriptor.layout = nullptr;
    descriptor.vertexStage.module = vsModule;
    descriptor.cFragmentStage.module = fsModule;

    device.CreateRenderPipeline(&descriptor);
}

// Test it is invalid to have conflicting binding types in the shaders.
TEST_F(GetBindGroupLayoutTests, ConflictingBindingType) {
    wgpu::ShaderModule vsModule =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
        #version 450
        layout(set = 0, binding = 0) uniform UniformBuffer {
            vec4 pos;
        };

        void main() {})");

    wgpu::ShaderModule fsModule =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, R"(
        #version 450
        layout(set = 0, binding = 0) buffer StorageBuffer {
            vec4 pos;
        };

        void main() {})");

    utils::ComboRenderPipelineDescriptor descriptor(device);
    descriptor.layout = nullptr;
    descriptor.vertexStage.module = vsModule;
    descriptor.cFragmentStage.module = fsModule;

    ASSERT_DEVICE_ERROR(device.CreateRenderPipeline(&descriptor));
}

// Test it is invalid to have conflicting binding texture multisampling in the shaders.
// TODO: Support multisampling
TEST_F(GetBindGroupLayoutTests, DISABLED_ConflictingBindingTextureMultisampling) {
    wgpu::ShaderModule vsModule =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
        #version 450
        layout(set = 0, binding = 0) uniform texture2D tex;

        void main() {})");

    wgpu::ShaderModule fsModule =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, R"(
        #version 450
        layout(set = 0, binding = 0) uniform texture2DMS tex;

        void main() {})");

    utils::ComboRenderPipelineDescriptor descriptor(device);
    descriptor.layout = nullptr;
    descriptor.vertexStage.module = vsModule;
    descriptor.cFragmentStage.module = fsModule;

    ASSERT_DEVICE_ERROR(device.CreateRenderPipeline(&descriptor));
}

// Test it is invalid to have conflicting binding texture dimension in the shaders.
TEST_F(GetBindGroupLayoutTests, ConflictingBindingViewDimension) {
    wgpu::ShaderModule vsModule =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
        #version 450
        layout(set = 0, binding = 0) uniform texture2D tex;

        void main() {})");

    wgpu::ShaderModule fsModule =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, R"(
        #version 450
        layout(set = 0, binding = 0) uniform texture3D tex;

        void main() {})");

    utils::ComboRenderPipelineDescriptor descriptor(device);
    descriptor.layout = nullptr;
    descriptor.vertexStage.module = vsModule;
    descriptor.cFragmentStage.module = fsModule;

    ASSERT_DEVICE_ERROR(device.CreateRenderPipeline(&descriptor));
}

// Test it is invalid to have conflicting binding texture component type in the shaders.
TEST_F(GetBindGroupLayoutTests, ConflictingBindingTextureComponentType) {
    wgpu::ShaderModule vsModule =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
        #version 450
        layout(set = 0, binding = 0) uniform texture2D tex;

        void main() {})");

    wgpu::ShaderModule fsModule =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, R"(
        #version 450
        layout(set = 0, binding = 0) uniform utexture2D tex;

        void main() {})");

    utils::ComboRenderPipelineDescriptor descriptor(device);
    descriptor.layout = nullptr;
    descriptor.vertexStage.module = vsModule;
    descriptor.cFragmentStage.module = fsModule;

    ASSERT_DEVICE_ERROR(device.CreateRenderPipeline(&descriptor));
}

// Test it is an error to query an out of range bind group layout.
TEST_F(GetBindGroupLayoutTests, OutOfRangeIndex) {
    ASSERT_DEVICE_ERROR(RenderPipelineFromFragmentShader(R"(
        #version 450
        layout(set = 0, binding = 0) uniform Buffer1 {
            vec4 pos1;
        };
        void main() {})")
                            .GetBindGroupLayout(kMaxBindGroups));

    ASSERT_DEVICE_ERROR(RenderPipelineFromFragmentShader(R"(
        #version 450
        layout(set = 0, binding = 0) uniform Buffer1 {
            vec4 pos1;
        };
        void main() {})")
                            .GetBindGroupLayout(kMaxBindGroups + 1));
}

// Test that unused indices return the empty bind group layout.
TEST_F(GetBindGroupLayoutTests, UnusedIndex) {
    wgpu::RenderPipeline pipeline = RenderPipelineFromFragmentShader(R"(
        #version 450
        layout(set = 0, binding = 0) uniform Buffer1 {
            vec4 pos1;
        };

        layout(set = 2, binding = 0) uniform Buffer2 {
            vec4 pos2;
        };

        void main() {})");

    wgpu::BindGroupLayoutDescriptor desc = {};
    desc.entryCount = 0;
    desc.entries = nullptr;

    wgpu::BindGroupLayout emptyBindGroupLayout = device.CreateBindGroupLayout(&desc);

    EXPECT_NE(pipeline.GetBindGroupLayout(0).Get(), emptyBindGroupLayout.Get());  // Used
    EXPECT_EQ(pipeline.GetBindGroupLayout(1).Get(), emptyBindGroupLayout.Get());  // Not Used.
    EXPECT_NE(pipeline.GetBindGroupLayout(2).Get(), emptyBindGroupLayout.Get());  // Used.
    EXPECT_EQ(pipeline.GetBindGroupLayout(3).Get(), emptyBindGroupLayout.Get());  // Not used
}

// Test that after explicitly creating a pipeline with a pipeline layout, calling
// GetBindGroupLayout reflects the same bind group layouts.
TEST_F(GetBindGroupLayoutTests, Reflection) {
    wgpu::BindGroupLayoutEntry binding = {};
    binding.binding = 0;
    binding.type = wgpu::BindingType::UniformBuffer;
    binding.visibility = wgpu::ShaderStage::Vertex;

    wgpu::BindGroupLayoutDescriptor bglDesc = {};
    bglDesc.entryCount = 1;
    bglDesc.entries = &binding;

    wgpu::BindGroupLayout bindGroupLayout = device.CreateBindGroupLayout(&bglDesc);

    wgpu::PipelineLayoutDescriptor pipelineLayoutDesc = {};
    pipelineLayoutDesc.bindGroupLayoutCount = 1;
    pipelineLayoutDesc.bindGroupLayouts = &bindGroupLayout;

    wgpu::PipelineLayout pipelineLayout = device.CreatePipelineLayout(&pipelineLayoutDesc);

    wgpu::ShaderModule vsModule =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
        #version 450
        layout(set = 0, binding = 0) uniform Buffer1 {
            vec4 pos1;
        };

        void main() {
        })");

    wgpu::ShaderModule fsModule =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, R"(
        #version 450
        void main() {
        })");

    utils::ComboRenderPipelineDescriptor pipelineDesc(device);
    pipelineDesc.layout = pipelineLayout;
    pipelineDesc.vertexStage.module = vsModule;
    pipelineDesc.cFragmentStage.module = fsModule;

    wgpu::RenderPipeline pipeline = device.CreateRenderPipeline(&pipelineDesc);

    EXPECT_EQ(pipeline.GetBindGroupLayout(0).Get(), bindGroupLayout.Get());

    {
        wgpu::BindGroupLayoutDescriptor emptyDesc = {};
        emptyDesc.entryCount = 0;
        emptyDesc.entries = nullptr;

        wgpu::BindGroupLayout emptyBindGroupLayout = device.CreateBindGroupLayout(&emptyDesc);

        // Check that the rest of the bind group layouts reflect the empty one.
        EXPECT_EQ(pipeline.GetBindGroupLayout(1).Get(), emptyBindGroupLayout.Get());
        EXPECT_EQ(pipeline.GetBindGroupLayout(2).Get(), emptyBindGroupLayout.Get());
        EXPECT_EQ(pipeline.GetBindGroupLayout(3).Get(), emptyBindGroupLayout.Get());
    }
}
