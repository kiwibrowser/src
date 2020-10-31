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

#include "tests/unittests/wire/WireTest.h"

using namespace testing;
using namespace dawn_wire;

class WireExtensionTests : public WireTest {
  public:
    WireExtensionTests() {
    }
    ~WireExtensionTests() override = default;
};

// Serialize/Deserializes a chained struct correctly.
TEST_F(WireExtensionTests, ChainedStruct) {
    WGPUSamplerDescriptorDummyAnisotropicFiltering clientExt = {};
    clientExt.chain.sType = WGPUSType_SamplerDescriptorDummyAnisotropicFiltering;
    clientExt.chain.next = nullptr;
    clientExt.maxAnisotropy = 3.14;

    WGPUSamplerDescriptor clientDesc = {};
    clientDesc.nextInChain = &clientExt.chain;
    clientDesc.label = "sampler with anisotropic filtering";

    wgpuDeviceCreateSampler(device, &clientDesc);
    EXPECT_CALL(api, DeviceCreateSampler(apiDevice, NotNull()))
        .WillOnce(Invoke([&](Unused, const WGPUSamplerDescriptor* serverDesc) -> WGPUSampler {
            EXPECT_STREQ(serverDesc->label, clientDesc.label);

            const auto* ext =
                reinterpret_cast<const WGPUSamplerDescriptorDummyAnisotropicFiltering*>(
                    serverDesc->nextInChain);
            EXPECT_EQ(ext->chain.sType, clientExt.chain.sType);
            EXPECT_EQ(ext->maxAnisotropy, clientExt.maxAnisotropy);

            EXPECT_EQ(ext->chain.next, nullptr);

            return api.GetNewSampler();
        }));
    FlushClient();
}

// Serialize/Deserializes multiple chained structs correctly.
TEST_F(WireExtensionTests, MutlipleChainedStructs) {
    WGPUSamplerDescriptorDummyAnisotropicFiltering clientExt2 = {};
    clientExt2.chain.sType = WGPUSType_SamplerDescriptorDummyAnisotropicFiltering;
    clientExt2.chain.next = nullptr;
    clientExt2.maxAnisotropy = 2.71828;

    WGPUSamplerDescriptorDummyAnisotropicFiltering clientExt1 = {};
    clientExt1.chain.sType = WGPUSType_SamplerDescriptorDummyAnisotropicFiltering;
    clientExt1.chain.next = &clientExt2.chain;
    clientExt1.maxAnisotropy = 3.14;

    WGPUSamplerDescriptor clientDesc = {};
    clientDesc.nextInChain = &clientExt1.chain;
    clientDesc.label = "sampler with anisotropic filtering";

    wgpuDeviceCreateSampler(device, &clientDesc);
    EXPECT_CALL(api, DeviceCreateSampler(apiDevice, NotNull()))
        .WillOnce(Invoke([&](Unused, const WGPUSamplerDescriptor* serverDesc) -> WGPUSampler {
            EXPECT_STREQ(serverDesc->label, clientDesc.label);

            const auto* ext1 =
                reinterpret_cast<const WGPUSamplerDescriptorDummyAnisotropicFiltering*>(
                    serverDesc->nextInChain);
            EXPECT_EQ(ext1->chain.sType, clientExt1.chain.sType);
            EXPECT_EQ(ext1->maxAnisotropy, clientExt1.maxAnisotropy);

            const auto* ext2 =
                reinterpret_cast<const WGPUSamplerDescriptorDummyAnisotropicFiltering*>(
                    ext1->chain.next);
            EXPECT_EQ(ext2->chain.sType, clientExt2.chain.sType);
            EXPECT_EQ(ext2->maxAnisotropy, clientExt2.maxAnisotropy);

            EXPECT_EQ(ext2->chain.next, nullptr);

            return api.GetNewSampler();
        }));
    FlushClient();

    // Swap the order of the chained structs.
    clientDesc.nextInChain = &clientExt2.chain;
    clientExt2.chain.next = &clientExt1.chain;
    clientExt1.chain.next = nullptr;

    wgpuDeviceCreateSampler(device, &clientDesc);
    EXPECT_CALL(api, DeviceCreateSampler(apiDevice, NotNull()))
        .WillOnce(Invoke([&](Unused, const WGPUSamplerDescriptor* serverDesc) -> WGPUSampler {
            EXPECT_STREQ(serverDesc->label, clientDesc.label);

            const auto* ext2 =
                reinterpret_cast<const WGPUSamplerDescriptorDummyAnisotropicFiltering*>(
                    serverDesc->nextInChain);
            EXPECT_EQ(ext2->chain.sType, clientExt2.chain.sType);
            EXPECT_EQ(ext2->maxAnisotropy, clientExt2.maxAnisotropy);

            const auto* ext1 =
                reinterpret_cast<const WGPUSamplerDescriptorDummyAnisotropicFiltering*>(
                    ext2->chain.next);
            EXPECT_EQ(ext1->chain.sType, clientExt1.chain.sType);
            EXPECT_EQ(ext1->maxAnisotropy, clientExt1.maxAnisotropy);

            EXPECT_EQ(ext1->chain.next, nullptr);

            return api.GetNewSampler();
        }));
    FlushClient();
}

// Test that a chained struct with Invalid sType is an error.
TEST_F(WireExtensionTests, InvalidSType) {
    WGPUSamplerDescriptorDummyAnisotropicFiltering clientExt = {};
    clientExt.chain.sType = WGPUSType_Invalid;
    clientExt.chain.next = nullptr;

    WGPUSamplerDescriptor clientDesc = {};
    clientDesc.nextInChain = &clientExt.chain;
    clientDesc.label = "sampler with anisotropic filtering";

    wgpuDeviceCreateSampler(device, &clientDesc);
    FlushClient(false);
}

// Test that if both an invalid and valid stype are passed on the chain, it is an error.
TEST_F(WireExtensionTests, ValidAndInvalidSTypeInChain) {
    WGPUSamplerDescriptorDummyAnisotropicFiltering clientExt2 = {};
    clientExt2.chain.sType = WGPUSType_Invalid;
    clientExt2.chain.next = nullptr;
    clientExt2.maxAnisotropy = 2.71828;

    WGPUSamplerDescriptorDummyAnisotropicFiltering clientExt1 = {};
    clientExt1.chain.sType = WGPUSType_SamplerDescriptorDummyAnisotropicFiltering;
    clientExt1.chain.next = &clientExt2.chain;
    clientExt1.maxAnisotropy = 3.14;

    WGPUSamplerDescriptor clientDesc = {};
    clientDesc.nextInChain = &clientExt1.chain;
    clientDesc.label = "sampler with anisotropic filtering";

    wgpuDeviceCreateSampler(device, &clientDesc);
    FlushClient(false);

    // Swap the order of the chained structs.
    clientDesc.nextInChain = &clientExt2.chain;
    clientExt2.chain.next = &clientExt1.chain;
    clientExt1.chain.next = nullptr;

    wgpuDeviceCreateSampler(device, &clientDesc);
    FlushClient(false);
}

// Test that (de)?serializing a chained struct with subdescriptors works.
TEST_F(WireExtensionTests, ChainedStructWithSubdescriptor) {
    WGPUShaderModuleDescriptor shaderModuleDesc = {};

    WGPUShaderModule apiShaderModule1 = api.GetNewShaderModule();
    WGPUShaderModule shaderModule1 = wgpuDeviceCreateShaderModule(device, &shaderModuleDesc);
    EXPECT_CALL(api, DeviceCreateShaderModule(apiDevice, _)).WillOnce(Return(apiShaderModule1));
    FlushClient();

    WGPUShaderModule apiShaderModule2 = api.GetNewShaderModule();
    WGPUShaderModule shaderModule2 = wgpuDeviceCreateShaderModule(device, &shaderModuleDesc);
    EXPECT_CALL(api, DeviceCreateShaderModule(apiDevice, _)).WillOnce(Return(apiShaderModule2));
    FlushClient();

    WGPUProgrammableStageDescriptor extraStageDesc = {};
    extraStageDesc.module = shaderModule1;
    extraStageDesc.entryPoint = "my other module";

    WGPURenderPipelineDescriptorDummyExtension clientExt = {};
    clientExt.chain.sType = WGPUSType_RenderPipelineDescriptorDummyExtension;
    clientExt.chain.next = nullptr;
    clientExt.dummyStage = extraStageDesc;

    WGPURenderPipelineDescriptor renderPipelineDesc = {};
    renderPipelineDesc.nextInChain = &clientExt.chain;
    renderPipelineDesc.vertexStage.module = shaderModule2;
    renderPipelineDesc.vertexStage.entryPoint = "my vertex module";

    wgpuDeviceCreateRenderPipeline(device, &renderPipelineDesc);
    EXPECT_CALL(api, DeviceCreateRenderPipeline(apiDevice, NotNull()))
        .WillOnce(Invoke([&](Unused,
                             const WGPURenderPipelineDescriptor* serverDesc) -> WGPURenderPipeline {
            EXPECT_EQ(serverDesc->vertexStage.module, apiShaderModule2);
            EXPECT_STREQ(serverDesc->vertexStage.entryPoint,
                         renderPipelineDesc.vertexStage.entryPoint);

            const auto* ext = reinterpret_cast<const WGPURenderPipelineDescriptorDummyExtension*>(
                serverDesc->nextInChain);
            EXPECT_EQ(ext->chain.sType, clientExt.chain.sType);
            EXPECT_EQ(ext->dummyStage.module, apiShaderModule1);
            EXPECT_STREQ(ext->dummyStage.entryPoint, extraStageDesc.entryPoint);

            EXPECT_EQ(ext->chain.next, nullptr);

            return api.GetNewRenderPipeline();
        }));
    FlushClient();
}
