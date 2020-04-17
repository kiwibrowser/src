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
    DawnBindGroupLayoutDescriptor bglDesc;
    bglDesc.nextInChain = nullptr;
    bglDesc.bindingCount = 0;
    DawnBindGroupLayout bgl = dawnDeviceCreateBindGroupLayout(device, &bglDesc);

    DawnBindGroupLayout apiBindGroupLayout = api.GetNewBindGroupLayout();
    EXPECT_CALL(api, DeviceCreateBindGroupLayout(apiDevice, _))
        .WillOnce(Return(apiBindGroupLayout));

    // The `sampler`, `textureView` and `buffer` members of a binding are optional.
    DawnBindGroupBinding binding;
    binding.binding = 0;
    binding.sampler = nullptr;
    binding.textureView = nullptr;
    binding.buffer = nullptr;

    DawnBindGroupDescriptor bgDesc;
    bgDesc.nextInChain = nullptr;
    bgDesc.layout = bgl;
    bgDesc.bindingCount = 1;
    bgDesc.bindings = &binding;

    dawnDeviceCreateBindGroup(device, &bgDesc);

    DawnBindGroup apiDummyBindGroup = api.GetNewBindGroup();
    EXPECT_CALL(api, DeviceCreateBindGroup(
                         apiDevice, MatchesLambda([](const DawnBindGroupDescriptor* desc) -> bool {
                             return desc->nextInChain == nullptr && desc->bindingCount == 1 &&
                                    desc->bindings[0].binding == 0 &&
                                    desc->bindings[0].sampler == nullptr &&
                                    desc->bindings[0].buffer == nullptr &&
                                    desc->bindings[0].textureView == nullptr;
                         })))
        .WillOnce(Return(apiDummyBindGroup));

    FlushClient();
}

// Test that the wire is able to send optional pointers to structures
TEST_F(WireOptionalTests, OptionalStructPointer) {
    // Create shader module
    DawnShaderModuleDescriptor vertexDescriptor;
    vertexDescriptor.nextInChain = nullptr;
    vertexDescriptor.codeSize = 0;
    DawnShaderModule vsModule = dawnDeviceCreateShaderModule(device, &vertexDescriptor);
    DawnShaderModule apiVsModule = api.GetNewShaderModule();
    EXPECT_CALL(api, DeviceCreateShaderModule(apiDevice, _)).WillOnce(Return(apiVsModule));

    // Create the color state descriptor
    DawnBlendDescriptor blendDescriptor;
    blendDescriptor.operation = DAWN_BLEND_OPERATION_ADD;
    blendDescriptor.srcFactor = DAWN_BLEND_FACTOR_ONE;
    blendDescriptor.dstFactor = DAWN_BLEND_FACTOR_ONE;
    DawnColorStateDescriptor colorStateDescriptor;
    colorStateDescriptor.nextInChain = nullptr;
    colorStateDescriptor.format = DAWN_TEXTURE_FORMAT_R8_G8_B8_A8_UNORM;
    colorStateDescriptor.alphaBlend = blendDescriptor;
    colorStateDescriptor.colorBlend = blendDescriptor;
    colorStateDescriptor.writeMask = DAWN_COLOR_WRITE_MASK_ALL;

    // Create the input state
    DawnVertexInputDescriptor vertexInput;
    vertexInput.nextInChain = nullptr;
    vertexInput.indexFormat = DAWN_INDEX_FORMAT_UINT32;
    vertexInput.bufferCount = 0;
    vertexInput.buffers = nullptr;

    // Create the rasterization state
    DawnRasterizationStateDescriptor rasterizationState;
    rasterizationState.nextInChain = nullptr;
    rasterizationState.frontFace = DAWN_FRONT_FACE_CCW;
    rasterizationState.cullMode = DAWN_CULL_MODE_NONE;
    rasterizationState.depthBias = 0;
    rasterizationState.depthBiasSlopeScale = 0.0;
    rasterizationState.depthBiasClamp = 0.0;

    // Create the depth-stencil state
    DawnStencilStateFaceDescriptor stencilFace;
    stencilFace.compare = DAWN_COMPARE_FUNCTION_ALWAYS;
    stencilFace.failOp = DAWN_STENCIL_OPERATION_KEEP;
    stencilFace.depthFailOp = DAWN_STENCIL_OPERATION_KEEP;
    stencilFace.passOp = DAWN_STENCIL_OPERATION_KEEP;

    DawnDepthStencilStateDescriptor depthStencilState;
    depthStencilState.nextInChain = nullptr;
    depthStencilState.format = DAWN_TEXTURE_FORMAT_D32_FLOAT_S8_UINT;
    depthStencilState.depthWriteEnabled = false;
    depthStencilState.depthCompare = DAWN_COMPARE_FUNCTION_ALWAYS;
    depthStencilState.stencilBack = stencilFace;
    depthStencilState.stencilFront = stencilFace;
    depthStencilState.stencilReadMask = 0xff;
    depthStencilState.stencilWriteMask = 0xff;

    // Create the pipeline layout
    DawnPipelineLayoutDescriptor layoutDescriptor;
    layoutDescriptor.nextInChain = nullptr;
    layoutDescriptor.bindGroupLayoutCount = 0;
    layoutDescriptor.bindGroupLayouts = nullptr;
    DawnPipelineLayout layout = dawnDeviceCreatePipelineLayout(device, &layoutDescriptor);
    DawnPipelineLayout apiLayout = api.GetNewPipelineLayout();
    EXPECT_CALL(api, DeviceCreatePipelineLayout(apiDevice, _)).WillOnce(Return(apiLayout));

    // Create pipeline
    DawnRenderPipelineDescriptor pipelineDescriptor;
    pipelineDescriptor.nextInChain = nullptr;

    DawnPipelineStageDescriptor vertexStage;
    vertexStage.nextInChain = nullptr;
    vertexStage.module = vsModule;
    vertexStage.entryPoint = "main";
    pipelineDescriptor.vertexStage = &vertexStage;

    DawnPipelineStageDescriptor fragmentStage;
    fragmentStage.nextInChain = nullptr;
    fragmentStage.module = vsModule;
    fragmentStage.entryPoint = "main";
    pipelineDescriptor.fragmentStage = &fragmentStage;

    pipelineDescriptor.colorStateCount = 1;
    DawnColorStateDescriptor* colorStatesPtr[] = {&colorStateDescriptor};
    pipelineDescriptor.colorStates = colorStatesPtr;

    pipelineDescriptor.sampleCount = 1;
    pipelineDescriptor.layout = layout;
    pipelineDescriptor.vertexInput = &vertexInput;
    pipelineDescriptor.primitiveTopology = DAWN_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipelineDescriptor.rasterizationState = &rasterizationState;

    // First case: depthStencilState is not null.
    pipelineDescriptor.depthStencilState = &depthStencilState;
    dawnDeviceCreateRenderPipeline(device, &pipelineDescriptor);

    DawnRenderPipeline apiDummyPipeline = api.GetNewRenderPipeline();
    EXPECT_CALL(
        api,
        DeviceCreateRenderPipeline(
            apiDevice, MatchesLambda([](const DawnRenderPipelineDescriptor* desc) -> bool {
                return desc->depthStencilState != nullptr &&
                       desc->depthStencilState->nextInChain == nullptr &&
                       desc->depthStencilState->depthWriteEnabled == false &&
                       desc->depthStencilState->depthCompare == DAWN_COMPARE_FUNCTION_ALWAYS &&
                       desc->depthStencilState->stencilBack.compare ==
                           DAWN_COMPARE_FUNCTION_ALWAYS &&
                       desc->depthStencilState->stencilBack.failOp == DAWN_STENCIL_OPERATION_KEEP &&
                       desc->depthStencilState->stencilBack.depthFailOp ==
                           DAWN_STENCIL_OPERATION_KEEP &&
                       desc->depthStencilState->stencilBack.passOp == DAWN_STENCIL_OPERATION_KEEP &&
                       desc->depthStencilState->stencilFront.compare ==
                           DAWN_COMPARE_FUNCTION_ALWAYS &&
                       desc->depthStencilState->stencilFront.failOp ==
                           DAWN_STENCIL_OPERATION_KEEP &&
                       desc->depthStencilState->stencilFront.depthFailOp ==
                           DAWN_STENCIL_OPERATION_KEEP &&
                       desc->depthStencilState->stencilFront.passOp ==
                           DAWN_STENCIL_OPERATION_KEEP &&
                       desc->depthStencilState->stencilReadMask == 0xff &&
                       desc->depthStencilState->stencilWriteMask == 0xff;
            })))
        .WillOnce(Return(apiDummyPipeline));

    FlushClient();

    // Second case: depthStencilState is null.
    pipelineDescriptor.depthStencilState = nullptr;
    dawnDeviceCreateRenderPipeline(device, &pipelineDescriptor);
    EXPECT_CALL(api,
                DeviceCreateRenderPipeline(
                    apiDevice, MatchesLambda([](const DawnRenderPipelineDescriptor* desc) -> bool {
                        return desc->depthStencilState == nullptr;
                    })))
        .WillOnce(Return(apiDummyPipeline));

    FlushClient();
}
