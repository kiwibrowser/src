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

#include "tests/DawnTest.h"

#include "common/Assert.h"
#include "utils/ComboRenderPipelineDescriptor.h"
#include "utils/DawnHelpers.h"

using dawn::InputStepMode;
using dawn::VertexFormat;

// Input state tests all work the same way: the test will render triangles in a grid up to 4x4. Each triangle
// is position in the grid such that X will correspond to the "triangle number" and the Y to the instance number.
// Each test will set up an input state and buffers, and the vertex shader will check that the vertex attributes
// corresponds to predetermined values. On success it outputs green, otherwise red.
//
// The predetermined values are "K * gl_VertexID + componentIndex" for vertex-indexed buffers, and
// "K * gl_InstanceID + componentIndex" for instance-indexed buffers.

constexpr static unsigned int kRTSize = 400;
constexpr static unsigned int kRTCellOffset = 50;
constexpr static unsigned int kRTCellSize = 100;

class VertexInputTest : public DawnTest {
  protected:
    void SetUp() override {
        DawnTest::SetUp();

        renderPass = utils::CreateBasicRenderPass(device, kRTSize, kRTSize);
    }

    bool ShouldComponentBeDefault(VertexFormat format, int component) {
        EXPECT_TRUE(component >= 0 && component < 4);
        switch (format) {
            case VertexFormat::Float4:
            case VertexFormat::UChar4Norm:
                return component >= 4;
            case VertexFormat::Float3:
                return component >= 3;
            case VertexFormat::Float2:
            case VertexFormat::UChar2Norm:
                return component >= 2;
            case VertexFormat::Float:
                return component >= 1;
            default:
                DAWN_UNREACHABLE();
        }
    }

    struct ShaderTestSpec {
        uint32_t location;
        VertexFormat format;
        InputStepMode step;
    };
    dawn::RenderPipeline MakeTestPipeline(const dawn::VertexInputDescriptor& vertexInput,
                                          int multiplier,
                                          const std::vector<ShaderTestSpec>& testSpec) {
        std::ostringstream vs;
        vs << "#version 450\n";

        // TODO(cwallez@chromium.org): this only handles float attributes, we should extend it to
        // other types Adds line of the form
        //    layout(location=1) in vec4 input1;
        for (const auto& input : testSpec) {
            vs << "layout(location=" << input.location << ") in vec4 input" << input.location
               << ";\n";
        }

        vs << "layout(location = 0) out vec4 color;\n";
        vs << "void main() {\n";

        // Hard code the triangle in the shader so that we don't have to add a vertex input for it.
        // Also this places the triangle in the grid based on its VertexID and InstanceID
        vs << "    const vec2 pos[3] = vec2[3](vec2(0.5f, 1.0f), vec2(0.0f, 0.0f), vec2(1.0f, "
              "0.0f));\n";
        vs << "    vec2 offset = vec2(float(gl_VertexIndex / 3), float(gl_InstanceIndex));\n";
        vs << "    vec2 worldPos = pos[gl_VertexIndex % 3] + offset;\n";
        vs << "    gl_Position = vec4(worldPos / 2 - vec2(1.0f), 0.0f, 1.0f);\n";

        // Perform the checks by successively ANDing a boolean
        vs << "    bool success = true;\n";
        for (const auto& input : testSpec) {
            for (int component = 0; component < 4; ++component) {
                vs << "    success = success && (input" << input.location << "[" << component
                   << "] == ";
                if (ShouldComponentBeDefault(input.format, component)) {
                    vs << (component == 3 ? "1.0f" : "0.0f");
                } else {
                    if (input.step == InputStepMode::Vertex) {
                        vs << multiplier << " * gl_VertexIndex + " << component << ".0f";
                    } else {
                        vs << multiplier << " * gl_InstanceIndex + " << component << ".0f";
                    }
                }
                vs << ");\n";
            }
        }

        // Choose the color
        vs << "    if (success) {\n";
        vs << "        color = vec4(0.0f, 1.0f, 0.0f, 1.0f);\n";
        vs << "    } else {\n";
        vs << "        color = vec4(1.0f, 0.0f, 0.0f, 1.0f);\n";
        vs << "    }\n;";
        vs << "}\n";

        dawn::ShaderModule vsModule =
            utils::CreateShaderModule(device, dawn::ShaderStage::Vertex, vs.str().c_str());
        dawn::ShaderModule fsModule =
            utils::CreateShaderModule(device, dawn::ShaderStage::Fragment, R"(
                #version 450
                layout(location = 0) in vec4 color;
                layout(location = 0) out vec4 fragColor;
                void main() {
                    fragColor = color;
                })");

        utils::ComboRenderPipelineDescriptor descriptor(device);
        descriptor.cVertexStage.module = vsModule;
        descriptor.cFragmentStage.module = fsModule;
        descriptor.vertexInput = &vertexInput;
        descriptor.cColorStates[0]->format = renderPass.colorFormat;

        return device.CreateRenderPipeline(&descriptor);
    }

    struct VertexAttributeSpec {
        uint32_t location;
        uint64_t offset;
        VertexFormat format;
    };
    struct VertexBufferSpec {
        uint64_t stride;
        InputStepMode step;
        std::vector<VertexAttributeSpec> attributes;
    };

    utils::ComboVertexInputDescriptor MakeVertexInput(
        const std::vector<VertexBufferSpec>& buffers) {
        utils::ComboVertexInputDescriptor vertexInput;
        uint32_t bufferCount = 0;
        uint32_t totalNumAttributes = 0;
        for (const VertexBufferSpec& buffer : buffers) {
            vertexInput.cBuffers[bufferCount].stride = buffer.stride;
            vertexInput.cBuffers[bufferCount].stepMode = buffer.step;

            vertexInput.cBuffers[bufferCount].attributes =
                &vertexInput.cAttributes[totalNumAttributes];

            for (const VertexAttributeSpec& attribute : buffer.attributes) {
                vertexInput.cAttributes[totalNumAttributes].shaderLocation = attribute.location;
                vertexInput.cAttributes[totalNumAttributes].offset = attribute.offset;
                vertexInput.cAttributes[totalNumAttributes].format = attribute.format;
                totalNumAttributes++;
            }
            vertexInput.cBuffers[bufferCount].attributeCount =
                static_cast<uint32_t>(buffer.attributes.size());

            bufferCount++;
        }

        vertexInput.bufferCount = bufferCount;
        return vertexInput;
    }

    template <typename T>
    dawn::Buffer MakeVertexBuffer(std::vector<T> data) {
        return utils::CreateBufferFromData(device, data.data(),
                                           static_cast<uint32_t>(data.size() * sizeof(T)),
                                           dawn::BufferUsageBit::Vertex);
    }

    struct DrawVertexBuffer {
        uint32_t location;
        dawn::Buffer* buffer;
    };
    void DoTestDraw(const dawn::RenderPipeline& pipeline,
                    unsigned int triangles,
                    unsigned int instances,
                    std::vector<DrawVertexBuffer> vertexBuffers) {
        EXPECT_LE(triangles, 4u);
        EXPECT_LE(instances, 4u);

        dawn::CommandEncoder encoder = device.CreateCommandEncoder();

        dawn::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
        pass.SetPipeline(pipeline);

        uint64_t zeroOffset = 0;
        for (const auto& buffer : vertexBuffers) {
            pass.SetVertexBuffers(buffer.location, 1, buffer.buffer, &zeroOffset);
        }

        pass.Draw(triangles * 3, instances, 0, 0);
        pass.EndPass();

        dawn::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);

        CheckResult(triangles, instances);
    }

    void CheckResult(unsigned int triangles, unsigned int instances) {
        // Check that the center of each triangle is pure green, so that if a single vertex shader
        // instance fails, linear interpolation makes the pixel check fail.
        for (unsigned int triangle = 0; triangle < 4; triangle++) {
            for (unsigned int instance = 0; instance < 4; instance++) {
                unsigned int x = kRTCellOffset + kRTCellSize * triangle;
                unsigned int y = kRTCellOffset + kRTCellSize * instance;
                if (triangle < triangles && instance < instances) {
                    EXPECT_PIXEL_RGBA8_EQ(RGBA8(0, 255, 0, 255), renderPass.color, x, y);
                } else {
                    EXPECT_PIXEL_RGBA8_EQ(RGBA8(0, 0, 0, 0), renderPass.color, x, y);
                }
            }
        }
    }

    utils::BasicRenderPass renderPass;
};

// Test compilation and usage of the fixture :)
TEST_P(VertexInputTest, Basic) {
    utils::ComboVertexInputDescriptor vertexInput = MakeVertexInput(
        {{4 * sizeof(float), InputStepMode::Vertex, {{0, 0, VertexFormat::Float4}}}});
    dawn::RenderPipeline pipeline =
        MakeTestPipeline(vertexInput, 1, {{0, VertexFormat::Float4, InputStepMode::Vertex}});

    dawn::Buffer buffer0 = MakeVertexBuffer<float>({
        0, 1, 2, 3,
        1, 2, 3, 4,
        2, 3, 4, 5
    });
    DoTestDraw(pipeline, 1, 1, {DrawVertexBuffer{0, &buffer0}});
}

// Test a stride of 0 works
TEST_P(VertexInputTest, ZeroStride) {
    // This test was failing only on AMD but the OpenGL backend doesn't gather PCI info yet.
    DAWN_SKIP_TEST_IF(IsLinux() && IsOpenGL());

    utils::ComboVertexInputDescriptor vertexInput =
        MakeVertexInput({{0, InputStepMode::Vertex, {{0, 0, VertexFormat::Float4}}}});
    dawn::RenderPipeline pipeline =
        MakeTestPipeline(vertexInput, 0, {{0, VertexFormat::Float4, InputStepMode::Vertex}});

    dawn::Buffer buffer0 = MakeVertexBuffer<float>({
        0, 1, 2, 3,
    });
    DoTestDraw(pipeline, 1, 1, {DrawVertexBuffer{0, &buffer0}});
}

// Test attributes defaults to (0, 0, 0, 1) if the input state doesn't have all components
TEST_P(VertexInputTest, AttributeExpanding) {
    // This test was failing only on AMD but the OpenGL backend doesn't gather PCI info yet.
    DAWN_SKIP_TEST_IF(IsLinux() && IsOpenGL());

    // R32F case
    {
        utils::ComboVertexInputDescriptor vertexInput =
            MakeVertexInput({{0, InputStepMode::Vertex, {{0, 0, VertexFormat::Float}}}});
        dawn::RenderPipeline pipeline =
            MakeTestPipeline(vertexInput, 0, {{0, VertexFormat::Float, InputStepMode::Vertex}});

        dawn::Buffer buffer0 = MakeVertexBuffer<float>({
            0, 1, 2, 3
        });
        DoTestDraw(pipeline, 1, 1, {DrawVertexBuffer{0, &buffer0}});
    }
    // RG32F case
    {
        utils::ComboVertexInputDescriptor vertexInput =
            MakeVertexInput({{0, InputStepMode::Vertex, {{0, 0, VertexFormat::Float2}}}});
        dawn::RenderPipeline pipeline =
            MakeTestPipeline(vertexInput, 0, {{0, VertexFormat::Float2, InputStepMode::Vertex}});

        dawn::Buffer buffer0 = MakeVertexBuffer<float>({
            0, 1, 2, 3
        });
        DoTestDraw(pipeline, 1, 1, {DrawVertexBuffer{0, &buffer0}});
    }
    // RGB32F case
    {
        utils::ComboVertexInputDescriptor vertexInput =
            MakeVertexInput({{0, InputStepMode::Vertex, {{0, 0, VertexFormat::Float3}}}});
        dawn::RenderPipeline pipeline =
            MakeTestPipeline(vertexInput, 0, {{0, VertexFormat::Float3, InputStepMode::Vertex}});

        dawn::Buffer buffer0 = MakeVertexBuffer<float>({
            0, 1, 2, 3
        });
        DoTestDraw(pipeline, 1, 1, {DrawVertexBuffer{0, &buffer0}});
    }
}

// Test a stride larger than the attributes
TEST_P(VertexInputTest, StrideLargerThanAttributes) {
    // This test was failing only on AMD but the OpenGL backend doesn't gather PCI info yet.
    DAWN_SKIP_TEST_IF(IsLinux() && IsOpenGL());

    utils::ComboVertexInputDescriptor vertexInput = MakeVertexInput(
        {{8 * sizeof(float), InputStepMode::Vertex, {{0, 0, VertexFormat::Float4}}}});
    dawn::RenderPipeline pipeline =
        MakeTestPipeline(vertexInput, 1, {{0, VertexFormat::Float4, InputStepMode::Vertex}});

    dawn::Buffer buffer0 = MakeVertexBuffer<float>({
        0, 1, 2, 3, 0, 0, 0, 0,
        1, 2, 3, 4, 0, 0, 0, 0,
        2, 3, 4, 5, 0, 0, 0, 0,
    });
    DoTestDraw(pipeline, 1, 1, {DrawVertexBuffer{0, &buffer0}});
}

// Test two attributes at an offset, vertex version
TEST_P(VertexInputTest, TwoAttributesAtAnOffsetVertex) {
    utils::ComboVertexInputDescriptor vertexInput = MakeVertexInput(
        {{8 * sizeof(float),
          InputStepMode::Vertex,
          {{0, 0, VertexFormat::Float4}, {1, 4 * sizeof(float), VertexFormat::Float4}}}});
    dawn::RenderPipeline pipeline =
        MakeTestPipeline(vertexInput, 1, {{0, VertexFormat::Float4, InputStepMode::Vertex}});

    dawn::Buffer buffer0 = MakeVertexBuffer<float>({
        0, 1, 2, 3, 0, 1, 2, 3,
        1, 2, 3, 4, 1, 2, 3, 4,
        2, 3, 4, 5, 2, 3, 4, 5,
    });
    DoTestDraw(pipeline, 1, 1, {DrawVertexBuffer{0, &buffer0}});
}

// Test two attributes at an offset, instance version
TEST_P(VertexInputTest, TwoAttributesAtAnOffsetInstance) {
    utils::ComboVertexInputDescriptor vertexInput = MakeVertexInput(
        {{8 * sizeof(float),
          InputStepMode::Instance,
          {{0, 0, VertexFormat::Float4}, {1, 4 * sizeof(float), VertexFormat::Float4}}}});
    dawn::RenderPipeline pipeline =
        MakeTestPipeline(vertexInput, 1, {{0, VertexFormat::Float4, InputStepMode::Instance}});

    dawn::Buffer buffer0 = MakeVertexBuffer<float>({
        0, 1, 2, 3, 0, 1, 2, 3,
        1, 2, 3, 4, 1, 2, 3, 4,
        2, 3, 4, 5, 2, 3, 4, 5,
    });
    DoTestDraw(pipeline, 1, 1, {DrawVertexBuffer{0, &buffer0}});
}

// Test a pure-instance input state
TEST_P(VertexInputTest, PureInstance) {
    utils::ComboVertexInputDescriptor vertexInput = MakeVertexInput(
        {{4 * sizeof(float), InputStepMode::Instance, {{0, 0, VertexFormat::Float4}}}});
    dawn::RenderPipeline pipeline =
        MakeTestPipeline(vertexInput, 1, {{0, VertexFormat::Float4, InputStepMode::Instance}});

    dawn::Buffer buffer0 = MakeVertexBuffer<float>({
        0, 1, 2, 3,
        1, 2, 3, 4,
        2, 3, 4, 5,
        3, 4, 5, 6,
    });
    DoTestDraw(pipeline, 1, 4, {DrawVertexBuffer{0, &buffer0}});
}

// Test with mixed everything, vertex vs. instance, different stride and offsets
// different attribute types
TEST_P(VertexInputTest, MixedEverything) {
    utils::ComboVertexInputDescriptor vertexInput = MakeVertexInput(
        {{12 * sizeof(float),
          InputStepMode::Vertex,
          {{0, 0, VertexFormat::Float}, {1, 6 * sizeof(float), VertexFormat::Float2}}},
         {10 * sizeof(float),
          InputStepMode::Instance,
          {{2, 0, VertexFormat::Float3}, {3, 5 * sizeof(float), VertexFormat::Float4}}}});
    dawn::RenderPipeline pipeline =
        MakeTestPipeline(vertexInput, 1,
                         {{0, VertexFormat::Float, InputStepMode::Vertex},
                          {1, VertexFormat::Float2, InputStepMode::Vertex},
                          {2, VertexFormat::Float3, InputStepMode::Instance},
                          {3, VertexFormat::Float4, InputStepMode::Instance}});

    dawn::Buffer buffer0 = MakeVertexBuffer<float>({
        0, 1, 2, 3, 0, 0, 0, 1, 2, 3, 0, 0,
        1, 2, 3, 4, 0, 0, 1, 2, 3, 4, 0, 0,
        2, 3, 4, 5, 0, 0, 2, 3, 4, 5, 0, 0,
        3, 4, 5, 6, 0, 0, 3, 4, 5, 6, 0, 0,
    });
    dawn::Buffer buffer1 = MakeVertexBuffer<float>({
        0, 1, 2, 3, 0, 0, 1, 2, 3, 0,
        1, 2, 3, 4, 0, 1, 2, 3, 4, 0,
        2, 3, 4, 5, 0, 2, 3, 4, 5, 0,
        3, 4, 5, 6, 0, 3, 4, 5, 6, 0,
    });
    DoTestDraw(pipeline, 1, 1, {{0, &buffer0}, {1, &buffer1}});
}

// Test input state is unaffected by unused vertex slot
TEST_P(VertexInputTest, UnusedVertexSlot) {
    // Instance input state, using slot 1
    utils::ComboVertexInputDescriptor instanceVertexInput = MakeVertexInput(
        {{0, InputStepMode::Vertex, {}},
         {4 * sizeof(float), InputStepMode::Instance, {{0, 0, VertexFormat::Float4}}}});
    dawn::RenderPipeline instancePipeline = MakeTestPipeline(
        instanceVertexInput, 1, {{0, VertexFormat::Float4, InputStepMode::Instance}});

    dawn::Buffer buffer = MakeVertexBuffer<float>({
        0, 1, 2, 3,
        1, 2, 3, 4,
        2, 3, 4, 5,
        3, 4, 5, 6,
    });

    dawn::CommandEncoder encoder = device.CreateCommandEncoder();

    dawn::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);

    uint64_t zeroOffset = 0;
    pass.SetVertexBuffers(0, 1, &buffer, &zeroOffset);
    pass.SetVertexBuffers(1, 1, &buffer, &zeroOffset);

    pass.SetPipeline(instancePipeline);
    pass.Draw(1 * 3, 4, 0, 0);

    pass.EndPass();

    dawn::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    CheckResult(1, 4);
}

// Test setting a different pipeline with a different input state.
// This was a problem with the D3D12 backend where SetVertexBuffers
// was getting the input from the last set pipeline, not the current.
// SetVertexBuffers should be reapplied when the input state changes.
TEST_P(VertexInputTest, MultiplePipelinesMixedVertexInput) {
    // Basic input state, using slot 0
    utils::ComboVertexInputDescriptor vertexVertexInput = MakeVertexInput(
        {{4 * sizeof(float), InputStepMode::Vertex, {{0, 0, VertexFormat::Float4}}}});
    dawn::RenderPipeline vertexPipeline =
        MakeTestPipeline(vertexVertexInput, 1, {{0, VertexFormat::Float4, InputStepMode::Vertex}});

    // Instance input state, using slot 1
    utils::ComboVertexInputDescriptor instanceVertexInput = MakeVertexInput(
        {{0, InputStepMode::Instance, {}},
         {4 * sizeof(float), InputStepMode::Instance, {{0, 0, VertexFormat::Float4}}}});
    dawn::RenderPipeline instancePipeline = MakeTestPipeline(
        instanceVertexInput, 1, {{0, VertexFormat::Float4, InputStepMode::Instance}});

    dawn::Buffer buffer = MakeVertexBuffer<float>({
        0, 1, 2, 3,
        1, 2, 3, 4,
        2, 3, 4, 5,
        3, 4, 5, 6,
    });

    dawn::CommandEncoder encoder = device.CreateCommandEncoder();

    dawn::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);

    uint64_t zeroOffset = 0;
    pass.SetVertexBuffers(0, 1, &buffer, &zeroOffset);
    pass.SetVertexBuffers(1, 1, &buffer, &zeroOffset);

    pass.SetPipeline(vertexPipeline);
    pass.Draw(1 * 3, 1, 0, 0);

    pass.SetPipeline(instancePipeline);
    pass.Draw(1 * 3, 4, 0, 0);

    pass.EndPass();

    dawn::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    CheckResult(1, 4);
}

DAWN_INSTANTIATE_TEST(VertexInputTest, D3D12Backend, MetalBackend, OpenGLBackend, VulkanBackend);

// TODO for the input state:
//  - Add more vertex formats
//  - Add checks that the stride is enough to contain all attributes
//  - Add checks stride less than some limit
//  - Add checks for alignement of vertex buffers and attributes if needed
//  - Check for attribute narrowing
//  - Check that the input state and the pipeline vertex input types match
