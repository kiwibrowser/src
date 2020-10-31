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

#include "utils/ComboRenderPipelineDescriptor.h"
#include "utils/WGPUHelpers.h"

class VertexStateTest : public ValidationTest {
  protected:
    void CreatePipeline(bool success,
                        const utils::ComboVertexStateDescriptor& state,
                        std::string vertexSource) {
        wgpu::ShaderModule vsModule = utils::CreateShaderModule(
            device, utils::SingleShaderStage::Vertex, vertexSource.c_str());
        wgpu::ShaderModule fsModule =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, R"(
                #version 450
                layout(location = 0) out vec4 fragColor;
                void main() {
                    fragColor = vec4(1.0, 0.0, 0.0, 1.0);
                }
            )");

        utils::ComboRenderPipelineDescriptor descriptor(device);
        descriptor.vertexStage.module = vsModule;
        descriptor.cFragmentStage.module = fsModule;
        descriptor.vertexState = &state;
        descriptor.cColorStates[0].format = wgpu::TextureFormat::RGBA8Unorm;

        if (!success) {
            ASSERT_DEVICE_ERROR(device.CreateRenderPipeline(&descriptor));
        } else {
            device.CreateRenderPipeline(&descriptor);
        }
    }
};

// Check an empty vertex input is valid
TEST_F(VertexStateTest, EmptyIsOk) {
    utils::ComboVertexStateDescriptor state;
    CreatePipeline(true, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");
}

// Check null buffer is valid
TEST_F(VertexStateTest, NullBufferIsOk) {
    utils::ComboVertexStateDescriptor state;
    // One null buffer (buffer[0]) is OK
    state.vertexBufferCount = 1;
    state.cVertexBuffers[0].arrayStride = 0;
    state.cVertexBuffers[0].attributeCount = 0;
    state.cVertexBuffers[0].attributes = nullptr;
    CreatePipeline(true, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");

    // One null buffer (buffer[0]) followed by a buffer (buffer[1]) is OK
    state.vertexBufferCount = 2;
    state.cVertexBuffers[1].arrayStride = 0;
    state.cVertexBuffers[1].attributeCount = 1;
    state.cVertexBuffers[1].attributes = &state.cAttributes[0];
    state.cAttributes[0].shaderLocation = 0;
    CreatePipeline(true, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");

    // Null buffer (buffer[2]) sitting between buffers (buffer[1] and buffer[3]) is OK
    state.vertexBufferCount = 4;
    state.cVertexBuffers[2].attributeCount = 0;
    state.cVertexBuffers[2].attributes = nullptr;
    state.cVertexBuffers[3].attributeCount = 1;
    state.cVertexBuffers[3].attributes = &state.cAttributes[1];
    state.cAttributes[1].shaderLocation = 1;
    CreatePipeline(true, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");
}

// Check validation that pipeline vertex buffers are backed by attributes in the vertex input
// Check validation that pipeline vertex buffers are backed by attributes in the vertex input
TEST_F(VertexStateTest, PipelineCompatibility) {
    utils::ComboVertexStateDescriptor state;
    state.vertexBufferCount = 1;
    state.cVertexBuffers[0].arrayStride = 2 * sizeof(float);
    state.cVertexBuffers[0].attributeCount = 2;
    state.cAttributes[0].shaderLocation = 0;
    state.cAttributes[1].shaderLocation = 1;
    state.cAttributes[1].offset = sizeof(float);

    // Control case: pipeline with one input per attribute
    CreatePipeline(true, state, R"(
        #version 450
        layout(location = 0) in vec4 a;
        layout(location = 1) in vec4 b;
        void main() {
            gl_Position = vec4(0.0);
        }
    )");

    // Check it is valid for the pipeline to use a subset of the VertexState
    CreatePipeline(true, state, R"(
        #version 450
        layout(location = 0) in vec4 a;
        void main() {
            gl_Position = vec4(0.0);
        }
    )");

    // Check for an error when the pipeline uses an attribute not in the vertex input
    CreatePipeline(false, state, R"(
        #version 450
        layout(location = 2) in vec4 a;
        void main() {
            gl_Position = vec4(0.0);
        }
    )");
}

// Test that a arrayStride of 0 is valid
TEST_F(VertexStateTest, StrideZero) {
    // Works ok without attributes
    utils::ComboVertexStateDescriptor state;
    state.vertexBufferCount = 1;
    state.cVertexBuffers[0].arrayStride = 0;
    state.cVertexBuffers[0].attributeCount = 1;
    CreatePipeline(true, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");

    // Works ok with attributes at a large-ish offset
    state.cAttributes[0].offset = 128;
    CreatePipeline(true, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");
}

// Check validation that vertex attribute offset should be within vertex buffer arrayStride,
// if vertex buffer arrayStride is not zero.
TEST_F(VertexStateTest, SetOffsetOutOfBounds) {
    // Control case, setting correct arrayStride and offset
    utils::ComboVertexStateDescriptor state;
    state.vertexBufferCount = 1;
    state.cVertexBuffers[0].arrayStride = 2 * sizeof(float);
    state.cVertexBuffers[0].attributeCount = 2;
    state.cAttributes[0].shaderLocation = 0;
    state.cAttributes[1].shaderLocation = 1;
    state.cAttributes[1].offset = sizeof(float);
    CreatePipeline(true, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");

    // Test vertex attribute offset exceed vertex buffer arrayStride range
    state.cVertexBuffers[0].arrayStride = sizeof(float);
    CreatePipeline(false, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");

    // It's OK if arrayStride is zero
    state.cVertexBuffers[0].arrayStride = 0;
    CreatePipeline(true, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");
}

// Check out of bounds condition on total number of vertex buffers
TEST_F(VertexStateTest, SetVertexBuffersNumLimit) {
    // Control case, setting max vertex buffer number
    utils::ComboVertexStateDescriptor state;
    state.vertexBufferCount = kMaxVertexBuffers;
    for (uint32_t i = 0; i < kMaxVertexBuffers; ++i) {
        state.cVertexBuffers[i].attributeCount = 1;
        state.cVertexBuffers[i].attributes = &state.cAttributes[i];
        state.cAttributes[i].shaderLocation = i;
    }
    CreatePipeline(true, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");

    // Test vertex buffer number exceed the limit
    state.vertexBufferCount = kMaxVertexBuffers + 1;
    CreatePipeline(false, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");
}

// Check out of bounds condition on total number of vertex attributes
TEST_F(VertexStateTest, SetVertexAttributesNumLimit) {
    // Control case, setting max vertex attribute number
    utils::ComboVertexStateDescriptor state;
    state.vertexBufferCount = 2;
    state.cVertexBuffers[0].attributeCount = kMaxVertexAttributes;
    for (uint32_t i = 0; i < kMaxVertexAttributes; ++i) {
        state.cAttributes[i].shaderLocation = i;
    }
    CreatePipeline(true, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");

    // Test vertex attribute number exceed the limit
    state.cVertexBuffers[1].attributeCount = 1;
    state.cVertexBuffers[1].attributes = &state.cAttributes[kMaxVertexAttributes - 1];
    CreatePipeline(false, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");
}

// Check out of bounds condition on input arrayStride
TEST_F(VertexStateTest, SetInputStrideOutOfBounds) {
    // Control case, setting max input arrayStride
    utils::ComboVertexStateDescriptor state;
    state.vertexBufferCount = 1;
    state.cVertexBuffers[0].arrayStride = kMaxVertexBufferStride;
    state.cVertexBuffers[0].attributeCount = 1;
    CreatePipeline(true, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");

    // Test input arrayStride OOB
    state.cVertexBuffers[0].arrayStride = kMaxVertexBufferStride + 1;
    CreatePipeline(false, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");
}

// Check multiple of 4 bytes constraint on input arrayStride
TEST_F(VertexStateTest, SetInputStrideNotAligned) {
    // Control case, setting input arrayStride 4 bytes.
    utils::ComboVertexStateDescriptor state;
    state.vertexBufferCount = 1;
    state.cVertexBuffers[0].arrayStride = 4;
    state.cVertexBuffers[0].attributeCount = 1;
    CreatePipeline(true, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");

    // Test input arrayStride not multiple of 4 bytes
    state.cVertexBuffers[0].arrayStride = 2;
    CreatePipeline(false, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");
}

// Test that we cannot set an already set attribute
TEST_F(VertexStateTest, AlreadySetAttribute) {
    // Control case, setting attribute 0
    utils::ComboVertexStateDescriptor state;
    state.vertexBufferCount = 1;
    state.cVertexBuffers[0].attributeCount = 1;
    state.cAttributes[0].shaderLocation = 0;
    CreatePipeline(true, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");

    // Oh no, attribute 0 is set twice
    state.cVertexBuffers[0].attributeCount = 2;
    state.cAttributes[0].shaderLocation = 0;
    state.cAttributes[1].shaderLocation = 0;
    CreatePipeline(false, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");
}

// Test that a arrayStride of 0 is valid
TEST_F(VertexStateTest, SetSameShaderLocation) {
    // Control case, setting different shader locations in two attributes
    utils::ComboVertexStateDescriptor state;
    state.vertexBufferCount = 1;
    state.cVertexBuffers[0].attributeCount = 2;
    state.cAttributes[0].shaderLocation = 0;
    state.cAttributes[1].shaderLocation = 1;
    state.cAttributes[1].offset = sizeof(float);
    CreatePipeline(true, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");

    // Test same shader location in two attributes in the same buffer
    state.cAttributes[1].shaderLocation = 0;
    CreatePipeline(false, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");

    // Test same shader location in two attributes in different buffers
    state.vertexBufferCount = 2;
    state.cVertexBuffers[0].attributeCount = 1;
    state.cAttributes[0].shaderLocation = 0;
    state.cVertexBuffers[1].attributeCount = 1;
    state.cVertexBuffers[1].attributes = &state.cAttributes[1];
    state.cAttributes[1].shaderLocation = 0;
    CreatePipeline(false, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");
}

// Check out of bounds condition on attribute shader location
TEST_F(VertexStateTest, SetAttributeLocationOutOfBounds) {
    // Control case, setting last attribute shader location
    utils::ComboVertexStateDescriptor state;
    state.vertexBufferCount = 1;
    state.cVertexBuffers[0].attributeCount = 1;
    state.cAttributes[0].shaderLocation = kMaxVertexAttributes - 1;
    CreatePipeline(true, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");

    // Test attribute location OOB
    state.cAttributes[0].shaderLocation = kMaxVertexAttributes;
    CreatePipeline(false, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");
}

// Check attribute offset out of bounds
TEST_F(VertexStateTest, SetAttributeOffsetOutOfBounds) {
    // Control case, setting max attribute offset for FloatR32 vertex format
    utils::ComboVertexStateDescriptor state;
    state.vertexBufferCount = 1;
    state.cVertexBuffers[0].attributeCount = 1;
    state.cAttributes[0].offset = kMaxVertexAttributeEnd - sizeof(wgpu::VertexFormat::Float);
    CreatePipeline(true, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");

    // Test attribute offset out of bounds
    state.cAttributes[0].offset = kMaxVertexAttributeEnd - 1;
    CreatePipeline(false, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");
}

// Check multiple of 4 bytes constraint on offset
TEST_F(VertexStateTest, SetOffsetNotAligned) {
    // Control case, setting offset 4 bytes.
    utils::ComboVertexStateDescriptor state;
    state.vertexBufferCount = 1;
    state.cVertexBuffers[0].attributeCount = 1;
    state.cAttributes[0].offset = 4;
    CreatePipeline(true, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");

    // Test offset not multiple of 4 bytes
    state.cAttributes[0].offset = 2;
    CreatePipeline(false, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");
}

// Check attribute offset overflow
TEST_F(VertexStateTest, SetAttributeOffsetOverflow) {
    utils::ComboVertexStateDescriptor state;
    state.vertexBufferCount = 1;
    state.cVertexBuffers[0].attributeCount = 1;
    state.cAttributes[0].offset = std::numeric_limits<uint32_t>::max();
    CreatePipeline(false, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");
}

// Check for some potential underflow in the vertex input validation
TEST_F(VertexStateTest, VertexFormatLargerThanNonZeroStride) {
    utils::ComboVertexStateDescriptor state;
    state.vertexBufferCount = 1;
    state.cVertexBuffers[0].arrayStride = 4;
    state.cVertexBuffers[0].attributeCount = 1;
    state.cAttributes[0].format = wgpu::VertexFormat::Float4;
    CreatePipeline(false, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");
}
