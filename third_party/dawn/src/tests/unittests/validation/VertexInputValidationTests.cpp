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
#include "utils/DawnHelpers.h"

class VertexInputTest : public ValidationTest {
  protected:
    void CreatePipeline(bool success,
                        const utils::ComboVertexInputDescriptor& state,
                        std::string vertexSource) {
        dawn::ShaderModule vsModule =
            utils::CreateShaderModule(device, dawn::ShaderStage::Vertex, vertexSource.c_str());
        dawn::ShaderModule fsModule =
            utils::CreateShaderModule(device, dawn::ShaderStage::Fragment, R"(
                #version 450
                layout(location = 0) out vec4 fragColor;
                void main() {
                    fragColor = vec4(1.0, 0.0, 0.0, 1.0);
                }
            )");

        utils::ComboRenderPipelineDescriptor descriptor(device);
        descriptor.cVertexStage.module = vsModule;
        descriptor.cFragmentStage.module = fsModule;
        descriptor.vertexInput = &state;
        descriptor.cColorStates[0]->format = dawn::TextureFormat::R8G8B8A8Unorm;

        if (!success) {
            ASSERT_DEVICE_ERROR(device.CreateRenderPipeline(&descriptor));
        } else {
            device.CreateRenderPipeline(&descriptor);
        }
    }
};

// Check an empty vertex input is valid
TEST_F(VertexInputTest, EmptyIsOk) {
    utils::ComboVertexInputDescriptor state;
    CreatePipeline(true, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");
}

// Check null buffer is valid
TEST_F(VertexInputTest, NullBufferIsOk) {
    utils::ComboVertexInputDescriptor state;
    // One null buffer (buffer[0]) is OK
    state.bufferCount = 1;
    state.cBuffers[0].stride = 0;
    state.cBuffers[0].attributeCount = 0;
    state.cBuffers[0].attributes = nullptr;
    CreatePipeline(true, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");

    // One null buffer (buffer[0]) followed by a buffer (buffer[1]) is OK
    state.bufferCount = 2;
    state.cBuffers[1].stride = 0;
    state.cBuffers[1].attributeCount = 1;
    state.cBuffers[1].attributes = &state.cAttributes[0];
    state.cAttributes[0].shaderLocation = 0;
    CreatePipeline(true, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");

    // Null buffer (buffer[2]) sitting between buffers (buffer[1] and buffer[3]) is OK
    state.bufferCount = 4;
    state.cBuffers[2].attributeCount = 0;
    state.cBuffers[2].attributes = nullptr;
    state.cBuffers[3].attributeCount = 1;
    state.cBuffers[3].attributes = &state.cAttributes[1];
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
TEST_F(VertexInputTest, PipelineCompatibility) {
    utils::ComboVertexInputDescriptor state;
    state.bufferCount = 1;
    state.cBuffers[0].stride = 2 * sizeof(float);
    state.cBuffers[0].attributeCount = 2;
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

    // Check it is valid for the pipeline to use a subset of the VertexInput
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

// Test that a stride of 0 is valid
TEST_F(VertexInputTest, StrideZero) {
    // Works ok without attributes
    utils::ComboVertexInputDescriptor state;
    state.bufferCount = 1;
    state.cBuffers[0].stride = 0;
    state.cBuffers[0].attributeCount = 1;
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

// Check validation that vertex attribute offset should be within vertex buffer stride,
// if vertex buffer stride is not zero.
TEST_F(VertexInputTest, SetOffsetOutOfBounds) {
    // Control case, setting correct stride and offset
    utils::ComboVertexInputDescriptor state;
    state.bufferCount = 1;
    state.cBuffers[0].stride = 2 * sizeof(float);
    state.cBuffers[0].attributeCount = 2;
    state.cAttributes[0].shaderLocation = 0;
    state.cAttributes[1].shaderLocation = 1;
    state.cAttributes[1].offset = sizeof(float);
    CreatePipeline(true, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");

    // Test vertex attribute offset exceed vertex buffer stride range
    state.cBuffers[0].stride = sizeof(float);
    CreatePipeline(false, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");

    // It's OK if stride is zero
    state.cBuffers[0].stride = 0;
    CreatePipeline(true, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");
}

// Check out of bounds condition on total number of vertex buffers
TEST_F(VertexInputTest, SetVertexBuffersNumLimit) {
    // Control case, setting max vertex buffer number
    utils::ComboVertexInputDescriptor state;
    state.bufferCount = kMaxVertexBuffers;
    for (uint32_t i = 0; i < kMaxVertexBuffers; ++i) {
        state.cBuffers[i].attributeCount = 1;
        state.cBuffers[i].attributes = &state.cAttributes[i];
        state.cAttributes[i].shaderLocation = i;
    }
    CreatePipeline(true, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");

    // Test vertex buffer number exceed the limit
    state.bufferCount = kMaxVertexBuffers + 1;
    CreatePipeline(false, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");
}

// Check out of bounds condition on total number of vertex attributes
TEST_F(VertexInputTest, SetVertexAttributesNumLimit) {
    // Control case, setting max vertex attribute number
    utils::ComboVertexInputDescriptor state;
    state.bufferCount = 2;
    state.cBuffers[0].attributeCount = kMaxVertexAttributes;
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
    state.cBuffers[1].attributeCount = 1;
    state.cBuffers[1].attributes = &state.cAttributes[kMaxVertexAttributes - 1];
    CreatePipeline(false, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");
}

// Check out of bounds condition on input stride
TEST_F(VertexInputTest, SetInputStrideOutOfBounds) {
    // Control case, setting max input stride
    utils::ComboVertexInputDescriptor state;
    state.bufferCount = 1;
    state.cBuffers[0].stride = kMaxVertexBufferStride;
    state.cBuffers[0].attributeCount = 1;
    CreatePipeline(true, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");

    // Test input stride OOB
    state.cBuffers[0].stride = kMaxVertexBufferStride + 1;
    CreatePipeline(false, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");
}

// Test that we cannot set an already set attribute
TEST_F(VertexInputTest, AlreadySetAttribute) {
    // Control case, setting attribute 0
    utils::ComboVertexInputDescriptor state;
    state.bufferCount = 1;
    state.cBuffers[0].attributeCount = 1;
    state.cAttributes[0].shaderLocation = 0;
    CreatePipeline(true, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");

    // Oh no, attribute 0 is set twice
    state.cBuffers[0].attributeCount = 2;
    state.cAttributes[0].shaderLocation = 0;
    state.cAttributes[1].shaderLocation = 0;
    CreatePipeline(false, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");
}

// Test that a stride of 0 is valid
TEST_F(VertexInputTest, SetSameShaderLocation) {
    // Control case, setting different shader locations in two attributes
    utils::ComboVertexInputDescriptor state;
    state.bufferCount = 1;
    state.cBuffers[0].attributeCount = 2;
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
    state.bufferCount = 2;
    state.cBuffers[0].attributeCount = 1;
    state.cAttributes[0].shaderLocation = 0;
    state.cBuffers[1].attributeCount = 1;
    state.cBuffers[1].attributes = &state.cAttributes[1];
    state.cAttributes[1].shaderLocation = 0;
    CreatePipeline(false, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");
}

// Check out of bounds condition on attribute shader location
TEST_F(VertexInputTest, SetAttributeLocationOutOfBounds) {
    // Control case, setting last attribute shader location
    utils::ComboVertexInputDescriptor state;
    state.bufferCount = 1;
    state.cBuffers[0].attributeCount = 1;
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
TEST_F(VertexInputTest, SetAttributeOffsetOutOfBounds) {
    // Control case, setting max attribute offset for FloatR32 vertex format
    utils::ComboVertexInputDescriptor state;
    state.bufferCount = 1;
    state.cBuffers[0].attributeCount = 1;
    state.cAttributes[0].offset = kMaxVertexAttributeEnd - sizeof(dawn::VertexFormat::Float);
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

// Check attribute offset overflow
TEST_F(VertexInputTest, SetAttributeOffsetOverflow) {
    utils::ComboVertexInputDescriptor state;
    state.bufferCount = 1;
    state.cBuffers[0].attributeCount = 1;
    state.cAttributes[0].offset = std::numeric_limits<uint32_t>::max();
    CreatePipeline(false, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");
}

// Check for some potential underflow in the vertex input validation
TEST_F(VertexInputTest, VertexFormatLargerThanNonZeroStride) {
    utils::ComboVertexInputDescriptor state;
    state.bufferCount = 1;
    state.cBuffers[0].stride = 4;
    state.cBuffers[0].attributeCount = 1;
    state.cAttributes[0].format = dawn::VertexFormat::Float4;
    CreatePipeline(false, state, R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0);
        }
    )");
}
