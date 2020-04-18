/**************************************************************************
 * 
 * Copyright 2008 Red Hat Inc.
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
 **************************************************************************/


#include "main/imports.h"
#include "main/macros.h"
#include "main/mfeatures.h"
#include "main/mtypes.h"
#include "main/enums.h"
#include "main/fbobject.h"
#include "main/framebuffer.h"
#include "main/renderbuffer.h"
#include "main/context.h"
#include "swrast/swrast.h"
#include "drivers/common/meta.h"

#include "radeon_common.h"
#include "radeon_mipmap_tree.h"

#define FILE_DEBUG_FLAG RADEON_TEXTURE
#define DBG(...) do {                                           \
        if (RADEON_DEBUG & FILE_DEBUG_FLAG)                      \
                printf(__VA_ARGS__);                      \
} while(0)

static struct gl_framebuffer *
radeon_new_framebuffer(struct gl_context *ctx, GLuint name)
{
  return _mesa_new_framebuffer(ctx, name);
}

static void
radeon_delete_renderbuffer(struct gl_context *ctx, struct gl_renderbuffer *rb)
{
  struct radeon_renderbuffer *rrb = radeon_renderbuffer(rb);

  radeon_print(RADEON_TEXTURE, RADEON_TRACE,
		"%s(rb %p, rrb %p) \n",
		__func__, rb, rrb);

  ASSERT(rrb);

  if (rrb && rrb->bo) {
    radeon_bo_unref(rrb->bo);
  }
  _mesa_delete_renderbuffer(ctx, rb);
}

#if defined(RADEON_R100)
static GLuint get_depth_z32(const struct radeon_renderbuffer * rrb,
			       GLint x, GLint y)
{
    GLuint ba, address = 0;

    ba = (y >> 4) * (rrb->pitch >> 6) + (x >> 4);

    address |= (x & 0x7) << 2;
    address |= (y & 0x3) << 5;
    address |= (((x & 0x10) >> 2) ^ (y & 0x4)) << 5;
    address |= (ba & 3) << 8;
    address |= (y & 0x8) << 7;
    address |= (((x & 0x8) << 1) ^ (y & 0x10)) << 7;
    address |= (ba & ~0x3) << 10;
    return address;
}

static GLuint get_depth_z16(const struct radeon_renderbuffer * rrb,
			       GLint x, GLint y)
{
    GLuint ba, address = 0;                   /* a[0]    = 0           */

    ba = (y / 16) * (rrb->pitch >> 6) + (x / 32);

    address |= (x & 0x7) << 1;                /* a[1..3] = x[0..2]     */
    address |= (y & 0x7) << 4;                /* a[4..6] = y[0..2]     */
    address |= (x & 0x8) << 4;                /* a[7]    = x[3]        */
    address |= (ba & 0x3) << 8;               /* a[8..9] = ba[0..1]    */
    address |= (y & 0x8) << 7;                /* a[10]   = y[3]        */
    address |= ((x & 0x10) ^ (y & 0x10)) << 7;/* a[11]   = x[4] ^ y[4] */
    address |= (ba & ~0x3) << 10;             /* a[12..] = ba[2..] */
    return address;
}
#endif

#if defined(RADEON_R200)
static GLuint get_depth_z32(const struct radeon_renderbuffer * rrb,
				 GLint x, GLint y)
{
    GLuint offset;
    GLuint b;
    offset = 0;
    b = (((y & 0x7ff) >> 4) * (rrb->pitch >> 7) + (x >> 5));
    offset += (b >> 1) << 12;
    offset += (((rrb->pitch >> 7) & 0x1) ? (b & 0x1) : ((b & 0x1) ^ ((y >> 4) & 0x1))) << 11;
    offset += ((y >> 2) & 0x3) << 9;
    offset += ((x >> 2) & 0x1) << 8;
    offset += ((x >> 3) & 0x3) << 6;
    offset += ((y >> 1) & 0x1) << 5;
    offset += ((x >> 1) & 0x1) << 4;
    offset += (y & 0x1) << 3;
    offset += (x & 0x1) << 2;

    return offset;
}

static GLuint get_depth_z16(const struct radeon_renderbuffer *rrb,
			       GLint x, GLint y)
{
   GLuint offset;
   GLuint b;

   offset = 0;
   b = (((y  >> 4) * (rrb->pitch >> 7) + (x >> 6)));
   offset += (b >> 1) << 12;
   offset += (((rrb->pitch >> 7) & 0x1) ? (b & 0x1) : ((b & 0x1) ^ ((y >> 4) & 0x1))) << 11;
   offset += ((y >> 2) & 0x3) << 9;
   offset += ((x >> 3) & 0x1) << 8;
   offset += ((x >> 4) & 0x3) << 6;
   offset += ((x >> 2) & 0x1) << 5;
   offset += ((y >> 1) & 0x1) << 4;
   offset += ((x >> 1) & 0x1) << 3;
   offset += (y & 0x1) << 2;
   offset += (x & 0x1) << 1;

   return offset;
}
#endif

static void
radeon_map_renderbuffer_s8z24(struct gl_context *ctx,
		       struct gl_renderbuffer *rb,
		       GLuint x, GLuint y, GLuint w, GLuint h,
		       GLbitfield mode,
		       GLubyte **out_map,
		       GLint *out_stride)
{
    struct radeon_renderbuffer *rrb = radeon_renderbuffer(rb);
    uint32_t *untiled_s8z24_map, *tiled_s8z24_map;
    int ret;
    int y_flip = (rb->Name == 0) ? -1 : 1;
    int y_bias = (rb->Name == 0) ? (rb->Height - 1) : 0;
    uint32_t pitch = w * rrb->cpp;

    rrb->map_pitch = pitch;

    rrb->map_buffer = malloc(w * h * 4);
    ret = radeon_bo_map(rrb->bo, !!(mode & GL_MAP_WRITE_BIT));
    assert(!ret);
    untiled_s8z24_map = rrb->map_buffer;
    tiled_s8z24_map = rrb->bo->ptr;

    for (uint32_t pix_y = 0; pix_y < h; ++ pix_y) {
	for (uint32_t pix_x = 0; pix_x < w; ++pix_x) {
	    uint32_t flipped_y = y_flip * (int32_t)(y + pix_y) + y_bias;
	    uint32_t src_offset = get_depth_z32(rrb, x + pix_x, flipped_y);
	    uint32_t dst_offset = pix_y * rrb->map_pitch + pix_x * rrb->cpp;
	    untiled_s8z24_map[dst_offset/4] = tiled_s8z24_map[src_offset/4];
	}
    }

    radeon_bo_unmap(rrb->bo);
		   
    *out_map = rrb->map_buffer;
    *out_stride = rrb->map_pitch;
}

static void
radeon_map_renderbuffer_z16(struct gl_context *ctx,
			    struct gl_renderbuffer *rb,
			    GLuint x, GLuint y, GLuint w, GLuint h,
			    GLbitfield mode,
			    GLubyte **out_map,
			    GLint *out_stride)
{
    struct radeon_renderbuffer *rrb = radeon_renderbuffer(rb);
    uint16_t *untiled_z16_map, *tiled_z16_map;
    int ret;
    int y_flip = (rb->Name == 0) ? -1 : 1;
    int y_bias = (rb->Name == 0) ? (rb->Height - 1) : 0;
    uint32_t pitch = w * rrb->cpp;

    rrb->map_pitch = pitch;

    rrb->map_buffer = malloc(w * h * 2);
    ret = radeon_bo_map(rrb->bo, !!(mode & GL_MAP_WRITE_BIT));
    assert(!ret);

    untiled_z16_map = rrb->map_buffer;
    tiled_z16_map = rrb->bo->ptr;

    for (uint32_t pix_y = 0; pix_y < h; ++ pix_y) {
	for (uint32_t pix_x = 0; pix_x < w; ++pix_x) {
	    uint32_t flipped_y = y_flip * (int32_t)(y + pix_y) + y_bias;
	    uint32_t src_offset = get_depth_z16(rrb, x + pix_x, flipped_y);
	    uint32_t dst_offset = pix_y * rrb->map_pitch + pix_x * rrb->cpp;
	    untiled_z16_map[dst_offset/2] = tiled_z16_map[src_offset/2];
	}
    }

    radeon_bo_unmap(rrb->bo);

    *out_map = rrb->map_buffer;
    *out_stride = rrb->map_pitch;
}

static void
radeon_map_renderbuffer(struct gl_context *ctx,
		       struct gl_renderbuffer *rb,
		       GLuint x, GLuint y, GLuint w, GLuint h,
		       GLbitfield mode,
		       GLubyte **out_map,
		       GLint *out_stride)
{
   struct radeon_context *const rmesa = RADEON_CONTEXT(ctx);
   struct radeon_renderbuffer *rrb = radeon_renderbuffer(rb);
   GLubyte *map;
   GLboolean ok;
   int stride, flip_stride;
   int ret;
   int src_x, src_y;

   if (!rrb || !rrb->bo) {
	   *out_map = NULL;
	   *out_stride = 0;
	   return;
   }

   rrb->map_mode = mode;
   rrb->map_x = x;
   rrb->map_y = y;
   rrb->map_w = w;
   rrb->map_h = h;
   rrb->map_pitch = rrb->pitch;

   ok = rmesa->vtbl.check_blit(rb->Format, rrb->pitch / rrb->cpp);
   if (ok) {
       if (rb->Name) {
	   src_x = x;
	   src_y = y;
       } else {
	   src_x = x;
	   src_y = rrb->base.Base.Height - y - h;
       }

       /* Make a temporary buffer and blit the current contents of the renderbuffer
	* out to it.  This gives us linear access to the buffer, instead of having
	* to do detiling in software.
	*/

       rrb->map_pitch = rrb->pitch;

       assert(!rrb->map_bo);
       rrb->map_bo = radeon_bo_open(rmesa->radeonScreen->bom, 0,
				    rrb->map_pitch * h, 4,
				    RADEON_GEM_DOMAIN_GTT, 0);
       
       ok = rmesa->vtbl.blit(ctx, rrb->bo, rrb->draw_offset,
			     rb->Format, rrb->pitch / rrb->cpp,
			     rb->Width, rb->Height,
			     src_x, src_y,
			     rrb->map_bo, 0,
			     rb->Format, rrb->map_pitch / rrb->cpp,
			     w, h,
			     0, 0,
			     w, h,
			     GL_FALSE);
       assert(ok);

       ret = radeon_bo_map(rrb->map_bo, !!(mode & GL_MAP_WRITE_BIT));
       assert(!ret);

       map = rrb->map_bo->ptr;

       if (rb->Name) {
	   *out_map = map;
	   *out_stride = rrb->map_pitch;
       } else {
	   *out_map = map + (h - 1) * rrb->map_pitch;
	   *out_stride = -rrb->map_pitch;
       }
       return;
   }

   /* sw fallback flush stuff */
   if (radeon_bo_is_referenced_by_cs(rrb->bo, rmesa->cmdbuf.cs)) {
      radeon_firevertices(rmesa);
   }

   if ((rmesa->radeonScreen->chip_flags & RADEON_CHIPSET_DEPTH_ALWAYS_TILED) && !rrb->has_surface) {
       if (rb->Format == MESA_FORMAT_S8_Z24 || rb->Format == MESA_FORMAT_X8_Z24) {
	   radeon_map_renderbuffer_s8z24(ctx, rb, x, y, w, h,
					 mode, out_map, out_stride);
	   return;
       }
       if (rb->Format == MESA_FORMAT_Z16) {
	   radeon_map_renderbuffer_z16(ctx, rb, x, y, w, h,
				       mode, out_map, out_stride);
	   return;
       }
   }

   ret = radeon_bo_map(rrb->bo, !!(mode & GL_MAP_WRITE_BIT));
   assert(!ret);

   map = rrb->bo->ptr;
   stride = rrb->map_pitch;

   if (rb->Name == 0) {
      y = rb->Height - 1 - y;
      flip_stride = -stride;
   } else {
      flip_stride = stride;
      map += rrb->draw_offset;
   }

   map += x * rrb->cpp;
   map += (int)y * stride;

   *out_map = map;
   *out_stride = flip_stride;
}

static void
radeon_unmap_renderbuffer_s8z24(struct gl_context *ctx,
			  struct gl_renderbuffer *rb)
{
   struct radeon_renderbuffer *rrb = radeon_renderbuffer(rb);

   if (!rrb->map_buffer)
     return;

   if (rrb->map_mode & GL_MAP_WRITE_BIT) {
       uint32_t *untiled_s8z24_map = rrb->map_buffer;
       uint32_t *tiled_s8z24_map;
       int y_flip = (rb->Name == 0) ? -1 : 1;
       int y_bias = (rb->Name == 0) ? (rb->Height - 1) : 0;

       radeon_bo_map(rrb->bo, 1);
       
       tiled_s8z24_map = rrb->bo->ptr;

       for (uint32_t pix_y = 0; pix_y < rrb->map_h; pix_y++) {
	   for (uint32_t pix_x = 0; pix_x < rrb->map_w; pix_x++) {
	       uint32_t flipped_y = y_flip * (int32_t)(pix_y + rrb->map_y) + y_bias;
	       uint32_t dst_offset = get_depth_z32(rrb, rrb->map_x + pix_x, flipped_y);
	       uint32_t src_offset = pix_y * rrb->map_pitch + pix_x * rrb->cpp;
	       tiled_s8z24_map[dst_offset/4] = untiled_s8z24_map[src_offset/4];
	   }
       }
       radeon_bo_unmap(rrb->bo);
   }
   free(rrb->map_buffer);
   rrb->map_buffer = NULL;
}

static void
radeon_unmap_renderbuffer_z16(struct gl_context *ctx,
			      struct gl_renderbuffer *rb)
{
   struct radeon_renderbuffer *rrb = radeon_renderbuffer(rb);

   if (!rrb->map_buffer)
     return;

   if (rrb->map_mode & GL_MAP_WRITE_BIT) {
       uint16_t *untiled_z16_map = rrb->map_buffer;
       uint16_t *tiled_z16_map;
       int y_flip = (rb->Name == 0) ? -1 : 1;
       int y_bias = (rb->Name == 0) ? (rb->Height - 1) : 0;

       radeon_bo_map(rrb->bo, 1);
       
       tiled_z16_map = rrb->bo->ptr;

       for (uint32_t pix_y = 0; pix_y < rrb->map_h; pix_y++) {
	   for (uint32_t pix_x = 0; pix_x < rrb->map_w; pix_x++) {
	       uint32_t flipped_y = y_flip * (int32_t)(pix_y + rrb->map_y) + y_bias;
	       uint32_t dst_offset = get_depth_z16(rrb, rrb->map_x + pix_x, flipped_y);
	       uint32_t src_offset = pix_y * rrb->map_pitch + pix_x * rrb->cpp;
	       tiled_z16_map[dst_offset/2] = untiled_z16_map[src_offset/2];
	   }
       }
       radeon_bo_unmap(rrb->bo);
   }
   free(rrb->map_buffer);
   rrb->map_buffer = NULL;
}


static void
radeon_unmap_renderbuffer(struct gl_context *ctx,
			  struct gl_renderbuffer *rb)
{
   struct radeon_context *const rmesa = RADEON_CONTEXT(ctx);
   struct radeon_renderbuffer *rrb = radeon_renderbuffer(rb);
   GLboolean ok;

   if ((rmesa->radeonScreen->chip_flags & RADEON_CHIPSET_DEPTH_ALWAYS_TILED) && !rrb->has_surface) {
       if (rb->Format == MESA_FORMAT_S8_Z24 || rb->Format == MESA_FORMAT_X8_Z24) {
	   radeon_unmap_renderbuffer_s8z24(ctx, rb);
	   return;
       }
       if (rb->Format == MESA_FORMAT_Z16) {
	   radeon_unmap_renderbuffer_z16(ctx, rb);
	   return;
       }
   }

   if (!rrb->map_bo) {
	   if (rrb->bo)
		   radeon_bo_unmap(rrb->bo);
	   return;
   }

   radeon_bo_unmap(rrb->map_bo);

   if (rrb->map_mode & GL_MAP_WRITE_BIT) {
      ok = rmesa->vtbl.blit(ctx, rrb->map_bo, 0,
			    rb->Format, rrb->map_pitch / rrb->cpp,
			    rrb->map_w, rrb->map_h,
			    0, 0,
			    rrb->bo, rrb->draw_offset,
			    rb->Format, rrb->pitch / rrb->cpp,
			    rb->Width, rb->Height,
			    rrb->map_x, rrb->map_y,
			    rrb->map_w, rrb->map_h,
			    GL_FALSE);
      assert(ok);
   }

   radeon_bo_unref(rrb->map_bo);
   rrb->map_bo = NULL;
}


/**
 * Called via glRenderbufferStorageEXT() to set the format and allocate
 * storage for a user-created renderbuffer.
 */
static GLboolean
radeon_alloc_renderbuffer_storage(struct gl_context * ctx, struct gl_renderbuffer *rb,
                                 GLenum internalFormat,
                                 GLuint width, GLuint height)
{
  struct radeon_context *radeon = RADEON_CONTEXT(ctx);
  struct radeon_renderbuffer *rrb = radeon_renderbuffer(rb);
  uint32_t size, pitch;
  int cpp;

  radeon_print(RADEON_TEXTURE, RADEON_TRACE,
		"%s(%p, rb %p) \n",
		__func__, ctx, rb);

   ASSERT(rb->Name != 0);
  switch (internalFormat) {
   case GL_R3_G3_B2:
   case GL_RGB4:
   case GL_RGB5:
      rb->Format = _radeon_texformat_rgb565;
      cpp = 2;
      break;
   case GL_RGB:
   case GL_RGB8:
   case GL_RGB10:
   case GL_RGB12:
   case GL_RGB16:
      rb->Format = _radeon_texformat_argb8888;
      cpp = 4;
      break;
   case GL_RGBA:
   case GL_RGBA2:
   case GL_RGBA4:
   case GL_RGB5_A1:
   case GL_RGBA8:
   case GL_RGB10_A2:
   case GL_RGBA12:
   case GL_RGBA16:
      rb->Format = _radeon_texformat_argb8888;
      cpp = 4;
      break;
   case GL_STENCIL_INDEX:
   case GL_STENCIL_INDEX1_EXT:
   case GL_STENCIL_INDEX4_EXT:
   case GL_STENCIL_INDEX8_EXT:
   case GL_STENCIL_INDEX16_EXT:
      /* alloc a depth+stencil buffer */
      rb->Format = MESA_FORMAT_S8_Z24;
      cpp = 4;
      break;
   case GL_DEPTH_COMPONENT16:
      rb->Format = MESA_FORMAT_Z16;
      cpp = 2;
      break;
   case GL_DEPTH_COMPONENT:
   case GL_DEPTH_COMPONENT24:
   case GL_DEPTH_COMPONENT32:
      rb->Format = MESA_FORMAT_X8_Z24;
      cpp = 4;
      break;
   case GL_DEPTH_STENCIL_EXT:
   case GL_DEPTH24_STENCIL8_EXT:
      rb->Format = MESA_FORMAT_S8_Z24;
      cpp = 4;
      break;
   default:
      _mesa_problem(ctx,
                    "Unexpected format in radeon_alloc_renderbuffer_storage");
      return GL_FALSE;
   }

  rb->_BaseFormat = _mesa_base_fbo_format(ctx, internalFormat);

  if (ctx->Driver.Flush)
	  ctx->Driver.Flush(ctx); /* +r6/r7 */

  if (rrb->bo)
    radeon_bo_unref(rrb->bo);

   pitch = ((cpp * width + 63) & ~63) / cpp;

   if (RADEON_DEBUG & RADEON_MEMORY)
      fprintf(stderr,"Allocating %d x %d radeon RBO (pitch %d)\n", width,
	      height, pitch);

   size = pitch * height * cpp;
   rrb->pitch = pitch * cpp;
   rrb->cpp = cpp;
   rrb->bo = radeon_bo_open(radeon->radeonScreen->bom,
			    0,
			    size,
			    0,
			    RADEON_GEM_DOMAIN_VRAM,
			    0);
   rb->Width = width;
   rb->Height = height;
   return GL_TRUE;
}

#if FEATURE_OES_EGL_image
static void
radeon_image_target_renderbuffer_storage(struct gl_context *ctx,
                                         struct gl_renderbuffer *rb,
                                         void *image_handle)
{
   radeonContextPtr radeon = RADEON_CONTEXT(ctx);
   struct radeon_renderbuffer *rrb;
   __DRIscreen *screen;
   __DRIimage *image;

   screen = radeon->radeonScreen->driScreen;
   image = screen->dri2.image->lookupEGLImage(screen, image_handle,
					      screen->loaderPrivate);
   if (image == NULL)
      return;

   rrb = radeon_renderbuffer(rb);

   if (ctx->Driver.Flush)
      ctx->Driver.Flush(ctx); /* +r6/r7 */

   if (rrb->bo)
      radeon_bo_unref(rrb->bo);
   rrb->bo = image->bo;
   radeon_bo_ref(rrb->bo);
   fprintf(stderr, "image->bo: %p, name: %d, rbs: w %d -> p %d\n", image->bo, image->bo->handle,
           image->width, image->pitch);

   rrb->cpp = image->cpp;
   rrb->pitch = image->pitch * image->cpp;

   rb->Format = image->format;
   rb->InternalFormat = image->internal_format;
   rb->Width = image->width;
   rb->Height = image->height;
   rb->Format = image->format;
   rb->_BaseFormat = _mesa_base_fbo_format(radeon->glCtx,
                                           image->internal_format);
}
#endif

/**
 * Called for each hardware renderbuffer when a _window_ is resized.
 * Just update fields.
 * Not used for user-created renderbuffers!
 */
static GLboolean
radeon_alloc_window_storage(struct gl_context * ctx, struct gl_renderbuffer *rb,
                           GLenum internalFormat, GLuint width, GLuint height)
{
   ASSERT(rb->Name == 0);
   rb->Width = width;
   rb->Height = height;
   rb->InternalFormat = internalFormat;
  radeon_print(RADEON_TEXTURE, RADEON_TRACE,
		"%s(%p, rb %p) \n",
		__func__, ctx, rb);


   return GL_TRUE;
}


static void
radeon_resize_buffers(struct gl_context *ctx, struct gl_framebuffer *fb,
		     GLuint width, GLuint height)
{
     struct radeon_framebuffer *radeon_fb = (struct radeon_framebuffer*)fb;
   int i;

  radeon_print(RADEON_TEXTURE, RADEON_TRACE,
		"%s(%p, fb %p) \n",
		__func__, ctx, fb);

   _mesa_resize_framebuffer(ctx, fb, width, height);

   fb->Initialized = GL_TRUE; /* XXX remove someday */

   if (fb->Name != 0) {
      return;
   }

   /* Make sure all window system renderbuffers are up to date */
   for (i = 0; i < 2; i++) {
      struct gl_renderbuffer *rb = &radeon_fb->color_rb[i]->base.Base;

      /* only resize if size is changing */
      if (rb && (rb->Width != width || rb->Height != height)) {
	 rb->AllocStorage(ctx, rb, rb->InternalFormat, width, height);
      }
   }
}


/** Dummy function for gl_renderbuffer::AllocStorage() */
static GLboolean
radeon_nop_alloc_storage(struct gl_context * ctx, struct gl_renderbuffer *rb,
			 GLenum internalFormat, GLuint width, GLuint height)
{
   _mesa_problem(ctx, "radeon_op_alloc_storage should never be called.");
   return GL_FALSE;
}


/**
 * Create a renderbuffer for a window's color, depth and/or stencil buffer.
 * Not used for user-created renderbuffers.
 */
struct radeon_renderbuffer *
radeon_create_renderbuffer(gl_format format, __DRIdrawable *driDrawPriv)
{
    struct radeon_renderbuffer *rrb;
    struct gl_renderbuffer *rb;

    rrb = CALLOC_STRUCT(radeon_renderbuffer);

    radeon_print(RADEON_TEXTURE, RADEON_TRACE,
		"%s( rrb %p ) \n",
		__func__, rrb);

    if (!rrb)
	return NULL;

    rb = &rrb->base.Base;

    _mesa_init_renderbuffer(rb, 0);
    rb->ClassID = RADEON_RB_CLASS;
    rb->Format = format;
    rb->_BaseFormat = _mesa_get_format_base_format(format);
    rb->InternalFormat = _mesa_get_format_base_format(format);

    rrb->dPriv = driDrawPriv;

    rb->Delete = radeon_delete_renderbuffer;
    rb->AllocStorage = radeon_alloc_window_storage;

    rrb->bo = NULL;
    return rrb;
}

static struct gl_renderbuffer *
radeon_new_renderbuffer(struct gl_context * ctx, GLuint name)
{
  struct radeon_renderbuffer *rrb;
  struct gl_renderbuffer *rb;


  rrb = CALLOC_STRUCT(radeon_renderbuffer);

  radeon_print(RADEON_TEXTURE, RADEON_TRACE,
		"%s(%p, rrb %p) \n",
		__func__, ctx, rrb);

  if (!rrb)
    return NULL;

  rb = &rrb->base.Base;

  _mesa_init_renderbuffer(rb, name);
  rb->ClassID = RADEON_RB_CLASS;
  rb->Delete = radeon_delete_renderbuffer;
  rb->AllocStorage = radeon_alloc_renderbuffer_storage;

  return rb;
}

static void
radeon_bind_framebuffer(struct gl_context * ctx, GLenum target,
                       struct gl_framebuffer *fb, struct gl_framebuffer *fbread)
{
  radeon_print(RADEON_TEXTURE, RADEON_TRACE,
		"%s(%p, fb %p, target %s) \n",
		__func__, ctx, fb,
		_mesa_lookup_enum_by_nr(target));

   if (target == GL_FRAMEBUFFER_EXT || target == GL_DRAW_FRAMEBUFFER_EXT) {
      radeon_draw_buffer(ctx, fb);
   }
   else {
      /* don't need to do anything if target == GL_READ_FRAMEBUFFER_EXT */
   }
}

static void
radeon_framebuffer_renderbuffer(struct gl_context * ctx,
                               struct gl_framebuffer *fb,
                               GLenum attachment, struct gl_renderbuffer *rb)
{

	if (ctx->Driver.Flush)
		ctx->Driver.Flush(ctx); /* +r6/r7 */

	radeon_print(RADEON_TEXTURE, RADEON_TRACE,
		"%s(%p, fb %p, rb %p) \n",
		__func__, ctx, fb, rb);

   _mesa_framebuffer_renderbuffer(ctx, fb, attachment, rb);
   radeon_draw_buffer(ctx, fb);
}

static GLboolean
radeon_update_wrapper(struct gl_context *ctx, struct radeon_renderbuffer *rrb, 
		     struct gl_texture_image *texImage)
{
	struct gl_renderbuffer *rb = &rrb->base.Base;

	radeon_print(RADEON_TEXTURE, RADEON_TRACE,
		"%s(%p, rrb %p, texImage %p, texFormat %s) \n",
		__func__, ctx, rrb, texImage, _mesa_get_format_name(texImage->TexFormat));

	rrb->cpp = _mesa_get_format_bytes(texImage->TexFormat);
	rrb->pitch = texImage->Width * rrb->cpp;
	rb->Format = texImage->TexFormat;
	rb->InternalFormat = texImage->InternalFormat;
	rb->_BaseFormat = _mesa_base_fbo_format(ctx, rb->InternalFormat);
	rb->Width = texImage->Width;
	rb->Height = texImage->Height;
	rb->Delete = radeon_delete_renderbuffer;
	rb->AllocStorage = radeon_nop_alloc_storage;

	return GL_TRUE;
}


static struct radeon_renderbuffer *
radeon_wrap_texture(struct gl_context * ctx, struct gl_texture_image *texImage)
{
  const GLuint name = ~0;   /* not significant, but distinct for debugging */
  struct radeon_renderbuffer *rrb;

   /* make an radeon_renderbuffer to wrap the texture image */
   rrb = CALLOC_STRUCT(radeon_renderbuffer);

   radeon_print(RADEON_TEXTURE, RADEON_TRACE,
		"%s(%p, rrb %p, texImage %p) \n",
		__func__, ctx, rrb, texImage);

   if (!rrb) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glFramebufferTexture");
      return NULL;
   }

   _mesa_init_renderbuffer(&rrb->base.Base, name);
   rrb->base.Base.ClassID = RADEON_RB_CLASS;

   if (!radeon_update_wrapper(ctx, rrb, texImage)) {
      free(rrb);
      return NULL;
   }

   return rrb;
  
}
static void
radeon_render_texture(struct gl_context * ctx,
                     struct gl_framebuffer *fb,
                     struct gl_renderbuffer_attachment *att)
{
   struct gl_texture_image *newImage
      = att->Texture->Image[att->CubeMapFace][att->TextureLevel];
   struct radeon_renderbuffer *rrb = radeon_renderbuffer(att->Renderbuffer);
   radeon_texture_image *radeon_image;
   GLuint imageOffset;

  radeon_print(RADEON_TEXTURE, RADEON_TRACE,
		"%s(%p, fb %p, rrb %p, att %p)\n",
		__func__, ctx, fb, rrb, att);

   (void) fb;

   ASSERT(newImage);

   radeon_image = (radeon_texture_image *)newImage;

   if (!radeon_image->mt) {
      /* Fallback on drawing to a texture without a miptree.
       */
      _mesa_reference_renderbuffer(&att->Renderbuffer, NULL);
      _swrast_render_texture(ctx, fb, att);
      return;
   }
   else if (!rrb) {
      rrb = radeon_wrap_texture(ctx, newImage);
      if (rrb) {
         /* bind the wrapper to the attachment point */
         _mesa_reference_renderbuffer(&att->Renderbuffer, &rrb->base.Base);
      }
      else {
         /* fallback to software rendering */
         _swrast_render_texture(ctx, fb, att);
         return;
      }
   }

   if (!radeon_update_wrapper(ctx, rrb, newImage)) {
       _mesa_reference_renderbuffer(&att->Renderbuffer, NULL);
       _swrast_render_texture(ctx, fb, att);
       return;
   }

   DBG("Begin render texture tid %lx tex=%u w=%d h=%d refcount=%d\n",
       _glthread_GetID(),
       att->Texture->Name, newImage->Width, newImage->Height,
       rrb->base.Base.RefCount);

   /* point the renderbufer's region to the texture image region */
   if (rrb->bo != radeon_image->mt->bo) {
      if (rrb->bo)
  	radeon_bo_unref(rrb->bo);
      rrb->bo = radeon_image->mt->bo;
      radeon_bo_ref(rrb->bo);
   }

   /* compute offset of the particular 2D image within the texture region */
   imageOffset = radeon_miptree_image_offset(radeon_image->mt,
                                            att->CubeMapFace,
                                            att->TextureLevel);

   if (att->Texture->Target == GL_TEXTURE_3D) {
      imageOffset += radeon_image->mt->levels[att->TextureLevel].rowstride *
                     radeon_image->mt->levels[att->TextureLevel].height *
                     att->Zoffset;
   }

   /* store that offset in the region, along with the correct pitch for
    * the image we are rendering to */
   rrb->draw_offset = imageOffset;
   rrb->pitch = radeon_image->mt->levels[att->TextureLevel].rowstride;
   radeon_image->used_as_render_target = GL_TRUE;

   /* update drawing region, etc */
   radeon_draw_buffer(ctx, fb);
}

static void
radeon_finish_render_texture(struct gl_context * ctx,
                            struct gl_renderbuffer_attachment *att)
{
    struct gl_texture_object *tex_obj = att->Texture;
    struct gl_texture_image *image =
	tex_obj->Image[att->CubeMapFace][att->TextureLevel];
    radeon_texture_image *radeon_image = (radeon_texture_image *)image;
    
    if (radeon_image)
	radeon_image->used_as_render_target = GL_FALSE;

    if (ctx->Driver.Flush)
        ctx->Driver.Flush(ctx); /* +r6/r7 */
}
static void
radeon_validate_framebuffer(struct gl_context *ctx, struct gl_framebuffer *fb)
{
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);
	gl_format mesa_format;
	int i;

	for (i = -2; i < (GLint) ctx->Const.MaxColorAttachments; i++) {
		struct gl_renderbuffer_attachment *att;
		if (i == -2) {
			att = &fb->Attachment[BUFFER_DEPTH];
		} else if (i == -1) {
			att = &fb->Attachment[BUFFER_STENCIL];
		} else {
			att = &fb->Attachment[BUFFER_COLOR0 + i];
		}

		if (att->Type == GL_TEXTURE) {
			mesa_format = att->Texture->Image[att->CubeMapFace][att->TextureLevel]->TexFormat;
		} else {
			/* All renderbuffer formats are renderable, but not sampable */
			continue;
		}

		if (!radeon->vtbl.is_format_renderable(mesa_format)){
			fb->_Status = GL_FRAMEBUFFER_UNSUPPORTED;
			radeon_print(RADEON_TEXTURE, RADEON_TRACE,
						"%s: HW doesn't support format %s as output format of attachment %d\n",
						__FUNCTION__, _mesa_get_format_name(mesa_format), i);
			return;
		}
	}
}

void radeon_fbo_init(struct radeon_context *radeon)
{
#if FEATURE_EXT_framebuffer_object
  radeon->glCtx->Driver.NewFramebuffer = radeon_new_framebuffer;
  radeon->glCtx->Driver.NewRenderbuffer = radeon_new_renderbuffer;
  radeon->glCtx->Driver.MapRenderbuffer = radeon_map_renderbuffer;
  radeon->glCtx->Driver.UnmapRenderbuffer = radeon_unmap_renderbuffer;
  radeon->glCtx->Driver.BindFramebuffer = radeon_bind_framebuffer;
  radeon->glCtx->Driver.FramebufferRenderbuffer = radeon_framebuffer_renderbuffer;
  radeon->glCtx->Driver.RenderTexture = radeon_render_texture;
  radeon->glCtx->Driver.FinishRenderTexture = radeon_finish_render_texture;
  radeon->glCtx->Driver.ResizeBuffers = radeon_resize_buffers;
  radeon->glCtx->Driver.ValidateFramebuffer = radeon_validate_framebuffer;
#endif
#if FEATURE_EXT_framebuffer_blit
  radeon->glCtx->Driver.BlitFramebuffer = _mesa_meta_BlitFramebuffer;
#endif
#if FEATURE_OES_EGL_image
  radeon->glCtx->Driver.EGLImageTargetRenderbufferStorage =
	  radeon_image_target_renderbuffer_storage;
#endif
}

  
void radeon_renderbuffer_set_bo(struct radeon_renderbuffer *rb,
				struct radeon_bo *bo)
{
  struct radeon_bo *old;
  old = rb->bo;
  rb->bo = bo;
  radeon_bo_ref(bo);
  if (old)
    radeon_bo_unref(old);
}
