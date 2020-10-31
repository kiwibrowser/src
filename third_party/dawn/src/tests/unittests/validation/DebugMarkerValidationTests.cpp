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

#include "tests/unittests/validation/ValidationTest.h"

#include "utils/WGPUHelpers.h"

class DebugMarkerValidationTest : public ValidationTest {};

// Correct usage of debug markers should succeed in render pass.
TEST_F(DebugMarkerValidationTest, RenderSuccess) {
    DummyRenderPass renderPass(device);

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.PushDebugGroup("Event Start");
        pass.PushDebugGroup("Event Start");
        pass.InsertDebugMarker("Marker");
        pass.PopDebugGroup();
        pass.PopDebugGroup();
        pass.EndPass();
    }

    encoder.Finish();
}

// A PushDebugGroup call without a following PopDebugGroup produces an error in render pass.
TEST_F(DebugMarkerValidationTest, RenderUnbalancedPush) {
    DummyRenderPass renderPass(device);

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.PushDebugGroup("Event Start");
        pass.PushDebugGroup("Event Start");
        pass.InsertDebugMarker("Marker");
        pass.PopDebugGroup();
        pass.EndPass();
    }

    ASSERT_DEVICE_ERROR(encoder.Finish());
}

// A PopDebugGroup call without a preceding PushDebugGroup produces an error in render pass.
TEST_F(DebugMarkerValidationTest, RenderUnbalancedPop) {
    DummyRenderPass renderPass(device);

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.PushDebugGroup("Event Start");
        pass.InsertDebugMarker("Marker");
        pass.PopDebugGroup();
        pass.PopDebugGroup();
        pass.EndPass();
    }

    ASSERT_DEVICE_ERROR(encoder.Finish());
}

// Correct usage of debug markers should succeed in compute pass.
TEST_F(DebugMarkerValidationTest, ComputeSuccess) {
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
        pass.PushDebugGroup("Event Start");
        pass.PushDebugGroup("Event Start");
        pass.InsertDebugMarker("Marker");
        pass.PopDebugGroup();
        pass.PopDebugGroup();
        pass.EndPass();
    }

    encoder.Finish();
}

// A PushDebugGroup call without a following PopDebugGroup produces an error in compute pass.
TEST_F(DebugMarkerValidationTest, ComputeUnbalancedPush) {
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
        pass.PushDebugGroup("Event Start");
        pass.PushDebugGroup("Event Start");
        pass.InsertDebugMarker("Marker");
        pass.PopDebugGroup();
        pass.EndPass();
    }

    ASSERT_DEVICE_ERROR(encoder.Finish());
}

// A PopDebugGroup call without a preceding PushDebugGroup produces an error in compute pass.
TEST_F(DebugMarkerValidationTest, ComputeUnbalancedPop) {
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
        pass.PushDebugGroup("Event Start");
        pass.InsertDebugMarker("Marker");
        pass.PopDebugGroup();
        pass.PopDebugGroup();
        pass.EndPass();
    }

    ASSERT_DEVICE_ERROR(encoder.Finish());
}

// Correct usage of debug markers should succeed in command encoder.
TEST_F(DebugMarkerValidationTest, CommandEncoderSuccess) {
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    encoder.PushDebugGroup("Event Start");
    encoder.PushDebugGroup("Event Start");
    encoder.InsertDebugMarker("Marker");
    encoder.PopDebugGroup();
    encoder.PopDebugGroup();
    encoder.Finish();
}

// A PushDebugGroup call without a following PopDebugGroup produces an error in command encoder.
TEST_F(DebugMarkerValidationTest, CommandEncoderUnbalancedPush) {
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    encoder.PushDebugGroup("Event Start");
    encoder.PushDebugGroup("Event Start");
    encoder.InsertDebugMarker("Marker");
    encoder.PopDebugGroup();
    ASSERT_DEVICE_ERROR(encoder.Finish());
}

// A PopDebugGroup call without a preceding PushDebugGroup produces an error in command encoder.
TEST_F(DebugMarkerValidationTest, CommandEncoderUnbalancedPop) {
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    encoder.PushDebugGroup("Event Start");
    encoder.InsertDebugMarker("Marker");
    encoder.PopDebugGroup();
    encoder.PopDebugGroup();
    ASSERT_DEVICE_ERROR(encoder.Finish());
}

// It is possible to nested pushes in a compute pass in a command encoder.
TEST_F(DebugMarkerValidationTest, NestedComputeInCommandEncoder) {
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    encoder.PushDebugGroup("Event Start");
    {
        wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
        pass.PushDebugGroup("Event Start");
        pass.InsertDebugMarker("Marker");
        pass.PopDebugGroup();
        pass.EndPass();
    }
    encoder.PopDebugGroup();
    encoder.Finish();
}

// Command encoder and compute pass pushes must be balanced independently.
TEST_F(DebugMarkerValidationTest, NestedComputeInCommandEncoderIndependent) {
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    encoder.PushDebugGroup("Event Start");
    {
        wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
        pass.InsertDebugMarker("Marker");
        pass.PopDebugGroup();
        pass.EndPass();
    }
    ASSERT_DEVICE_ERROR(encoder.Finish());
}

// It is possible to nested pushes in a render pass in a command encoder.
TEST_F(DebugMarkerValidationTest, NestedRenderInCommandEncoder) {
    DummyRenderPass renderPass(device);

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    encoder.PushDebugGroup("Event Start");
    {
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.PushDebugGroup("Event Start");
        pass.InsertDebugMarker("Marker");
        pass.PopDebugGroup();
        pass.EndPass();
    }
    encoder.PopDebugGroup();
    encoder.Finish();
}

// Command encoder and render pass pushes must be balanced independently.
TEST_F(DebugMarkerValidationTest, NestedRenderInCommandEncoderIndependent) {
    DummyRenderPass renderPass(device);

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    encoder.PushDebugGroup("Event Start");
    {
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.InsertDebugMarker("Marker");
        pass.PopDebugGroup();
        pass.EndPass();
    }
    ASSERT_DEVICE_ERROR(encoder.Finish());
}
