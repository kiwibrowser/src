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

using namespace testing;
using namespace dawn_wire;

class WireOptionalTests : public WireTest {
  public:
    WireOptionalTests() {
    }
    ~WireOptionalTests() override = default;
};

// Test passing nullptr instead of objects - object as value version
TEST_F(WireOptionalTests, OptionalObjectValue) {
    WGPUBindGroupLayoutDescriptor bglDesc = {};
    bglDesc.entryCount = 0;
    WGPUBindGroupLayout bgl = wgpuDeviceCreateBindGroupLayout(device, &bglDesc);

    WGPUBindGroupLayout apiBindGroupLayout = api.GetNewBindGroupLayout();
    EXPECT_CALL(api, DeviceCreateBindGroupLayout(apiDevice, _))
        .WillOnce(Return(apiBindGroupLayout));

    // The `sampler`, `textureView` and `buffer` members of a binding are optional.
    WGPUBindGroupEntry entry;
    entry.binding = 0;
    entry.sampler = nullptr;
    entry.textureView = nullptr;
    entry.buffer = nullptr;

    WGPUBindGroupDescriptor bgDesc = {};
    bgDesc.layout = bgl;
    bgDesc.entryCount = 1;
    bgDesc.entries = &entry;

    wgpuDeviceCreateBindGroup(device, &bgDesc);

    WGPUBindGroup apiDummyBindGroup = api.GetNewBindGroup();
    EXPECT_CALL(api, DeviceCreateBindGroup(
                         apiDevice, MatchesLambda([](const WGPUBindGroupDescriptor* desc) -> bool {
                             return desc->nextInChain == nullptr && desc->entryCount == 1 &&
                                    desc->entries[0].binding == 0 &&
                                    desc->entries[0].sampler == nullptr &&
                                    desc->entries[0].buffer == nullptr &&
                                    desc->entries[0].textureView == nullptr;
                         })))
        .WillOnce(Return(apiDummyBindGroup));

    FlushClient();
}

// Test that the wire is able to send optional pointers to structures
TEST_F(WireOptionalTests, OptionalStructPointer) {
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

    // First case: depthStencilState is not null.
    pipelineDescriptor.depthStencilState = &depthStencilState;
    wgpuDeviceCreateRenderPipeline(device, &pipelineDescriptor);

    WGPURenderPipeline apiDummyPipeline = api.GetNewRenderPipeline();
    EXPECT_CALL(
        api,
        DeviceCreateRenderPipeline(
            apiDevice, MatchesLambda([](const WGPURenderPipelineDescriptor* desc) -> bool {
                return desc->depthStencilState != nullptr &&
                       desc->depthStencilState->nextInChain == nullptr &&
                       desc->depthStencilState->depthWriteEnabled == false &&
                       desc->depthStencilState->depthCompare == WGPUCompareFunction_Always &&
                       desc->depthStencilState->stencilBack.compare == WGPUCompareFunction_Always &&
                       desc->depthStencilState->stencilBack.failOp == WGPUStencilOperation_Keep &&
                       desc->depthStencilState->stencilBack.depthFailOp ==
                           WGPUStencilOperation_Keep &&
                       desc->depthStencilState->stencilBack.passOp == WGPUStencilOperation_Keep &&
                       desc->depthStencilState->stencilFront.compare ==
                           WGPUCompareFunction_Always &&
                       desc->depthStencilState->stencilFront.failOp == WGPUStencilOperation_Keep &&
                       desc->depthStencilState->stencilFront.depthFailOp ==
                           WGPUStencilOperation_Keep &&
                       desc->depthStencilState->stencilFront.passOp == WGPUStencilOperation_Keep &&
                       desc->depthStencilState->stencilReadMask == 0xff &&
                       desc->depthStencilState->stencilWriteMask == 0xff;
            })))
        .WillOnce(Return(apiDummyPipeline));

    FlushClient();

    // Second case: depthStencilState is null.
    pipelineDescriptor.depthStencilState = nullptr;
    wgpuDeviceCreateRenderPipeline(device, &pipelineDescriptor);
    EXPECT_CALL(api,
                DeviceCreateRenderPipeline(
                    apiDevice, MatchesLambda([](const WGPURenderPipelineDescriptor* desc) -> bool {
                        return desc->depthStencilState == nullptr;
                    })))
        .WillOnce(Return(apiDummyPipeline));

    FlushClient();
}
