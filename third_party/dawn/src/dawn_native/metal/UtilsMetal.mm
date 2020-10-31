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

#include "dawn_native/metal/UtilsMetal.h"
#include "dawn_native/CommandBuffer.h"

#include "common/Assert.h"

namespace dawn_native { namespace metal {

    MTLCompareFunction ToMetalCompareFunction(wgpu::CompareFunction compareFunction) {
        switch (compareFunction) {
            case wgpu::CompareFunction::Never:
                return MTLCompareFunctionNever;
            case wgpu::CompareFunction::Less:
                return MTLCompareFunctionLess;
            case wgpu::CompareFunction::LessEqual:
                return MTLCompareFunctionLessEqual;
            case wgpu::CompareFunction::Greater:
                return MTLCompareFunctionGreater;
            case wgpu::CompareFunction::GreaterEqual:
                return MTLCompareFunctionGreaterEqual;
            case wgpu::CompareFunction::NotEqual:
                return MTLCompareFunctionNotEqual;
            case wgpu::CompareFunction::Equal:
                return MTLCompareFunctionEqual;
            case wgpu::CompareFunction::Always:
                return MTLCompareFunctionAlways;
            default:
                UNREACHABLE();
        }
    }

    TextureBufferCopySplit ComputeTextureBufferCopySplit(const Texture* texture,
                                                         uint32_t mipLevel,
                                                         Origin3D origin,
                                                         Extent3D copyExtent,
                                                         uint64_t bufferSize,
                                                         uint64_t bufferOffset,
                                                         uint32_t bytesPerRow,
                                                         uint32_t rowsPerImage) {
        TextureBufferCopySplit copy;
        const Format textureFormat = texture->GetFormat();

        // When copying textures from/to an unpacked buffer, the Metal validation layer doesn't
        // compute the correct range when checking if the buffer is big enough to contain the
        // data for the whole copy. Instead of looking at the position of the last texel in the
        // buffer, it computes the volume of the 3D box with bytesPerRow * (rowsPerImage /
        // format.blockHeight) * copySize.depth. For example considering the pixel buffer below
        // where in memory, each row data (D) of the texture is followed by some padding data
        // (P):
        //     |DDDDDDD|PP|
        //     |DDDDDDD|PP|
        //     |DDDDDDD|PP|
        //     |DDDDDDD|PP|
        //     |DDDDDDA|PP|
        // The last pixel read will be A, but the driver will think it is the whole last padding
        // row, causing it to generate an error when the pixel buffer is just big enough.

        // We work around this limitation by detecting when Metal would complain and copy the
        // last image and row separately using tight sourceBytesPerRow or sourceBytesPerImage.
        uint32_t dataRowsPerImage = rowsPerImage / textureFormat.blockHeight;
        uint32_t bytesPerImage = bytesPerRow * dataRowsPerImage;

        // Metal validation layer requires that if the texture's pixel format is a compressed
        // format, the sourceSize must be a multiple of the pixel format's block size or be
        // clamped to the edge of the texture if the block extends outside the bounds of a
        // texture.
        const Extent3D clampedCopyExtent =
            texture->ClampToMipLevelVirtualSize(mipLevel, origin, copyExtent);

        ASSERT(texture->GetDimension() == wgpu::TextureDimension::e2D);

        // Check whether buffer size is big enough.
        bool needWorkaround = bufferSize - bufferOffset < bytesPerImage * copyExtent.depth;
        if (!needWorkaround) {
            copy.count = 1;
            copy.copies[0].bufferOffset = bufferOffset;
            copy.copies[0].bytesPerRow = bytesPerRow;
            copy.copies[0].bytesPerImage = bytesPerImage;
            copy.copies[0].textureOrigin = origin;
            copy.copies[0].copyExtent = {clampedCopyExtent.width, clampedCopyExtent.height,
                                         copyExtent.depth};
            return copy;
        }

        uint64_t currentOffset = bufferOffset;

        // Doing all the copy except the last image.
        if (copyExtent.depth > 1) {
            copy.copies[copy.count].bufferOffset = currentOffset;
            copy.copies[copy.count].bytesPerRow = bytesPerRow;
            copy.copies[copy.count].bytesPerImage = bytesPerImage;
            copy.copies[copy.count].textureOrigin = origin;
            copy.copies[copy.count].copyExtent = {clampedCopyExtent.width, clampedCopyExtent.height,
                                                  copyExtent.depth - 1};

            ++copy.count;

            // Update offset to copy to the last image.
            currentOffset += (copyExtent.depth - 1) * bytesPerImage;
        }

        // Doing all the copy in last image except the last row.
        uint32_t copyBlockRowCount = copyExtent.height / textureFormat.blockHeight;
        if (copyBlockRowCount > 1) {
            copy.copies[copy.count].bufferOffset = currentOffset;
            copy.copies[copy.count].bytesPerRow = bytesPerRow;
            copy.copies[copy.count].bytesPerImage = bytesPerRow * (copyBlockRowCount - 1);
            copy.copies[copy.count].textureOrigin = {origin.x, origin.y,
                                                     origin.z + copyExtent.depth - 1};

            ASSERT(copyExtent.height - textureFormat.blockHeight <
                   texture->GetMipLevelVirtualSize(mipLevel).height);
            copy.copies[copy.count].copyExtent = {clampedCopyExtent.width,
                                                  copyExtent.height - textureFormat.blockHeight, 1};

            ++copy.count;

            // Update offset to copy to the last row.
            currentOffset += (copyBlockRowCount - 1) * bytesPerRow;
        }

        // Doing the last row copy with the exact number of bytes in last row.
        // Workaround this issue in a way just like the copy to a 1D texture.
        uint32_t lastRowDataSize =
            (copyExtent.width / textureFormat.blockWidth) * textureFormat.blockByteSize;
        uint32_t lastRowCopyExtentHeight =
            textureFormat.blockHeight + clampedCopyExtent.height - copyExtent.height;
        ASSERT(lastRowCopyExtentHeight <= textureFormat.blockHeight);

        copy.copies[copy.count].bufferOffset = currentOffset;
        copy.copies[copy.count].bytesPerRow = lastRowDataSize;
        copy.copies[copy.count].bytesPerImage = lastRowDataSize;
        copy.copies[copy.count].textureOrigin = {
            origin.x, origin.y + copyExtent.height - textureFormat.blockHeight,
            origin.z + copyExtent.depth - 1};
        copy.copies[copy.count].copyExtent = {clampedCopyExtent.width, lastRowCopyExtentHeight, 1};
        ++copy.count;

        return copy;
    }

    void EnsureDestinationTextureInitialized(Texture* texture,
                                             const TextureCopy& dst,
                                             const Extent3D& size) {
        ASSERT(texture == dst.texture.Get());
        SubresourceRange range = GetSubresourcesAffectedByCopy(dst, size);
        if (IsCompleteSubresourceCopiedTo(dst.texture.Get(), size, dst.mipLevel)) {
            texture->SetIsSubresourceContentInitialized(true, range);
        } else {
            texture->EnsureSubresourceContentInitialized(range);
        }
    }

}}  // namespace dawn_native::metal
