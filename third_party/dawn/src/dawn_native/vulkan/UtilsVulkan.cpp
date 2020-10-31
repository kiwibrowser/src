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

#include "dawn_native/vulkan/UtilsVulkan.h"

#include "common/Assert.h"
#include "dawn_native/Format.h"
#include "dawn_native/vulkan/Forward.h"
#include "dawn_native/vulkan/TextureVk.h"

namespace dawn_native { namespace vulkan {

    VkCompareOp ToVulkanCompareOp(wgpu::CompareFunction op) {
        switch (op) {
            case wgpu::CompareFunction::Never:
                return VK_COMPARE_OP_NEVER;
            case wgpu::CompareFunction::Less:
                return VK_COMPARE_OP_LESS;
            case wgpu::CompareFunction::LessEqual:
                return VK_COMPARE_OP_LESS_OR_EQUAL;
            case wgpu::CompareFunction::Greater:
                return VK_COMPARE_OP_GREATER;
            case wgpu::CompareFunction::GreaterEqual:
                return VK_COMPARE_OP_GREATER_OR_EQUAL;
            case wgpu::CompareFunction::Equal:
                return VK_COMPARE_OP_EQUAL;
            case wgpu::CompareFunction::NotEqual:
                return VK_COMPARE_OP_NOT_EQUAL;
            case wgpu::CompareFunction::Always:
                return VK_COMPARE_OP_ALWAYS;
            default:
                UNREACHABLE();
        }
    }

    // Vulkan SPEC requires the source/destination region specified by each element of
    // pRegions must be a region that is contained within srcImage/dstImage. Here the size of
    // the image refers to the virtual size, while Dawn validates texture copy extent with the
    // physical size, so we need to re-calculate the texture copy extent to ensure it should fit
    // in the virtual size of the subresource.
    Extent3D ComputeTextureCopyExtent(const TextureCopy& textureCopy, const Extent3D& copySize) {
        Extent3D validTextureCopyExtent = copySize;
        const TextureBase* texture = textureCopy.texture.Get();
        Extent3D virtualSizeAtLevel = texture->GetMipLevelVirtualSize(textureCopy.mipLevel);
        if (textureCopy.origin.x + copySize.width > virtualSizeAtLevel.width) {
            ASSERT(texture->GetFormat().isCompressed);
            validTextureCopyExtent.width = virtualSizeAtLevel.width - textureCopy.origin.x;
        }
        if (textureCopy.origin.y + copySize.height > virtualSizeAtLevel.height) {
            ASSERT(texture->GetFormat().isCompressed);
            validTextureCopyExtent.height = virtualSizeAtLevel.height - textureCopy.origin.y;
        }

        return validTextureCopyExtent;
    }

    VkBufferImageCopy ComputeBufferImageCopyRegion(const BufferCopy& bufferCopy,
                                                   const TextureCopy& textureCopy,
                                                   const Extent3D& copySize) {
        TextureDataLayout passDataLayout;
        passDataLayout.offset = bufferCopy.offset;
        passDataLayout.rowsPerImage = bufferCopy.rowsPerImage;
        passDataLayout.bytesPerRow = bufferCopy.bytesPerRow;
        return ComputeBufferImageCopyRegion(passDataLayout, textureCopy, copySize);
    }

    VkBufferImageCopy ComputeBufferImageCopyRegion(const TextureDataLayout& dataLayout,
                                                   const TextureCopy& textureCopy,
                                                   const Extent3D& copySize) {
        const Texture* texture = ToBackend(textureCopy.texture.Get());

        VkBufferImageCopy region;

        region.bufferOffset = dataLayout.offset;
        // In Vulkan the row length is in texels while it is in bytes for Dawn
        const Format& format = texture->GetFormat();
        ASSERT(dataLayout.bytesPerRow % format.blockByteSize == 0);
        region.bufferRowLength = dataLayout.bytesPerRow / format.blockByteSize * format.blockWidth;
        region.bufferImageHeight = dataLayout.rowsPerImage;

        region.imageSubresource.aspectMask = texture->GetVkAspectMask();
        region.imageSubresource.mipLevel = textureCopy.mipLevel;

        switch (textureCopy.texture->GetDimension()) {
            case wgpu::TextureDimension::e2D: {
                region.imageOffset.x = textureCopy.origin.x;
                region.imageOffset.y = textureCopy.origin.y;
                region.imageOffset.z = 0;

                region.imageSubresource.baseArrayLayer = textureCopy.origin.z;
                region.imageSubresource.layerCount = copySize.depth;

                Extent3D imageExtent = ComputeTextureCopyExtent(textureCopy, copySize);
                region.imageExtent.width = imageExtent.width;
                region.imageExtent.height = imageExtent.height;
                region.imageExtent.depth = 1;
                break;
            }

            default:
                UNREACHABLE();
                break;
        }

        return region;
    }
}}  // namespace dawn_native::vulkan