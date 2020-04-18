/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#ifndef INTEL_BLIT_H
#define INTEL_BLIT_H

#include "intel_context.h"

extern void intelCopyBuffer(const __DRIdrawable * dpriv,
                            const drm_clip_rect_t * rect);

extern GLbitfield intelClearWithBlit(struct gl_context * ctx, GLbitfield mask);

bool
intelEmitCopyBlit(struct intel_context *intel,
                              GLuint cpp,
                              GLshort src_pitch,
                              drm_intel_bo *src_buffer,
                              GLuint src_offset,
			      uint32_t src_tiling,
                              GLshort dst_pitch,
                              drm_intel_bo *dst_buffer,
                              GLuint dst_offset,
			      uint32_t dst_tiling,
                              GLshort srcx, GLshort srcy,
                              GLshort dstx, GLshort dsty,
                              GLshort w, GLshort h,
			      GLenum logicop );

bool
intelEmitImmediateColorExpandBlit(struct intel_context *intel,
				  GLuint cpp,
				  GLubyte *src_bits, GLuint src_size,
				  GLuint fg_color,
				  GLshort dst_pitch,
				  drm_intel_bo *dst_buffer,
				  GLuint dst_offset,
				  uint32_t dst_tiling,
				  GLshort x, GLshort y,
				  GLshort w, GLshort h,
				  GLenum logic_op);
void intel_emit_linear_blit(struct intel_context *intel,
			    drm_intel_bo *dst_bo,
			    unsigned int dst_offset,
			    drm_intel_bo *src_bo,
			    unsigned int src_offset,
			    unsigned int size);
void intel_set_teximage_alpha_to_one(struct gl_context *ctx,
				     struct intel_texture_image *intel_image);

#endif
