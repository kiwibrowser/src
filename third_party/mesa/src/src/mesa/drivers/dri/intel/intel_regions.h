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

#ifndef INTEL_REGIONS_H
#define INTEL_REGIONS_H

/** @file intel_regions.h
 *
 * Structure definitions and prototypes for intel_region handling,
 * which is the basic structure for rectangular collections of pixels
 * stored in a drm_intel_bo.
 */

#include <stdbool.h>
#include <xf86drm.h>

#include "main/mtypes.h"
#include "intel_bufmgr.h"

#ifdef __cplusplus
extern "C" {
#endif

struct intel_context;
struct intel_buffer_object;

/**
 * A layer on top of the bufmgr buffers that adds a few useful things:
 *
 * - Refcounting for local buffer references.
 * - Refcounting for buffer maps
 * - Buffer dimensions - pitch and height.
 * - Blitter commands for copying 2D regions between buffers. (really???)
 */
struct intel_region
{
   drm_intel_bo *bo;  /**< buffer manager's buffer */
   GLuint refcount; /**< Reference count for region */
   GLuint cpp;      /**< bytes per pixel */
   GLuint width;    /**< in pixels */
   GLuint height;   /**< in pixels */
   GLuint pitch;    /**< in pixels */
   GLubyte *map;    /**< only non-NULL when region is actually mapped */
   GLuint map_refcount;  /**< Reference count for mapping */

   uint32_t tiling; /**< Which tiling mode the region is in */

   uint32_t name; /**< Global name for the bo */
   struct intel_screen *screen;
};


/* Allocate a refcounted region.  Pointers to regions should only be
 * copied by calling intel_reference_region().
 */
struct intel_region *intel_region_alloc(struct intel_screen *screen,
                                        uint32_t tiling,
					GLuint cpp, GLuint width,
                                        GLuint height,
					bool expect_accelerated_upload);

struct intel_region *
intel_region_alloc_for_handle(struct intel_screen *screen,
			      GLuint cpp,
			      GLuint width, GLuint height, GLuint pitch,
			      unsigned int handle, const char *name);

bool
intel_region_flink(struct intel_region *region, uint32_t *name);

void intel_region_reference(struct intel_region **dst,
                            struct intel_region *src);

void intel_region_release(struct intel_region **ib);

void intel_recreate_static_regions(struct intel_context *intel);

/**
 * Map/unmap regions.  This is refcounted also:
 *
 * \param mode  bitmask of GL_MAP_READ_BIT, GL_MAP_WRITE_BIT
 */
void *intel_region_map(struct intel_context *intel,
		       struct intel_region *ib,
		       GLbitfield mode);

void intel_region_unmap(struct intel_context *intel, struct intel_region *ib);

/* Copy rectangular sub-regions
 */
bool
intel_region_copy(struct intel_context *intel,
		  struct intel_region *dest,
		  GLuint dest_offset,
		  GLuint destx, GLuint desty,
		  struct intel_region *src,
		  GLuint src_offset,
		  GLuint srcx, GLuint srcy, GLuint width, GLuint height,
		  bool flip,
		  GLenum logicop);

void _mesa_copy_rect(GLubyte * dst,
                GLuint cpp,
                GLuint dst_pitch,
                GLuint dst_x,
                GLuint dst_y,
                GLuint width,
                GLuint height,
                const GLubyte * src,
                GLuint src_pitch, GLuint src_x, GLuint src_y);

void
intel_region_get_tile_masks(struct intel_region *region,
                            uint32_t *mask_x, uint32_t *mask_y,
                            bool map_stencil_as_y_tiled);

uint32_t
intel_region_get_aligned_offset(struct intel_region *region, uint32_t x,
                                uint32_t y, bool map_stencil_as_y_tiled);

/**
 * Used with images created with image_from_names
 * to help support planar images.
 */
struct intel_image_format {
   int fourcc;
   int components;
   int nplanes;
   struct {
      int buffer_index;
      int width_shift;
      int height_shift;
      uint32_t dri_format;
      int cpp;
   } planes[3];
};

struct __DRIimageRec {
   struct intel_region *region;
   GLenum internal_format;
   uint32_t dri_format;
   GLuint format;
   uint32_t offset;

   /*
    * Need to save these here between calls to
    * image_from_names and calls to image_from_planar.
    */
   uint32_t strides[3];
   uint32_t offsets[3];
   struct intel_image_format *planar_format;

   void *data;
};

#ifdef __cplusplus
}
#endif

#endif
