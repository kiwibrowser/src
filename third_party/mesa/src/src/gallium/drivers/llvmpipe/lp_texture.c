/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  *   Michel Dänzer <michel@tungstengraphics.com>
  */

#include <stdio.h>

#include "pipe/p_context.h"
#include "pipe/p_defines.h"

#include "util/u_inlines.h"
#include "util/u_cpu_detect.h"
#include "util/u_format.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_simple_list.h"
#include "util/u_transfer.h"

#include "lp_context.h"
#include "lp_flush.h"
#include "lp_screen.h"
#include "lp_tile_image.h"
#include "lp_texture.h"
#include "lp_setup.h"
#include "lp_state.h"

#include "state_tracker/sw_winsys.h"


#ifdef DEBUG
static struct llvmpipe_resource resource_list;
#endif
static unsigned id_counter = 0;


static INLINE boolean
resource_is_texture(const struct pipe_resource *resource)
{
   switch (resource->target) {
   case PIPE_BUFFER:
      return FALSE;
   case PIPE_TEXTURE_1D:
   case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_RECT:
   case PIPE_TEXTURE_3D:
   case PIPE_TEXTURE_CUBE:
      return TRUE;
   default:
      assert(0);
      return FALSE;
   }
}



/**
 * Allocate storage for llvmpipe_texture::layout array.
 * The number of elements is width_in_tiles * height_in_tiles.
 */
static enum lp_texture_layout *
alloc_layout_array(unsigned num_slices, unsigned width, unsigned height)
{
   const unsigned tx = align(width, TILE_SIZE) / TILE_SIZE;
   const unsigned ty = align(height, TILE_SIZE) / TILE_SIZE;

   assert(num_slices * tx * ty > 0);
   assert(LP_TEX_LAYOUT_NONE == 0); /* calloc'ing LP_TEX_LAYOUT_NONE here */

   return (enum lp_texture_layout *)
      CALLOC(num_slices * tx * ty, sizeof(enum lp_texture_layout));
}



/**
 * Conventional allocation path for non-display textures:
 * Just compute row strides here.  Storage is allocated on demand later.
 */
static boolean
llvmpipe_texture_layout(struct llvmpipe_screen *screen,
                        struct llvmpipe_resource *lpr)
{
   struct pipe_resource *pt = &lpr->base;
   unsigned level;
   unsigned width = pt->width0;
   unsigned height = pt->height0;
   unsigned depth = pt->depth0;
   size_t total_size = 0;

   assert(LP_MAX_TEXTURE_2D_LEVELS <= LP_MAX_TEXTURE_LEVELS);
   assert(LP_MAX_TEXTURE_3D_LEVELS <= LP_MAX_TEXTURE_LEVELS);

   for (level = 0; level <= pt->last_level; level++) {

      /* Row stride and image stride (for linear layout) */
      {
         unsigned alignment, nblocksx, nblocksy, block_size;

         /* For non-compressed formats we need to align the texture size
          * to the tile size to facilitate render-to-texture.
          */
         if (util_format_is_compressed(pt->format))
            alignment = 1;
         else
            alignment = TILE_SIZE;

         nblocksx = util_format_get_nblocksx(pt->format,
                                             align(width, alignment));
         nblocksy = util_format_get_nblocksy(pt->format,
                                             align(height, alignment));
         block_size = util_format_get_blocksize(pt->format);

         lpr->row_stride[level] = align(nblocksx * block_size, 16);

         lpr->img_stride[level] = lpr->row_stride[level] * nblocksy;
      }

      /* Size of the image in tiles (for tiled layout) */
      {
         const unsigned width_t = align(width, TILE_SIZE) / TILE_SIZE;
         const unsigned height_t = align(height, TILE_SIZE) / TILE_SIZE;
         lpr->tiles_per_row[level] = width_t;
         lpr->tiles_per_image[level] = width_t * height_t;
      }

      /* Number of 3D image slices or cube faces */
      {
         unsigned num_slices;

         if (lpr->base.target == PIPE_TEXTURE_CUBE)
            num_slices = 6;
         else if (lpr->base.target == PIPE_TEXTURE_3D)
            num_slices = depth;
         else
            num_slices = 1;

         lpr->num_slices_faces[level] = num_slices;

         lpr->layout[level] = alloc_layout_array(num_slices, width, height);
         if (!lpr->layout[level]) {
            goto fail;
         }
      }

      total_size += lpr->num_slices_faces[level] * lpr->img_stride[level];
      if (total_size > LP_MAX_TEXTURE_SIZE) {
         goto fail;
      }

      /* Compute size of next mipmap level */
      width = u_minify(width, 1);
      height = u_minify(height, 1);
      depth = u_minify(depth, 1);
   }

   return TRUE;

fail:
   for (level = 0; level <= pt->last_level; level++) {
      if (lpr->layout[level]) {
         FREE(lpr->layout[level]);
      }
   }

   return FALSE;
}



static boolean
llvmpipe_displaytarget_layout(struct llvmpipe_screen *screen,
                              struct llvmpipe_resource *lpr)
{
   struct sw_winsys *winsys = screen->winsys;

   /* Round up the surface size to a multiple of the tile size to
    * avoid tile clipping.
    */
   const unsigned width = align(lpr->base.width0, TILE_SIZE);
   const unsigned height = align(lpr->base.height0, TILE_SIZE);
   const unsigned width_t = width / TILE_SIZE;
   const unsigned height_t = height / TILE_SIZE;

   lpr->tiles_per_row[0] = width_t;
   lpr->tiles_per_image[0] = width_t * height_t;
   lpr->num_slices_faces[0] = 1;
   lpr->img_stride[0] = 0;

   lpr->layout[0] = alloc_layout_array(1, width, height);
   if (!lpr->layout[0]) {
      return FALSE;
   }

   lpr->dt = winsys->displaytarget_create(winsys,
                                          lpr->base.bind,
                                          lpr->base.format,
                                          width, height,
                                          16,
                                          &lpr->row_stride[0] );

   if (lpr->dt == NULL)
      return FALSE;

   {
      void *map = winsys->displaytarget_map(winsys, lpr->dt,
                                            PIPE_TRANSFER_WRITE);

      if (map)
         memset(map, 0, height * lpr->row_stride[0]);

      winsys->displaytarget_unmap(winsys, lpr->dt);
   }

   return TRUE;
}


static struct pipe_resource *
llvmpipe_resource_create(struct pipe_screen *_screen,
                         const struct pipe_resource *templat)
{
   struct llvmpipe_screen *screen = llvmpipe_screen(_screen);
   struct llvmpipe_resource *lpr = CALLOC_STRUCT(llvmpipe_resource);
   if (!lpr)
      return NULL;

   lpr->base = *templat;
   pipe_reference_init(&lpr->base.reference, 1);
   lpr->base.screen = &screen->base;

   /* assert(lpr->base.bind); */

   if (resource_is_texture(&lpr->base)) {
      if (lpr->base.bind & PIPE_BIND_DISPLAY_TARGET) {
         /* displayable surface */
         if (!llvmpipe_displaytarget_layout(screen, lpr))
            goto fail;
         assert(lpr->layout[0][0] == LP_TEX_LAYOUT_NONE);
      }
      else {
         /* texture map */
         if (!llvmpipe_texture_layout(screen, lpr))
            goto fail;
         assert(lpr->layout[0][0] == LP_TEX_LAYOUT_NONE);
      }
      assert(lpr->layout[0]);
   }
   else {
      /* other data (vertex buffer, const buffer, etc) */
      const enum pipe_format format = templat->format;
      const uint w = templat->width0 / util_format_get_blockheight(format);
      /* XXX buffers should only have one dimension, those values should be 1 */
      const uint h = templat->height0 / util_format_get_blockwidth(format);
      const uint d = templat->depth0;
      const uint bpp = util_format_get_blocksize(format);
      const uint bytes = w * h * d * bpp;
      lpr->data = align_malloc(bytes, 16);
      if (!lpr->data)
         goto fail;
      memset(lpr->data, 0, bytes);
   }

   lpr->id = id_counter++;

#ifdef DEBUG
   insert_at_tail(&resource_list, lpr);
#endif

   return &lpr->base;

 fail:
   FREE(lpr);
   return NULL;
}


static void
llvmpipe_resource_destroy(struct pipe_screen *pscreen,
			  struct pipe_resource *pt)
{
   struct llvmpipe_screen *screen = llvmpipe_screen(pscreen);
   struct llvmpipe_resource *lpr = llvmpipe_resource(pt);

   if (lpr->dt) {
      /* display target */
      struct sw_winsys *winsys = screen->winsys;
      winsys->displaytarget_destroy(winsys, lpr->dt);

      if (lpr->tiled[0].data) {
         align_free(lpr->tiled[0].data);
         lpr->tiled[0].data = NULL;
      }

      FREE(lpr->layout[0]);
   }
   else if (resource_is_texture(pt)) {
      /* regular texture */
      uint level;

      /* free linear image data */
      for (level = 0; level < Elements(lpr->linear); level++) {
         if (lpr->linear[level].data) {
            align_free(lpr->linear[level].data);
            lpr->linear[level].data = NULL;
         }
      }

      /* free tiled image data */
      for (level = 0; level < Elements(lpr->tiled); level++) {
         if (lpr->tiled[level].data) {
            align_free(lpr->tiled[level].data);
            lpr->tiled[level].data = NULL;
         }
      }

      /* free layout flag arrays */
      for (level = 0; level < Elements(lpr->tiled); level++) {
         FREE(lpr->layout[level]);
         lpr->layout[level] = NULL;
      }
   }
   else if (!lpr->userBuffer) {
      assert(lpr->data);
      align_free(lpr->data);
   }

#ifdef DEBUG
   if (lpr->next)
      remove_from_list(lpr);
#endif

   FREE(lpr);
}


/**
 * Map a resource for read/write.
 */
void *
llvmpipe_resource_map(struct pipe_resource *resource,
                      unsigned level,
                      unsigned layer,
                      enum lp_texture_usage tex_usage,
                      enum lp_texture_layout layout)
{
   struct llvmpipe_resource *lpr = llvmpipe_resource(resource);
   uint8_t *map;

   assert(level < LP_MAX_TEXTURE_LEVELS);
   assert(layer < (u_minify(resource->depth0, level) + resource->array_size - 1));

   assert(tex_usage == LP_TEX_USAGE_READ ||
          tex_usage == LP_TEX_USAGE_READ_WRITE ||
          tex_usage == LP_TEX_USAGE_WRITE_ALL);

   assert(layout == LP_TEX_LAYOUT_NONE ||
          layout == LP_TEX_LAYOUT_TILED ||
          layout == LP_TEX_LAYOUT_LINEAR);

   if (lpr->dt) {
      /* display target */
      struct llvmpipe_screen *screen = llvmpipe_screen(resource->screen);
      struct sw_winsys *winsys = screen->winsys;
      unsigned dt_usage;
      uint8_t *map2;

      if (tex_usage == LP_TEX_USAGE_READ) {
         dt_usage = PIPE_TRANSFER_READ;
      }
      else {
         dt_usage = PIPE_TRANSFER_READ_WRITE;
      }

      assert(level == 0);
      assert(layer == 0);

      /* FIXME: keep map count? */
      map = winsys->displaytarget_map(winsys, lpr->dt, dt_usage);

      /* install this linear image in texture data structure */
      lpr->linear[level].data = map;

      /* make sure tiled data gets converted to linear data */
      map2 = llvmpipe_get_texture_image(lpr, 0, 0, tex_usage, layout);
      if (layout == LP_TEX_LAYOUT_LINEAR)
         assert(map == map2);

      return map2;
   }
   else if (resource_is_texture(resource)) {

      map = llvmpipe_get_texture_image(lpr, layer, level,
                                       tex_usage, layout);
      return map;
   }
   else {
      return lpr->data;
   }
}


/**
 * Unmap a resource.
 */
void
llvmpipe_resource_unmap(struct pipe_resource *resource,
                       unsigned level,
                       unsigned layer)
{
   struct llvmpipe_resource *lpr = llvmpipe_resource(resource);

   if (lpr->dt) {
      /* display target */
      struct llvmpipe_screen *lp_screen = llvmpipe_screen(resource->screen);
      struct sw_winsys *winsys = lp_screen->winsys;

      assert(level == 0);
      assert(layer == 0);

      /* make sure linear image is up to date */
      (void) llvmpipe_get_texture_image(lpr, layer, level,
                                        LP_TEX_USAGE_READ,
                                        LP_TEX_LAYOUT_LINEAR);

      winsys->displaytarget_unmap(winsys, lpr->dt);
   }
}


void *
llvmpipe_resource_data(struct pipe_resource *resource)
{
   struct llvmpipe_resource *lpr = llvmpipe_resource(resource);

   assert(!resource_is_texture(resource));

   return lpr->data;
}


static struct pipe_resource *
llvmpipe_resource_from_handle(struct pipe_screen *screen,
			      const struct pipe_resource *template,
			      struct winsys_handle *whandle)
{
   struct sw_winsys *winsys = llvmpipe_screen(screen)->winsys;
   struct llvmpipe_resource *lpr;
   unsigned width, height, width_t, height_t;

   /* XXX Seems like from_handled depth textures doesn't work that well */

   lpr = CALLOC_STRUCT(llvmpipe_resource);
   if (!lpr) {
      goto no_lpr;
   }

   lpr->base = *template;
   pipe_reference_init(&lpr->base.reference, 1);
   lpr->base.screen = screen;

   width = align(lpr->base.width0, TILE_SIZE);
   height = align(lpr->base.height0, TILE_SIZE);
   width_t = width / TILE_SIZE;
   height_t = height / TILE_SIZE;

   /*
    * Looks like unaligned displaytargets work just fine,
    * at least sampler/render ones.
    */
#if 0
   assert(lpr->base.width0 == width);
   assert(lpr->base.height0 == height);
#endif

   lpr->tiles_per_row[0] = width_t;
   lpr->tiles_per_image[0] = width_t * height_t;
   lpr->num_slices_faces[0] = 1;
   lpr->img_stride[0] = 0;

   lpr->dt = winsys->displaytarget_from_handle(winsys,
                                               template,
                                               whandle,
                                               &lpr->row_stride[0]);
   if (!lpr->dt) {
      goto no_dt;
   }

   lpr->layout[0] = alloc_layout_array(1, lpr->base.width0, lpr->base.height0);
   if (!lpr->layout[0]) {
      goto no_layout_0;
   }

   assert(lpr->layout[0][0] == LP_TEX_LAYOUT_NONE);

   lpr->id = id_counter++;

#ifdef DEBUG
   insert_at_tail(&resource_list, lpr);
#endif

   return &lpr->base;

no_layout_0:
   winsys->displaytarget_destroy(winsys, lpr->dt);
no_dt:
   FREE(lpr);
no_lpr:
   return NULL;
}


static boolean
llvmpipe_resource_get_handle(struct pipe_screen *screen,
                            struct pipe_resource *pt,
                            struct winsys_handle *whandle)
{
   struct sw_winsys *winsys = llvmpipe_screen(screen)->winsys;
   struct llvmpipe_resource *lpr = llvmpipe_resource(pt);

   assert(lpr->dt);
   if (!lpr->dt)
      return FALSE;

   return winsys->displaytarget_get_handle(winsys, lpr->dt, whandle);
}


static struct pipe_surface *
llvmpipe_create_surface(struct pipe_context *pipe,
                        struct pipe_resource *pt,
                        const struct pipe_surface *surf_tmpl)
{
   struct pipe_surface *ps;

   assert(surf_tmpl->u.tex.level <= pt->last_level);

   ps = CALLOC_STRUCT(pipe_surface);
   if (ps) {
      pipe_reference_init(&ps->reference, 1);
      pipe_resource_reference(&ps->texture, pt);
      ps->context = pipe;
      ps->format = surf_tmpl->format;
      ps->width = u_minify(pt->width0, surf_tmpl->u.tex.level);
      ps->height = u_minify(pt->height0, surf_tmpl->u.tex.level);
      ps->usage = surf_tmpl->usage;

      ps->u.tex.level = surf_tmpl->u.tex.level;
      ps->u.tex.first_layer = surf_tmpl->u.tex.first_layer;
      ps->u.tex.last_layer = surf_tmpl->u.tex.last_layer;
   }
   return ps;
}


static void 
llvmpipe_surface_destroy(struct pipe_context *pipe,
                         struct pipe_surface *surf)
{
   /* Effectively do the texture_update work here - if texture images
    * needed post-processing to put them into hardware layout, this is
    * where it would happen.  For llvmpipe, nothing to do.
    */
   assert(surf->texture);
   pipe_resource_reference(&surf->texture, NULL);
   FREE(surf);
}


static struct pipe_transfer *
llvmpipe_get_transfer(struct pipe_context *pipe,
                      struct pipe_resource *resource,
                      unsigned level,
                      unsigned usage,
                      const struct pipe_box *box)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
   struct llvmpipe_resource *lprex = llvmpipe_resource(resource);
   struct llvmpipe_transfer *lpr;

   assert(resource);
   assert(level <= resource->last_level);

   /*
    * Transfers, like other pipe operations, must happen in order, so flush the
    * context if necessary.
    */
   if (!(usage & PIPE_TRANSFER_UNSYNCHRONIZED)) {
      boolean read_only = !(usage & PIPE_TRANSFER_WRITE);
      boolean do_not_block = !!(usage & PIPE_TRANSFER_DONTBLOCK);
      if (!llvmpipe_flush_resource(pipe, resource,
                                   level,
                                   box->depth > 1 ? -1 : box->z,
                                   read_only,
                                   TRUE, /* cpu_access */
                                   do_not_block,
                                   __FUNCTION__)) {
         /*
          * It would have blocked, but state tracker requested no to.
          */
         assert(do_not_block);
         return NULL;
      }
   }

   if (resource == llvmpipe->constants[PIPE_SHADER_FRAGMENT][0])
      llvmpipe->dirty |= LP_NEW_CONSTANTS;

   lpr = CALLOC_STRUCT(llvmpipe_transfer);
   if (lpr) {
      struct pipe_transfer *pt = &lpr->base;
      pipe_resource_reference(&pt->resource, resource);
      pt->box = *box;
      pt->level = level;
      pt->stride = lprex->row_stride[level];
      pt->layer_stride = lprex->img_stride[level];
      pt->usage = usage;

      return pt;
   }
   return NULL;
}


static void 
llvmpipe_transfer_destroy(struct pipe_context *pipe,
                              struct pipe_transfer *transfer)
{
   /* Effectively do the texture_update work here - if texture images
    * needed post-processing to put them into hardware layout, this is
    * where it would happen.  For llvmpipe, nothing to do.
    */
   assert (transfer->resource);
   pipe_resource_reference(&transfer->resource, NULL);
   FREE(transfer);
}


static void *
llvmpipe_transfer_map( struct pipe_context *pipe,
                       struct pipe_transfer *transfer )
{
   struct llvmpipe_screen *screen = llvmpipe_screen(pipe->screen);
   ubyte *map;
   struct llvmpipe_resource *lpr;
   enum pipe_format format;
   enum lp_texture_usage tex_usage;
   const char *mode;

   assert(transfer->level < LP_MAX_TEXTURE_LEVELS);

   /*
   printf("tex_transfer_map(%d, %d  %d x %d of %d x %d,  usage %d )\n",
          transfer->x, transfer->y, transfer->width, transfer->height,
          transfer->texture->width0,
          transfer->texture->height0,
          transfer->usage);
   */

   if (transfer->usage == PIPE_TRANSFER_READ) {
      tex_usage = LP_TEX_USAGE_READ;
      mode = "read";
   }
   else {
      tex_usage = LP_TEX_USAGE_READ_WRITE;
      mode = "read/write";
   }

   if (0) {
      struct llvmpipe_resource *lpr = llvmpipe_resource(transfer->resource);
      printf("transfer map tex %u  mode %s\n", lpr->id, mode);
   }


   assert(transfer->resource);
   lpr = llvmpipe_resource(transfer->resource);
   format = lpr->base.format;

   map = llvmpipe_resource_map(transfer->resource,
                               transfer->level,
                               transfer->box.z,
                               tex_usage, LP_TEX_LAYOUT_LINEAR);


   /* May want to do different things here depending on read/write nature
    * of the map:
    */
   if (transfer->usage & PIPE_TRANSFER_WRITE) {
      /* Do something to notify sharing contexts of a texture change.
       */
      screen->timestamp++;
   }

   map +=
      transfer->box.y / util_format_get_blockheight(format) * transfer->stride +
      transfer->box.x / util_format_get_blockwidth(format) * util_format_get_blocksize(format);

   return map;
}


static void
llvmpipe_transfer_unmap(struct pipe_context *pipe,
                        struct pipe_transfer *transfer)
{
   assert(transfer->resource);

   llvmpipe_resource_unmap(transfer->resource,
                           transfer->level,
                           transfer->box.z);
}

unsigned int
llvmpipe_is_resource_referenced( struct pipe_context *pipe,
                                 struct pipe_resource *presource,
                                 unsigned level, int layer)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context( pipe );

   if (presource->target == PIPE_BUFFER)
      return LP_UNREFERENCED;

   return lp_setup_is_resource_referenced(llvmpipe->setup, presource);
}



/**
 * Create buffer which wraps user-space data.
 */
struct pipe_resource *
llvmpipe_user_buffer_create(struct pipe_screen *screen,
                            void *ptr,
                            unsigned bytes,
			    unsigned bind_flags)
{
   struct llvmpipe_resource *buffer;

   buffer = CALLOC_STRUCT(llvmpipe_resource);
   if(!buffer)
      return NULL;

   pipe_reference_init(&buffer->base.reference, 1);
   buffer->base.screen = screen;
   buffer->base.format = PIPE_FORMAT_R8_UNORM; /* ?? */
   buffer->base.bind = bind_flags;
   buffer->base.usage = PIPE_USAGE_IMMUTABLE;
   buffer->base.flags = 0;
   buffer->base.width0 = bytes;
   buffer->base.height0 = 1;
   buffer->base.depth0 = 1;
   buffer->base.array_size = 1;
   buffer->userBuffer = TRUE;
   buffer->data = ptr;

   return &buffer->base;
}


/**
 * Compute size (in bytes) need to store a texture image / mipmap level,
 * for just one cube face or one 3D texture slice
 */
static unsigned
tex_image_face_size(const struct llvmpipe_resource *lpr, unsigned level,
                    enum lp_texture_layout layout)
{
   const unsigned width = u_minify(lpr->base.width0, level);
   const unsigned height = u_minify(lpr->base.height0, level);

   assert(layout == LP_TEX_LAYOUT_TILED ||
          layout == LP_TEX_LAYOUT_LINEAR);

   if (layout == LP_TEX_LAYOUT_TILED) {
      /* for tiled layout, force a 32bpp format */
      const enum pipe_format format = PIPE_FORMAT_B8G8R8A8_UNORM;
      const unsigned block_size = util_format_get_blocksize(format);
      const unsigned nblocksy =
         util_format_get_nblocksy(format, align(height, TILE_SIZE));
      const unsigned nblocksx =
         util_format_get_nblocksx(format, align(width, TILE_SIZE));
      const unsigned buffer_size = block_size * nblocksy * nblocksx;
      return buffer_size;
   }
   else {
      /* we already computed this */
      return lpr->img_stride[level];
   }
}


/**
 * Compute size (in bytes) need to store a texture image / mipmap level,
 * including all cube faces or 3D image slices
 */
static unsigned
tex_image_size(const struct llvmpipe_resource *lpr, unsigned level,
               enum lp_texture_layout layout)
{
   const unsigned buf_size = tex_image_face_size(lpr, level, layout);
   return buf_size * lpr->num_slices_faces[level];
}


/**
 * This function encapsulates some complicated logic for determining
 * how to convert a tile of image data from linear layout to tiled
 * layout, or vice versa.
 * \param cur_layout  the current tile layout
 * \param target_layout  the desired tile layout
 * \param usage  how the tile will be accessed (R/W vs. read-only, etc)
 * \param new_layout_return  returns the new layout mode
 * \param convert_return  returns TRUE if image conversion is needed
 */
static void
layout_logic(enum lp_texture_layout cur_layout,
             enum lp_texture_layout target_layout,
             enum lp_texture_usage usage,
             enum lp_texture_layout *new_layout_return,
             boolean *convert)
{
   enum lp_texture_layout other_layout, new_layout;

   *convert = FALSE;

   new_layout = 99; /* debug check */

   if (target_layout == LP_TEX_LAYOUT_LINEAR) {
      other_layout = LP_TEX_LAYOUT_TILED;
   }
   else {
      assert(target_layout == LP_TEX_LAYOUT_TILED);
      other_layout = LP_TEX_LAYOUT_LINEAR;
   }

   new_layout = target_layout;  /* may get changed below */

   if (cur_layout == LP_TEX_LAYOUT_BOTH) {
      if (usage == LP_TEX_USAGE_READ) {
         new_layout = LP_TEX_LAYOUT_BOTH;
      }
   }
   else if (cur_layout == other_layout) {
      if (usage != LP_TEX_USAGE_WRITE_ALL) {
         /* need to convert tiled data to linear or vice versa */
         *convert = TRUE;

         if (usage == LP_TEX_USAGE_READ)
            new_layout = LP_TEX_LAYOUT_BOTH;
      }
   }
   else {
      assert(cur_layout == LP_TEX_LAYOUT_NONE ||
             cur_layout == target_layout);
   }

   assert(new_layout == LP_TEX_LAYOUT_BOTH ||
          new_layout == target_layout);

   *new_layout_return = new_layout;
}


/**
 * Return pointer to a 2D texture image/face/slice.
 * No tiled/linear conversion is done.
 */
ubyte *
llvmpipe_get_texture_image_address(struct llvmpipe_resource *lpr,
                                   unsigned face_slice, unsigned level,
                                   enum lp_texture_layout layout)
{
   struct llvmpipe_texture_image *img;
   unsigned offset;

   if (layout == LP_TEX_LAYOUT_LINEAR) {
      img = &lpr->linear[level];
   }
   else {
      assert (layout == LP_TEX_LAYOUT_TILED);
      img = &lpr->tiled[level];
   }

   if (face_slice > 0)
      offset = face_slice * tex_image_face_size(lpr, level, layout);
   else
      offset = 0;

   return (ubyte *) img->data + offset;
}


static INLINE enum lp_texture_layout
llvmpipe_get_texture_tile_layout(const struct llvmpipe_resource *lpr,
                                 unsigned face_slice, unsigned level,
                                 unsigned x, unsigned y)
{
   uint i;
   assert(resource_is_texture(&lpr->base));
   assert(x < lpr->tiles_per_row[level]);
   i = face_slice * lpr->tiles_per_image[level]
      + y * lpr->tiles_per_row[level] + x;
   return lpr->layout[level][i];
}


static INLINE void
llvmpipe_set_texture_tile_layout(struct llvmpipe_resource *lpr,
                                 unsigned face_slice, unsigned level,
                                 unsigned x, unsigned y,
                                 enum lp_texture_layout layout)
{
   uint i;
   assert(resource_is_texture(&lpr->base));
   assert(x < lpr->tiles_per_row[level]);
   i = face_slice * lpr->tiles_per_image[level]
      + y * lpr->tiles_per_row[level] + x;
   lpr->layout[level][i] = layout;
}


/**
 * Set the layout mode for all tiles in a particular image.
 */
static INLINE void
llvmpipe_set_texture_image_layout(struct llvmpipe_resource *lpr,
                                  unsigned face_slice, unsigned level,
                                  unsigned width_t, unsigned height_t,
                                  enum lp_texture_layout layout)
{
   const unsigned start = face_slice * lpr->tiles_per_image[level];
   unsigned i;

   for (i = 0; i < width_t * height_t; i++) {
      lpr->layout[level][start + i] = layout;
   }
}


/**
 * Allocate storage for a linear or tile texture image (all cube
 * faces and all 3D slices.
 */
static void
alloc_image_data(struct llvmpipe_resource *lpr, unsigned level,
                 enum lp_texture_layout layout)
{
   uint alignment = MAX2(16, util_cpu_caps.cacheline);

   if (lpr->dt)
      assert(level == 0);

   if (layout == LP_TEX_LAYOUT_TILED) {
      /* tiled data is stored in regular memory */
      uint buffer_size = tex_image_size(lpr, level, layout);
      lpr->tiled[level].data = align_malloc(buffer_size, alignment);
      if (lpr->tiled[level].data) {
         memset(lpr->tiled[level].data, 0, buffer_size);
      }
   }
   else {
      assert(layout == LP_TEX_LAYOUT_LINEAR);
      if (lpr->dt) {
         /* we get the linear memory from the winsys, and it has
          * already been zeroed
          */
         struct llvmpipe_screen *screen = llvmpipe_screen(lpr->base.screen);
         struct sw_winsys *winsys = screen->winsys;

         lpr->linear[0].data =
            winsys->displaytarget_map(winsys, lpr->dt,
                                      PIPE_TRANSFER_READ_WRITE);
      }
      else {
         /* not a display target - allocate regular memory */
         uint buffer_size = tex_image_size(lpr, level, LP_TEX_LAYOUT_LINEAR);
         lpr->linear[level].data = align_malloc(buffer_size, alignment);
         if (lpr->linear[level].data) {
            memset(lpr->linear[level].data, 0, buffer_size);
         }
      }
   }
}



/**
 * Return pointer to texture image data (either linear or tiled layout)
 * for a particular cube face or 3D texture slice.
 *
 * \param face_slice  the cube face or 3D slice of interest
 * \param usage  one of LP_TEX_USAGE_READ/WRITE_ALL/READ_WRITE
 * \param layout  either LP_TEX_LAYOUT_LINEAR or _TILED or _NONE
 */
void *
llvmpipe_get_texture_image(struct llvmpipe_resource *lpr,
                           unsigned face_slice, unsigned level,
                           enum lp_texture_usage usage,
                           enum lp_texture_layout layout)
{
   /*
    * 'target' refers to the image which we're retrieving (either in
    * tiled or linear layout).
    * 'other' refers to the same image but in the other layout. (it may
    *  or may not exist.
    */
   struct llvmpipe_texture_image *target_img;
   struct llvmpipe_texture_image *other_img;
   void *target_data;
   void *other_data;
   const unsigned width = u_minify(lpr->base.width0, level);
   const unsigned height = u_minify(lpr->base.height0, level);
   const unsigned width_t = align(width, TILE_SIZE) / TILE_SIZE;
   const unsigned height_t = align(height, TILE_SIZE) / TILE_SIZE;
   enum lp_texture_layout other_layout;
   boolean only_allocate;

   assert(layout == LP_TEX_LAYOUT_NONE ||
          layout == LP_TEX_LAYOUT_TILED ||
          layout == LP_TEX_LAYOUT_LINEAR);

   assert(usage == LP_TEX_USAGE_READ ||
          usage == LP_TEX_USAGE_READ_WRITE ||
          usage == LP_TEX_USAGE_WRITE_ALL);

   /* check for the special case of layout == LP_TEX_LAYOUT_NONE */
   if (layout == LP_TEX_LAYOUT_NONE) {
      only_allocate = TRUE;
      layout = LP_TEX_LAYOUT_TILED;
   }
   else {
      only_allocate = FALSE;
   }

   if (lpr->dt) {
      assert(lpr->linear[level].data);
   }

   /* which is target?  which is other? */
   if (layout == LP_TEX_LAYOUT_LINEAR) {
      target_img = &lpr->linear[level];
      other_img = &lpr->tiled[level];
      other_layout = LP_TEX_LAYOUT_TILED;
   }
   else {
      target_img = &lpr->tiled[level];
      other_img = &lpr->linear[level];
      other_layout = LP_TEX_LAYOUT_LINEAR;
   }

   target_data = target_img->data;
   other_data = other_img->data;

   if (!target_data) {
      /* allocate memory for the target image now */
      alloc_image_data(lpr, level, layout);
      target_data = target_img->data;
   }

   if (face_slice > 0) {
      unsigned target_offset, other_offset;

      target_offset = face_slice * tex_image_face_size(lpr, level, layout);
      other_offset = face_slice * tex_image_face_size(lpr, level, other_layout);
      if (target_data) {
         target_data = (uint8_t *) target_data + target_offset;
      }
      if (other_data) {
         other_data = (uint8_t *) other_data + other_offset;
      }
   }

   if (only_allocate) {
      /* Just allocating tiled memory.  Don't initialize it from the
       * linear data if it exists.
       */
      return target_data;
   }

   if (other_data) {
      /* may need to convert other data to the requested layout */
      enum lp_texture_layout new_layout;
      unsigned x, y;

      /* loop over all image tiles, doing layout conversion where needed */
      for (y = 0; y < height_t; y++) {
         for (x = 0; x < width_t; x++) {
            enum lp_texture_layout cur_layout =
               llvmpipe_get_texture_tile_layout(lpr, face_slice, level, x, y);
            boolean convert;

            layout_logic(cur_layout, layout, usage, &new_layout, &convert);

            if (convert && other_data && target_data) {
               if (layout == LP_TEX_LAYOUT_TILED) {
                  lp_linear_to_tiled(other_data, target_data,
                                     x * TILE_SIZE, y * TILE_SIZE,
                                     TILE_SIZE, TILE_SIZE,
                                     lpr->base.format,
                                     lpr->row_stride[level],
                                     lpr->tiles_per_row[level]);
               }
               else {
                  assert(layout == LP_TEX_LAYOUT_LINEAR);
                  lp_tiled_to_linear(other_data, target_data,
                                     x * TILE_SIZE, y * TILE_SIZE,
                                     TILE_SIZE, TILE_SIZE,
                                     lpr->base.format,
                                     lpr->row_stride[level],
                                     lpr->tiles_per_row[level]);
               }
            }

            if (new_layout != cur_layout)
               llvmpipe_set_texture_tile_layout(lpr, face_slice, level, x, y,
                                                new_layout);
         }
      }
   }
   else {
      /* no other data */
      llvmpipe_set_texture_image_layout(lpr, face_slice, level,
                                        width_t, height_t, layout);
   }

   return target_data;
}


/**
 * Return pointer to start of a texture image (1D, 2D, 3D, CUBE).
 * All cube faces and 3D slices will be converted to the requested
 * layout if needed.
 * This is typically used when we're about to sample from a texture.
 */
void *
llvmpipe_get_texture_image_all(struct llvmpipe_resource *lpr,
                               unsigned level,
                               enum lp_texture_usage usage,
                               enum lp_texture_layout layout)
{
   const int slices = lpr->num_slices_faces[level];
   int slice;
   void *map = NULL;

   assert(slices > 0);

   for (slice = slices - 1; slice >= 0; slice--) {
      map = llvmpipe_get_texture_image(lpr, slice, level, usage, layout);
   }

   return map;
}


/**
 * Get pointer to a linear image (not the tile!) where the tile at (x,y)
 * is known to be in linear layout.
 * Conversion from tiled to linear will be done if necessary.
 * \return pointer to start of image/face (not the tile)
 */
ubyte *
llvmpipe_get_texture_tile_linear(struct llvmpipe_resource *lpr,
                                 unsigned face_slice, unsigned level,
                                 enum lp_texture_usage usage,
                                 unsigned x, unsigned y)
{
   struct llvmpipe_texture_image *linear_img = &lpr->linear[level];
   enum lp_texture_layout cur_layout, new_layout;
   const unsigned tx = x / TILE_SIZE, ty = y / TILE_SIZE;
   boolean convert;
   uint8_t *tiled_image, *linear_image;

   assert(resource_is_texture(&lpr->base));
   assert(x % TILE_SIZE == 0);
   assert(y % TILE_SIZE == 0);

   if (!linear_img->data) {
      /* allocate memory for the linear image now */
      alloc_image_data(lpr, level, LP_TEX_LAYOUT_LINEAR);
   }

   /* compute address of the slice/face of the image that contains the tile */
   tiled_image = llvmpipe_get_texture_image_address(lpr, face_slice, level,
                                                    LP_TEX_LAYOUT_TILED);
   linear_image = llvmpipe_get_texture_image_address(lpr, face_slice, level,
                                                     LP_TEX_LAYOUT_LINEAR);

   /* get current tile layout and determine if data conversion is needed */
   cur_layout = llvmpipe_get_texture_tile_layout(lpr, face_slice, level, tx, ty);

   layout_logic(cur_layout, LP_TEX_LAYOUT_LINEAR, usage,
                &new_layout, &convert);

   if (convert && tiled_image && linear_image) {
      lp_tiled_to_linear(tiled_image, linear_image,
                         x, y, TILE_SIZE, TILE_SIZE, lpr->base.format,
                         lpr->row_stride[level],
                         lpr->tiles_per_row[level]);
   }

   if (new_layout != cur_layout)
      llvmpipe_set_texture_tile_layout(lpr, face_slice, level, tx, ty, new_layout);

   return linear_image;
}


/**
 * Get pointer to tiled data for rendering.
 * \return pointer to the tiled data at the given tile position
 */
ubyte *
llvmpipe_get_texture_tile(struct llvmpipe_resource *lpr,
                          unsigned face_slice, unsigned level,
                          enum lp_texture_usage usage,
                          unsigned x, unsigned y)
{
   struct llvmpipe_texture_image *tiled_img = &lpr->tiled[level];
   enum lp_texture_layout cur_layout, new_layout;
   const unsigned tx = x / TILE_SIZE, ty = y / TILE_SIZE;
   boolean convert;
   uint8_t *tiled_image, *linear_image;
   unsigned tile_offset;

   assert(x % TILE_SIZE == 0);
   assert(y % TILE_SIZE == 0);

   if (!tiled_img->data) {
      /* allocate memory for the tiled image now */
      alloc_image_data(lpr, level, LP_TEX_LAYOUT_TILED);
   }

   /* compute address of the slice/face of the image that contains the tile */
   tiled_image = llvmpipe_get_texture_image_address(lpr, face_slice, level,
                                                    LP_TEX_LAYOUT_TILED);
   linear_image = llvmpipe_get_texture_image_address(lpr, face_slice, level,
                                                     LP_TEX_LAYOUT_LINEAR);

   /* get current tile layout and see if we need to convert the data */
   cur_layout = llvmpipe_get_texture_tile_layout(lpr, face_slice, level, tx, ty);

   layout_logic(cur_layout, LP_TEX_LAYOUT_TILED, usage, &new_layout, &convert);
   if (convert && linear_image && tiled_image) {
      lp_linear_to_tiled(linear_image, tiled_image,
                         x, y, TILE_SIZE, TILE_SIZE, lpr->base.format,
                         lpr->row_stride[level],
                         lpr->tiles_per_row[level]);
   }

   if (!tiled_image)
      return NULL;

   if (new_layout != cur_layout)
      llvmpipe_set_texture_tile_layout(lpr, face_slice, level, tx, ty, new_layout);

   /* compute, return address of the 64x64 tile */
   tile_offset = (ty * lpr->tiles_per_row[level] + tx)
         * TILE_SIZE * TILE_SIZE * 4;

   return (ubyte *) tiled_image + tile_offset;
}


/**
 * Get pointer to tiled data for rendering.
 * \return pointer to the tiled data at the given tile position
 */
void
llvmpipe_unswizzle_cbuf_tile(struct llvmpipe_resource *lpr,
                             unsigned face_slice, unsigned level,
                             unsigned x, unsigned y,
                             uint8_t *tile)
{
   struct llvmpipe_texture_image *linear_img = &lpr->linear[level];
   const unsigned tx = x / TILE_SIZE, ty = y / TILE_SIZE;
   uint8_t *linear_image;

   assert(x % TILE_SIZE == 0);
   assert(y % TILE_SIZE == 0);

   if (!linear_img->data) {
      /* allocate memory for the linear image now */
      alloc_image_data(lpr, level, LP_TEX_LAYOUT_LINEAR);
   }

   /* compute address of the slice/face of the image that contains the tile */
   linear_image = llvmpipe_get_texture_image_address(lpr, face_slice, level,
                                                     LP_TEX_LAYOUT_LINEAR);

   {
      uint ii = x, jj = y;
      uint tile_offset = jj / TILE_SIZE + ii / TILE_SIZE;
      uint byte_offset = tile_offset * TILE_SIZE * TILE_SIZE * 4;
      
      /* Note that lp_tiled_to_linear expects the tile parameter to
       * point at the first tile in a whole-image sized array.  In
       * this code, we have only a single tile and have to do some
       * pointer arithmetic to figure out where the "image" would have
       * started.
       */
      lp_tiled_to_linear(tile - byte_offset, linear_image,
                         x, y, TILE_SIZE, TILE_SIZE,
                         lpr->base.format,
                         lpr->row_stride[level],
                         1);       /* tiles per row */
   }

   llvmpipe_set_texture_tile_layout(lpr, face_slice, level, tx, ty,
                                    LP_TEX_LAYOUT_LINEAR);
}


/**
 * Get pointer to tiled data for rendering.
 * \return pointer to the tiled data at the given tile position
 */
void
llvmpipe_swizzle_cbuf_tile(struct llvmpipe_resource *lpr,
                           unsigned face_slice, unsigned level,
                           unsigned x, unsigned y,
                           uint8_t *tile)
{
   uint8_t *linear_image;

   assert(x % TILE_SIZE == 0);
   assert(y % TILE_SIZE == 0);

   /* compute address of the slice/face of the image that contains the tile */
   linear_image = llvmpipe_get_texture_image_address(lpr, face_slice, level,
                                                     LP_TEX_LAYOUT_LINEAR);

   if (linear_image) {
      uint ii = x, jj = y;
      uint tile_offset = jj / TILE_SIZE + ii / TILE_SIZE;
      uint byte_offset = tile_offset * TILE_SIZE * TILE_SIZE * 4;

      /* Note that lp_linear_to_tiled expects the tile parameter to
       * point at the first tile in a whole-image sized array.  In
       * this code, we have only a single tile and have to do some
       * pointer arithmetic to figure out where the "image" would have
       * started.
       */
      lp_linear_to_tiled(linear_image, tile - byte_offset,
                         x, y, TILE_SIZE, TILE_SIZE,
                         lpr->base.format,
                         lpr->row_stride[level],
                         1);       /* tiles per row */
   }
}


/**
 * Return size of resource in bytes
 */
unsigned
llvmpipe_resource_size(const struct pipe_resource *resource)
{
   const struct llvmpipe_resource *lpr = llvmpipe_resource_const(resource);
   unsigned lvl, size = 0;

   for (lvl = 0; lvl <= lpr->base.last_level; lvl++) {
      if (lpr->linear[lvl].data)
         size += tex_image_size(lpr, lvl, LP_TEX_LAYOUT_LINEAR);

      if (lpr->tiled[lvl].data)
         size += tex_image_size(lpr, lvl, LP_TEX_LAYOUT_TILED);
   }

   return size;
}


#ifdef DEBUG
void
llvmpipe_print_resources(void)
{
   struct llvmpipe_resource *lpr;
   unsigned n = 0, total = 0;

   debug_printf("LLVMPIPE: current resources:\n");
   foreach(lpr, &resource_list) {
      unsigned size = llvmpipe_resource_size(&lpr->base);
      debug_printf("resource %u at %p, size %ux%ux%u: %u bytes, refcount %u\n",
                   lpr->id, (void *) lpr,
                   lpr->base.width0, lpr->base.height0, lpr->base.depth0,
                   size, lpr->base.reference.count);
      total += size;
      n++;
   }
   debug_printf("LLVMPIPE: total size of %u resources: %u\n", n, total);
}
#endif


void
llvmpipe_init_screen_resource_funcs(struct pipe_screen *screen)
{
#ifdef DEBUG
   /* init linked list for tracking resources */
   {
      static boolean first_call = TRUE;
      if (first_call) {
         memset(&resource_list, 0, sizeof(resource_list));
         make_empty_list(&resource_list);
         first_call = FALSE;
      }
   }
#endif

   screen->resource_create = llvmpipe_resource_create;
   screen->resource_destroy = llvmpipe_resource_destroy;
   screen->resource_from_handle = llvmpipe_resource_from_handle;
   screen->resource_get_handle = llvmpipe_resource_get_handle;
}


void
llvmpipe_init_context_resource_funcs(struct pipe_context *pipe)
{
   pipe->get_transfer = llvmpipe_get_transfer;
   pipe->transfer_destroy = llvmpipe_transfer_destroy;
   pipe->transfer_map = llvmpipe_transfer_map;
   pipe->transfer_unmap = llvmpipe_transfer_unmap;
 
   pipe->transfer_flush_region = u_default_transfer_flush_region;
   pipe->transfer_inline_write = u_default_transfer_inline_write;

   pipe->create_surface = llvmpipe_create_surface;
   pipe->surface_destroy = llvmpipe_surface_destroy;
}
