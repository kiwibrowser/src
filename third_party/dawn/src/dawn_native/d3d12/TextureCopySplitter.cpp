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

#include "dawn_native/d3d12/TextureCopySplitter.h"

#include "common/Assert.h"
#include "dawn_native/Format.h"
#include "dawn_native/d3d12/d3d12_platform.h"

namespace dawn_native { namespace d3d12 {

    namespace {
        Origin3D ComputeTexelOffsets(const Format& format,
                                     uint32_t offset,
                                     uint32_t bytesPerRow,
                                     uint32_t slicePitch) {
            ASSERT(bytesPerRow != 0);
            ASSERT(slicePitch != 0);
            uint32_t byteOffsetX = offset % bytesPerRow;
            offset -= byteOffsetX;
            uint32_t byteOffsetY = offset % slicePitch;
            uint32_t byteOffsetZ = offset - byteOffsetY;

            return {byteOffsetX / format.blockByteSize * format.blockWidth,
                    byteOffsetY / bytesPerRow * format.blockHeight, byteOffsetZ / slicePitch};
        }
    }  // namespace

    Texture2DCopySplit ComputeTextureCopySplit(Origin3D origin,
                                               Extent3D copySize,
                                               const Format& format,
                                               uint64_t offset,
                                               uint32_t bytesPerRow,
                                               uint32_t rowsPerImage) {
        Texture2DCopySplit copy;

        ASSERT(bytesPerRow % format.blockByteSize == 0);

        uint64_t alignedOffset =
            offset & ~static_cast<uint64_t>(D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT - 1);

        copy.offset = alignedOffset;
        if (offset == alignedOffset) {
            copy.count = 1;

            copy.copies[0].textureOffset = origin;

            copy.copies[0].copySize = copySize;

            copy.copies[0].bufferOffset.x = 0;
            copy.copies[0].bufferOffset.y = 0;
            copy.copies[0].bufferOffset.z = 0;
            copy.copies[0].bufferSize = copySize;

            // Return early. There is only one copy needed because the offset is already 512-byte
            // aligned
            return copy;
        }

        ASSERT(alignedOffset < offset);
        ASSERT(offset - alignedOffset < D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

        uint32_t slicePitch = bytesPerRow * (rowsPerImage / format.blockHeight);
        Origin3D texelOffset = ComputeTexelOffsets(
            format, static_cast<uint32_t>(offset - alignedOffset), bytesPerRow, slicePitch);

        uint32_t copyBytesPerRowPitch = copySize.width / format.blockWidth * format.blockByteSize;
        uint32_t byteOffsetInRowPitch = texelOffset.x / format.blockWidth * format.blockByteSize;
        if (copyBytesPerRowPitch + byteOffsetInRowPitch <= bytesPerRow) {
            // The region's rows fit inside the bytes per row. In this case, extend the width of the
            // PlacedFootprint and copy the buffer with an offset location
            //  |<------------- bytes per row ------------->|
            //
            //  |-------------------------------------------|
            //  |                                           |
            //  |                 +++++++++++++++++~~~~~~~~~|
            //  |~~~~~~~~~~~~~~~~~+++++++++++++++++~~~~~~~~~|
            //  |~~~~~~~~~~~~~~~~~+++++++++++++++++~~~~~~~~~|
            //  |~~~~~~~~~~~~~~~~~+++++++++++++++++~~~~~~~~~|
            //  |~~~~~~~~~~~~~~~~~+++++++++++++++++         |
            //  |-------------------------------------------|

            // Copy 0:
            //  |----------------------------------|
            //  |                                  |
            //  |                 +++++++++++++++++|
            //  |~~~~~~~~~~~~~~~~~+++++++++++++++++|
            //  |~~~~~~~~~~~~~~~~~+++++++++++++++++|
            //  |~~~~~~~~~~~~~~~~~+++++++++++++++++|
            //  |~~~~~~~~~~~~~~~~~+++++++++++++++++|
            //  |----------------------------------|

            copy.count = 1;

            copy.copies[0].textureOffset = origin;

            copy.copies[0].copySize = copySize;

            copy.copies[0].bufferOffset = texelOffset;
            copy.copies[0].bufferSize.width = copySize.width + texelOffset.x;
            copy.copies[0].bufferSize.height = rowsPerImage + texelOffset.y;
            copy.copies[0].bufferSize.depth = copySize.depth + texelOffset.z;

            return copy;
        }

        // The region's rows straddle the bytes per row. Split the copy into two copies
        //  |<------------- bytes per row ------------->|
        //
        //  |-------------------------------------------|
        //  |                                           |
        //  |                                   ++++++++|
        //  |+++++++++~~~~~~~~~~~~~~~~~~~~~~~~~~++++++++|
        //  |+++++++++~~~~~~~~~~~~~~~~~~~~~~~~~~++++++++|
        //  |+++++++++~~~~~~~~~~~~~~~~~~~~~~~~~~++++++++|
        //  |+++++++++~~~~~~~~~~~~~~~~~~~~~~~~~~++++++++|
        //  |+++++++++                                  |
        //  |-------------------------------------------|

        //  Copy 0:
        //  |-------------------------------------------|
        //  |                                           |
        //  |                                   ++++++++|
        //  |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~++++++++|
        //  |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~++++++++|
        //  |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~++++++++|
        //  |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~++++++++|
        //  |-------------------------------------------|

        //  Copy 1:
        //  |---------|
        //  |         |
        //  |         |
        //  |+++++++++|
        //  |+++++++++|
        //  |+++++++++|
        //  |+++++++++|
        //  |+++++++++|
        //  |---------|

        copy.count = 2;

        copy.copies[0].textureOffset = origin;

        ASSERT(bytesPerRow > byteOffsetInRowPitch);
        uint32_t texelsPerRow = bytesPerRow / format.blockByteSize * format.blockWidth;
        copy.copies[0].copySize.width = texelsPerRow - texelOffset.x;
        copy.copies[0].copySize.height = copySize.height;
        copy.copies[0].copySize.depth = copySize.depth;

        copy.copies[0].bufferOffset = texelOffset;
        copy.copies[0].bufferSize.width = texelsPerRow;
        copy.copies[0].bufferSize.height = rowsPerImage + texelOffset.y;
        copy.copies[0].bufferSize.depth = copySize.depth + texelOffset.z;

        copy.copies[1].textureOffset.x = origin.x + copy.copies[0].copySize.width;
        copy.copies[1].textureOffset.y = origin.y;
        copy.copies[1].textureOffset.z = origin.z;

        ASSERT(copySize.width > copy.copies[0].copySize.width);
        copy.copies[1].copySize.width = copySize.width - copy.copies[0].copySize.width;
        copy.copies[1].copySize.height = copySize.height;
        copy.copies[1].copySize.depth = copySize.depth;

        copy.copies[1].bufferOffset.x = 0;
        copy.copies[1].bufferOffset.y = texelOffset.y + format.blockHeight;
        copy.copies[1].bufferOffset.z = texelOffset.z;
        copy.copies[1].bufferSize.width = copy.copies[1].copySize.width;
        copy.copies[1].bufferSize.height = rowsPerImage + texelOffset.y + format.blockHeight;
        copy.copies[1].bufferSize.depth = copySize.depth + texelOffset.z;

        return copy;
    }

    TextureCopySplits ComputeTextureCopySplits(Origin3D origin,
                                               Extent3D copySize,
                                               const Format& format,
                                               uint64_t offset,
                                               uint32_t bytesPerRow,
                                               uint32_t rowsPerImage) {
        TextureCopySplits copies;

        const uint64_t bytesPerSlice = bytesPerRow * (rowsPerImage / format.blockHeight);

        // The function ComputeTextureCopySplit() decides how to split the copy based on:
        // - the alignment of the buffer offset with D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT (512)
        // - the alignment of the buffer offset with D3D12_TEXTURE_DATA_PITCH_ALIGNMENT (256)
        // Each slice of a 2D array or 3D copy might need to be split, but because of the WebGPU
        // constraint that "bytesPerRow" must be a multiple of 256, all odd (resp. all even) slices
        // will be at an offset multiple of 512 of each other, which means they will all result in
        // the same 2D split. Thus we can just compute the copy splits for the first and second
        // slices, and reuse them for the remaining slices by adding the related offset of each
        // slice. Moreover, if "rowsPerImage" is even, both the first and second copy layers can
        // share the same copy split, so in this situation we just need to compute copy split once
        // and reuse it for all the slices.
        const dawn_native::Extent3D copyOneLayerSize = {copySize.width, copySize.height, 1};
        const dawn_native::Origin3D copyFirstLayerOrigin = {origin.x, origin.y, 0};

        copies.copies2D[0] = ComputeTextureCopySplit(copyFirstLayerOrigin, copyOneLayerSize, format,
                                                     offset, bytesPerRow, rowsPerImage);

        // When the copy only refers one texture 2D array layer copies.copies2D[1] will never be
        // used so we can safely early return here.
        if (copySize.depth == 1) {
            return copies;
        }

        if (bytesPerSlice % D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT == 0) {
            copies.copies2D[1] = copies.copies2D[0];
            copies.copies2D[1].offset += bytesPerSlice;
        } else {
            const uint64_t bufferOffsetNextLayer = offset + bytesPerSlice;
            copies.copies2D[1] =
                ComputeTextureCopySplit(copyFirstLayerOrigin, copyOneLayerSize, format,
                                        bufferOffsetNextLayer, bytesPerRow, rowsPerImage);
        }

        return copies;
    }

}}  // namespace dawn_native::d3d12
