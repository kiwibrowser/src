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
    DawnCommandEncoder encoder = dawnDeviceCreateCommandEncoder(device);
    DawnComputePassEncoder pass = dawnCommandEncoderBeginComputePass(encoder);
    dawnComputePassEncoderDispatch(pass, 1, 2, 3);

    DawnCommandEncoder apiEncoder = api.GetNewCommandEncoder();
    EXPECT_CALL(api, DeviceCreateCommandEncoder(apiDevice)).WillOnce(Return(apiEncoder));

    DawnComputePassEncoder apiPass = api.GetNewComputePassEncoder();
    EXPECT_CALL(api, CommandEncoderBeginComputePass(apiEncoder)).WillOnce(Return(apiPass));

    EXPECT_CALL(api, ComputePassEncoderDispatch(apiPass, 1, 2, 3)).Times(1);

    FlushClient();
}

// Test that the wire is able to send arrays of numerical values
TEST_F(WireArgumentTests, ValueArrayArgument) {
    // Create a bindgroup.
    DawnBindGroupLayoutDescriptor bglDescriptor;
    bglDescriptor.nextInChain = nullptr;
    bglDescriptor.bindingCount = 0;
    bglDescriptor.bindings = nullptr;

    DawnBindGroupLayout bgl = dawnDeviceCreateBindGroupLayout(device, &bglDescriptor);
    DawnBindGroupLayout apiBgl = api.GetNewBindGroupLayout();
    EXPECT_CALL(api, DeviceCreateBindGroupLayout(apiDevice, _)).WillOnce(Return(apiBgl));

    DawnBindGroupDescriptor bindGroupDescriptor;
    bindGroupDescriptor.nextInChain = nullptr;
    bindGroupDescriptor.layout = bgl;
    bindGroupDescriptor.bindingCount = 0;
    bindGroupDescriptor.bindings = nullptr;

    DawnBindGroup bindGroup = dawnDeviceCreateBindGroup(device, &bindGroupDescriptor);
    DawnBindGroup apiBindGroup = api.GetNewBindGroup();
    EXPECT_CALL(api, DeviceCreateBindGroup(apiDevice, _)).WillOnce(Return(apiBindGroup));

    // Use the bindgroup in SetBindGroup that takes an array of value offsets.
    DawnCommandEncoder encoder = dawnDeviceCreateCommandEncoder(device);
    DawnComputePassEncoder pass = dawnCommandEncoderBeginComputePass(encoder);

    std::array<uint64_t, 4> testOffsets = {0, 42, 0xDEAD'BEEF'DEAD'BEEFu, 0xFFFF'FFFF'FFFF'FFFFu};
    dawnComputePassEncoderSetBindGroup(pass, 0, bindGroup, testOffsets.size(), testOffsets.data());

    DawnCommandEncoder apiEncoder = api.GetNewCommandEncoder();
    EXPECT_CALL(api, DeviceCreateCommandEncoder(apiDevice)).WillOnce(Return(apiEncoder));

    DawnComputePassEncoder apiPass = api.GetNewComputePassEncoder();
    EXPECT_CALL(api, CommandEncoderBeginComputePass(apiEncoder)).WillOnce(Return(apiPass));

    EXPECT_CALL(api, ComputePassEncoderSetBindGroup(
                         apiPass, 0, apiBindGroup, testOffsets.size(),
                         MatchesLambda([testOffsets](const uint64_t* offsets) -> bool {
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
    pipelineDescriptor.depthStencilState = &depthStencilState;

    dawnDeviceCreateRenderPipeline(device, &pipelineDescriptor);

    DawnRenderPipeline apiDummyPipeline = api.GetNewRenderPipeline();
    EXPECT_CALL(api,
                DeviceCreateRenderPipeline(
                    apiDevice, MatchesLambda([](const DawnRenderPipelineDescriptor* desc) -> bool {
                        return desc->vertexStage->entryPoint == std::string("main");
                    })))
        .WillOnce(Return(apiDummyPipeline));

    FlushClient();
}


// Test that the wire is able to send objects as value arguments
TEST_F(WireArgumentTests, ObjectAsValueArgument) {
    DawnCommandEncoder cmdBufEncoder = dawnDeviceCreateCommandEncoder(device);
    DawnCommandEncoder apiEncoder = api.GetNewCommandEncoder();
    EXPECT_CALL(api, DeviceCreateCommandEncoder(apiDevice)).WillOnce(Return(apiEncoder));

    DawnBufferDescriptor descriptor;
    descriptor.nextInChain = nullptr;
    descriptor.size = 8;
    descriptor.usage = static_cast<DawnBufferUsageBit>(DAWN_BUFFER_USAGE_BIT_TRANSFER_SRC |
                                                       DAWN_BUFFER_USAGE_BIT_TRANSFER_DST);

    DawnBuffer buffer = dawnDeviceCreateBuffer(device, &descriptor);
    DawnBuffer apiBuffer = api.GetNewBuffer();
    EXPECT_CALL(api, DeviceCreateBuffer(apiDevice, _))
        .WillOnce(Return(apiBuffer))
        .RetiresOnSaturation();

    dawnCommandEncoderCopyBufferToBuffer(cmdBufEncoder, buffer, 0, buffer, 4, 4);
    EXPECT_CALL(api, CommandEncoderCopyBufferToBuffer(apiEncoder, apiBuffer, 0, apiBuffer, 4, 4));

    FlushClient();
}

// Test that the wire is able to send array of objects
TEST_F(WireArgumentTests, ObjectsAsPointerArgument) {
    DawnCommandBuffer cmdBufs[2];
    DawnCommandBuffer apiCmdBufs[2];

    // Create two command buffers we need to use a GMock sequence otherwise the order of the
    // CreateCommandEncoder might be swapped since they are equivalent in term of matchers
    Sequence s;
    for (int i = 0; i < 2; ++i) {
        DawnCommandEncoder cmdBufEncoder = dawnDeviceCreateCommandEncoder(device);
        cmdBufs[i] = dawnCommandEncoderFinish(cmdBufEncoder);

        DawnCommandEncoder apiCmdBufEncoder = api.GetNewCommandEncoder();
        EXPECT_CALL(api, DeviceCreateCommandEncoder(apiDevice))
            .InSequence(s)
            .WillOnce(Return(apiCmdBufEncoder));

        apiCmdBufs[i] = api.GetNewCommandBuffer();
        EXPECT_CALL(api, CommandEncoderFinish(apiCmdBufEncoder))
            .WillOnce(Return(apiCmdBufs[i]));
    }

    // Create queue
    DawnQueue queue = dawnDeviceCreateQueue(device);
    DawnQueue apiQueue = api.GetNewQueue();
    EXPECT_CALL(api, DeviceCreateQueue(apiDevice)).WillOnce(Return(apiQueue));

    // Submit command buffer and check we got a call with both API-side command buffers
    dawnQueueSubmit(queue, 2, cmdBufs);

    EXPECT_CALL(
        api, QueueSubmit(apiQueue, 2, MatchesLambda([=](const DawnCommandBuffer* cmdBufs) -> bool {
                             return cmdBufs[0] == apiCmdBufs[0] && cmdBufs[1] == apiCmdBufs[1];
                         })));

    FlushClient();
}

// Test that the wire is able to send structures that contain pure values (non-objects)
TEST_F(WireArgumentTests, StructureOfValuesArgument) {
    DawnSamplerDescriptor descriptor;
    descriptor.nextInChain = nullptr;
    descriptor.magFilter = DAWN_FILTER_MODE_LINEAR;
    descriptor.minFilter = DAWN_FILTER_MODE_NEAREST;
    descriptor.mipmapFilter = DAWN_FILTER_MODE_LINEAR;
    descriptor.addressModeU = DAWN_ADDRESS_MODE_CLAMP_TO_EDGE;
    descriptor.addressModeV = DAWN_ADDRESS_MODE_REPEAT;
    descriptor.addressModeW = DAWN_ADDRESS_MODE_MIRRORED_REPEAT;
    descriptor.lodMinClamp = kLodMin;
    descriptor.lodMaxClamp = kLodMax;
    descriptor.compareFunction = DAWN_COMPARE_FUNCTION_NEVER;

    dawnDeviceCreateSampler(device, &descriptor);

    DawnSampler apiDummySampler = api.GetNewSampler();
    EXPECT_CALL(api, DeviceCreateSampler(
                         apiDevice, MatchesLambda([](const DawnSamplerDescriptor* desc) -> bool {
                             return desc->nextInChain == nullptr &&
                                    desc->magFilter == DAWN_FILTER_MODE_LINEAR &&
                                    desc->minFilter == DAWN_FILTER_MODE_NEAREST &&
                                    desc->mipmapFilter == DAWN_FILTER_MODE_LINEAR &&
                                    desc->addressModeU == DAWN_ADDRESS_MODE_CLAMP_TO_EDGE &&
                                    desc->addressModeV == DAWN_ADDRESS_MODE_REPEAT &&
                                    desc->addressModeW == DAWN_ADDRESS_MODE_MIRRORED_REPEAT &&
                                    desc->compareFunction == DAWN_COMPARE_FUNCTION_NEVER &&
                                    desc->lodMinClamp == kLodMin && desc->lodMaxClamp == kLodMax;
                         })))
        .WillOnce(Return(apiDummySampler));

    FlushClient();
}

// Test that the wire is able to send structures that contain objects
TEST_F(WireArgumentTests, StructureOfObjectArrayArgument) {
    DawnBindGroupLayoutDescriptor bglDescriptor;
    bglDescriptor.nextInChain = nullptr;
    bglDescriptor.bindingCount = 0;
    bglDescriptor.bindings = nullptr;

    DawnBindGroupLayout bgl = dawnDeviceCreateBindGroupLayout(device, &bglDescriptor);
    DawnBindGroupLayout apiBgl = api.GetNewBindGroupLayout();
    EXPECT_CALL(api, DeviceCreateBindGroupLayout(apiDevice, _)).WillOnce(Return(apiBgl));

    DawnPipelineLayoutDescriptor descriptor;
    descriptor.nextInChain = nullptr;
    descriptor.bindGroupLayoutCount = 1;
    descriptor.bindGroupLayouts = &bgl;

    dawnDeviceCreatePipelineLayout(device, &descriptor);

    DawnPipelineLayout apiDummyLayout = api.GetNewPipelineLayout();
    EXPECT_CALL(api, DeviceCreatePipelineLayout(
                         apiDevice,
                         MatchesLambda([apiBgl](const DawnPipelineLayoutDescriptor* desc) -> bool {
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
    DawnBindGroupLayoutBinding bindings[NUM_BINDINGS]{
        {0, DAWN_SHADER_STAGE_BIT_VERTEX, DAWN_BINDING_TYPE_SAMPLER},
        {1, DAWN_SHADER_STAGE_BIT_VERTEX, DAWN_BINDING_TYPE_SAMPLED_TEXTURE},
        {2,
         static_cast<DawnShaderStageBit>(DAWN_SHADER_STAGE_BIT_VERTEX |
                                         DAWN_SHADER_STAGE_BIT_FRAGMENT),
         DAWN_BINDING_TYPE_UNIFORM_BUFFER},
    };
    DawnBindGroupLayoutDescriptor bglDescriptor;
    bglDescriptor.bindingCount = NUM_BINDINGS;
    bglDescriptor.bindings = bindings;

    dawnDeviceCreateBindGroupLayout(device, &bglDescriptor);
    DawnBindGroupLayout apiBgl = api.GetNewBindGroupLayout();
    EXPECT_CALL(
        api,
        DeviceCreateBindGroupLayout(
            apiDevice, MatchesLambda([bindings](const DawnBindGroupLayoutDescriptor* desc) -> bool {
                for (int i = 0; i < NUM_BINDINGS; ++i) {
                    const auto& a = desc->bindings[i];
                    const auto& b = bindings[i];
                    if (a.binding != b.binding || a.visibility != b.visibility ||
                        a.type != b.type) {
                        return false;
                    }
                }
                return desc->nextInChain == nullptr && desc->bindingCount == 3;
            })))
        .WillOnce(Return(apiBgl));

    FlushClient();
}

// Test passing nullptr instead of objects - array of objects version
TEST_F(WireArgumentTests, DISABLED_NullptrInArray) {
    DawnBindGroupLayout nullBGL = nullptr;

    DawnPipelineLayoutDescriptor descriptor;
    descriptor.nextInChain = nullptr;
    descriptor.bindGroupLayoutCount = 1;
    descriptor.bindGroupLayouts = &nullBGL;

    dawnDeviceCreatePipelineLayout(device, &descriptor);
    EXPECT_CALL(api,
                DeviceCreatePipelineLayout(
                    apiDevice, MatchesLambda([](const DawnPipelineLayoutDescriptor* desc) -> bool {
                        return desc->nextInChain == nullptr && desc->bindGroupLayoutCount == 1 &&
                               desc->bindGroupLayouts[0] == nullptr;
                    })))
        .WillOnce(Return(nullptr));

    FlushClient();
}
