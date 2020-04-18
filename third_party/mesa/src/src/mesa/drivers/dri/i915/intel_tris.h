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

#ifndef INTELTRIS_INC
#define INTELTRIS_INC

#include "main/mtypes.h"

#define INTEL_VB_SIZE		(32 * 1024)
/** 3 dwords of state_immediate and 2 of 3dprim, in intel_flush_prim */
#define INTEL_PRIM_EMIT_SIZE	(5 * 4)

#define _INTEL_NEW_RENDERSTATE (_NEW_LINE | \
                                _NEW_POLYGON | \
                                _NEW_LIGHT | \
                                _NEW_PROGRAM | \
                                _NEW_POLYGONSTIPPLE)

extern void intelInitTriFuncs(struct gl_context * ctx);

extern void intelChooseRenderState(struct gl_context * ctx);

void intel_set_prim(struct intel_context *intel, uint32_t prim);
GLuint *intel_get_prim_space(struct intel_context *intel, unsigned int count);
void intel_flush_prim(struct intel_context *intel);
void intel_finish_vb(struct intel_context *intel);

#endif
