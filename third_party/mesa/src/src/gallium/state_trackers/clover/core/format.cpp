//
// Copyright 2012 Francisco Jerez
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
// OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include <algorithm>

#include "core/format.hpp"
#include "core/memory.hpp"
#include "pipe/p_screen.h"
#include "pipe/p_context.h"

namespace clover {
   static const std::map<cl_image_format, pipe_format> formats {
      { { CL_BGRA, CL_UNORM_INT8 }, PIPE_FORMAT_B8G8R8A8_UNORM },
      { { CL_ARGB, CL_UNORM_INT8 }, PIPE_FORMAT_A8R8G8B8_UNORM },
      { { CL_RGB, CL_UNORM_SHORT_565 }, PIPE_FORMAT_B5G6R5_UNORM },
      { { CL_LUMINANCE, CL_UNORM_INT8 }, PIPE_FORMAT_L8_UNORM },
      { { CL_A, CL_UNORM_INT8 }, PIPE_FORMAT_A8_UNORM },
      { { CL_INTENSITY, CL_UNORM_INT8 }, PIPE_FORMAT_I8_UNORM },
      { { CL_LUMINANCE, CL_UNORM_INT16 }, PIPE_FORMAT_L16_UNORM },
      { { CL_R, CL_FLOAT }, PIPE_FORMAT_R32_FLOAT },
      { { CL_RG, CL_FLOAT }, PIPE_FORMAT_R32G32_FLOAT },
      { { CL_RGB, CL_FLOAT }, PIPE_FORMAT_R32G32B32_FLOAT },
      { { CL_RGBA, CL_FLOAT }, PIPE_FORMAT_R32G32B32A32_FLOAT },
      { { CL_R, CL_UNORM_INT16 }, PIPE_FORMAT_R16_UNORM },
      { { CL_RG, CL_UNORM_INT16 }, PIPE_FORMAT_R16G16_UNORM },
      { { CL_RGB, CL_UNORM_INT16 }, PIPE_FORMAT_R16G16B16_UNORM },
      { { CL_RGBA, CL_UNORM_INT16 }, PIPE_FORMAT_R16G16B16A16_UNORM },
      { { CL_R, CL_SNORM_INT16 }, PIPE_FORMAT_R16_SNORM },
      { { CL_RG, CL_SNORM_INT16 }, PIPE_FORMAT_R16G16_SNORM },
      { { CL_RGB, CL_SNORM_INT16 }, PIPE_FORMAT_R16G16B16_SNORM },
      { { CL_RGBA, CL_SNORM_INT16 }, PIPE_FORMAT_R16G16B16A16_SNORM },
      { { CL_R, CL_UNORM_INT8 }, PIPE_FORMAT_R8_UNORM },
      { { CL_RG, CL_UNORM_INT8 }, PIPE_FORMAT_R8G8_UNORM },
      { { CL_RGB, CL_UNORM_INT8 }, PIPE_FORMAT_R8G8B8_UNORM },
      { { CL_RGBA, CL_UNORM_INT8 }, PIPE_FORMAT_R8G8B8A8_UNORM },
      { { CL_R, CL_SNORM_INT8 }, PIPE_FORMAT_R8_SNORM },
      { { CL_RG, CL_SNORM_INT8 }, PIPE_FORMAT_R8G8_SNORM },
      { { CL_RGB, CL_SNORM_INT8 }, PIPE_FORMAT_R8G8B8_SNORM },
      { { CL_RGBA, CL_SNORM_INT8 }, PIPE_FORMAT_R8G8B8A8_SNORM },
      { { CL_R, CL_HALF_FLOAT }, PIPE_FORMAT_R16_FLOAT },
      { { CL_RG, CL_HALF_FLOAT }, PIPE_FORMAT_R16G16_FLOAT },
      { { CL_RGB, CL_HALF_FLOAT }, PIPE_FORMAT_R16G16B16_FLOAT },
      { { CL_RGBA, CL_HALF_FLOAT }, PIPE_FORMAT_R16G16B16A16_FLOAT },
      { { CL_RGBx, CL_UNORM_SHORT_555 }, PIPE_FORMAT_B5G5R5X1_UNORM },
      { { CL_RGBx, CL_UNORM_INT8 }, PIPE_FORMAT_R8G8B8X8_UNORM },
      { { CL_A, CL_UNORM_INT16 }, PIPE_FORMAT_A16_UNORM },
      { { CL_INTENSITY, CL_UNORM_INT16 }, PIPE_FORMAT_I16_UNORM },
      { { CL_LUMINANCE, CL_SNORM_INT8 }, PIPE_FORMAT_L8_SNORM },
      { { CL_INTENSITY, CL_SNORM_INT8 }, PIPE_FORMAT_I8_SNORM },
      { { CL_A, CL_SNORM_INT16 }, PIPE_FORMAT_A16_SNORM },
      { { CL_LUMINANCE, CL_SNORM_INT16 }, PIPE_FORMAT_L16_SNORM },
      { { CL_INTENSITY, CL_SNORM_INT16 }, PIPE_FORMAT_I16_SNORM },
      { { CL_A, CL_HALF_FLOAT }, PIPE_FORMAT_A16_FLOAT },
      { { CL_LUMINANCE, CL_HALF_FLOAT }, PIPE_FORMAT_L16_FLOAT },
      { { CL_INTENSITY, CL_HALF_FLOAT }, PIPE_FORMAT_I16_FLOAT },
      { { CL_A, CL_FLOAT }, PIPE_FORMAT_A32_FLOAT },
      { { CL_LUMINANCE, CL_FLOAT }, PIPE_FORMAT_L32_FLOAT },
      { { CL_INTENSITY, CL_FLOAT }, PIPE_FORMAT_I32_FLOAT },
      { { CL_RA, CL_UNORM_INT8 }, PIPE_FORMAT_R8A8_UNORM },
      { { CL_R, CL_UNSIGNED_INT8 }, PIPE_FORMAT_R8_UINT },
      { { CL_RG, CL_UNSIGNED_INT8 }, PIPE_FORMAT_R8G8_UINT },
      { { CL_RGB, CL_UNSIGNED_INT8 }, PIPE_FORMAT_R8G8B8_UINT },
      { { CL_RGBA, CL_UNSIGNED_INT8 }, PIPE_FORMAT_R8G8B8A8_UINT },
      { { CL_R, CL_SIGNED_INT8 }, PIPE_FORMAT_R8_SINT },
      { { CL_RG, CL_SIGNED_INT8 }, PIPE_FORMAT_R8G8_SINT },
      { { CL_RGB, CL_SIGNED_INT8 }, PIPE_FORMAT_R8G8B8_SINT },
      { { CL_RGBA, CL_SIGNED_INT8 }, PIPE_FORMAT_R8G8B8A8_SINT },
      { { CL_R, CL_UNSIGNED_INT16 }, PIPE_FORMAT_R16_UINT },
      { { CL_RG, CL_UNSIGNED_INT16 }, PIPE_FORMAT_R16G16_UINT },
      { { CL_RGB, CL_UNSIGNED_INT16 }, PIPE_FORMAT_R16G16B16_UINT },
      { { CL_RGBA, CL_UNSIGNED_INT16 }, PIPE_FORMAT_R16G16B16A16_UINT },
      { { CL_R, CL_SIGNED_INT16 }, PIPE_FORMAT_R16_SINT },
      { { CL_RG, CL_SIGNED_INT16 }, PIPE_FORMAT_R16G16_SINT },
      { { CL_RGB, CL_SIGNED_INT16 }, PIPE_FORMAT_R16G16B16_SINT },
      { { CL_RGBA, CL_SIGNED_INT16 }, PIPE_FORMAT_R16G16B16A16_SINT },
      { { CL_R, CL_UNSIGNED_INT32 }, PIPE_FORMAT_R32_UINT },
      { { CL_RG, CL_UNSIGNED_INT32 }, PIPE_FORMAT_R32G32_UINT },
      { { CL_RGB, CL_UNSIGNED_INT32 }, PIPE_FORMAT_R32G32B32_UINT },
      { { CL_RGBA, CL_UNSIGNED_INT32 }, PIPE_FORMAT_R32G32B32A32_UINT },
      { { CL_R, CL_SIGNED_INT32 }, PIPE_FORMAT_R32_SINT },
      { { CL_RG, CL_SIGNED_INT32 }, PIPE_FORMAT_R32G32_SINT },
      { { CL_RGB, CL_SIGNED_INT32 }, PIPE_FORMAT_R32G32B32_SINT },
      { { CL_RGBA, CL_SIGNED_INT32 }, PIPE_FORMAT_R32G32B32A32_SINT },
      { { CL_A, CL_UNSIGNED_INT8 }, PIPE_FORMAT_A8_UINT },
      { { CL_INTENSITY, CL_UNSIGNED_INT8 }, PIPE_FORMAT_I8_UINT },
      { { CL_LUMINANCE, CL_UNSIGNED_INT8 }, PIPE_FORMAT_L8_UINT },
      { { CL_A, CL_SIGNED_INT8 }, PIPE_FORMAT_A8_SINT },
      { { CL_INTENSITY, CL_SIGNED_INT8 }, PIPE_FORMAT_I8_SINT },
      { { CL_LUMINANCE, CL_SIGNED_INT8 }, PIPE_FORMAT_L8_SINT },
      { { CL_A, CL_UNSIGNED_INT16 }, PIPE_FORMAT_A16_UINT },
      { { CL_INTENSITY, CL_UNSIGNED_INT16 }, PIPE_FORMAT_I16_UINT },
      { { CL_LUMINANCE, CL_UNSIGNED_INT16 }, PIPE_FORMAT_L16_UINT },
      { { CL_A, CL_SIGNED_INT16 }, PIPE_FORMAT_A16_SINT },
      { { CL_INTENSITY, CL_SIGNED_INT16 }, PIPE_FORMAT_I16_SINT },
      { { CL_LUMINANCE, CL_SIGNED_INT16 }, PIPE_FORMAT_L16_SINT },
      { { CL_A, CL_UNSIGNED_INT32 }, PIPE_FORMAT_A32_UINT },
      { { CL_INTENSITY, CL_UNSIGNED_INT32 }, PIPE_FORMAT_I32_UINT },
      { { CL_LUMINANCE, CL_UNSIGNED_INT32 }, PIPE_FORMAT_L32_UINT },
      { { CL_A, CL_SIGNED_INT32 }, PIPE_FORMAT_A32_SINT },
      { { CL_INTENSITY, CL_SIGNED_INT32 }, PIPE_FORMAT_I32_SINT },
      { { CL_LUMINANCE, CL_SIGNED_INT32 }, PIPE_FORMAT_L32_SINT }
   };

   pipe_texture_target
   translate_target(cl_mem_object_type type) {
      switch (type) {
      case CL_MEM_OBJECT_BUFFER:
         return PIPE_BUFFER;
      case CL_MEM_OBJECT_IMAGE2D:
         return PIPE_TEXTURE_2D;
      case CL_MEM_OBJECT_IMAGE3D:
         return PIPE_TEXTURE_3D;
      default:
         throw error(CL_INVALID_VALUE);
      }
   }

   pipe_format
   translate_format(const cl_image_format &format) {
      auto it = formats.find(format);

      if (it == formats.end())
         throw error(CL_IMAGE_FORMAT_NOT_SUPPORTED);

      return it->second;
   }

   std::set<cl_image_format>
   supported_formats(cl_context ctx, cl_mem_object_type type) {
      std::set<cl_image_format> s;
      pipe_texture_target target = translate_target(type);
      unsigned bindings = (PIPE_BIND_SAMPLER_VIEW |
                           PIPE_BIND_COMPUTE_RESOURCE |
                           PIPE_BIND_TRANSFER_READ |
                           PIPE_BIND_TRANSFER_WRITE);

      for (auto f : formats) {
         if (std::all_of(ctx->devs.begin(), ctx->devs.end(),
                         [=](const device *dev) {
                            return dev->pipe->is_format_supported(
                               dev->pipe, f.second, target, 1, bindings);
                         }))
            s.insert(f.first);
      }

      return s;
   }
}
