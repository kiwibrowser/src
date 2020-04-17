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

#include <gtest/gtest.h>

#include "dawn_native/d3d12/d3d12_platform.h"
#include "dawn_native/d3d12/TextureCopySplitter.h"
#include "common/Assert.h"
#include "common/Constants.h"
#include "common/Math.h"

using namespace dawn_native::d3d12;

namespace {

    struct TextureSpec {
        uint32_t x;
        uint32_t y;
        uint32_t z;
        uint32_t width;
        uint32_t height;
        uint32_t depth;
        uint32_t texelSize;
    };

    struct BufferSpec {
        uint64_t offset;
        uint32_t rowPitch;
        uint32_t imageHeight;
    };

    // Check that each copy region fits inside the buffer footprint
    void ValidateFootprints(const TextureCopySplit& copySplit) {
        for (uint32_t i = 0; i < copySplit.count; ++i) {
            const auto& copy = copySplit.copies[i];
            ASSERT_LE(copy.bufferOffset.x + copy.copySize.width, copy.bufferSize.width);
            ASSERT_LE(copy.bufferOffset.y + copy.copySize.height, copy.bufferSize.height);
            ASSERT_LE(copy.bufferOffset.z + copy.copySize.depth, copy.bufferSize.depth);
        }
    }

    // Check that the offset is aligned
    void ValidateOffset(const TextureCopySplit& copySplit) {
        ASSERT_TRUE(Align(copySplit.offset, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT) == copySplit.offset);
    }

    bool RangesOverlap(uint32_t minA, uint32_t maxA, uint32_t minB, uint32_t maxB) {
        return (minA < minB && minB <= maxA) || (minB < minA && minA <= maxB);
    }

    // Check that no pair of copy regions intersect each other
    void ValidateDisjoint(const TextureCopySplit& copySplit) {
        for (uint32_t i = 0; i < copySplit.count; ++i) {
            const auto& a = copySplit.copies[i];
            for (uint32_t j = i + 1; j < copySplit.count; ++j) {
                const auto& b = copySplit.copies[j];
                bool overlapX = RangesOverlap(a.textureOffset.x, a.textureOffset.x + a.copySize.width, b.textureOffset.x, b.textureOffset.x + b.copySize.width);
                bool overlapY = RangesOverlap(a.textureOffset.y, a.textureOffset.y + a.copySize.height, b.textureOffset.y, b.textureOffset.y + b.copySize.height);
                bool overlapZ = RangesOverlap(a.textureOffset.z, a.textureOffset.z + a.copySize.depth, b.textureOffset.z, b.textureOffset.z + b.copySize.depth);
                ASSERT_TRUE(!overlapX || !overlapY || !overlapZ);
            }
        }
    }

    // Check that the union of the copy regions exactly covers the texture region
    void ValidateTextureBounds(const TextureSpec& textureSpec, const TextureCopySplit& copySplit) {
        ASSERT_TRUE(copySplit.count > 0);

        uint32_t minX = copySplit.copies[0].textureOffset.x;
        uint32_t minY = copySplit.copies[0].textureOffset.y;
        uint32_t minZ = copySplit.copies[0].textureOffset.z;
        uint32_t maxX = copySplit.copies[0].textureOffset.x + copySplit.copies[0].copySize.width;
        uint32_t maxY = copySplit.copies[0].textureOffset.y + copySplit.copies[0].copySize.height;
        uint32_t maxZ = copySplit.copies[0].textureOffset.z + copySplit.copies[0].copySize.depth;

        for (uint32_t i = 1; i < copySplit.count; ++i) {
            const auto& copy = copySplit.copies[i];
            minX = std::min(minX, copy.textureOffset.x);
            minY = std::min(minY, copy.textureOffset.y);
            minZ = std::min(minZ, copy.textureOffset.z);
            maxX = std::max(maxX, copy.textureOffset.x + copy.copySize.width);
            maxY = std::max(maxY, copy.textureOffset.y + copy.copySize.height);
            maxZ = std::max(maxZ, copy.textureOffset.z + copy.copySize.depth);
        }

        ASSERT_EQ(minX, textureSpec.x);
        ASSERT_EQ(minY, textureSpec.y);
        ASSERT_EQ(minZ, textureSpec.z);
        ASSERT_EQ(maxX, textureSpec.x + textureSpec.width);
        ASSERT_EQ(maxY, textureSpec.y + textureSpec.height);
        ASSERT_EQ(maxZ, textureSpec.z + textureSpec.depth);
    }

    // Validate that the number of pixels copied is exactly equal to the number of pixels in the texture region
    void ValidatePixelCount(const TextureSpec& textureSpec, const TextureCopySplit& copySplit) {
        uint32_t count = 0;
        for (uint32_t i = 0; i < copySplit.count; ++i) {
            const auto& copy = copySplit.copies[i];
            count += copy.copySize.width * copy.copySize.height * copy.copySize.depth;
        }
        ASSERT_EQ(count, textureSpec.width * textureSpec.height * textureSpec.depth);
    }

    // Check that every buffer offset is at the correct pixel location
    void ValidateBufferOffset(const TextureSpec& textureSpec, const BufferSpec& bufferSpec, const TextureCopySplit& copySplit) {
        ASSERT_TRUE(copySplit.count > 0);

        for (uint32_t i = 0; i < copySplit.count; ++i) {
            const auto& copy = copySplit.copies[i];

            uint32_t rowPitchInTexels = bufferSpec.rowPitch / textureSpec.texelSize;
            uint32_t slicePitchInTexels = rowPitchInTexels * bufferSpec.imageHeight;
            uint32_t absoluteTexelOffset = copySplit.offset / textureSpec.texelSize + copy.bufferOffset.x + copy.bufferOffset.y * rowPitchInTexels + copy.bufferOffset.z * slicePitchInTexels;

            ASSERT(absoluteTexelOffset >= bufferSpec.offset / textureSpec.texelSize);
            uint32_t relativeTexelOffset = absoluteTexelOffset - bufferSpec.offset / textureSpec.texelSize;

            uint32_t z = relativeTexelOffset / slicePitchInTexels;
            uint32_t y = (relativeTexelOffset % slicePitchInTexels) / rowPitchInTexels;
            uint32_t x = relativeTexelOffset % rowPitchInTexels;

            ASSERT_EQ(copy.textureOffset.x - textureSpec.x, x);
            ASSERT_EQ(copy.textureOffset.y - textureSpec.y, y);
            ASSERT_EQ(copy.textureOffset.z - textureSpec.z, z);
        }
    }

    void ValidateCopySplit(const TextureSpec& textureSpec, const BufferSpec& bufferSpec, const TextureCopySplit& copySplit) {
        ValidateFootprints(copySplit);
        ValidateOffset(copySplit);
        ValidateDisjoint(copySplit);
        ValidateTextureBounds(textureSpec, copySplit);
        ValidatePixelCount(textureSpec, copySplit);
        ValidateBufferOffset(textureSpec, bufferSpec, copySplit);
    }

    std::ostream& operator<<(std::ostream& os, const TextureSpec& textureSpec) {
        os << "TextureSpec("
            << "[(" << textureSpec.x << ", " << textureSpec.y << ", " << textureSpec.z << "), (" << textureSpec.width << ", " << textureSpec.height << ", " << textureSpec.depth << ")], "
            << textureSpec.texelSize
            << ")";
        return os;
    }

    std::ostream& operator<<(std::ostream& os, const BufferSpec& bufferSpec) {
        os << "BufferSpec(" << bufferSpec.offset << ", " << bufferSpec.rowPitch << ", "
           << bufferSpec.imageHeight << ")";
        return os;
    }

    std::ostream& operator<<(std::ostream& os, const TextureCopySplit& copySplit) {
        os << "CopySplit" << std::endl;
        for (uint32_t i = 0; i < copySplit.count; ++i) {
            const auto& copy = copySplit.copies[i];
            os << "  " << i << ": Texture at (" << copy.textureOffset.x << ", " << copy.textureOffset.y << ", " << copy.textureOffset.z << "), size (" << copy.copySize.width << ", " << copy.copySize.height << ", " << copy.copySize.depth << ")" << std::endl;
            os << "  " << i << ": Buffer at (" << copy.bufferOffset.x << ", " << copy.bufferOffset.y << ", " << copy.bufferOffset.z << "), footprint (" << copy.bufferSize.width << ", " << copy.bufferSize.height << ", " << copy.bufferSize.depth << ")" << std::endl;
        }
        return os;
    }

    // Define base texture sizes and offsets to test with: some aligned, some unaligned
    constexpr TextureSpec kBaseTextureSpecs[] = {
        {0, 0, 0, 1, 1, 1, 4},
        {31, 16, 0, 1, 1, 1, 4},
        {64, 16, 0, 1, 1, 1, 4},
        {64, 16, 8, 1, 1, 1, 4},

        {0, 0, 0, 1024, 1024, 1, 4},
        {256, 512, 0, 1024, 1024, 1, 4},
        {64, 48, 0, 1024, 1024, 1, 4},
        {64, 48, 16, 1024, 1024, 1024, 4},

        {0, 0, 0, 257, 31, 1, 4},
        {0, 0, 0, 17, 93, 1, 4},
        {59, 13, 0, 257, 31, 1, 4},
        {17, 73, 0, 17, 93, 1, 4},
        {17, 73, 59, 17, 93, 99, 4},
    };

    // Define base buffer sizes to work with: some offsets aligned, some unaligned. rowPitch is the minimum required
    std::array<BufferSpec, 13> BaseBufferSpecs(const TextureSpec& textureSpec) {
        uint32_t rowPitch = Align(textureSpec.texelSize * textureSpec.width, kTextureRowPitchAlignment);

        auto alignNonPow2 = [](uint32_t value, uint32_t size) -> uint32_t {
            return value == 0 ? 0 : ((value - 1) / size + 1) * size;
        };

        return {
            BufferSpec{alignNonPow2(0, textureSpec.texelSize), rowPitch, textureSpec.height},
            BufferSpec{alignNonPow2(512, textureSpec.texelSize), rowPitch, textureSpec.height},
            BufferSpec{alignNonPow2(1024, textureSpec.texelSize), rowPitch, textureSpec.height},
            BufferSpec{alignNonPow2(1024, textureSpec.texelSize), rowPitch, textureSpec.height * 2},

            BufferSpec{alignNonPow2(32, textureSpec.texelSize), rowPitch, textureSpec.height},
            BufferSpec{alignNonPow2(64, textureSpec.texelSize), rowPitch, textureSpec.height},
            BufferSpec{alignNonPow2(64, textureSpec.texelSize), rowPitch, textureSpec.height * 2},

            BufferSpec{alignNonPow2(31, textureSpec.texelSize), rowPitch, textureSpec.height},
            BufferSpec{alignNonPow2(257, textureSpec.texelSize), rowPitch, textureSpec.height},
            BufferSpec{alignNonPow2(511, textureSpec.texelSize), rowPitch, textureSpec.height},
            BufferSpec{alignNonPow2(513, textureSpec.texelSize), rowPitch, textureSpec.height},
            BufferSpec{alignNonPow2(1023, textureSpec.texelSize), rowPitch, textureSpec.height},
            BufferSpec{alignNonPow2(1023, textureSpec.texelSize), rowPitch, textureSpec.height * 2},
        };
    }

    // Define a list of values to set properties in the spec structs
    constexpr uint32_t kCheckValues[] = {
        1, 2, 3, 4, 5, 6, 7, 8,                // small values
        16, 32, 64, 128, 256, 512, 1024, 2048, // powers of 2
        15, 31, 63, 127, 257, 511, 1023, 2047, // misalignments
        17, 33, 65, 129, 257, 513, 1025, 2049
    };

}

class CopySplitTest : public testing::Test {
    protected:
        TextureCopySplit DoTest(const TextureSpec& textureSpec, const BufferSpec& bufferSpec) {
            TextureCopySplit copySplit = ComputeTextureCopySplit(
                {textureSpec.x, textureSpec.y, textureSpec.z},
                {textureSpec.width, textureSpec.height, textureSpec.depth}, textureSpec.texelSize,
                bufferSpec.offset, bufferSpec.rowPitch, bufferSpec.imageHeight);
            ValidateCopySplit(textureSpec, bufferSpec, copySplit);
            return copySplit;
        }
};

TEST_F(CopySplitTest, General) {
    for (TextureSpec textureSpec : kBaseTextureSpecs) {
        for (BufferSpec bufferSpec : BaseBufferSpecs(textureSpec)) {

            TextureCopySplit copySplit = DoTest(textureSpec, bufferSpec);
            if (HasFatalFailure()) {
                std::ostringstream message;
                message << "Failed generating splits: " << textureSpec << ", " << bufferSpec << std::endl
                    << copySplit << std::endl;
                FAIL() << message.str();
            }
        }
    }
}

TEST_F(CopySplitTest, TextureWidth) {
    for (TextureSpec textureSpec : kBaseTextureSpecs) {
        for (uint32_t val : kCheckValues) {
            textureSpec.width = val;
            for (BufferSpec bufferSpec : BaseBufferSpecs(textureSpec)) {

                TextureCopySplit copySplit = DoTest(textureSpec, bufferSpec);
                if (HasFatalFailure()) {
                    std::ostringstream message;
                    message << "Failed generating splits: " << textureSpec << ", " << bufferSpec << std::endl
                        << copySplit << std::endl;
                    FAIL() << message.str();
                }
            }
        }
    }
}

TEST_F(CopySplitTest, TextureHeight) {
    for (TextureSpec textureSpec : kBaseTextureSpecs) {
        for (uint32_t val : kCheckValues) {
            textureSpec.height = val;
            for (BufferSpec bufferSpec : BaseBufferSpecs(textureSpec)) {

                TextureCopySplit copySplit = DoTest(textureSpec, bufferSpec);
                if (HasFatalFailure()) {
                    std::ostringstream message;
                    message << "Failed generating splits: " << textureSpec << ", " << bufferSpec << std::endl
                        << copySplit << std::endl;
                    FAIL() << message.str();
                }
            }
        }
    }
}

TEST_F(CopySplitTest, TextureX) {
    for (TextureSpec textureSpec : kBaseTextureSpecs) {
        for (uint32_t val : kCheckValues) {
            textureSpec.x = val;
            for (BufferSpec bufferSpec : BaseBufferSpecs(textureSpec)) {

                TextureCopySplit copySplit = DoTest(textureSpec, bufferSpec);
                if (HasFatalFailure()) {
                    std::ostringstream message;
                    message << "Failed generating splits: " << textureSpec << ", " << bufferSpec << std::endl
                        << copySplit << std::endl;
                    FAIL() << message.str();
                }
            }
        }
    }
}

TEST_F(CopySplitTest, TextureY) {
    for (TextureSpec textureSpec : kBaseTextureSpecs) {
        for (uint32_t val : kCheckValues) {
            textureSpec.y = val;
            for (BufferSpec bufferSpec : BaseBufferSpecs(textureSpec)) {

                TextureCopySplit copySplit = DoTest(textureSpec, bufferSpec);
                if (HasFatalFailure()) {
                    std::ostringstream message;
                    message << "Failed generating splits: " << textureSpec << ", " << bufferSpec << std::endl
                        << copySplit << std::endl;
                    FAIL() << message.str();
                }
            }
        }
    }
}

TEST_F(CopySplitTest, TexelSize) {
    for (TextureSpec textureSpec : kBaseTextureSpecs) {
        for (uint32_t texelSize : {4, 8, 16, 32, 64}) {
            textureSpec.texelSize = texelSize;
            for (BufferSpec bufferSpec : BaseBufferSpecs(textureSpec)) {

                TextureCopySplit copySplit = DoTest(textureSpec, bufferSpec);
                if (HasFatalFailure()) {
                    std::ostringstream message;
                    message << "Failed generating splits: " << textureSpec << ", " << bufferSpec << std::endl
                        << copySplit << std::endl;
                    FAIL() << message.str();
                }
            }
        }
    }
}

TEST_F(CopySplitTest, BufferOffset) {
    for (TextureSpec textureSpec : kBaseTextureSpecs) {
        for (BufferSpec bufferSpec : BaseBufferSpecs(textureSpec)) {
            for (uint32_t val : kCheckValues) {
                bufferSpec.offset = textureSpec.texelSize * val;

                TextureCopySplit copySplit = DoTest(textureSpec, bufferSpec);
                if (HasFatalFailure()) {
                    std::ostringstream message;
                    message << "Failed generating splits: " << textureSpec << ", " << bufferSpec << std::endl
                        << copySplit << std::endl;
                    FAIL() << message.str();
                }
            }
        }
    }
}

TEST_F(CopySplitTest, RowPitch) {
    for (TextureSpec textureSpec : kBaseTextureSpecs) {
        for (BufferSpec bufferSpec : BaseBufferSpecs(textureSpec)) {
            uint32_t baseRowPitch = bufferSpec.rowPitch;
            for (uint32_t i = 0; i < 5; ++i) {
                bufferSpec.rowPitch = baseRowPitch + i * 256;

                TextureCopySplit copySplit = DoTest(textureSpec, bufferSpec);
                if (HasFatalFailure()) {
                    std::ostringstream message;
                    message << "Failed generating splits: " << textureSpec << ", " << bufferSpec << std::endl
                        << copySplit << std::endl;
                    FAIL() << message.str();
                }
            }
        }
    }
}

TEST_F(CopySplitTest, ImageHeight) {
    for (TextureSpec textureSpec : kBaseTextureSpecs) {
        for (BufferSpec bufferSpec : BaseBufferSpecs(textureSpec)) {
            uint32_t baseImageHeight = bufferSpec.imageHeight;
            for (uint32_t i = 0; i < 5; ++i) {
                bufferSpec.imageHeight = baseImageHeight + i * 256;

                TextureCopySplit copySplit = DoTest(textureSpec, bufferSpec);
                if (HasFatalFailure()) {
                    std::ostringstream message;
                    message << "Failed generating splits: " << textureSpec << ", " << bufferSpec
                            << std::endl
                            << copySplit << std::endl;
                    FAIL() << message.str();
                }
            }
        }
    }
}
