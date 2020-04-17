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

#include "utils/DawnHelpers.h"

namespace {

class RenderPassDescriptorValidationTest : public ValidationTest {
  public:
    void AssertBeginRenderPassSuccess(const dawn::RenderPassDescriptor* descriptor) {
        dawn::CommandEncoder commandEncoder = TestBeginRenderPass(descriptor);
        commandEncoder.Finish();
    }
    void AssertBeginRenderPassError(const dawn::RenderPassDescriptor* descriptor) {
        dawn::CommandEncoder commandEncoder = TestBeginRenderPass(descriptor);
        ASSERT_DEVICE_ERROR(commandEncoder.Finish());
    }

  private:
    dawn::CommandEncoder TestBeginRenderPass(const dawn::RenderPassDescriptor* descriptor) {
        dawn::CommandEncoder commandEncoder = device.CreateCommandEncoder();
        dawn::RenderPassEncoder renderPassEncoder = commandEncoder.BeginRenderPass(descriptor);
        renderPassEncoder.EndPass();
        return commandEncoder;
    }
};

dawn::Texture CreateTexture(dawn::Device& device,
                            dawn::TextureDimension dimension,
                            dawn::TextureFormat format,
                            uint32_t width,
                            uint32_t height,
                            uint32_t arrayLayerCount,
                            uint32_t mipLevelCount,
                            uint32_t sampleCount = 1,
                            dawn::TextureUsageBit usage = dawn::TextureUsageBit::OutputAttachment) {
    dawn::TextureDescriptor descriptor;
    descriptor.dimension = dimension;
    descriptor.size.width = width;
    descriptor.size.height = height;
    descriptor.size.depth = 1;
    descriptor.arrayLayerCount = arrayLayerCount;
    descriptor.sampleCount = sampleCount;
    descriptor.format = format;
    descriptor.mipLevelCount = mipLevelCount;
    descriptor.usage = usage;

    return device.CreateTexture(&descriptor);
}

dawn::TextureView Create2DAttachment(dawn::Device& device,
                                     uint32_t width,
                                     uint32_t height,
                                     dawn::TextureFormat format) {
    dawn::Texture texture = CreateTexture(
        device, dawn::TextureDimension::e2D, format, width, height, 1, 1);
    return texture.CreateDefaultView();
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
        dawn::TextureView color = Create2DAttachment(device, 1, 1, dawn::TextureFormat::R8G8B8A8Unorm);
        utils::ComboRenderPassDescriptor renderPass({color});

        AssertBeginRenderPassSuccess(&renderPass);
    }
    // One depth-stencil attachment
    {
        dawn::TextureView depthStencil = Create2DAttachment(device, 1, 1, dawn::TextureFormat::D32FloatS8Uint);
        utils::ComboRenderPassDescriptor renderPass({}, depthStencil);

        AssertBeginRenderPassSuccess(&renderPass);
    }
}

// Test OOB color attachment indices are handled
TEST_F(RenderPassDescriptorValidationTest, ColorAttachmentOutOfBounds) {
    dawn::TextureView color1 = Create2DAttachment(device, 1, 1,
                                                  dawn::TextureFormat::R8G8B8A8Unorm);
    dawn::TextureView color2 = Create2DAttachment(device, 1, 1,
                                                  dawn::TextureFormat::R8G8B8A8Unorm);
    dawn::TextureView color3 = Create2DAttachment(device, 1, 1,
                                                  dawn::TextureFormat::R8G8B8A8Unorm);
    dawn::TextureView color4 = Create2DAttachment(device, 1, 1,
                                                  dawn::TextureFormat::R8G8B8A8Unorm);
    // For setting the color attachment, control case
    {
        utils::ComboRenderPassDescriptor renderPass({color1, color2, color3, color4});
        AssertBeginRenderPassSuccess(&renderPass);
    }
    // For setting the color attachment, OOB
    {
        // We cannot use utils::ComboRenderPassDescriptor here because it only supports at most
        // kMaxColorAttachments(4) color attachments.
        dawn::RenderPassColorAttachmentDescriptor colorAttachment1;
        colorAttachment1.attachment = color1;
        colorAttachment1.resolveTarget = nullptr;
        colorAttachment1.clearColor = {0.0f, 0.0f, 0.0f, 0.0f};
        colorAttachment1.loadOp = dawn::LoadOp::Clear;
        colorAttachment1.storeOp = dawn::StoreOp::Store;

        dawn::RenderPassColorAttachmentDescriptor colorAttachment2 = colorAttachment1;
        dawn::RenderPassColorAttachmentDescriptor colorAttachment3 = colorAttachment1;
        dawn::RenderPassColorAttachmentDescriptor colorAttachment4 = colorAttachment1;
        colorAttachment2.attachment = color2;
        colorAttachment3.attachment = color3;
        colorAttachment4.attachment = color4;

        dawn::TextureView color5 = Create2DAttachment(device, 1, 1,
                                                      dawn::TextureFormat::R8G8B8A8Unorm);
        dawn::RenderPassColorAttachmentDescriptor colorAttachment5 = colorAttachment1;
        colorAttachment5.attachment = color5;

        dawn::RenderPassColorAttachmentDescriptor* colorAttachments[] = {&colorAttachment1,
                                                                         &colorAttachment2,
                                                                         &colorAttachment3,
                                                                         &colorAttachment4,
                                                                         &colorAttachment5};
        dawn::RenderPassDescriptor renderPass;
        renderPass.colorAttachmentCount = kMaxColorAttachments + 1;
        renderPass.colorAttachments = colorAttachments;
        renderPass.depthStencilAttachment = nullptr;
        AssertBeginRenderPassError(&renderPass);
    }
}

// Attachments must have the same size
TEST_F(RenderPassDescriptorValidationTest, SizeMustMatch) {
    dawn::TextureView color1x1A = Create2DAttachment(device, 1, 1, dawn::TextureFormat::R8G8B8A8Unorm);
    dawn::TextureView color1x1B = Create2DAttachment(device, 1, 1, dawn::TextureFormat::R8G8B8A8Unorm);
    dawn::TextureView color2x2 = Create2DAttachment(device, 2, 2, dawn::TextureFormat::R8G8B8A8Unorm);

    dawn::TextureView depthStencil1x1 = Create2DAttachment(device, 1, 1, dawn::TextureFormat::D32FloatS8Uint);
    dawn::TextureView depthStencil2x2 = Create2DAttachment(device, 2, 2, dawn::TextureFormat::D32FloatS8Uint);

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
    dawn::TextureView color = Create2DAttachment(device, 1, 1, dawn::TextureFormat::R8G8B8A8Unorm);
    dawn::TextureView depthStencil = Create2DAttachment(device, 1, 1, dawn::TextureFormat::D32FloatS8Uint);

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

// Currently only texture views with arrayLayerCount == 1 are allowed to be color and depth stencil
// attachments
TEST_F(RenderPassDescriptorValidationTest, TextureViewLayerCountForColorAndDepthStencil) {
    constexpr uint32_t kLevelCount = 1;
    constexpr uint32_t kSize = 32;
    constexpr dawn::TextureFormat kColorFormat = dawn::TextureFormat::R8G8B8A8Unorm;
    constexpr dawn::TextureFormat kDepthStencilFormat = dawn::TextureFormat::D32FloatS8Uint;

    constexpr uint32_t kArrayLayers = 10;

    dawn::Texture colorTexture = CreateTexture(
        device, dawn::TextureDimension::e2D, kColorFormat, kSize, kSize, kArrayLayers, kLevelCount);
    dawn::Texture depthStencilTexture = CreateTexture(
        device, dawn::TextureDimension::e2D, kDepthStencilFormat, kSize, kSize, kArrayLayers,
        kLevelCount);

    dawn::TextureViewDescriptor baseDescriptor;
    baseDescriptor.dimension = dawn::TextureViewDimension::e2DArray;
    baseDescriptor.baseArrayLayer = 0;
    baseDescriptor.arrayLayerCount = kArrayLayers;
    baseDescriptor.baseMipLevel = 0;
    baseDescriptor.mipLevelCount = kLevelCount;

    // Using 2D array texture view with arrayLayerCount > 1 is not allowed for color
    {
        dawn::TextureViewDescriptor descriptor = baseDescriptor;
        descriptor.format = kColorFormat;
        descriptor.arrayLayerCount = 5;

        dawn::TextureView colorTextureView = colorTexture.CreateView(&descriptor);
        utils::ComboRenderPassDescriptor renderPass({colorTextureView});
        AssertBeginRenderPassError(&renderPass);
    }

    // Using 2D array texture view with arrayLayerCount > 1 is not allowed for depth stencil
    {
        dawn::TextureViewDescriptor descriptor = baseDescriptor;
        descriptor.format = kDepthStencilFormat;
        descriptor.arrayLayerCount = 5;

        dawn::TextureView depthStencilView = depthStencilTexture.CreateView(&descriptor);
        utils::ComboRenderPassDescriptor renderPass({}, depthStencilView);
        AssertBeginRenderPassError(&renderPass);
    }

    // Using 2D array texture view that covers the first layer of the texture is OK for color
    {
        dawn::TextureViewDescriptor descriptor = baseDescriptor;
        descriptor.format = kColorFormat;
        descriptor.baseArrayLayer = 0;
        descriptor.arrayLayerCount = 1;

        dawn::TextureView colorTextureView = colorTexture.CreateView(&descriptor);
        utils::ComboRenderPassDescriptor renderPass({colorTextureView});
        AssertBeginRenderPassSuccess(&renderPass);
    }

    // Using 2D array texture view that covers the first layer is OK for depth stencil
    {
        dawn::TextureViewDescriptor descriptor = baseDescriptor;
        descriptor.format = kDepthStencilFormat;
        descriptor.baseArrayLayer = 0;
        descriptor.arrayLayerCount = 1;

        dawn::TextureView depthStencilView = depthStencilTexture.CreateView(&descriptor);
        utils::ComboRenderPassDescriptor renderPass({}, depthStencilView);
        AssertBeginRenderPassSuccess(&renderPass);
    }

    // Using 2D array texture view that covers the last layer is OK for color
    {
        dawn::TextureViewDescriptor descriptor = baseDescriptor;
        descriptor.format = kColorFormat;
        descriptor.baseArrayLayer = kArrayLayers - 1;
        descriptor.arrayLayerCount = 1;

        dawn::TextureView colorTextureView = colorTexture.CreateView(&descriptor);
        utils::ComboRenderPassDescriptor renderPass({colorTextureView});
        AssertBeginRenderPassSuccess(&renderPass);
    }

    // Using 2D array texture view that covers the last layer is OK for depth stencil
    {
        dawn::TextureViewDescriptor descriptor = baseDescriptor;
        descriptor.format = kDepthStencilFormat;
        descriptor.baseArrayLayer = kArrayLayers - 1;
        descriptor.arrayLayerCount = 1;

        dawn::TextureView depthStencilView = depthStencilTexture.CreateView(&descriptor);
        utils::ComboRenderPassDescriptor renderPass({}, depthStencilView);
        AssertBeginRenderPassSuccess(&renderPass);
    }
}

// Only 2D texture views with mipLevelCount == 1 are allowed to be color attachments
TEST_F(RenderPassDescriptorValidationTest, TextureViewLevelCountForColorAndDepthStencil) {
    constexpr uint32_t kArrayLayers = 1;
    constexpr uint32_t kSize = 32;
    constexpr dawn::TextureFormat kColorFormat = dawn::TextureFormat::R8G8B8A8Unorm;
    constexpr dawn::TextureFormat kDepthStencilFormat = dawn::TextureFormat::D32FloatS8Uint;

    constexpr uint32_t kLevelCount = 4;

    dawn::Texture colorTexture = CreateTexture(
        device, dawn::TextureDimension::e2D, kColorFormat, kSize, kSize, kArrayLayers, kLevelCount);
    dawn::Texture depthStencilTexture = CreateTexture(
        device, dawn::TextureDimension::e2D, kDepthStencilFormat, kSize, kSize, kArrayLayers,
        kLevelCount);

    dawn::TextureViewDescriptor baseDescriptor;
    baseDescriptor.dimension = dawn::TextureViewDimension::e2D;
    baseDescriptor.baseArrayLayer = 0;
    baseDescriptor.arrayLayerCount = kArrayLayers;
    baseDescriptor.baseMipLevel = 0;
    baseDescriptor.mipLevelCount = kLevelCount;

    // Using 2D texture view with mipLevelCount > 1 is not allowed for color
    {
        dawn::TextureViewDescriptor descriptor = baseDescriptor;
        descriptor.format = kColorFormat;
        descriptor.mipLevelCount = 2;

        dawn::TextureView colorTextureView = colorTexture.CreateView(&descriptor);
        utils::ComboRenderPassDescriptor renderPass({colorTextureView});
        AssertBeginRenderPassError(&renderPass);
    }

    // Using 2D texture view with mipLevelCount > 1 is not allowed for depth stencil
    {
        dawn::TextureViewDescriptor descriptor = baseDescriptor;
        descriptor.format = kDepthStencilFormat;
        descriptor.mipLevelCount = 2;

        dawn::TextureView depthStencilView = depthStencilTexture.CreateView(&descriptor);
        utils::ComboRenderPassDescriptor renderPass({}, depthStencilView);
        AssertBeginRenderPassError(&renderPass);
    }

    // Using 2D texture view that covers the first level of the texture is OK for color
    {
        dawn::TextureViewDescriptor descriptor = baseDescriptor;
        descriptor.format = kColorFormat;
        descriptor.baseMipLevel = 0;
        descriptor.mipLevelCount = 1;

        dawn::TextureView colorTextureView = colorTexture.CreateView(&descriptor);
        utils::ComboRenderPassDescriptor renderPass({colorTextureView});
        AssertBeginRenderPassSuccess(&renderPass);
    }

    // Using 2D texture view that covers the first level is OK for depth stencil
    {
        dawn::TextureViewDescriptor descriptor = baseDescriptor;
        descriptor.format = kDepthStencilFormat;
        descriptor.baseMipLevel = 0;
        descriptor.mipLevelCount = 1;

        dawn::TextureView depthStencilView = depthStencilTexture.CreateView(&descriptor);
        utils::ComboRenderPassDescriptor renderPass({}, depthStencilView);
        AssertBeginRenderPassSuccess(&renderPass);
    }

    // Using 2D texture view that covers the last level is OK for color
    {
        dawn::TextureViewDescriptor descriptor = baseDescriptor;
        descriptor.format = kColorFormat;
        descriptor.baseMipLevel = kLevelCount - 1;
        descriptor.mipLevelCount = 1;

        dawn::TextureView colorTextureView = colorTexture.CreateView(&descriptor);
        utils::ComboRenderPassDescriptor renderPass({colorTextureView});
        AssertBeginRenderPassSuccess(&renderPass);
    }

    // Using 2D texture view that covers the last level is OK for depth stencil
    {
        dawn::TextureViewDescriptor descriptor = baseDescriptor;
        descriptor.format = kDepthStencilFormat;
        descriptor.baseMipLevel = kLevelCount - 1;
        descriptor.mipLevelCount = 1;

        dawn::TextureView depthStencilView = depthStencilTexture.CreateView(&descriptor);
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
    static constexpr dawn::TextureFormat kColorFormat = dawn::TextureFormat::R8G8B8A8Unorm;

    dawn::Texture colorTexture = CreateTexture(
        device, dawn::TextureDimension::e2D, kColorFormat, kSize, kSize, kArrayLayers,
        kLevelCount, kSampleCount);
    dawn::Texture resolveTargetTexture = CreateTexture(
        device, dawn::TextureDimension::e2D, kColorFormat, kSize, kSize, kArrayLayers,
        kLevelCount, kSampleCount);
    dawn::TextureView colorTextureView = colorTexture.CreateDefaultView();
    dawn::TextureView resolveTargetTextureView = resolveTargetTexture.CreateDefaultView();

    utils::ComboRenderPassDescriptor renderPass({colorTextureView});
    renderPass.cColorAttachmentsInfoPtr[0]->resolveTarget = resolveTargetTextureView;
    AssertBeginRenderPassError(&renderPass);
}

class MultisampledRenderPassDescriptorValidationTest : public RenderPassDescriptorValidationTest {
  public:
    utils::ComboRenderPassDescriptor CreateMultisampledRenderPass() {
        return utils::ComboRenderPassDescriptor({CreateMultisampledColorTextureView()});
    }

    dawn::TextureView CreateMultisampledColorTextureView() {
        return CreateColorTextureView(kSampleCount);
    }

    dawn::TextureView CreateNonMultisampledColorTextureView() {
        return CreateColorTextureView(1);
    }

    static constexpr uint32_t kArrayLayers = 1;
    static constexpr uint32_t kLevelCount = 1;
    static constexpr uint32_t kSize = 32;
    static constexpr uint32_t kSampleCount = 4;
    static constexpr dawn::TextureFormat kColorFormat = dawn::TextureFormat::R8G8B8A8Unorm;

  private:
    dawn::TextureView CreateColorTextureView(uint32_t sampleCount) {
        dawn::Texture colorTexture = CreateTexture(
            device, dawn::TextureDimension::e2D, kColorFormat, kSize, kSize, kArrayLayers,
            kLevelCount, sampleCount);

        return colorTexture.CreateDefaultView();
    }
};

// Tests on the use of multisampled textures as color attachments
TEST_F(MultisampledRenderPassDescriptorValidationTest, MultisampledColorAttachments) {
    dawn::TextureView colorTextureView = CreateNonMultisampledColorTextureView();
    dawn::TextureView resolveTargetTextureView = CreateNonMultisampledColorTextureView();
    dawn::TextureView multisampledColorTextureView = CreateMultisampledColorTextureView();

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
    dawn::TextureView multisampledResolveTargetView = CreateMultisampledColorTextureView();

    utils::ComboRenderPassDescriptor renderPass = CreateMultisampledRenderPass();
    renderPass.cColorAttachmentsInfoPtr[0]->resolveTarget = multisampledResolveTargetView;
    AssertBeginRenderPassError(&renderPass);
}

// It is not allowed to use a resolve target with array layer count > 1.
TEST_F(MultisampledRenderPassDescriptorValidationTest, ResolveTargetArrayLayerMoreThanOne) {
    constexpr uint32_t kArrayLayers2 = 2;
    dawn::Texture resolveTexture = CreateTexture(
        device, dawn::TextureDimension::e2D, kColorFormat, kSize, kSize, kArrayLayers2,
        kLevelCount);
    dawn::TextureView resolveTextureView = resolveTexture.CreateDefaultView();

    utils::ComboRenderPassDescriptor renderPass = CreateMultisampledRenderPass();
    renderPass.cColorAttachmentsInfoPtr[0]->resolveTarget = resolveTextureView;
    AssertBeginRenderPassError(&renderPass);
}

// It is not allowed to use a resolve target with mipmap level count > 1.
TEST_F(MultisampledRenderPassDescriptorValidationTest, ResolveTargetMipmapLevelMoreThanOne) {
    constexpr uint32_t kLevelCount2 = 2;
    dawn::Texture resolveTexture = CreateTexture(
        device, dawn::TextureDimension::e2D, kColorFormat, kSize, kSize, kArrayLayers,
        kLevelCount2);
    dawn::TextureView resolveTextureView = resolveTexture.CreateDefaultView();

    utils::ComboRenderPassDescriptor renderPass = CreateMultisampledRenderPass();
    renderPass.cColorAttachmentsInfoPtr[0]->resolveTarget = resolveTextureView;
    AssertBeginRenderPassError(&renderPass);
}

// It is not allowed to use a resolve target which is created from a texture whose usage does not
// include dawn::TextureUsageBit::OutputAttachment.
TEST_F(MultisampledRenderPassDescriptorValidationTest, ResolveTargetUsageNoOutputAttachment) {
    constexpr dawn::TextureUsageBit kUsage =
        dawn::TextureUsageBit::TransferDst | dawn::TextureUsageBit::TransferSrc;
    dawn::Texture nonColorUsageResolveTexture = CreateTexture(
        device, dawn::TextureDimension::e2D, kColorFormat, kSize, kSize, kArrayLayers,
        kLevelCount, 1, kUsage);
    dawn::TextureView nonColorUsageResolveTextureView =
        nonColorUsageResolveTexture.CreateDefaultView();

    utils::ComboRenderPassDescriptor renderPass = CreateMultisampledRenderPass();
    renderPass.cColorAttachmentsInfoPtr[0]->resolveTarget = nonColorUsageResolveTextureView;
    AssertBeginRenderPassError(&renderPass);
}

// It is not allowed to use a resolve target which is in error state.
TEST_F(MultisampledRenderPassDescriptorValidationTest, ResolveTargetInErrorState) {
    dawn::Texture resolveTexture = CreateTexture(
        device, dawn::TextureDimension::e2D, kColorFormat, kSize, kSize, kArrayLayers,
        kLevelCount);
    dawn::TextureViewDescriptor errorTextureView;
    errorTextureView.dimension = dawn::TextureViewDimension::e2D;
    errorTextureView.format = kColorFormat;
    errorTextureView.baseArrayLayer = kArrayLayers + 1;
    ASSERT_DEVICE_ERROR(
        dawn::TextureView errorResolveTarget =
        resolveTexture.CreateView(&errorTextureView));

    utils::ComboRenderPassDescriptor renderPass = CreateMultisampledRenderPass();
    renderPass.cColorAttachmentsInfoPtr[0]->resolveTarget = errorResolveTarget;
    AssertBeginRenderPassError(&renderPass);
}

// It is allowed to use a multisampled color attachment and a non-multisampled resolve target.
TEST_F(MultisampledRenderPassDescriptorValidationTest, MultisampledColorWithResolveTarget) {
    dawn::TextureView resolveTargetTextureView = CreateNonMultisampledColorTextureView();

    utils::ComboRenderPassDescriptor renderPass = CreateMultisampledRenderPass();
    renderPass.cColorAttachmentsInfoPtr[0]->resolveTarget = resolveTargetTextureView;
    AssertBeginRenderPassSuccess(&renderPass);
}

// It is not allowed to use a resolve target in a format different from the color attachment.
TEST_F(MultisampledRenderPassDescriptorValidationTest, ResolveTargetDifferentFormat) {
    constexpr dawn::TextureFormat kColorFormat2 = dawn::TextureFormat::B8G8R8A8Unorm;
    dawn::Texture resolveTexture = CreateTexture(
        device, dawn::TextureDimension::e2D, kColorFormat2, kSize, kSize, kArrayLayers,
        kLevelCount);
    dawn::TextureView resolveTextureView = resolveTexture.CreateDefaultView();

    utils::ComboRenderPassDescriptor renderPass = CreateMultisampledRenderPass();
    renderPass.cColorAttachmentsInfoPtr[0]->resolveTarget = resolveTextureView;
    AssertBeginRenderPassError(&renderPass);
}

// Tests on the size of the resolve target.
TEST_F(MultisampledRenderPassDescriptorValidationTest, ColorAttachmentResolveTargetCompatibility) {
    constexpr uint32_t kSize2 = kSize * 2;
    dawn::Texture resolveTexture = CreateTexture(
        device, dawn::TextureDimension::e2D, kColorFormat, kSize2, kSize2, kArrayLayers,
        kLevelCount + 1);

    dawn::TextureViewDescriptor textureViewDescriptor;
    textureViewDescriptor.nextInChain = nullptr;
    textureViewDescriptor.dimension = dawn::TextureViewDimension::e2D;
    textureViewDescriptor.format = kColorFormat;
    textureViewDescriptor.mipLevelCount = 1;
    textureViewDescriptor.baseArrayLayer = 0;
    textureViewDescriptor.arrayLayerCount = 1;

    {
        dawn::TextureViewDescriptor firstMipLevelDescriptor = textureViewDescriptor;
        firstMipLevelDescriptor.baseMipLevel = 0;

        dawn::TextureView resolveTextureView =
            resolveTexture.CreateView(&firstMipLevelDescriptor);

        utils::ComboRenderPassDescriptor renderPass = CreateMultisampledRenderPass();
        renderPass.cColorAttachmentsInfoPtr[0]->resolveTarget = resolveTextureView;
        AssertBeginRenderPassError(&renderPass);
    }

    {
        dawn::TextureViewDescriptor secondMipLevelDescriptor = textureViewDescriptor;
        secondMipLevelDescriptor.baseMipLevel = 1;

        dawn::TextureView resolveTextureView =
            resolveTexture.CreateView(&secondMipLevelDescriptor);

        utils::ComboRenderPassDescriptor renderPass = CreateMultisampledRenderPass();
        renderPass.cColorAttachmentsInfoPtr[0]->resolveTarget = resolveTextureView;
        AssertBeginRenderPassSuccess(&renderPass);
    }
}

// Tests on the sample count of depth stencil attachment.
TEST_F(MultisampledRenderPassDescriptorValidationTest, DepthStencilAttachmentSampleCount) {
    constexpr dawn::TextureFormat kDepthStencilFormat = dawn::TextureFormat::D32FloatS8Uint;
    dawn::Texture multisampledDepthStencilTexture = CreateTexture(
        device, dawn::TextureDimension::e2D, kDepthStencilFormat, kSize, kSize, kArrayLayers,
        kLevelCount, kSampleCount);
    dawn::TextureView multisampledDepthStencilTextureView =
        multisampledDepthStencilTexture.CreateDefaultView();

    // It is not allowed to use a depth stencil attachment whose sample count is different from the
    // one of the color attachment.
    {
        dawn::Texture depthStencilTexture = CreateTexture(
            device, dawn::TextureDimension::e2D, kDepthStencilFormat, kSize, kSize, kArrayLayers,
            kLevelCount);
        dawn::TextureView depthStencilTextureView = depthStencilTexture.CreateDefaultView();

        utils::ComboRenderPassDescriptor renderPass(
            {CreateMultisampledColorTextureView()}, depthStencilTextureView);
        AssertBeginRenderPassError(&renderPass);
    }

    {
        utils::ComboRenderPassDescriptor renderPass(
            {CreateNonMultisampledColorTextureView()}, multisampledDepthStencilTextureView);
        AssertBeginRenderPassError(&renderPass);
    }

    // It is allowed to use a multisampled depth stencil attachment whose sample count is equal to
    // the one of the color attachment.
    {
        utils::ComboRenderPassDescriptor renderPass(
            {CreateMultisampledColorTextureView()}, multisampledDepthStencilTextureView);
        AssertBeginRenderPassSuccess(&renderPass);
    }

    // It is allowed to use a multisampled depth stencil attachment while there is no color
    // attachment.
    {
        utils::ComboRenderPassDescriptor renderPass({}, multisampledDepthStencilTextureView);
        AssertBeginRenderPassSuccess(&renderPass);
    }
}

// TODO(cwallez@chromium.org): Constraints on attachment aliasing?

} // anonymous namespace
