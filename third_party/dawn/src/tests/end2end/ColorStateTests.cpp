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

#include <array>
#include <cmath>

#include "tests/DawnTest.h"

#include "common/Assert.h"
#include "common/Constants.h"
#include "utils/ComboRenderPipelineDescriptor.h"
#include "utils/DawnHelpers.h"

constexpr static unsigned int kRTSize = 64;

class ColorStateTest : public DawnTest {
  protected:
    void SetUp() override {
        DawnTest::SetUp();

        vsModule = utils::CreateShaderModule(device, dawn::ShaderStage::Vertex, R"(
                #version 450
                void main() {
                    const vec2 pos[3] = vec2[3](vec2(-1.f, -1.f), vec2(3.f, -1.f), vec2(-1.f, 3.f));
                    gl_Position = vec4(pos[gl_VertexIndex], 0.f, 1.f);
                }
            )");

        bindGroupLayout = utils::MakeBindGroupLayout(
            device, {
                        {0, dawn::ShaderStageBit::Fragment, dawn::BindingType::UniformBuffer},
                    });

        pipelineLayout = utils::MakeBasicPipelineLayout(device, &bindGroupLayout);

        renderPass = utils::CreateBasicRenderPass(device, kRTSize, kRTSize);
    }

    struct TriangleSpec {
        RGBA8 color;
        std::array<float, 4> blendFactor = {};
    };

    // Set up basePipeline and testPipeline. testPipeline has the given blend state on the first
    // attachment. basePipeline has no blending
    void SetupSingleSourcePipelines(dawn::ColorStateDescriptor colorStateDescriptor) {
        dawn::ShaderModule fsModule =
            utils::CreateShaderModule(device, dawn::ShaderStage::Fragment, R"(
                #version 450
                layout(set = 0, binding = 0) uniform myBlock {
                    vec4 color;
                } myUbo;

                layout(location = 0) out vec4 fragColor;

                void main() {
                    fragColor = myUbo.color;
                }
            )");

        utils::ComboRenderPipelineDescriptor baseDescriptor(device);
        baseDescriptor.layout = pipelineLayout;
        baseDescriptor.cVertexStage.module = vsModule;
        baseDescriptor.cFragmentStage.module = fsModule;
        baseDescriptor.cColorStates[0]->format = renderPass.colorFormat;

        basePipeline = device.CreateRenderPipeline(&baseDescriptor);

        utils::ComboRenderPipelineDescriptor testDescriptor(device);
        testDescriptor.layout = pipelineLayout;
        testDescriptor.cVertexStage.module = vsModule;
        testDescriptor.cFragmentStage.module = fsModule;
        testDescriptor.cColorStates[0] = &colorStateDescriptor;
        testDescriptor.cColorStates[0]->format = renderPass.colorFormat;

        testPipeline = device.CreateRenderPipeline(&testDescriptor);
    }

    // Create a bind group to set the colors as a uniform buffer
    template <size_t N>
    dawn::BindGroup MakeBindGroupForColors(std::array<RGBA8, N> colors) {
        std::array<float, 4 * N> data;
        for (unsigned int i = 0; i < N; ++i) {
            data[4 * i + 0] = static_cast<float>(colors[i].r) / 255.f;
            data[4 * i + 1] = static_cast<float>(colors[i].g) / 255.f;
            data[4 * i + 2] = static_cast<float>(colors[i].b) / 255.f;
            data[4 * i + 3] = static_cast<float>(colors[i].a) / 255.f;
        }

        uint32_t bufferSize = static_cast<uint32_t>(4 * N * sizeof(float));

        dawn::Buffer buffer =
            utils::CreateBufferFromData(device, &data, bufferSize, dawn::BufferUsageBit::Uniform);
        return utils::MakeBindGroup(device, bindGroupLayout, {{0, buffer, 0, bufferSize}});
    }

    // Test that after drawing a triangle with the base color, and then the given triangle spec, the
    // color is as expected
    void DoSingleSourceTest(RGBA8 base, const TriangleSpec& triangle, const RGBA8& expected) {
        dawn::Color blendColor{triangle.blendFactor[0], triangle.blendFactor[1],
                               triangle.blendFactor[2], triangle.blendFactor[3]};

        dawn::CommandEncoder encoder = device.CreateCommandEncoder();
        {
            dawn::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
            // First use the base pipeline to draw a triangle with no blending
            pass.SetPipeline(basePipeline);
            pass.SetBindGroup(0, MakeBindGroupForColors(std::array<RGBA8, 1>({{base}})), 0, nullptr);
            pass.Draw(3, 1, 0, 0);

            // Then use the test pipeline to draw the test triangle with blending
            pass.SetPipeline(testPipeline);
            pass.SetBindGroup(0, MakeBindGroupForColors(std::array<RGBA8, 1>({{triangle.color}})), 0, nullptr);
            pass.SetBlendColor(&blendColor);
            pass.Draw(3, 1, 0, 0);
            pass.EndPass();
        }

        dawn::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);

        EXPECT_PIXEL_RGBA8_EQ(expected, renderPass.color, kRTSize / 2, kRTSize / 2);
    }

    // Given a vector of tests where each element is <testColor, expectedColor>, check that all
    // expectations are true for the given blend operation
    void CheckBlendOperation(RGBA8 base,
                             dawn::BlendOperation operation,
                             std::vector<std::pair<RGBA8, RGBA8>> tests) {
        dawn::BlendDescriptor blend;
        blend.operation = operation;
        blend.srcFactor = dawn::BlendFactor::One;
        blend.dstFactor = dawn::BlendFactor::One;

        dawn::ColorStateDescriptor descriptor;
        descriptor.alphaBlend = blend;
        descriptor.colorBlend = blend;
        descriptor.writeMask = dawn::ColorWriteMask::All;

        SetupSingleSourcePipelines(descriptor);

        for (const auto& test : tests) {
            DoSingleSourceTest(base, {test.first}, test.second);
        }
    }

    // Given a vector of tests where each element is <testSpec, expectedColor>, check that all
    // expectations are true for the given blend factors
    void CheckBlendFactor(RGBA8 base,
                          dawn::BlendFactor colorSrcFactor,
                          dawn::BlendFactor colorDstFactor,
                          dawn::BlendFactor alphaSrcFactor,
                          dawn::BlendFactor alphaDstFactor,
                          std::vector<std::pair<TriangleSpec, RGBA8>> tests) {
        dawn::BlendDescriptor colorBlend;
        colorBlend.operation = dawn::BlendOperation::Add;
        colorBlend.srcFactor = colorSrcFactor;
        colorBlend.dstFactor = colorDstFactor;

        dawn::BlendDescriptor alphaBlend;
        alphaBlend.operation = dawn::BlendOperation::Add;
        alphaBlend.srcFactor = alphaSrcFactor;
        alphaBlend.dstFactor = alphaDstFactor;

        dawn::ColorStateDescriptor descriptor;
        descriptor.colorBlend = colorBlend;
        descriptor.alphaBlend = alphaBlend;
        descriptor.writeMask = dawn::ColorWriteMask::All;

        SetupSingleSourcePipelines(descriptor);

        for (const auto& test : tests) {
            DoSingleSourceTest(base, test.first, test.second);
        }
    }

    void CheckSrcBlendFactor(RGBA8 base,
                             dawn::BlendFactor colorFactor,
                             dawn::BlendFactor alphaFactor,
                             std::vector<std::pair<TriangleSpec, RGBA8>> tests) {
        CheckBlendFactor(base, colorFactor, dawn::BlendFactor::One, alphaFactor,
                         dawn::BlendFactor::One, tests);
    }

    void CheckDstBlendFactor(RGBA8 base,
                             dawn::BlendFactor colorFactor,
                             dawn::BlendFactor alphaFactor,
                             std::vector<std::pair<TriangleSpec, RGBA8>> tests) {
        CheckBlendFactor(base, dawn::BlendFactor::One, colorFactor, dawn::BlendFactor::One,
                         alphaFactor, tests);
    }

    utils::BasicRenderPass renderPass;
    dawn::RenderPipeline basePipeline;
    dawn::RenderPipeline testPipeline;
    dawn::ShaderModule vsModule;
    dawn::BindGroupLayout bindGroupLayout;
    dawn::PipelineLayout pipelineLayout;
};

namespace {
    // Add two colors and clamp
    constexpr RGBA8 operator+(const RGBA8& col1, const RGBA8& col2) {
        int r = static_cast<int>(col1.r) + static_cast<int>(col2.r);
        int g = static_cast<int>(col1.g) + static_cast<int>(col2.g);
        int b = static_cast<int>(col1.b) + static_cast<int>(col2.b);
        int a = static_cast<int>(col1.a) + static_cast<int>(col2.a);
        r = (r > 255 ? 255 : (r < 0 ? 0 : r));
        g = (g > 255 ? 255 : (g < 0 ? 0 : g));
        b = (b > 255 ? 255 : (b < 0 ? 0 : b));
        a = (a > 255 ? 255 : (a < 0 ? 0 : a));

        return RGBA8(static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b),
                     static_cast<uint8_t>(a));
    }

    // Subtract two colors and clamp
    constexpr RGBA8 operator-(const RGBA8& col1, const RGBA8& col2) {
        int r = static_cast<int>(col1.r) - static_cast<int>(col2.r);
        int g = static_cast<int>(col1.g) - static_cast<int>(col2.g);
        int b = static_cast<int>(col1.b) - static_cast<int>(col2.b);
        int a = static_cast<int>(col1.a) - static_cast<int>(col2.a);
        r = (r > 255 ? 255 : (r < 0 ? 0 : r));
        g = (g > 255 ? 255 : (g < 0 ? 0 : g));
        b = (b > 255 ? 255 : (b < 0 ? 0 : b));
        a = (a > 255 ? 255 : (a < 0 ? 0 : a));

        return RGBA8(static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b),
                     static_cast<uint8_t>(a));
    }

    // Get the component-wise minimum of two colors
    RGBA8 min(const RGBA8& col1, const RGBA8& col2) {
        return RGBA8(std::min(col1.r, col2.r), std::min(col1.g, col2.g), std::min(col1.b, col2.b),
                     std::min(col1.a, col2.a));
    }

    // Get the component-wise maximum of two colors
    RGBA8 max(const RGBA8& col1, const RGBA8& col2) {
        return RGBA8(std::max(col1.r, col2.r), std::max(col1.g, col2.g), std::max(col1.b, col2.b),
                     std::max(col1.a, col2.a));
    }

    // Blend two RGBA8 color values parameterized by the provided factors in the range [0.f, 1.f]
    RGBA8 mix(const RGBA8& col1, const RGBA8& col2, std::array<float, 4> fac) {
        float r = static_cast<float>(col1.r) * (1.f - fac[0]) + static_cast<float>(col2.r) * fac[0];
        float g = static_cast<float>(col1.g) * (1.f - fac[1]) + static_cast<float>(col2.g) * fac[1];
        float b = static_cast<float>(col1.b) * (1.f - fac[2]) + static_cast<float>(col2.b) * fac[2];
        float a = static_cast<float>(col1.a) * (1.f - fac[3]) + static_cast<float>(col2.a) * fac[3];

        return RGBA8({static_cast<uint8_t>(std::round(r)), static_cast<uint8_t>(std::round(g)),
                      static_cast<uint8_t>(std::round(b)), static_cast<uint8_t>(std::round(a))});
    }

    // Blend two RGBA8 color values parameterized by the provided RGBA8 factor
    RGBA8 mix(const RGBA8& col1, const RGBA8& col2, const RGBA8& fac) {
        std::array<float, 4> f = {{
            static_cast<float>(fac.r) / 255.f,
            static_cast<float>(fac.g) / 255.f,
            static_cast<float>(fac.b) / 255.f,
            static_cast<float>(fac.a) / 255.f,
        }};
        return mix(col1, col2, f);
    }

    constexpr std::array<RGBA8, 8> kColors = {{
        // check operations over multiple channels
        RGBA8(64, 0, 0, 0),
        RGBA8(0, 64, 0, 0),
        RGBA8(64, 0, 32, 0),
        RGBA8(0, 64, 32, 0),
        RGBA8(128, 0, 128, 128),
        RGBA8(0, 128, 128, 128),

        // check cases that may cause overflow
        RGBA8(0, 0, 0, 0),
        RGBA8(255, 255, 255, 255),
    }};
}  // namespace

// Test compilation and usage of the fixture
TEST_P(ColorStateTest, Basic) {
    dawn::BlendDescriptor blend;
    blend.operation = dawn::BlendOperation::Add;
    blend.srcFactor = dawn::BlendFactor::One;
    blend.dstFactor = dawn::BlendFactor::Zero;
    dawn::ColorStateDescriptor descriptor;
    descriptor.alphaBlend = blend;
    descriptor.colorBlend = blend;
    descriptor.writeMask = dawn::ColorWriteMask::All;

    SetupSingleSourcePipelines(descriptor);

    DoSingleSourceTest(RGBA8(0, 0, 0, 0), {RGBA8(255, 0, 0, 0)}, RGBA8(255, 0, 0, 0));
}

// The following tests check test that the blend operation works
TEST_P(ColorStateTest, BlendOperationAdd) {
    RGBA8 base(32, 64, 128, 192);
    std::vector<std::pair<RGBA8, RGBA8>> tests;
    std::transform(kColors.begin(), kColors.end(), std::back_inserter(tests),
                   [&](const RGBA8& color) { return std::make_pair(color, base + color); });
    CheckBlendOperation(base, dawn::BlendOperation::Add, tests);
}

TEST_P(ColorStateTest, BlendOperationSubtract) {
    RGBA8 base(32, 64, 128, 192);
    std::vector<std::pair<RGBA8, RGBA8>> tests;
    std::transform(kColors.begin(), kColors.end(), std::back_inserter(tests),
                   [&](const RGBA8& color) { return std::make_pair(color, color - base); });
    CheckBlendOperation(base, dawn::BlendOperation::Subtract, tests);
}

TEST_P(ColorStateTest, BlendOperationReverseSubtract) {
    RGBA8 base(32, 64, 128, 192);
    std::vector<std::pair<RGBA8, RGBA8>> tests;
    std::transform(kColors.begin(), kColors.end(), std::back_inserter(tests),
                   [&](const RGBA8& color) { return std::make_pair(color, base - color); });
    CheckBlendOperation(base, dawn::BlendOperation::ReverseSubtract, tests);
}

TEST_P(ColorStateTest, BlendOperationMin) {
    RGBA8 base(32, 64, 128, 192);
    std::vector<std::pair<RGBA8, RGBA8>> tests;
    std::transform(kColors.begin(), kColors.end(), std::back_inserter(tests),
                   [&](const RGBA8& color) { return std::make_pair(color, min(base, color)); });
    CheckBlendOperation(base, dawn::BlendOperation::Min, tests);
}

TEST_P(ColorStateTest, BlendOperationMax) {
    RGBA8 base(32, 64, 128, 192);
    std::vector<std::pair<RGBA8, RGBA8>> tests;
    std::transform(kColors.begin(), kColors.end(), std::back_inserter(tests),
                   [&](const RGBA8& color) { return std::make_pair(color, max(base, color)); });
    CheckBlendOperation(base, dawn::BlendOperation::Max, tests);
}

// The following tests check that the Source blend factor works
TEST_P(ColorStateTest, SrcBlendFactorZero) {
    RGBA8 base(32, 64, 128, 192);
    std::vector<std::pair<TriangleSpec, RGBA8>> tests;
    std::transform(
        kColors.begin(), kColors.end(), std::back_inserter(tests),
        [&](const RGBA8& color) { return std::make_pair(TriangleSpec({{color}}), base); });
    CheckSrcBlendFactor(base, dawn::BlendFactor::Zero, dawn::BlendFactor::Zero, tests);
}

TEST_P(ColorStateTest, SrcBlendFactorOne) {
    RGBA8 base(32, 64, 128, 192);
    std::vector<std::pair<TriangleSpec, RGBA8>> tests;
    std::transform(
        kColors.begin(), kColors.end(), std::back_inserter(tests),
        [&](const RGBA8& color) { return std::make_pair(TriangleSpec({{color}}), base + color); });
    CheckSrcBlendFactor(base, dawn::BlendFactor::One, dawn::BlendFactor::One, tests);
}

TEST_P(ColorStateTest, SrcBlendFactorSrcColor) {
    RGBA8 base(32, 64, 128, 192);
    std::vector<std::pair<TriangleSpec, RGBA8>> tests;
    std::transform(kColors.begin(), kColors.end(), std::back_inserter(tests),
                   [&](const RGBA8& color) {
                       RGBA8 fac = color;
                       fac.a = 0;
                       RGBA8 expected = base + mix(RGBA8(0, 0, 0, 0), color, fac);
                       return std::make_pair(TriangleSpec({{color}}), expected);
                   });
    CheckSrcBlendFactor(base, dawn::BlendFactor::SrcColor, dawn::BlendFactor::Zero, tests);
}

TEST_P(ColorStateTest, SrcBlendFactorOneMinusSrcColor) {
    RGBA8 base(32, 64, 128, 192);
    std::vector<std::pair<TriangleSpec, RGBA8>> tests;
    std::transform(kColors.begin(), kColors.end(), std::back_inserter(tests),
                   [&](const RGBA8& color) {
                       RGBA8 fac = RGBA8(255, 255, 255, 255) - color;
                       fac.a = 0;
                       RGBA8 expected = base + mix(RGBA8(0, 0, 0, 0), color, fac);
                       return std::make_pair(TriangleSpec({{color}}), expected);
                   });
    CheckSrcBlendFactor(base, dawn::BlendFactor::OneMinusSrcColor, dawn::BlendFactor::Zero, tests);
}

TEST_P(ColorStateTest, SrcBlendFactorSrcAlpha) {
    RGBA8 base(32, 64, 128, 192);
    std::vector<std::pair<TriangleSpec, RGBA8>> tests;
    std::transform(kColors.begin(), kColors.end(), std::back_inserter(tests),
                   [&](const RGBA8& color) {
                       RGBA8 fac(color.a, color.a, color.a, color.a);
                       RGBA8 expected = base + mix(RGBA8(0, 0, 0, 0), color, fac);
                       return std::make_pair(TriangleSpec({{color}}), expected);
                   });
    CheckSrcBlendFactor(base, dawn::BlendFactor::SrcAlpha, dawn::BlendFactor::SrcAlpha, tests);
}

TEST_P(ColorStateTest, SrcBlendFactorOneMinusSrcAlpha) {
    RGBA8 base(32, 64, 128, 192);
    std::vector<std::pair<TriangleSpec, RGBA8>> tests;
    std::transform(
        kColors.begin(), kColors.end(), std::back_inserter(tests), [&](const RGBA8& color) {
            RGBA8 fac = RGBA8(255, 255, 255, 255) - RGBA8(color.a, color.a, color.a, color.a);
            RGBA8 expected = base + mix(RGBA8(0, 0, 0, 0), color, fac);
            return std::make_pair(TriangleSpec({{color}}), expected);
        });
    CheckSrcBlendFactor(base, dawn::BlendFactor::OneMinusSrcAlpha,
                        dawn::BlendFactor::OneMinusSrcAlpha, tests);
}

TEST_P(ColorStateTest, SrcBlendFactorDstColor) {
    RGBA8 base(32, 64, 128, 192);
    std::vector<std::pair<TriangleSpec, RGBA8>> tests;
    std::transform(kColors.begin(), kColors.end(), std::back_inserter(tests),
                   [&](const RGBA8& color) {
                       RGBA8 fac = base;
                       fac.a = 0;
                       RGBA8 expected = base + mix(RGBA8(0, 0, 0, 0), color, fac);
                       return std::make_pair(TriangleSpec({{color}}), expected);
                   });
    CheckSrcBlendFactor(base, dawn::BlendFactor::DstColor, dawn::BlendFactor::Zero, tests);
}

TEST_P(ColorStateTest, SrcBlendFactorOneMinusDstColor) {
    RGBA8 base(32, 64, 128, 192);
    std::vector<std::pair<TriangleSpec, RGBA8>> tests;
    std::transform(kColors.begin(), kColors.end(), std::back_inserter(tests),
                   [&](const RGBA8& color) {
                       RGBA8 fac = RGBA8(255, 255, 255, 255) - base;
                       fac.a = 0;
                       RGBA8 expected = base + mix(RGBA8(0, 0, 0, 0), color, fac);
                       return std::make_pair(TriangleSpec({{color}}), expected);
                   });
    CheckSrcBlendFactor(base, dawn::BlendFactor::OneMinusDstColor, dawn::BlendFactor::Zero, tests);
}

TEST_P(ColorStateTest, SrcBlendFactorDstAlpha) {
    RGBA8 base(32, 64, 128, 192);
    std::vector<std::pair<TriangleSpec, RGBA8>> tests;
    std::transform(kColors.begin(), kColors.end(), std::back_inserter(tests),
                   [&](const RGBA8& color) {
                       RGBA8 fac(base.a, base.a, base.a, base.a);
                       RGBA8 expected = base + mix(RGBA8(0, 0, 0, 0), color, fac);
                       return std::make_pair(TriangleSpec({{color}}), expected);
                   });
    CheckSrcBlendFactor(base, dawn::BlendFactor::DstAlpha, dawn::BlendFactor::DstAlpha, tests);
}

TEST_P(ColorStateTest, SrcBlendFactorOneMinusDstAlpha) {
    RGBA8 base(32, 64, 128, 192);
    std::vector<std::pair<TriangleSpec, RGBA8>> tests;
    std::transform(
        kColors.begin(), kColors.end(), std::back_inserter(tests), [&](const RGBA8& color) {
            RGBA8 fac = RGBA8(255, 255, 255, 255) - RGBA8(base.a, base.a, base.a, base.a);
            RGBA8 expected = base + mix(RGBA8(0, 0, 0, 0), color, fac);
            return std::make_pair(TriangleSpec({{color}}), expected);
        });
    CheckSrcBlendFactor(base, dawn::BlendFactor::OneMinusDstAlpha,
                        dawn::BlendFactor::OneMinusDstAlpha, tests);
}

TEST_P(ColorStateTest, SrcBlendFactorSrcAlphaSaturated) {
    RGBA8 base(32, 64, 128, 192);
    std::vector<std::pair<TriangleSpec, RGBA8>> tests;
    std::transform(kColors.begin(), kColors.end(), std::back_inserter(tests),
                   [&](const RGBA8& color) {
                       uint8_t f = std::min(color.a, static_cast<uint8_t>(255 - base.a));
                       RGBA8 fac(f, f, f, 255);
                       RGBA8 expected = base + mix(RGBA8(0, 0, 0, 0), color, fac);
                       return std::make_pair(TriangleSpec({{color}}), expected);
                   });
    CheckSrcBlendFactor(base, dawn::BlendFactor::SrcAlphaSaturated,
                        dawn::BlendFactor::SrcAlphaSaturated, tests);
}

TEST_P(ColorStateTest, SrcBlendFactorBlendColor) {
    RGBA8 base(32, 64, 128, 192);
    std::vector<std::pair<TriangleSpec, RGBA8>> tests;
    std::transform(
        kColors.begin(), kColors.end(), std::back_inserter(tests), [&](const RGBA8& color) {
            auto triangleSpec = TriangleSpec({{color}, {{0.2f, 0.4f, 0.6f, 0.8f}}});
            RGBA8 expected = base + mix(RGBA8(0, 0, 0, 0), color, triangleSpec.blendFactor);
            return std::make_pair(triangleSpec, expected);
        });
    CheckSrcBlendFactor(base, dawn::BlendFactor::BlendColor, dawn::BlendFactor::BlendColor, tests);
}

TEST_P(ColorStateTest, SrcBlendFactorOneMinusBlendColor) {
    RGBA8 base(32, 64, 128, 192);
    std::vector<std::pair<TriangleSpec, RGBA8>> tests;
    std::transform(kColors.begin(), kColors.end(), std::back_inserter(tests),
                   [&](const RGBA8& color) {
                       auto triangleSpec = TriangleSpec({{color}, {{0.2f, 0.4f, 0.6f, 0.8f}}});
                       std::array<float, 4> f = {{0.8f, 0.6f, 0.4f, 0.2f}};
                       RGBA8 expected = base + mix(RGBA8(0, 0, 0, 0), color, f);
                       return std::make_pair(triangleSpec, expected);
                   });
    CheckSrcBlendFactor(base, dawn::BlendFactor::OneMinusBlendColor,
                        dawn::BlendFactor::OneMinusBlendColor, tests);
}

// The following tests check that the Destination blend factor works
TEST_P(ColorStateTest, DstBlendFactorZero) {
    RGBA8 base(32, 64, 128, 192);
    std::vector<std::pair<TriangleSpec, RGBA8>> tests;
    std::transform(
        kColors.begin(), kColors.end(), std::back_inserter(tests),
        [&](const RGBA8& color) { return std::make_pair(TriangleSpec({{color}}), color); });
    CheckDstBlendFactor(base, dawn::BlendFactor::Zero, dawn::BlendFactor::Zero, tests);
}

TEST_P(ColorStateTest, DstBlendFactorOne) {
    RGBA8 base(32, 64, 128, 192);
    std::vector<std::pair<TriangleSpec, RGBA8>> tests;
    std::transform(
        kColors.begin(), kColors.end(), std::back_inserter(tests),
        [&](const RGBA8& color) { return std::make_pair(TriangleSpec({{color}}), base + color); });
    CheckDstBlendFactor(base, dawn::BlendFactor::One, dawn::BlendFactor::One, tests);
}

TEST_P(ColorStateTest, DstBlendFactorSrcColor) {
    RGBA8 base(32, 64, 128, 192);
    std::vector<std::pair<TriangleSpec, RGBA8>> tests;
    std::transform(kColors.begin(), kColors.end(), std::back_inserter(tests),
                   [&](const RGBA8& color) {
                       RGBA8 fac = color;
                       fac.a = 0;
                       RGBA8 expected = color + mix(RGBA8(0, 0, 0, 0), base, fac);
                       return std::make_pair(TriangleSpec({{color}}), expected);
                   });
    CheckDstBlendFactor(base, dawn::BlendFactor::SrcColor, dawn::BlendFactor::Zero, tests);
}

TEST_P(ColorStateTest, DstBlendFactorOneMinusSrcColor) {
    RGBA8 base(32, 64, 128, 192);
    std::vector<std::pair<TriangleSpec, RGBA8>> tests;
    std::transform(kColors.begin(), kColors.end(), std::back_inserter(tests),
                   [&](const RGBA8& color) {
                       RGBA8 fac = RGBA8(255, 255, 255, 255) - color;
                       fac.a = 0;
                       RGBA8 expected = color + mix(RGBA8(0, 0, 0, 0), base, fac);
                       return std::make_pair(TriangleSpec({{color}}), expected);
                   });
    CheckDstBlendFactor(base, dawn::BlendFactor::OneMinusSrcColor, dawn::BlendFactor::Zero, tests);
}

TEST_P(ColorStateTest, DstBlendFactorSrcAlpha) {
    RGBA8 base(32, 64, 128, 192);
    std::vector<std::pair<TriangleSpec, RGBA8>> tests;
    std::transform(kColors.begin(), kColors.end(), std::back_inserter(tests),
                   [&](const RGBA8& color) {
                       RGBA8 fac(color.a, color.a, color.a, color.a);
                       RGBA8 expected = color + mix(RGBA8(0, 0, 0, 0), base, fac);
                       return std::make_pair(TriangleSpec({{color}}), expected);
                   });
    CheckDstBlendFactor(base, dawn::BlendFactor::SrcAlpha, dawn::BlendFactor::SrcAlpha, tests);
}

TEST_P(ColorStateTest, DstBlendFactorOneMinusSrcAlpha) {
    RGBA8 base(32, 64, 128, 192);
    std::vector<std::pair<TriangleSpec, RGBA8>> tests;
    std::transform(
        kColors.begin(), kColors.end(), std::back_inserter(tests), [&](const RGBA8& color) {
            RGBA8 fac = RGBA8(255, 255, 255, 255) - RGBA8(color.a, color.a, color.a, color.a);
            RGBA8 expected = color + mix(RGBA8(0, 0, 0, 0), base, fac);
            return std::make_pair(TriangleSpec({{color}}), expected);
        });
    CheckDstBlendFactor(base, dawn::BlendFactor::OneMinusSrcAlpha,
                        dawn::BlendFactor::OneMinusSrcAlpha, tests);
}

TEST_P(ColorStateTest, DstBlendFactorDstColor) {
    RGBA8 base(32, 64, 128, 192);
    std::vector<std::pair<TriangleSpec, RGBA8>> tests;
    std::transform(kColors.begin(), kColors.end(), std::back_inserter(tests),
                   [&](const RGBA8& color) {
                       RGBA8 fac = base;
                       fac.a = 0;
                       RGBA8 expected = color + mix(RGBA8(0, 0, 0, 0), base, fac);
                       return std::make_pair(TriangleSpec({{color}}), expected);
                   });
    CheckDstBlendFactor(base, dawn::BlendFactor::DstColor, dawn::BlendFactor::Zero, tests);
}

TEST_P(ColorStateTest, DstBlendFactorOneMinusDstColor) {
    RGBA8 base(32, 64, 128, 192);
    std::vector<std::pair<TriangleSpec, RGBA8>> tests;
    std::transform(kColors.begin(), kColors.end(), std::back_inserter(tests),
                   [&](const RGBA8& color) {
                       RGBA8 fac = RGBA8(255, 255, 255, 255) - base;
                       fac.a = 0;
                       RGBA8 expected = color + mix(RGBA8(0, 0, 0, 0), base, fac);
                       return std::make_pair(TriangleSpec({{color}}), expected);
                   });
    CheckDstBlendFactor(base, dawn::BlendFactor::OneMinusDstColor, dawn::BlendFactor::Zero, tests);
}

TEST_P(ColorStateTest, DstBlendFactorDstAlpha) {
    RGBA8 base(32, 64, 128, 192);
    std::vector<std::pair<TriangleSpec, RGBA8>> tests;
    std::transform(kColors.begin(), kColors.end(), std::back_inserter(tests),
                   [&](const RGBA8& color) {
                       RGBA8 fac(base.a, base.a, base.a, base.a);
                       RGBA8 expected = color + mix(RGBA8(0, 0, 0, 0), base, fac);
                       return std::make_pair(TriangleSpec({{color}}), expected);
                   });
    CheckDstBlendFactor(base, dawn::BlendFactor::DstAlpha, dawn::BlendFactor::DstAlpha, tests);
}

TEST_P(ColorStateTest, DstBlendFactorOneMinusDstAlpha) {
    RGBA8 base(32, 64, 128, 192);
    std::vector<std::pair<TriangleSpec, RGBA8>> tests;
    std::transform(
        kColors.begin(), kColors.end(), std::back_inserter(tests), [&](const RGBA8& color) {
            RGBA8 fac = RGBA8(255, 255, 255, 255) - RGBA8(base.a, base.a, base.a, base.a);
            RGBA8 expected = color + mix(RGBA8(0, 0, 0, 0), base, fac);
            return std::make_pair(TriangleSpec({{color}}), expected);
        });
    CheckDstBlendFactor(base, dawn::BlendFactor::OneMinusDstAlpha,
                        dawn::BlendFactor::OneMinusDstAlpha, tests);
}

TEST_P(ColorStateTest, DstBlendFactorSrcAlphaSaturated) {
    RGBA8 base(32, 64, 128, 192);
    std::vector<std::pair<TriangleSpec, RGBA8>> tests;
    std::transform(kColors.begin(), kColors.end(), std::back_inserter(tests),
                   [&](const RGBA8& color) {
                       uint8_t f = std::min(color.a, static_cast<uint8_t>(255 - base.a));
                       RGBA8 fac(f, f, f, 255);
                       RGBA8 expected = color + mix(RGBA8(0, 0, 0, 0), base, fac);
                       return std::make_pair(TriangleSpec({{color}}), expected);
                   });
    CheckDstBlendFactor(base, dawn::BlendFactor::SrcAlphaSaturated,
                        dawn::BlendFactor::SrcAlphaSaturated, tests);
}

TEST_P(ColorStateTest, DstBlendFactorBlendColor) {
    RGBA8 base(32, 64, 128, 192);
    std::vector<std::pair<TriangleSpec, RGBA8>> tests;
    std::transform(
        kColors.begin(), kColors.end(), std::back_inserter(tests), [&](const RGBA8& color) {
            auto triangleSpec = TriangleSpec({{color}, {{0.2f, 0.4f, 0.6f, 0.8f}}});
            RGBA8 expected = color + mix(RGBA8(0, 0, 0, 0), base, triangleSpec.blendFactor);
            return std::make_pair(triangleSpec, expected);
        });
    CheckDstBlendFactor(base, dawn::BlendFactor::BlendColor, dawn::BlendFactor::BlendColor, tests);
}

TEST_P(ColorStateTest, DstBlendFactorOneMinusBlendColor) {
    RGBA8 base(32, 64, 128, 192);
    std::vector<std::pair<TriangleSpec, RGBA8>> tests;
    std::transform(kColors.begin(), kColors.end(), std::back_inserter(tests),
                   [&](const RGBA8& color) {
                       auto triangleSpec = TriangleSpec({{color}, {{0.2f, 0.4f, 0.6f, 0.8f}}});
                       std::array<float, 4> f = {{0.8f, 0.6f, 0.4f, 0.2f}};
                       RGBA8 expected = color + mix(RGBA8(0, 0, 0, 0), base, f);
                       return std::make_pair(triangleSpec, expected);
                   });
    CheckDstBlendFactor(base, dawn::BlendFactor::OneMinusBlendColor,
                        dawn::BlendFactor::OneMinusBlendColor, tests);
}

// Check that the color write mask works
TEST_P(ColorStateTest, ColorWriteMask) {
    dawn::BlendDescriptor blend;
    blend.operation = dawn::BlendOperation::Add;
    blend.srcFactor = dawn::BlendFactor::One;
    blend.dstFactor = dawn::BlendFactor::One;

    dawn::ColorStateDescriptor descriptor;
    descriptor.colorBlend = blend;
    descriptor.alphaBlend = blend;
    {
        // Test single channel color write
        descriptor.writeMask = dawn::ColorWriteMask::Red;
        SetupSingleSourcePipelines(descriptor);

        RGBA8 base(32, 64, 128, 192);
        for (auto& color : kColors) {
            RGBA8 expected = base + RGBA8(color.r, 0, 0, 0);
            DoSingleSourceTest(base, {color}, expected);
        }
    }

    {
        // Test multi channel color write
        descriptor.writeMask = dawn::ColorWriteMask::Green | dawn::ColorWriteMask::Alpha;
        SetupSingleSourcePipelines(descriptor);

        RGBA8 base(32, 64, 128, 192);
        for (auto& color : kColors) {
            RGBA8 expected = base + RGBA8(0, color.g, 0, color.a);
            DoSingleSourceTest(base, {color}, expected);
        }
    }

    {
        // Test no channel color write
        descriptor.writeMask = dawn::ColorWriteMask::None;
        SetupSingleSourcePipelines(descriptor);

        RGBA8 base(32, 64, 128, 192);
        for (auto& color : kColors) {
            DoSingleSourceTest(base, {color}, base);
        }
    }
}

// Check that the color write mask works when blending is disabled
TEST_P(ColorStateTest, ColorWriteMaskBlendingDisabled) {
    {
        dawn::BlendDescriptor blend;
        blend.operation = dawn::BlendOperation::Add;
        blend.srcFactor = dawn::BlendFactor::One;
        blend.dstFactor = dawn::BlendFactor::Zero;
        dawn::ColorStateDescriptor descriptor;
        descriptor.alphaBlend = blend;
        descriptor.colorBlend = blend;

        descriptor.writeMask = dawn::ColorWriteMask::Red;
        SetupSingleSourcePipelines(descriptor);

        RGBA8 base(32, 64, 128, 192);
        RGBA8 expected(32, 0, 0, 0);

        dawn::CommandEncoder encoder = device.CreateCommandEncoder();
        {
            dawn::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
            pass.SetPipeline(testPipeline);
            pass.SetBindGroup(0, MakeBindGroupForColors(std::array<RGBA8, 1>({{base}})), 0, nullptr);
            pass.Draw(3, 1, 0, 0);
            pass.EndPass();
        }

        dawn::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);
        EXPECT_PIXEL_RGBA8_EQ(expected, renderPass.color, kRTSize / 2, kRTSize / 2);
    }
}

// Test that independent color states on render targets works
TEST_P(ColorStateTest, IndependentColorState) {
    DAWN_SKIP_TEST_IF(IsWindows() && IsVulkan() && IsIntel());

    std::array<dawn::Texture, 4> renderTargets;
    std::array<dawn::TextureView, 4> renderTargetViews;

    dawn::TextureDescriptor descriptor;
    descriptor.dimension = dawn::TextureDimension::e2D;
    descriptor.size.width = kRTSize;
    descriptor.size.height = kRTSize;
    descriptor.size.depth = 1;
    descriptor.arrayLayerCount = 1;
    descriptor.sampleCount = 1;
    descriptor.format = dawn::TextureFormat::R8G8B8A8Unorm;
    descriptor.mipLevelCount = 1;
    descriptor.usage = dawn::TextureUsageBit::OutputAttachment | dawn::TextureUsageBit::TransferSrc;

    for (uint32_t i = 0; i < 4; ++i) {
        renderTargets[i] = device.CreateTexture(&descriptor);
        renderTargetViews[i] = renderTargets[i].CreateDefaultView();
    }

    utils::ComboRenderPassDescriptor renderPass({renderTargetViews[0], renderTargetViews[1],
                                                renderTargetViews[2], renderTargetViews[3]});

    dawn::ShaderModule fsModule = utils::CreateShaderModule(device, dawn::ShaderStage::Fragment, R"(
        #version 450
        layout(set = 0, binding = 0) uniform myBlock {
            vec4 color0;
            vec4 color1;
            vec4 color2;
            vec4 color3;
        } myUbo;

        layout(location = 0) out vec4 fragColor0;
        layout(location = 1) out vec4 fragColor1;
        layout(location = 2) out vec4 fragColor2;
        layout(location = 3) out vec4 fragColor3;

        void main() {
            fragColor0 = myUbo.color0;
            fragColor1 = myUbo.color1;
            fragColor2 = myUbo.color2;
            fragColor3 = myUbo.color3;
        }
    )");

    utils::ComboRenderPipelineDescriptor baseDescriptor(device);
    baseDescriptor.layout = pipelineLayout;
    baseDescriptor.cVertexStage.module = vsModule;
    baseDescriptor.cFragmentStage.module = fsModule;
    baseDescriptor.colorStateCount = 4;

    basePipeline = device.CreateRenderPipeline(&baseDescriptor);

    utils::ComboRenderPipelineDescriptor testDescriptor(device);
    testDescriptor.layout = pipelineLayout;
    testDescriptor.cVertexStage.module = vsModule;
    testDescriptor.cFragmentStage.module = fsModule;
    testDescriptor.colorStateCount = 4;

    // set color states
    dawn::BlendDescriptor blend1;
    blend1.operation = dawn::BlendOperation::Add;
    blend1.srcFactor = dawn::BlendFactor::One;
    blend1.dstFactor = dawn::BlendFactor::One;

    dawn::BlendDescriptor blend2;
    blend2.operation = dawn::BlendOperation::Subtract;
    blend2.srcFactor = dawn::BlendFactor::One;
    blend2.dstFactor = dawn::BlendFactor::One;

    dawn::BlendDescriptor blend3;
    blend3.operation = dawn::BlendOperation::Min;
    blend3.srcFactor = dawn::BlendFactor::One;
    blend3.dstFactor = dawn::BlendFactor::One;

    testDescriptor.cColorStates[0]->colorBlend = blend1;
    testDescriptor.cColorStates[0]->alphaBlend = blend1;

    testDescriptor.cColorStates[1]->colorBlend = blend2;
    testDescriptor.cColorStates[1]->alphaBlend = blend2;

    testDescriptor.cColorStates[3]->colorBlend = blend3;
    testDescriptor.cColorStates[3]->alphaBlend = blend3;

    testPipeline = device.CreateRenderPipeline(&testDescriptor);

    for (unsigned int c = 0; c < kColors.size(); ++c) {
        RGBA8 base = kColors[((c + 31) * 29) % kColors.size()];
        RGBA8 color0 = kColors[((c + 19) * 13) % kColors.size()];
        RGBA8 color1 = kColors[((c + 11) * 43) % kColors.size()];
        RGBA8 color2 = kColors[((c + 7) * 3) % kColors.size()];
        RGBA8 color3 = kColors[((c + 13) * 71) % kColors.size()];

        RGBA8 expected0 = color0 + base;
        RGBA8 expected1 = color1 - base;
        RGBA8 expected2 = color2;
        RGBA8 expected3 = min(color3, base);

        dawn::CommandEncoder encoder = device.CreateCommandEncoder();
        {
            dawn::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
            pass.SetPipeline(basePipeline);
            pass.SetBindGroup(
                0, MakeBindGroupForColors(std::array<RGBA8, 4>({{base, base, base, base}})), 0, nullptr);
            pass.Draw(3, 1, 0, 0);

            pass.SetPipeline(testPipeline);
            pass.SetBindGroup(0, MakeBindGroupForColors(
                                     std::array<RGBA8, 4>({{color0, color1, color2, color3}})), 0, nullptr);
            pass.Draw(3, 1, 0, 0);
            pass.EndPass();
        }

        dawn::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);

        EXPECT_PIXEL_RGBA8_EQ(expected0, renderTargets[0], kRTSize / 2, kRTSize / 2)
            << "Attachment slot 0 should have been " << color0 << " + " << base << " = "
            << expected0;
        EXPECT_PIXEL_RGBA8_EQ(expected1, renderTargets[1], kRTSize / 2, kRTSize / 2)
            << "Attachment slot 1 should have been " << color1 << " - " << base << " = "
            << expected1;
        EXPECT_PIXEL_RGBA8_EQ(expected2, renderTargets[2], kRTSize / 2, kRTSize / 2)
            << "Attachment slot 2 should have been " << color2 << " = " << expected2
            << "(no blending)";
        EXPECT_PIXEL_RGBA8_EQ(expected3, renderTargets[3], kRTSize / 2, kRTSize / 2)
            << "Attachment slot 3 should have been min(" << color3 << ", " << base
            << ") = " << expected3;
    }
}

// Test that the default blend color is correctly set at the beginning of every subpass
TEST_P(ColorStateTest, DefaultBlendColor) {
    dawn::ShaderModule fsModule = utils::CreateShaderModule(device, dawn::ShaderStage::Fragment, R"(
        #version 450
        layout(set = 0, binding = 0) uniform myBlock {
            vec4 color;
        } myUbo;

        layout(location = 0) out vec4 fragColor;

        void main() {
            fragColor = myUbo.color;
        }
    )");

    utils::ComboRenderPipelineDescriptor baseDescriptor(device);
    baseDescriptor.layout = pipelineLayout;
    baseDescriptor.cVertexStage.module = vsModule;
    baseDescriptor.cFragmentStage.module = fsModule;
    baseDescriptor.cColorStates[0]->format = renderPass.colorFormat;

    basePipeline = device.CreateRenderPipeline(&baseDescriptor);

    utils::ComboRenderPipelineDescriptor testDescriptor(device);
    testDescriptor.layout = pipelineLayout;
    testDescriptor.cVertexStage.module = vsModule;
    testDescriptor.cFragmentStage.module = fsModule;
    testDescriptor.cColorStates[0]->format = renderPass.colorFormat;

    dawn::BlendDescriptor blend;
    blend.operation = dawn::BlendOperation::Add;
    blend.srcFactor = dawn::BlendFactor::BlendColor;
    blend.dstFactor = dawn::BlendFactor::One;
    testDescriptor.cColorStates[0]->colorBlend = blend;
    testDescriptor.cColorStates[0]->alphaBlend = blend;

    testPipeline = device.CreateRenderPipeline(&testDescriptor);
    constexpr dawn::Color kWhite{1.0f, 1.0f, 1.0f, 1.0f};

    // Check that the initial blend color is (0,0,0,0)
    {
        dawn::CommandEncoder encoder = device.CreateCommandEncoder();
        {
            dawn::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
            pass.SetPipeline(basePipeline);
            pass.SetBindGroup(0,
                              MakeBindGroupForColors(std::array<RGBA8, 1>({{RGBA8(0, 0, 0, 0)}})), 0, nullptr);
            pass.Draw(3, 1, 0, 0);
            pass.SetPipeline(testPipeline);
            pass.SetBindGroup(
                0, MakeBindGroupForColors(std::array<RGBA8, 1>({{RGBA8(255, 255, 255, 255)}})), 0, nullptr);
            pass.Draw(3, 1, 0, 0);
            pass.EndPass();
        }

        dawn::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);

        EXPECT_PIXEL_RGBA8_EQ(RGBA8(0, 0, 0, 0), renderPass.color, kRTSize / 2, kRTSize / 2);
    }

    // Check that setting the blend color works
    {
        dawn::CommandEncoder encoder = device.CreateCommandEncoder();
        {
            dawn::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
            pass.SetPipeline(basePipeline);
            pass.SetBindGroup(0,
                              MakeBindGroupForColors(std::array<RGBA8, 1>({{RGBA8(0, 0, 0, 0)}})), 0, nullptr);
            pass.Draw(3, 1, 0, 0);
            pass.SetPipeline(testPipeline);
            pass.SetBlendColor(&kWhite);
            pass.SetBindGroup(
                0, MakeBindGroupForColors(std::array<RGBA8, 1>({{RGBA8(255, 255, 255, 255)}})), 0, nullptr);
            pass.Draw(3, 1, 0, 0);
            pass.EndPass();
        }

        dawn::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);

        EXPECT_PIXEL_RGBA8_EQ(RGBA8(255, 255, 255, 255), renderPass.color, kRTSize / 2,
                              kRTSize / 2);
    }

    // Check that the blend color is not inherited between render passes
    {
        dawn::CommandEncoder encoder = device.CreateCommandEncoder();
        {
            dawn::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
            pass.SetPipeline(basePipeline);
            pass.SetBindGroup(0,
                              MakeBindGroupForColors(std::array<RGBA8, 1>({{RGBA8(0, 0, 0, 0)}})), 0, nullptr);
            pass.Draw(3, 1, 0, 0);
            pass.SetPipeline(testPipeline);
            pass.SetBlendColor(&kWhite);
            pass.SetBindGroup(
                0, MakeBindGroupForColors(std::array<RGBA8, 1>({{RGBA8(255, 255, 255, 255)}})), 0, nullptr);
            pass.Draw(3, 1, 0, 0);
            pass.EndPass();
        }
        {
            dawn::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
            pass.SetPipeline(basePipeline);
            pass.SetBindGroup(0,
                              MakeBindGroupForColors(std::array<RGBA8, 1>({{RGBA8(0, 0, 0, 0)}})), 0, nullptr);
            pass.Draw(3, 1, 0, 0);
            pass.SetPipeline(testPipeline);
            pass.SetBindGroup(
                0, MakeBindGroupForColors(std::array<RGBA8, 1>({{RGBA8(255, 255, 255, 255)}})), 0, nullptr);
            pass.Draw(3, 1, 0, 0);
            pass.EndPass();
        }

        dawn::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);

        EXPECT_PIXEL_RGBA8_EQ(RGBA8(0, 0, 0, 0), renderPass.color, kRTSize / 2, kRTSize / 2);
    }
}

// This tests a problem in the OpenGL backend where a previous color write mask
// persisted and prevented a render pass loadOp from fully clearing the output
// attachment.
TEST_P(ColorStateTest, ColorWriteMaskDoesNotAffectRenderPassLoadOpClear) {
    dawn::ShaderModule fsModule = utils::CreateShaderModule(device, dawn::ShaderStage::Fragment, R"(
        #version 450
        layout(set = 0, binding = 0) uniform myBlock {
            vec4 color;
        } myUbo;

        layout(location = 0) out vec4 fragColor;

        void main() {
            fragColor = myUbo.color;
        }
    )");

    utils::ComboRenderPipelineDescriptor baseDescriptor(device);
    baseDescriptor.layout = pipelineLayout;
    baseDescriptor.cVertexStage.module = vsModule;
    baseDescriptor.cFragmentStage.module = fsModule;
    baseDescriptor.cColorStates[0]->format = renderPass.colorFormat;

    basePipeline = device.CreateRenderPipeline(&baseDescriptor);

    utils::ComboRenderPipelineDescriptor testDescriptor(device);
    testDescriptor.layout = pipelineLayout;
    testDescriptor.cVertexStage.module = vsModule;
    testDescriptor.cFragmentStage.module = fsModule;
    testDescriptor.cColorStates[0]->format = renderPass.colorFormat;
    testDescriptor.cColorStates[0]->writeMask = dawn::ColorWriteMask::Red;

    testPipeline = device.CreateRenderPipeline(&testDescriptor);

    RGBA8 base(32, 64, 128, 192);
    RGBA8 expected(0, 0, 0, 0);

    dawn::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        // Clear the output attachment to |base|
        dawn::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
        pass.SetPipeline(basePipeline);
        pass.SetBindGroup(0, MakeBindGroupForColors(std::array<RGBA8, 1>({{base}})), 0, nullptr);
        pass.Draw(3, 1, 0, 0);

        // Set a pipeline that will dirty the color write mask
        pass.SetPipeline(testPipeline);
        pass.EndPass();
    }
    {
        // This renderpass' loadOp should clear all channels of the output attachment
        dawn::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
        pass.EndPass();
    }
    dawn::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    EXPECT_PIXEL_RGBA8_EQ(expected, renderPass.color, kRTSize / 2, kRTSize / 2);
}

DAWN_INSTANTIATE_TEST(ColorStateTest, D3D12Backend, MetalBackend, OpenGLBackend, VulkanBackend);
