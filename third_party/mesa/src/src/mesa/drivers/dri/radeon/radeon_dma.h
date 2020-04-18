/**************************************************************************

Copyright (C) 2004 Nicolai Haehnle.
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ATI, VA LINUX SYSTEMS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

#ifndef RADEON_DMA_H
#define RADEON_DMA_H

void radeonEmitVec4(uint32_t *out, const GLvoid * data, int stride, int count);
void radeonEmitVec8(uint32_t *out, const GLvoid * data, int stride, int count);
void radeonEmitVec12(uint32_t *out, const GLvoid * data, int stride, int count);
void radeonEmitVec16(uint32_t *out, const GLvoid * data, int stride, int count);

void rcommon_emit_vector(struct gl_context * ctx, struct radeon_aos *aos,
			 const GLvoid * data, int size, int stride, int count);
void rcommon_emit_vecfog(struct gl_context *ctx, struct radeon_aos *aos,
			 GLvoid *data, int stride, int count);

void radeonReturnDmaRegion(radeonContextPtr rmesa, int return_bytes);
void radeonRefillCurrentDmaRegion(radeonContextPtr rmesa, int size);
void radeon_init_dma(radeonContextPtr rmesa);
void radeonReturnDmaRegion(radeonContextPtr rmesa, int return_bytes);
void radeonAllocDmaRegion(radeonContextPtr rmesa,
			  struct radeon_bo **pbo, int *poffset,
			  int bytes, int alignment);
void radeonReleaseDmaRegions(radeonContextPtr rmesa);

void rcommon_flush_last_swtcl_prim(struct gl_context *ctx);

void *rcommonAllocDmaLowVerts(radeonContextPtr rmesa, int nverts, int vsize);
void radeonFreeDmaRegions(radeonContextPtr rmesa);
void radeonReleaseArrays( struct gl_context *ctx, GLuint newinputs );
#endif
