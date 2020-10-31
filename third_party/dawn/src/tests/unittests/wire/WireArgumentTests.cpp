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

#include "tests/unittests/wire/WireTest.h"

#include "common/Constants.h"

#include <array>

using namespace testing;
using namespace dawn_wire;

class WireArgumentTests : public WireTest {
  public:
    WireArgumentTests() {
    }
    ~WireArgumentTests() override = default;
};

// Test that the wire is able to send numerical values
TEST_F(WireArgumentTests, ValueArgument) {
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, nullptr);
    WGPUComputePassEncoder pass = wgpuCommandEncoderBeginComputePass(encoder, nullptr);
    wgpuComputePassEncoderDispatch(pass, 1, 2, 3);

    WGPUCommandEncoder apiEncoder = api.GetNewCommandEncoder();
    EXPECT_CALL(api, DeviceCreateCommandEncoder(apiDevice, nullptr)).WillOnce(Return(apiEncoder));

    WGPUComputePassEncoder apiPass = api.GetNewComputePassEncoder();
    EXPECT_CALL(api, CommandEncoderBeginComputePass(apiEncoder, nullptr)).WillOnce(Return(apiPass));

    EXPECT_CALL(api, ComputePassEncoderDispatch(apiPass, 1, 2, 3)).Times(1);

    FlushClient();
}

// Test that the wire is able to send arrays of numerical values
TEST_F(WireArgumentTests, ValueArrayArgument) {
    // Create a bindgroup.
    WGPUBindGroupLayoutDescriptor bglDescriptor = {};
    bglDescriptor.entryCount = 0;
    bglDescriptor.entries = nullptr;

    WGPUBindGroupLayout bgl = wgpuDeviceCreateBindGroupLayout(device, &bglDescriptor);
    WGPUBindGroupLayout apiBgl = api.GetNewBindGroupLayout();
    EXPECT_CALL(api, DeviceCreateBindGroupLayout(apiDevice, _)).WillOnce(Return(apiBgl));

    WGPUBindGroupDescriptor bindGroupDescriptor = {};
    bindGroupDescriptor.layout = bgl;
    bindGroupDescriptor.entryCount = 0;
    bindGroupDescriptor.entries = nullptr;

    WGPUBindGroup bindGroup = wgpuDeviceCreateBindGroup(device, &bindGroupDescriptor);
    WGPUBindGroup apiBindGroup = api.GetNewBindGroup();
    EXPECT_CALL(api, DeviceCreateBindGroup(apiDevice, _)).WillOnce(Return(apiBindGroup));

    // Use the bindgroup in SetBindGroup that takes an array of value offsets.
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, nullptr);
    WGPUComputePassEncoder pass = wgpuCommandEncoderBeginComputePass(encoder, nullptr);

    std::array<uint32_t, 4> testOffsets = {0, 42, 0xDEAD'BEEFu, 0xFFFF'FFFFu};
    wgpuComputePassEncoderSetBindGroup(pass, 0, bindGroup, testOffsets.size(), testOffsets.data());

    WGPUCommandEncoder apiEncoder = api.GetNewCommandEncoder();
    EXPECT_CALL(api, DeviceCreateCommandEncoder(apiDevice, nullptr)).WillOnce(Return(apiEncoder));

    WGPUComputePassEncoder apiPass = api.GetNewComputePassEncoder();
    EXPECT_CALL(api, CommandEncoderBeginComputePass(apiEncoder, nullptr)).WillOnce(Return(apiPass));

    EXPECT_CALL(api, ComputePassEncoderSetBindGroup(
                         apiPass, 0, apiBindGroup, testOffsets.size(),
                         MatchesLambda([testOffsets](const uint32_t* offsets) -> bool {
                             for (size_t i = 0; i < testOffsets.size(); i++) {
                                 if (offsets[i] != testOffsets[i]) {
                                     return false;
                                 }
                             }
                             return true;
                         })));

    FlushClient();
}

// Test that the wire is able to send C strings
TEST_F(WireArgumentTests, CStringArgument) {
    // Create shader module
    WGPUShaderModuleDescriptor vertexDescriptor = {};
    WGPUShaderModule vsModule = wgpuDeviceCreateShaderModule(device, &vertexDescriptor);
    WGPUShaderModule apiVsModule = api.GetNewShaderModule();
    EXPECT_CALL(api, DeviceCreateShaderModule(apiDevice, _)).WillOnce(Return(apiVsModule));

    // Create the color state descriptor
    WGPUBlendDescriptor blendDescriptor = {};
    blendDescriptor.operation = WGPUBlendOperation_Add;
    blendDescriptor.srcFactor = WGPUBlendFactor_One;
    blendDescriptor.dstFactor = WGPUBlendFactor_One;
    WGPUColorStateDescriptor colorStateDescriptor = {};
    colorStateDescriptor.format = WGPUTextureFormat_RGBA8Unorm;
    colorStateDescriptor.alphaBlend = blendDescriptor;
    colorStateDescriptor.colorBlend = blendDescriptor;
    colorStateDescriptor.writeMask = WGPUColorWriteMask_All;

    // Create the input state
    WGPUVertexStateDescriptor vertexState = {};
    vertexState.indexFormat = WGPUIndexFormat_Uint32;
    vertexState.vertexBufferCount = 0;
    vertexState.vertexBuffers = nullptr;

    // Create the rasterization state
    WGPURasterizationStateDescriptor rasterizationState = {};
    rasterizationState.frontFace = WGPUFrontFace_CCW;
    rasterizationState.cullMode = WGPUCullMode_None;
    rasterizationState.depthBias = 0;
    rasterizationState.depthBiasSlopeScale = 0.0;
    rasterizationState.depthBiasClamp = 0.0;

    // Create the depth-stencil state
    WGPUStencilStateFaceDescriptor stencilFace = {};
    stencilFace.compare = WGPUCompareFunction_Always;
    stencilFace.failOp = WGPUStencilOperation_Keep;
    stencilFace.depthFailOp = WGPUStencilOperation_Keep;
    stencilFace.passOp = WGPUStencilOperation_Keep;

    WGPUDepthStencilStateDescriptor depthStencilState = {};
    depthStencilState.format = WGPUTextureFormat_Depth24PlusStencil8;
    depthStencilState.depthWriteEnabled = false;
    depthStencilState.depthCompare = WGPUCompareFunction_Always;
    depthStencilState.stencilBack = stencilFace;
    depthStencilState.stencilFront = stencilFace;
    depthStencilState.stencilReadMask = 0xff;
    depthStencilState.stencilWriteMask = 0xff;

    // Create the pipeline layout
    WGPUPipelineLayoutDescriptor layoutDescriptor = {};
    layoutDescriptor.bindGroupLayoutCount = 0;
    layoutDescriptor.bindGroupLayouts = nullptr;
    WGPUPipelineLayout layout = wgpuDeviceCreatePipelineLayout(device, &layoutDescriptor);
    WGPUPipelineLayout apiLayout = api.GetNewPipelineLayout();
    EXPECT_CALL(api, DeviceCreatePipelineLayout(apiDevice, _)).WillOnce(Return(apiLayout));

    // Create pipeline
    WGPURenderPipelineDescriptor pipelineDescriptor = {};

    pipelineDescriptor.vertexStage.module = vsModule;
    pipelineDescriptor.vertexStage.entryPoint = "main";

    WGPUProgrammableStageDescriptor fragmentStage = {};
    fragmentStage.module = vsModule;
    fragmentStage.entryPoint = "main";
    pipelineDescriptor.fragmentStage = &fragmentStage;

    pipelineDescriptor.colorStateCount = 1;
    pipelineDescriptor.colorStates = &colorStateDescriptor;

    pipelineDescriptor.sampleCount = 1;
    pipelineDescriptor.sampleMask = 0xFFFFFFFF;
    pipelineDescriptor.alphaToCoverageEnabled = false;
    pipelineDescriptor.layout = layout;
    pipelineDescriptor.vertexState = &vertexState;
    pipelineDescriptor.primitiveTopology = WGPUPrimitiveTopology_TriangleList;
    pipelineDescriptor.rasterizationState = &rasterizationState;
    pipelineDescriptor.depthStencilState = &depthStencilState;

    wgpuDeviceCreateRenderPipeline(device, &pipelineDescriptor);

    WGPURenderPipeline apiDummyPipeline = api.GetNewRenderPipeline();
    EXPECT_CALL(api,
                DeviceCreateRenderPipeline(
                    apiDevice, MatchesLambda([](const WGPURenderPipelineDescriptor* desc) -> bool {
                        return desc->vertexStage.entryPoint == std::string("main");
                    })))
        .WillOnce(Return(apiDummyPipeline));

    FlushClient();
}

// Test that the wire is able to send objects as value arguments
TEST_F(WireArgumentTests, ObjectAsValueArgument) {
    WGPUCommandEncoder cmdBufEncoder = wgpuDeviceCreateCommandEncoder(device, nullptr);
    WGPUCommandEncoder apiEncoder = api.GetNewCommandEncoder();
    EXPECT_CALL(api, DeviceCreateCommandEncoder(apiDevice, nullptr)).WillOnce(Return(apiEncoder));

    WGPUBufferDescriptor descriptor = {};
    descriptor.size = 8;
    descriptor.usage =
        static_cast<WGPUBufferUsage>(WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst);

    WGPUBuffer buffer = wgpuDeviceCreateBuffer(device, &descriptor);
    WGPUBuffer apiBuffer = api.GetNewBuffer();
    EXPECT_CALL(api, DeviceCreateBuffer(apiDevice, _))
        .WillOnce(Return(apiBuffer))
        .RetiresOnSaturation();

    wgpuCommandEncoderCopyBufferToBuffer(cmdBufEncoder, buffer, 0, buffer, 4, 4);
    EXPECT_CALL(api, CommandEncoderCopyBufferToBuffer(apiEncoder, apiBuffer, 0, apiBuffer, 4, 4));

    FlushClient();
}

// Test that the wire is able to send array of objects
TEST_F(WireArgumentTests, ObjectsAsPointerArgument) {
    WGPUCommandBuffer cmdBufs[2];
    WGPUCommandBuffer apiCmdBufs[2];

    // Create two command buffers we need to use a GMock sequence otherwise the order of the
    // CreateCommandEncoder might be swapped since they are equivalent in term of matchers
    Sequence s;
    for (int i = 0; i < 2; ++i) {
        WGPUCommandEncoder cmdBufEncoder = wgpuDeviceCreateCommandEncoder(device, nullptr);
        cmdBufs[i] = wgpuCommandEncoderFinish(cmdBufEncoder, nullptr);

        WGPUCommandEncoder apiCmdBufEncoder = api.GetNewCommandEncoder();
        EXPECT_CALL(api, DeviceCreateCommandEncoder(apiDevice, nullptr))
            .InSequence(s)
            .WillOnce(Return(apiCmdBufEncoder));

        apiCmdBufs[i] = api.GetNewCommandBuffer();
        EXPECT_CALL(api, CommandEncoderFinish(apiCmdBufEncoder, nullptr))
            .WillOnce(Return(apiCmdBufs[i]));
    }

    // Submit command buffer and check we got a call with both API-side command buffers
    wgpuQueueSubmit(queue, 2, cmdBufs);

    EXPECT_CALL(
        api, QueueSubmit(apiQueue, 2, MatchesLambda([=](const WGPUCommandBuffer* cmdBufs) -> bool {
                             return cmdBufs[0] == apiCmdBufs[0] && cmdBufs[1] == apiCmdBufs[1];
                         })));

    FlushClient();
}

// Test that the wire is able to send structures that contain pure values (non-objects)
TEST_F(WireArgumentTests, StructureOfValuesArgument) {
    WGPUSamplerDescriptor descriptor = {};
    descriptor.magFilter = WGPUFilterMode_Linear;
    descriptor.minFilter = WGPUFilterMode_Nearest;
    descriptor.mipmapFilter = WGPUFilterMode_Linear;
    descriptor.addressModeU = WGPUAddressMode_ClampToEdge;
    descriptor.addressModeV = WGPUAddressMode_Repeat;
    descriptor.addressModeW = WGPUAddressMode_MirrorRepeat;
    descriptor.lodMinClamp = kLodMin;
    descriptor.lodMaxClamp = kLodMax;
    descriptor.compare = WGPUCompareFunction_Never;

    wgpuDeviceCreateSampler(device, &descriptor);

    WGPUSampler apiDummySampler = api.GetNewSampler();
    EXPECT_CALL(api, DeviceCreateSampler(
                         apiDevice, MatchesLambda([](const WGPUSamplerDescriptor* desc) -> bool {
                             return desc->nextInChain == nullptr &&
                                    desc->magFilter == WGPUFilterMode_Linear &&
                                    desc->minFilter == WGPUFilterMode_Nearest &&
                                    desc->mipmapFilter == WGPUFilterMode_Linear &&
                                    desc->addressModeU == WGPUAddressMode_ClampToEdge &&
                                    desc->addressModeV == WGPUAddressMode_Repeat &&
                                    desc->addressModeW == WGPUAddressMode_MirrorRepeat &&
                                    desc->compare == WGPUCompareFunction_Never &&
                                    desc->lodMinClamp == kLodMin && desc->lodMaxClamp == kLodMax;
                         })))
        .WillOnce(Return(apiDummySampler));

    FlushClient();
}

// Test that the wire is able to send structures that contain objects
TEST_F(WireArgumentTests, StructureOfObjectArrayArgument) {
    WGPUBindGroupLayoutDescriptor bglDescriptor = {};
    bglDescriptor.entryCount = 0;
    bglDescriptor.entries = nullptr;

    WGPUBindGroupLayout bgl = wgpuDeviceCreateBindGroupLayout(device, &bglDescriptor);
    WGPUBindGroupLayout apiBgl = api.GetNewBindGroupLayout();
    EXPECT_CALL(api, DeviceCreateBindGroupLayout(apiDevice, _)).WillOnce(Return(apiBgl));

    WGPUPipelineLayoutDescriptor descriptor = {};
    descriptor.bindGroupLayoutCount = 1;
    descriptor.bindGroupLayouts = &bgl;

    wgpuDeviceCreatePipelineLayout(device, &descriptor);

    WGPUPipelineLayout apiDummyLayout = api.GetNewPipelineLayout();
    EXPECT_CALL(api, DeviceCreatePipelineLayout(
                         apiDevice,
                         MatchesLambda([apiBgl](const WGPUPipelineLayoutDescriptor* desc) -> bool {
                             return desc->nextInChain == nullptr &&
                                    desc->bindGroupLayoutCount == 1 &&
                                    desc->bindGroupLayouts[0] == apiBgl;
                         })))
        .WillOnce(Return(apiDummyLayout));

    FlushClient();
}

// Test that the wire is able to send structures that contain objects
TEST_F(WireArgumentTests, StructureOfStructureArrayArgument) {
    static constexpr int NUM_BINDINGS = 3;
    WGPUBindGroupLayoutEntry entries[NUM_BINDINGS]{
        {0, WGPUShaderStage_Vertex, WGPUBindingType_Sampler, false, 0, false,
         WGPUTextureViewDimension_2D, WGPUTextureComponentType_Float, WGPUTextureFormat_RGBA8Unorm},
        {1, WGPUShaderStage_Vertex, WGPUBindingType_SampledTexture, false, 0, false,
         WGPUTextureViewDimension_2D, WGPUTextureComponentType_Float, WGPUTextureFormat_RGBA8Unorm},
        {2, static_cast<WGPUShaderStage>(WGPUShaderStage_Vertex | WGPUShaderStage_Fragment),
         WGPUBindingType_UniformBuffer, false, 0, false, WGPUTextureViewDimension_2D,
         WGPUTextureComponentType_Float, WGPUTextureFormat_RGBA8Unorm},
    };
    WGPUBindGroupLayoutDescriptor bglDescriptor = {};
    bglDescriptor.entryCount = NUM_BINDINGS;
    bglDescriptor.entries = entries;

    wgpuDeviceCreateBindGroupLayout(device, &bglDescriptor);
    WGPUBindGroupLayout apiBgl = api.GetNewBindGroupLayout();
    EXPECT_CALL(
        api,
        DeviceCreateBindGroupLayout(
            apiDevice, MatchesLambda([entries](const WGPUBindGroupLayoutDescriptor* desc) -> bool {
                for (int i = 0; i < NUM_BINDINGS; ++i) {
                    const auto& a = desc->entries[i];
                    const auto& b = entries[i];
                    if (a.binding != b.binding || a.visibility != b.visibility ||
                        a.type != b.type) {
                        return false;
                    }
                }
                return desc->nextInChain == nullptr && desc->entryCount == 3;
            })))
        .WillOnce(Return(apiBgl));

    FlushClient();
}

// Test passing nullptr instead of objects - array of objects version
TEST_F(WireArgumentTests, DISABLED_NullptrInArray) {
    WGPUBindGroupLayout nullBGL = nullptr;

    WGPUPipelineLayoutDescriptor descriptor = {};
    descriptor.bindGroupLayoutCount = 1;
    descriptor.bindGroupLayouts = &nullBGL;

    wgpuDeviceCreatePipelineLayout(device, &descriptor);
    EXPECT_CALL(api,
                DeviceCreatePipelineLayout(
                    apiDevice, MatchesLambda([](const WGPUPipelineLayoutDescriptor* desc) -> bool {
                        return desc->nextInChain == nullptr && desc->bindGroupLayoutCount == 1 &&
                               desc->bindGroupLayouts[0] == nullptr;
                    })))
        .WillOnce(Return(nullptr));

    FlushClient();
}
