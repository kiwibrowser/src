/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * Copyright 2011 Intel Corporation
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 * Authors:
 *     Chad Versace <chad@chad-versace.us>
 *
 **************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include "main/glheader.h"
#include "main/macros.h"
#include "main/mtypes.h"
#include "main/colormac.h"
#include "main/renderbuffer.h"

#include "intel_buffers.h"
#include "intel_fbo.h"
#include "intel_mipmap_tree.h"
#include "intel_screen.h"
#include "intel_span.h"
#include "intel_regions.h"
#include "intel_tex.h"

#include "swrast/swrast.h"
#include "swrast/s_renderbuffer.h"

/**
 * \brief Get pointer offset into stencil buffer.
 *
 * The stencil buffer is W tiled. Since the GTT is incapable of W fencing, we
 * must decode the tile's layout in software.
 *
 * See
 *   - PRM, 2011 Sandy Bridge, Volume 1, Part 2, Section 4.5.2.1 W-Major Tile
 *     Format.
 *   - PRM, 2011 Sandy Bridge, Volume 1, Part 2, Section 4.5.3 Tiling Algorithm
 *
 * Even though the returned offset is always positive, the return type is
 * signed due to
 *    commit e8b1c6d6f55f5be3bef25084fdd8b6127517e137
 *    mesa: Fix return type of  _mesa_get_format_bytes() (#37351)
 */
intptr_t
intel_offset_S8(uint32_t stride, uint32_t x, uint32_t y, bool swizzled)
{
   uint32_t tile_size = 4096;
   uint32_t tile_width = 64;
   uint32_t tile_height = 64;
   uint32_t row_size = 64 * stride;

   uint32_t tile_x = x / tile_width;
   uint32_t tile_y = y / tile_height;

   /* The byte's address relative to the tile's base addres. */
   uint32_t byte_x = x % tile_width;
   uint32_t byte_y = y % tile_height;

   uintptr_t u = tile_y * row_size
               + tile_x * tile_size
               + 512 * (byte_x / 8)
               +  64 * (byte_y / 8)
               +  32 * ((byte_y / 4) % 2)
               +  16 * ((byte_x / 4) % 2)
               +   8 * ((byte_y / 2) % 2)
               +   4 * ((byte_x / 2) % 2)
               +   2 * (byte_y % 2)
               +   1 * (byte_x % 2);

   if (swizzled) {
      /* adjust for bit6 swizzling */
      if (((byte_x / 8) % 2) == 1) {
	 if (((byte_y / 8) % 2) == 0) {
	    u += 64;
	 } else {
	    u -= 64;
	 }
      }
   }

   return u;
}

/**
 * Map the regions needed by intelSpanRenderStart().
 */
static void
intel_span_map_buffers(struct intel_context *intel)
{
   struct gl_context *ctx = &intel->ctx;
   struct intel_texture_object *tex_obj;

   for (int i = 0; i < ctx->Const.MaxTextureImageUnits; i++) {
      if (!ctx->Texture.Unit[i]._ReallyEnabled)
	 continue;
      tex_obj = intel_texture_object(ctx->Texture.Unit[i]._Current);
      intel_finalize_mipmap_tree(intel, i);
      intel_tex_map_images(intel, tex_obj,
			   GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);
   }

   _swrast_map_renderbuffers(ctx);
}

/**
 * Prepare for software rendering.  Map current read/draw framebuffers'
 * renderbuffes and all currently bound texture objects.
 *
 * Old note: Moved locking out to get reasonable span performance.
 */
void
intelSpanRenderStart(struct gl_context * ctx)
{
   struct intel_context *intel = intel_context(ctx);

   intel_flush(ctx);
   intel_prepare_render(intel);
   intel_flush(ctx);
   intel_span_map_buffers(intel);
}

/**
 * Called when done software rendering.  Unmap the buffers we mapped in
 * the above function.
 */
void
intelSpanRenderFinish(struct gl_context * ctx)
{
   struct intel_context *intel = intel_context(ctx);
   GLuint i;

   _swrast_flush(ctx);

   for (i = 0; i < ctx->Const.MaxTextureImageUnits; i++) {
      if (ctx->Texture.Unit[i]._ReallyEnabled) {
         struct gl_texture_object *texObj = ctx->Texture.Unit[i]._Current;
         intel_tex_unmap_images(intel, intel_texture_object(texObj));
      }
   }

   _swrast_unmap_renderbuffers(ctx);
}


void
intelInitSpanFuncs(struct gl_context * ctx)
{
   struct swrast_device_driver *swdd = _swrast_GetDeviceDriverReference(ctx);
   if (swdd) {
      swdd->SpanRenderStart = intelSpanRenderStart;
      swdd->SpanRenderFinish = intelSpanRenderFinish;
   }
}

void
intel_map_vertex_shader_textures(struct gl_context *ctx)
{
   struct intel_context *intel = intel_context(ctx);
   int i;

   if (ctx->VertexProgram._Current == NULL)
      return;

   for (i = 0; i < ctx->Const.MaxTextureImageUnits; i++) {
      if (ctx->Texture.Unit[i]._ReallyEnabled &&
	  ctx->VertexProgram._Current->Base.TexturesUsed[i] != 0) {
         struct gl_texture_object *texObj = ctx->Texture.Unit[i]._Current;

         intel_tex_map_images(intel, intel_texture_object(texObj),
                              GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);
      }
   }
}

void
intel_unmap_vertex_shader_textures(struct gl_context *ctx)
{
   struct intel_context *intel = intel_context(ctx);
   int i;

   if (ctx->VertexProgram._Current == NULL)
      return;

   for (i = 0; i < ctx->Const.MaxTextureImageUnits; i++) {
      if (ctx->Texture.Unit[i]._ReallyEnabled &&
	  ctx->VertexProgram._Current->Base.TexturesUsed[i] != 0) {
         struct gl_texture_object *texObj = ctx->Texture.Unit[i]._Current;

         intel_tex_unmap_images(intel, intel_texture_object(texObj));
      }
   }
}
