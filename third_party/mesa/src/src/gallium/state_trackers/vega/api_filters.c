/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.  All Rights Reserved.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "VG/openvg.h"

#include "vg_context.h"
#include "image.h"
#include "api.h"
#include "handle.h"
#include "renderer.h"
#include "shaders_cache.h"

#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "util/u_inlines.h"
#include "pipe/p_screen.h"

#include "util/u_format.h"
#include "util/u_sampler.h"
#include "util/u_string.h"

#include "asm_filters.h"


struct filter_info {
   struct vg_image *dst;
   struct vg_image *src;
   struct vg_shader * (*setup_shader)(struct vg_context *, void *);
   void *user_data;
   const void *const_buffer;
   VGint const_buffer_len;
   VGTilingMode tiling_mode;
   struct pipe_sampler_view *extra_texture_view;
};

static INLINE struct pipe_resource *create_texture_1d(struct vg_context *ctx,
                                                     const VGuint *color_data,
                                                     const VGint color_data_len)
{
   struct pipe_context *pipe = ctx->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct pipe_resource *tex = 0;
   struct pipe_resource templ;

   memset(&templ, 0, sizeof(templ));
   templ.target = PIPE_TEXTURE_1D;
   templ.format = PIPE_FORMAT_B8G8R8A8_UNORM;
   templ.last_level = 0;
   templ.width0 = color_data_len;
   templ.height0 = 1;
   templ.depth0 = 1;
   templ.array_size = 1;
   templ.bind = PIPE_BIND_SAMPLER_VIEW;

   tex = screen->resource_create(screen, &templ);

   { /* upload color_data */
      struct pipe_transfer *transfer =
         pipe_get_transfer(pipe, tex,
                           0, 0,
                           PIPE_TRANSFER_READ_WRITE ,
                           0, 0, tex->width0, tex->height0);
      void *map = pipe->transfer_map(pipe, transfer);
      memcpy(map, color_data, sizeof(VGint)*color_data_len);
      pipe->transfer_unmap(pipe, transfer);
      pipe->transfer_destroy(pipe, transfer);
   }

   return tex;
}

static INLINE struct pipe_sampler_view *create_texture_1d_view(struct vg_context *ctx,
                                                               const VGuint *color_data,
                                                               const VGint color_data_len)
{
   struct pipe_context *pipe = ctx->pipe;
   struct pipe_resource *texture;
   struct pipe_sampler_view view_templ;
   struct pipe_sampler_view *view;

   texture = create_texture_1d(ctx, color_data, color_data_len);

   if (!texture)
      return NULL;

   u_sampler_view_default_template(&view_templ, texture, texture->format);
   view = pipe->create_sampler_view(pipe, texture, &view_templ);
   /* want the texture to go away if the view is freed */
   pipe_resource_reference(&texture, NULL);

   return view;
}

static struct vg_shader * setup_color_matrix(struct vg_context *ctx, void *user_data)
{
   struct vg_shader *shader =
      shader_create_from_text(ctx->pipe, color_matrix_asm, 200,
         PIPE_SHADER_FRAGMENT);
   return shader;
}

static struct vg_shader * setup_convolution(struct vg_context *ctx, void *user_data)
{
   char buffer[1024];
   VGint num_consts = (VGint)(long)(user_data);
   struct vg_shader *shader;

   util_snprintf(buffer, 1023, convolution_asm, num_consts, num_consts / 2 + 1);

   shader = shader_create_from_text(ctx->pipe, buffer, 200,
                                    PIPE_SHADER_FRAGMENT);

   return shader;
}

static struct vg_shader * setup_lookup(struct vg_context *ctx, void *user_data)
{
   struct vg_shader *shader =
      shader_create_from_text(ctx->pipe, lookup_asm,
                              200, PIPE_SHADER_FRAGMENT);

   return shader;
}


static struct vg_shader * setup_lookup_single(struct vg_context *ctx, void *user_data)
{
   char buffer[1024];
   VGImageChannel channel = (VGImageChannel)(user_data);
   struct vg_shader *shader;

   switch(channel) {
   case VG_RED:
      util_snprintf(buffer, 1023, lookup_single_asm, "xxxx");
      break;
   case VG_GREEN:
      util_snprintf(buffer, 1023, lookup_single_asm, "yyyy");
      break;
   case VG_BLUE:
      util_snprintf(buffer, 1023, lookup_single_asm, "zzzz");
      break;
   case VG_ALPHA:
      util_snprintf(buffer, 1023, lookup_single_asm, "wwww");
      break;
   default:
      debug_assert(!"Unknown color channel");
   }

   shader = shader_create_from_text(ctx->pipe, buffer, 200,
                                    PIPE_SHADER_FRAGMENT);

   return shader;
}

static void execute_filter(struct vg_context *ctx,
                           struct filter_info *info)
{
   struct vg_shader *shader;
   const struct pipe_sampler_state *samplers[2];
   struct pipe_sampler_view *views[2];
   struct pipe_sampler_state sampler;
   uint tex_wrap;

   memset(&sampler, 0, sizeof(sampler));
   sampler.min_img_filter = PIPE_TEX_FILTER_LINEAR;
   sampler.mag_img_filter = PIPE_TEX_FILTER_LINEAR;
   sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
   sampler.normalized_coords = 1;

   switch (info->tiling_mode) {
   case VG_TILE_FILL:
      tex_wrap = PIPE_TEX_WRAP_CLAMP_TO_BORDER;
      /* copy border color */
      memcpy(sampler.border_color.f, ctx->state.vg.tile_fill_color,
            sizeof(sampler.border_color));
      break;
   case VG_TILE_PAD:
      tex_wrap = PIPE_TEX_WRAP_CLAMP_TO_EDGE;;
      break;
   case VG_TILE_REPEAT:
      tex_wrap = PIPE_TEX_WRAP_REPEAT;;
      break;
   case VG_TILE_REFLECT:
      tex_wrap = PIPE_TEX_WRAP_MIRROR_REPEAT;
      break;
   default:
      debug_assert(!"Unknown tiling mode");
      tex_wrap = 0;
      break;
   }

   sampler.wrap_s = tex_wrap;
   sampler.wrap_t = tex_wrap;
   sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;

   samplers[0] = samplers[1] = &sampler;
   views[0] = info->src->sampler_view;
   views[1] = info->extra_texture_view;

   shader = info->setup_shader(ctx, info->user_data);

   if (renderer_filter_begin(ctx->renderer,
            info->dst->sampler_view->texture, VG_TRUE,
            ctx->state.vg.filter_channel_mask,
            samplers, views, (info->extra_texture_view) ? 2 : 1,
            shader->driver, info->const_buffer, info->const_buffer_len)) {
      renderer_filter(ctx->renderer,
            info->dst->x, info->dst->y, info->dst->width, info->dst->height,
            info->src->x, info->src->y, info->src->width, info->src->height);
      renderer_filter_end(ctx->renderer);
   }

   vg_shader_destroy(ctx, shader);
}

void vegaColorMatrix(VGImage dst, VGImage src,
                     const VGfloat * matrix)
{
   struct vg_context *ctx = vg_current_context();
   struct vg_image *d, *s;
   struct filter_info info;

   if (dst == VG_INVALID_HANDLE || src == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }
   if (!matrix || !is_aligned(matrix)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   d = handle_to_image(dst);
   s = handle_to_image(src);

   if (vg_image_overlaps(d, s)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   info.dst = d;
   info.src = s;
   info.setup_shader = &setup_color_matrix;
   info.user_data = NULL;
   info.const_buffer = matrix;
   info.const_buffer_len = 20 * sizeof(VGfloat);
   info.tiling_mode = VG_TILE_PAD;
   info.extra_texture_view = NULL;
   execute_filter(ctx, &info);
}

static VGfloat texture_offset(VGfloat width, VGint kernelSize, VGint current, VGint shift)
{
   VGfloat diff = current - shift;

   return diff / width;
}

void vegaConvolve(VGImage dst, VGImage src,
                  VGint kernelWidth, VGint kernelHeight,
                  VGint shiftX, VGint shiftY,
                  const VGshort * kernel,
                  VGfloat scale,
                  VGfloat bias,
                  VGTilingMode tilingMode)
{
   struct vg_context *ctx = vg_current_context();
   VGfloat *buffer;
   VGint buffer_len;
   VGint i, j;
   VGint idx = 0;
   struct vg_image *d, *s;
   VGint kernel_size = kernelWidth * kernelHeight;
   struct filter_info info;
   const VGint max_kernel_size = vegaGeti(VG_MAX_KERNEL_SIZE);

   if (dst == VG_INVALID_HANDLE || src == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   if (kernelWidth <= 0 || kernelHeight <= 0 ||
      kernelWidth > max_kernel_size || kernelHeight > max_kernel_size) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   if (!kernel || !is_aligned_to(kernel, 2)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   if (tilingMode < VG_TILE_FILL ||
       tilingMode > VG_TILE_REFLECT) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   d = handle_to_image(dst);
   s = handle_to_image(src);

   if (vg_image_overlaps(d, s)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   vg_validate_state(ctx);

   buffer_len = 8 + 2 * 4 * kernel_size;
   buffer = (VGfloat*)malloc(buffer_len * sizeof(VGfloat));

   buffer[0] = 0.f;
   buffer[1] = 1.f;
   buffer[2] = 2.f; /*unused*/
   buffer[3] = 4.f; /*unused*/

   buffer[4] = kernelWidth * kernelHeight;
   buffer[5] = scale;
   buffer[6] = bias;
   buffer[7] = 0.f;

   idx = 8;
   for (j = 0; j < kernelHeight; ++j) {
      for (i = 0; i < kernelWidth; ++i) {
         VGint index = j * kernelWidth + i;
         VGfloat x, y;

         x = texture_offset(s->width, kernelWidth, i, shiftX);
         y = texture_offset(s->height, kernelHeight, j, shiftY);

         buffer[idx + index*4 + 0] = x;
         buffer[idx + index*4 + 1] = y;
         buffer[idx + index*4 + 2] = 0.f;
         buffer[idx + index*4 + 3] = 0.f;
      }
   }
   idx += kernel_size * 4;

   for (j = 0; j < kernelHeight; ++j) {
      for (i = 0; i < kernelWidth; ++i) {
         /* transpose the kernel */
         VGint index = j * kernelWidth + i;
         VGint kindex = (kernelWidth - i - 1) * kernelHeight + (kernelHeight - j - 1);
         buffer[idx + index*4 + 0] = kernel[kindex];
         buffer[idx + index*4 + 1] = kernel[kindex];
         buffer[idx + index*4 + 2] = kernel[kindex];
         buffer[idx + index*4 + 3] = kernel[kindex];
      }
   }

   info.dst = d;
   info.src = s;
   info.setup_shader = &setup_convolution;
   info.user_data = (void*)(long)(buffer_len/4);
   info.const_buffer = buffer;
   info.const_buffer_len = buffer_len * sizeof(VGfloat);
   info.tiling_mode = tilingMode;
   info.extra_texture_view = NULL;
   execute_filter(ctx, &info);

   free(buffer);
}

void vegaSeparableConvolve(VGImage dst, VGImage src,
                           VGint kernelWidth,
                           VGint kernelHeight,
                           VGint shiftX, VGint shiftY,
                           const VGshort * kernelX,
                           const VGshort * kernelY,
                           VGfloat scale,
                           VGfloat bias,
                           VGTilingMode tilingMode)
{
   struct vg_context *ctx = vg_current_context();
   VGshort *kernel;
   VGint i, j, idx = 0;
   const VGint max_kernel_size = vegaGeti(VG_MAX_SEPARABLE_KERNEL_SIZE);

   if (dst == VG_INVALID_HANDLE || src == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   if (kernelWidth <= 0 || kernelHeight <= 0 ||
       kernelWidth > max_kernel_size || kernelHeight > max_kernel_size) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   if (!kernelX || !kernelY ||
       !is_aligned_to(kernelX, 2) || !is_aligned_to(kernelY, 2)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }
   if (tilingMode < VG_TILE_FILL ||
       tilingMode > VG_TILE_REFLECT) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }
   kernel = malloc(sizeof(VGshort)*kernelWidth*kernelHeight);
   for (i = 0; i < kernelWidth; ++i) {
      for (j = 0; j < kernelHeight; ++j) {
         kernel[idx] = kernelX[i] * kernelY[j];
         ++idx;
      }
   }
   vegaConvolve(dst, src, kernelWidth, kernelHeight, shiftX, shiftY,
                kernel, scale, bias, tilingMode);
   free(kernel);
}

static INLINE VGfloat compute_gaussian_componenet(VGfloat x, VGfloat y,
                                                  VGfloat stdDeviationX,
                                                  VGfloat stdDeviationY)
{
   VGfloat mult = 1 / ( 2 * M_PI * stdDeviationX * stdDeviationY);
   VGfloat e = exp( - ( pow(x, 2)/(2*pow(stdDeviationX, 2)) +
                        pow(y, 2)/(2*pow(stdDeviationY, 2)) ) );
   return mult * e;
}

static INLINE VGint compute_kernel_size(VGfloat deviation)
{
   VGint size = ceil(2.146 * deviation);
   if (size > 11)
      return 11;
   return size;
}

static void compute_gaussian_kernel(VGfloat *kernel,
                                    VGint width, VGint height,
                                    VGfloat stdDeviationX,
                                    VGfloat stdDeviationY)
{
   VGint i, j;
   VGfloat scale = 0.0f;

   for (j = 0; j < height; ++j) {
      for (i = 0; i < width; ++i) {
         VGint idx =  (height - j -1) * width + (width - i -1);
         kernel[idx] = compute_gaussian_componenet(i-(ceil(width/2))-1,
                                                   j-ceil(height/2)-1,
                                                   stdDeviationX, stdDeviationY);
         scale += kernel[idx];
      }
   }

   for (j = 0; j < height; ++j) {
      for (i = 0; i < width; ++i) {
         VGint idx = j * width + i;
         kernel[idx] /= scale;
      }
   }
}

void vegaGaussianBlur(VGImage dst, VGImage src,
                      VGfloat stdDeviationX,
                      VGfloat stdDeviationY,
                      VGTilingMode tilingMode)
{
   struct vg_context *ctx = vg_current_context();
   struct vg_image *d, *s;
   VGfloat *buffer, *kernel;
   VGint kernel_width, kernel_height, kernel_size;
   VGint buffer_len;
   VGint idx, i, j;
   struct filter_info info;

   if (dst == VG_INVALID_HANDLE || src == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }
   if (stdDeviationX <= 0 || stdDeviationY <= 0) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   if (tilingMode < VG_TILE_FILL ||
       tilingMode > VG_TILE_REFLECT) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   d = handle_to_image(dst);
   s = handle_to_image(src);

   if (vg_image_overlaps(d, s)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   kernel_width = compute_kernel_size(stdDeviationX);
   kernel_height = compute_kernel_size(stdDeviationY);
   kernel_size = kernel_width * kernel_height;
   kernel = malloc(sizeof(VGfloat)*kernel_size);
   compute_gaussian_kernel(kernel, kernel_width, kernel_height,
                           stdDeviationX, stdDeviationY);

   buffer_len = 8 + 2 * 4 * kernel_size;
   buffer = (VGfloat*)malloc(buffer_len * sizeof(VGfloat));

   buffer[0] = 0.f;
   buffer[1] = 1.f;
   buffer[2] = 2.f; /*unused*/
   buffer[3] = 4.f; /*unused*/

   buffer[4] = kernel_width * kernel_height;
   buffer[5] = 1.f;/*scale*/
   buffer[6] = 0.f;/*bias*/
   buffer[7] = 0.f;

   idx = 8;
   for (j = 0; j < kernel_height; ++j) {
      for (i = 0; i < kernel_width; ++i) {
         VGint index = j * kernel_width + i;
         VGfloat x, y;

         x = texture_offset(s->width, kernel_width, i, kernel_width/2);
         y = texture_offset(s->height, kernel_height, j, kernel_height/2);

         buffer[idx + index*4 + 0] = x;
         buffer[idx + index*4 + 1] = y;
         buffer[idx + index*4 + 2] = 0.f;
         buffer[idx + index*4 + 3] = 0.f;
      }
   }
   idx += kernel_size * 4;

   for (j = 0; j < kernel_height; ++j) {
      for (i = 0; i < kernel_width; ++i) {
         /* transpose the kernel */
         VGint index = j * kernel_width + i;
         VGint kindex = (kernel_width - i - 1) * kernel_height + (kernel_height - j - 1);
         buffer[idx + index*4 + 0] = kernel[kindex];
         buffer[idx + index*4 + 1] = kernel[kindex];
         buffer[idx + index*4 + 2] = kernel[kindex];
         buffer[idx + index*4 + 3] = kernel[kindex];
      }
   }

   info.dst = d;
   info.src = s;
   info.setup_shader = &setup_convolution;
   info.user_data = (void*)(long)(buffer_len/4);
   info.const_buffer = buffer;
   info.const_buffer_len = buffer_len * sizeof(VGfloat);
   info.tiling_mode = tilingMode;
   info.extra_texture_view = NULL;
   execute_filter(ctx, &info);

   free(buffer);
   free(kernel);
}

void vegaLookup(VGImage dst, VGImage src,
                const VGubyte * redLUT,
                const VGubyte * greenLUT,
                const VGubyte * blueLUT,
                const VGubyte * alphaLUT,
                VGboolean outputLinear,
                VGboolean outputPremultiplied)
{
   struct vg_context *ctx = vg_current_context();
   struct vg_image *d, *s;
   VGuint color_data[256];
   VGint i;
   struct pipe_sampler_view *lut_texture_view;
   VGfloat buffer[4];
   struct filter_info info;

   if (dst == VG_INVALID_HANDLE || src == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   if (!redLUT || !greenLUT || !blueLUT || !alphaLUT) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   d = handle_to_image(dst);
   s = handle_to_image(src);

   if (vg_image_overlaps(d, s)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   for (i = 0; i < 256; ++i) {
      color_data[i] = blueLUT[i] << 24 | greenLUT[i] << 16 |
                      redLUT[i]  <<  8 | alphaLUT[i];
   }
   lut_texture_view = create_texture_1d_view(ctx, color_data, 255);

   buffer[0] = 0.f;
   buffer[1] = 0.f;
   buffer[2] = 1.f;
   buffer[3] = 1.f;

   info.dst = d;
   info.src = s;
   info.setup_shader = &setup_lookup;
   info.user_data = NULL;
   info.const_buffer = buffer;
   info.const_buffer_len = 4 * sizeof(VGfloat);
   info.tiling_mode = VG_TILE_PAD;
   info.extra_texture_view = lut_texture_view;

   execute_filter(ctx, &info);

   pipe_sampler_view_reference(&lut_texture_view, NULL);
}

void vegaLookupSingle(VGImage dst, VGImage src,
                      const VGuint * lookupTable,
                      VGImageChannel sourceChannel,
                      VGboolean outputLinear,
                      VGboolean outputPremultiplied)
{
   struct vg_context *ctx = vg_current_context();
   struct vg_image *d, *s;
   struct pipe_sampler_view *lut_texture_view;
   VGfloat buffer[4];
   struct filter_info info;
   VGuint color_data[256];
   VGint i;

   if (dst == VG_INVALID_HANDLE || src == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   if (!lookupTable || !is_aligned(lookupTable)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   if (sourceChannel != VG_RED && sourceChannel != VG_GREEN &&
       sourceChannel != VG_BLUE && sourceChannel != VG_ALPHA) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   d = handle_to_image(dst);
   s = handle_to_image(src);

   if (vg_image_overlaps(d, s)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   vg_validate_state(ctx);

   for (i = 0; i < 256; ++i) {
      VGuint rgba = lookupTable[i];
      VGubyte blue, green, red, alpha;
      red   = (rgba & 0xff000000)>>24;
      green = (rgba & 0x00ff0000)>>16;
      blue  = (rgba & 0x0000ff00)>> 8;
      alpha = (rgba & 0x000000ff)>> 0;
      color_data[i] = blue << 24 | green << 16 |
                      red  <<  8 | alpha;
   }
   lut_texture_view = create_texture_1d_view(ctx, color_data, 256);

   buffer[0] = 0.f;
   buffer[1] = 0.f;
   buffer[2] = 1.f;
   buffer[3] = 1.f;

   info.dst = d;
   info.src = s;
   info.setup_shader = &setup_lookup_single;
   info.user_data = (void*)sourceChannel;
   info.const_buffer = buffer;
   info.const_buffer_len = 4 * sizeof(VGfloat);
   info.tiling_mode = VG_TILE_PAD;
   info.extra_texture_view = lut_texture_view;

   execute_filter(ctx, &info);

   pipe_sampler_view_reference(&lut_texture_view, NULL);
}
