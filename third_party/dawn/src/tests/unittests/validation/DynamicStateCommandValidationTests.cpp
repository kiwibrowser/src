// Copyright 2018 The Dawn Authors
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

#include <cmath>

class SetViewportTest : public ValidationTest {};

// Test to check basic use of SetViewport
TEST_F(SetViewportTest, Success) {
    DummyRenderPass renderPass(device);

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.SetViewport(0.0, 0.0, 1.0, 1.0, 0.0, 1.0);
        pass.EndPass();
    }
    encoder.Finish();
}

// Test to check that NaN in viewport parameters is not allowed
TEST_F(SetViewportTest, ViewportParameterNaN) {
    DummyRenderPass renderPass(device);

    // x or y is NaN.
    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.SetViewport(NAN, 0.0, 1.0, 1.0, 0.0, 1.0);
        pass.EndPass();
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }

    // width or height is NaN.
    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.SetViewport(0.0, 0.0, NAN, 1.0, 0.0, 1.0);
        pass.EndPass();
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }

    // minDepth or maxDepth is NaN.
    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.SetViewport(0.0, 0.0, 1.0, 1.0, NAN, 1.0);
        pass.EndPass();
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }
}

// Test to check that an empty viewport is not allowed
TEST_F(SetViewportTest, EmptyViewport) {
    DummyRenderPass renderPass(device);

    // Width of viewport is zero.
    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.SetViewport(0.0, 0.0, 0.0, 1.0, 0.0, 1.0);
        pass.EndPass();
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }

    // Height of viewport is zero.
    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.SetViewport(0.0, 0.0, 1.0, 0.0, 0.0, 1.0);
        pass.EndPass();
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }

    // Both width and height of viewport are zero.
    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.SetViewport(0.0, 0.0, 0.0, 0.0, 0.0, 1.0);
        pass.EndPass();
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }
}

// Test to check that viewport larger than the framebuffer is allowed
TEST_F(SetViewportTest, ViewportLargerThanFramebuffer) {
    DummyRenderPass renderPass(device);

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.SetViewport(0.0, 0.0, renderPass.width + 1, renderPass.height + 1, 0.0, 1.0);
        pass.EndPass();
    }
    encoder.Finish();
}

// Test to check that negative x in viewport is allowed
TEST_F(SetViewportTest, NegativeX) {
    DummyRenderPass renderPass(device);

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.SetViewport(-1.0, 0.0, 1.0, 1.0, 0.0, 1.0);
        pass.EndPass();
    }
    encoder.Finish();
}

// Test to check that negative y in viewport is allowed
TEST_F(SetViewportTest, NegativeY) {
    DummyRenderPass renderPass(device);

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.SetViewport(0.0, -1.0, 1.0, 1.0, 0.0, 1.0);
        pass.EndPass();
    }
    encoder.Finish();
}

// Test to check that negative width in viewport is not allowed
TEST_F(SetViewportTest, NegativeWidth) {
    DummyRenderPass renderPass(device);

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.SetViewport(0.0, 0.0, -1.0, 1.0, 0.0, 1.0);
        pass.EndPass();
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }
}

// Test to check that negative height in viewport is not allowed
TEST_F(SetViewportTest, NegativeHeight) {
    DummyRenderPass renderPass(device);

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.SetViewport(0.0, 0.0, 0.0, -1.0, 0.0, 1.0);
        pass.EndPass();
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }
}

// Test to check that minDepth out of range [0, 1] is not allowed
TEST_F(SetViewportTest, MinDepthOutOfRange) {
    DummyRenderPass renderPass(device);

    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.SetViewport(0.0, 0.0, 1.0, 1.0, -1.0, 1.0);
        pass.EndPass();
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }

    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.SetViewport(0.0, 0.0, 1.0, 1.0, 2.0, 1.0);
        pass.EndPass();
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }
}

// Test to check that maxDepth out of range [0, 1] is not allowed
TEST_F(SetViewportTest, MaxDepthOutOfRange) {
    DummyRenderPass renderPass(device);

    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.SetViewport(0.0, 0.0, 1.0, 1.0, 0.0, -1.0);
        pass.EndPass();
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }

    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.SetViewport(0.0, 0.0, 1.0, 1.0, 0.0, 2.0);
        pass.EndPass();
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }
}

// Test to check that minDepth equal or greater than maxDepth is allowed
TEST_F(SetViewportTest, MinDepthEqualOrGreaterThanMaxDepth) {
    DummyRenderPass renderPass(device);

    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.SetViewport(0.0, 0.0, 1.0, 1.0, 0.5, 0.5);
        pass.EndPass();
        encoder.Finish();
    }

    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.SetViewport(0.0, 0.0, 1.0, 1.0, 0.8, 0.5);
        pass.EndPass();
        encoder.Finish();
    }
}

class SetScissorRectTest : public ValidationTest {};

// Test to check basic use of SetScissor
TEST_F(SetScissorRectTest, Success) {
    DummyRenderPass renderPass(device);

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.SetScissorRect(0, 0, 1, 1);
        pass.EndPass();
    }
    encoder.Finish();
}

// Test to check that an empty scissor is not allowed
TEST_F(SetScissorRectTest, EmptyScissor) {
    DummyRenderPass renderPass(device);

    // Width of scissor rect is zero.
    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.SetScissorRect(0, 0, 0, 1);
        pass.EndPass();
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }

    // Height of scissor rect is zero.
    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.SetScissorRect(0, 0, 1, 0);
        pass.EndPass();
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }

    // Both width and height of scissor rect are zero.
    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.SetScissorRect(0, 0, 0, 0);
        pass.EndPass();
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }
}

// Test to check that a scissor larger than the framebuffer is allowed
TEST_F(SetScissorRectTest, ScissorLargerThanFramebuffer) {
    DummyRenderPass renderPass(device);

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.SetScissorRect(0, 0, renderPass.width + 1, renderPass.height + 1);
        pass.EndPass();
    }
    encoder.Finish();
}

class SetBlendColorTest : public ValidationTest {};

// Test to check basic use of SetBlendColor
TEST_F(SetBlendColorTest, Success) {
    DummyRenderPass renderPass(device);

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        constexpr wgpu::Color kTransparentBlack{0.0f, 0.0f, 0.0f, 0.0f};
        pass.SetBlendColor(&kTransparentBlack);
        pass.EndPass();
    }
    encoder.Finish();
}

// Test that SetBlendColor allows any value, large, small or negative
TEST_F(SetBlendColorTest, AnyValueAllowed) {
    DummyRenderPass renderPass(device);

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        constexpr wgpu::Color kAnyColorValue{-1.0f, 42.0f, -0.0f, 0.0f};
        pass.SetBlendColor(&kAnyColorValue);
        pass.EndPass();
    }
    encoder.Finish();
}

class SetStencilReferenceTest : public ValidationTest {};

// Test to check basic use of SetStencilReferenceTest
TEST_F(SetStencilReferenceTest, Success) {
    DummyRenderPass renderPass(device);

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.SetStencilReference(0);
        pass.EndPass();
    }
    encoder.Finish();
}

// Test that SetStencilReference allows any bit to be set
TEST_F(SetStencilReferenceTest, AllBitsAllowed) {
    DummyRenderPass renderPass(device);

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.SetStencilReference(0xFFFFFFFF);
        pass.EndPass();
    }
    encoder.Finish();
}
