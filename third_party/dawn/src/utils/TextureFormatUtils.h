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

#ifndef UTILS_TEXTURE_FORMAT_UTILS_H_
#define UTILS_TEXTURE_FORMAT_UTILS_H_

#include <array>

#include <dawn/webgpu_cpp.h>

#include "common/Assert.h"

namespace utils {
    static constexpr std::array<wgpu::TextureFormat, 52> kAllTextureFormats = {
        wgpu::TextureFormat::R8Unorm,        wgpu::TextureFormat::R8Snorm,
        wgpu::TextureFormat::R8Uint,         wgpu::TextureFormat::R8Sint,
        wgpu::TextureFormat::R16Uint,        wgpu::TextureFormat::R16Sint,
        wgpu::TextureFormat::R16Float,       wgpu::TextureFormat::RG8Unorm,
        wgpu::TextureFormat::RG8Snorm,       wgpu::TextureFormat::RG8Uint,
        wgpu::TextureFormat::RG8Sint,        wgpu::TextureFormat::R32Float,
        wgpu::TextureFormat::R32Uint,        wgpu::TextureFormat::R32Sint,
        wgpu::TextureFormat::RG16Uint,       wgpu::TextureFormat::RG16Sint,
        wgpu::TextureFormat::RG16Float,      wgpu::TextureFormat::RGBA8Unorm,
        wgpu::TextureFormat::RGBA8UnormSrgb, wgpu::TextureFormat::RGBA8Snorm,
        wgpu::TextureFormat::RGBA8Uint,      wgpu::TextureFormat::RGBA8Sint,
        wgpu::TextureFormat::BGRA8Unorm,     wgpu::TextureFormat::BGRA8UnormSrgb,
        wgpu::TextureFormat::RGB10A2Unorm,   wgpu::TextureFormat::RG11B10Float,
        wgpu::TextureFormat::RG32Float,      wgpu::TextureFormat::RG32Uint,
        wgpu::TextureFormat::RG32Sint,       wgpu::TextureFormat::RGBA16Uint,
        wgpu::TextureFormat::RGBA16Sint,     wgpu::TextureFormat::RGBA16Float,
        wgpu::TextureFormat::RGBA32Float,    wgpu::TextureFormat::RGBA32Uint,
        wgpu::TextureFormat::RGBA32Sint,     wgpu::TextureFormat::Depth32Float,
        wgpu::TextureFormat::Depth24Plus,    wgpu::TextureFormat::Depth24PlusStencil8,
        wgpu::TextureFormat::BC1RGBAUnorm,   wgpu::TextureFormat::BC1RGBAUnormSrgb,
        wgpu::TextureFormat::BC2RGBAUnorm,   wgpu::TextureFormat::BC2RGBAUnormSrgb,
        wgpu::TextureFormat::BC3RGBAUnorm,   wgpu::TextureFormat::BC3RGBAUnormSrgb,
        wgpu::TextureFormat::BC4RUnorm,      wgpu::TextureFormat::BC4RSnorm,
        wgpu::TextureFormat::BC5RGUnorm,     wgpu::TextureFormat::BC5RGSnorm,
        wgpu::TextureFormat::BC6HRGBUfloat,  wgpu::TextureFormat::BC6HRGBSfloat,
        wgpu::TextureFormat::BC7RGBAUnorm,   wgpu::TextureFormat::BC7RGBAUnormSrgb,
    };

    const char* GetColorTextureComponentTypePrefix(wgpu::TextureFormat textureFormat);
    bool TextureFormatSupportsStorageTexture(wgpu::TextureFormat format);

    uint32_t GetTexelBlockSizeInBytes(wgpu::TextureFormat textureFormat);
    uint32_t GetTextureFormatBlockWidth(wgpu::TextureFormat textureFormat);
    uint32_t GetTextureFormatBlockHeight(wgpu::TextureFormat textureFormat);
    const char* GetGLSLImageFormatQualifier(wgpu::TextureFormat textureFormat);
}  // namespace utils

#endif
