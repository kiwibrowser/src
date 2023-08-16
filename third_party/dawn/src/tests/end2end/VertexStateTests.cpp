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
#include "common/Math.h"
#include "utils/ComboRenderPipelineDescriptor.h"
#include "utils/WGPUHelpers.h"

using wgpu::InputStepMode;
using wgpu::VertexFormat;

// Input state tests all work the same way: the test will render triangles in a grid up to 4x4. Each
// triangle is position in the grid such that X will correspond to the "triangle number" and the Y
// to the instance number. Each test will set up an input state and buffers, and the vertex shader
// will check that the vertex attributes corresponds to predetermined values. On success it outputs
// green, otherwise red.
//
// The predetermined values are "K * gl_VertexID + componentIndex" for vertex-indexed buffers, and
// "K * gl_InstanceID + componentIndex" for instance-indexed buffers.

constexpr static unsigned int kRTSize = 400;
constexpr static unsigned int kRTCellOffset = 50;
constexpr static unsigned int kRTCellSize = 100;

class VertexStateTest : public DawnTest {
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
    wgpu::RenderPipeline MakeTestPipeline(const wgpu::VertexStateDescriptor& vertexState,
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
        vs << "    vec4 position = vec4(worldPos / 2 - vec2(1.0f), 0.0f, 1.0f);\n";
        vs << "    gl_Position = vec4(position.x, -position.y, position.z, position.w);\n";

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

        wgpu::ShaderModule vsModule =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, vs.str().c_str());
        wgpu::ShaderModule fsModule =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, R"(
                #version 450
                layout(location = 0) in vec4 color;
                layout(location = 0) out vec4 fragColor;
                void main() {
                    fragColor = color;
                })");

        utils::ComboRenderPipelineDescriptor descriptor(device);
        descriptor.vertexStage.module = vsModule;
        descriptor.cFragmentStage.module = fsModule;
        descriptor.vertexState = &vertexState;
        descriptor.cColorStates[0].format = renderPass.colorFormat;

        return device.CreateRenderPipeline(&descriptor);
    }

    struct VertexAttributeSpec {
        uint32_t location;
        uint64_t offset;
        VertexFormat format;
    };
    struct VertexBufferSpec {
        uint64_t arrayStride;
        InputStepMode step;
        std::vector<VertexAttributeSpec> attributes;
    };

    utils::ComboVertexStateDescriptor MakeVertexState(
        const std::vector<VertexBufferSpec>& buffers) {
        utils::ComboVertexStateDescriptor vertexState;
        uint32_t vertexBufferCount = 0;
        uint32_t totalNumAttributes = 0;
        for (const VertexBufferSpec& buffer : buffers) {
            vertexState.cVertexBuffers[vertexBufferCount].arrayStride = buffer.arrayStride;
            vertexState.cVertexBuffers[vertexBufferCount].stepMode = buffer.step;

            vertexState.cVertexBuffers[vertexBufferCount].attributes =
                &vertexState.cAttributes[totalNumAttributes];

            for (const VertexAttributeSpec& attribute : buffer.attributes) {
                vertexState.cAttributes[totalNumAttributes].shaderLocation = attribute.location;
                vertexState.cAttributes[totalNumAttributes].offset = attribute.offset;
                vertexState.cAttributes[totalNumAttributes].format = attribute.format;
                totalNumAttributes++;
            }
            vertexState.cVertexBuffers[vertexBufferCount].attributeCount =
                static_cast<uint32_t>(buffer.attributes.size());

            vertexBufferCount++;
        }

        vertexState.vertexBufferCount = vertexBufferCount;
        return vertexState;
    }

    template <typename T>
    wgpu::Buffer MakeVertexBuffer(std::vector<T> data) {
        return utils::CreateBufferFromData(device, data.data(),
                                           static_cast<uint32_t>(data.size() * sizeof(T)),
                                           wgpu::BufferUsage::Vertex);
    }

    struct DrawVertexBuffer {
        uint32_t location;
        wgpu::Buffer* buffer;
    };
    void DoTestDraw(const wgpu::RenderPipeline& pipeline,
                    unsigned int triangles,
                    unsigned int instances,
                    std::vector<DrawVertexBuffer> vertexBuffers) {
        EXPECT_LE(triangles, 4u);
        EXPECT_LE(instances, 4u);

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
        pass.SetPipeline(pipeline);

        for (const DrawVertexBuffer& buffer : vertexBuffers) {
            pass.SetVertexBuffer(buffer.location, *buffer.buffer);
        }

        pass.Draw(triangles * 3, instances);
        pass.EndPass();

        wgpu::CommandBuffer commands = encoder.Finish();
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
                    EXPECT_PIXEL_RGBA8_EQ(RGBA8::kGreen, renderPass.color, x, y);
                } else {
                    EXPECT_PIXEL_RGBA8_EQ(RGBA8::kZero, renderPass.color, x, y);
                }
            }
        }
    }

    utils::BasicRenderPass renderPass;
};

// Test compilation and usage of the fixture :)
TEST_P(VertexStateTest, Basic) {
    utils::ComboVertexStateDescriptor vertexState = MakeVertexState(
        {{4 * sizeof(float), InputStepMode::Vertex, {{0, 0, VertexFormat::Float4}}}});
    wgpu::RenderPipeline pipeline =
        MakeTestPipeline(vertexState, 1, {{0, VertexFormat::Float4, InputStepMode::Vertex}});

    // clang-format off
    wgpu::Buffer buffer0 = MakeVertexBuffer<float>({
        0, 1, 2, 3,
        1, 2, 3, 4,
        2, 3, 4, 5
    });
    // clang-format on
    DoTestDraw(pipeline, 1, 1, {DrawVertexBuffer{0, &buffer0}});
}

// Test a stride of 0 works
TEST_P(VertexStateTest, ZeroStride) {
    // This test was failing only on AMD but the OpenGL backend doesn't gather PCI info yet.
    DAWN_SKIP_TEST_IF(IsLinux() && IsOpenGL());

    utils::ComboVertexStateDescriptor vertexState =
        MakeVertexState({{0, InputStepMode::Vertex, {{0, 0, VertexFormat::Float4}}}});
    wgpu::RenderPipeline pipeline =
        MakeTestPipeline(vertexState, 0, {{0, VertexFormat::Float4, InputStepMode::Vertex}});

    wgpu::Buffer buffer0 = MakeVertexBuffer<float>({
        0,
        1,
        2,
        3,
    });
    DoTestDraw(pipeline, 1, 1, {DrawVertexBuffer{0, &buffer0}});
}

// Test attributes defaults to (0, 0, 0, 1) if the input state doesn't have all components
TEST_P(VertexStateTest, AttributeExpanding) {
    // This test was failing only on AMD but the OpenGL backend doesn't gather PCI info yet.
    DAWN_SKIP_TEST_IF(IsLinux() && IsOpenGL());

    // R32F case
    {
        utils::ComboVertexStateDescriptor vertexState =
            MakeVertexState({{0, InputStepMode::Vertex, {{0, 0, VertexFormat::Float}}}});
        wgpu::RenderPipeline pipeline =
            MakeTestPipeline(vertexState, 0, {{0, VertexFormat::Float, InputStepMode::Vertex}});

        wgpu::Buffer buffer0 = MakeVertexBuffer<float>({0, 1, 2, 3});
        DoTestDraw(pipeline, 1, 1, {DrawVertexBuffer{0, &buffer0}});
    }
    // RG32F case
    {
        utils::ComboVertexStateDescriptor vertexState =
            MakeVertexState({{0, InputStepMode::Vertex, {{0, 0, VertexFormat::Float2}}}});
        wgpu::RenderPipeline pipeline =
            MakeTestPipeline(vertexState, 0, {{0, VertexFormat::Float2, InputStepMode::Vertex}});

        wgpu::Buffer buffer0 = MakeVertexBuffer<float>({0, 1, 2, 3});
        DoTestDraw(pipeline, 1, 1, {DrawVertexBuffer{0, &buffer0}});
    }
    // RGB32F case
    {
        utils::ComboVertexStateDescriptor vertexState =
            MakeVertexState({{0, InputStepMode::Vertex, {{0, 0, VertexFormat::Float3}}}});
        wgpu::RenderPipeline pipeline =
            MakeTestPipeline(vertexState, 0, {{0, VertexFormat::Float3, InputStepMode::Vertex}});

        wgpu::Buffer buffer0 = MakeVertexBuffer<float>({0, 1, 2, 3});
        DoTestDraw(pipeline, 1, 1, {DrawVertexBuffer{0, &buffer0}});
    }
}

// Test a stride larger than the attributes
TEST_P(VertexStateTest, StrideLargerThanAttributes) {
    // This test was failing only on AMD but the OpenGL backend doesn't gather PCI info yet.
    DAWN_SKIP_TEST_IF(IsLinux() && IsOpenGL());

    utils::ComboVertexStateDescriptor vertexState = MakeVertexState(
        {{8 * sizeof(float), InputStepMode::Vertex, {{0, 0, VertexFormat::Float4}}}});
    wgpu::RenderPipeline pipeline =
        MakeTestPipeline(vertexState, 1, {{0, VertexFormat::Float4, InputStepMode::Vertex}});

    // clang-format off
    wgpu::Buffer buffer0 = MakeVertexBuffer<float>({
        0, 1, 2, 3, 0, 0, 0, 0,
        1, 2, 3, 4, 0, 0, 0, 0,
        2, 3, 4, 5, 0, 0, 0, 0,
    });
    // clang-format on
    DoTestDraw(pipeline, 1, 1, {DrawVertexBuffer{0, &buffer0}});
}

// Test two attributes at an offset, vertex version
TEST_P(VertexStateTest, TwoAttributesAtAnOffsetVertex) {
    utils::ComboVertexStateDescriptor vertexState = MakeVertexState(
        {{8 * sizeof(float),
          InputStepMode::Vertex,
          {{0, 0, VertexFormat::Float4}, {1, 4 * sizeof(float), VertexFormat::Float4}}}});
    wgpu::RenderPipeline pipeline =
        MakeTestPipeline(vertexState, 1, {{0, VertexFormat::Float4, InputStepMode::Vertex}});

    // clang-format off
    wgpu::Buffer buffer0 = MakeVertexBuffer<float>({
        0, 1, 2, 3, 0, 1, 2, 3,
        1, 2, 3, 4, 1, 2, 3, 4,
        2, 3, 4, 5, 2, 3, 4, 5,
    });
    // clang-format on
    DoTestDraw(pipeline, 1, 1, {DrawVertexBuffer{0, &buffer0}});
}

// Test two attributes at an offset, instance version
TEST_P(VertexStateTest, TwoAttributesAtAnOffsetInstance) {
    utils::ComboVertexStateDescriptor vertexState = MakeVertexState(
        {{8 * sizeof(float),
          InputStepMode::Instance,
          {{0, 0, VertexFormat::Float4}, {1, 4 * sizeof(float), VertexFormat::Float4}}}});
    wgpu::RenderPipeline pipeline =
        MakeTestPipeline(vertexState, 1, {{0, VertexFormat::Float4, InputStepMode::Instance}});

    // clang-format off
    wgpu::Buffer buffer0 = MakeVertexBuffer<float>({
        0, 1, 2, 3, 0, 1, 2, 3,
        1, 2, 3, 4, 1, 2, 3, 4,
        2, 3, 4, 5, 2, 3, 4, 5,
    });
    // clang-format on
    DoTestDraw(pipeline, 1, 1, {DrawVertexBuffer{0, &buffer0}});
}

// Test a pure-instance input state
TEST_P(VertexStateTest, PureInstance) {
    utils::ComboVertexStateDescriptor vertexState = MakeVertexState(
        {{4 * sizeof(float), InputStepMode::Instance, {{0, 0, VertexFormat::Float4}}}});
    wgpu::RenderPipeline pipeline =
        MakeTestPipeline(vertexState, 1, {{0, VertexFormat::Float4, InputStepMode::Instance}});

    // clang-format off
    wgpu::Buffer buffer0 = MakeVertexBuffer<float>({
        0, 1, 2, 3,
        1, 2, 3, 4,
        2, 3, 4, 5,
        3, 4, 5, 6,
    });
    // clang-format on
    DoTestDraw(pipeline, 1, 4, {DrawVertexBuffer{0, &buffer0}});
}

// Test with mixed everything, vertex vs. instance, different stride and offsets
// different attribute types
TEST_P(VertexStateTest, MixedEverything) {
    utils::ComboVertexStateDescriptor vertexState = MakeVertexState(
        {{12 * sizeof(float),
          InputStepMode::Vertex,
          {{0, 0, VertexFormat::Float}, {1, 6 * sizeof(float), VertexFormat::Float2}}},
         {10 * sizeof(float),
          InputStepMode::Instance,
          {{2, 0, VertexFormat::Float3}, {3, 5 * sizeof(float), VertexFormat::Float4}}}});
    wgpu::RenderPipeline pipeline =
        MakeTestPipeline(vertexState, 1,
                         {{0, VertexFormat::Float, InputStepMode::Vertex},
                          {1, VertexFormat::Float2, InputStepMode::Vertex},
                          {2, VertexFormat::Float3, InputStepMode::Instance},
                          {3, VertexFormat::Float4, InputStepMode::Instance}});

    // clang-format off
    wgpu::Buffer buffer0 = MakeVertexBuffer<float>({
        0, 1, 2, 3, 0, 0, 0, 1, 2, 3, 0, 0,
        1, 2, 3, 4, 0, 0, 1, 2, 3, 4, 0, 0,
        2, 3, 4, 5, 0, 0, 2, 3, 4, 5, 0, 0,
        3, 4, 5, 6, 0, 0, 3, 4, 5, 6, 0, 0,
    });
    wgpu::Buffer buffer1 = MakeVertexBuffer<float>({
        0, 1, 2, 3, 0, 0, 1, 2, 3, 0,
        1, 2, 3, 4, 0, 1, 2, 3, 4, 0,
        2, 3, 4, 5, 0, 2, 3, 4, 5, 0,
        3, 4, 5, 6, 0, 3, 4, 5, 6, 0,
    });
    // clang-format on
    DoTestDraw(pipeline, 1, 1, {{0, &buffer0}, {1, &buffer1}});
}

// Test input state is unaffected by unused vertex slot
TEST_P(VertexStateTest, UnusedVertexSlot) {
    // Instance input state, using slot 1
    utils::ComboVertexStateDescriptor instanceVertexState = MakeVertexState(
        {{0, InputStepMode::Vertex, {}},
         {4 * sizeof(float), InputStepMode::Instance, {{0, 0, VertexFormat::Float4}}}});
    wgpu::RenderPipeline instancePipeline = MakeTestPipeline(
        instanceVertexState, 1, {{0, VertexFormat::Float4, InputStepMode::Instance}});

    // clang-format off
    wgpu::Buffer buffer = MakeVertexBuffer<float>({
        0, 1, 2, 3,
        1, 2, 3, 4,
        2, 3, 4, 5,
        3, 4, 5, 6,
    });
    // clang-format on

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);

    pass.SetVertexBuffer(0, buffer);
    pass.SetVertexBuffer(1, buffer);

    pass.SetPipeline(instancePipeline);
    pass.Draw(3, 4);

    pass.EndPass();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    CheckResult(1, 4);
}

// Test setting a different pipeline with a different input state.
// This was a problem with the D3D12 backend where SetVertexBuffer
// was getting the input from the last set pipeline, not the current.
// SetVertexBuffer should be reapplied when the input state changes.
TEST_P(VertexStateTest, MultiplePipelinesMixedVertexState) {
    // Basic input state, using slot 0
    utils::ComboVertexStateDescriptor vertexVertexState = MakeVertexState(
        {{4 * sizeof(float), InputStepMode::Vertex, {{0, 0, VertexFormat::Float4}}}});
    wgpu::RenderPipeline vertexPipeline =
        MakeTestPipeline(vertexVertexState, 1, {{0, VertexFormat::Float4, InputStepMode::Vertex}});

    // Instance input state, using slot 1
    utils::ComboVertexStateDescriptor instanceVertexState = MakeVertexState(
        {{0, InputStepMode::Instance, {}},
         {4 * sizeof(float), InputStepMode::Instance, {{0, 0, VertexFormat::Float4}}}});
    wgpu::RenderPipeline instancePipeline = MakeTestPipeline(
        instanceVertexState, 1, {{0, VertexFormat::Float4, InputStepMode::Instance}});

    // clang-format off
    wgpu::Buffer buffer = MakeVertexBuffer<float>({
        0, 1, 2, 3,
        1, 2, 3, 4,
        2, 3, 4, 5,
        3, 4, 5, 6,
    });
    // clang-format on

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);

    pass.SetVertexBuffer(0, buffer);
    pass.SetVertexBuffer(1, buffer);

    pass.SetPipeline(vertexPipeline);
    pass.Draw(3);

    pass.SetPipeline(instancePipeline);
    pass.Draw(3, 4);

    pass.EndPass();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    CheckResult(1, 4);
}

// Checks that using the last vertex buffer doesn't overflow the vertex buffer table in Metal.
TEST_P(VertexStateTest, LastAllowedVertexBuffer) {
    constexpr uint32_t kBufferIndex = kMaxVertexBuffers - 1;

    utils::ComboVertexStateDescriptor vertexState;
    // All the other vertex buffers default to no attributes
    vertexState.vertexBufferCount = kMaxVertexBuffers;
    vertexState.cVertexBuffers[kBufferIndex].arrayStride = 4 * sizeof(float);
    vertexState.cVertexBuffers[kBufferIndex].stepMode = InputStepMode::Vertex;
    vertexState.cVertexBuffers[kBufferIndex].attributeCount = 1;
    vertexState.cVertexBuffers[kBufferIndex].attributes = &vertexState.cAttributes[0];
    vertexState.cAttributes[0].shaderLocation = 0;
    vertexState.cAttributes[0].offset = 0;
    vertexState.cAttributes[0].format = VertexFormat::Float4;

    wgpu::RenderPipeline pipeline =
        MakeTestPipeline(vertexState, 1, {{0, VertexFormat::Float4, InputStepMode::Vertex}});

    wgpu::Buffer buffer0 = MakeVertexBuffer<float>({0, 1, 2, 3, 1, 2, 3, 4, 2, 3, 4, 5});
    DoTestDraw(pipeline, 1, 1, {DrawVertexBuffer{kMaxVertexBuffers - 1, &buffer0}});
}

// Test that overlapping vertex attributes are permitted and load data correctly
TEST_P(VertexStateTest, OverlappingVertexAttributes) {
    utils::BasicRenderPass renderPass = utils::CreateBasicRenderPass(device, 3, 3);

    utils::ComboVertexStateDescriptor vertexState =
        MakeVertexState({{16,
                          InputStepMode::Vertex,
                          {
                              // "****" represents the bytes we'll actually read in the shader.
                              {0, 0 /* offset */, VertexFormat::Float4},  // |****|----|----|----|
                              {1, 4 /* offset */, VertexFormat::UInt2},   //      |****|****|
                              {2, 8 /* offset */, VertexFormat::Half4},   //           |-----****|
                              {3, 0 /* offset */, VertexFormat::Float},   // |****|
                          }}});

    struct Data {
        float fvalue;
        uint32_t uints[2];
        uint16_t halfs[2];
    };
    static_assert(sizeof(Data) == 16, "");
    Data data{1.f, {2u, 3u}, {Float32ToFloat16(4.f), Float32ToFloat16(5.f)}};

    wgpu::Buffer vertexBuffer =
        utils::CreateBufferFromData(device, &data, sizeof(data), wgpu::BufferUsage::Vertex);

    utils::ComboRenderPipelineDescriptor pipelineDesc(device);
    pipelineDesc.vertexStage.module =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
                #version 450
                layout(location = 0) in vec4 attr0;
                layout(location = 1) in uvec2 attr1;
                layout(location = 2) in vec4 attr2;
                layout(location = 3) in float attr3;

                layout(location = 0) out vec4 color;

                void main() {
                    gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
                    gl_PointSize = 1.0;

                    bool success = (
                        attr0.x == 1.0f &&
                        attr1.x == 2u &&
                        attr1.y == 3u &&
                        attr2.z == 4.0f &&
                        attr2.w == 5.0f &&
                        attr3 == 1.0f
                    );
                    color = success ? vec4(0.0, 1.0, 0.0, 1.0) : vec4(1.0, 0.0, 0.0, 1.0);
                })");
    pipelineDesc.cFragmentStage.module =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, R"(
                #version 450
                layout(location = 0) in vec4 color;
                layout(location = 0) out vec4 fragColor;
                void main() {
                    fragColor = color;
                })");
    pipelineDesc.vertexState = &vertexState;
    pipelineDesc.cColorStates[0].format = renderPass.colorFormat;
    pipelineDesc.primitiveTopology = wgpu::PrimitiveTopology::PointList;

    wgpu::RenderPipeline pipeline = device.CreateRenderPipeline(&pipelineDesc);

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
    pass.SetPipeline(pipeline);
    pass.SetVertexBuffer(0, vertexBuffer);
    pass.Draw(1);
    pass.EndPass();
    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    EXPECT_PIXEL_RGBA8_EQ(RGBA8::kGreen, renderPass.color, 1, 1);
}

DAWN_INSTANTIATE_TEST(VertexStateTest,
                      D3D12Backend(),
                      MetalBackend(),
                      OpenGLBackend(),
                      VulkanBackend());

// TODO for the input state:
//  - Add more vertex formats
//  - Add checks that the stride is enough to contain all attributes
//  - Add checks stride less than some limit
//  - Add checks for alignement of vertex buffers and attributes if needed
//  - Check for attribute narrowing
//  - Check that the input state and the pipeline vertex input types match

class OptionalVertexStateTest : public DawnTest {};

// Test that vertex input is not required in render pipeline descriptor.
TEST_P(OptionalVertexStateTest, Basic) {
    utils::BasicRenderPass renderPass = utils::CreateBasicRenderPass(device, 3, 3);

    wgpu::ShaderModule vsModule =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
            #version 450
            void main() {
                gl_Position = vec4(0.0f, 0.0f, 0.0f, 1.0f);
                gl_PointSize = 1.0;
            })");

    wgpu::ShaderModule fsModule =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, R"(
            #version 450
            layout(location = 0) out vec4 fragColor;
            void main() {
                fragColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);
            })");

    utils::ComboRenderPipelineDescriptor descriptor(device);
    descriptor.vertexStage.module = vsModule;
    descriptor.cFragmentStage.module = fsModule;
    descriptor.primitiveTopology = wgpu::PrimitiveTopology::PointList;
    descriptor.vertexState = nullptr;

    wgpu::RenderPipeline pipeline = device.CreateRenderPipeline(&descriptor);

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
        pass.SetPipeline(pipeline);
        pass.Draw(1);
        pass.EndPass();
    }

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    EXPECT_PIXEL_RGBA8_EQ(RGBA8::kGreen, renderPass.color, 1, 1);
}

DAWN_INSTANTIATE_TEST(OptionalVertexStateTest,
                      D3D12Backend(),
                      MetalBackend(),
                      OpenGLBackend(),
                      VulkanBackend());
