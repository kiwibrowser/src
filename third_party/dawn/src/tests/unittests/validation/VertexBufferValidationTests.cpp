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

#include "tests/unittests/validation/ValidationTest.h"

#include "utils/ComboRenderPipelineDescriptor.h"
#include "utils/DawnHelpers.h"

class VertexBufferValidationTest : public ValidationTest {
    protected:
        void SetUp() override {
            ValidationTest::SetUp();

            fsModule = utils::CreateShaderModule(device, dawn::ShaderStage::Fragment, R"(
                #version 450
                layout(location = 0) out vec4 fragColor;
                void main() {
                    fragColor = vec4(0.0, 1.0, 0.0, 1.0);
                })");
        }

        template <unsigned int N>
        std::array<dawn::Buffer, N> MakeVertexBuffers() {
            std::array<dawn::Buffer, N> buffers;
            for (auto& buffer : buffers) {
                dawn::BufferDescriptor descriptor;
                descriptor.size = 256;
                descriptor.usage = dawn::BufferUsageBit::Vertex;

                buffer = device.CreateBuffer(&descriptor);
            }
            return buffers;
        }

        dawn::ShaderModule MakeVertexShader(unsigned int bufferCount) {
            std::ostringstream vs;
            vs << "#version 450\n";
            for (unsigned int i = 0; i < bufferCount; ++i) {
                vs << "layout(location = " << i << ") in vec3 a_position" << i << ";\n";
            }
            vs << "void main() {\n";

            vs << "gl_Position = vec4(";
            for (unsigned int i = 0; i < bufferCount; ++i) {
                vs << "a_position" << i;
                if (i != bufferCount - 1) {
                    vs << " + ";
                }
            }
            vs << ", 1.0);";

            vs << "}\n";

            return utils::CreateShaderModule(device, dawn::ShaderStage::Vertex, vs.str().c_str());
        }

        dawn::RenderPipeline MakeRenderPipeline(const dawn::ShaderModule& vsModule,
                                                unsigned int bufferCount) {
            utils::ComboRenderPipelineDescriptor descriptor(device);
            descriptor.cVertexStage.module = vsModule;
            descriptor.cFragmentStage.module = fsModule;

            for (unsigned int i = 0; i < bufferCount; ++i) {
                descriptor.cVertexInput.cBuffers[i].attributeCount = 1;
                descriptor.cVertexInput.cBuffers[i].attributes =
                    &descriptor.cVertexInput.cAttributes[i];
                descriptor.cVertexInput.cAttributes[i].shaderLocation = i;
                descriptor.cVertexInput.cAttributes[i].format = dawn::VertexFormat::Float3;
            }
            descriptor.cVertexInput.bufferCount = bufferCount;

            return device.CreateRenderPipeline(&descriptor);
        }

        dawn::ShaderModule fsModule;
};

TEST_F(VertexBufferValidationTest, VertexBuffersInheritedBetweenPipelines) {
    DummyRenderPass renderPass(device);
    auto vsModule2 = MakeVertexShader(2);
    auto vsModule1 = MakeVertexShader(1);

    auto pipeline2 = MakeRenderPipeline(vsModule2, 2);
    auto pipeline1 = MakeRenderPipeline(vsModule1, 1);

    auto vertexBuffers = MakeVertexBuffers<2>();
    uint64_t offsets[] = { 0, 0 };

    // Check failure when vertex buffer is not set
    dawn::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        dawn::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.SetPipeline(pipeline1);
        pass.Draw(3, 1, 0, 0);
        pass.EndPass();
    }
    ASSERT_DEVICE_ERROR(encoder.Finish());

    // Check success when vertex buffer is inherited from previous pipeline
    encoder = device.CreateCommandEncoder();
    {
        dawn::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.SetPipeline(pipeline2);
        pass.SetVertexBuffers(0, 2, vertexBuffers.data(), offsets);
        pass.Draw(3, 1, 0, 0);
        pass.SetPipeline(pipeline1);
        pass.Draw(3, 1, 0, 0);
        pass.EndPass();
    }
    encoder.Finish();
}

TEST_F(VertexBufferValidationTest, VertexBuffersNotInheritedBetweenRendePasses) {
    DummyRenderPass renderPass(device);
    auto vsModule2 = MakeVertexShader(2);
    auto vsModule1 = MakeVertexShader(1);

    auto pipeline2 = MakeRenderPipeline(vsModule2, 2);
    auto pipeline1 = MakeRenderPipeline(vsModule1, 1);

    auto vertexBuffers = MakeVertexBuffers<2>();
    uint64_t offsets[] = { 0, 0 };

    // Check success when vertex buffer is set for each render pass
    dawn::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        dawn::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.SetPipeline(pipeline2);
        pass.SetVertexBuffers(0, 2, vertexBuffers.data(), offsets);
        pass.Draw(3, 1, 0, 0);
        pass.EndPass();
    }
    {
        dawn::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.SetPipeline(pipeline1);
        pass.SetVertexBuffers(0, 1, vertexBuffers.data(), offsets);
        pass.Draw(3, 1, 0, 0);
        pass.EndPass();
    }
    encoder.Finish();

    // Check failure because vertex buffer is not inherited in second subpass
    encoder = device.CreateCommandEncoder();
    {
        dawn::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.SetPipeline(pipeline2);
        pass.SetVertexBuffers(0, 2, vertexBuffers.data(), offsets);
        pass.Draw(3, 1, 0, 0);
        pass.EndPass();
    }
    {
        dawn::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.SetPipeline(pipeline1);
        pass.Draw(3, 1, 0, 0);
        pass.EndPass();
    }
    ASSERT_DEVICE_ERROR(encoder.Finish());
}
