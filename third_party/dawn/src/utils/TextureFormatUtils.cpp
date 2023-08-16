// Copyright 2020 The Dawn Authors
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

#include "TextureFormatUtils.h"

namespace utils {
    const char* GetColorTextureComponentTypePrefix(wgpu::TextureFormat textureFormat) {
        switch (textureFormat) {
            case wgpu::TextureFormat::R8Unorm:
            case wgpu::TextureFormat::R8Snorm:
            case wgpu::TextureFormat::R16Float:
            case wgpu::TextureFormat::RG8Unorm:
            case wgpu::TextureFormat::RG8Snorm:
            case wgpu::TextureFormat::R32Float:
            case wgpu::TextureFormat::RG16Float:
            case wgpu::TextureFormat::RGBA8Unorm:
            case wgpu::TextureFormat::RGBA8Snorm:
            case wgpu::TextureFormat::RGB10A2Unorm:
            case wgpu::TextureFormat::RG11B10Float:
            case wgpu::TextureFormat::RG32Float:
            case wgpu::TextureFormat::RGBA16Float:
            case wgpu::TextureFormat::RGBA32Float:
            case wgpu::TextureFormat::BGRA8Unorm:
            case wgpu::TextureFormat::BGRA8UnormSrgb:
            case wgpu::TextureFormat::RGBA8UnormSrgb:
                return "";

            case wgpu::TextureFormat::R8Uint:
            case wgpu::TextureFormat::R16Uint:
            case wgpu::TextureFormat::RG8Uint:
            case wgpu::TextureFormat::R32Uint:
            case wgpu::TextureFormat::RG16Uint:
            case wgpu::TextureFormat::RGBA8Uint:
            case wgpu::TextureFormat::RG32Uint:
            case wgpu::TextureFormat::RGBA16Uint:
            case wgpu::TextureFormat::RGBA32Uint:
                return "u";

            case wgpu::TextureFormat::R8Sint:
            case wgpu::TextureFormat::R16Sint:
            case wgpu::TextureFormat::RG8Sint:
            case wgpu::TextureFormat::R32Sint:
            case wgpu::TextureFormat::RG16Sint:
            case wgpu::TextureFormat::RGBA8Sint:
            case wgpu::TextureFormat::RG32Sint:
            case wgpu::TextureFormat::RGBA16Sint:
            case wgpu::TextureFormat::RGBA32Sint:
                return "i";
            default:
                UNREACHABLE();
                return "";
        }
    }

    bool TextureFormatSupportsStorageTexture(wgpu::TextureFormat format) {
        switch (format) {
            case wgpu::TextureFormat::R32Uint:
            case wgpu::TextureFormat::R32Sint:
            case wgpu::TextureFormat::R32Float:
            case wgpu::TextureFormat::RGBA8Unorm:
            case wgpu::TextureFormat::RGBA8Snorm:
            case wgpu::TextureFormat::RGBA8Uint:
            case wgpu::TextureFormat::RGBA8Sint:
            case wgpu::TextureFormat::RG32Uint:
            case wgpu::TextureFormat::RG32Sint:
            case wgpu::TextureFormat::RG32Float:
            case wgpu::TextureFormat::RGBA16Uint:
            case wgpu::TextureFormat::RGBA16Sint:
            case wgpu::TextureFormat::RGBA16Float:
            case wgpu::TextureFormat::RGBA32Uint:
            case wgpu::TextureFormat::RGBA32Sint:
            case wgpu::TextureFormat::RGBA32Float:
                return true;
            default:
                return false;
        }
    }

    uint32_t GetTexelBlockSizeInBytes(wgpu::TextureFormat textureFormat) {
        switch (textureFormat) {
            case wgpu::TextureFormat::R8Unorm:
            case wgpu::TextureFormat::R8Snorm:
            case wgpu::TextureFormat::R8Uint:
            case wgpu::TextureFormat::R8Sint:
                return 1u;

            case wgpu::TextureFormat::R16Uint:
            case wgpu::TextureFormat::R16Sint:
            case wgpu::TextureFormat::R16Float:
            case wgpu::TextureFormat::RG8Unorm:
            case wgpu::TextureFormat::RG8Snorm:
            case wgpu::TextureFormat::RG8Uint:
            case wgpu::TextureFormat::RG8Sint:
                return 2u;

            case wgpu::TextureFormat::R32Float:
            case wgpu::TextureFormat::R32Uint:
            case wgpu::TextureFormat::R32Sint:
            case wgpu::TextureFormat::RG16Uint:
            case wgpu::TextureFormat::RG16Sint:
            case wgpu::TextureFormat::RG16Float:
            case wgpu::TextureFormat::RGBA8Unorm:
            case wgpu::TextureFormat::RGBA8UnormSrgb:
            case wgpu::TextureFormat::RGBA8Snorm:
            case wgpu::TextureFormat::RGBA8Uint:
            case wgpu::TextureFormat::RGBA8Sint:
            case wgpu::TextureFormat::BGRA8Unorm:
            case wgpu::TextureFormat::BGRA8UnormSrgb:
            case wgpu::TextureFormat::RGB10A2Unorm:
            case wgpu::TextureFormat::RG11B10Float:
            case wgpu::TextureFormat::Depth32Float:
                return 4u;

            case wgpu::TextureFormat::RG32Float:
            case wgpu::TextureFormat::RG32Uint:
            case wgpu::TextureFormat::RG32Sint:
            case wgpu::TextureFormat::RGBA16Uint:
            case wgpu::TextureFormat::RGBA16Sint:
            case wgpu::TextureFormat::RGBA16Float:
                return 8u;

            case wgpu::TextureFormat::RGBA32Float:
            case wgpu::TextureFormat::RGBA32Uint:
            case wgpu::TextureFormat::RGBA32Sint:
                return 16u;

            case wgpu::TextureFormat::BC1RGBAUnorm:
            case wgpu::TextureFormat::BC1RGBAUnormSrgb:
            case wgpu::TextureFormat::BC4RUnorm:
            case wgpu::TextureFormat::BC4RSnorm:
                return 8u;

            case wgpu::TextureFormat::BC2RGBAUnorm:
            case wgpu::TextureFormat::BC2RGBAUnormSrgb:
            case wgpu::TextureFormat::BC3RGBAUnorm:
            case wgpu::TextureFormat::BC3RGBAUnormSrgb:
            case wgpu::TextureFormat::BC5RGUnorm:
            case wgpu::TextureFormat::BC5RGSnorm:
            case wgpu::TextureFormat::BC6HRGBUfloat:
            case wgpu::TextureFormat::BC6HRGBSfloat:
            case wgpu::TextureFormat::BC7RGBAUnorm:
            case wgpu::TextureFormat::BC7RGBAUnormSrgb:
                return 16u;

            case wgpu::TextureFormat::Depth24Plus:
            case wgpu::TextureFormat::Depth24PlusStencil8:
            case wgpu::TextureFormat::Undefined:
            default:
                UNREACHABLE();
                return 0u;
        }
    }

    uint32_t GetTextureFormatBlockWidth(wgpu::TextureFormat textureFormat) {
        switch (textureFormat) {
            case wgpu::TextureFormat::R8Unorm:
            case wgpu::TextureFormat::R8Snorm:
            case wgpu::TextureFormat::R8Uint:
            case wgpu::TextureFormat::R8Sint:
            case wgpu::TextureFormat::R16Uint:
            case wgpu::TextureFormat::R16Sint:
            case wgpu::TextureFormat::R16Float:
            case wgpu::TextureFormat::RG8Unorm:
            case wgpu::TextureFormat::RG8Snorm:
            case wgpu::TextureFormat::RG8Uint:
            case wgpu::TextureFormat::RG8Sint:
            case wgpu::TextureFormat::R32Float:
            case wgpu::TextureFormat::R32Uint:
            case wgpu::TextureFormat::R32Sint:
            case wgpu::TextureFormat::RG16Uint:
            case wgpu::TextureFormat::RG16Sint:
            case wgpu::TextureFormat::RG16Float:
            case wgpu::TextureFormat::RGBA8Unorm:
            case wgpu::TextureFormat::RGBA8UnormSrgb:
            case wgpu::TextureFormat::RGBA8Snorm:
            case wgpu::TextureFormat::RGBA8Uint:
            case wgpu::TextureFormat::RGBA8Sint:
            case wgpu::TextureFormat::BGRA8Unorm:
            case wgpu::TextureFormat::BGRA8UnormSrgb:
            case wgpu::TextureFormat::RGB10A2Unorm:
            case wgpu::TextureFormat::RG11B10Float:
            case wgpu::TextureFormat::RG32Float:
            case wgpu::TextureFormat::RG32Uint:
            case wgpu::TextureFormat::RG32Sint:
            case wgpu::TextureFormat::RGBA16Uint:
            case wgpu::TextureFormat::RGBA16Sint:
            case wgpu::TextureFormat::RGBA16Float:
            case wgpu::TextureFormat::RGBA32Float:
            case wgpu::TextureFormat::RGBA32Uint:
            case wgpu::TextureFormat::RGBA32Sint:
            case wgpu::TextureFormat::Depth32Float:
            case wgpu::TextureFormat::Depth24Plus:
            case wgpu::TextureFormat::Depth24PlusStencil8:
                return 1u;

            case wgpu::TextureFormat::BC1RGBAUnorm:
            case wgpu::TextureFormat::BC1RGBAUnormSrgb:
            case wgpu::TextureFormat::BC4RUnorm:
            case wgpu::TextureFormat::BC4RSnorm:
            case wgpu::TextureFormat::BC2RGBAUnorm:
            case wgpu::TextureFormat::BC2RGBAUnormSrgb:
            case wgpu::TextureFormat::BC3RGBAUnorm:
            case wgpu::TextureFormat::BC3RGBAUnormSrgb:
            case wgpu::TextureFormat::BC5RGUnorm:
            case wgpu::TextureFormat::BC5RGSnorm:
            case wgpu::TextureFormat::BC6HRGBUfloat:
            case wgpu::TextureFormat::BC6HRGBSfloat:
            case wgpu::TextureFormat::BC7RGBAUnorm:
            case wgpu::TextureFormat::BC7RGBAUnormSrgb:
                return 4u;

            case wgpu::TextureFormat::Undefined:
            default:
                UNREACHABLE();
                return 0u;
        }
    }

    uint32_t GetTextureFormatBlockHeight(wgpu::TextureFormat textureFormat) {
        switch (textureFormat) {
            case wgpu::TextureFormat::R8Unorm:
            case wgpu::TextureFormat::R8Snorm:
            case wgpu::TextureFormat::R8Uint:
            case wgpu::TextureFormat::R8Sint:
            case wgpu::TextureFormat::R16Uint:
            case wgpu::TextureFormat::R16Sint:
            case wgpu::TextureFormat::R16Float:
            case wgpu::TextureFormat::RG8Unorm:
            case wgpu::TextureFormat::RG8Snorm:
            case wgpu::TextureFormat::RG8Uint:
            case wgpu::TextureFormat::RG8Sint:
            case wgpu::TextureFormat::R32Float:
            case wgpu::TextureFormat::R32Uint:
            case wgpu::TextureFormat::R32Sint:
            case wgpu::TextureFormat::RG16Uint:
            case wgpu::TextureFormat::RG16Sint:
            case wgpu::TextureFormat::RG16Float:
            case wgpu::TextureFormat::RGBA8Unorm:
            case wgpu::TextureFormat::RGBA8UnormSrgb:
            case wgpu::TextureFormat::RGBA8Snorm:
            case wgpu::TextureFormat::RGBA8Uint:
            case wgpu::TextureFormat::RGBA8Sint:
            case wgpu::TextureFormat::BGRA8Unorm:
            case wgpu::TextureFormat::BGRA8UnormSrgb:
            case wgpu::TextureFormat::RGB10A2Unorm:
            case wgpu::TextureFormat::RG11B10Float:
            case wgpu::TextureFormat::RG32Float:
            case wgpu::TextureFormat::RG32Uint:
            case wgpu::TextureFormat::RG32Sint:
            case wgpu::TextureFormat::RGBA16Uint:
            case wgpu::TextureFormat::RGBA16Sint:
            case wgpu::TextureFormat::RGBA16Float:
            case wgpu::TextureFormat::RGBA32Float:
            case wgpu::TextureFormat::RGBA32Uint:
            case wgpu::TextureFormat::RGBA32Sint:
            case wgpu::TextureFormat::Depth32Float:
            case wgpu::TextureFormat::Depth24Plus:
            case wgpu::TextureFormat::Depth24PlusStencil8:
                return 1u;

            case wgpu::TextureFormat::BC1RGBAUnorm:
            case wgpu::TextureFormat::BC1RGBAUnormSrgb:
            case wgpu::TextureFormat::BC4RUnorm:
            case wgpu::TextureFormat::BC4RSnorm:
            case wgpu::TextureFormat::BC2RGBAUnorm:
            case wgpu::TextureFormat::BC2RGBAUnormSrgb:
            case wgpu::TextureFormat::BC3RGBAUnorm:
            case wgpu::TextureFormat::BC3RGBAUnormSrgb:
            case wgpu::TextureFormat::BC5RGUnorm:
            case wgpu::TextureFormat::BC5RGSnorm:
            case wgpu::TextureFormat::BC6HRGBUfloat:
            case wgpu::TextureFormat::BC6HRGBSfloat:
            case wgpu::TextureFormat::BC7RGBAUnorm:
            case wgpu::TextureFormat::BC7RGBAUnormSrgb:
                return 4u;

            case wgpu::TextureFormat::Undefined:
            default:
                UNREACHABLE();
                return 0u;
        }
    }

    const char* GetGLSLImageFormatQualifier(wgpu::TextureFormat textureFormat) {
        switch (textureFormat) {
            case wgpu::TextureFormat::R8Unorm:
                return "r8";
            case wgpu::TextureFormat::R8Snorm:
                return "r8_snorm";
            case wgpu::TextureFormat::R8Uint:
                return "r8ui";
            case wgpu::TextureFormat::R8Sint:
                return "r8i";
            case wgpu::TextureFormat::R16Uint:
                return "r16ui";
            case wgpu::TextureFormat::R16Sint:
                return "r16i";
            case wgpu::TextureFormat::R16Float:
                return "r16f";
            case wgpu::TextureFormat::RG8Unorm:
                return "rg8";
            case wgpu::TextureFormat::RG8Snorm:
                return "rg8_snorm";
            case wgpu::TextureFormat::RG8Uint:
                return "rg8ui";
            case wgpu::TextureFormat::RG8Sint:
                return "rg8i";
            case wgpu::TextureFormat::R32Float:
                return "r32f";
            case wgpu::TextureFormat::R32Uint:
                return "r32ui";
            case wgpu::TextureFormat::R32Sint:
                return "r32i";
            case wgpu::TextureFormat::RG16Uint:
                return "rg16ui";
            case wgpu::TextureFormat::RG16Sint:
                return "rg16i";
            case wgpu::TextureFormat::RG16Float:
                return "rg16f";
            case wgpu::TextureFormat::RGBA8Unorm:
                return "rgba8";
            case wgpu::TextureFormat::RGBA8Snorm:
                return "rgba8_snorm";
            case wgpu::TextureFormat::RGBA8Uint:
                return "rgba8ui";
            case wgpu::TextureFormat::RGBA8Sint:
                return "rgba8i";
            case wgpu::TextureFormat::RGB10A2Unorm:
                return "rgb10_a2";
            case wgpu::TextureFormat::RG11B10Float:
                return "r11f_g11f_b10f";
            case wgpu::TextureFormat::RG32Float:
                return "rg32f";
            case wgpu::TextureFormat::RG32Uint:
                return "rg32ui";
            case wgpu::TextureFormat::RG32Sint:
                return "rg32i";
            case wgpu::TextureFormat::RGBA16Uint:
                return "rgba16ui";
            case wgpu::TextureFormat::RGBA16Sint:
                return "rgba16i";
            case wgpu::TextureFormat::RGBA16Float:
                return "rgba16f";
            case wgpu::TextureFormat::RGBA32Float:
                return "rgba32f";
            case wgpu::TextureFormat::RGBA32Uint:
                return "rgba32ui";
            case wgpu::TextureFormat::RGBA32Sint:
                return "rgba32i";

            case wgpu::TextureFormat::RGBA8UnormSrgb:
            case wgpu::TextureFormat::BGRA8Unorm:
            case wgpu::TextureFormat::BGRA8UnormSrgb:
            case wgpu::TextureFormat::BC1RGBAUnorm:
            case wgpu::TextureFormat::BC1RGBAUnormSrgb:
            case wgpu::TextureFormat::BC4RUnorm:
            case wgpu::TextureFormat::BC4RSnorm:
            case wgpu::TextureFormat::BC2RGBAUnorm:
            case wgpu::TextureFormat::BC2RGBAUnormSrgb:
            case wgpu::TextureFormat::BC3RGBAUnorm:
            case wgpu::TextureFormat::BC3RGBAUnormSrgb:
            case wgpu::TextureFormat::BC5RGUnorm:
            case wgpu::TextureFormat::BC5RGSnorm:
            case wgpu::TextureFormat::BC6HRGBUfloat:
            case wgpu::TextureFormat::BC6HRGBSfloat:
            case wgpu::TextureFormat::BC7RGBAUnorm:
            case wgpu::TextureFormat::BC7RGBAUnormSrgb:
            case wgpu::TextureFormat::Depth32Float:
            case wgpu::TextureFormat::Depth24Plus:
            case wgpu::TextureFormat::Depth24PlusStencil8:
            case wgpu::TextureFormat::Undefined:
                UNREACHABLE();
                return "";
        }
    }
}  // namespace utils
