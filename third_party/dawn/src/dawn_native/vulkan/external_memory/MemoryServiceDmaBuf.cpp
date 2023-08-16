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

#include "common/Assert.h"
#include "dawn_native/vulkan/AdapterVk.h"
#include "dawn_native/vulkan/BackendVk.h"
#include "dawn_native/vulkan/DeviceVk.h"
#include "dawn_native/vulkan/VulkanError.h"
#include "dawn_native/vulkan/external_memory/MemoryService.h"

namespace dawn_native { namespace vulkan { namespace external_memory {

    namespace {

        // Some modifiers use multiple planes (for example, see the comment for
        // I915_FORMAT_MOD_Y_TILED_CCS in drm/drm_fourcc.h), but dma-buf import in Dawn only
        // supports single-plane formats.
        ResultOrError<uint32_t> GetModifierPlaneCount(const VulkanFunctions& fn,
                                                      VkPhysicalDevice physicalDevice,
                                                      VkFormat format,
                                                      uint64_t modifier) {
            VkDrmFormatModifierPropertiesListEXT formatModifierPropsList;
            formatModifierPropsList.sType =
                VK_STRUCTURE_TYPE_DRM_FORMAT_MODIFIER_PROPERTIES_LIST_EXT;
            formatModifierPropsList.pNext = nullptr;
            formatModifierPropsList.drmFormatModifierCount = 0;
            formatModifierPropsList.pDrmFormatModifierProperties = nullptr;

            VkFormatProperties2 formatProps;
            formatProps.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2;
            formatProps.pNext = &formatModifierPropsList;

            fn.GetPhysicalDeviceFormatProperties2(physicalDevice, format, &formatProps);

            uint32_t modifierCount = formatModifierPropsList.drmFormatModifierCount;
            std::vector<VkDrmFormatModifierPropertiesEXT> formatModifierProps(modifierCount);
            formatModifierPropsList.pDrmFormatModifierProperties = formatModifierProps.data();

            fn.GetPhysicalDeviceFormatProperties2(physicalDevice, format, &formatProps);
            for (const auto& props : formatModifierProps) {
                if (props.drmFormatModifier == modifier) {
                    uint32_t count = props.drmFormatModifierPlaneCount;
                    return count;
                }
            }
            return DAWN_VALIDATION_ERROR("DRM format modifier not supported");
        }

    }  // anonymous namespace

    Service::Service(Device* device) : mDevice(device) {
        const VulkanDeviceInfo& deviceInfo = mDevice->GetDeviceInfo();

        mSupported = deviceInfo.HasExt(DeviceExt::ExternalMemoryFD) &&
                     deviceInfo.HasExt(DeviceExt::ImageDrmFormatModifier);
    }

    Service::~Service() = default;

    bool Service::SupportsImportMemory(VkFormat format,
                                       VkImageType type,
                                       VkImageTiling tiling,
                                       VkImageUsageFlags usage,
                                       VkImageCreateFlags flags) {
        return mSupported;
    }

    bool Service::SupportsCreateImage(const ExternalImageDescriptor* descriptor,
                                      VkFormat format,
                                      VkImageUsageFlags usage) {
        // Early out before we try using extension functions
        if (!mSupported) {
            return false;
        }
        if (descriptor->type != ExternalImageDescriptorType::DmaBuf) {
            return false;
        }
        const ExternalImageDescriptorDmaBuf* dmaBufDescriptor =
            static_cast<const ExternalImageDescriptorDmaBuf*>(descriptor);

        // Verify plane count for the modifier.
        VkPhysicalDevice physicalDevice = ToBackend(mDevice->GetAdapter())->GetPhysicalDevice();
        uint32_t planeCount = 0;
        if (mDevice->ConsumedError(GetModifierPlaneCount(mDevice->fn, physicalDevice, format,
                                                         dmaBufDescriptor->drmModifier),
                                   &planeCount)) {
            return false;
        }
        if (planeCount == 0) {
            return false;
        }
        // TODO(hob): Support multi-plane formats like I915_FORMAT_MOD_Y_TILED_CCS.
        if (planeCount > 1) {
            return false;
        }

        // Verify that the format modifier of the external memory and the requested Vulkan format
        // are actually supported together in a dma-buf import.
        VkPhysicalDeviceImageDrmFormatModifierInfoEXT drmModifierInfo;
        drmModifierInfo.sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_DRM_FORMAT_MODIFIER_INFO_EXT;
        drmModifierInfo.pNext = nullptr;
        drmModifierInfo.drmFormatModifier = dmaBufDescriptor->drmModifier;
        drmModifierInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkPhysicalDeviceExternalImageFormatInfo externalImageFormatInfo;
        externalImageFormatInfo.sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO;
        externalImageFormatInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;
        externalImageFormatInfo.pNext = &drmModifierInfo;

        VkPhysicalDeviceImageFormatInfo2 imageFormatInfo;
        imageFormatInfo.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2;
        imageFormatInfo.format = format;
        imageFormatInfo.type = VK_IMAGE_TYPE_2D;
        imageFormatInfo.tiling = VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT;
        imageFormatInfo.usage = usage;
        imageFormatInfo.flags = 0;
        imageFormatInfo.pNext = &externalImageFormatInfo;

        VkExternalImageFormatProperties externalImageFormatProps;
        externalImageFormatProps.sType = VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES;
        externalImageFormatProps.pNext = nullptr;

        VkImageFormatProperties2 imageFormatProps;
        imageFormatProps.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2;
        imageFormatProps.pNext = &externalImageFormatProps;

        VkResult result = VkResult::WrapUnsafe(mDevice->fn.GetPhysicalDeviceImageFormatProperties2(
            physicalDevice, &imageFormatInfo, &imageFormatProps));
        if (result != VK_SUCCESS) {
            return false;
        }
        VkExternalMemoryFeatureFlags featureFlags =
            externalImageFormatProps.externalMemoryProperties.externalMemoryFeatures;
        return featureFlags & VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT;
    }

    ResultOrError<MemoryImportParams> Service::GetMemoryImportParams(
        const ExternalImageDescriptor* descriptor,
        VkImage image) {
        if (descriptor->type != ExternalImageDescriptorType::DmaBuf) {
            return DAWN_VALIDATION_ERROR("ExternalImageDescriptor is not a dma-buf descriptor");
        }
        const ExternalImageDescriptorDmaBuf* dmaBufDescriptor =
            static_cast<const ExternalImageDescriptorDmaBuf*>(descriptor);
        VkDevice device = mDevice->GetVkDevice();

        // Get the valid memory types for the VkImage.
        VkMemoryRequirements memoryRequirements;
        mDevice->fn.GetImageMemoryRequirements(device, image, &memoryRequirements);

        VkMemoryFdPropertiesKHR fdProperties;
        fdProperties.sType = VK_STRUCTURE_TYPE_MEMORY_FD_PROPERTIES_KHR;
        fdProperties.pNext = nullptr;

        // Get the valid memory types that the external memory can be imported as.
        mDevice->fn.GetMemoryFdPropertiesKHR(device, VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT,
                                             dmaBufDescriptor->memoryFD, &fdProperties);
        // Choose the best memory type that satisfies both the image's constraint and the import's
        // constraint.
        memoryRequirements.memoryTypeBits &= fdProperties.memoryTypeBits;
        int memoryTypeIndex =
            mDevice->FindBestMemoryTypeIndex(memoryRequirements, false /** mappable */);
        if (memoryTypeIndex == -1) {
            return DAWN_VALIDATION_ERROR("Unable to find appropriate memory type for import");
        }
        MemoryImportParams params = {memoryRequirements.size,
                                     static_cast<uint32_t>(memoryTypeIndex)};
        return params;
    }

    ResultOrError<VkDeviceMemory> Service::ImportMemory(ExternalMemoryHandle handle,
                                                        const MemoryImportParams& importParams,
                                                        VkImage image) {
        if (handle < 0) {
            return DAWN_VALIDATION_ERROR("Trying to import memory with invalid handle");
        }

        VkMemoryDedicatedAllocateInfo memoryDedicatedAllocateInfo;
        memoryDedicatedAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
        memoryDedicatedAllocateInfo.pNext = nullptr;
        memoryDedicatedAllocateInfo.image = image;
        memoryDedicatedAllocateInfo.buffer = VkBuffer{};

        VkImportMemoryFdInfoKHR importMemoryFdInfo;
        importMemoryFdInfo.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR;
        importMemoryFdInfo.pNext = &memoryDedicatedAllocateInfo;
        importMemoryFdInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT,
        importMemoryFdInfo.fd = handle;

        VkMemoryAllocateInfo memoryAllocateInfo;
        memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memoryAllocateInfo.pNext = &importMemoryFdInfo;
        memoryAllocateInfo.allocationSize = importParams.allocationSize;
        memoryAllocateInfo.memoryTypeIndex = importParams.memoryTypeIndex;

        VkDeviceMemory allocatedMemory = VK_NULL_HANDLE;
        DAWN_TRY(
            CheckVkSuccess(mDevice->fn.AllocateMemory(mDevice->GetVkDevice(), &memoryAllocateInfo,
                                                      nullptr, &*allocatedMemory),
                           "vkAllocateMemory"));
        return allocatedMemory;
    }

    ResultOrError<VkImage> Service::CreateImage(const ExternalImageDescriptor* descriptor,
                                                const VkImageCreateInfo& baseCreateInfo) {
        if (descriptor->type != ExternalImageDescriptorType::DmaBuf) {
            return DAWN_VALIDATION_ERROR("ExternalImageDescriptor is not a dma-buf descriptor");
        }
        const ExternalImageDescriptorDmaBuf* dmaBufDescriptor =
            static_cast<const ExternalImageDescriptorDmaBuf*>(descriptor);
        VkPhysicalDevice physicalDevice = ToBackend(mDevice->GetAdapter())->GetPhysicalDevice();
        VkDevice device = mDevice->GetVkDevice();

        // Dawn currently doesn't support multi-plane formats, so we only need to create a single
        // VkSubresourceLayout here.
        VkSubresourceLayout planeLayout;
        planeLayout.offset = 0;
        planeLayout.size = 0;  // VK_EXT_image_drm_format_modifier mandates size = 0.
        planeLayout.rowPitch = dmaBufDescriptor->stride;
        planeLayout.arrayPitch = 0;  // Not an array texture
        planeLayout.depthPitch = 0;  // Not a depth texture

        uint32_t planeCount;
        DAWN_TRY_ASSIGN(planeCount,
                        GetModifierPlaneCount(mDevice->fn, physicalDevice, baseCreateInfo.format,
                                              dmaBufDescriptor->drmModifier));
        ASSERT(planeCount == 1);

        VkImageDrmFormatModifierExplicitCreateInfoEXT explicitCreateInfo;
        explicitCreateInfo.sType =
            VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_EXPLICIT_CREATE_INFO_EXT;
        explicitCreateInfo.pNext = NULL;
        explicitCreateInfo.drmFormatModifier = dmaBufDescriptor->drmModifier;
        explicitCreateInfo.drmFormatModifierPlaneCount = planeCount;
        explicitCreateInfo.pPlaneLayouts = &planeLayout;

        VkExternalMemoryImageCreateInfo externalMemoryImageCreateInfo;
        externalMemoryImageCreateInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
        externalMemoryImageCreateInfo.pNext = &explicitCreateInfo;
        externalMemoryImageCreateInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;

        VkImageCreateInfo createInfo = baseCreateInfo;
        createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        createInfo.pNext = &externalMemoryImageCreateInfo;
        createInfo.flags = 0;
        createInfo.tiling = VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT;

        // Create a new VkImage with tiling equal to the DRM format modifier.
        VkImage image;
        DAWN_TRY(CheckVkSuccess(mDevice->fn.CreateImage(device, &createInfo, nullptr, &*image),
                                "CreateImage"));
        return image;
    }

}}}  // namespace dawn_native::vulkan::external_memory
