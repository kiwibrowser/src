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

// Primitive topology tests work by drawing the following vertices with all the different primitive
// topology states:
// -------------------------------------
// |                                   |
// |        1        2        5        |
// |                                   |
// |                                   |
// |                                   |
// |                                   |
// |        0        3        4        |
// |                                   |
// -------------------------------------
//
// Points: This case looks exactly like above
//
// Lines
// -------------------------------------
// |                                   |
// |        1        2        5        |
// |        |        |        |        |
// |        |        |        |        |
// |        |        |        |        |
// |        |        |        |        |
// |        0        3        4        |
// |                                   |
// -------------------------------------
//
// Line Strip
// -------------------------------------
// |                                   |
// |        1--------2        5        |
// |        |        |        |        |
// |        |        |        |        |
// |        |        |        |        |
// |        |        |        |        |
// |        0        3--------4        |
// |                                   |
// -------------------------------------
//
// Triangle
// -------------------------------------
// |                                   |
// |        1--------2        5        |
// |        |xxxxxxx         x|        |
// |        |xxxxx         xxx|        |
// |        |xxx         xxxxx|        |
// |        |x         xxxxxxx|        |
// |        0        3--------4        |
// |                                   |
// -------------------------------------
//
// Triangle Strip
// -------------------------------------
// |                                   |
// |        1--------2        5        |
// |        |xxxxxxxxx       x|        |
// |        |xxxxxxxxxxx   xxx|        |
// |        |xxx   xxxxxxxxxxx|        |
// |        |x      xxxxxxxxxx|        |
// |        0        3--------4        |
// |                                   |
// -------------------------------------
//
// Each of these different states is a superset of some of the previous states,
// so for every state, we check any new added test locations that are not contained in previous
// states We also check that the test locations of subsequent states are untouched

constexpr static unsigned int kRTSize = 32;

struct TestLocation {
    unsigned int x, y;
};

constexpr TestLocation GetMidpoint(const TestLocation& a, const TestLocation& b) noexcept {
    return {(a.x + b.x) / 2, (a.y + b.y) / 2};
}

constexpr TestLocation GetCentroid(const TestLocation& a,
                                   const TestLocation& b,
                                   const TestLocation& c) noexcept {
    return {(a.x + b.x + c.x) / 3, (a.y + b.y + c.y) / 3};
}

// clang-format off
// Offset towards one corner to avoid x or y symmetry false positives
constexpr static unsigned int kOffset = kRTSize / 8;

constexpr static TestLocation kPointTestLocations[] = {
    { kRTSize * 1 / 4 + kOffset, kRTSize * 1 / 4 + kOffset },
    { kRTSize * 1 / 4 + kOffset, kRTSize * 3 / 4 + kOffset },
    { kRTSize * 2 / 4 + kOffset, kRTSize * 3 / 4 + kOffset },
    { kRTSize * 2 / 4 + kOffset, kRTSize * 1 / 4 + kOffset },
    { kRTSize * 3 / 4 + kOffset, kRTSize * 1 / 4 + kOffset },
    { kRTSize * 3 / 4 + kOffset, kRTSize * 3 / 4 + kOffset },
};

constexpr static TestLocation kLineTestLocations[] = {
    GetMidpoint(kPointTestLocations[0], kPointTestLocations[1]),
    GetMidpoint(kPointTestLocations[2], kPointTestLocations[3]),
    GetMidpoint(kPointTestLocations[4], kPointTestLocations[5]),
};

constexpr static TestLocation kLineStripTestLocations[] = {
    GetMidpoint(kPointTestLocations[1], kPointTestLocations[2]),
    GetMidpoint(kPointTestLocations[3], kPointTestLocations[4]),
};

constexpr static TestLocation kTriangleTestLocations[] = {
    GetCentroid(kPointTestLocations[0], kPointTestLocations[1], kPointTestLocations[2]),
    GetCentroid(kPointTestLocations[3], kPointTestLocations[4], kPointTestLocations[5]),
};

constexpr static TestLocation kTriangleStripTestLocations[] = {
    GetCentroid(kPointTestLocations[1], kPointTestLocations[2], kPointTestLocations[3]),
    GetCentroid(kPointTestLocations[2], kPointTestLocations[3], kPointTestLocations[4]),
};

constexpr static float kRTSizef = static_cast<float>(kRTSize);
constexpr static float kVertices[] = {
    2.f * (kPointTestLocations[0].x + 0.5f) / kRTSizef - 1.f, -2.f * (kPointTestLocations[0].y + 0.5f) / kRTSizef + 1.0f, 0.f, 1.f,
    2.f * (kPointTestLocations[1].x + 0.5f) / kRTSizef - 1.f, -2.f * (kPointTestLocations[1].y + 0.5f) / kRTSizef + 1.0f, 0.f, 1.f,
    2.f * (kPointTestLocations[2].x + 0.5f) / kRTSizef - 1.f, -2.f * (kPointTestLocations[2].y + 0.5f) / kRTSizef + 1.0f, 0.f, 1.f,
    2.f * (kPointTestLocations[3].x + 0.5f) / kRTSizef - 1.f, -2.f * (kPointTestLocations[3].y + 0.5f) / kRTSizef + 1.0f, 0.f, 1.f,
    2.f * (kPointTestLocations[4].x + 0.5f) / kRTSizef - 1.f, -2.f * (kPointTestLocations[4].y + 0.5f) / kRTSizef + 1.0f, 0.f, 1.f,
    2.f * (kPointTestLocations[5].x + 0.5f) / kRTSizef - 1.f, -2.f * (kPointTestLocations[5].y + 0.5f) / kRTSizef + 1.0f, 0.f, 1.f,
};
// clang-format on

class PrimitiveTopologyTest : public DawnTest {
  protected:
    void SetUp() override {
        DawnTest::SetUp();

        renderPass = utils::CreateBasicRenderPass(device, kRTSize, kRTSize);

        vsModule = utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
                #version 450
                layout(location = 0) in vec4 pos;
                void main() {
                    gl_Position = pos;
                    gl_PointSize = 1.0;
                })");

        fsModule = utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, R"(
                #version 450
                layout(location = 0) out vec4 fragColor;
                void main() {
                    fragColor = vec4(0.0, 1.0, 0.0, 1.0);
                })");

        vertexBuffer = utils::CreateBufferFromData(device, kVertices, sizeof(kVertices),
                                                   wgpu::BufferUsage::Vertex);
    }

    struct LocationSpec {
        const TestLocation* locations;
        size_t count;
        bool include;
    };

    template <std::size_t N>
    constexpr LocationSpec TestPoints(TestLocation const (&points)[N], bool include) noexcept {
        return {points, N, include};
    }

    // Draw the vertices with the given primitive topology and check the pixel values of the test
    // locations
    void DoTest(wgpu::PrimitiveTopology primitiveTopology,
                const std::vector<LocationSpec>& locationSpecs) {
        utils::ComboRenderPipelineDescriptor descriptor(device);
        descriptor.vertexStage.module = vsModule;
        descriptor.cFragmentStage.module = fsModule;
        descriptor.primitiveTopology = primitiveTopology;
        descriptor.cVertexState.vertexBufferCount = 1;
        descriptor.cVertexState.cVertexBuffers[0].arrayStride = 4 * sizeof(float);
        descriptor.cVertexState.cVertexBuffers[0].attributeCount = 1;
        descriptor.cVertexState.cAttributes[0].format = wgpu::VertexFormat::Float4;
        descriptor.cColorStates[0].format = renderPass.colorFormat;

        wgpu::RenderPipeline pipeline = device.CreateRenderPipeline(&descriptor);

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        {
            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
            pass.SetPipeline(pipeline);
            pass.SetVertexBuffer(0, vertexBuffer);
            pass.Draw(6);
            pass.EndPass();
        }

        wgpu::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);

        for (auto& locationSpec : locationSpecs) {
            for (size_t i = 0; i < locationSpec.count; ++i) {
                // If this pixel is included, check that it is green. Otherwise, check that it is
                // black
                RGBA8 color = locationSpec.include ? RGBA8::kGreen : RGBA8::kZero;
                EXPECT_PIXEL_RGBA8_EQ(color, renderPass.color, locationSpec.locations[i].x,
                                      locationSpec.locations[i].y)
                    << "Expected (" << locationSpec.locations[i].x << ", "
                    << locationSpec.locations[i].y << ") to be " << color;
            }
        }
    }

    utils::BasicRenderPass renderPass;
    wgpu::ShaderModule vsModule;
    wgpu::ShaderModule fsModule;
    wgpu::Buffer vertexBuffer;
};

// Test Point primitive topology
TEST_P(PrimitiveTopologyTest, PointList) {
    DoTest(wgpu::PrimitiveTopology::PointList,
           {
               // Check that the points are drawn
               TestPoints(kPointTestLocations, true),

               // Check that line and triangle locations are untouched
               TestPoints(kLineTestLocations, false),
               TestPoints(kLineStripTestLocations, false),
               TestPoints(kTriangleTestLocations, false),
               TestPoints(kTriangleStripTestLocations, false),
           });
}

// Test Line primitive topology
TEST_P(PrimitiveTopologyTest, LineList) {
    DoTest(wgpu::PrimitiveTopology::LineList,
           {
               // Check that lines are drawn
               TestPoints(kLineTestLocations, true),

               // Check that line strip and triangle locations are untouched
               TestPoints(kLineStripTestLocations, false),
               TestPoints(kTriangleTestLocations, false),
               TestPoints(kTriangleStripTestLocations, false),
           });
}

// Test LineStrip primitive topology
TEST_P(PrimitiveTopologyTest, LineStrip) {
    DoTest(wgpu::PrimitiveTopology::LineStrip, {
                                                   // Check that lines are drawn
                                                   TestPoints(kLineTestLocations, true),
                                                   TestPoints(kLineStripTestLocations, true),

                                                   // Check that triangle locations are untouched
                                                   TestPoints(kTriangleTestLocations, false),
                                                   TestPoints(kTriangleStripTestLocations, false),
                                               });
}

// Test Triangle primitive topology
TEST_P(PrimitiveTopologyTest, TriangleList) {
    DoTest(wgpu::PrimitiveTopology::TriangleList,
           {
               // Check that triangles are drawn
               TestPoints(kTriangleTestLocations, true),

               // Check that triangle strip locations are untouched
               TestPoints(kTriangleStripTestLocations, false),
           });
}

// Test TriangleStrip primitive topology
TEST_P(PrimitiveTopologyTest, TriangleStrip) {
    DoTest(wgpu::PrimitiveTopology::TriangleStrip,
           {
               TestPoints(kTriangleTestLocations, true),
               TestPoints(kTriangleStripTestLocations, true),
           });
}

DAWN_INSTANTIATE_TEST(PrimitiveTopologyTest,
                      D3D12Backend(),
                      MetalBackend(),
                      OpenGLBackend(),
                      VulkanBackend());
