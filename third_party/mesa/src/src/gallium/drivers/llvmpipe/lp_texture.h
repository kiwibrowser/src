/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#ifndef LP_TEXTURE_H
#define LP_TEXTURE_H


#include "pipe/p_state.h"
#include "util/u_debug.h"
#include "lp_limits.h"


enum lp_texture_usage
{
   LP_TEX_USAGE_READ = 100,
   LP_TEX_USAGE_READ_WRITE,
   LP_TEX_USAGE_WRITE_ALL
};


/** Per-tile layout mode */
enum lp_texture_layout
{
   LP_TEX_LAYOUT_NONE = 0,  /**< no layout for the tile data yet */
   LP_TEX_LAYOUT_TILED,     /**< the tile data is in tiled layout */
   LP_TEX_LAYOUT_LINEAR,    /**< the tile data is in linear layout */
   LP_TEX_LAYOUT_BOTH       /**< the tile data is in both modes */
};


struct pipe_context;
struct pipe_screen;
struct llvmpipe_context;

struct sw_displaytarget;


/**
 * We keep one or two copies of the texture image data:  one in a simple
 * linear layout (for texture sampling) and another in a tiled layout (for
 * render targets).  We keep track of whether each image tile is linear
 * or tiled on a per-tile basis.
 */


/** A 1D/2D/3D image, one mipmap level */
struct llvmpipe_texture_image
{
   void *data;
};


/**
 * llvmpipe subclass of pipe_resource.  A texture, drawing surface,
 * vertex buffer, const buffer, etc.
 * Textures are stored differently than othere types of objects such as
 * vertex buffers and const buffers.
 * The former are tiled and have per-tile layout flags.
 * The later are simple malloc'd blocks of memory.
 */
struct llvmpipe_resource
{
   struct pipe_resource base;

   /** Row stride in bytes */
   unsigned row_stride[LP_MAX_TEXTURE_LEVELS];
   /** Image stride (for cube maps or 3D textures) in bytes */
   unsigned img_stride[LP_MAX_TEXTURE_LEVELS];
   unsigned tiles_per_row[LP_MAX_TEXTURE_LEVELS];
   unsigned tiles_per_image[LP_MAX_TEXTURE_LEVELS];
   /** Number of 3D slices or cube faces per level */
   unsigned num_slices_faces[LP_MAX_TEXTURE_LEVELS];

   /**
    * Display target, for textures with the PIPE_BIND_DISPLAY_TARGET
    * usage.
    */
   struct sw_displaytarget *dt;

   /**
    * Malloc'ed data for regular textures, or a mapping to dt above.
    */
   struct llvmpipe_texture_image tiled[LP_MAX_TEXTURE_LEVELS];
   struct llvmpipe_texture_image linear[LP_MAX_TEXTURE_LEVELS];

   /**
    * Data for non-texture resources.
    */
   void *data;

   /** array [level][face or slice][tile_y][tile_x] of layout values) */
   enum lp_texture_layout *layout[LP_MAX_TEXTURE_LEVELS];

   boolean userBuffer;  /** Is this a user-space buffer? */
   unsigned timestamp;

   unsigned id;  /**< temporary, for debugging */

#ifdef DEBUG
   /** for linked list */
   struct llvmpipe_resource *prev, *next;
#endif
};


struct llvmpipe_transfer
{
   struct pipe_transfer base;

   unsigned long offset;
};


/** cast wrappers */
static INLINE struct llvmpipe_resource *
llvmpipe_resource(struct pipe_resource *pt)
{
   return (struct llvmpipe_resource *) pt;
}


static INLINE const struct llvmpipe_resource *
llvmpipe_resource_const(const struct pipe_resource *pt)
{
   return (const struct llvmpipe_resource *) pt;
}


static INLINE struct llvmpipe_transfer *
llvmpipe_transfer(struct pipe_transfer *pt)
{
   return (struct llvmpipe_transfer *) pt;
}


void llvmpipe_init_screen_resource_funcs(struct pipe_screen *screen);
void llvmpipe_init_context_resource_funcs(struct pipe_context *pipe);

static INLINE unsigned
llvmpipe_resource_stride(struct pipe_resource *resource,
                        unsigned level)
{
   struct llvmpipe_resource *lpr = llvmpipe_resource(resource);
   assert(level < LP_MAX_TEXTURE_2D_LEVELS);
   return lpr->row_stride[level];
}


void *
llvmpipe_resource_map(struct pipe_resource *resource,
                      unsigned level,
                      unsigned layer,
                      enum lp_texture_usage tex_usage,
                      enum lp_texture_layout layout);

void
llvmpipe_resource_unmap(struct pipe_resource *resource,
                       unsigned level,
                       unsigned layer);


void *
llvmpipe_resource_data(struct pipe_resource *resource);


unsigned
llvmpipe_resource_size(const struct pipe_resource *resource);


ubyte *
llvmpipe_get_texture_image_address(struct llvmpipe_resource *lpr,
                                    unsigned face_slice, unsigned level,
                                    enum lp_texture_layout layout);

void *
llvmpipe_get_texture_image(struct llvmpipe_resource *resource,
                            unsigned face_slice, unsigned level,
                            enum lp_texture_usage usage,
                            enum lp_texture_layout layout);

void *
llvmpipe_get_texture_image_all(struct llvmpipe_resource *lpr,
                               unsigned level,
                               enum lp_texture_usage usage,
                               enum lp_texture_layout layout);

ubyte *
llvmpipe_get_texture_tile_linear(struct llvmpipe_resource *lpr,
                                  unsigned face_slice, unsigned level,
                                  enum lp_texture_usage usage,
                                  unsigned x, unsigned y);

ubyte *
llvmpipe_get_texture_tile(struct llvmpipe_resource *lpr,
                           unsigned face_slice, unsigned level,
                           enum lp_texture_usage usage,
                           unsigned x, unsigned y);


void
llvmpipe_unswizzle_cbuf_tile(struct llvmpipe_resource *lpr,
                             unsigned face_slice, unsigned level,
                             unsigned x, unsigned y,
                             uint8_t *tile);

void
llvmpipe_swizzle_cbuf_tile(struct llvmpipe_resource *lpr,
                           unsigned face_slice, unsigned level,
                           unsigned x, unsigned y,
                           uint8_t *tile);

extern void
llvmpipe_print_resources(void);


extern void
llvmpipe_init_screen_texture_funcs(struct pipe_screen *screen);

extern void
llvmpipe_init_context_texture_funcs(struct pipe_context *pipe);


#define LP_UNREFERENCED         0
#define LP_REFERENCED_FOR_READ  (1 << 0)
#define LP_REFERENCED_FOR_WRITE (1 << 1)

unsigned int
llvmpipe_is_resource_referenced( struct pipe_context *pipe,
                                 struct pipe_resource *presource,
                                 unsigned level, int layer);

#endif /* LP_TEXTURE_H */
