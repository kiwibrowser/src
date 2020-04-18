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
#include "VG/vgu.h"

#include "matrix.h"
#include "path.h"
#include "handle.h"

#include "util/u_debug.h"
#include "util/u_pointer.h"

#include <math.h>
#include <assert.h>


static void vgu_append_float_coords(VGPath path,
                                    const VGubyte *cmds,
                                    VGint num_cmds,
                                    const VGfloat *coords,
                                    VGint num_coords)
{
   VGubyte common_data[40 * sizeof(VGfloat)];
   struct path *p = handle_to_path(path);

   vg_float_to_datatype(path_datatype(p), common_data, coords, num_coords);
   vgAppendPathData(path, num_cmds, cmds, common_data);
}

VGUErrorCode vguLine(VGPath path,
                     VGfloat x0, VGfloat y0,
                     VGfloat x1, VGfloat y1)
{
   static const VGubyte cmds[] = {VG_MOVE_TO_ABS, VG_LINE_TO_ABS};
   VGfloat coords[4];
   VGbitfield caps;

   if (path == VG_INVALID_HANDLE) {
      return VGU_BAD_HANDLE_ERROR;
   }
   caps = vgGetPathCapabilities(path);
   if (!(caps & VG_PATH_CAPABILITY_APPEND_TO)) {
      return VGU_PATH_CAPABILITY_ERROR;
   }

   coords[0] = x0;
   coords[1] = y0;
   coords[2] = x1;
   coords[3] = y1;

   vgu_append_float_coords(path, cmds, 2, coords, 4);

   return VGU_NO_ERROR;
}

VGUErrorCode vguPolygon(VGPath path,
                        const VGfloat * points,
                        VGint count,
                        VGboolean closed)
{
   VGubyte *cmds;
   VGfloat *coords;
   VGbitfield caps;
   VGint i;

   if (path == VG_INVALID_HANDLE) {
      return VGU_BAD_HANDLE_ERROR;
   }

   if (!points || count <= 0 || !is_aligned(points)) {
      return VGU_ILLEGAL_ARGUMENT_ERROR;
   }

   caps = vgGetPathCapabilities(path);
   if (!(caps & VG_PATH_CAPABILITY_APPEND_TO)) {
      return VGU_PATH_CAPABILITY_ERROR;
   }

   cmds   = malloc(sizeof(VGubyte) * count + 1);
   coords = malloc(sizeof(VGfloat) * count * 2);

   cmds[0] = VG_MOVE_TO_ABS;
   coords[0] = points[0];
   coords[1] = points[1];
   for (i = 1; i < count; ++i) {
      cmds[i] = VG_LINE_TO_ABS;
      coords[2*i + 0] = points[2*i + 0];
      coords[2*i + 1] = points[2*i + 1];
   }

   if (closed) {
      cmds[i] = VG_CLOSE_PATH;
      ++i;
   }

   vgu_append_float_coords(path, cmds, i, coords, 2*i);

   free(cmds);
   free(coords);

   return VGU_NO_ERROR;
}

VGUErrorCode  vguRect(VGPath path,
                      VGfloat x, VGfloat y,
                      VGfloat width, VGfloat height)
{
   static const VGubyte cmds[] = {VG_MOVE_TO_ABS,
                                  VG_HLINE_TO_REL,
                                  VG_VLINE_TO_REL,
                                  VG_HLINE_TO_REL,
                                  VG_CLOSE_PATH
   };
   VGfloat coords[5];
   VGbitfield caps;

   if (path == VG_INVALID_HANDLE) {
      return VGU_BAD_HANDLE_ERROR;
   }
   caps = vgGetPathCapabilities(path);
   if (!(caps & VG_PATH_CAPABILITY_APPEND_TO)) {
      return VGU_PATH_CAPABILITY_ERROR;
   }
   if (width <= 0 || height <= 0) {
      return VGU_ILLEGAL_ARGUMENT_ERROR;
   }

   coords[0] =  x;
   coords[1] =  y;
   coords[2] =  width;
   coords[3] =  height;
   coords[4] = -width;

   vgu_append_float_coords(path, cmds, 5, coords, 5);

   return VGU_NO_ERROR;
}

VGUErrorCode vguRoundRect(VGPath path,
                          VGfloat x, VGfloat y,
                          VGfloat width,
                          VGfloat height,
                          VGfloat arcWidth,
                          VGfloat arcHeight)
{
   static const VGubyte cmds[] = {VG_MOVE_TO_ABS,
                                  VG_HLINE_TO_REL,
                                  VG_SCCWARC_TO_REL,
                                  VG_VLINE_TO_REL,
                                  VG_SCCWARC_TO_REL,
                                  VG_HLINE_TO_REL,
                                  VG_SCCWARC_TO_REL,
                                  VG_VLINE_TO_REL,
                                  VG_SCCWARC_TO_REL,
                                  VG_CLOSE_PATH
   };
   VGfloat c[26];
   VGbitfield caps;

   if (path == VG_INVALID_HANDLE) {
      return VGU_BAD_HANDLE_ERROR;
   }
   caps = vgGetPathCapabilities(path);
   if (!(caps & VG_PATH_CAPABILITY_APPEND_TO)) {
      return VGU_PATH_CAPABILITY_ERROR;
   }
   if (width <= 0 || height <= 0) {
      return VGU_ILLEGAL_ARGUMENT_ERROR;
   }

   c[0] =  x + arcWidth/2; c[1] =  y;

   c[2] = width - arcWidth;

   c[3] = arcWidth/2; c[4] = arcHeight/2; c[5] = 0;
   c[6] = arcWidth/2; c[7] = arcHeight/2;

   c[8] = height - arcHeight;

   c[9] = arcWidth/2; c[10] = arcHeight/2; c[11] = 0;
   c[12] = -arcWidth/2; c[13] = arcHeight/2;

   c[14] = -(width - arcWidth);

   c[15] = arcWidth/2; c[16] = arcHeight/2; c[17] = 0;
   c[18] = -arcWidth/2; c[19] = -arcHeight/2;

   c[20] = -(height - arcHeight);

   c[21] = arcWidth/2; c[22] = arcHeight/2; c[23] = 0;
   c[24] = arcWidth/2; c[25] = -arcHeight/2;

   vgu_append_float_coords(path, cmds, 10, c, 26);

   return VGU_NO_ERROR;
}

VGUErrorCode vguEllipse(VGPath path,
                        VGfloat cx, VGfloat cy,
                        VGfloat width,
                        VGfloat height)
{
   static const VGubyte cmds[] = {VG_MOVE_TO_ABS,
                                  VG_SCCWARC_TO_REL,
                                  VG_SCCWARC_TO_REL,
                                  VG_CLOSE_PATH
   };
   VGfloat coords[12];
   VGbitfield caps;

   if (path == VG_INVALID_HANDLE) {
      return VGU_BAD_HANDLE_ERROR;
   }
   caps = vgGetPathCapabilities(path);
   if (!(caps & VG_PATH_CAPABILITY_APPEND_TO)) {
      return VGU_PATH_CAPABILITY_ERROR;
   }
   if (width <= 0 || height <= 0) {
      return VGU_ILLEGAL_ARGUMENT_ERROR;
   }

   coords[0] = cx + width/2; coords[1] = cy;

   coords[2] = width/2; coords[3] = height/2; coords[4] = 0;
   coords[5] = -width; coords[6] = 0;

   coords[7] = width/2; coords[8] = height/2; coords[9] = 0;
   coords[10] = width; coords[11] = 0;

   vgu_append_float_coords(path, cmds, 4, coords, 11);

   return VGU_NO_ERROR;
}

VGUErrorCode vguArc(VGPath path,
                    VGfloat x, VGfloat y,
                    VGfloat width, VGfloat height,
                    VGfloat startAngle,
                    VGfloat angleExtent,
                    VGUArcType arcType)
{
   VGubyte cmds[11];
   VGfloat coords[40];
   VGbitfield caps;
   VGfloat last = startAngle + angleExtent;
   VGint i, c = 0;

   if (path == VG_INVALID_HANDLE) {
      return VGU_BAD_HANDLE_ERROR;
   }
   caps = vgGetPathCapabilities(path);
   if (!(caps & VG_PATH_CAPABILITY_APPEND_TO)) {
      return VGU_PATH_CAPABILITY_ERROR;
   }
   if (width <= 0 || height <= 0) {
      return VGU_ILLEGAL_ARGUMENT_ERROR;
   }
   if (arcType != VGU_ARC_OPEN &&
       arcType != VGU_ARC_CHORD &&
       arcType != VGU_ARC_PIE) {
      return VGU_ILLEGAL_ARGUMENT_ERROR;
   }

   cmds[c] = VG_MOVE_TO_ABS; ++c;
   coords[0] = x+cos(DEGREES_TO_RADIANS(startAngle))*width/2;
   coords[1] = y+sin(DEGREES_TO_RADIANS(startAngle))*height/2;
#ifdef DEBUG_VGUARC
   debug_printf("start [%f, %f]\n", coords[0], coords[1]);
#endif
   i = 2;
   if (angleExtent > 0) {
      VGfloat angle = startAngle + 180;
      while (angle < last) {
         cmds[c] = VG_SCCWARC_TO_ABS; ++c;
         coords[i] = width/2; coords[i+1] = height/2; coords[i+2] = 0;
         coords[i+3] = x + cos(DEGREES_TO_RADIANS(angle))*width/2;
         coords[i+4] = y + sin(DEGREES_TO_RADIANS(angle))*height/2;
#ifdef DEBUG_VGUARC
         debug_printf("1 [%f, %f]\n", coords[i+3],
                      coords[i+4]);
#endif
         i += 5;
         angle += 180;
      }
      cmds[c] = VG_SCCWARC_TO_ABS; ++c;
      coords[i] = width/2; coords[i+1] = height/2; coords[i+2] = 0;
      coords[i+3] = x+cos(DEGREES_TO_RADIANS(last))*width/2;
      coords[i+4] = y+sin(DEGREES_TO_RADIANS(last))*height/2;
#ifdef DEBUG_VGUARC
      debug_printf("2 [%f, %f]\n", coords[i+3],
                   coords[i+4]);
#endif
      i += 5;
   } else {
      VGfloat angle = startAngle - 180;
      while (angle > last) {
         cmds[c] = VG_SCWARC_TO_ABS; ++c;
         coords[i] =  width/2; coords[i+1] = height/2; coords[i+2] = 0;
         coords[i+3] = x + cos(DEGREES_TO_RADIANS(angle)) * width/2;
         coords[i+4] = y + sin(DEGREES_TO_RADIANS(angle)) * height/2;
#ifdef DEBUG_VGUARC
         debug_printf("3 [%f, %f]\n", coords[i+3],
                      coords[i+4]);
#endif
         angle -= 180;
         i += 5;
      }
      cmds[c] = VG_SCWARC_TO_ABS; ++c;
      coords[i] = width/2; coords[i+1] = height/2; coords[i+2] = 0;
      coords[i+3] = x + cos(DEGREES_TO_RADIANS(last)) * width/2;
      coords[i+4] = y + sin(DEGREES_TO_RADIANS(last)) * height/2;
#ifdef DEBUG_VGUARC
      debug_printf("4 [%f, %f]\n", coords[i+3],
                   coords[i+4]);
#endif
      i += 5;
   }

   if (arcType == VGU_ARC_PIE) {
      cmds[c] = VG_LINE_TO_ABS; ++c;
      coords[i] = x; coords[i + 1] = y;
      i += 2;
   }
   if (arcType == VGU_ARC_PIE || arcType == VGU_ARC_CHORD) {
      cmds[c] = VG_CLOSE_PATH;
      ++c;
   }

   assert(c < 11);

   vgu_append_float_coords(path, cmds, c, coords, i);

   return VGU_NO_ERROR;
}

VGUErrorCode vguComputeWarpQuadToSquare(VGfloat sx0, VGfloat sy0,
                                        VGfloat sx1, VGfloat sy1,
                                        VGfloat sx2, VGfloat sy2,
                                        VGfloat sx3, VGfloat sy3,
                                        VGfloat * matrix)
{
   struct matrix mat;

   if (!matrix || !is_aligned(matrix))
      return VGU_ILLEGAL_ARGUMENT_ERROR;

   if (!matrix_quad_to_square(sx0, sy0,
                              sx1, sy1,
                              sx2, sy2,
                              sx3, sy3,
                              &mat))
      return VGU_BAD_WARP_ERROR;

   if (!matrix_is_invertible(&mat))
      return VGU_BAD_WARP_ERROR;

   memcpy(matrix, mat.m, sizeof(VGfloat) * 9);

   return VGU_NO_ERROR;
}

VGUErrorCode vguComputeWarpSquareToQuad(VGfloat dx0, VGfloat dy0,
                                        VGfloat dx1, VGfloat dy1,
                                        VGfloat dx2, VGfloat dy2,
                                        VGfloat dx3, VGfloat dy3,
                                        VGfloat * matrix)
{
   struct matrix mat;

   if (!matrix || !is_aligned(matrix))
      return VGU_ILLEGAL_ARGUMENT_ERROR;

   if (!matrix_square_to_quad(dx0, dy0,
                              dx1, dy1,
                              dx2, dy2,
                              dx3, dy3,
                              &mat))
      return VGU_BAD_WARP_ERROR;

   if (!matrix_is_invertible(&mat))
      return VGU_BAD_WARP_ERROR;

   memcpy(matrix, mat.m, sizeof(VGfloat) * 9);

   return VGU_NO_ERROR;
}

VGUErrorCode vguComputeWarpQuadToQuad(VGfloat dx0, VGfloat dy0,
                                      VGfloat dx1, VGfloat dy1,
                                      VGfloat dx2, VGfloat dy2,
                                      VGfloat dx3, VGfloat dy3,
                                      VGfloat sx0, VGfloat sy0,
                                      VGfloat sx1, VGfloat sy1,
                                      VGfloat sx2, VGfloat sy2,
                                      VGfloat sx3, VGfloat sy3,
                                      VGfloat * matrix)
{
   struct matrix mat;

   if (!matrix || !is_aligned(matrix))
      return VGU_ILLEGAL_ARGUMENT_ERROR;

   if (!matrix_quad_to_quad(dx0, dy0,
                            dx1, dy1,
                            dx2, dy2,
                            dx3, dy3,
                            sx0, sy0,
                            sx1, sy1,
                            sx2, sy2,
                            sx3, sy3,
                            &mat))
      return VGU_BAD_WARP_ERROR;

   memcpy(matrix, mat.m, sizeof(VGfloat) * 9);

   return VGU_NO_ERROR;
}
