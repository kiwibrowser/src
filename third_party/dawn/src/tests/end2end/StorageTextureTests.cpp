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

#include "tests/DawnTest.h"

#include "common/Assert.h"
#include "common/Constants.h"
#include "common/Math.h"
#include "utils/ComboRenderPipelineDescriptor.h"
#include "utils/TextureFormatUtils.h"
#include "utils/WGPUHelpers.h"

class StorageTextureTests : public DawnTest {
  public:
    static void FillExpectedData(void* pixelValuePtr,
                                 wgpu::TextureFormat format,
                                 uint32_t x,
                                 uint32_t y,
                                 uint32_t arrayLayer) {
        const uint32_t pixelValue = 1 + x + kWidth * (y + kHeight * arrayLayer);
        ASSERT(pixelValue <= 255u / 4);

        switch (format) {
            // 32-bit unsigned integer formats
            case wgpu::TextureFormat::R32Uint: {
                uint32_t* valuePtr = static_cast<uint32_t*>(pixelValuePtr);
                *valuePtr = pixelValue;
                break;
            }

            case wgpu::TextureFormat::RG32Uint: {
                uint32_t* valuePtr = static_cast<uint32_t*>(pixelValuePtr);
                valuePtr[0] = pixelValue;
                valuePtr[1] = pixelValue * 2;
                break;
            }

            case wgpu::TextureFormat::RGBA32Uint: {
                uint32_t* valuePtr = static_cast<uint32_t*>(pixelValuePtr);
                valuePtr[0] = pixelValue;
                valuePtr[1] = pixelValue * 2;
                valuePtr[2] = pixelValue * 3;
                valuePtr[3] = pixelValue * 4;
                break;
            }

            // 32-bit signed integer formats
            case wgpu::TextureFormat::R32Sint: {
                int32_t* valuePtr = static_cast<int32_t*>(pixelValuePtr);
                *valuePtr = static_cast<int32_t>(pixelValue);
                break;
            }

            case wgpu::TextureFormat::RG32Sint: {
                int32_t* valuePtr = static_cast<int32_t*>(pixelValuePtr);
                valuePtr[0] = static_cast<int32_t>(pixelValue);
                valuePtr[1] = -static_cast<int32_t>(pixelValue);
                break;
            }

            case wgpu::TextureFormat::RGBA32Sint: {
                int32_t* valuePtr = static_cast<int32_t*>(pixelValuePtr);
                valuePtr[0] = static_cast<int32_t>(pixelValue);
                valuePtr[1] = -static_cast<int32_t>(pixelValue);
                valuePtr[2] = static_cast<int32_t>(pixelValue * 2);
                valuePtr[3] = -static_cast<int32_t>(pixelValue * 2);
                break;
            }

            // 32-bit float formats
            case wgpu::TextureFormat::R32Float: {
                float_t* valuePtr = static_cast<float_t*>(pixelValuePtr);
                *valuePtr = static_cast<float_t>(pixelValue * 1.1f);
                break;
            }

            case wgpu::TextureFormat::RG32Float: {
                float_t* valuePtr = static_cast<float_t*>(pixelValuePtr);
                valuePtr[0] = static_cast<float_t>(pixelValue * 1.1f);
                valuePtr[1] = -static_cast<float_t>(pixelValue * 2.2f);
                break;
            }

            case wgpu::TextureFormat::RGBA32Float: {
                float_t* valuePtr = static_cast<float_t*>(pixelValuePtr);
                valuePtr[0] = static_cast<float_t>(pixelValue * 1.1f);
                valuePtr[1] = -static_cast<float_t>(pixelValue * 1.1f);
                valuePtr[2] = static_cast<float_t>(pixelValue * 2.2f);
                valuePtr[3] = -static_cast<float_t>(pixelValue * 2.2f);
                break;
            }

            // 16-bit (unsigned integer, signed integer and float) 4-component formats
            case wgpu::TextureFormat::RGBA16Uint: {
                uint16_t* valuePtr = static_cast<uint16_t*>(pixelValuePtr);
                valuePtr[0] = static_cast<uint16_t>(pixelValue);
                valuePtr[1] = static_cast<uint16_t>(pixelValue * 2);
                valuePtr[2] = static_cast<uint16_t>(pixelValue * 3);
                valuePtr[3] = static_cast<uint16_t>(pixelValue * 4);
                break;
            }
            case wgpu::TextureFormat::RGBA16Sint: {
                int16_t* valuePtr = static_cast<int16_t*>(pixelValuePtr);
                valuePtr[0] = static_cast<int16_t>(pixelValue);
                valuePtr[1] = -static_cast<int16_t>(pixelValue);
                valuePtr[2] = static_cast<int16_t>(pixelValue * 2);
                valuePtr[3] = -static_cast<int16_t>(pixelValue * 2);
                break;
            }

            case wgpu::TextureFormat::RGBA16Float: {
                uint16_t* valuePtr = static_cast<uint16_t*>(pixelValuePtr);
                valuePtr[0] = Float32ToFloat16(static_cast<float_t>(pixelValue));
                valuePtr[1] = Float32ToFloat16(-static_cast<float_t>(pixelValue));
                valuePtr[2] = Float32ToFloat16(static_cast<float_t>(pixelValue * 2));
                valuePtr[3] = Float32ToFloat16(-static_cast<float_t>(pixelValue * 2));
                break;
            }

            // 8-bit (normalized/non-normalized signed/unsigned integer) 4-component formats
            case wgpu::TextureFormat::RGBA8Unorm:
            case wgpu::TextureFormat::RGBA8Uint: {
                RGBA8* valuePtr = static_cast<RGBA8*>(pixelValuePtr);
                *valuePtr = RGBA8(pixelValue, pixelValue * 2, pixelValue * 3, pixelValue * 4);
                break;
            }

            case wgpu::TextureFormat::RGBA8Snorm:
            case wgpu::TextureFormat::RGBA8Sint: {
                int8_t* valuePtr = static_cast<int8_t*>(pixelValuePtr);
                valuePtr[0] = static_cast<int8_t>(pixelValue);
                valuePtr[1] = -static_cast<int8_t>(pixelValue);
                valuePtr[2] = static_cast<int8_t>(pixelValue) * 2;
                valuePtr[3] = -static_cast<int8_t>(pixelValue) * 2;
                break;
            }

            default:
                UNREACHABLE();
                break;
        }
    }

    std::string GetGLSLImageDeclaration(wgpu::TextureFormat format,
                                        std::string accessQualifier,
                                        bool is2DArray,
                                        uint32_t binding) {
        std::ostringstream ostream;
        ostream << "layout(set = 0, binding = " << binding << ", "
                << utils::GetGLSLImageFormatQualifier(format) << ") uniform " << accessQualifier
                << " " << utils::GetColorTextureComponentTypePrefix(format) << "image2D";
        if (is2DArray) {
            ostream << "Array";
        }
        ostream << " storageImage" << binding << ";";
        return ostream.str();
    }

    const char* GetExpectedPixelValue(wgpu::TextureFormat format) {
        switch (format) {
            // non-normalized unsigned integer formats
            case wgpu::TextureFormat::R32Uint:
                return "uvec4(value, 0, 0, 1u)";

            case wgpu::TextureFormat::RG32Uint:
                return "uvec4(value, value * 2, 0, 1);";

            case wgpu::TextureFormat::RGBA8Uint:
            case wgpu::TextureFormat::RGBA16Uint:
            case wgpu::TextureFormat::RGBA32Uint:
                return "uvec4(value, value * 2, value * 3, value * 4);";

            // non-normalized signed integer formats
            case wgpu::TextureFormat::R32Sint:
                return "ivec4(value, 0, 0, 1)";

            case wgpu::TextureFormat::RG32Sint:
                return "ivec4(value, -value, 0, 1);";

            case wgpu::TextureFormat::RGBA8Sint:
            case wgpu::TextureFormat::RGBA16Sint:
            case wgpu::TextureFormat::RGBA32Sint:
                return "ivec4(value, -value, value * 2, -value * 2);";

            // float formats
            case wgpu::TextureFormat::R32Float:
                return "vec4(value * 1.1f, 0, 0, 1);";

            case wgpu::TextureFormat::RG32Float:
                return "vec4(value * 1.1f, -(value * 2.2f), 0, 1);";

            case wgpu::TextureFormat::RGBA16Float:
                return "vec4(value, -float(value), float(value * 2), -float(value * 2));";

            case wgpu::TextureFormat::RGBA32Float:
                return "vec4(value * 1.1f, -(value * 1.1f), value * 2.2f, -(value * 2.2f));";

            // normalized signed/unsigned integer formats
            case wgpu::TextureFormat::RGBA8Unorm:
                return "vec4(value / 255.0, value / 255.0 * 2, value / 255.0 * 3, value / 255.0 * "
                       "4);";

            case wgpu::TextureFormat::RGBA8Snorm:
                return "vec4(value / 127.0, -(value / 127.0), (value * 2 / 127.0), -(value * 2 / "
                       "127.0));";

            default:
                UNREACHABLE();
                break;
        }
    }

    const char* GetGLSLComparisonFunction(wgpu::TextureFormat format) {
        switch (format) {
            // non-normalized unsigned integer formats
            case wgpu::TextureFormat::R32Uint:
            case wgpu::TextureFormat::RG32Uint:
            case wgpu::TextureFormat::RGBA8Uint:
            case wgpu::TextureFormat::RGBA16Uint:
            case wgpu::TextureFormat::RGBA32Uint:
                return R"(bool IsEqualTo(uvec4 pixel, uvec4 expected) {
                              return pixel == expected;
                       })";

            // non-normalized signed integer formats
            case wgpu::TextureFormat::R32Sint:
            case wgpu::TextureFormat::RG32Sint:
            case wgpu::TextureFormat::RGBA8Sint:
            case wgpu::TextureFormat::RGBA16Sint:
            case wgpu::TextureFormat::RGBA32Sint:
                return R"(bool IsEqualTo(ivec4 pixel, ivec4 expected) {
                              return pixel == expected;
                       })";

            // float formats
            case wgpu::TextureFormat::R32Float:
            case wgpu::TextureFormat::RG32Float:
            case wgpu::TextureFormat::RGBA16Float:
            case wgpu::TextureFormat::RGBA32Float:
                return R"(bool IsEqualTo(vec4 pixel, vec4 expected) {
                              return pixel == expected;
                       })";

            // normalized signed/unsigned integer formats
            case wgpu::TextureFormat::RGBA8Unorm:
            case wgpu::TextureFormat::RGBA8Snorm:
                // On Windows Intel drivers the tests will fail if tolerance <= 0.00000001f.
                return R"(bool IsEqualTo(vec4 pixel, vec4 expected) {
                              const float tolerance = 0.0000001f;
                              return all(lessThan(abs(pixel - expected), vec4(tolerance)));
                       })";

            default:
                UNREACHABLE();
                break;
        }

        return "";
    }

    std::string CommonReadOnlyTestCode(wgpu::TextureFormat format, bool is2DArray = false) {
        std::ostringstream ostream;

        const char* prefix = utils::GetColorTextureComponentTypePrefix(format);

        ostream << GetGLSLImageDeclaration(format, "readonly", is2DArray, 0) << "\n"
                << GetGLSLComparisonFunction(format) << "bool doTest() {\n";
        if (is2DArray) {
            ostream << R"(ivec3 size = imageSize(storageImage0);
                          const uint layerCount = size.z;)";
        } else {
            ostream << R"(ivec2 size = imageSize(storageImage0);
                          const uint layerCount = 1;)";
        }
        ostream << R"(for (uint layer = 0; layer < layerCount; ++layer) {
                          for (uint y = 0; y < size.y; ++y) {
                              for (uint x = 0; x < size.x; ++x) {
                                  uint value = )"
                << kComputeExpectedValueGLSL << ";\n"
                << prefix << "vec4 expected = " << GetExpectedPixelValue(format) << ";\n"
                << prefix << R"(vec4 pixel = imageLoad(storageImage0, )";
        if (is2DArray) {
            ostream << "ivec3(x, y, layer));";
        } else {
            ostream << "ivec2(x, y));";
        }
        ostream << R"(
                                  if (!IsEqualTo(pixel, expected)) {
                                      return false;
                                  }
                              }
                          }
                      }
                      return true;
                   })";

        return ostream.str();
    }

    std::string CommonWriteOnlyTestCode(wgpu::TextureFormat format, bool is2DArray = false) {
        std::ostringstream ostream;

        const char* prefix = utils::GetColorTextureComponentTypePrefix(format);

        ostream << R"(
            #version 450
        )" << GetGLSLImageDeclaration(format, "writeonly", is2DArray, 0)
                << R"(
            void main() {
        )";
        if (is2DArray) {
            ostream << R"(ivec3 size = imageSize(storageImage0);
                          const uint layerCount = size.z;
            )";
        } else {
            ostream << R"(ivec2 size = imageSize(storageImage0);
                          const uint layerCount = 1;
            )";
        }

        ostream << R"(for (uint layer = 0; layer < layerCount; ++layer) {
                          for (uint y = 0; y < size.y; ++y) {
                              for (uint x = 0; x < size.x; ++x) {
                                  uint value = )"
                << kComputeExpectedValueGLSL << ";\n"
                << prefix << "vec4 expected = " << GetExpectedPixelValue(format) << ";\n";
        if (is2DArray) {
            ostream << "ivec3 texcoord = ivec3(x, y, layer);\n";
        } else {
            ostream << "ivec2 texcoord = ivec2(x, y);\n";
        }

        ostream << R"(           imageStore(storageImage0, texcoord, expected);
                             }
                         }
                     }
                 })";

        return ostream.str();
    }

    std::string CommonReadWriteTestCode(wgpu::TextureFormat format, bool is2DArray = false) {
        std::ostringstream ostream;

        ostream << R"(
        #version 450
        )" << GetGLSLImageDeclaration(format, "writeonly", is2DArray, 0)
                << GetGLSLImageDeclaration(format, "readonly", is2DArray, 1) << R"(
            void main() {
        )";
        if (is2DArray) {
            ostream << R"(ivec3 size = imageSize(storageImage0);
                          const uint layerCount = size.z;
            )";
        } else {
            ostream << R"(ivec2 size = imageSize(storageImage0);
                          const uint layerCount = 1;
            )";
        }

        ostream << R"(for (uint layer = 0; layer < layerCount; ++layer) {
                          for (uint y = 0; y < size.y; ++y) {
                              for (uint x = 0; x < size.x; ++x) {)"
                   "\n";
        if (is2DArray) {
            ostream << "ivec3 texcoord = ivec3(x, y, layer);\n";
        } else {
            ostream << "ivec2 texcoord = ivec2(x, y);\n";
        }

        ostream
            << R"(           imageStore(storageImage0, texcoord, imageLoad(storageImage1, texcoord));
                             }
                         }
                     }
                 })";
        return ostream.str();
    }

    static std::vector<uint8_t> GetExpectedData(wgpu::TextureFormat format,
                                                uint32_t arrayLayerCount = 1) {
        const uint32_t texelSizeInBytes = utils::GetTexelBlockSizeInBytes(format);

        std::vector<uint8_t> outputData(texelSizeInBytes * kWidth * kHeight * arrayLayerCount);

        for (uint32_t i = 0; i < outputData.size() / texelSizeInBytes; ++i) {
            uint8_t* pixelValuePtr = &outputData[i * texelSizeInBytes];
            const uint32_t x = i % kWidth;
            const uint32_t y = (i % (kWidth * kHeight)) / kWidth;
            const uint32_t arrayLayer = i / (kWidth * kHeight);
            FillExpectedData(pixelValuePtr, format, x, y, arrayLayer);
        }

        return outputData;
    }

    wgpu::Texture CreateTexture(wgpu::TextureFormat format,
                                wgpu::TextureUsage usage,
                                uint32_t width = kWidth,
                                uint32_t height = kHeight,
                                uint32_t arrayLayerCount = 1) {
        wgpu::TextureDescriptor descriptor;
        descriptor.size = {width, height, arrayLayerCount};
        descriptor.format = format;
        descriptor.usage = usage;
        return device.CreateTexture(&descriptor);
    }

    wgpu::Buffer CreateEmptyBufferForTextureCopy(uint32_t texelSize, uint32_t arrayLayerCount = 1) {
        ASSERT(kWidth * texelSize <= kTextureBytesPerRowAlignment);
        const size_t uploadBufferSize =
            kTextureBytesPerRowAlignment * (kHeight * arrayLayerCount - 1) + kWidth * texelSize;
        wgpu::BufferDescriptor descriptor;
        descriptor.size = uploadBufferSize;
        descriptor.usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst;
        return device.CreateBuffer(&descriptor);
    }

    wgpu::Texture CreateTextureWithTestData(const std::vector<uint8_t>& initialTextureData,
                                            wgpu::TextureFormat format) {
        uint32_t texelSize = utils::GetTexelBlockSizeInBytes(format);
        ASSERT(kWidth * texelSize <= kTextureBytesPerRowAlignment);

        const uint32_t bytesPerTextureRow = texelSize * kWidth;
        const uint32_t arrayLayerCount =
            static_cast<uint32_t>(initialTextureData.size() / texelSize / (kWidth * kHeight));
        const size_t uploadBufferSize =
            kTextureBytesPerRowAlignment * (kHeight * arrayLayerCount - 1) +
            kWidth * bytesPerTextureRow;

        std::vector<uint8_t> uploadBufferData(uploadBufferSize);
        for (uint32_t layer = 0; layer < arrayLayerCount; ++layer) {
            const size_t initialDataOffset = bytesPerTextureRow * kHeight * layer;
            for (size_t y = 0; y < kHeight; ++y) {
                for (size_t x = 0; x < bytesPerTextureRow; ++x) {
                    uint8_t data =
                        initialTextureData[initialDataOffset + bytesPerTextureRow * y + x];
                    size_t indexInUploadBuffer =
                        (kHeight * layer + y) * kTextureBytesPerRowAlignment + x;
                    uploadBufferData[indexInUploadBuffer] = data;
                }
            }
        }
        wgpu::Buffer uploadBuffer =
            utils::CreateBufferFromData(device, uploadBufferData.data(), uploadBufferSize,
                                        wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst);

        wgpu::Texture outputTexture =
            CreateTexture(format, wgpu::TextureUsage::Storage | wgpu::TextureUsage::CopyDst, kWidth,
                          kHeight, arrayLayerCount);

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

        const wgpu::Extent3D copyExtent = {kWidth, kHeight, arrayLayerCount};
        wgpu::BufferCopyView bufferCopyView =
            utils::CreateBufferCopyView(uploadBuffer, 0, kTextureBytesPerRowAlignment, 0);
        wgpu::TextureCopyView textureCopyView;
        textureCopyView.texture = outputTexture;
        encoder.CopyBufferToTexture(&bufferCopyView, &textureCopyView, &copyExtent);

        wgpu::CommandBuffer commandBuffer = encoder.Finish();
        queue.Submit(1, &commandBuffer);

        return outputTexture;
    }

    wgpu::ComputePipeline CreateComputePipeline(const char* computeShader) {
        wgpu::ShaderModule csModule =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Compute, computeShader);
        wgpu::ComputePipelineDescriptor computeDescriptor;
        computeDescriptor.layout = nullptr;
        computeDescriptor.computeStage.module = csModule;
        computeDescriptor.computeStage.entryPoint = "main";
        return device.CreateComputePipeline(&computeDescriptor);
    }

    wgpu::RenderPipeline CreateRenderPipeline(const char* vertexShader,
                                              const char* fragmentShader) {
        wgpu::ShaderModule vsModule =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, vertexShader);
        wgpu::ShaderModule fsModule =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, fragmentShader);

        utils::ComboRenderPipelineDescriptor desc(device);
        desc.vertexStage.module = vsModule;
        desc.cFragmentStage.module = fsModule;
        desc.cColorStates[0].format = kOutputAttachmentFormat;
        desc.primitiveTopology = wgpu::PrimitiveTopology::PointList;
        return device.CreateRenderPipeline(&desc);
    }

    void CheckDrawsGreen(const char* vertexShader,
                         const char* fragmentShader,
                         wgpu::Texture readonlyStorageTexture) {
        wgpu::RenderPipeline pipeline = CreateRenderPipeline(vertexShader, fragmentShader);
        wgpu::BindGroup bindGroup = utils::MakeBindGroup(
            device, pipeline.GetBindGroupLayout(0), {{0, readonlyStorageTexture.CreateView()}});

        // Clear the output attachment to red at the beginning of the render pass.
        wgpu::Texture outputTexture =
            CreateTexture(kOutputAttachmentFormat,
                          wgpu::TextureUsage::OutputAttachment | wgpu::TextureUsage::CopySrc, 1, 1);
        utils::ComboRenderPassDescriptor renderPassDescriptor({outputTexture.CreateView()});
        renderPassDescriptor.cColorAttachments[0].loadOp = wgpu::LoadOp::Clear;
        renderPassDescriptor.cColorAttachments[0].clearColor = {1.f, 0.f, 0.f, 1.f};
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::RenderPassEncoder renderPassEncoder = encoder.BeginRenderPass(&renderPassDescriptor);
        renderPassEncoder.SetBindGroup(0, bindGroup);
        renderPassEncoder.SetPipeline(pipeline);
        renderPassEncoder.Draw(1);
        renderPassEncoder.EndPass();

        wgpu::CommandBuffer commandBuffer = encoder.Finish();
        queue.Submit(1, &commandBuffer);

        // Check if the contents in the output texture are all as expected (green).
        EXPECT_PIXEL_RGBA8_EQ(RGBA8::kGreen, outputTexture, 0, 0);
    }

    void CheckResultInStorageBuffer(wgpu::Texture readonlyStorageTexture,
                                    const std::string& computeShader) {
        wgpu::ComputePipeline pipeline = CreateComputePipeline(computeShader.c_str());

        // Clear the content of the result buffer into 0.
        constexpr uint32_t kInitialValue = 0;
        wgpu::Buffer resultBuffer =
            utils::CreateBufferFromData(device, &kInitialValue, sizeof(kInitialValue),
                                        wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc);
        wgpu::BindGroup bindGroup =
            utils::MakeBindGroup(device, pipeline.GetBindGroupLayout(0),
                                 {{0, readonlyStorageTexture.CreateView()}, {1, resultBuffer}});

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::ComputePassEncoder computeEncoder = encoder.BeginComputePass();
        computeEncoder.SetBindGroup(0, bindGroup);
        computeEncoder.SetPipeline(pipeline);
        computeEncoder.Dispatch(1);
        computeEncoder.EndPass();

        wgpu::CommandBuffer commandBuffer = encoder.Finish();
        queue.Submit(1, &commandBuffer);

        // Check if the contents in the result buffer are what we expect.
        constexpr uint32_t kExpectedValue = 1u;
        EXPECT_BUFFER_U32_RANGE_EQ(&kExpectedValue, resultBuffer, 0, 1u);
    }

    void WriteIntoStorageTextureInRenderPass(wgpu::Texture writeonlyStorageTexture,
                                             const char* kVertexShader,
                                             const char* kFragmentShader) {
        // Create a render pipeline that writes the expected pixel values into the storage texture
        // without fragment shader outputs.
        wgpu::RenderPipeline pipeline = CreateRenderPipeline(kVertexShader, kFragmentShader);
        wgpu::BindGroup bindGroup = utils::MakeBindGroup(
            device, pipeline.GetBindGroupLayout(0), {{0, writeonlyStorageTexture.CreateView()}});

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

        // TODO(jiawei.shao@intel.com): remove the output attachment when Dawn supports beginning a
        // render pass with no attachments.
        wgpu::Texture dummyOutputTexture =
            CreateTexture(kOutputAttachmentFormat,
                          wgpu::TextureUsage::OutputAttachment | wgpu::TextureUsage::CopySrc, 1, 1);
        utils::ComboRenderPassDescriptor renderPassDescriptor({dummyOutputTexture.CreateView()});
        wgpu::RenderPassEncoder renderPassEncoder = encoder.BeginRenderPass(&renderPassDescriptor);
        renderPassEncoder.SetBindGroup(0, bindGroup);
        renderPassEncoder.SetPipeline(pipeline);
        renderPassEncoder.Draw(1);
        renderPassEncoder.EndPass();
        wgpu::CommandBuffer commandBuffer = encoder.Finish();
        queue.Submit(1, &commandBuffer);
    }

    void WriteIntoStorageTextureInComputePass(wgpu::Texture writeonlyStorageTexture,
                                              const char* computeShader) {
        // Create a compute pipeline that writes the expected pixel values into the storage texture.
        wgpu::ComputePipeline pipeline = CreateComputePipeline(computeShader);
        wgpu::BindGroup bindGroup = utils::MakeBindGroup(
            device, pipeline.GetBindGroupLayout(0), {{0, writeonlyStorageTexture.CreateView()}});

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::ComputePassEncoder computePassEncoder = encoder.BeginComputePass();
        computePassEncoder.SetBindGroup(0, bindGroup);
        computePassEncoder.SetPipeline(pipeline);
        computePassEncoder.Dispatch(1);
        computePassEncoder.EndPass();
        wgpu::CommandBuffer commandBuffer = encoder.Finish();
        queue.Submit(1, &commandBuffer);
    }

    void ReadWriteIntoStorageTextureInComputePass(wgpu::Texture readonlyStorageTexture,
                                                  wgpu::Texture writeonlyStorageTexture,
                                                  const char* computeShader) {
        // Create a compute pipeline that writes the expected pixel values into the storage texture.
        wgpu::ComputePipeline pipeline = CreateComputePipeline(computeShader);
        wgpu::BindGroup bindGroup = utils::MakeBindGroup(
            device, pipeline.GetBindGroupLayout(0),
            {{0, writeonlyStorageTexture.CreateView()}, {1, readonlyStorageTexture.CreateView()}});

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::ComputePassEncoder computePassEncoder = encoder.BeginComputePass();
        computePassEncoder.SetBindGroup(0, bindGroup);
        computePassEncoder.SetPipeline(pipeline);
        computePassEncoder.Dispatch(1);
        computePassEncoder.EndPass();
        wgpu::CommandBuffer commandBuffer = encoder.Finish();
        queue.Submit(1, &commandBuffer);
    }

    void CheckOutputStorageTexture(wgpu::Texture writeonlyStorageTexture,
                                   wgpu::TextureFormat format,
                                   uint32_t arrayLayerCount = 1) {
        const uint32_t texelSize = utils::GetTexelBlockSizeInBytes(format);
        const std::vector<uint8_t>& expectedData = GetExpectedData(format, arrayLayerCount);
        CheckOutputStorageTexture(writeonlyStorageTexture, texelSize, expectedData);
    }

    void CheckOutputStorageTexture(wgpu::Texture writeonlyStorageTexture,
                                   uint32_t texelSize,
                                   const std::vector<uint8_t>& expectedData) {
        // Copy the content from the write-only storage texture to the result buffer.
        const uint32_t arrayLayerCount =
            static_cast<uint32_t>(expectedData.size() / texelSize / (kWidth * kHeight));
        wgpu::Buffer resultBuffer = CreateEmptyBufferForTextureCopy(texelSize, arrayLayerCount);

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

        const wgpu::Extent3D copyExtent = {kWidth, kHeight, arrayLayerCount};
        wgpu::TextureCopyView textureCopyView =
            utils::CreateTextureCopyView(writeonlyStorageTexture, 0, {0, 0, 0});
        wgpu::BufferCopyView bufferCopyView =
            utils::CreateBufferCopyView(resultBuffer, 0, kTextureBytesPerRowAlignment, 0);
        encoder.CopyTextureToBuffer(&textureCopyView, &bufferCopyView, &copyExtent);
        wgpu::CommandBuffer commandBuffer = encoder.Finish();
        queue.Submit(1, &commandBuffer);

        // Check if the contents in the result buffer are what we expect.
        for (size_t layer = 0; layer < arrayLayerCount; ++layer) {
            for (size_t y = 0; y < kHeight; ++y) {
                const size_t resultBufferOffset =
                    kTextureBytesPerRowAlignment * (kHeight * layer + y);
                const size_t expectedDataOffset = texelSize * kWidth * (kHeight * layer + y);
                EXPECT_BUFFER_U32_RANGE_EQ(
                    reinterpret_cast<const uint32_t*>(expectedData.data() + expectedDataOffset),
                    resultBuffer, resultBufferOffset, kWidth);
            }
        }
    }

    static constexpr size_t kWidth = 4u;
    static constexpr size_t kHeight = 4u;
    static constexpr wgpu::TextureFormat kOutputAttachmentFormat = wgpu::TextureFormat::RGBA8Unorm;

    const char* kSimpleVertexShader = R"(
        #version 450
        void main() {
            gl_Position = vec4(0.f, 0.f, 0.f, 1.f);
            gl_PointSize = 1.0f;
        })";

    const char* kComputeExpectedValueGLSL = "1 + x + size.x * (y + size.y * layer)";
};

// Test that using read-only storage texture and write-only storage texture in BindGroupLayout is
// valid on all backends. This test is a regression test for chromium:1061156 and passes by not
// asserting or crashing.
TEST_P(StorageTextureTests, BindGroupLayoutWithStorageTextureBindingType) {
    // wgpu::BindingType::ReadonlyStorageTexture is a valid binding type to create a bind group
    // layout.
    {
        wgpu::BindGroupLayoutEntry entry = {0, wgpu::ShaderStage::Compute,
                                            wgpu::BindingType::ReadonlyStorageTexture};
        entry.storageTextureFormat = wgpu::TextureFormat::R32Float;
        wgpu::BindGroupLayoutDescriptor descriptor;
        descriptor.entryCount = 1;
        descriptor.entries = &entry;
        device.CreateBindGroupLayout(&descriptor);
    }

    // wgpu::BindingType::WriteonlyStorageTexture is a valid binding type to create a bind group
    // layout.
    {
        wgpu::BindGroupLayoutEntry entry = {0, wgpu::ShaderStage::Compute,
                                            wgpu::BindingType::WriteonlyStorageTexture};
        entry.storageTextureFormat = wgpu::TextureFormat::R32Float;
        wgpu::BindGroupLayoutDescriptor descriptor;
        descriptor.entryCount = 1;
        descriptor.entries = &entry;
        device.CreateBindGroupLayout(&descriptor);
    }
}

// Test that read-only storage textures are supported in compute shader.
TEST_P(StorageTextureTests, ReadonlyStorageTextureInComputeShader) {
    // When we run dawn_end2end_tests with "--use-spvc-parser", extracting the binding type of a
    // read-only image will always return shaderc_spvc_binding_type_writeonly_storage_texture.
    // TODO(jiawei.shao@intel.com): enable this test when we specify "--use-spvc-parser" after the
    // bug in spvc parser is fixed.
    DAWN_SKIP_TEST_IF(IsD3D12() && IsSpvcParserBeingUsed());

    for (wgpu::TextureFormat format : utils::kAllTextureFormats) {
        if (!utils::TextureFormatSupportsStorageTexture(format)) {
            continue;
        }

        // Prepare the read-only storage texture and fill it with the expected data.
        const std::vector<uint8_t> kInitialTextureData = GetExpectedData(format);
        wgpu::Texture readonlyStorageTexture =
            CreateTextureWithTestData(kInitialTextureData, format);

        // Create a compute shader that reads the pixels from the read-only storage texture and
        // writes 1 to DstBuffer if they all have to expected value.
        std::ostringstream csStream;
        csStream << R"(
            #version 450
            layout(set = 0, binding = 1, std430) buffer DstBuffer {
                uint result;
            } dstBuffer;
         )" << CommonReadOnlyTestCode(format)
                 << R"(
            void main() {
            if (doTest()) {
                dstBuffer.result = 1;
            } else {
                dstBuffer.result = 0;
            }
        })";

        CheckResultInStorageBuffer(readonlyStorageTexture, csStream.str());
    }
}

// Test that read-only storage textures are supported in vertex shader.
TEST_P(StorageTextureTests, ReadonlyStorageTextureInVertexShader) {
    // When we run dawn_end2end_tests with "--use-spvc-parser", extracting the binding type of a
    // read-only image will always return shaderc_spvc_binding_type_writeonly_storage_texture.
    // TODO(jiawei.shao@intel.com): enable this test when we specify "--use-spvc-parser" after the
    // bug in spvc parser is fixed.
    DAWN_SKIP_TEST_IF(IsSpvcParserBeingUsed());

    for (wgpu::TextureFormat format : utils::kAllTextureFormats) {
        if (!utils::TextureFormatSupportsStorageTexture(format)) {
            continue;
        }

        // Prepare the read-only storage texture and fill it with the expected data.
        const std::vector<uint8_t> kInitialTextureData = GetExpectedData(format);
        wgpu::Texture readonlyStorageTexture =
            CreateTextureWithTestData(kInitialTextureData, format);

        // Create a rendering pipeline that reads the pixels from the read-only storage texture and
        // uses green as the output color, otherwise uses red instead.
        std::ostringstream vsStream;
        vsStream << R"(
            #version 450
            layout(location = 0) out vec4 o_color;
        )" << CommonReadOnlyTestCode(format)
                 << R"(
            void main() {
                gl_Position = vec4(0.f, 0.f, 0.f, 1.f);
                if (doTest()) {
                    o_color = vec4(0.f, 1.f, 0.f, 1.f);
                } else {
                    o_color = vec4(1.f, 0.f, 0.f, 1.f);
                }
                gl_PointSize = 1.0f;
            })";
        const char* kFragmentShader = R"(
            #version 450
            layout(location = 0) in vec4 o_color;
            layout(location = 0) out vec4 fragColor;
            void main() {
                fragColor = o_color;
            })";
        CheckDrawsGreen(vsStream.str().c_str(), kFragmentShader, readonlyStorageTexture);
    }
}

// Test that read-only storage textures are supported in fragment shader.
TEST_P(StorageTextureTests, ReadonlyStorageTextureInFragmentShader) {
    // When we run dawn_end2end_tests with "--use-spvc-parser", extracting the binding type of a
    // read-only image will always return shaderc_spvc_binding_type_writeonly_storage_texture.
    // TODO(jiawei.shao@intel.com): enable this test when we specify "--use-spvc-parser" after the
    // bug in spvc parser is fixed.
    DAWN_SKIP_TEST_IF(IsSpvcParserBeingUsed());

    for (wgpu::TextureFormat format : utils::kAllTextureFormats) {
        if (!utils::TextureFormatSupportsStorageTexture(format)) {
            continue;
        }

        // Prepare the read-only storage texture and fill it with the expected data.
        const std::vector<uint8_t> kInitialTextureData = GetExpectedData(format);
        wgpu::Texture readonlyStorageTexture =
            CreateTextureWithTestData(kInitialTextureData, format);

        // Create a rendering pipeline that reads the pixels from the read-only storage texture and
        // uses green as the output color if the pixel value is expected, otherwise uses red
        // instead.
        std::ostringstream fsStream;
        fsStream << R"(
            #version 450
            layout(location = 0) out vec4 o_color;
        )" << CommonReadOnlyTestCode(format)
                 << R"(
            void main() {
                if (doTest()) {
                    o_color = vec4(0.f, 1.f, 0.f, 1.f);
                } else {
                    o_color = vec4(1.f, 0.f, 0.f, 1.f);
                }
            })";
        CheckDrawsGreen(kSimpleVertexShader, fsStream.str().c_str(), readonlyStorageTexture);
    }
}

// Test that write-only storage textures are supported in compute shader.
TEST_P(StorageTextureTests, WriteonlyStorageTextureInComputeShader) {
    // When we run dawn_end2end_tests with "--use-spvc-parser", extracting the binding type of a
    // read-only image will always return shaderc_spvc_binding_type_writeonly_storage_texture.
    // TODO(jiawei.shao@intel.com): enable this test when we specify "--use-spvc-parser" after the
    // bug in spvc parser is fixed.
    DAWN_SKIP_TEST_IF(IsD3D12() && IsSpvcParserBeingUsed());

    for (wgpu::TextureFormat format : utils::kAllTextureFormats) {
        if (!utils::TextureFormatSupportsStorageTexture(format)) {
            continue;
        }

        // TODO(jiawei.shao@intel.com): investigate why this test fails with RGBA8Snorm on Linux
        // Intel OpenGL driver.
        if (format == wgpu::TextureFormat::RGBA8Snorm && IsIntel() && IsOpenGL() && IsLinux()) {
            continue;
        }

        // Prepare the write-only storage texture.
        wgpu::Texture writeonlyStorageTexture =
            CreateTexture(format, wgpu::TextureUsage::Storage | wgpu::TextureUsage::CopySrc);

        // Write the expected pixel values into the write-only storage texture.
        const std::string computeShader = CommonWriteOnlyTestCode(format);
        WriteIntoStorageTextureInComputePass(writeonlyStorageTexture, computeShader.c_str());

        // Verify the pixel data in the write-only storage texture is expected.
        CheckOutputStorageTexture(writeonlyStorageTexture, format);
    }
}

// Test that reading from one read-only storage texture then writing into another write-only storage
// texture in one dispatch are supported in compute shader.
TEST_P(StorageTextureTests, ReadWriteDifferentStorageTextureInOneDispatchInComputeShader) {
    // When we run dawn_end2end_tests with "--use-spvc-parser", extracting the binding type of a
    // read-only image will always return shaderc_spvc_binding_type_writeonly_storage_texture.
    // TODO(jiawei.shao@intel.com): enable this test when we specify "--use-spvc-parser" after the
    // bug in spvc parser is fixed.
    DAWN_SKIP_TEST_IF(IsD3D12() && IsSpvcParserBeingUsed());

    for (wgpu::TextureFormat format : utils::kAllTextureFormats) {
        if (!utils::TextureFormatSupportsStorageTexture(format)) {
            continue;
        }

        // TODO(jiawei.shao@intel.com): investigate why this test fails with RGBA8Snorm on Linux
        // Intel OpenGL driver.
        if (format == wgpu::TextureFormat::RGBA8Snorm && IsIntel() && IsOpenGL() && IsLinux()) {
            continue;
        }

        // Prepare the read-only storage texture.
        const std::vector<uint8_t> kInitialTextureData = GetExpectedData(format);
        wgpu::Texture readonlyStorageTexture =
            CreateTextureWithTestData(kInitialTextureData, format);

        // Prepare the write-only storage texture.
        wgpu::Texture writeonlyStorageTexture =
            CreateTexture(format, wgpu::TextureUsage::Storage | wgpu::TextureUsage::CopySrc);

        // Write the expected pixel values into the write-only storage texture.
        const std::string computeShader = CommonReadWriteTestCode(format);
        ReadWriteIntoStorageTextureInComputePass(readonlyStorageTexture, writeonlyStorageTexture,
                                                 computeShader.c_str());

        // Verify the pixel data in the write-only storage texture is expected.
        CheckOutputStorageTexture(writeonlyStorageTexture, format);
    }
}

// Test that write-only storage textures are supported in fragment shader.
TEST_P(StorageTextureTests, WriteonlyStorageTextureInFragmentShader) {
    // When we run dawn_end2end_tests with "--use-spvc-parser", extracting the binding type of a
    // read-only image will always return shaderc_spvc_binding_type_writeonly_storage_texture.
    // TODO(jiawei.shao@intel.com): enable this test when we specify "--use-spvc-parser" after the
    // bug in spvc parser is fixed.
    DAWN_SKIP_TEST_IF(IsD3D12() && IsSpvcParserBeingUsed());

    for (wgpu::TextureFormat format : utils::kAllTextureFormats) {
        if (!utils::TextureFormatSupportsStorageTexture(format)) {
            continue;
        }

        // TODO(jiawei.shao@intel.com): investigate why this test fails with RGBA8Snorm on Linux
        // Intel OpenGL driver.
        if (format == wgpu::TextureFormat::RGBA8Snorm && IsIntel() && IsOpenGL() && IsLinux()) {
            continue;
        }

        // Prepare the write-only storage texture.
        wgpu::Texture writeonlyStorageTexture =
            CreateTexture(format, wgpu::TextureUsage::Storage | wgpu::TextureUsage::CopySrc);

        // Write the expected pixel values into the write-only storage texture.
        const std::string fragmentShader = CommonWriteOnlyTestCode(format);
        WriteIntoStorageTextureInRenderPass(writeonlyStorageTexture, kSimpleVertexShader,
                                            fragmentShader.c_str());

        // Verify the pixel data in the write-only storage texture is expected.
        CheckOutputStorageTexture(writeonlyStorageTexture, format);
    }
}

// Verify 2D array read-only storage texture works correctly.
TEST_P(StorageTextureTests, Readonly2DArrayStorageTexture) {
    // When we run dawn_end2end_tests with "--use-spvc-parser", extracting the binding type of a
    // read-only image will always return shaderc_spvc_binding_type_writeonly_storage_texture.
    // TODO(jiawei.shao@intel.com): enable this test when we specify "--use-spvc-parser" after the
    // bug in spvc parser is fixed.
    DAWN_SKIP_TEST_IF(IsSpvcParserBeingUsed());

    constexpr uint32_t kArrayLayerCount = 3u;

    constexpr wgpu::TextureFormat kTextureFormat = wgpu::TextureFormat::R32Uint;

    const std::vector<uint8_t> initialTextureData =
        GetExpectedData(kTextureFormat, kArrayLayerCount);
    wgpu::Texture readonlyStorageTexture =
        CreateTextureWithTestData(initialTextureData, kTextureFormat);

    // Create a compute shader that reads the pixels from the read-only storage texture and writes 1
    // to DstBuffer if they all have to expected value.
    std::ostringstream csStream;
    csStream << R"(
        #version 450
        layout (set = 0, binding = 1, std430) buffer DstBuffer {
            uint result;
        } dstBuffer;
    )" << CommonReadOnlyTestCode(kTextureFormat, true)
             << R"(
        void main() {
            if (doTest()) {
                dstBuffer.result = 1;
            } else {
                dstBuffer.result = 0;
            }
        })";

    CheckResultInStorageBuffer(readonlyStorageTexture, csStream.str());
}

// Verify 2D array write-only storage texture works correctly.
TEST_P(StorageTextureTests, Writeonly2DArrayStorageTexture) {
    // When we run dawn_end2end_tests with "--use-spvc-parser", extracting the binding type of a
    // read-only image will always return shaderc_spvc_binding_type_writeonly_storage_texture.
    // TODO(jiawei.shao@intel.com): enable this test when we specify "--use-spvc-parser" after the
    // bug in spvc parser is fixed.
    DAWN_SKIP_TEST_IF(IsD3D12() && IsSpvcParserBeingUsed());

    constexpr uint32_t kArrayLayerCount = 3u;

    constexpr wgpu::TextureFormat kTextureFormat = wgpu::TextureFormat::R32Uint;

    // Prepare the write-only storage texture.
    wgpu::Texture writeonlyStorageTexture =
        CreateTexture(kTextureFormat, wgpu::TextureUsage::Storage | wgpu::TextureUsage::CopySrc,
                      kWidth, kHeight, kArrayLayerCount);

    // Write the expected pixel values into the write-only storage texture.
    const std::string computeShader = CommonWriteOnlyTestCode(kTextureFormat, true);
    WriteIntoStorageTextureInComputePass(writeonlyStorageTexture, computeShader.c_str());

    // Verify the pixel data in the write-only storage texture is expected.
    CheckOutputStorageTexture(writeonlyStorageTexture, kTextureFormat, kArrayLayerCount);
}

DAWN_INSTANTIATE_TEST(StorageTextureTests,
                      D3D12Backend(),
                      MetalBackend(),
                      OpenGLBackend(),
                      VulkanBackend());

class StorageTextureZeroInitTests : public StorageTextureTests {
  public:
    static std::vector<uint8_t> GetExpectedData() {
        constexpr wgpu::TextureFormat kTextureFormat = wgpu::TextureFormat::R32Uint;

        const uint32_t texelSizeInBytes = utils::GetTexelBlockSizeInBytes(kTextureFormat);
        const size_t kDataCount = texelSizeInBytes * kWidth * kHeight;
        std::vector<uint8_t> outputData(kDataCount, 0);

        uint32_t* outputDataPtr = reinterpret_cast<uint32_t*>(&outputData[0]);
        *outputDataPtr = 1u;

        return outputData;
    }

    const char* kCommonReadOnlyZeroInitTestCode = R"(
        bool doTest() {
            for (uint y = 0; y < 4; ++y) {
                for (uint x = 0; x < 4; ++x) {
                    uvec4 pixel = imageLoad(srcImage, ivec2(x, y));
                    if (pixel != uvec4(0, 0, 0, 1u)) {
                        return false;
                    }
                }
            }
            return true;
        })";

    const char* kCommonWriteOnlyZeroInitTestCode = R"(
        #version 450
        layout(set = 0, binding = 0, r32ui) uniform writeonly uimage2D dstImage;
        void main() {
            imageStore(dstImage, ivec2(0, 0), uvec4(1u, 0, 0, 1u));
        })";
};

// Verify that the texture is correctly cleared to 0 before its first usage as a read-only storage
// texture in a render pass.
TEST_P(StorageTextureZeroInitTests, ReadonlyStorageTextureClearsToZeroInRenderPass) {
    // When we run dawn_end2end_tests with "--use-spvc-parser", extracting the binding type of a
    // read-only image will always return shaderc_spvc_binding_type_writeonly_storage_texture.
    // TODO(jiawei.shao@intel.com): enable this test when we specify "--use-spvc-parser" after the
    // bug in spvc parser is fixed.
    DAWN_SKIP_TEST_IF(IsSpvcParserBeingUsed());

    wgpu::Texture readonlyStorageTexture =
        CreateTexture(wgpu::TextureFormat::R32Uint, wgpu::TextureUsage::Storage);

    // Create a rendering pipeline that reads the pixels from the read-only storage texture and uses
    // green as the output color, otherwise uses red instead.
    const char* kVertexShader = kSimpleVertexShader;
    const std::string kFragmentShader = std::string(R"(
            #version 450
            layout(set = 0, binding = 0, r32ui) uniform readonly uimage2D srcImage;
            layout(location = 0) out vec4 o_color;)") +
                                        kCommonReadOnlyZeroInitTestCode +
                                        R"(

            void main() {
                if (doTest()) {
                    o_color = vec4(0.f, 1.f, 0.f, 1.f);
                } else {
                    o_color = vec4(1.f, 0.f, 0.f, 1.f);
                }
            })";
    CheckDrawsGreen(kVertexShader, kFragmentShader.c_str(), readonlyStorageTexture);
}

// Verify that the texture is correctly cleared to 0 before its first usage as a read-only storage
// texture in a compute pass.
TEST_P(StorageTextureZeroInitTests, ReadonlyStorageTextureClearsToZeroInComputePass) {
    // When we run dawn_end2end_tests with "--use-spvc-parser", extracting the binding type of a
    // read-only image will always return shaderc_spvc_binding_type_writeonly_storage_texture.
    // TODO(jiawei.shao@intel.com): enable this test when we specify "--use-spvc-parser" after the
    // bug in spvc parser is fixed.
    DAWN_SKIP_TEST_IF(IsSpvcParserBeingUsed());

    wgpu::Texture readonlyStorageTexture =
        CreateTexture(wgpu::TextureFormat::R32Uint, wgpu::TextureUsage::Storage);

    // Create a compute shader that reads the pixels from the read-only storage texture and writes 1
    // to DstBuffer if they all have to expected value.
    const std::string kComputeShader = std::string(R"(
        #version 450
        layout (set = 0, binding = 0, r32ui) uniform readonly uimage2D srcImage;
        layout (set = 0, binding = 1, std430) buffer DstBuffer {
            uint result;
        } dstBuffer;)") + kCommonReadOnlyZeroInitTestCode +
                                       R"(

        void main() {
            if (doTest()) {
                dstBuffer.result = 1;
            } else {
                dstBuffer.result = 0;
            }
        })";

    CheckResultInStorageBuffer(readonlyStorageTexture, kComputeShader);
}

// Verify that the texture is correctly cleared to 0 before its first usage as a write-only storage
// storage texture in a render pass.
TEST_P(StorageTextureZeroInitTests, WriteonlyStorageTextureClearsToZeroInRenderPass) {
    // When we run dawn_end2end_tests with "--use-spvc-parser", extracting the binding type of a
    // read-only image will always return shaderc_spvc_binding_type_writeonly_storage_texture.
    // TODO(jiawei.shao@intel.com): enable this test when we specify "--use-spvc-parser" after the
    // bug in spvc parser is fixed.
    DAWN_SKIP_TEST_IF(IsD3D12() && IsSpvcParserBeingUsed());

    // Prepare the write-only storage texture.
    constexpr uint32_t kTexelSizeR32Uint = 4u;
    wgpu::Texture writeonlyStorageTexture = CreateTexture(
        wgpu::TextureFormat::R32Uint, wgpu::TextureUsage::Storage | wgpu::TextureUsage::CopySrc);

    WriteIntoStorageTextureInRenderPass(writeonlyStorageTexture, kSimpleVertexShader,
                                        kCommonWriteOnlyZeroInitTestCode);
    CheckOutputStorageTexture(writeonlyStorageTexture, kTexelSizeR32Uint, GetExpectedData());
}

// Verify that the texture is correctly cleared to 0 before its first usage as a write-only storage
// texture in a compute pass.
TEST_P(StorageTextureZeroInitTests, WriteonlyStorageTextureClearsToZeroInComputePass) {
    // When we run dawn_end2end_tests with "--use-spvc-parser", extracting the binding type of a
    // read-only image will always return shaderc_spvc_binding_type_writeonly_storage_texture.
    // TODO(jiawei.shao@intel.com): enable this test when we specify "--use-spvc-parser" after the
    // bug in spvc parser is fixed.
    DAWN_SKIP_TEST_IF(IsD3D12() && IsSpvcParserBeingUsed());

    // Prepare the write-only storage texture.
    constexpr uint32_t kTexelSizeR32Uint = 4u;
    wgpu::Texture writeonlyStorageTexture = CreateTexture(
        wgpu::TextureFormat::R32Uint, wgpu::TextureUsage::Storage | wgpu::TextureUsage::CopySrc);

    WriteIntoStorageTextureInComputePass(writeonlyStorageTexture, kCommonWriteOnlyZeroInitTestCode);
    CheckOutputStorageTexture(writeonlyStorageTexture, kTexelSizeR32Uint, GetExpectedData());
}

DAWN_INSTANTIATE_TEST(StorageTextureZeroInitTests,
                      D3D12Backend({"nonzero_clear_resources_on_creation_for_testing"}),
                      OpenGLBackend({"nonzero_clear_resources_on_creation_for_testing"}),
                      MetalBackend({"nonzero_clear_resources_on_creation_for_testing"}),
                      VulkanBackend({"nonzero_clear_resources_on_creation_for_testing"}));
