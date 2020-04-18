/**********************************************************
 * Copyright 2008-2009 VMware, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **********************************************************/

/*
 * svga_cmd.h --
 *
 *      Command construction utility for the SVGA3D protocol used by
 *      the VMware SVGA device, based on the svgautil library.
 */

#ifndef __SVGA3D_H__
#define __SVGA3D_H__


#include "svga_types.h"
#include "svga_reg.h"
#include "svga3d_reg.h"

#include "pipe/p_defines.h"


struct pipe_surface;
struct svga_transfer;
struct svga_winsys_context;
struct svga_winsys_buffer;
struct svga_winsys_surface;


/*
 * SVGA Device Interoperability
 */

void *
SVGA3D_FIFOReserve(struct svga_winsys_context *swc, uint32 cmd, uint32 cmdSize, uint32 nr_relocs);

void
SVGA_FIFOCommitAll(struct svga_winsys_context *swc);


/*
 * Context Management
 */

enum pipe_error
SVGA3D_DefineContext(struct svga_winsys_context *swc);

enum pipe_error
SVGA3D_DestroyContext(struct svga_winsys_context *swc);


/*
 * Surface Management
 */

enum pipe_error
SVGA3D_BeginDefineSurface(struct svga_winsys_context *swc,
                          struct svga_winsys_surface *sid,
                          SVGA3dSurfaceFlags flags,
                          SVGA3dSurfaceFormat format,
                          SVGA3dSurfaceFace **faces,
                          SVGA3dSize **mipSizes,
                          uint32 numMipSizes);
enum pipe_error
SVGA3D_DefineSurface2D(struct svga_winsys_context *swc,
                       struct svga_winsys_surface *sid,
                       uint32 width,
                       uint32 height,
                       SVGA3dSurfaceFormat format);
enum pipe_error
SVGA3D_DestroySurface(struct svga_winsys_context *swc,
                      struct svga_winsys_surface *sid);


/*
 * Surface Operations
 */

enum pipe_error
SVGA3D_SurfaceDMA(struct svga_winsys_context *swc,
                  struct svga_transfer *st,
                  SVGA3dTransferType transfer,
                  const SVGA3dCopyBox *boxes,
                  uint32 numBoxes,
                  SVGA3dSurfaceDMAFlags flags);

enum pipe_error
SVGA3D_BufferDMA(struct svga_winsys_context *swc,
                 struct svga_winsys_buffer *guest,
                 struct svga_winsys_surface *host,
                 SVGA3dTransferType transfer,
                 uint32 size,
                 uint32 guest_offset,
                 uint32 host_offset,
                 SVGA3dSurfaceDMAFlags flags);

/*
 * Drawing Operations
 */


enum pipe_error
SVGA3D_BeginClear(struct svga_winsys_context *swc,
                  SVGA3dClearFlag flags,
                  uint32 color, float depth, uint32 stencil,
                  SVGA3dRect **rects, uint32 numRects);

enum pipe_error
SVGA3D_ClearRect(struct svga_winsys_context *swc,
                 SVGA3dClearFlag flags, uint32 color, float depth,
                 uint32 stencil, uint32 x, uint32 y, uint32 w, uint32 h);

enum pipe_error
SVGA3D_BeginDrawPrimitives(struct svga_winsys_context *swc,
                           SVGA3dVertexDecl **decls,
                           uint32 numVertexDecls,
                           SVGA3dPrimitiveRange **ranges,
                           uint32 numRanges);

/*
 * Blits
 */

enum pipe_error
SVGA3D_BeginSurfaceCopy(struct svga_winsys_context *swc,
                        struct pipe_surface *src,
                        struct pipe_surface *dest,
                        SVGA3dCopyBox **boxes, uint32 numBoxes);


enum pipe_error
SVGA3D_SurfaceStretchBlt(struct svga_winsys_context *swc,
                         struct pipe_surface *src,
                         struct pipe_surface *dest,
                         SVGA3dBox *boxSrc, SVGA3dBox *boxDest,
                         SVGA3dStretchBltMode mode);

/*
 * Shared FFP/Shader Render State
 */

enum pipe_error
SVGA3D_SetRenderTarget(struct svga_winsys_context *swc,
                       SVGA3dRenderTargetType type,
                       struct pipe_surface *surface);

enum pipe_error
SVGA3D_SetZRange(struct svga_winsys_context *swc,
                 float zMin, float zMax);

enum pipe_error
SVGA3D_SetViewport(struct svga_winsys_context *swc,
                   SVGA3dRect *rect);

enum pipe_error
SVGA3D_SetScissorRect(struct svga_winsys_context *swc,
                      SVGA3dRect *rect);

enum pipe_error
SVGA3D_SetClipPlane(struct svga_winsys_context *swc,
                    uint32 index, const float *plane);

enum pipe_error
SVGA3D_BeginSetTextureState(struct svga_winsys_context *swc,
                            SVGA3dTextureState **states,
                            uint32 numStates);

enum pipe_error
SVGA3D_BeginSetRenderState(struct svga_winsys_context *swc,
                           SVGA3dRenderState **states,
                           uint32 numStates);


/*
 * Shaders
 */

enum pipe_error
SVGA3D_DefineShader(struct svga_winsys_context *swc,
                    uint32 shid, SVGA3dShaderType type,
                    const uint32 *bytecode, uint32 bytecodeLen);

enum pipe_error
SVGA3D_DestroyShader(struct svga_winsys_context *swc,
                     uint32 shid, SVGA3dShaderType type);

enum pipe_error
SVGA3D_SetShaderConst(struct svga_winsys_context *swc,
                      uint32 reg, SVGA3dShaderType type,
                      SVGA3dShaderConstType ctype, const void *value);

enum pipe_error
SVGA3D_SetShaderConsts(struct svga_winsys_context *swc,
                       uint32 reg,
                       uint32 numRegs,
                       SVGA3dShaderType type,
                       SVGA3dShaderConstType ctype,
                       const void *values);

enum pipe_error
SVGA3D_SetShader(struct svga_winsys_context *swc,
                 SVGA3dShaderType type, uint32 shid);


/*
 * Queries
 */

enum pipe_error
SVGA3D_BeginQuery(struct svga_winsys_context *swc,
                  SVGA3dQueryType type);

enum pipe_error
SVGA3D_EndQuery(struct svga_winsys_context *swc,
                SVGA3dQueryType type,
                struct svga_winsys_buffer *buffer);

enum pipe_error
SVGA3D_WaitForQuery(struct svga_winsys_context *swc,
                    SVGA3dQueryType type,
                    struct svga_winsys_buffer *buffer);

#endif /* __SVGA3D_H__ */
