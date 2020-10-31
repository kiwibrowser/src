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
#include "utils/WGPUHelpers.h"

constexpr static unsigned int kRTSize = 64;

class DepthStencilStateTest : public DawnTest {
  protected:
    void SetUp() override {
        DawnTest::SetUp();

        wgpu::TextureDescriptor renderTargetDescriptor;
        renderTargetDescriptor.dimension = wgpu::TextureDimension::e2D;
        renderTargetDescriptor.size.width = kRTSize;
        renderTargetDescriptor.size.height = kRTSize;
        renderTargetDescriptor.size.depth = 1;
        renderTargetDescriptor.sampleCount = 1;
        renderTargetDescriptor.format = wgpu::TextureFormat::RGBA8Unorm;
        renderTargetDescriptor.mipLevelCount = 1;
        renderTargetDescriptor.usage =
            wgpu::TextureUsage::OutputAttachment | wgpu::TextureUsage::CopySrc;
        renderTarget = device.CreateTexture(&renderTargetDescriptor);

        renderTargetView = renderTarget.CreateView();

        wgpu::TextureDescriptor depthDescriptor;
        depthDescriptor.dimension = wgpu::TextureDimension::e2D;
        depthDescriptor.size.width = kRTSize;
        depthDescriptor.size.height = kRTSize;
        depthDescriptor.size.depth = 1;
        depthDescriptor.sampleCount = 1;
        depthDescriptor.format = wgpu::TextureFormat::Depth24PlusStencil8;
        depthDescriptor.mipLevelCount = 1;
        depthDescriptor.usage = wgpu::TextureUsage::OutputAttachment;
        depthTexture = device.CreateTexture(&depthDescriptor);

        depthTextureView = depthTexture.CreateView();

        vsModule = utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
                #version 450
                layout(set = 0, binding = 0) uniform myBlock {
                    vec3 color;
                    float depth;
                } myUbo;
                void main() {
                    const vec2 pos[6] = vec2[6](
                        vec2(-1.f, 1.f), vec2(-1.f, -1.f), vec2(1.f, -1.f), // front-facing
                        vec2(-1.f, 1.f), vec2(1.f, 1.f), vec2(1.f, -1.f)    // back-facing
                    );
                    gl_Position = vec4(pos[gl_VertexIndex], myUbo.depth, 1.f);
                }
            )");

        fsModule = utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, R"(
                #version 450
                layout(set = 0, binding = 0) uniform myBlock {
                    vec3 color;
                    float depth;
                } myUbo;
                layout(location = 0) out vec4 fragColor;
                void main() {
                    fragColor = vec4(myUbo.color, 1.f);
                }
            )");
    }

    struct TestSpec {
        const wgpu::DepthStencilStateDescriptor& depthStencilState;
        RGBA8 color;
        float depth;
        uint32_t stencil;
    };

    // Check whether a depth comparison function works as expected
    // The less, equal, greater booleans denote wether the respective triangle should be visible
    // based on the comparison function
    void CheckDepthCompareFunction(wgpu::CompareFunction compareFunction,
                                   bool less,
                                   bool equal,
                                   bool greater) {
        wgpu::StencilStateFaceDescriptor stencilFace;
        stencilFace.compare = wgpu::CompareFunction::Always;
        stencilFace.failOp = wgpu::StencilOperation::Keep;
        stencilFace.depthFailOp = wgpu::StencilOperation::Keep;
        stencilFace.passOp = wgpu::StencilOperation::Keep;

        wgpu::DepthStencilStateDescriptor baseState;
        baseState.depthWriteEnabled = true;
        baseState.depthCompare = wgpu::CompareFunction::Always;
        baseState.stencilBack = stencilFace;
        baseState.stencilFront = stencilFace;
        baseState.stencilReadMask = 0xff;
        baseState.stencilWriteMask = 0xff;

        wgpu::DepthStencilStateDescriptor state;
        state.depthWriteEnabled = true;
        state.depthCompare = compareFunction;
        state.stencilBack = stencilFace;
        state.stencilFront = stencilFace;
        state.stencilReadMask = 0xff;
        state.stencilWriteMask = 0xff;

        RGBA8 baseColor = RGBA8(255, 255, 255, 255);
        RGBA8 lessColor = RGBA8(255, 0, 0, 255);
        RGBA8 equalColor = RGBA8(0, 255, 0, 255);
        RGBA8 greaterColor = RGBA8(0, 0, 255, 255);

        // Base triangle at depth 0.5, depth always, depth write enabled
        TestSpec base = {baseState, baseColor, 0.5f, 0u};

        // Draw the base triangle, then a triangle in stencilFront of the base triangle with the
        // given depth comparison function
        DoTest({base, {state, lessColor, 0.f, 0u}}, less ? lessColor : baseColor);

        // Draw the base triangle, then a triangle in at the same depth as the base triangle with
        // the given depth comparison function
        DoTest({base, {state, equalColor, 0.5f, 0u}}, equal ? equalColor : baseColor);

        // Draw the base triangle, then a triangle behind the base triangle with the given depth
        // comparison function
        DoTest({base, {state, greaterColor, 1.0f, 0u}}, greater ? greaterColor : baseColor);
    }

    // Check whether a stencil comparison function works as expected
    // The less, equal, greater booleans denote wether the respective triangle should be visible
    // based on the comparison function
    void CheckStencilCompareFunction(wgpu::CompareFunction compareFunction,
                                     bool less,
                                     bool equal,
                                     bool greater) {
        wgpu::StencilStateFaceDescriptor baseStencilFaceDescriptor;
        baseStencilFaceDescriptor.compare = wgpu::CompareFunction::Always;
        baseStencilFaceDescriptor.failOp = wgpu::StencilOperation::Keep;
        baseStencilFaceDescriptor.depthFailOp = wgpu::StencilOperation::Keep;
        baseStencilFaceDescriptor.passOp = wgpu::StencilOperation::Replace;
        wgpu::DepthStencilStateDescriptor baseState;
        baseState.depthWriteEnabled = false;
        baseState.depthCompare = wgpu::CompareFunction::Always;
        baseState.stencilBack = baseStencilFaceDescriptor;
        baseState.stencilFront = baseStencilFaceDescriptor;
        baseState.stencilReadMask = 0xff;
        baseState.stencilWriteMask = 0xff;

        wgpu::StencilStateFaceDescriptor stencilFaceDescriptor;
        stencilFaceDescriptor.compare = compareFunction;
        stencilFaceDescriptor.failOp = wgpu::StencilOperation::Keep;
        stencilFaceDescriptor.depthFailOp = wgpu::StencilOperation::Keep;
        stencilFaceDescriptor.passOp = wgpu::StencilOperation::Keep;
        wgpu::DepthStencilStateDescriptor state;
        state.depthWriteEnabled = false;
        state.depthCompare = wgpu::CompareFunction::Always;
        state.stencilBack = stencilFaceDescriptor;
        state.stencilFront = stencilFaceDescriptor;
        state.stencilReadMask = 0xff;
        state.stencilWriteMask = 0xff;

        RGBA8 baseColor = RGBA8(255, 255, 255, 255);
        RGBA8 lessColor = RGBA8(255, 0, 0, 255);
        RGBA8 equalColor = RGBA8(0, 255, 0, 255);
        RGBA8 greaterColor = RGBA8(0, 0, 255, 255);

        // Base triangle with stencil reference 1
        TestSpec base = {baseState, baseColor, 0.0f, 1u};

        // Draw the base triangle, then a triangle with stencil reference 0 with the given stencil
        // comparison function
        DoTest({base, {state, lessColor, 0.f, 0u}}, less ? lessColor : baseColor);

        // Draw the base triangle, then a triangle with stencil reference 1 with the given stencil
        // comparison function
        DoTest({base, {state, equalColor, 0.f, 1u}}, equal ? equalColor : baseColor);

        // Draw the base triangle, then a triangle with stencil reference 2 with the given stencil
        // comparison function
        DoTest({base, {state, greaterColor, 0.f, 2u}}, greater ? greaterColor : baseColor);
    }

    // Given the provided `initialStencil` and `reference`, check that applying the
    // `stencilOperation` produces the `expectedStencil`
    void CheckStencilOperation(wgpu::StencilOperation stencilOperation,
                               uint32_t initialStencil,
                               uint32_t reference,
                               uint32_t expectedStencil) {
        wgpu::StencilStateFaceDescriptor baseStencilFaceDescriptor;
        baseStencilFaceDescriptor.compare = wgpu::CompareFunction::Always;
        baseStencilFaceDescriptor.failOp = wgpu::StencilOperation::Keep;
        baseStencilFaceDescriptor.depthFailOp = wgpu::StencilOperation::Keep;
        baseStencilFaceDescriptor.passOp = wgpu::StencilOperation::Replace;
        wgpu::DepthStencilStateDescriptor baseState;
        baseState.depthWriteEnabled = false;
        baseState.depthCompare = wgpu::CompareFunction::Always;
        baseState.stencilBack = baseStencilFaceDescriptor;
        baseState.stencilFront = baseStencilFaceDescriptor;
        baseState.stencilReadMask = 0xff;
        baseState.stencilWriteMask = 0xff;

        wgpu::StencilStateFaceDescriptor stencilFaceDescriptor;
        stencilFaceDescriptor.compare = wgpu::CompareFunction::Always;
        stencilFaceDescriptor.failOp = wgpu::StencilOperation::Keep;
        stencilFaceDescriptor.depthFailOp = wgpu::StencilOperation::Keep;
        stencilFaceDescriptor.passOp = stencilOperation;
        wgpu::DepthStencilStateDescriptor state;
        state.depthWriteEnabled = false;
        state.depthCompare = wgpu::CompareFunction::Always;
        state.stencilBack = stencilFaceDescriptor;
        state.stencilFront = stencilFaceDescriptor;
        state.stencilReadMask = 0xff;
        state.stencilWriteMask = 0xff;

        CheckStencil(
            {
                // Wipe the stencil buffer with the initialStencil value
                {baseState, RGBA8(255, 255, 255, 255), 0.f, initialStencil},

                // Draw a triangle with the provided stencil operation and reference
                {state, RGBA8(255, 0, 0, 255), 0.f, reference},
            },
            expectedStencil);
    }

    // Draw a list of test specs, and check if the stencil value is equal to the expected value
    void CheckStencil(std::vector<TestSpec> testParams, uint32_t expectedStencil) {
        wgpu::StencilStateFaceDescriptor stencilFaceDescriptor;
        stencilFaceDescriptor.compare = wgpu::CompareFunction::Equal;
        stencilFaceDescriptor.failOp = wgpu::StencilOperation::Keep;
        stencilFaceDescriptor.depthFailOp = wgpu::StencilOperation::Keep;
        stencilFaceDescriptor.passOp = wgpu::StencilOperation::Keep;
        wgpu::DepthStencilStateDescriptor state;
        state.depthWriteEnabled = false;
        state.depthCompare = wgpu::CompareFunction::Always;
        state.stencilBack = stencilFaceDescriptor;
        state.stencilFront = stencilFaceDescriptor;
        state.stencilReadMask = 0xff;
        state.stencilWriteMask = 0xff;

        testParams.push_back({state, RGBA8(0, 255, 0, 255), 0, expectedStencil});
        DoTest(testParams, RGBA8(0, 255, 0, 255));
    }

    // Each test param represents a pair of triangles with a color, depth, stencil value, and
    // depthStencil state, one frontfacing, one backfacing Draw the triangles in order and check the
    // expected colors for the frontfaces and backfaces
    void DoTest(const std::vector<TestSpec>& testParams,
                const RGBA8& expectedFront,
                const RGBA8& expectedBack) {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

        struct TriangleData {
            float color[3];
            float depth;
        };

        utils::ComboRenderPassDescriptor renderPass({renderTargetView}, depthTextureView);
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);

        for (size_t i = 0; i < testParams.size(); ++i) {
            const TestSpec& test = testParams[i];

            TriangleData data = {
                {static_cast<float>(test.color.r) / 255.f, static_cast<float>(test.color.g) / 255.f,
                 static_cast<float>(test.color.b) / 255.f},
                test.depth,
            };
            // Upload a buffer for each triangle's depth and color data
            wgpu::Buffer buffer = utils::CreateBufferFromData(device, &data, sizeof(TriangleData),
                                                              wgpu::BufferUsage::Uniform);

            // Create a pipeline for the triangles with the test spec's depth stencil state

            utils::ComboRenderPipelineDescriptor descriptor(device);
            descriptor.vertexStage.module = vsModule;
            descriptor.cFragmentStage.module = fsModule;
            descriptor.cDepthStencilState = test.depthStencilState;
            descriptor.cDepthStencilState.format = wgpu::TextureFormat::Depth24PlusStencil8;
            descriptor.depthStencilState = &descriptor.cDepthStencilState;

            wgpu::RenderPipeline pipeline = device.CreateRenderPipeline(&descriptor);

            // Create a bind group for the data
            wgpu::BindGroup bindGroup = utils::MakeBindGroup(
                device, pipeline.GetBindGroupLayout(0), {{0, buffer, 0, sizeof(TriangleData)}});

            pass.SetPipeline(pipeline);
            pass.SetStencilReference(test.stencil);  // Set the stencil reference
            pass.SetBindGroup(0,
                              bindGroup);  // Set the bind group which contains color and depth data
            pass.Draw(6);
        }
        pass.EndPass();

        wgpu::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);

        EXPECT_PIXEL_RGBA8_EQ(expectedFront, renderTarget, kRTSize / 4, kRTSize / 2)
            << "Front face check failed";
        EXPECT_PIXEL_RGBA8_EQ(expectedBack, renderTarget, 3 * kRTSize / 4, kRTSize / 2)
            << "Back face check failed";
    }

    void DoTest(const std::vector<TestSpec>& testParams, const RGBA8& expected) {
        DoTest(testParams, expected, expected);
    }

    wgpu::Texture renderTarget;
    wgpu::Texture depthTexture;
    wgpu::TextureView renderTargetView;
    wgpu::TextureView depthTextureView;
    wgpu::ShaderModule vsModule;
    wgpu::ShaderModule fsModule;
};

// Test compilation and usage of the fixture
TEST_P(DepthStencilStateTest, Basic) {
    wgpu::StencilStateFaceDescriptor stencilFace;
    stencilFace.compare = wgpu::CompareFunction::Always;
    stencilFace.failOp = wgpu::StencilOperation::Keep;
    stencilFace.depthFailOp = wgpu::StencilOperation::Keep;
    stencilFace.passOp = wgpu::StencilOperation::Keep;

    wgpu::DepthStencilStateDescriptor state;
    state.depthWriteEnabled = false;
    state.depthCompare = wgpu::CompareFunction::Always;
    state.stencilBack = stencilFace;
    state.stencilFront = stencilFace;
    state.stencilReadMask = 0xff;
    state.stencilWriteMask = 0xff;

    DoTest(
        {
            {state, RGBA8(0, 255, 0, 255), 0.5f, 0u},
        },
        RGBA8(0, 255, 0, 255));
}

// Test defaults: depth and stencil tests disabled
TEST_P(DepthStencilStateTest, DepthStencilDisabled) {
    wgpu::StencilStateFaceDescriptor stencilFace;
    stencilFace.compare = wgpu::CompareFunction::Always;
    stencilFace.failOp = wgpu::StencilOperation::Keep;
    stencilFace.depthFailOp = wgpu::StencilOperation::Keep;
    stencilFace.passOp = wgpu::StencilOperation::Keep;

    wgpu::DepthStencilStateDescriptor state;
    state.depthWriteEnabled = false;
    state.depthCompare = wgpu::CompareFunction::Always;
    state.stencilBack = stencilFace;
    state.stencilFront = stencilFace;
    state.stencilReadMask = 0xff;
    state.stencilWriteMask = 0xff;

    TestSpec specs[3] = {
        {state, RGBA8(255, 0, 0, 255), 0.0f, 0u},
        {state, RGBA8(0, 255, 0, 255), 0.5f, 0u},
        {state, RGBA8(0, 0, 255, 255), 1.0f, 0u},
    };

    // Test that for all combinations, the last triangle drawn is the one visible
    // We check against three triangles because the stencil test may modify results
    for (uint32_t last = 0; last < 3; ++last) {
        uint32_t i = (last + 1) % 3;
        uint32_t j = (last + 2) % 3;
        DoTest({specs[i], specs[j], specs[last]}, specs[last].color);
        DoTest({specs[j], specs[i], specs[last]}, specs[last].color);
    }
}

// The following tests check that each depth comparison function works
TEST_P(DepthStencilStateTest, DepthAlways) {
    CheckDepthCompareFunction(wgpu::CompareFunction::Always, true, true, true);
}

TEST_P(DepthStencilStateTest, DepthEqual) {
    CheckDepthCompareFunction(wgpu::CompareFunction::Equal, false, true, false);
}

TEST_P(DepthStencilStateTest, DepthGreater) {
    CheckDepthCompareFunction(wgpu::CompareFunction::Greater, false, false, true);
}

TEST_P(DepthStencilStateTest, DepthGreaterEqual) {
    CheckDepthCompareFunction(wgpu::CompareFunction::GreaterEqual, false, true, true);
}

TEST_P(DepthStencilStateTest, DepthLess) {
    CheckDepthCompareFunction(wgpu::CompareFunction::Less, true, false, false);
}

TEST_P(DepthStencilStateTest, DepthLessEqual) {
    CheckDepthCompareFunction(wgpu::CompareFunction::LessEqual, true, true, false);
}

TEST_P(DepthStencilStateTest, DepthNever) {
    CheckDepthCompareFunction(wgpu::CompareFunction::Never, false, false, false);
}

TEST_P(DepthStencilStateTest, DepthNotEqual) {
    CheckDepthCompareFunction(wgpu::CompareFunction::NotEqual, true, false, true);
}

// Test that disabling depth writes works and leaves the depth buffer unchanged
TEST_P(DepthStencilStateTest, DepthWriteDisabled) {
    wgpu::StencilStateFaceDescriptor stencilFace;
    stencilFace.compare = wgpu::CompareFunction::Always;
    stencilFace.failOp = wgpu::StencilOperation::Keep;
    stencilFace.depthFailOp = wgpu::StencilOperation::Keep;
    stencilFace.passOp = wgpu::StencilOperation::Keep;

    wgpu::DepthStencilStateDescriptor baseState;
    baseState.depthWriteEnabled = true;
    baseState.depthCompare = wgpu::CompareFunction::Always;
    baseState.stencilBack = stencilFace;
    baseState.stencilFront = stencilFace;
    baseState.stencilReadMask = 0xff;
    baseState.stencilWriteMask = 0xff;

    wgpu::DepthStencilStateDescriptor noDepthWrite;
    noDepthWrite.depthWriteEnabled = false;
    noDepthWrite.depthCompare = wgpu::CompareFunction::Always;
    noDepthWrite.stencilBack = stencilFace;
    noDepthWrite.stencilFront = stencilFace;
    noDepthWrite.stencilReadMask = 0xff;
    noDepthWrite.stencilWriteMask = 0xff;

    wgpu::DepthStencilStateDescriptor checkState;
    checkState.depthWriteEnabled = false;
    checkState.depthCompare = wgpu::CompareFunction::Equal;
    checkState.stencilBack = stencilFace;
    checkState.stencilFront = stencilFace;
    checkState.stencilReadMask = 0xff;
    checkState.stencilWriteMask = 0xff;

    DoTest(
        {
            {baseState, RGBA8(255, 255, 255, 255), 1.f,
             0u},  // Draw a base triangle with depth enabled
            {noDepthWrite, RGBA8(0, 0, 0, 255), 0.f,
             0u},  // Draw a second triangle without depth enabled
            {checkState, RGBA8(0, 255, 0, 255), 1.f,
             0u},  // Draw a third triangle which should occlude the second even though it is behind
                   // it
        },
        RGBA8(0, 255, 0, 255));
}

// The following tests check that each stencil comparison function works
TEST_P(DepthStencilStateTest, StencilAlways) {
    CheckStencilCompareFunction(wgpu::CompareFunction::Always, true, true, true);
}

TEST_P(DepthStencilStateTest, StencilEqual) {
    CheckStencilCompareFunction(wgpu::CompareFunction::Equal, false, true, false);
}

TEST_P(DepthStencilStateTest, StencilGreater) {
    CheckStencilCompareFunction(wgpu::CompareFunction::Greater, false, false, true);
}

TEST_P(DepthStencilStateTest, StencilGreaterEqual) {
    CheckStencilCompareFunction(wgpu::CompareFunction::GreaterEqual, false, true, true);
}

TEST_P(DepthStencilStateTest, StencilLess) {
    CheckStencilCompareFunction(wgpu::CompareFunction::Less, true, false, false);
}

TEST_P(DepthStencilStateTest, StencilLessEqual) {
    CheckStencilCompareFunction(wgpu::CompareFunction::LessEqual, true, true, false);
}

TEST_P(DepthStencilStateTest, StencilNever) {
    CheckStencilCompareFunction(wgpu::CompareFunction::Never, false, false, false);
}

TEST_P(DepthStencilStateTest, StencilNotEqual) {
    CheckStencilCompareFunction(wgpu::CompareFunction::NotEqual, true, false, true);
}

// The following tests check that each stencil operation works
TEST_P(DepthStencilStateTest, StencilKeep) {
    CheckStencilOperation(wgpu::StencilOperation::Keep, 1, 3, 1);
}

TEST_P(DepthStencilStateTest, StencilZero) {
    CheckStencilOperation(wgpu::StencilOperation::Zero, 1, 3, 0);
}

TEST_P(DepthStencilStateTest, StencilReplace) {
    CheckStencilOperation(wgpu::StencilOperation::Replace, 1, 3, 3);
}

TEST_P(DepthStencilStateTest, StencilInvert) {
    CheckStencilOperation(wgpu::StencilOperation::Invert, 0xf0, 3, 0x0f);
}

TEST_P(DepthStencilStateTest, StencilIncrementClamp) {
    CheckStencilOperation(wgpu::StencilOperation::IncrementClamp, 1, 3, 2);
    CheckStencilOperation(wgpu::StencilOperation::IncrementClamp, 0xff, 3, 0xff);
}

TEST_P(DepthStencilStateTest, StencilIncrementWrap) {
    CheckStencilOperation(wgpu::StencilOperation::IncrementWrap, 1, 3, 2);
    CheckStencilOperation(wgpu::StencilOperation::IncrementWrap, 0xff, 3, 0);
}

TEST_P(DepthStencilStateTest, StencilDecrementClamp) {
    CheckStencilOperation(wgpu::StencilOperation::DecrementClamp, 1, 3, 0);
    CheckStencilOperation(wgpu::StencilOperation::DecrementClamp, 0, 3, 0);
}

TEST_P(DepthStencilStateTest, StencilDecrementWrap) {
    CheckStencilOperation(wgpu::StencilOperation::DecrementWrap, 1, 3, 0);
    CheckStencilOperation(wgpu::StencilOperation::DecrementWrap, 0, 3, 0xff);
}

// Check that the setting a stencil read mask works
TEST_P(DepthStencilStateTest, StencilReadMask) {
    wgpu::StencilStateFaceDescriptor baseStencilFaceDescriptor;
    baseStencilFaceDescriptor.compare = wgpu::CompareFunction::Always;
    baseStencilFaceDescriptor.failOp = wgpu::StencilOperation::Keep;
    baseStencilFaceDescriptor.depthFailOp = wgpu::StencilOperation::Keep;
    baseStencilFaceDescriptor.passOp = wgpu::StencilOperation::Replace;
    wgpu::DepthStencilStateDescriptor baseState;
    baseState.depthWriteEnabled = false;
    baseState.depthCompare = wgpu::CompareFunction::Always;
    baseState.stencilBack = baseStencilFaceDescriptor;
    baseState.stencilFront = baseStencilFaceDescriptor;
    baseState.stencilReadMask = 0xff;
    baseState.stencilWriteMask = 0xff;

    wgpu::StencilStateFaceDescriptor stencilFaceDescriptor;
    stencilFaceDescriptor.compare = wgpu::CompareFunction::Equal;
    stencilFaceDescriptor.failOp = wgpu::StencilOperation::Keep;
    stencilFaceDescriptor.depthFailOp = wgpu::StencilOperation::Keep;
    stencilFaceDescriptor.passOp = wgpu::StencilOperation::Keep;
    wgpu::DepthStencilStateDescriptor state;
    state.depthWriteEnabled = false;
    state.depthCompare = wgpu::CompareFunction::Always;
    state.stencilBack = stencilFaceDescriptor;
    state.stencilFront = stencilFaceDescriptor;
    state.stencilReadMask = 0x2;
    state.stencilWriteMask = 0xff;

    RGBA8 baseColor = RGBA8(255, 255, 255, 255);
    RGBA8 red = RGBA8(255, 0, 0, 255);
    RGBA8 green = RGBA8(0, 255, 0, 255);

    TestSpec base = {baseState, baseColor, 0.5f, 3u};  // Base triangle to set the stencil to 3
    DoTest({base, {state, red, 0.f, 1u}}, baseColor);  // Triangle with stencil reference 1 and read
                                                       // mask 2 does not draw because (3 & 2 != 1)
    DoTest({base, {state, green, 0.f, 2u}},
           green);  // Triangle with stencil reference 2 and read mask 2 draws because (3 & 2 == 2)
}

// Check that setting a stencil write mask works
TEST_P(DepthStencilStateTest, StencilWriteMask) {
    wgpu::StencilStateFaceDescriptor baseStencilFaceDescriptor;
    baseStencilFaceDescriptor.compare = wgpu::CompareFunction::Always;
    baseStencilFaceDescriptor.failOp = wgpu::StencilOperation::Keep;
    baseStencilFaceDescriptor.depthFailOp = wgpu::StencilOperation::Keep;
    baseStencilFaceDescriptor.passOp = wgpu::StencilOperation::Replace;
    wgpu::DepthStencilStateDescriptor baseState;
    baseState.depthWriteEnabled = false;
    baseState.depthCompare = wgpu::CompareFunction::Always;
    baseState.stencilBack = baseStencilFaceDescriptor;
    baseState.stencilFront = baseStencilFaceDescriptor;
    baseState.stencilReadMask = 0xff;
    baseState.stencilWriteMask = 0x1;

    wgpu::StencilStateFaceDescriptor stencilFaceDescriptor;
    stencilFaceDescriptor.compare = wgpu::CompareFunction::Equal;
    stencilFaceDescriptor.failOp = wgpu::StencilOperation::Keep;
    stencilFaceDescriptor.depthFailOp = wgpu::StencilOperation::Keep;
    stencilFaceDescriptor.passOp = wgpu::StencilOperation::Keep;
    wgpu::DepthStencilStateDescriptor state;
    state.depthWriteEnabled = false;
    state.depthCompare = wgpu::CompareFunction::Always;
    state.stencilBack = stencilFaceDescriptor;
    state.stencilFront = stencilFaceDescriptor;
    state.stencilReadMask = 0xff;
    state.stencilWriteMask = 0xff;

    RGBA8 baseColor = RGBA8(255, 255, 255, 255);
    RGBA8 green = RGBA8(0, 255, 0, 255);

    TestSpec base = {baseState, baseColor, 0.5f,
                     3u};  // Base triangle with stencil reference 3 and mask 1 to set the stencil 1
    DoTest({base, {state, green, 0.f, 2u}},
           baseColor);  // Triangle with stencil reference 2 does not draw because 2 != (3 & 1)
    DoTest({base, {state, green, 0.f, 1u}},
           green);  // Triangle with stencil reference 1 draws because 1 == (3 & 1)
}

// Test that the stencil operation is executed on stencil fail
TEST_P(DepthStencilStateTest, StencilFail) {
    wgpu::StencilStateFaceDescriptor baseStencilFaceDescriptor;
    baseStencilFaceDescriptor.compare = wgpu::CompareFunction::Always;
    baseStencilFaceDescriptor.failOp = wgpu::StencilOperation::Keep;
    baseStencilFaceDescriptor.depthFailOp = wgpu::StencilOperation::Keep;
    baseStencilFaceDescriptor.passOp = wgpu::StencilOperation::Replace;
    wgpu::DepthStencilStateDescriptor baseState;
    baseState.depthWriteEnabled = false;
    baseState.depthCompare = wgpu::CompareFunction::Always;
    baseState.stencilBack = baseStencilFaceDescriptor;
    baseState.stencilFront = baseStencilFaceDescriptor;
    baseState.stencilReadMask = 0xff;
    baseState.stencilWriteMask = 0xff;

    wgpu::StencilStateFaceDescriptor stencilFaceDescriptor;
    stencilFaceDescriptor.compare = wgpu::CompareFunction::Less;
    stencilFaceDescriptor.failOp = wgpu::StencilOperation::Replace;
    stencilFaceDescriptor.depthFailOp = wgpu::StencilOperation::Keep;
    stencilFaceDescriptor.passOp = wgpu::StencilOperation::Keep;
    wgpu::DepthStencilStateDescriptor state;
    state.depthWriteEnabled = false;
    state.depthCompare = wgpu::CompareFunction::Always;
    state.stencilBack = stencilFaceDescriptor;
    state.stencilFront = stencilFaceDescriptor;
    state.stencilReadMask = 0xff;
    state.stencilWriteMask = 0xff;

    CheckStencil(
        {
            {baseState, RGBA8(255, 255, 255, 255), 1.f, 1},  // Triangle to set stencil value to 1
            {state, RGBA8(0, 0, 0, 255), 0.f,
             2}  // Triangle with stencil reference 2 fails the Less comparison function
        },
        2);  // Replace the stencil on failure, so it should be 2
}

// Test that the stencil operation is executed on stencil pass, depth fail
TEST_P(DepthStencilStateTest, StencilDepthFail) {
    wgpu::StencilStateFaceDescriptor baseStencilFaceDescriptor;
    baseStencilFaceDescriptor.compare = wgpu::CompareFunction::Always;
    baseStencilFaceDescriptor.failOp = wgpu::StencilOperation::Keep;
    baseStencilFaceDescriptor.depthFailOp = wgpu::StencilOperation::Keep;
    baseStencilFaceDescriptor.passOp = wgpu::StencilOperation::Replace;
    wgpu::DepthStencilStateDescriptor baseState;
    baseState.depthWriteEnabled = true;
    baseState.depthCompare = wgpu::CompareFunction::Always;
    baseState.stencilBack = baseStencilFaceDescriptor;
    baseState.stencilFront = baseStencilFaceDescriptor;
    baseState.stencilReadMask = 0xff;
    baseState.stencilWriteMask = 0xff;

    wgpu::StencilStateFaceDescriptor stencilFaceDescriptor;
    stencilFaceDescriptor.compare = wgpu::CompareFunction::Greater;
    stencilFaceDescriptor.failOp = wgpu::StencilOperation::Keep;
    stencilFaceDescriptor.depthFailOp = wgpu::StencilOperation::Replace;
    stencilFaceDescriptor.passOp = wgpu::StencilOperation::Keep;
    wgpu::DepthStencilStateDescriptor state;
    state.depthWriteEnabled = true;
    state.depthCompare = wgpu::CompareFunction::Less;
    state.stencilBack = stencilFaceDescriptor;
    state.stencilFront = stencilFaceDescriptor;
    state.stencilReadMask = 0xff;
    state.stencilWriteMask = 0xff;

    CheckStencil({{baseState, RGBA8(255, 255, 255, 255), 0.f,
                   1},  // Triangle to set stencil value to 1. Depth is 0
                  {state, RGBA8(0, 0, 0, 255), 1.f,
                   2}},  // Triangle with stencil reference 2 passes the Greater comparison
                         // function. At depth 1, it fails the Less depth test
                 2);     // Replace the stencil on stencil pass, depth failure, so it should be 2
}

// Test that the stencil operation is executed on stencil pass, depth pass
TEST_P(DepthStencilStateTest, StencilDepthPass) {
    wgpu::StencilStateFaceDescriptor baseStencilFaceDescriptor;
    baseStencilFaceDescriptor.compare = wgpu::CompareFunction::Always;
    baseStencilFaceDescriptor.failOp = wgpu::StencilOperation::Keep;
    baseStencilFaceDescriptor.depthFailOp = wgpu::StencilOperation::Keep;
    baseStencilFaceDescriptor.passOp = wgpu::StencilOperation::Replace;
    wgpu::DepthStencilStateDescriptor baseState;
    baseState.depthWriteEnabled = true;
    baseState.depthCompare = wgpu::CompareFunction::Always;
    baseState.stencilBack = baseStencilFaceDescriptor;
    baseState.stencilFront = baseStencilFaceDescriptor;
    baseState.stencilReadMask = 0xff;
    baseState.stencilWriteMask = 0xff;

    wgpu::StencilStateFaceDescriptor stencilFaceDescriptor;
    stencilFaceDescriptor.compare = wgpu::CompareFunction::Greater;
    stencilFaceDescriptor.failOp = wgpu::StencilOperation::Keep;
    stencilFaceDescriptor.depthFailOp = wgpu::StencilOperation::Keep;
    stencilFaceDescriptor.passOp = wgpu::StencilOperation::Replace;
    wgpu::DepthStencilStateDescriptor state;
    state.depthWriteEnabled = true;
    state.depthCompare = wgpu::CompareFunction::Less;
    state.stencilBack = stencilFaceDescriptor;
    state.stencilFront = stencilFaceDescriptor;
    state.stencilReadMask = 0xff;
    state.stencilWriteMask = 0xff;

    CheckStencil({{baseState, RGBA8(255, 255, 255, 255), 1.f,
                   1},  // Triangle to set stencil value to 1. Depth is 0
                  {state, RGBA8(0, 0, 0, 255), 0.f,
                   2}},  // Triangle with stencil reference 2 passes the Greater comparison
                         // function. At depth 0, it pass the Less depth test
                 2);     // Replace the stencil on stencil pass, depth pass, so it should be 2
}

// Test that creating a render pipeline works with for all depth and combined formats
TEST_P(DepthStencilStateTest, CreatePipelineWithAllFormats) {
    constexpr wgpu::TextureFormat kDepthStencilFormats[] = {
        wgpu::TextureFormat::Depth32Float,
        wgpu::TextureFormat::Depth24PlusStencil8,
        wgpu::TextureFormat::Depth24Plus,
    };

    for (wgpu::TextureFormat depthStencilFormat : kDepthStencilFormats) {
        utils::ComboRenderPipelineDescriptor descriptor(device);
        descriptor.vertexStage.module = vsModule;
        descriptor.cFragmentStage.module = fsModule;
        descriptor.cDepthStencilState.format = depthStencilFormat;
        descriptor.depthStencilState = &descriptor.cDepthStencilState;

        device.CreateRenderPipeline(&descriptor);
    }
}

DAWN_INSTANTIATE_TEST(DepthStencilStateTest,
                      D3D12Backend(),
                      MetalBackend(),
                      OpenGLBackend(),
                      VulkanBackend({"vulkan_use_d32s8"}, {}),
                      VulkanBackend({}, {"vulkan_use_d32s8"}));
