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

#include "dawn_native/opengl/GLFormat.h"

namespace dawn_native { namespace opengl {

    GLFormatTable BuildGLFormatTable() {
        GLFormatTable table;

        using Type = GLFormat::ComponentType;

        auto AddFormat = [&table](wgpu::TextureFormat dawnFormat, GLenum internalFormat,
                                  GLenum format, GLenum type, Type componentType) {
            size_t index = ComputeFormatIndex(dawnFormat);
            ASSERT(index < table.size());

            table[index].internalFormat = internalFormat;
            table[index].format = format;
            table[index].type = type;
            table[index].componentType = componentType;
            table[index].isSupportedOnBackend = true;
        };

        // It's dangerous to go alone, take this:
        //
        //     [ANGLE's formatutils.cpp]
        //     [ANGLE's formatutilsgl.cpp]
        //
        // The format tables in these files are extremely complete and the best reference on GL
        // format support, enums, etc.

        // clang-format off

        // 1 byte color formats
        AddFormat(wgpu::TextureFormat::R8Unorm, GL_R8, GL_RED, GL_UNSIGNED_BYTE, Type::Float);
        AddFormat(wgpu::TextureFormat::R8Snorm, GL_R8_SNORM, GL_RED, GL_BYTE, Type::Float);
        AddFormat(wgpu::TextureFormat::R8Uint, GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, Type::Uint);
        AddFormat(wgpu::TextureFormat::R8Sint, GL_R8I, GL_RED_INTEGER, GL_BYTE, Type::Int);

        // 2 bytes color formats
        AddFormat(wgpu::TextureFormat::R16Uint, GL_R16UI, GL_RED_INTEGER, GL_UNSIGNED_SHORT, Type::Uint);
        AddFormat(wgpu::TextureFormat::R16Sint, GL_R16I, GL_RED_INTEGER, GL_SHORT, Type::Int);
        AddFormat(wgpu::TextureFormat::R16Float, GL_R16F, GL_RED, GL_HALF_FLOAT, Type::Float);
        AddFormat(wgpu::TextureFormat::RG8Unorm, GL_RG8, GL_RG, GL_UNSIGNED_BYTE, Type::Float);
        AddFormat(wgpu::TextureFormat::RG8Snorm, GL_RG8_SNORM, GL_RG, GL_BYTE, Type::Float);
        AddFormat(wgpu::TextureFormat::RG8Uint, GL_RG8UI, GL_RG_INTEGER, GL_UNSIGNED_BYTE, Type::Uint);
        AddFormat(wgpu::TextureFormat::RG8Sint, GL_RG8I, GL_RG_INTEGER, GL_BYTE, Type::Int);

        // 4 bytes color formats
        AddFormat(wgpu::TextureFormat::R32Uint, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, Type::Uint);
        AddFormat(wgpu::TextureFormat::R32Sint, GL_R32I, GL_RED_INTEGER, GL_INT, Type::Int);
        AddFormat(wgpu::TextureFormat::R32Float, GL_R32F, GL_RED, GL_FLOAT, Type::Float);
        AddFormat(wgpu::TextureFormat::RG16Uint, GL_RG16UI, GL_RG_INTEGER, GL_UNSIGNED_SHORT, Type::Uint);
        AddFormat(wgpu::TextureFormat::RG16Sint, GL_RG16I, GL_RG_INTEGER, GL_SHORT, Type::Int);
        AddFormat(wgpu::TextureFormat::RG16Float, GL_RG16F, GL_RG, GL_HALF_FLOAT, Type::Float);
        AddFormat(wgpu::TextureFormat::RGBA8Unorm, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, Type::Float);
        AddFormat(wgpu::TextureFormat::RGBA8UnormSrgb, GL_SRGB8_ALPHA8, GL_RGBA, GL_UNSIGNED_BYTE, Type::Float);
        AddFormat(wgpu::TextureFormat::RGBA8Snorm, GL_RGBA8_SNORM, GL_RGBA, GL_BYTE, Type::Float);
        AddFormat(wgpu::TextureFormat::RGBA8Uint, GL_RGBA8UI, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE, Type::Uint);
        AddFormat(wgpu::TextureFormat::RGBA8Sint, GL_RGBA8I, GL_RGBA_INTEGER, GL_BYTE, Type::Int);

        // This doesn't have an enum for the internal format in OpenGL, so use RGBA8.
        AddFormat(wgpu::TextureFormat::BGRA8Unorm, GL_RGBA8, GL_BGRA, GL_UNSIGNED_BYTE, Type::Float);
        AddFormat(wgpu::TextureFormat::RGB10A2Unorm, GL_RGB10_A2, GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV, Type::Float);
        AddFormat(wgpu::TextureFormat::RG11B10Float, GL_R11F_G11F_B10F, GL_RGB, GL_UNSIGNED_INT_10F_11F_11F_REV, Type::Float);

        // 8 bytes color formats
        AddFormat(wgpu::TextureFormat::RG32Uint, GL_RG32UI, GL_RG_INTEGER, GL_UNSIGNED_INT, Type::Uint);
        AddFormat(wgpu::TextureFormat::RG32Sint, GL_RG32I, GL_RG_INTEGER, GL_INT, Type::Int);
        AddFormat(wgpu::TextureFormat::RG32Float, GL_RG32F, GL_RG, GL_FLOAT, Type::Float);
        AddFormat(wgpu::TextureFormat::RGBA16Uint, GL_RGBA16UI, GL_RGBA_INTEGER, GL_UNSIGNED_SHORT, Type::Uint);
        AddFormat(wgpu::TextureFormat::RGBA16Sint, GL_RGBA16I, GL_RGBA_INTEGER, GL_SHORT, Type::Int);
        AddFormat(wgpu::TextureFormat::RGBA16Float, GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT, Type::Float);

        // 16 bytes color formats
        AddFormat(wgpu::TextureFormat::RGBA32Uint, GL_RGBA32UI, GL_RGBA_INTEGER, GL_UNSIGNED_INT, Type::Uint);
        AddFormat(wgpu::TextureFormat::RGBA32Sint, GL_RGBA32I, GL_RGBA_INTEGER, GL_INT, Type::Int);
        AddFormat(wgpu::TextureFormat::RGBA32Float, GL_RGBA32F, GL_RGBA, GL_FLOAT, Type::Float);

        // Depth stencil formats
        AddFormat(wgpu::TextureFormat::Depth32Float, GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT, Type::DepthStencil);
        AddFormat(wgpu::TextureFormat::Depth24Plus, GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT, Type::DepthStencil);
        AddFormat(wgpu::TextureFormat::Depth24PlusStencil8, GL_DEPTH32F_STENCIL8, GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV, Type::DepthStencil);

        // Block compressed formats
        AddFormat(wgpu::TextureFormat::BC1RGBAUnorm, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, GL_RGBA, GL_UNSIGNED_BYTE, Type::Float);
        AddFormat(wgpu::TextureFormat::BC1RGBAUnormSrgb, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT, GL_RGBA, GL_UNSIGNED_BYTE, Type::Float);
        AddFormat(wgpu::TextureFormat::BC2RGBAUnorm, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, GL_RGBA, GL_UNSIGNED_BYTE, Type::Float);
        AddFormat(wgpu::TextureFormat::BC2RGBAUnormSrgb, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT, GL_RGBA, GL_UNSIGNED_BYTE, Type::Float);
        AddFormat(wgpu::TextureFormat::BC3RGBAUnorm, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_RGBA, GL_UNSIGNED_BYTE, Type::Float);
        AddFormat(wgpu::TextureFormat::BC3RGBAUnormSrgb, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT, GL_RGBA, GL_UNSIGNED_BYTE, Type::Float);
        AddFormat(wgpu::TextureFormat::BC4RSnorm, GL_COMPRESSED_SIGNED_RED_RGTC1, GL_RED, GL_BYTE, Type::Float);
        AddFormat(wgpu::TextureFormat::BC4RUnorm, GL_COMPRESSED_RED_RGTC1, GL_RED, GL_UNSIGNED_BYTE, Type::Float);
        AddFormat(wgpu::TextureFormat::BC5RGSnorm, GL_COMPRESSED_SIGNED_RG_RGTC2, GL_RG, GL_BYTE, Type::Float);
        AddFormat(wgpu::TextureFormat::BC5RGUnorm, GL_COMPRESSED_RG_RGTC2, GL_RG, GL_UNSIGNED_BYTE, Type::Float);
        AddFormat(wgpu::TextureFormat::BC6HRGBSfloat, GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT, GL_RGB, GL_HALF_FLOAT, Type::Float);
        AddFormat(wgpu::TextureFormat::BC6HRGBUfloat, GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT, GL_RGB, GL_HALF_FLOAT, Type::Float);
        AddFormat(wgpu::TextureFormat::BC7RGBAUnorm, GL_COMPRESSED_RGBA_BPTC_UNORM, GL_RGBA, GL_UNSIGNED_BYTE, Type::Float);
        AddFormat(wgpu::TextureFormat::BC7RGBAUnormSrgb, GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM, GL_RGBA, GL_UNSIGNED_BYTE, Type::Float);

        // clang-format on

        return table;
    }

}}  // namespace dawn_native::opengl
