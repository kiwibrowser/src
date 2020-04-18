/*
 * (C) Copyright IBM Corporation 2002, 2004
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEM, IBM AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Ian Romanick <idr@us.ibm.com>
 */

#ifndef DRI_DEBUG_H
#define DRI_DEBUG_H

#include <GL/gl.h>
#include <GL/internal/dri_interface.h>
#include "main/context.h"

struct dri_debug_control {
    const char * string;
    unsigned     flag;
};

extern unsigned driParseDebugString( const char * debug,
    const struct dri_debug_control * control );

extern unsigned driGetRendererString( char * buffer,
    const char * hardware_name, GLuint agp_mode );

struct __DRIconfigRec {
    struct gl_config modes;
};

extern __DRIconfig **
driCreateConfigs(GLenum fb_format, GLenum fb_type,
		 const uint8_t * depth_bits, const uint8_t * stencil_bits,
		 unsigned num_depth_stencil_bits,
		 const GLenum * db_modes, unsigned num_db_modes,
		 const uint8_t * msaa_samples, unsigned num_msaa_modes,
		 GLboolean enable_accum);

__DRIconfig **driConcatConfigs(__DRIconfig **a,
			       __DRIconfig **b);

int
driGetConfigAttrib(const __DRIconfig *config,
		   unsigned int attrib, unsigned int *value);
int
driIndexConfigAttrib(const __DRIconfig *config, int index,
		     unsigned int *attrib, unsigned int *value);

#endif /* DRI_DEBUG_H */
