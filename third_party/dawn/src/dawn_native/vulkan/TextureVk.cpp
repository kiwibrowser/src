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

#include "common/Assert.h"
#include "common/Math.h"
#include "dawn_native/DynamicUploader.h"
#include "dawn_native/Error.h"
#include "dawn_native/VulkanBackend.h"
#include "dawn_native/vulkan/AdapterVk.h"
#include "dawn_native/vulkan/BufferVk.h"
#include "dawn_native/vulkan/CommandRecordingContext.h"
#include "dawn_native/vulkan/DeviceVk.h"
#include "dawn_native/vulkan/FencedDeleter.h"
#include "dawn_native/vulkan/ResourceHeapVk.h"
#include "dawn_native/vulkan/StagingBufferVk.h"
#include "dawn_native/vulkan/UtilsVulkan.h"
#include "dawn_native/vulkan/VulkanError.h"

namespace dawn_native { namespace vulkan {

    namespace {
        // Converts an Dawn texture dimension to a Vulkan image view type.
        // Contrary to image types, image view types include arrayness and cubemapness
        VkImageViewType VulkanImageViewType(wgpu::TextureViewDimension dimension) {
            switch (dimension) {
                case wgpu::TextureViewDimension::e2D:
                    return VK_IMAGE_VIEW_TYPE_2D;
                case wgpu::TextureViewDimension::e2DArray:
                    return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
                case wgpu::TextureViewDimension::Cube:
                    return VK_IMAGE_VIEW_TYPE_CUBE;
                case wgpu::TextureViewDimension::CubeArray:
                    return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
                default:
                    UNREACHABLE();
            }
        }

        // Computes which vulkan access type could be required for the given Dawn usage.
        VkAccessFlags VulkanAccessFlags(wgpu::TextureUsage usage, const Format& format) {
            VkAccessFlags flags = 0;

            if (usage & wgpu::TextureUsage::CopySrc) {
                flags |= VK_ACCESS_TRANSFER_READ_BIT;
            }
            if (usage & wgpu::TextureUsage::CopyDst) {
                flags |= VK_ACCESS_TRANSFER_WRITE_BIT;
            }
            if (usage & wgpu::TextureUsage::Sampled) {
                flags |= VK_ACCESS_SHADER_READ_BIT;
            }
            if (usage & wgpu::TextureUsage::Storage) {
                flags |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            }
            if (usage & wgpu::TextureUsage::OutputAttachment) {
                if (format.HasDepthOrStencil()) {
                    flags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                             VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                } else {
                    flags |=
                        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                }
            }
            if (usage & kPresentTextureUsage) {
                // The present usage is only used internally by the swapchain and is never used in
                // combination with other usages.
                ASSERT(usage == kPresentTextureUsage);
                // The Vulkan spec has the following note:
                //
                //   When transitioning the image to VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR or
                //   VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, there is no need to delay subsequent
                //   processing, or perform any visibility operations (as vkQueuePresentKHR performs
                //   automatic visibility operations). To achieve this, the dstAccessMask member of
                //   the VkImageMemoryBarrier should be set to 0, and the dstStageMask parameter
                //   should be set to VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT.
                //
                // So on the transition to Present we don't need an access flag. The other
                // direction doesn't matter because swapchain textures always start a new frame
                // as uninitialized.
                flags |= 0;
            }

            return flags;
        }

        // Chooses which Vulkan image layout should be used for the given Dawn usage
        VkImageLayout VulkanImageLayout(wgpu::TextureUsage usage, const Format& format) {
            if (usage == wgpu::TextureUsage::None) {
                return VK_IMAGE_LAYOUT_UNDEFINED;
            }

            if (!wgpu::HasZeroOrOneBits(usage)) {
                return VK_IMAGE_LAYOUT_GENERAL;
            }

            // Usage has a single bit so we can switch on its value directly.
            switch (usage) {
                case wgpu::TextureUsage::CopyDst:
                    return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                case wgpu::TextureUsage::Sampled:
                    return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                // Vulkan texture copy functions require the image to be in _one_  known layout.
                // Depending on whether parts of the texture have been transitioned to only
                // CopySrc or a combination with something else, the texture could be in a
                // combination of GENERAL and TRANSFER_SRC_OPTIMAL. This would be a problem, so we
                // make CopySrc use GENERAL.
                case wgpu::TextureUsage::CopySrc:
                // Read-only and write-only storage textures must use general layout because load
                // and store operations on storage images can only be done on the images in
                // VK_IMAGE_LAYOUT_GENERAL layout.
                case wgpu::TextureUsage::Storage:
                case kReadonlyStorageTexture:
                    return VK_IMAGE_LAYOUT_GENERAL;
                case wgpu::TextureUsage::OutputAttachment:
                    if (format.HasDepthOrStencil()) {
                        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                    } else {
                        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                    }
                case kPresentTextureUsage:
                    return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                default:
                    UNREACHABLE();
            }
        }

        // Computes which Vulkan pipeline stage can access a texture in the given Dawn usage
        VkPipelineStageFlags VulkanPipelineStage(wgpu::TextureUsage usage, const Format& format) {
            VkPipelineStageFlags flags = 0;

            if (usage == wgpu::TextureUsage::None) {
                // This only happens when a texture is initially created (and for srcAccessMask) in
                // which case there is no need to wait on anything to stop accessing this texture.
                return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            }
            if (usage & (wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::CopyDst)) {
                flags |= VK_PIPELINE_STAGE_TRANSFER_BIT;
            }
            if (usage & (wgpu::TextureUsage::Sampled | kReadonlyStorageTexture)) {
                flags |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            }
            if (usage & wgpu::TextureUsage::Storage) {
                flags |=
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            }
            if (usage & wgpu::TextureUsage::OutputAttachment) {
                if (format.HasDepthOrStencil()) {
                    flags |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                             VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                    // TODO(cwallez@chromium.org): This is missing the stage where the depth and
                    // stencil values are written, but it isn't clear which one it is.
                } else {
                    flags |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                }
            }
            if (usage & kPresentTextureUsage) {
                // The present usage is only used internally by the swapchain and is never used in
                // combination with other usages.
                ASSERT(usage == kPresentTextureUsage);
                // The Vulkan spec has the following note:
                //
                //   When transitioning the image to VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR or
                //   VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, there is no need to delay subsequent
                //   processing, or perform any visibility operations (as vkQueuePresentKHR performs
                //   automatic visibility operations). To achieve this, the dstAccessMask member of
                //   the VkImageMemoryBarrier should be set to 0, and the dstStageMask parameter
                //   should be set to VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT.
                //
                // So on the transition to Present we use the "bottom of pipe" stage. The other
                // direction doesn't matter because swapchain textures always start a new frame
                // as uninitialized.
                flags |= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            }

            // A zero value isn't a valid pipeline stage mask
            ASSERT(flags != 0);
            return flags;
        }

        // Computes which Vulkan texture aspects are relevant for the given Dawn format
        VkImageAspectFlags VulkanAspectMask(const Format& format) {
            switch (format.aspect) {
                case Format::Aspect::Color:
                    return VK_IMAGE_ASPECT_COLOR_BIT;
                case Format::Aspect::Depth:
                    return VK_IMAGE_ASPECT_DEPTH_BIT;
                case Format::Aspect::Stencil:
                    return VK_IMAGE_ASPECT_STENCIL_BIT;
                case Format::Aspect::DepthStencil:
                    return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
                default:
                    UNREACHABLE();
                    return 0;
            }
        }

        VkImageMemoryBarrier BuildMemoryBarrier(const Format& format,
                                                const VkImage& image,
                                                wgpu::TextureUsage lastUsage,
                                                wgpu::TextureUsage usage,
                                                const SubresourceRange& range) {
            VkImageMemoryBarrier barrier;
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.pNext = nullptr;
            barrier.srcAccessMask = VulkanAccessFlags(lastUsage, format);
            barrier.dstAccessMask = VulkanAccessFlags(usage, format);
            barrier.oldLayout = VulkanImageLayout(lastUsage, format);
            barrier.newLayout = VulkanImageLayout(usage, format);
            barrier.image = image;
            barrier.subresourceRange.aspectMask = VulkanAspectMask(format);
            barrier.subresourceRange.baseMipLevel = range.baseMipLevel;
            barrier.subresourceRange.levelCount = range.levelCount;
            barrier.subresourceRange.baseArrayLayer = range.baseArrayLayer;
            barrier.subresourceRange.layerCount = range.layerCount;

            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            return barrier;
        }

        void FillVulkanCreateInfoSizesAndType(const Texture& texture, VkImageCreateInfo* info) {
            const Extent3D& size = texture.GetSize();

            info->mipLevels = texture.GetNumMipLevels();
            info->samples = VulkanSampleCount(texture.GetSampleCount());

            // Fill in the image type, and paper over differences in how the array layer count is
            // specified between WebGPU and Vulkan.
            switch (texture.GetDimension()) {
                case wgpu::TextureDimension::e2D:
                    info->imageType = VK_IMAGE_TYPE_2D;
                    info->extent = {size.width, size.height, 1};
                    info->arrayLayers = size.depth;
                    break;

                default:
                    UNREACHABLE();
                    break;
            }
        }

    }  // namespace

    // Converts Dawn texture format to Vulkan formats.
    VkFormat VulkanImageFormat(const Device* device, wgpu::TextureFormat format) {
        switch (format) {
            case wgpu::TextureFormat::R8Unorm:
                return VK_FORMAT_R8_UNORM;
            case wgpu::TextureFormat::R8Snorm:
                return VK_FORMAT_R8_SNORM;
            case wgpu::TextureFormat::R8Uint:
                return VK_FORMAT_R8_UINT;
            case wgpu::TextureFormat::R8Sint:
                return VK_FORMAT_R8_SINT;

            case wgpu::TextureFormat::R16Uint:
                return VK_FORMAT_R16_UINT;
            case wgpu::TextureFormat::R16Sint:
                return VK_FORMAT_R16_SINT;
            case wgpu::TextureFormat::R16Float:
                return VK_FORMAT_R16_SFLOAT;
            case wgpu::TextureFormat::RG8Unorm:
                return VK_FORMAT_R8G8_UNORM;
            case wgpu::TextureFormat::RG8Snorm:
                return VK_FORMAT_R8G8_SNORM;
            case wgpu::TextureFormat::RG8Uint:
                return VK_FORMAT_R8G8_UINT;
            case wgpu::TextureFormat::RG8Sint:
                return VK_FORMAT_R8G8_SINT;

            case wgpu::TextureFormat::R32Uint:
                return VK_FORMAT_R32_UINT;
            case wgpu::TextureFormat::R32Sint:
                return VK_FORMAT_R32_SINT;
            case wgpu::TextureFormat::R32Float:
                return VK_FORMAT_R32_SFLOAT;
            case wgpu::TextureFormat::RG16Uint:
                return VK_FORMAT_R16G16_UINT;
            case wgpu::TextureFormat::RG16Sint:
                return VK_FORMAT_R16G16_SINT;
            case wgpu::TextureFormat::RG16Float:
                return VK_FORMAT_R16G16_SFLOAT;
            case wgpu::TextureFormat::RGBA8Unorm:
                return VK_FORMAT_R8G8B8A8_UNORM;
            case wgpu::TextureFormat::RGBA8UnormSrgb:
                return VK_FORMAT_R8G8B8A8_SRGB;
            case wgpu::TextureFormat::RGBA8Snorm:
                return VK_FORMAT_R8G8B8A8_SNORM;
            case wgpu::TextureFormat::RGBA8Uint:
                return VK_FORMAT_R8G8B8A8_UINT;
            case wgpu::TextureFormat::RGBA8Sint:
                return VK_FORMAT_R8G8B8A8_SINT;
            case wgpu::TextureFormat::BGRA8Unorm:
                return VK_FORMAT_B8G8R8A8_UNORM;
            case wgpu::TextureFormat::BGRA8UnormSrgb:
                return VK_FORMAT_B8G8R8A8_SRGB;
            case wgpu::TextureFormat::RGB10A2Unorm:
                return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
            case wgpu::TextureFormat::RG11B10Float:
                return VK_FORMAT_B10G11R11_UFLOAT_PACK32;

            case wgpu::TextureFormat::RG32Uint:
                return VK_FORMAT_R32G32_UINT;
            case wgpu::TextureFormat::RG32Sint:
                return VK_FORMAT_R32G32_SINT;
            case wgpu::TextureFormat::RG32Float:
                return VK_FORMAT_R32G32_SFLOAT;
            case wgpu::TextureFormat::RGBA16Uint:
                return VK_FORMAT_R16G16B16A16_UINT;
            case wgpu::TextureFormat::RGBA16Sint:
                return VK_FORMAT_R16G16B16A16_SINT;
            case wgpu::TextureFormat::RGBA16Float:
                return VK_FORMAT_R16G16B16A16_SFLOAT;

            case wgpu::TextureFormat::RGBA32Uint:
                return VK_FORMAT_R32G32B32A32_UINT;
            case wgpu::TextureFormat::RGBA32Sint:
                return VK_FORMAT_R32G32B32A32_SINT;
            case wgpu::TextureFormat::RGBA32Float:
                return VK_FORMAT_R32G32B32A32_SFLOAT;

            case wgpu::TextureFormat::Depth32Float:
                return VK_FORMAT_D32_SFLOAT;
            case wgpu::TextureFormat::Depth24Plus:
                return VK_FORMAT_D32_SFLOAT;
            case wgpu::TextureFormat::Depth24PlusStencil8:
                // Depth24PlusStencil8 maps to either of these two formats because only requires
                // that one of the two be present. The VulkanUseD32S8 toggle combines the wish of
                // the environment, default to using D32S8, and availability information so we know
                // that the format is available.
                if (device->IsToggleEnabled(Toggle::VulkanUseD32S8)) {
                    return VK_FORMAT_D32_SFLOAT_S8_UINT;
                } else {
                    return VK_FORMAT_D24_UNORM_S8_UINT;
                }

            case wgpu::TextureFormat::BC1RGBAUnorm:
                return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
            case wgpu::TextureFormat::BC1RGBAUnormSrgb:
                return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
            case wgpu::TextureFormat::BC2RGBAUnorm:
                return VK_FORMAT_BC2_UNORM_BLOCK;
            case wgpu::TextureFormat::BC2RGBAUnormSrgb:
                return VK_FORMAT_BC2_SRGB_BLOCK;
            case wgpu::TextureFormat::BC3RGBAUnorm:
                return VK_FORMAT_BC3_UNORM_BLOCK;
            case wgpu::TextureFormat::BC3RGBAUnormSrgb:
                return VK_FORMAT_BC3_SRGB_BLOCK;
            case wgpu::TextureFormat::BC4RSnorm:
                return VK_FORMAT_BC4_SNORM_BLOCK;
            case wgpu::TextureFormat::BC4RUnorm:
                return VK_FORMAT_BC4_UNORM_BLOCK;
            case wgpu::TextureFormat::BC5RGSnorm:
                return VK_FORMAT_BC5_SNORM_BLOCK;
            case wgpu::TextureFormat::BC5RGUnorm:
                return VK_FORMAT_BC5_UNORM_BLOCK;
            case wgpu::TextureFormat::BC6HRGBSfloat:
                return VK_FORMAT_BC6H_SFLOAT_BLOCK;
            case wgpu::TextureFormat::BC6HRGBUfloat:
                return VK_FORMAT_BC6H_UFLOAT_BLOCK;
            case wgpu::TextureFormat::BC7RGBAUnorm:
                return VK_FORMAT_BC7_UNORM_BLOCK;
            case wgpu::TextureFormat::BC7RGBAUnormSrgb:
                return VK_FORMAT_BC7_SRGB_BLOCK;

            default:
                UNREACHABLE();
        }
    }

    // Converts the Dawn usage flags to Vulkan usage flags. Also needs the format to choose
    // between color and depth attachment usages.
    VkImageUsageFlags VulkanImageUsage(wgpu::TextureUsage usage, const Format& format) {
        VkImageUsageFlags flags = 0;

        if (usage & wgpu::TextureUsage::CopySrc) {
            flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }
        if (usage & wgpu::TextureUsage::CopyDst) {
            flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }
        if (usage & wgpu::TextureUsage::Sampled) {
            flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
        }
        if (usage & wgpu::TextureUsage::Storage) {
            flags |= VK_IMAGE_USAGE_STORAGE_BIT;
        }
        if (usage & wgpu::TextureUsage::OutputAttachment) {
            if (format.HasDepthOrStencil()) {
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

    MaybeError ValidateVulkanImageCanBeWrapped(const DeviceBase*,
                                               const TextureDescriptor* descriptor) {
        if (descriptor->dimension != wgpu::TextureDimension::e2D) {
            return DAWN_VALIDATION_ERROR("Texture must be 2D");
        }

        if (descriptor->mipLevelCount != 1) {
            return DAWN_VALIDATION_ERROR("Mip level count must be 1");
        }

        if (descriptor->size.depth != 1) {
            return DAWN_VALIDATION_ERROR("Array layer count must be 1");
        }

        if (descriptor->sampleCount != 1) {
            return DAWN_VALIDATION_ERROR("Sample count must be 1");
        }

        return {};
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

    // static
    ResultOrError<Ref<TextureBase>> Texture::Create(Device* device,
                                                    const TextureDescriptor* descriptor) {
        Ref<Texture> texture =
            AcquireRef(new Texture(device, descriptor, TextureState::OwnedInternal));
        DAWN_TRY(texture->InitializeAsInternalTexture());
        return std::move(texture);
    }

    // static
    ResultOrError<Texture*> Texture::CreateFromExternal(
        Device* device,
        const ExternalImageDescriptor* descriptor,
        const TextureDescriptor* textureDescriptor,
        external_memory::Service* externalMemoryService) {
        Ref<Texture> texture =
            AcquireRef(new Texture(device, textureDescriptor, TextureState::OwnedInternal));
        DAWN_TRY(texture->InitializeFromExternal(descriptor, externalMemoryService));
        return texture.Detach();
    }

    // static
    Ref<Texture> Texture::CreateForSwapChain(Device* device,
                                             const TextureDescriptor* descriptor,
                                             VkImage nativeImage) {
        Ref<Texture> texture =
            AcquireRef(new Texture(device, descriptor, TextureState::OwnedExternal));
        texture->InitializeForSwapChain(nativeImage);
        return std::move(texture);
    }

    MaybeError Texture::InitializeAsInternalTexture() {
        Device* device = ToBackend(GetDevice());

        // Create the Vulkan image "container". We don't need to check that the format supports the
        // combination of sample, usage etc. because validation should have been done in the Dawn
        // frontend already based on the minimum supported formats in the Vulkan spec
        VkImageCreateInfo createInfo = {};
        FillVulkanCreateInfoSizesAndType(*this, &createInfo);

        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.format = VulkanImageFormat(device, GetFormat().format);
        createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        createInfo.usage = VulkanImageUsage(GetUsage(), GetFormat());
        createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
        createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        ASSERT(IsSampleCountSupported(device, createInfo));

        if (GetArrayLayers() >= 6 && GetWidth() == GetHeight()) {
            createInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        }

        // We always set VK_IMAGE_USAGE_TRANSFER_DST_BIT unconditionally beause the Vulkan images
        // that are used in vkCmdClearColorImage() must have been created with this flag, which is
        // also required for the implementation of robust resource initialization.
        createInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        DAWN_TRY(CheckVkSuccess(
            device->fn.CreateImage(device->GetVkDevice(), &createInfo, nullptr, &*mHandle),
            "CreateImage"));

        // Create the image memory and associate it with the container
        VkMemoryRequirements requirements;
        device->fn.GetImageMemoryRequirements(device->GetVkDevice(), mHandle, &requirements);

        DAWN_TRY_ASSIGN(mMemoryAllocation, device->AllocateMemory(requirements, false));

        DAWN_TRY(CheckVkSuccess(
            device->fn.BindImageMemory(device->GetVkDevice(), mHandle,
                                       ToBackend(mMemoryAllocation.GetResourceHeap())->GetMemory(),
                                       mMemoryAllocation.GetOffset()),
            "BindImageMemory"));

        if (device->IsToggleEnabled(Toggle::NonzeroClearResourcesOnCreationForTesting)) {
            DAWN_TRY(ClearTexture(ToBackend(GetDevice())->GetPendingRecordingContext(),
                                  GetAllSubresources(), TextureBase::ClearValue::NonZero));
        }

        return {};
    }

    // Internally managed, but imported from external handle
    MaybeError Texture::InitializeFromExternal(const ExternalImageDescriptor* descriptor,
                                               external_memory::Service* externalMemoryService) {
        VkFormat format = VulkanImageFormat(ToBackend(GetDevice()), GetFormat().format);
        VkImageUsageFlags usage = VulkanImageUsage(GetUsage(), GetFormat());
        if (!externalMemoryService->SupportsCreateImage(descriptor, format, usage)) {
            return DAWN_VALIDATION_ERROR("Creating an image from external memory is not supported");
        }

        mExternalState = ExternalState::PendingAcquire;

        VkImageCreateInfo baseCreateInfo = {};
        FillVulkanCreateInfoSizesAndType(*this, &baseCreateInfo);

        baseCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        baseCreateInfo.pNext = nullptr;
        baseCreateInfo.format = format;
        baseCreateInfo.usage = usage;
        baseCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        baseCreateInfo.queueFamilyIndexCount = 0;
        baseCreateInfo.pQueueFamilyIndices = nullptr;

        // We always set VK_IMAGE_USAGE_TRANSFER_DST_BIT unconditionally beause the Vulkan images
        // that are used in vkCmdClearColorImage() must have been created with this flag, which is
        // also required for the implementation of robust resource initialization.
        baseCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        DAWN_TRY_ASSIGN(mHandle, externalMemoryService->CreateImage(descriptor, baseCreateInfo));
        return {};
    }

    void Texture::InitializeForSwapChain(VkImage nativeImage) {
        mHandle = nativeImage;
    }

    MaybeError Texture::BindExternalMemory(const ExternalImageDescriptor* descriptor,
                                           VkSemaphore signalSemaphore,
                                           VkDeviceMemory externalMemoryAllocation,
                                           std::vector<VkSemaphore> waitSemaphores) {
        Device* device = ToBackend(GetDevice());
        DAWN_TRY(CheckVkSuccess(
            device->fn.BindImageMemory(device->GetVkDevice(), mHandle, externalMemoryAllocation, 0),
            "BindImageMemory (external)"));

        // Don't clear imported texture if already cleared
        if (descriptor->isCleared) {
            SetIsSubresourceContentInitialized(true, {0, 1, 0, 1});
        }

        // Success, acquire all the external objects.
        mExternalAllocation = externalMemoryAllocation;
        mSignalSemaphore = signalSemaphore;
        mWaitRequirements = std::move(waitSemaphores);
        return {};
    }

    MaybeError Texture::SignalAndDestroy(VkSemaphore* outSignalSemaphore) {
        Device* device = ToBackend(GetDevice());

        if (mExternalState == ExternalState::Released) {
            return DAWN_VALIDATION_ERROR("Can't export signal semaphore from signaled texture");
        }

        if (mExternalAllocation == VK_NULL_HANDLE) {
            return DAWN_VALIDATION_ERROR(
                "Can't export signal semaphore from destroyed / non-external texture");
        }

        ASSERT(mSignalSemaphore != VK_NULL_HANDLE);

        // Release the texture
        mExternalState = ExternalState::PendingRelease;
        TransitionFullUsage(device->GetPendingRecordingContext(), wgpu::TextureUsage::None);

        // Queue submit to signal we are done with the texture
        device->GetPendingRecordingContext()->signalSemaphores.push_back(mSignalSemaphore);
        DAWN_TRY(device->SubmitPendingCommands());

        // Write out the signal semaphore
        *outSignalSemaphore = mSignalSemaphore;
        mSignalSemaphore = VK_NULL_HANDLE;

        // Destroy the texture so it can't be used again
        DestroyInternal();
        return {};
    }

    Texture::~Texture() {
        DestroyInternal();
    }

    void Texture::DestroyImpl() {
        if (GetTextureState() == TextureState::OwnedInternal) {
            Device* device = ToBackend(GetDevice());

            // For textures created from a VkImage, the allocation if kInvalid so the Device knows
            // to skip the deallocation of the (absence of) VkDeviceMemory.
            device->DeallocateMemory(&mMemoryAllocation);

            if (mHandle != VK_NULL_HANDLE) {
                device->GetFencedDeleter()->DeleteWhenUnused(mHandle);
            }

            if (mExternalAllocation != VK_NULL_HANDLE) {
                device->GetFencedDeleter()->DeleteWhenUnused(mExternalAllocation);
            }

            mHandle = VK_NULL_HANDLE;
            mExternalAllocation = VK_NULL_HANDLE;
            // If a signal semaphore exists it should be requested before we delete the texture
            ASSERT(mSignalSemaphore == VK_NULL_HANDLE);
        }
    }

    VkImage Texture::GetHandle() const {
        return mHandle;
    }

    VkImageAspectFlags Texture::GetVkAspectMask() const {
        return VulkanAspectMask(GetFormat());
    }

    void Texture::TweakTransitionForExternalUsage(CommandRecordingContext* recordingContext,
                                                  std::vector<VkImageMemoryBarrier>* barriers,
                                                  size_t transitionBarrierStart) {
        ASSERT(GetNumMipLevels() == 1 && GetArrayLayers() == 1);

        // transitionBarrierStart specify the index where barriers for current transition start in
        // the vector. barriers->size() - transitionBarrierStart is the number of barriers that we
        // have already added into the vector during current transition.
        ASSERT(barriers->size() - transitionBarrierStart <= 1);

        if (mExternalState == ExternalState::PendingAcquire) {
            if (barriers->size() == transitionBarrierStart) {
                barriers->push_back(BuildMemoryBarrier(
                    GetFormat(), mHandle, wgpu::TextureUsage::None, wgpu::TextureUsage::None,
                    SubresourceRange::SingleSubresource(0, 0)));
            }

            // Transfer texture from external queue to graphics queue
            (*barriers)[transitionBarrierStart].srcQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL_KHR;
            (*barriers)[transitionBarrierStart].dstQueueFamilyIndex =
                ToBackend(GetDevice())->GetGraphicsQueueFamily();
            // Don't override oldLayout to leave it as VK_IMAGE_LAYOUT_UNDEFINED
            // TODO(http://crbug.com/dawn/200)
            mExternalState = ExternalState::Acquired;
        } else if (mExternalState == ExternalState::PendingRelease) {
            if (barriers->size() == transitionBarrierStart) {
                barriers->push_back(BuildMemoryBarrier(
                    GetFormat(), mHandle, wgpu::TextureUsage::None, wgpu::TextureUsage::None,
                    SubresourceRange::SingleSubresource(0, 0)));
            }

            // Transfer texture from graphics queue to external queue
            (*barriers)[transitionBarrierStart].srcQueueFamilyIndex =
                ToBackend(GetDevice())->GetGraphicsQueueFamily();
            (*barriers)[transitionBarrierStart].dstQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL_KHR;
            (*barriers)[transitionBarrierStart].newLayout = VK_IMAGE_LAYOUT_GENERAL;
            mExternalState = ExternalState::Released;
        }

        mLastExternalState = mExternalState;

        recordingContext->waitSemaphores.insert(recordingContext->waitSemaphores.end(),
                                                mWaitRequirements.begin(), mWaitRequirements.end());
        mWaitRequirements.clear();
    }

    bool Texture::CanReuseWithoutBarrier(wgpu::TextureUsage lastUsage, wgpu::TextureUsage usage) {
        // Reuse the texture directly and avoid encoding barriers when it isn't needed.
        bool lastReadOnly = (lastUsage & kReadOnlyTextureUsages) == lastUsage;
        if (lastReadOnly && lastUsage == usage && mLastExternalState == mExternalState) {
            return true;
        }
        return false;
    }

    void Texture::TransitionFullUsage(CommandRecordingContext* recordingContext,
                                      wgpu::TextureUsage usage) {
        TransitionUsageNow(recordingContext, usage, GetAllSubresources());
    }

    void Texture::TransitionUsageForPass(CommandRecordingContext* recordingContext,
                                         const PassTextureUsage& textureUsages,
                                         std::vector<VkImageMemoryBarrier>* imageBarriers,
                                         VkPipelineStageFlags* srcStages,
                                         VkPipelineStageFlags* dstStages) {
        size_t transitionBarrierStart = imageBarriers->size();
        const Format& format = GetFormat();

        wgpu::TextureUsage allUsages = wgpu::TextureUsage::None;
        wgpu::TextureUsage allLastUsages = wgpu::TextureUsage::None;

        uint32_t subresourceCount = GetSubresourceCount();
        ASSERT(textureUsages.subresourceUsages.size() == subresourceCount);
        // This transitions assume it is a 2D texture
        ASSERT(GetDimension() == wgpu::TextureDimension::e2D);

        // If new usages of all subresources are the same and old usages of all subresources are
        // the same too, we can use one barrier to do state transition for all subresources.
        // Note that if the texture has only one mip level and one array slice, it will fall into
        // this category.
        if (textureUsages.sameUsagesAcrossSubresources && mSameLastUsagesAcrossSubresources) {
            if (CanReuseWithoutBarrier(mSubresourceLastUsages[0], textureUsages.usage)) {
                return;
            }

            imageBarriers->push_back(BuildMemoryBarrier(format, mHandle, mSubresourceLastUsages[0],
                                                        textureUsages.usage, GetAllSubresources()));
            allLastUsages = mSubresourceLastUsages[0];
            allUsages = textureUsages.usage;
            for (uint32_t i = 0; i < subresourceCount; ++i) {
                mSubresourceLastUsages[i] = textureUsages.usage;
            }
        } else {
            for (uint32_t arrayLayer = 0; arrayLayer < GetArrayLayers(); ++arrayLayer) {
                for (uint32_t mipLevel = 0; mipLevel < GetNumMipLevels(); ++mipLevel) {
                    uint32_t index = GetSubresourceIndex(mipLevel, arrayLayer);

                    // Avoid encoding barriers when it isn't needed.
                    if (textureUsages.subresourceUsages[index] == wgpu::TextureUsage::None) {
                        continue;
                    }

                    if (CanReuseWithoutBarrier(mSubresourceLastUsages[index],
                                               textureUsages.subresourceUsages[index])) {
                        continue;
                    }
                    imageBarriers->push_back(BuildMemoryBarrier(
                        format, mHandle, mSubresourceLastUsages[index],
                        textureUsages.subresourceUsages[index],
                        SubresourceRange::SingleSubresource(mipLevel, arrayLayer)));
                    allLastUsages |= mSubresourceLastUsages[index];
                    allUsages |= textureUsages.subresourceUsages[index];
                    mSubresourceLastUsages[index] = textureUsages.subresourceUsages[index];
                }
            }
        }

        if (mExternalState != ExternalState::InternalOnly) {
            TweakTransitionForExternalUsage(recordingContext, imageBarriers,
                                            transitionBarrierStart);
        }

        *srcStages |= VulkanPipelineStage(allLastUsages, format);
        *dstStages |= VulkanPipelineStage(allUsages, format);
        mSameLastUsagesAcrossSubresources = textureUsages.sameUsagesAcrossSubresources;
    }

    void Texture::TransitionUsageNow(CommandRecordingContext* recordingContext,
                                     wgpu::TextureUsage usage,
                                     const SubresourceRange& range) {
        std::vector<VkImageMemoryBarrier> barriers;
        const Format& format = GetFormat();

        wgpu::TextureUsage allLastUsages = wgpu::TextureUsage::None;
        uint32_t subresourceCount = GetSubresourceCount();

        // This transitions assume it is a 2D texture
        ASSERT(GetDimension() == wgpu::TextureDimension::e2D);

        // If the usages transitions can cover all subresources, and old usages of all subresources
        // are the same, then we can use one barrier to do state transition for all subresources.
        // Note that if the texture has only one mip level and one array slice, it will fall into
        // this category.
        bool areAllSubresourcesCovered = range.levelCount * range.layerCount == subresourceCount;
        if (mSameLastUsagesAcrossSubresources && areAllSubresourcesCovered) {
            ASSERT(range.baseMipLevel == 0 && range.baseArrayLayer == 0);
            if (CanReuseWithoutBarrier(mSubresourceLastUsages[0], usage)) {
                return;
            }
            barriers.push_back(
                BuildMemoryBarrier(format, mHandle, mSubresourceLastUsages[0], usage, range));
            allLastUsages = mSubresourceLastUsages[0];
            for (uint32_t i = 0; i < subresourceCount; ++i) {
                mSubresourceLastUsages[i] = usage;
            }
        } else {
            for (uint32_t layer = range.baseArrayLayer;
                 layer < range.baseArrayLayer + range.layerCount; ++layer) {
                for (uint32_t level = range.baseMipLevel;
                     level < range.baseMipLevel + range.levelCount; ++level) {
                    uint32_t index = GetSubresourceIndex(level, layer);

                    if (CanReuseWithoutBarrier(mSubresourceLastUsages[index], usage)) {
                        continue;
                    }

                    barriers.push_back(
                        BuildMemoryBarrier(format, mHandle, mSubresourceLastUsages[index], usage,
                                           SubresourceRange::SingleSubresource(level, layer)));
                    allLastUsages |= mSubresourceLastUsages[index];
                    mSubresourceLastUsages[index] = usage;
                }
            }
        }

        if (mExternalState != ExternalState::InternalOnly) {
            TweakTransitionForExternalUsage(recordingContext, &barriers, 0);
        }

        VkPipelineStageFlags srcStages = VulkanPipelineStage(allLastUsages, format);
        VkPipelineStageFlags dstStages = VulkanPipelineStage(usage, format);
        ToBackend(GetDevice())
            ->fn.CmdPipelineBarrier(recordingContext->commandBuffer, srcStages, dstStages, 0, 0,
                                    nullptr, 0, nullptr, barriers.size(), barriers.data());

        mSameLastUsagesAcrossSubresources = areAllSubresourcesCovered;
    }

    MaybeError Texture::ClearTexture(CommandRecordingContext* recordingContext,
                                     const SubresourceRange& range,
                                     TextureBase::ClearValue clearValue) {
        Device* device = ToBackend(GetDevice());

        uint8_t clearColor = (clearValue == TextureBase::ClearValue::Zero) ? 0 : 1;
        float fClearColor = (clearValue == TextureBase::ClearValue::Zero) ? 0.f : 1.f;

        TransitionUsageNow(recordingContext, wgpu::TextureUsage::CopyDst, range);
        if (GetFormat().isRenderable) {
            VkImageSubresourceRange imageRange = {};
            imageRange.aspectMask = GetVkAspectMask();
            imageRange.levelCount = 1;
            imageRange.layerCount = 1;

            for (uint32_t level = range.baseMipLevel; level < range.baseMipLevel + range.levelCount;
                 ++level) {
                imageRange.baseMipLevel = level;
                for (uint32_t layer = range.baseArrayLayer;
                     layer < range.baseArrayLayer + range.layerCount; ++layer) {
                    if (clearValue == TextureBase::ClearValue::Zero &&
                        IsSubresourceContentInitialized(
                            SubresourceRange::SingleSubresource(level, layer))) {
                        // Skip lazy clears if already initialized.
                        continue;
                    }

                    imageRange.baseArrayLayer = layer;

                    if (GetFormat().HasDepthOrStencil()) {
                        VkClearDepthStencilValue clearDepthStencilValue[1];
                        clearDepthStencilValue[0].depth = fClearColor;
                        clearDepthStencilValue[0].stencil = clearColor;
                        device->fn.CmdClearDepthStencilImage(
                            recordingContext->commandBuffer, GetHandle(),
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, clearDepthStencilValue, 1,
                            &imageRange);
                    } else {
                        VkClearColorValue clearColorValue = {
                            {fClearColor, fClearColor, fClearColor, fClearColor}};
                        device->fn.CmdClearColorImage(recordingContext->commandBuffer, GetHandle(),
                                                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                      &clearColorValue, 1, &imageRange);
                    }
                }
            }
        } else {
            // TODO(natlee@microsoft.com): test compressed textures are cleared
            // create temp buffer with clear color to copy to the texture image
            uint32_t bytesPerRow =
                Align((GetWidth() / GetFormat().blockWidth) * GetFormat().blockByteSize,
                      kTextureBytesPerRowAlignment);
            uint64_t bufferSize64 = bytesPerRow * (GetHeight() / GetFormat().blockHeight);
            if (bufferSize64 > std::numeric_limits<uint32_t>::max()) {
                return DAWN_OUT_OF_MEMORY_ERROR("Unable to allocate buffer.");
            }
            uint32_t bufferSize = static_cast<uint32_t>(bufferSize64);
            DynamicUploader* uploader = device->GetDynamicUploader();
            UploadHandle uploadHandle;
            DAWN_TRY_ASSIGN(uploadHandle,
                            uploader->Allocate(bufferSize, device->GetPendingCommandSerial()));
            memset(uploadHandle.mappedBuffer, clearColor, bufferSize);

            // compute the buffer image copy to set the clear region of entire texture
            dawn_native::BufferCopy bufferCopy;
            bufferCopy.rowsPerImage = 0;
            bufferCopy.offset = uploadHandle.startOffset;
            bufferCopy.bytesPerRow = bytesPerRow;

            for (uint32_t level = range.baseMipLevel; level < range.baseMipLevel + range.levelCount;
                 ++level) {
                Extent3D copySize = GetMipLevelVirtualSize(level);

                for (uint32_t layer = range.baseArrayLayer;
                     layer < range.baseArrayLayer + range.layerCount; ++layer) {
                    if (clearValue == TextureBase::ClearValue::Zero &&
                        IsSubresourceContentInitialized(
                            SubresourceRange::SingleSubresource(level, layer))) {
                        // Skip lazy clears if already initialized.
                        continue;
                    }

                    dawn_native::TextureCopy textureCopy;
                    textureCopy.texture = this;
                    textureCopy.origin = {0, 0, layer};
                    textureCopy.mipLevel = level;

                    VkBufferImageCopy region =
                        ComputeBufferImageCopyRegion(bufferCopy, textureCopy, copySize);

                    // copy the clear buffer to the texture image
                    device->fn.CmdCopyBufferToImage(
                        recordingContext->commandBuffer,
                        ToBackend(uploadHandle.stagingBuffer)->GetBufferHandle(), GetHandle(),
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
                }
            }
        }
        if (clearValue == TextureBase::ClearValue::Zero) {
            SetIsSubresourceContentInitialized(true, range);
            device->IncrementLazyClearCountForTesting();
        }
        return {};
    }

    void Texture::EnsureSubresourceContentInitialized(CommandRecordingContext* recordingContext,
                                                      const SubresourceRange& range) {
        if (!GetDevice()->IsToggleEnabled(Toggle::LazyClearResourceOnFirstUse)) {
            return;
        }
        if (!IsSubresourceContentInitialized(range)) {
            // TODO(jiawei.shao@intel.com): initialize textures in BC formats with Buffer-to-Texture
            // copies.
            if (GetFormat().isCompressed) {
                return;
            }

            // If subresource has not been initialized, clear it to black as it could contain dirty
            // bits from recycled memory
            GetDevice()->ConsumedError(
                ClearTexture(recordingContext, range, TextureBase::ClearValue::Zero));
        }
    }

    // static
    ResultOrError<TextureView*> TextureView::Create(TextureBase* texture,
                                                    const TextureViewDescriptor* descriptor) {
        Ref<TextureView> view = AcquireRef(new TextureView(texture, descriptor));
        DAWN_TRY(view->Initialize(descriptor));
        return view.Detach();
    }

    MaybeError TextureView::Initialize(const TextureViewDescriptor* descriptor) {
        if ((GetTexture()->GetUsage() &
             ~(wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::CopyDst)) == 0) {
            // If the texture view has no other usage than CopySrc and CopyDst, then it can't
            // actually be used as a render pass attachment or sampled/storage texture. The Vulkan
            // validation errors warn if you create such a vkImageView, so return early.
            return {};
        }

        Device* device = ToBackend(GetTexture()->GetDevice());

        VkImageViewCreateInfo createInfo;
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.image = ToBackend(GetTexture())->GetHandle();
        createInfo.viewType = VulkanImageViewType(descriptor->dimension);
        createInfo.format = VulkanImageFormat(device, descriptor->format);
        createInfo.components = VkComponentMapping{VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G,
                                                   VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
        createInfo.subresourceRange.aspectMask = VulkanAspectMask(GetFormat());
        createInfo.subresourceRange.baseMipLevel = descriptor->baseMipLevel;
        createInfo.subresourceRange.levelCount = descriptor->mipLevelCount;
        createInfo.subresourceRange.baseArrayLayer = descriptor->baseArrayLayer;
        createInfo.subresourceRange.layerCount = descriptor->arrayLayerCount;

        return CheckVkSuccess(
            device->fn.CreateImageView(device->GetVkDevice(), &createInfo, nullptr, &*mHandle),
            "CreateImageView");
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
