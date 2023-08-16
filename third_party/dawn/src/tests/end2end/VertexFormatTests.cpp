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

#include "tests/DawnTest.h"

#include "common/Assert.h"
#include "common/Math.h"
#include "utils/ComboRenderPipelineDescriptor.h"
#include "utils/WGPUHelpers.h"

// Vertex format tests all work the same way: the test will render a triangle.
// Each test will set up a vertex buffer, and the vertex shader will check that
// the vertex content is the same as what we expected. On success it outputs green,
// otherwise red.

constexpr uint32_t kRTSize = 400;
constexpr uint32_t kVertexNum = 3;

std::vector<uint16_t> Float32ToFloat16(std::vector<float> data) {
    std::vector<uint16_t> expectedData;
    for (auto& element : data) {
        expectedData.push_back(Float32ToFloat16(element));
    }
    return expectedData;
}

template <typename destType, typename srcType>
std::vector<destType> BitCast(std::vector<srcType> data) {
    std::vector<destType> expectedData;
    for (auto& element : data) {
        expectedData.push_back(BitCast(element));
    }
    return expectedData;
}

class VertexFormatTest : public DawnTest {
  protected:
    void SetUp() override {
        DawnTest::SetUp();

        renderPass = utils::CreateBasicRenderPass(device, kRTSize, kRTSize);
    }

    utils::BasicRenderPass renderPass;

    bool IsNormalizedFormat(wgpu::VertexFormat format) {
        switch (format) {
            case wgpu::VertexFormat::UChar2Norm:
            case wgpu::VertexFormat::UChar4Norm:
            case wgpu::VertexFormat::Char2Norm:
            case wgpu::VertexFormat::Char4Norm:
            case wgpu::VertexFormat::UShort2Norm:
            case wgpu::VertexFormat::UShort4Norm:
            case wgpu::VertexFormat::Short2Norm:
            case wgpu::VertexFormat::Short4Norm:
                return true;
            default:
                return false;
        }
    }

    bool IsUnsignedFormat(wgpu::VertexFormat format) {
        switch (format) {
            case wgpu::VertexFormat::UInt:
            case wgpu::VertexFormat::UChar2:
            case wgpu::VertexFormat::UChar4:
            case wgpu::VertexFormat::UShort2:
            case wgpu::VertexFormat::UShort4:
            case wgpu::VertexFormat::UInt2:
            case wgpu::VertexFormat::UInt3:
            case wgpu::VertexFormat::UInt4:
            case wgpu::VertexFormat::UChar2Norm:
            case wgpu::VertexFormat::UChar4Norm:
            case wgpu::VertexFormat::UShort2Norm:
            case wgpu::VertexFormat::UShort4Norm:
                return true;
            default:
                return false;
        }
    }

    bool IsFloatFormat(wgpu::VertexFormat format) {
        switch (format) {
            case wgpu::VertexFormat::Half2:
            case wgpu::VertexFormat::Half4:
            case wgpu::VertexFormat::Float:
            case wgpu::VertexFormat::Float2:
            case wgpu::VertexFormat::Float3:
            case wgpu::VertexFormat::Float4:
                return true;
            default:
                return false;
        }
    }

    bool IsHalfFormat(wgpu::VertexFormat format) {
        switch (format) {
            case wgpu::VertexFormat::Half2:
            case wgpu::VertexFormat::Half4:
                return true;
            default:
                return false;
        }
    }

    uint32_t BytesPerComponents(wgpu::VertexFormat format) {
        switch (format) {
            case wgpu::VertexFormat::Char2:
            case wgpu::VertexFormat::Char4:
            case wgpu::VertexFormat::UChar2:
            case wgpu::VertexFormat::UChar4:
            case wgpu::VertexFormat::UChar2Norm:
            case wgpu::VertexFormat::UChar4Norm:
            case wgpu::VertexFormat::Char2Norm:
            case wgpu::VertexFormat::Char4Norm:
                return 1;
            case wgpu::VertexFormat::UShort2:
            case wgpu::VertexFormat::UShort4:
            case wgpu::VertexFormat::Short2:
            case wgpu::VertexFormat::Short4:
            case wgpu::VertexFormat::UShort2Norm:
            case wgpu::VertexFormat::UShort4Norm:
            case wgpu::VertexFormat::Short2Norm:
            case wgpu::VertexFormat::Short4Norm:
            case wgpu::VertexFormat::Half2:
            case wgpu::VertexFormat::Half4:
                return 2;
            case wgpu::VertexFormat::UInt:
            case wgpu::VertexFormat::Int:
            case wgpu::VertexFormat::Float:
            case wgpu::VertexFormat::UInt2:
            case wgpu::VertexFormat::UInt3:
            case wgpu::VertexFormat::UInt4:
            case wgpu::VertexFormat::Int2:
            case wgpu::VertexFormat::Int3:
            case wgpu::VertexFormat::Int4:
            case wgpu::VertexFormat::Float2:
            case wgpu::VertexFormat::Float3:
            case wgpu::VertexFormat::Float4:
                return 4;
            default:
                DAWN_UNREACHABLE();
        }
    }

    uint32_t ComponentCount(wgpu::VertexFormat format) {
        switch (format) {
            case wgpu::VertexFormat::UInt:
            case wgpu::VertexFormat::Int:
            case wgpu::VertexFormat::Float:
                return 1;
            case wgpu::VertexFormat::UChar2:
            case wgpu::VertexFormat::UShort2:
            case wgpu::VertexFormat::UInt2:
            case wgpu::VertexFormat::Char2:
            case wgpu::VertexFormat::Short2:
            case wgpu::VertexFormat::Int2:
            case wgpu::VertexFormat::UChar2Norm:
            case wgpu::VertexFormat::Char2Norm:
            case wgpu::VertexFormat::UShort2Norm:
            case wgpu::VertexFormat::Short2Norm:
            case wgpu::VertexFormat::Half2:
            case wgpu::VertexFormat::Float2:
                return 2;
            case wgpu::VertexFormat::Int3:
            case wgpu::VertexFormat::UInt3:
            case wgpu::VertexFormat::Float3:
                return 3;
            case wgpu::VertexFormat::UChar4:
            case wgpu::VertexFormat::UShort4:
            case wgpu::VertexFormat::UInt4:
            case wgpu::VertexFormat::Char4:
            case wgpu::VertexFormat::Short4:
            case wgpu::VertexFormat::Int4:
            case wgpu::VertexFormat::UChar4Norm:
            case wgpu::VertexFormat::Char4Norm:
            case wgpu::VertexFormat::UShort4Norm:
            case wgpu::VertexFormat::Short4Norm:
            case wgpu::VertexFormat::Half4:
            case wgpu::VertexFormat::Float4:
                return 4;
            default:
                DAWN_UNREACHABLE();
        }
    }

    std::string ShaderTypeGenerator(bool isFloat,
                                    bool isNormalized,
                                    bool isUnsigned,
                                    uint32_t componentCount) {
        if (componentCount == 1) {
            if (isFloat || isNormalized) {
                return "float";
            } else if (isUnsigned) {
                return "uint";
            } else {
                return "int";
            }
        } else {
            if (isNormalized || isFloat) {
                return "vec" + std::to_string(componentCount);
            } else if (isUnsigned) {
                return "uvec" + std::to_string(componentCount);
            } else {
                return "ivec" + std::to_string(componentCount);
            }
        }
    }

    // The length of vertexData is fixed to 3, it aligns to triangle vertex number
    template <typename T>
    wgpu::RenderPipeline MakeTestPipeline(wgpu::VertexFormat format, std::vector<T>& expectedData) {
        bool isFloat = IsFloatFormat(format);
        bool isNormalized = IsNormalizedFormat(format);
        bool isUnsigned = IsUnsignedFormat(format);
        bool isInputTypeFloat = isFloat || isNormalized;
        bool isHalf = IsHalfFormat(format);
        const uint16_t kNegativeZeroInHalf = 0x8000;

        uint32_t componentCount = ComponentCount(format);

        std::string variableType =
            ShaderTypeGenerator(isFloat, isNormalized, isUnsigned, componentCount);
        std::string expectedDataType = ShaderTypeGenerator(isFloat, isNormalized, isUnsigned, 1);
        std::ostringstream vs;
        vs << "#version 450\n";

        // layout(location = 0) in float/uint/int/ivecn/vecn/uvecn test;
        vs << "layout(location = 0) in " << variableType << " test;\n";
        vs << "layout(location = 0) out vec4 color;\n";
        // Because x86 CPU using "extended
        // precision"(https://en.wikipedia.org/wiki/Extended_precision) during float
        // math(https://developer.nvidia.com/sites/default/files/akamai/cuda/files/NVIDIA-CUDA-Floating-Point.pdf),
        // move normalization and Float16ToFloat32 into shader to generate
        // expected value.
        vs << "float Float16ToFloat32(uint fp16) {\n";
        vs << "  uint magic = (uint(254) - uint(15)) << 23;\n";
        vs << "  uint was_inf_nan = (uint(127) + uint(16)) << 23;\n";
        vs << "  uint fp32u;\n";
        vs << "  float fp32;\n";
        vs << "  fp32u = (fp16 & 0x7FFF) << 13;\n";
        vs << "  fp32 = uintBitsToFloat(fp32u) * uintBitsToFloat(magic);\n";
        vs << "  fp32u = floatBitsToUint(fp32);\n";
        vs << "  if (fp32 >= uintBitsToFloat(was_inf_nan)) {\n";
        vs << "    fp32u |= uint(255) << 23;\n";
        vs << "  }\n";
        vs << "  fp32u |= (fp16 & 0x8000) << 16;\n";
        vs << "  fp32 = uintBitsToFloat(fp32u);\n";
        vs << "  return fp32;\n";
        vs << "}\n";

        vs << "void main() {\n";

        // Hard code the triangle in the shader so that we don't have to add a vertex input for it.
        vs << "    const vec2 pos[3] = vec2[3](vec2(-1.0f, 0.0f), vec2(-1.0f, 1.0f), vec2(0.0f, "
              "1.0f));\n";
        vs << "    gl_Position = vec4(pos[gl_VertexIndex], 0.0, 1.0);\n";

        // Declare expected values.
        vs << "    " << expectedDataType << " expected[" + std::to_string(kVertexNum) + "]";
        vs << "[" + std::to_string(componentCount) + "];\n";
        // Assign each elements in expected values
        // e.g. expected[0][0] = uint(1);
        //      expected[0][1] = uint(2);
        for (uint32_t i = 0; i < kVertexNum; ++i) {
            for (uint32_t j = 0; j < componentCount; ++j) {
                vs << "    expected[" + std::to_string(i) + "][" + std::to_string(j) + "] = "
                   << expectedDataType << "(";
                if (isInputTypeFloat &&
                    std::isnan(static_cast<float>(expectedData[i * componentCount + j]))) {
                    // Set NaN.
                    vs << "0.0 / 0.0);\n";
                } else if (isNormalized) {
                    // Move normalize operation into shader because of CPU and GPU precision
                    // different on float math.
                    vs << "max(float(" << std::to_string(expectedData[i * componentCount + j])
                       << ") / " << std::to_string(std::numeric_limits<T>::max()) << ", -1.0));\n";
                } else if (isHalf) {
                    // Becasue Vulkan and D3D12 handle -0.0f through uintBitsToFloat have different
                    // result (Vulkan take -0.0f as -0.0 but D3D12 take -0.0f as 0), add workaround
                    // for -0.0f.
                    if (static_cast<uint16_t>(expectedData[i * componentCount + j]) ==
                        kNegativeZeroInHalf) {
                        vs << "-0.0f);\n";
                    } else {
                        vs << "Float16ToFloat32("
                           << std::to_string(expectedData[i * componentCount + j]);
                        vs << "));\n";
                    }
                } else {
                    vs << std::to_string(expectedData[i * componentCount + j]) << ");\n";
                }
            }
        }

        vs << "    bool success = true;\n";
        // Perform the checks by successively ANDing a boolean
        for (uint32_t component = 0; component < componentCount; ++component) {
            std::string suffix = componentCount == 1 ? "" : "[" + std::to_string(component) + "]";
            std::string testVal = "testVal" + std::to_string(component);
            std::string expectedVal = "expectedVal" + std::to_string(component);
            vs << "    " << expectedDataType << " " << testVal << ";\n";
            vs << "    " << expectedDataType << " " << expectedVal << ";\n";
            vs << "    " << testVal << " = test" << suffix << ";\n";
            vs << "    " << expectedVal << " = expected[gl_VertexIndex]"
               << "[" << component << "];\n";
            if (!isInputTypeFloat) {  // Integer / unsigned integer need to match exactly.
                vs << "    success = success && (" << testVal << " == " << expectedVal << ");\n";
            } else {
                // TODO(shaobo.yan@intel.com) : a difference of 8 ULPs is allowed in this test
                // because it is required on MacbookPro 11.5,AMD Radeon HD 8870M(on macOS 10.13.6),
                // but that it might be possible to tighten.
                vs << "    if (isnan(" << expectedVal << ")) {\n";
                vs << "        success = success && isnan(" << testVal << ");\n";
                vs << "    } else {\n";
                vs << "        uint testValFloatToUint = floatBitsToUint(" << testVal << ");\n";
                vs << "        uint expectedValFloatToUint = floatBitsToUint(" << expectedVal
                   << ");\n";
                vs << "        success = success && max(testValFloatToUint, "
                      "expectedValFloatToUint)";
                vs << "        - min(testValFloatToUint, expectedValFloatToUint) < uint(8);\n";
                vs << "    }\n";
            }
        }
        vs << "    if (success) {\n";
        vs << "        color = vec4(0.0f, 1.0f, 0.0f, 1.0f);\n";
        vs << "    } else {\n";
        vs << "        color = vec4(1.0f, 0.0f, 0.0f, 1.0f);\n";
        vs << "    }\n";
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

        uint32_t bytesPerComponents = BytesPerComponents(format);
        uint32_t strideBytes = bytesPerComponents * componentCount;
        // Stride size must be multiple of 4 bytes.
        if (strideBytes % 4 != 0) {
            strideBytes += (4 - strideBytes % 4);
        }

        utils::ComboRenderPipelineDescriptor descriptor(device);
        descriptor.vertexStage.module = vsModule;
        descriptor.cFragmentStage.module = fsModule;
        descriptor.cVertexState.vertexBufferCount = 1;
        descriptor.cVertexState.cVertexBuffers[0].arrayStride = strideBytes;
        descriptor.cVertexState.cVertexBuffers[0].attributeCount = 1;
        descriptor.cVertexState.cAttributes[0].format = format;
        descriptor.cColorStates[0].format = renderPass.colorFormat;

        return device.CreateRenderPipeline(&descriptor);
    }

    template <typename VertexType, typename ExpectedType>
    void DoVertexFormatTest(wgpu::VertexFormat format,
                            std::vector<VertexType> vertex,
                            std::vector<ExpectedType> expectedData) {
        wgpu::RenderPipeline pipeline = MakeTestPipeline(format, expectedData);
        wgpu::Buffer vertexBuffer = utils::CreateBufferFromData(
            device, vertex.data(), vertex.size() * sizeof(VertexType), wgpu::BufferUsage::Vertex);
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        {
            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
            pass.SetPipeline(pipeline);
            pass.SetVertexBuffer(0, vertexBuffer);
            pass.Draw(3);
            pass.EndPass();
        }

        wgpu::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);

        EXPECT_PIXEL_RGBA8_EQ(RGBA8::kGreen, renderPass.color, 0, 0);
    }
};

TEST_P(VertexFormatTest, UChar2) {
    // TODO(cwallez@chromium.org): Failing because of a SPIRV-Cross issue.
    // See http://crbug.com/dawn/259
    DAWN_SKIP_TEST_IF(IsMetal() && IsIntel());

    std::vector<uint8_t> vertexData = {
        std::numeric_limits<uint8_t>::max(),
        0,
        0,  // padding two bytes for stride
        0,
        std::numeric_limits<uint8_t>::min(),
        2,
        0,
        0,  // padding two bytes for stride
        200,
        201,
        0,
        0  // padding two bytes for buffer copy
    };

    std::vector<uint8_t> expectedData = {
        std::numeric_limits<uint8_t>::max(), 0, std::numeric_limits<uint8_t>::min(), 2, 200, 201,
    };

    DoVertexFormatTest(wgpu::VertexFormat::UChar2, vertexData, expectedData);
}

TEST_P(VertexFormatTest, UChar4) {
    // TODO(cwallez@chromium.org): Failing because of a SPIRV-Cross issue.
    // See http://crbug.com/dawn/259
    DAWN_SKIP_TEST_IF(IsMetal() && IsIntel());

    std::vector<uint8_t> vertexData = {
        std::numeric_limits<uint8_t>::max(),
        0,
        1,
        2,
        std::numeric_limits<uint8_t>::min(),
        2,
        3,
        4,
        200,
        201,
        202,
        203,
    };

    DoVertexFormatTest(wgpu::VertexFormat::UChar4, vertexData, vertexData);
}

TEST_P(VertexFormatTest, Char2) {
    // TODO(cwallez@chromium.org): Failing because of a SPIRV-Cross issue.
    // See http://crbug.com/dawn/259
    DAWN_SKIP_TEST_IF(IsMetal() && IsIntel());

    std::vector<int8_t> vertexData = {
        std::numeric_limits<int8_t>::max(),
        0,
        0,  // padding two bytes for stride
        0,
        std::numeric_limits<int8_t>::min(),
        -2,
        0,  // padding two bytes for stride
        0,
        120,
        -121,
        0,
        0  // padding two bytes for buffer copy
    };

    std::vector<int8_t> expectedData = {
        std::numeric_limits<int8_t>::max(), 0, std::numeric_limits<int8_t>::min(), -2, 120, -121,
    };

    DoVertexFormatTest(wgpu::VertexFormat::Char2, vertexData, expectedData);
}

TEST_P(VertexFormatTest, Char4) {
    // TODO(cwallez@chromium.org): Failing because of a SPIRV-Cross issue.
    // See http://crbug.com/dawn/259
    DAWN_SKIP_TEST_IF(IsMetal() && IsIntel());

    std::vector<int8_t> vertexData = {
        std::numeric_limits<int8_t>::max(),
        0,
        -1,
        2,
        std::numeric_limits<int8_t>::min(),
        -2,
        3,
        4,
        120,
        -121,
        122,
        -123,
    };

    DoVertexFormatTest(wgpu::VertexFormat::Char4, vertexData, vertexData);
}

TEST_P(VertexFormatTest, UChar2Norm) {
    // TODO(cwallez@chromium.org): Failing because of a SPIRV-Cross issue.
    // See http://crbug.com/dawn/259
    DAWN_SKIP_TEST_IF(IsMetal() && IsIntel());

    std::vector<uint8_t> vertexData = {
        std::numeric_limits<uint8_t>::max(),
        std::numeric_limits<uint8_t>::min(),
        0,  // padding two bytes for stride
        0,
        std::numeric_limits<uint8_t>::max() / 2u,
        std::numeric_limits<uint8_t>::min() / 2u,
        0,  // padding two bytes for stride
        0,
        200,
        201,
        0,
        0  // padding two bytes for buffer copy
    };

    std::vector<uint8_t> expectedData = {std::numeric_limits<uint8_t>::max(),
                                         std::numeric_limits<uint8_t>::min(),
                                         std::numeric_limits<uint8_t>::max() / 2u,
                                         std::numeric_limits<uint8_t>::min() / 2u,
                                         200,
                                         201};

    DoVertexFormatTest(wgpu::VertexFormat::UChar2Norm, vertexData, expectedData);
}

TEST_P(VertexFormatTest, UChar4Norm) {
    // TODO(cwallez@chromium.org): Failing because of a SPIRV-Cross issue.
    // See http://crbug.com/dawn/259
    DAWN_SKIP_TEST_IF(IsMetal() && IsIntel());

    std::vector<uint8_t> vertexData = {std::numeric_limits<uint8_t>::max(),
                                       std::numeric_limits<uint8_t>::min(),
                                       0,
                                       0,
                                       std::numeric_limits<uint8_t>::max() / 2u,
                                       std::numeric_limits<uint8_t>::min() / 2u,
                                       0,
                                       0,
                                       200,
                                       201,
                                       202,
                                       203};

    DoVertexFormatTest(wgpu::VertexFormat::UChar4Norm, vertexData, vertexData);
}

TEST_P(VertexFormatTest, Char2Norm) {
    // TODO(cwallez@chromium.org): Failing because of a SPIRV-Cross issue.
    // See http://crbug.com/dawn/259
    DAWN_SKIP_TEST_IF(IsMetal() && IsIntel());

    std::vector<int8_t> vertexData = {
        std::numeric_limits<int8_t>::max(),
        std::numeric_limits<int8_t>::min(),
        0,  // padding two bytes for stride
        0,
        std::numeric_limits<int8_t>::max() / 2,
        std::numeric_limits<int8_t>::min() / 2,
        0,  // padding two bytes for stride
        0,
        120,
        -121,
        0,
        0  // padding two bytes for buffer copy
    };

    std::vector<int8_t> expectedData = {
        std::numeric_limits<int8_t>::max(),
        std::numeric_limits<int8_t>::min(),
        std::numeric_limits<int8_t>::max() / 2,
        std::numeric_limits<int8_t>::min() / 2,
        120,
        -121,
    };

    DoVertexFormatTest(wgpu::VertexFormat::Char2Norm, vertexData, expectedData);
}

TEST_P(VertexFormatTest, Char4Norm) {
    // TODO(cwallez@chromium.org): Failing because of a SPIRV-Cross issue.
    // See http://crbug.com/dawn/259
    DAWN_SKIP_TEST_IF(IsMetal() && IsIntel());

    std::vector<int8_t> vertexData = {std::numeric_limits<int8_t>::max(),
                                      std::numeric_limits<int8_t>::min(),
                                      0,
                                      0,
                                      std::numeric_limits<int8_t>::max() / 2,
                                      std::numeric_limits<int8_t>::min() / 2,
                                      -2,
                                      2,
                                      120,
                                      -120,
                                      102,
                                      -123};

    DoVertexFormatTest(wgpu::VertexFormat::Char4Norm, vertexData, vertexData);
}

TEST_P(VertexFormatTest, UShort2) {
    // TODO(cwallez@chromium.org): Failing because of a SPIRV-Cross issue.
    // See http://crbug.com/dawn/259
    DAWN_SKIP_TEST_IF(IsMetal() && IsIntel());

    std::vector<uint16_t> vertexData = {std::numeric_limits<uint16_t>::max(),
                                        0,
                                        std::numeric_limits<uint16_t>::min(),
                                        2,
                                        65432,
                                        4890};

    DoVertexFormatTest(wgpu::VertexFormat::UShort2, vertexData, vertexData);
}

TEST_P(VertexFormatTest, UShort4) {
    // TODO(cwallez@chromium.org): Failing because of a SPIRV-Cross issue.
    // See http://crbug.com/dawn/259
    DAWN_SKIP_TEST_IF(IsMetal() && IsIntel());

    std::vector<uint16_t> vertexData = {
        std::numeric_limits<uint16_t>::max(),
        std::numeric_limits<uint8_t>::max(),
        1,
        2,
        std::numeric_limits<uint16_t>::min(),
        2,
        3,
        4,
        65520,
        65521,
        3435,
        3467,
    };

    DoVertexFormatTest(wgpu::VertexFormat::UShort4, vertexData, vertexData);
}

TEST_P(VertexFormatTest, Short2) {
    // TODO(cwallez@chromium.org): Failing because of a SPIRV-Cross issue.
    // See http://crbug.com/dawn/259
    DAWN_SKIP_TEST_IF(IsMetal() && IsIntel());

    std::vector<int16_t> vertexData = {std::numeric_limits<int16_t>::max(),
                                       0,
                                       std::numeric_limits<int16_t>::min(),
                                       -2,
                                       3876,
                                       -3948};

    DoVertexFormatTest(wgpu::VertexFormat::Short2, vertexData, vertexData);
}

TEST_P(VertexFormatTest, Short4) {
    // TODO(cwallez@chromium.org): Failing because of a SPIRV-Cross issue.
    // See http://crbug.com/dawn/259
    DAWN_SKIP_TEST_IF(IsMetal() && IsIntel());

    std::vector<int16_t> vertexData = {
        std::numeric_limits<int16_t>::max(),
        0,
        -1,
        2,
        std::numeric_limits<int16_t>::min(),
        -2,
        3,
        4,
        24567,
        -23545,
        4350,
        -2987,
    };

    DoVertexFormatTest(wgpu::VertexFormat::Short4, vertexData, vertexData);
}

TEST_P(VertexFormatTest, UShort2Norm) {
    // TODO(cwallez@chromium.org): Failing because of a SPIRV-Cross issue.
    // See http://crbug.com/dawn/259
    DAWN_SKIP_TEST_IF(IsMetal() && IsIntel());

    std::vector<uint16_t> vertexData = {std::numeric_limits<uint16_t>::max(),
                                        std::numeric_limits<uint16_t>::min(),
                                        std::numeric_limits<uint16_t>::max() / 2u,
                                        std::numeric_limits<uint16_t>::min() / 2u,
                                        3456,
                                        6543};

    DoVertexFormatTest(wgpu::VertexFormat::UShort2Norm, vertexData, vertexData);
}

TEST_P(VertexFormatTest, UShort4Norm) {
    // TODO(cwallez@chromium.org): Failing because of a SPIRV-Cross issue.
    // See http://crbug.com/dawn/259
    DAWN_SKIP_TEST_IF(IsMetal() && IsIntel());

    std::vector<uint16_t> vertexData = {std::numeric_limits<uint16_t>::max(),
                                        std::numeric_limits<uint16_t>::min(),
                                        0,
                                        0,
                                        std::numeric_limits<uint16_t>::max() / 2u,
                                        std::numeric_limits<uint16_t>::min() / 2u,
                                        0,
                                        0,
                                        2987,
                                        3055,
                                        2987,
                                        2987};

    DoVertexFormatTest(wgpu::VertexFormat::UShort4Norm, vertexData, vertexData);
}

TEST_P(VertexFormatTest, Short2Norm) {
    // TODO(cwallez@chromium.org): Failing because of a SPIRV-Cross issue.
    // See http://crbug.com/dawn/259
    DAWN_SKIP_TEST_IF(IsMetal() && IsIntel());

    std::vector<int16_t> vertexData = {std::numeric_limits<int16_t>::max(),
                                       std::numeric_limits<int16_t>::min(),
                                       std::numeric_limits<int16_t>::max() / 2,
                                       std::numeric_limits<int16_t>::min() / 2,
                                       4987,
                                       -6789};

    DoVertexFormatTest(wgpu::VertexFormat::Short2Norm, vertexData, vertexData);
}

TEST_P(VertexFormatTest, Short4Norm) {
    // TODO(cwallez@chromium.org): Failing because of a SPIRV-Cross issue.
    // See http://crbug.com/dawn/259
    DAWN_SKIP_TEST_IF(IsMetal() && IsIntel());

    std::vector<int16_t> vertexData = {std::numeric_limits<int16_t>::max(),
                                       std::numeric_limits<int16_t>::min(),
                                       0,
                                       0,
                                       std::numeric_limits<int16_t>::max() / 2,
                                       std::numeric_limits<int16_t>::min() / 2,
                                       -2,
                                       2,
                                       2890,
                                       -29011,
                                       20432,
                                       -2083};

    DoVertexFormatTest(wgpu::VertexFormat::Short4Norm, vertexData, vertexData);
}

TEST_P(VertexFormatTest, Half2) {
    // TODO(cwallez@chromium.org): Failing because of a SPIRV-Cross issue.
    // See http://crbug.com/dawn/259
    DAWN_SKIP_TEST_IF(IsMetal() && IsIntel());

    std::vector<uint16_t> vertexData =
        Float32ToFloat16(std::vector<float>({14.8f, -0.0f, 22.5f, 1.3f, +0.0f, -24.8f}));

    DoVertexFormatTest(wgpu::VertexFormat::Half2, vertexData, vertexData);
}

TEST_P(VertexFormatTest, Half4) {
    // TODO(cwallez@chromium.org): Failing because of a SPIRV-Cross issue.
    // See http://crbug.com/dawn/259
    DAWN_SKIP_TEST_IF(IsMetal() && IsIntel());

    std::vector<uint16_t> vertexData = Float32ToFloat16(std::vector<float>(
        {+0.0f, -16.8f, 18.2f, -0.0f, 12.5f, 1.3f, 14.8f, -12.4f, 22.5f, -48.8f, 47.4f, -24.8f}));

    DoVertexFormatTest(wgpu::VertexFormat::Half4, vertexData, vertexData);
}

TEST_P(VertexFormatTest, Float) {
    // TODO(cwallez@chromium.org): Failing because of a SPIRV-Cross issue.
    // See http://crbug.com/dawn/259
    DAWN_SKIP_TEST_IF(IsMetal() && IsIntel());

    std::vector<float> vertexData = {1.3f, +0.0f, -0.0f};

    DoVertexFormatTest(wgpu::VertexFormat::Float, vertexData, vertexData);

    vertexData = std::vector<float>{+1.0f, -1.0f, 18.23f};

    DoVertexFormatTest(wgpu::VertexFormat::Float, vertexData, vertexData);
}

TEST_P(VertexFormatTest, Float2) {
    // TODO(cwallez@chromium.org): Failing because of a SPIRV-Cross issue.
    // See http://crbug.com/dawn/259
    DAWN_SKIP_TEST_IF(IsMetal() && IsIntel());

    std::vector<float> vertexData = {18.23f, -0.0f, +0.0f, +1.0f, 1.3f, -1.0f};

    DoVertexFormatTest(wgpu::VertexFormat::Float2, vertexData, vertexData);
}

TEST_P(VertexFormatTest, Float3) {
    // TODO(cwallez@chromium.org): Failing because of a SPIRV-Cross issue.
    // See http://crbug.com/dawn/259
    DAWN_SKIP_TEST_IF(IsMetal() && IsIntel());

    std::vector<float> vertexData = {
        +0.0f, -1.0f, -0.0f, 1.0f, 1.3f, 99.45f, 23.6f, -81.2f, 55.0f,
    };

    DoVertexFormatTest(wgpu::VertexFormat::Float3, vertexData, vertexData);
}

TEST_P(VertexFormatTest, Float4) {
    // TODO(cwallez@chromium.org): Failing because of a SPIRV-Cross issue.
    // See http://crbug.com/dawn/259
    DAWN_SKIP_TEST_IF(IsMetal() && IsIntel());

    std::vector<float> vertexData = {
        19.2f, -19.3f, +0.0f, 1.0f, -0.0f, 1.0f, 1.3f, -1.0f, 13.078f, 21.1965f, -1.1f, -1.2f,
    };

    DoVertexFormatTest(wgpu::VertexFormat::Float4, vertexData, vertexData);
}

TEST_P(VertexFormatTest, UInt) {
    // TODO(cwallez@chromium.org): Failing because of a SPIRV-Cross issue.
    // See http://crbug.com/dawn/259
    DAWN_SKIP_TEST_IF(IsMetal() && IsIntel());

    std::vector<uint32_t> vertexData = {std::numeric_limits<uint32_t>::max(),
                                        std::numeric_limits<uint16_t>::max(),
                                        std::numeric_limits<uint8_t>::max()};

    DoVertexFormatTest(wgpu::VertexFormat::UInt, vertexData, vertexData);
}

TEST_P(VertexFormatTest, UInt2) {
    // TODO(cwallez@chromium.org): Failing because of a SPIRV-Cross issue.
    // See http://crbug.com/dawn/259
    DAWN_SKIP_TEST_IF(IsMetal() && IsIntel());

    std::vector<uint32_t> vertexData = {std::numeric_limits<uint32_t>::max(), 32,
                                        std::numeric_limits<uint16_t>::max(), 64,
                                        std::numeric_limits<uint8_t>::max(),  128};

    DoVertexFormatTest(wgpu::VertexFormat::UInt2, vertexData, vertexData);
}

TEST_P(VertexFormatTest, UInt3) {
    // TODO(cwallez@chromium.org): Failing because of a SPIRV-Cross issue.
    // See http://crbug.com/dawn/259
    DAWN_SKIP_TEST_IF(IsMetal() && IsIntel());

    std::vector<uint32_t> vertexData = {std::numeric_limits<uint32_t>::max(), 32,   64,
                                        std::numeric_limits<uint16_t>::max(), 164,  128,
                                        std::numeric_limits<uint8_t>::max(),  1283, 256};

    DoVertexFormatTest(wgpu::VertexFormat::UInt3, vertexData, vertexData);
}

TEST_P(VertexFormatTest, UInt4) {
    // TODO(cwallez@chromium.org): Failing because of a SPIRV-Cross issue.
    // See http://crbug.com/dawn/259
    DAWN_SKIP_TEST_IF(IsMetal() && IsIntel());

    std::vector<uint32_t> vertexData = {std::numeric_limits<uint32_t>::max(), 32,   64,  5460,
                                        std::numeric_limits<uint16_t>::max(), 164,  128, 0,
                                        std::numeric_limits<uint8_t>::max(),  1283, 256, 4567};

    DoVertexFormatTest(wgpu::VertexFormat::UInt4, vertexData, vertexData);
}

TEST_P(VertexFormatTest, Int) {
    // TODO(cwallez@chromium.org): Failing because of a SPIRV-Cross issue.
    // See http://crbug.com/dawn/259
    DAWN_SKIP_TEST_IF(IsMetal() && IsIntel());

    std::vector<int32_t> vertexData = {std::numeric_limits<int32_t>::max(),
                                       std::numeric_limits<int32_t>::min(),
                                       std::numeric_limits<int8_t>::max()};

    DoVertexFormatTest(wgpu::VertexFormat::Int, vertexData, vertexData);
}

TEST_P(VertexFormatTest, Int2) {
    // TODO(cwallez@chromium.org): Failing because of a SPIRV-Cross issue.
    // See http://crbug.com/dawn/259
    DAWN_SKIP_TEST_IF(IsMetal() && IsIntel());

    std::vector<int32_t> vertexData = {
        std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::min(),
        std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::min(),
        std::numeric_limits<int8_t>::max(),  std::numeric_limits<int8_t>::min()};

    DoVertexFormatTest(wgpu::VertexFormat::Int2, vertexData, vertexData);
}

TEST_P(VertexFormatTest, Int3) {
    // TODO(cwallez@chromium.org): Failing because of a SPIRV-Cross issue.
    // See http://crbug.com/dawn/259
    DAWN_SKIP_TEST_IF(IsMetal() && IsIntel());

    std::vector<int32_t> vertexData = {
        std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::min(), 64,
        std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::min(), 128,
        std::numeric_limits<int8_t>::max(),  std::numeric_limits<int8_t>::min(),  256};

    DoVertexFormatTest(wgpu::VertexFormat::Int3, vertexData, vertexData);
}

TEST_P(VertexFormatTest, Int4) {
    // TODO(cwallez@chromium.org): Failing because of a SPIRV-Cross issue.
    // See http://crbug.com/dawn/259
    DAWN_SKIP_TEST_IF(IsMetal() && IsIntel());

    std::vector<int32_t> vertexData = {
        std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::min(), 64,   -5460,
        std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::min(), -128, 0,
        std::numeric_limits<int8_t>::max(),  std::numeric_limits<int8_t>::min(),  256,  -4567};

    DoVertexFormatTest(wgpu::VertexFormat::Int4, vertexData, vertexData);
}

DAWN_INSTANTIATE_TEST(VertexFormatTest,
                      D3D12Backend(),
                      MetalBackend(),
                      OpenGLBackend(),
                      VulkanBackend());
