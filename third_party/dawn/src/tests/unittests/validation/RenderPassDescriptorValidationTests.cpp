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

#include "common/Constants.h"

#include "utils/WGPUHelpers.h"

#include <cmath>

namespace {

    class RenderPassDescriptorValidationTest : public ValidationTest {
      public:
        void AssertBeginRenderPassSuccess(const wgpu::RenderPassDescriptor* descriptor) {
            wgpu::CommandEncoder commandEncoder = TestBeginRenderPass(descriptor);
            commandEncoder.Finish();
        }
        void AssertBeginRenderPassError(const wgpu::RenderPassDescriptor* descriptor) {
            wgpu::CommandEncoder commandEncoder = TestBeginRenderPass(descriptor);
            ASSERT_DEVICE_ERROR(commandEncoder.Finish());
        }

      private:
        wgpu::CommandEncoder TestBeginRenderPass(const wgpu::RenderPassDescriptor* descriptor) {
            wgpu::CommandEncoder commandEncoder = device.CreateCommandEncoder();
            wgpu::RenderPassEncoder renderPassEncoder = commandEncoder.BeginRenderPass(descriptor);
            renderPassEncoder.EndPass();
            return commandEncoder;
        }
    };

    wgpu::Texture CreateTexture(wgpu::Device& device,
                                wgpu::TextureDimension dimension,
                                wgpu::TextureFormat format,
                                uint32_t width,
                                uint32_t height,
                                uint32_t arrayLayerCount,
                                uint32_t mipLevelCount,
                                uint32_t sampleCount = 1,
                                wgpu::TextureUsage usage = wgpu::TextureUsage::OutputAttachment) {
        wgpu::TextureDescriptor descriptor;
        descriptor.dimension = dimension;
        descriptor.size.width = width;
        descriptor.size.height = height;
        descriptor.size.depth = arrayLayerCount;
        descriptor.sampleCount = sampleCount;
        descriptor.format = format;
        descriptor.mipLevelCount = mipLevelCount;
        descriptor.usage = usage;

        return device.CreateTexture(&descriptor);
    }

    wgpu::TextureView Create2DAttachment(wgpu::Device& device,
                                         uint32_t width,
                                         uint32_t height,
                                         wgpu::TextureFormat format) {
        wgpu::Texture texture =
            CreateTexture(device, wgpu::TextureDimension::e2D, format, width, height, 1, 1);
        return texture.CreateView();
    }

    // Using BeginRenderPass with no attachments isn't valid
    TEST_F(RenderPassDescriptorValidationTest, Empty) {
        utils::ComboRenderPassDescriptor renderPass({}, nullptr);
        AssertBeginRenderPassError(&renderPass);
    }

    // A render pass with only one color or one depth attachment is ok
    TEST_F(RenderPassDescriptorValidationTest, OneAttachment) {
        // One color attachment
        {
            wgpu::TextureView color =
                Create2DAttachment(device, 1, 1, wgpu::TextureFormat::RGBA8Unorm);
            utils::ComboRenderPassDescriptor renderPass({color});

            AssertBeginRenderPassSuccess(&renderPass);
        }
        // One depth-stencil attachment
        {
            wgpu::TextureView depthStencil =
                Create2DAttachment(device, 1, 1, wgpu::TextureFormat::Depth24PlusStencil8);
            utils::ComboRenderPassDescriptor renderPass({}, depthStencil);

            AssertBeginRenderPassSuccess(&renderPass);
        }
    }

    // Test OOB color attachment indices are handled
    TEST_F(RenderPassDescriptorValidationTest, ColorAttachmentOutOfBounds) {
        wgpu::TextureView color0 =
            Create2DAttachment(device, 1, 1, wgpu::TextureFormat::RGBA8Unorm);
        wgpu::TextureView color1 =
            Create2DAttachment(device, 1, 1, wgpu::TextureFormat::RGBA8Unorm);
        wgpu::TextureView color2 =
            Create2DAttachment(device, 1, 1, wgpu::TextureFormat::RGBA8Unorm);
        wgpu::TextureView color3 =
            Create2DAttachment(device, 1, 1, wgpu::TextureFormat::RGBA8Unorm);
        // For setting the color attachment, control case
        {
            utils::ComboRenderPassDescriptor renderPass({color0, color1, color2, color3});
            AssertBeginRenderPassSuccess(&renderPass);
        }
        // For setting the color attachment, OOB
        {
            // We cannot use utils::ComboRenderPassDescriptor here because it only supports at most
            // kMaxColorAttachments(4) color attachments.
            std::array<wgpu::RenderPassColorAttachmentDescriptor, 5> colorAttachments;
            colorAttachments[0].attachment = color0;
            colorAttachments[0].resolveTarget = nullptr;
            colorAttachments[0].clearColor = {0.0f, 0.0f, 0.0f, 0.0f};
            colorAttachments[0].loadOp = wgpu::LoadOp::Clear;
            colorAttachments[0].storeOp = wgpu::StoreOp::Store;

            colorAttachments[1] = colorAttachments[0];
            colorAttachments[1].attachment = color1;

            colorAttachments[2] = colorAttachments[0];
            colorAttachments[2].attachment = color2;

            colorAttachments[3] = colorAttachments[0];
            colorAttachments[3].attachment = color3;

            colorAttachments[4] = colorAttachments[0];
            colorAttachments[4].attachment =
                Create2DAttachment(device, 1, 1, wgpu::TextureFormat::RGBA8Unorm);

            wgpu::RenderPassDescriptor renderPass;
            renderPass.colorAttachmentCount = 5;
            renderPass.colorAttachments = colorAttachments.data();
            renderPass.depthStencilAttachment = nullptr;
            AssertBeginRenderPassError(&renderPass);
        }
    }

    // Attachments must have the same size
    TEST_F(RenderPassDescriptorValidationTest, SizeMustMatch) {
        wgpu::TextureView color1x1A =
            Create2DAttachment(device, 1, 1, wgpu::TextureFormat::RGBA8Unorm);
        wgpu::TextureView color1x1B =
            Create2DAttachment(device, 1, 1, wgpu::TextureFormat::RGBA8Unorm);
        wgpu::TextureView color2x2 =
            Create2DAttachment(device, 2, 2, wgpu::TextureFormat::RGBA8Unorm);

        wgpu::TextureView depthStencil1x1 =
            Create2DAttachment(device, 1, 1, wgpu::TextureFormat::Depth24PlusStencil8);
        wgpu::TextureView depthStencil2x2 =
            Create2DAttachment(device, 2, 2, wgpu::TextureFormat::Depth24PlusStencil8);

        // Control case: all the same size (1x1)
        {
            utils::ComboRenderPassDescriptor renderPass({color1x1A, color1x1B}, depthStencil1x1);
            AssertBeginRenderPassSuccess(&renderPass);
        }

        // One of the color attachments has a different size
        {
            utils::ComboRenderPassDescriptor renderPass({color1x1A, color2x2});
            AssertBeginRenderPassError(&renderPass);
        }

        // The depth stencil attachment has a different size
        {
            utils::ComboRenderPassDescriptor renderPass({color1x1A, color1x1B}, depthStencil2x2);
            AssertBeginRenderPassError(&renderPass);
        }
    }

    // Attachments formats must match whether they are used for color or depth-stencil
    TEST_F(RenderPassDescriptorValidationTest, FormatMismatch) {
        wgpu::TextureView color = Create2DAttachment(device, 1, 1, wgpu::TextureFormat::RGBA8Unorm);
        wgpu::TextureView depthStencil =
            Create2DAttachment(device, 1, 1, wgpu::TextureFormat::Depth24PlusStencil8);

        // Using depth-stencil for color
        {
            utils::ComboRenderPassDescriptor renderPass({depthStencil});
            AssertBeginRenderPassError(&renderPass);
        }

        // Using color for depth-stencil
        {
            utils::ComboRenderPassDescriptor renderPass({}, color);
            AssertBeginRenderPassError(&renderPass);
        }
    }

    // Depth and stencil storeOps must match
    TEST_F(RenderPassDescriptorValidationTest, DepthStencilStoreOpMismatch) {
        constexpr uint32_t kArrayLayers = 1;
        constexpr uint32_t kLevelCount = 1;
        constexpr uint32_t kSize = 32;
        constexpr wgpu::TextureFormat kColorFormat = wgpu::TextureFormat::RGBA8Unorm;
        constexpr wgpu::TextureFormat kDepthStencilFormat =
            wgpu::TextureFormat::Depth24PlusStencil8;

        wgpu::Texture colorTexture =
            CreateTexture(device, wgpu::TextureDimension::e2D, kColorFormat, kSize, kSize,
                          kArrayLayers, kLevelCount);
        wgpu::Texture depthStencilTexture =
            CreateTexture(device, wgpu::TextureDimension::e2D, kDepthStencilFormat, kSize, kSize,
                          kArrayLayers, kLevelCount);

        wgpu::TextureViewDescriptor descriptor;
        descriptor.dimension = wgpu::TextureViewDimension::e2D;
        descriptor.baseArrayLayer = 0;
        descriptor.arrayLayerCount = kArrayLayers;
        descriptor.baseMipLevel = 0;
        descriptor.mipLevelCount = kLevelCount;
        wgpu::TextureView colorTextureView = colorTexture.CreateView(&descriptor);
        wgpu::TextureView depthStencilView = depthStencilTexture.CreateView(&descriptor);

        // StoreOps mismatch causing the render pass to error
        {
            utils::ComboRenderPassDescriptor renderPass({}, depthStencilView);
            renderPass.cDepthStencilAttachmentInfo.stencilStoreOp = wgpu::StoreOp::Store;
            renderPass.cDepthStencilAttachmentInfo.depthStoreOp = wgpu::StoreOp::Clear;
            AssertBeginRenderPassError(&renderPass);
        }

        // StoreOps match so render pass is a success
        {
            utils::ComboRenderPassDescriptor renderPass({}, depthStencilView);
            renderPass.cDepthStencilAttachmentInfo.stencilStoreOp = wgpu::StoreOp::Store;
            renderPass.cDepthStencilAttachmentInfo.depthStoreOp = wgpu::StoreOp::Store;
            AssertBeginRenderPassSuccess(&renderPass);
        }

        // StoreOps match so render pass is a success
        {
            utils::ComboRenderPassDescriptor renderPass({}, depthStencilView);
            renderPass.cDepthStencilAttachmentInfo.stencilStoreOp = wgpu::StoreOp::Clear;
            renderPass.cDepthStencilAttachmentInfo.depthStoreOp = wgpu::StoreOp::Clear;
            AssertBeginRenderPassSuccess(&renderPass);
        }
    }

    // Currently only texture views with arrayLayerCount == 1 are allowed to be color and depth
    // stencil attachments
    TEST_F(RenderPassDescriptorValidationTest, TextureViewLayerCountForColorAndDepthStencil) {
        constexpr uint32_t kLevelCount = 1;
        constexpr uint32_t kSize = 32;
        constexpr wgpu::TextureFormat kColorFormat = wgpu::TextureFormat::RGBA8Unorm;
        constexpr wgpu::TextureFormat kDepthStencilFormat =
            wgpu::TextureFormat::Depth24PlusStencil8;

        constexpr uint32_t kArrayLayers = 10;

        wgpu::Texture colorTexture =
            CreateTexture(device, wgpu::TextureDimension::e2D, kColorFormat, kSize, kSize,
                          kArrayLayers, kLevelCount);
        wgpu::Texture depthStencilTexture =
            CreateTexture(device, wgpu::TextureDimension::e2D, kDepthStencilFormat, kSize, kSize,
                          kArrayLayers, kLevelCount);

        wgpu::TextureViewDescriptor baseDescriptor;
        baseDescriptor.dimension = wgpu::TextureViewDimension::e2DArray;
        baseDescriptor.baseArrayLayer = 0;
        baseDescriptor.arrayLayerCount = kArrayLayers;
        baseDescriptor.baseMipLevel = 0;
        baseDescriptor.mipLevelCount = kLevelCount;

        // Using 2D array texture view with arrayLayerCount > 1 is not allowed for color
        {
            wgpu::TextureViewDescriptor descriptor = baseDescriptor;
            descriptor.format = kColorFormat;
            descriptor.arrayLayerCount = 5;

            wgpu::TextureView colorTextureView = colorTexture.CreateView(&descriptor);
            utils::ComboRenderPassDescriptor renderPass({colorTextureView});
            AssertBeginRenderPassError(&renderPass);
        }

        // Using 2D array texture view with arrayLayerCount > 1 is not allowed for depth stencil
        {
            wgpu::TextureViewDescriptor descriptor = baseDescriptor;
            descriptor.format = kDepthStencilFormat;
            descriptor.arrayLayerCount = 5;

            wgpu::TextureView depthStencilView = depthStencilTexture.CreateView(&descriptor);
            utils::ComboRenderPassDescriptor renderPass({}, depthStencilView);
            AssertBeginRenderPassError(&renderPass);
        }

        // Using 2D array texture view that covers the first layer of the texture is OK for color
        {
            wgpu::TextureViewDescriptor descriptor = baseDescriptor;
            descriptor.format = kColorFormat;
            descriptor.baseArrayLayer = 0;
            descriptor.arrayLayerCount = 1;

            wgpu::TextureView colorTextureView = colorTexture.CreateView(&descriptor);
            utils::ComboRenderPassDescriptor renderPass({colorTextureView});
            AssertBeginRenderPassSuccess(&renderPass);
        }

        // Using 2D array texture view that covers the first layer is OK for depth stencil
        {
            wgpu::TextureViewDescriptor descriptor = baseDescriptor;
            descriptor.format = kDepthStencilFormat;
            descriptor.baseArrayLayer = 0;
            descriptor.arrayLayerCount = 1;

            wgpu::TextureView depthStencilView = depthStencilTexture.CreateView(&descriptor);
            utils::ComboRenderPassDescriptor renderPass({}, depthStencilView);
            AssertBeginRenderPassSuccess(&renderPass);
        }

        // Using 2D array texture view that covers the last layer is OK for color
        {
            wgpu::TextureViewDescriptor descriptor = baseDescriptor;
            descriptor.format = kColorFormat;
            descriptor.baseArrayLayer = kArrayLayers - 1;
            descriptor.arrayLayerCount = 1;

            wgpu::TextureView colorTextureView = colorTexture.CreateView(&descriptor);
            utils::ComboRenderPassDescriptor renderPass({colorTextureView});
            AssertBeginRenderPassSuccess(&renderPass);
        }

        // Using 2D array texture view that covers the last layer is OK for depth stencil
        {
            wgpu::TextureViewDescriptor descriptor = baseDescriptor;
            descriptor.format = kDepthStencilFormat;
            descriptor.baseArrayLayer = kArrayLayers - 1;
            descriptor.arrayLayerCount = 1;

            wgpu::TextureView depthStencilView = depthStencilTexture.CreateView(&descriptor);
            utils::ComboRenderPassDescriptor renderPass({}, depthStencilView);
            AssertBeginRenderPassSuccess(&renderPass);
        }
    }

    // Only 2D texture views with mipLevelCount == 1 are allowed to be color attachments
    TEST_F(RenderPassDescriptorValidationTest, TextureViewLevelCountForColorAndDepthStencil) {
        constexpr uint32_t kArrayLayers = 1;
        constexpr uint32_t kSize = 32;
        constexpr wgpu::TextureFormat kColorFormat = wgpu::TextureFormat::RGBA8Unorm;
        constexpr wgpu::TextureFormat kDepthStencilFormat =
            wgpu::TextureFormat::Depth24PlusStencil8;

        constexpr uint32_t kLevelCount = 4;

        wgpu::Texture colorTexture =
            CreateTexture(device, wgpu::TextureDimension::e2D, kColorFormat, kSize, kSize,
                          kArrayLayers, kLevelCount);
        wgpu::Texture depthStencilTexture =
            CreateTexture(device, wgpu::TextureDimension::e2D, kDepthStencilFormat, kSize, kSize,
                          kArrayLayers, kLevelCount);

        wgpu::TextureViewDescriptor baseDescriptor;
        baseDescriptor.dimension = wgpu::TextureViewDimension::e2D;
        baseDescriptor.baseArrayLayer = 0;
        baseDescriptor.arrayLayerCount = kArrayLayers;
        baseDescriptor.baseMipLevel = 0;
        baseDescriptor.mipLevelCount = kLevelCount;

        // Using 2D texture view with mipLevelCount > 1 is not allowed for color
        {
            wgpu::TextureViewDescriptor descriptor = baseDescriptor;
            descriptor.format = kColorFormat;
            descriptor.mipLevelCount = 2;

            wgpu::TextureView colorTextureView = colorTexture.CreateView(&descriptor);
            utils::ComboRenderPassDescriptor renderPass({colorTextureView});
            AssertBeginRenderPassError(&renderPass);
        }

        // Using 2D texture view with mipLevelCount > 1 is not allowed for depth stencil
        {
            wgpu::TextureViewDescriptor descriptor = baseDescriptor;
            descriptor.format = kDepthStencilFormat;
            descriptor.mipLevelCount = 2;

            wgpu::TextureView depthStencilView = depthStencilTexture.CreateView(&descriptor);
            utils::ComboRenderPassDescriptor renderPass({}, depthStencilView);
            AssertBeginRenderPassError(&renderPass);
        }

        // Using 2D texture view that covers the first level of the texture is OK for color
        {
            wgpu::TextureViewDescriptor descriptor = baseDescriptor;
            descriptor.format = kColorFormat;
            descriptor.baseMipLevel = 0;
            descriptor.mipLevelCount = 1;

            wgpu::TextureView colorTextureView = colorTexture.CreateView(&descriptor);
            utils::ComboRenderPassDescriptor renderPass({colorTextureView});
            AssertBeginRenderPassSuccess(&renderPass);
        }

        // Using 2D texture view that covers the first level is OK for depth stencil
        {
            wgpu::TextureViewDescriptor descriptor = baseDescriptor;
            descriptor.format = kDepthStencilFormat;
            descriptor.baseMipLevel = 0;
            descriptor.mipLevelCount = 1;

            wgpu::TextureView depthStencilView = depthStencilTexture.CreateView(&descriptor);
            utils::ComboRenderPassDescriptor renderPass({}, depthStencilView);
            AssertBeginRenderPassSuccess(&renderPass);
        }

        // Using 2D texture view that covers the last level is OK for color
        {
            wgpu::TextureViewDescriptor descriptor = baseDescriptor;
            descriptor.format = kColorFormat;
            descriptor.baseMipLevel = kLevelCount - 1;
            descriptor.mipLevelCount = 1;

            wgpu::TextureView colorTextureView = colorTexture.CreateView(&descriptor);
            utils::ComboRenderPassDescriptor renderPass({colorTextureView});
            AssertBeginRenderPassSuccess(&renderPass);
        }

        // Using 2D texture view that covers the last level is OK for depth stencil
        {
            wgpu::TextureViewDescriptor descriptor = baseDescriptor;
            descriptor.format = kDepthStencilFormat;
            descriptor.baseMipLevel = kLevelCount - 1;
            descriptor.mipLevelCount = 1;

            wgpu::TextureView depthStencilView = depthStencilTexture.CreateView(&descriptor);
            utils::ComboRenderPassDescriptor renderPass({}, depthStencilView);
            AssertBeginRenderPassSuccess(&renderPass);
        }
    }

    // It is not allowed to set resolve target when the color attachment is non-multisampled.
    TEST_F(RenderPassDescriptorValidationTest, NonMultisampledColorWithResolveTarget) {
        static constexpr uint32_t kArrayLayers = 1;
        static constexpr uint32_t kLevelCount = 1;
        static constexpr uint32_t kSize = 32;
        static constexpr uint32_t kSampleCount = 1;
        static constexpr wgpu::TextureFormat kColorFormat = wgpu::TextureFormat::RGBA8Unorm;

        wgpu::Texture colorTexture =
            CreateTexture(device, wgpu::TextureDimension::e2D, kColorFormat, kSize, kSize,
                          kArrayLayers, kLevelCount, kSampleCount);
        wgpu::Texture resolveTargetTexture =
            CreateTexture(device, wgpu::TextureDimension::e2D, kColorFormat, kSize, kSize,
                          kArrayLayers, kLevelCount, kSampleCount);
        wgpu::TextureView colorTextureView = colorTexture.CreateView();
        wgpu::TextureView resolveTargetTextureView = resolveTargetTexture.CreateView();

        utils::ComboRenderPassDescriptor renderPass({colorTextureView});
        renderPass.cColorAttachments[0].resolveTarget = resolveTargetTextureView;
        AssertBeginRenderPassError(&renderPass);
    }

    class MultisampledRenderPassDescriptorValidationTest
        : public RenderPassDescriptorValidationTest {
      public:
        utils::ComboRenderPassDescriptor CreateMultisampledRenderPass() {
            return utils::ComboRenderPassDescriptor({CreateMultisampledColorTextureView()});
        }

        wgpu::TextureView CreateMultisampledColorTextureView() {
            return CreateColorTextureView(kSampleCount);
        }

        wgpu::TextureView CreateNonMultisampledColorTextureView() {
            return CreateColorTextureView(1);
        }

        static constexpr uint32_t kArrayLayers = 1;
        static constexpr uint32_t kLevelCount = 1;
        static constexpr uint32_t kSize = 32;
        static constexpr uint32_t kSampleCount = 4;
        static constexpr wgpu::TextureFormat kColorFormat = wgpu::TextureFormat::RGBA8Unorm;

      private:
        wgpu::TextureView CreateColorTextureView(uint32_t sampleCount) {
            wgpu::Texture colorTexture =
                CreateTexture(device, wgpu::TextureDimension::e2D, kColorFormat, kSize, kSize,
                              kArrayLayers, kLevelCount, sampleCount);

            return colorTexture.CreateView();
        }
    };

    // Tests on the use of multisampled textures as color attachments
    TEST_F(MultisampledRenderPassDescriptorValidationTest, MultisampledColorAttachments) {
        wgpu::TextureView colorTextureView = CreateNonMultisampledColorTextureView();
        wgpu::TextureView resolveTargetTextureView = CreateNonMultisampledColorTextureView();
        wgpu::TextureView multisampledColorTextureView = CreateMultisampledColorTextureView();

        // It is allowed to use a multisampled color attachment without setting resolve target.
        {
            utils::ComboRenderPassDescriptor renderPass = CreateMultisampledRenderPass();
            AssertBeginRenderPassSuccess(&renderPass);
        }

        // It is not allowed to use multiple color attachments with different sample counts.
        {
            utils::ComboRenderPassDescriptor renderPass(
                {multisampledColorTextureView, colorTextureView});
            AssertBeginRenderPassError(&renderPass);
        }
    }

    // It is not allowed to use a multisampled resolve target.
    TEST_F(MultisampledRenderPassDescriptorValidationTest, MultisampledResolveTarget) {
        wgpu::TextureView multisampledResolveTargetView = CreateMultisampledColorTextureView();

        utils::ComboRenderPassDescriptor renderPass = CreateMultisampledRenderPass();
        renderPass.cColorAttachments[0].resolveTarget = multisampledResolveTargetView;
        AssertBeginRenderPassError(&renderPass);
    }

    // It is not allowed to use a resolve target with array layer count > 1.
    TEST_F(MultisampledRenderPassDescriptorValidationTest, ResolveTargetArrayLayerMoreThanOne) {
        constexpr uint32_t kArrayLayers2 = 2;
        wgpu::Texture resolveTexture =
            CreateTexture(device, wgpu::TextureDimension::e2D, kColorFormat, kSize, kSize,
                          kArrayLayers2, kLevelCount);
        wgpu::TextureView resolveTextureView = resolveTexture.CreateView();

        utils::ComboRenderPassDescriptor renderPass = CreateMultisampledRenderPass();
        renderPass.cColorAttachments[0].resolveTarget = resolveTextureView;
        AssertBeginRenderPassError(&renderPass);
    }

    // It is not allowed to use a resolve target with mipmap level count > 1.
    TEST_F(MultisampledRenderPassDescriptorValidationTest, ResolveTargetMipmapLevelMoreThanOne) {
        constexpr uint32_t kLevelCount2 = 2;
        wgpu::Texture resolveTexture =
            CreateTexture(device, wgpu::TextureDimension::e2D, kColorFormat, kSize, kSize,
                          kArrayLayers, kLevelCount2);
        wgpu::TextureView resolveTextureView = resolveTexture.CreateView();

        utils::ComboRenderPassDescriptor renderPass = CreateMultisampledRenderPass();
        renderPass.cColorAttachments[0].resolveTarget = resolveTextureView;
        AssertBeginRenderPassError(&renderPass);
    }

    // It is not allowed to use a resolve target which is created from a texture whose usage does
    // not include wgpu::TextureUsage::OutputAttachment.
    TEST_F(MultisampledRenderPassDescriptorValidationTest, ResolveTargetUsageNoOutputAttachment) {
        constexpr wgpu::TextureUsage kUsage =
            wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::CopySrc;
        wgpu::Texture nonColorUsageResolveTexture =
            CreateTexture(device, wgpu::TextureDimension::e2D, kColorFormat, kSize, kSize,
                          kArrayLayers, kLevelCount, 1, kUsage);
        wgpu::TextureView nonColorUsageResolveTextureView =
            nonColorUsageResolveTexture.CreateView();

        utils::ComboRenderPassDescriptor renderPass = CreateMultisampledRenderPass();
        renderPass.cColorAttachments[0].resolveTarget = nonColorUsageResolveTextureView;
        AssertBeginRenderPassError(&renderPass);
    }

    // It is not allowed to use a resolve target which is in error state.
    TEST_F(MultisampledRenderPassDescriptorValidationTest, ResolveTargetInErrorState) {
        wgpu::Texture resolveTexture =
            CreateTexture(device, wgpu::TextureDimension::e2D, kColorFormat, kSize, kSize,
                          kArrayLayers, kLevelCount);
        wgpu::TextureViewDescriptor errorTextureView;
        errorTextureView.dimension = wgpu::TextureViewDimension::e2D;
        errorTextureView.format = kColorFormat;
        errorTextureView.baseArrayLayer = kArrayLayers + 1;
        ASSERT_DEVICE_ERROR(wgpu::TextureView errorResolveTarget =
                                resolveTexture.CreateView(&errorTextureView));

        utils::ComboRenderPassDescriptor renderPass = CreateMultisampledRenderPass();
        renderPass.cColorAttachments[0].resolveTarget = errorResolveTarget;
        AssertBeginRenderPassError(&renderPass);
    }

    // It is allowed to use a multisampled color attachment and a non-multisampled resolve target.
    TEST_F(MultisampledRenderPassDescriptorValidationTest, MultisampledColorWithResolveTarget) {
        wgpu::TextureView resolveTargetTextureView = CreateNonMultisampledColorTextureView();

        utils::ComboRenderPassDescriptor renderPass = CreateMultisampledRenderPass();
        renderPass.cColorAttachments[0].resolveTarget = resolveTargetTextureView;
        AssertBeginRenderPassSuccess(&renderPass);
    }

    // It is not allowed to use a resolve target in a format different from the color attachment.
    TEST_F(MultisampledRenderPassDescriptorValidationTest, ResolveTargetDifferentFormat) {
        constexpr wgpu::TextureFormat kColorFormat2 = wgpu::TextureFormat::BGRA8Unorm;
        wgpu::Texture resolveTexture =
            CreateTexture(device, wgpu::TextureDimension::e2D, kColorFormat2, kSize, kSize,
                          kArrayLayers, kLevelCount);
        wgpu::TextureView resolveTextureView = resolveTexture.CreateView();

        utils::ComboRenderPassDescriptor renderPass = CreateMultisampledRenderPass();
        renderPass.cColorAttachments[0].resolveTarget = resolveTextureView;
        AssertBeginRenderPassError(&renderPass);
    }

    // Tests on the size of the resolve target.
    TEST_F(MultisampledRenderPassDescriptorValidationTest,
           ColorAttachmentResolveTargetCompatibility) {
        constexpr uint32_t kSize2 = kSize * 2;
        wgpu::Texture resolveTexture =
            CreateTexture(device, wgpu::TextureDimension::e2D, kColorFormat, kSize2, kSize2,
                          kArrayLayers, kLevelCount + 1);

        wgpu::TextureViewDescriptor textureViewDescriptor;
        textureViewDescriptor.nextInChain = nullptr;
        textureViewDescriptor.dimension = wgpu::TextureViewDimension::e2D;
        textureViewDescriptor.format = kColorFormat;
        textureViewDescriptor.mipLevelCount = 1;
        textureViewDescriptor.baseArrayLayer = 0;
        textureViewDescriptor.arrayLayerCount = 1;

        {
            wgpu::TextureViewDescriptor firstMipLevelDescriptor = textureViewDescriptor;
            firstMipLevelDescriptor.baseMipLevel = 0;

            wgpu::TextureView resolveTextureView =
                resolveTexture.CreateView(&firstMipLevelDescriptor);

            utils::ComboRenderPassDescriptor renderPass = CreateMultisampledRenderPass();
            renderPass.cColorAttachments[0].resolveTarget = resolveTextureView;
            AssertBeginRenderPassError(&renderPass);
        }

        {
            wgpu::TextureViewDescriptor secondMipLevelDescriptor = textureViewDescriptor;
            secondMipLevelDescriptor.baseMipLevel = 1;

            wgpu::TextureView resolveTextureView =
                resolveTexture.CreateView(&secondMipLevelDescriptor);

            utils::ComboRenderPassDescriptor renderPass = CreateMultisampledRenderPass();
            renderPass.cColorAttachments[0].resolveTarget = resolveTextureView;
            AssertBeginRenderPassSuccess(&renderPass);
        }
    }

    // Tests on the sample count of depth stencil attachment.
    TEST_F(MultisampledRenderPassDescriptorValidationTest, DepthStencilAttachmentSampleCount) {
        constexpr wgpu::TextureFormat kDepthStencilFormat =
            wgpu::TextureFormat::Depth24PlusStencil8;
        wgpu::Texture multisampledDepthStencilTexture =
            CreateTexture(device, wgpu::TextureDimension::e2D, kDepthStencilFormat, kSize, kSize,
                          kArrayLayers, kLevelCount, kSampleCount);
        wgpu::TextureView multisampledDepthStencilTextureView =
            multisampledDepthStencilTexture.CreateView();

        // It is not allowed to use a depth stencil attachment whose sample count is different from
        // the one of the color attachment.
        {
            wgpu::Texture depthStencilTexture =
                CreateTexture(device, wgpu::TextureDimension::e2D, kDepthStencilFormat, kSize,
                              kSize, kArrayLayers, kLevelCount);
            wgpu::TextureView depthStencilTextureView = depthStencilTexture.CreateView();

            utils::ComboRenderPassDescriptor renderPass({CreateMultisampledColorTextureView()},
                                                        depthStencilTextureView);
            AssertBeginRenderPassError(&renderPass);
        }

        {
            utils::ComboRenderPassDescriptor renderPass({CreateNonMultisampledColorTextureView()},
                                                        multisampledDepthStencilTextureView);
            AssertBeginRenderPassError(&renderPass);
        }

        // It is allowed to use a multisampled depth stencil attachment whose sample count is equal
        // to the one of the color attachment.
        {
            utils::ComboRenderPassDescriptor renderPass({CreateMultisampledColorTextureView()},
                                                        multisampledDepthStencilTextureView);
            AssertBeginRenderPassSuccess(&renderPass);
        }

        // It is allowed to use a multisampled depth stencil attachment while there is no color
        // attachment.
        {
            utils::ComboRenderPassDescriptor renderPass({}, multisampledDepthStencilTextureView);
            AssertBeginRenderPassSuccess(&renderPass);
        }
    }

    // Tests that NaN cannot be accepted as a valid color or depth clear value and INFINITY is valid
    // in both color and depth clear values.
    TEST_F(RenderPassDescriptorValidationTest, UseNaNOrINFINITYAsColorOrDepthClearValue) {
        wgpu::TextureView color = Create2DAttachment(device, 1, 1, wgpu::TextureFormat::RGBA8Unorm);

        // Tests that NaN cannot be used in clearColor.
        {
            utils::ComboRenderPassDescriptor renderPass({color}, nullptr);
            renderPass.cColorAttachments[0].clearColor.r = NAN;
            AssertBeginRenderPassError(&renderPass);
        }

        {
            utils::ComboRenderPassDescriptor renderPass({color}, nullptr);
            renderPass.cColorAttachments[0].clearColor.g = NAN;
            AssertBeginRenderPassError(&renderPass);
        }

        {
            utils::ComboRenderPassDescriptor renderPass({color}, nullptr);
            renderPass.cColorAttachments[0].clearColor.b = NAN;
            AssertBeginRenderPassError(&renderPass);
        }

        {
            utils::ComboRenderPassDescriptor renderPass({color}, nullptr);
            renderPass.cColorAttachments[0].clearColor.a = NAN;
            AssertBeginRenderPassError(&renderPass);
        }

        // Tests that INFINITY can be used in clearColor.
        {
            utils::ComboRenderPassDescriptor renderPass({color}, nullptr);
            renderPass.cColorAttachments[0].clearColor.r = INFINITY;
            AssertBeginRenderPassSuccess(&renderPass);
        }

        {
            utils::ComboRenderPassDescriptor renderPass({color}, nullptr);
            renderPass.cColorAttachments[0].clearColor.g = INFINITY;
            AssertBeginRenderPassSuccess(&renderPass);
        }

        {
            utils::ComboRenderPassDescriptor renderPass({color}, nullptr);
            renderPass.cColorAttachments[0].clearColor.b = INFINITY;
            AssertBeginRenderPassSuccess(&renderPass);
        }

        {
            utils::ComboRenderPassDescriptor renderPass({color}, nullptr);
            renderPass.cColorAttachments[0].clearColor.a = INFINITY;
            AssertBeginRenderPassSuccess(&renderPass);
        }

        // Tests that NaN cannot be used in clearDepth.
        {
            wgpu::TextureView depth =
                Create2DAttachment(device, 1, 1, wgpu::TextureFormat::Depth24Plus);
            utils::ComboRenderPassDescriptor renderPass({color}, depth);
            renderPass.cDepthStencilAttachmentInfo.clearDepth = NAN;
            AssertBeginRenderPassError(&renderPass);
        }

        // Tests that INFINITY can be used in clearDepth.
        {
            wgpu::TextureView depth =
                Create2DAttachment(device, 1, 1, wgpu::TextureFormat::Depth24Plus);
            utils::ComboRenderPassDescriptor renderPass({color}, depth);
            renderPass.cDepthStencilAttachmentInfo.clearDepth = INFINITY;
            AssertBeginRenderPassSuccess(&renderPass);
        }
    }

    TEST_F(RenderPassDescriptorValidationTest, ValidateDepthStencilReadOnly) {
        wgpu::TextureView colorView =
            Create2DAttachment(device, 1, 1, wgpu::TextureFormat::RGBA8Unorm);
        wgpu::TextureView depthStencilView =
            Create2DAttachment(device, 1, 1, wgpu::TextureFormat::Depth24PlusStencil8);
        wgpu::TextureView depthStencilViewNoStencil =
            Create2DAttachment(device, 1, 1, wgpu::TextureFormat::Depth24Plus);

        // Tests that a read-only pass with depthReadOnly set to true succeeds.
        {
            utils::ComboRenderPassDescriptor renderPass({colorView}, depthStencilView);
            renderPass.cDepthStencilAttachmentInfo.depthLoadOp = wgpu::LoadOp::Load;
            renderPass.cDepthStencilAttachmentInfo.depthStoreOp = wgpu::StoreOp::Store;
            renderPass.cDepthStencilAttachmentInfo.depthReadOnly = true;
            renderPass.cDepthStencilAttachmentInfo.stencilLoadOp = wgpu::LoadOp::Load;
            renderPass.cDepthStencilAttachmentInfo.stencilStoreOp = wgpu::StoreOp::Store;
            renderPass.cDepthStencilAttachmentInfo.stencilReadOnly = true;
            AssertBeginRenderPassSuccess(&renderPass);
        }

        // Tests that a pass with mismatched depthReadOnly and stencilReadOnly values passes when
        // there is no stencil component in the format.
        {
            utils::ComboRenderPassDescriptor renderPass({colorView}, depthStencilViewNoStencil);
            renderPass.cDepthStencilAttachmentInfo.depthLoadOp = wgpu::LoadOp::Load;
            renderPass.cDepthStencilAttachmentInfo.depthStoreOp = wgpu::StoreOp::Store;
            renderPass.cDepthStencilAttachmentInfo.depthReadOnly = true;
            renderPass.cDepthStencilAttachmentInfo.stencilLoadOp = wgpu::LoadOp::Load;
            renderPass.cDepthStencilAttachmentInfo.stencilStoreOp = wgpu::StoreOp::Store;
            renderPass.cDepthStencilAttachmentInfo.stencilReadOnly = false;
            AssertBeginRenderPassSuccess(&renderPass);
        }

        // Tests that a pass with mismatched depthReadOnly and stencilReadOnly values fails when
        // both depth and stencil components exist.
        {
            utils::ComboRenderPassDescriptor renderPass({colorView}, depthStencilView);
            renderPass.cDepthStencilAttachmentInfo.depthLoadOp = wgpu::LoadOp::Load;
            renderPass.cDepthStencilAttachmentInfo.depthStoreOp = wgpu::StoreOp::Store;
            renderPass.cDepthStencilAttachmentInfo.depthReadOnly = true;
            renderPass.cDepthStencilAttachmentInfo.stencilLoadOp = wgpu::LoadOp::Load;
            renderPass.cDepthStencilAttachmentInfo.stencilStoreOp = wgpu::StoreOp::Store;
            renderPass.cDepthStencilAttachmentInfo.stencilReadOnly = false;
            AssertBeginRenderPassError(&renderPass);
        }

        // Tests that a pass with loadOp set to clear and readOnly set to true fails.
        {
            utils::ComboRenderPassDescriptor renderPass({colorView}, depthStencilView);
            renderPass.cDepthStencilAttachmentInfo.depthLoadOp = wgpu::LoadOp::Clear;
            renderPass.cDepthStencilAttachmentInfo.depthStoreOp = wgpu::StoreOp::Store;
            renderPass.cDepthStencilAttachmentInfo.depthReadOnly = true;
            renderPass.cDepthStencilAttachmentInfo.stencilLoadOp = wgpu::LoadOp::Clear;
            renderPass.cDepthStencilAttachmentInfo.stencilStoreOp = wgpu::StoreOp::Store;
            renderPass.cDepthStencilAttachmentInfo.stencilReadOnly = true;
            AssertBeginRenderPassError(&renderPass);
        }

        // Tests that a pass with storeOp set to clear and readOnly set to true fails.
        {
            utils::ComboRenderPassDescriptor renderPass({colorView}, depthStencilView);
            renderPass.cDepthStencilAttachmentInfo.depthLoadOp = wgpu::LoadOp::Load;
            renderPass.cDepthStencilAttachmentInfo.depthStoreOp = wgpu::StoreOp::Clear;
            renderPass.cDepthStencilAttachmentInfo.depthReadOnly = true;
            renderPass.cDepthStencilAttachmentInfo.stencilLoadOp = wgpu::LoadOp::Load;
            renderPass.cDepthStencilAttachmentInfo.stencilStoreOp = wgpu::StoreOp::Clear;
            renderPass.cDepthStencilAttachmentInfo.stencilReadOnly = true;
            AssertBeginRenderPassError(&renderPass);
        }
    }

    // TODO(cwallez@chromium.org): Constraints on attachment aliasing?

}  // anonymous namespace
