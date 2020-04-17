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

#include "dawn_native/vulkan/TextureVk.h"

#include "dawn_native/vulkan/AdapterVk.h"
#include "dawn_native/vulkan/DeviceVk.h"
#include "dawn_native/vulkan/FencedDeleter.h"

namespace dawn_native { namespace vulkan {

    namespace {
        // Converts an Dawn texture dimension to a Vulkan image type.
        // Note that in Vulkan dimensionality is only 1D, 2D, 3D. Arrays and cube maps are expressed
        // via the array size and a "cubemap compatible" flag.
        VkImageType VulkanImageType(dawn::TextureDimension dimension) {
            switch (dimension) {
                case dawn::TextureDimension::e2D:
                    return VK_IMAGE_TYPE_2D;
                default:
                    UNREACHABLE();
            }
        }

        // Converts an Dawn texture dimension to a Vulkan image view type.
        // Contrary to image types, image view types include arrayness and cubemapness
        VkImageViewType VulkanImageViewType(dawn::TextureViewDimension dimension) {
            switch (dimension) {
                case dawn::TextureViewDimension::e2D:
                    return VK_IMAGE_VIEW_TYPE_2D;
                case dawn::TextureViewDimension::e2DArray:
                    return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
                case dawn::TextureViewDimension::Cube:
                    return VK_IMAGE_VIEW_TYPE_CUBE;
                case dawn::TextureViewDimension::CubeArray:
                    return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
                default:
                    UNREACHABLE();
            }
        }

        // Computes which vulkan access type could be required for the given Dawn usage.
        VkAccessFlags VulkanAccessFlags(dawn::TextureUsageBit usage, dawn::TextureFormat format) {
            VkAccessFlags flags = 0;

            if (usage & dawn::TextureUsageBit::TransferSrc) {
                flags |= VK_ACCESS_TRANSFER_READ_BIT;
            }
            if (usage & dawn::TextureUsageBit::TransferDst) {
                flags |= VK_ACCESS_TRANSFER_WRITE_BIT;
            }
            if (usage & dawn::TextureUsageBit::Sampled) {
                flags |= VK_ACCESS_SHADER_READ_BIT;
            }
            if (usage & dawn::TextureUsageBit::Storage) {
                flags |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            }
            if (usage & dawn::TextureUsageBit::OutputAttachment) {
                if (TextureFormatHasDepthOrStencil(format)) {
                    flags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                             VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                } else {
                    flags |=
                        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                }
            }
            if (usage & dawn::TextureUsageBit::Present) {
                // There is no access flag for present because the VK_KHR_SWAPCHAIN extension says
                // that vkQueuePresentKHR makes the memory of the image visible to the presentation
                // engine. There's also a note explicitly saying dstAccessMask should be 0. On the
                // other side srcAccessMask can also be 0 because synchronization is required to
                // happen with a semaphore instead.
                flags |= 0;
            }

            return flags;
        }

        // Chooses which Vulkan image layout should be used for the given Dawn usage
        VkImageLayout VulkanImageLayout(dawn::TextureUsageBit usage, dawn::TextureFormat format) {
            if (usage == dawn::TextureUsageBit::None) {
                return VK_IMAGE_LAYOUT_UNDEFINED;
            }

            if (!dawn::HasZeroOrOneBits(usage)) {
                return VK_IMAGE_LAYOUT_GENERAL;
            }

            // Usage has a single bit so we can switch on its value directly.
            switch (usage) {
                case dawn::TextureUsageBit::TransferDst:
                    return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                case dawn::TextureUsageBit::Sampled:
                    return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                // Vulkan texture copy functions require the image to be in _one_  known layout.
                // Depending on whether parts of the texture have been transitioned to only
                // TransferSrc or a combination with something else, the texture could be in a
                // combination of GENERAL and TRANSFER_SRC_OPTIMAL. This would be a problem, so we
                // make TransferSrc use GENERAL.
                case dawn::TextureUsageBit::TransferSrc:
                // Writable storage textures must use general. If we could know the texture is read
                // only we could use SHADER_READ_ONLY_OPTIMAL
                case dawn::TextureUsageBit::Storage:
                    return VK_IMAGE_LAYOUT_GENERAL;
                case dawn::TextureUsageBit::OutputAttachment:
                    if (TextureFormatHasDepthOrStencil(format)) {
                        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                    } else {
                        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                    }
                case dawn::TextureUsageBit::Present:
                    return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                default:
                    UNREACHABLE();
            }
        }

        // Computes which Vulkan pipeline stage can access a texture in the given Dawn usage
        VkPipelineStageFlags VulkanPipelineStage(dawn::TextureUsageBit usage,
                                                 dawn::TextureFormat format) {
            VkPipelineStageFlags flags = 0;

            if (usage == dawn::TextureUsageBit::None) {
                // This only happens when a texture is initially created (and for srcAccessMask) in
                // which case there is no need to wait on anything to stop accessing this texture.
                return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            }
            if (usage & (dawn::TextureUsageBit::TransferSrc | dawn::TextureUsageBit::TransferDst)) {
                flags |= VK_PIPELINE_STAGE_TRANSFER_BIT;
            }
            if (usage & (dawn::TextureUsageBit::Sampled | dawn::TextureUsageBit::Storage)) {
                flags |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            }
            if (usage & dawn::TextureUsageBit::OutputAttachment) {
                if (TextureFormatHasDepthOrStencil(format)) {
                    flags |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                             VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                    // TODO(cwallez@chromium.org): This is missing the stage where the depth and
                    // stencil values are written, but it isn't clear which one it is.
                } else {
                    flags |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                }
            }
            if (usage & dawn::TextureUsageBit::Present) {
                // There is no pipeline stage for present but a pipeline stage is required so we use
                // "bottom of pipe" to block as little as possible and vkQueuePresentKHR will make
                // the memory visible to the presentation engine. The spec explicitly mentions that
                // "bottom of pipe" is ok. On the other direction, synchronization happens with a
                // semaphore so bottom of pipe is ok too (but maybe it could be "top of pipe" to
                // block less?)
                flags |= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            }

            // A zero value isn't a valid pipeline stage mask
            ASSERT(flags != 0);
            return flags;
        }

        // Computes which Vulkan texture aspects are relevant for the given Dawn format
        VkImageAspectFlags VulkanAspectMask(dawn::TextureFormat format) {
            bool isDepth = TextureFormatHasDepth(format);
            bool isStencil = TextureFormatHasStencil(format);

            VkImageAspectFlags flags = 0;
            if (isDepth) {
                flags |= VK_IMAGE_ASPECT_DEPTH_BIT;
            }
            if (isStencil) {
                flags |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }

            if (flags != 0) {
                return flags;
            }
            return VK_IMAGE_ASPECT_COLOR_BIT;
        }

        VkExtent3D VulkanExtent3D(const Extent3D& extent) {
            return {extent.width, extent.height, extent.depth};
        }

        bool IsSampleCountSupported(const dawn_native::vulkan::Device* device,
                                    const VkImageCreateInfo& imageCreateInfo) {
            ASSERT(device);

            VkPhysicalDevice physicalDevice = ToBackend(device->GetAdapter())->GetPhysicalDevice();
            VkImageFormatProperties properties;
            if (device->fn.GetPhysicalDeviceImageFormatProperties(
                    physicalDevice, imageCreateInfo.format, imageCreateInfo.imageType,
                    imageCreateInfo.tiling, imageCreateInfo.usage, imageCreateInfo.flags,
                    &properties) != VK_SUCCESS) {
                UNREACHABLE();
            }

            return properties.sampleCounts & imageCreateInfo.samples;
        }

    }  // namespace

    // Converts Dawn texture format to Vulkan formats.
    VkFormat VulkanImageFormat(dawn::TextureFormat format) {
        switch (format) {
            case dawn::TextureFormat::R8G8B8A8Unorm:
                return VK_FORMAT_R8G8B8A8_UNORM;
            case dawn::TextureFormat::R8G8Unorm:
                return VK_FORMAT_R8G8_UNORM;
            case dawn::TextureFormat::R8Unorm:
                return VK_FORMAT_R8_UNORM;
            case dawn::TextureFormat::R8G8B8A8Uint:
                return VK_FORMAT_R8G8B8A8_UINT;
            case dawn::TextureFormat::R8G8Uint:
                return VK_FORMAT_R8G8_UINT;
            case dawn::TextureFormat::R8Uint:
                return VK_FORMAT_R8_UINT;
            case dawn::TextureFormat::B8G8R8A8Unorm:
                return VK_FORMAT_B8G8R8A8_UNORM;
            case dawn::TextureFormat::D32FloatS8Uint:
                return VK_FORMAT_D32_SFLOAT_S8_UINT;
            default:
                UNREACHABLE();
        }
    }

    // Converts the Dawn usage flags to Vulkan usage flags. Also needs the format to choose
    // between color and depth attachment usages.
    VkImageUsageFlags VulkanImageUsage(dawn::TextureUsageBit usage, dawn::TextureFormat format) {
        VkImageUsageFlags flags = 0;

        if (usage & dawn::TextureUsageBit::TransferSrc) {
            flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }
        if (usage & dawn::TextureUsageBit::TransferDst) {
            flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }
        if (usage & dawn::TextureUsageBit::Sampled) {
            flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
        }
        if (usage & dawn::TextureUsageBit::Storage) {
            flags |= VK_IMAGE_USAGE_STORAGE_BIT;
        }
        if (usage & dawn::TextureUsageBit::OutputAttachment) {
            if (TextureFormatHasDepthOrStencil(format)) {
                flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            } else {
                flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            }
        }

        return flags;
    }

    VkSampleCountFlagBits VulkanSampleCount(uint32_t sampleCount) {
        switch (sampleCount) {
            case 1:
                return VK_SAMPLE_COUNT_1_BIT;
            case 4:
                return VK_SAMPLE_COUNT_4_BIT;
            default:
                UNREACHABLE();
        }
    }

    Texture::Texture(Device* device, const TextureDescriptor* descriptor)
        : TextureBase(device, descriptor, TextureState::OwnedInternal) {
        // Create the Vulkan image "container". We don't need to check that the format supports the
        // combination of sample, usage etc. because validation should have been done in the Dawn
        // frontend already based on the minimum supported formats in the Vulkan spec
        VkImageCreateInfo createInfo;
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.imageType = VulkanImageType(GetDimension());
        createInfo.format = VulkanImageFormat(GetFormat());
        createInfo.extent = VulkanExtent3D(GetSize());
        createInfo.mipLevels = GetNumMipLevels();
        createInfo.arrayLayers = GetArrayLayers();
        createInfo.samples = VulkanSampleCount(GetSampleCount());
        createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        createInfo.usage = VulkanImageUsage(GetUsage(), GetFormat());
        createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
        createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        ASSERT(IsSampleCountSupported(device, createInfo));

        if (GetArrayLayers() >= 6 && GetSize().width == GetSize().height) {
            createInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        }

        // We always set VK_IMAGE_USAGE_TRANSFER_DST_BIT unconditionally beause the Vulkan images
        // that are used in vkCmdClearColorImage() must have been created with this flag, which is
        // also required for the implementation of robust resource initialization.
        createInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        if (device->fn.CreateImage(device->GetVkDevice(), &createInfo, nullptr, &mHandle) !=
            VK_SUCCESS) {
            ASSERT(false);
        }

        // Create the image memory and associate it with the container
        VkMemoryRequirements requirements;
        device->fn.GetImageMemoryRequirements(device->GetVkDevice(), mHandle, &requirements);

        if (!device->GetMemoryAllocator()->Allocate(requirements, false, &mMemoryAllocation)) {
            ASSERT(false);
        }

        if (device->fn.BindImageMemory(device->GetVkDevice(), mHandle,
                                       mMemoryAllocation.GetMemory(),
                                       mMemoryAllocation.GetMemoryOffset()) != VK_SUCCESS) {
            ASSERT(false);
        }
        if (device->IsToggleEnabled(Toggle::NonzeroClearResourcesOnCreationForTesting)) {
            VkImageSubresourceRange range = {};
            range.aspectMask = GetVkAspectMask();
            range.baseMipLevel = 0;
            range.levelCount = GetNumMipLevels();
            range.baseArrayLayer = 0;
            range.layerCount = GetArrayLayers();

            // TODO(natlee@microsoft.com): use correct union member depending on the texture format
            VkClearColorValue clear_color = {{1.0, 1.0, 1.0, 1.0}};

            TransitionUsageNow(ToBackend(GetDevice())->GetPendingCommandBuffer(),
                               dawn::TextureUsageBit::TransferDst);
            ToBackend(GetDevice())
                ->fn.CmdClearColorImage(ToBackend(GetDevice())->GetPendingCommandBuffer(),
                                        GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                        &clear_color, 1, &range);
        }
    }

    // With this constructor, the lifetime of the resource is externally managed.
    Texture::Texture(Device* device, const TextureDescriptor* descriptor, VkImage nativeImage)
        : TextureBase(device, descriptor, TextureState::OwnedExternal), mHandle(nativeImage) {
    }

    Texture::~Texture() {
        DestroyInternal();
    }

    void Texture::DestroyImpl() {
        Device* device = ToBackend(GetDevice());

        // If we own the resource, release it.
        if (mMemoryAllocation.GetMemory() != VK_NULL_HANDLE) {
            // We need to free both the memory allocation and the container. Memory should be
            // freed after the VkImage is destroyed and this is taken care of by the
            // FencedDeleter.
            device->GetMemoryAllocator()->Free(&mMemoryAllocation);

            if (mHandle != VK_NULL_HANDLE) {
                device->GetFencedDeleter()->DeleteWhenUnused(mHandle);
            }
        }
        mHandle = VK_NULL_HANDLE;
    }

    VkImage Texture::GetHandle() const {
        return mHandle;
    }

    VkImageAspectFlags Texture::GetVkAspectMask() const {
        return VulkanAspectMask(GetFormat());
    }

    void Texture::TransitionUsageNow(VkCommandBuffer commands, dawn::TextureUsageBit usage) {
        // Avoid encoding barriers when it isn't needed.
        bool lastReadOnly = (mLastUsage & kReadOnlyTextureUsages) == mLastUsage;
        if (lastReadOnly && mLastUsage == usage) {
            return;
        }

        dawn::TextureFormat format = GetFormat();

        VkPipelineStageFlags srcStages = VulkanPipelineStage(mLastUsage, format);
        VkPipelineStageFlags dstStages = VulkanPipelineStage(usage, format);

        VkImageMemoryBarrier barrier;
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.pNext = nullptr;
        barrier.srcAccessMask = VulkanAccessFlags(mLastUsage, format);
        barrier.dstAccessMask = VulkanAccessFlags(usage, format);
        barrier.oldLayout = VulkanImageLayout(mLastUsage, format);
        barrier.newLayout = VulkanImageLayout(usage, format);
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = mHandle;
        // This transitions the whole resource but assumes it is a 2D texture
        ASSERT(GetDimension() == dawn::TextureDimension::e2D);
        barrier.subresourceRange.aspectMask = VulkanAspectMask(format);
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = GetNumMipLevels();
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = GetArrayLayers();

        ToBackend(GetDevice())
            ->fn.CmdPipelineBarrier(commands, srcStages, dstStages, 0, 0, nullptr, 0, nullptr, 1,
                                    &barrier);

        mLastUsage = usage;
    }

    void Texture::ClearTexture(VkCommandBuffer commands,
                               uint32_t baseMipLevel,
                               uint32_t levelCount,
                               uint32_t baseArrayLayer,
                               uint32_t layerCount) {
        if (GetDevice()->IsToggleEnabled(Toggle::LazyClearResourceOnFirstUse)) {
            VkImageSubresourceRange range = {};
            range.aspectMask = GetVkAspectMask();
            range.baseMipLevel = baseMipLevel;
            range.levelCount = levelCount;
            range.baseArrayLayer = baseArrayLayer;
            range.layerCount = layerCount;

            TransitionUsageNow(commands, dawn::TextureUsageBit::TransferDst);
            if (TextureFormatHasDepthOrStencil(GetFormat())) {
                VkClearDepthStencilValue clear_color[1];
                clear_color[0].depth = 0.0f;
                clear_color[0].stencil = 0u;
                ToBackend(GetDevice())
                    ->fn.CmdClearDepthStencilImage(commands, GetHandle(),
                                                   VulkanImageLayout(GetUsage(), GetFormat()),
                                                   clear_color, 1, &range);
            } else {
                VkClearColorValue clear_color[1];
                clear_color[0].float32[0] = 0.0f;
                clear_color[0].float32[1] = 0.0f;
                clear_color[0].float32[2] = 0.0f;
                clear_color[0].float32[3] = 0.0f;
                ToBackend(GetDevice())
                    ->fn.CmdClearColorImage(commands, GetHandle(),
                                            VulkanImageLayout(GetUsage(), GetFormat()), clear_color,
                                            1, &range);
            }
            SetIsSubresourceContentInitialized(baseMipLevel, levelCount, baseArrayLayer,
                                               layerCount);
        }
    }

    void Texture::EnsureSubresourceContentInitialized(VkCommandBuffer commands,
                                                      uint32_t baseMipLevel,
                                                      uint32_t levelCount,
                                                      uint32_t baseArrayLayer,
                                                      uint32_t layerCount) {
        if (!IsSubresourceContentInitialized(baseMipLevel, levelCount, baseArrayLayer,
                                             layerCount)) {
            // If subresource has not been initialized, clear it to black as it could contain dirty
            // bits from recycled memory
            ClearTexture(commands, baseMipLevel, levelCount, baseArrayLayer, layerCount);
        }
    }

    // TODO(jiawei.shao@intel.com): create texture view by TextureViewDescriptor
    TextureView::TextureView(TextureBase* texture, const TextureViewDescriptor* descriptor)
        : TextureViewBase(texture, descriptor) {
        Device* device = ToBackend(texture->GetDevice());

        VkImageViewCreateInfo createInfo;
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.image = ToBackend(GetTexture())->GetHandle();
        createInfo.viewType = VulkanImageViewType(descriptor->dimension);
        createInfo.format = VulkanImageFormat(descriptor->format);
        createInfo.components = VkComponentMapping{VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G,
                                                   VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
        createInfo.subresourceRange.aspectMask = VulkanAspectMask(descriptor->format);
        createInfo.subresourceRange.baseMipLevel = descriptor->baseMipLevel;
        createInfo.subresourceRange.levelCount = descriptor->mipLevelCount;
        createInfo.subresourceRange.baseArrayLayer = descriptor->baseArrayLayer;
        createInfo.subresourceRange.layerCount = descriptor->arrayLayerCount;

        if (device->fn.CreateImageView(device->GetVkDevice(), &createInfo, nullptr, &mHandle) !=
            VK_SUCCESS) {
            ASSERT(false);
        }
    }

    TextureView::~TextureView() {
        Device* device = ToBackend(GetTexture()->GetDevice());

        if (mHandle != VK_NULL_HANDLE) {
            device->GetFencedDeleter()->DeleteWhenUnused(mHandle);
            mHandle = VK_NULL_HANDLE;
        }
    }

    VkImageView TextureView::GetHandle() const {
        return mHandle;
    }

}}  // namespace dawn_native::vulkan
