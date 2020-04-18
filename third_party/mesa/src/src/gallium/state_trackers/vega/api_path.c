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
#include "handle.h"
#include "path.h"
#include "api.h"

#include "pipe/p_context.h"

VGPath vegaCreatePath(VGint pathFormat,
                      VGPathDatatype datatype,
                      VGfloat scale, VGfloat bias,
                      VGint segmentCapacityHint,
                      VGint coordCapacityHint,
                      VGbitfield capabilities)
{
   struct vg_context *ctx = vg_current_context();

   if (pathFormat != VG_PATH_FORMAT_STANDARD) {
      vg_set_error(ctx, VG_UNSUPPORTED_PATH_FORMAT_ERROR);
      return VG_INVALID_HANDLE;
   }
   if (datatype < VG_PATH_DATATYPE_S_8 ||
       datatype > VG_PATH_DATATYPE_F) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return VG_INVALID_HANDLE;
   }
   if (!scale) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return VG_INVALID_HANDLE;
   }

   return path_to_handle(path_create(datatype, scale, bias,
                                     segmentCapacityHint, coordCapacityHint,
                                     capabilities));
}

void vegaClearPath(VGPath path, VGbitfield capabilities)
{
   struct vg_context *ctx = vg_current_context();
   struct path *p = 0;

   if (path == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   p = handle_to_path(path);
   path_clear(p, capabilities);
}

void vegaDestroyPath(VGPath p)
{
   struct path *path = 0;
   struct vg_context *ctx = vg_current_context();

   if (p == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   path = handle_to_path(p);
   path_destroy(path);
}

void vegaRemovePathCapabilities(VGPath path,
                                VGbitfield capabilities)
{
   struct vg_context *ctx = vg_current_context();
   VGbitfield current;
   struct path *p;

   if (path == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   p = handle_to_path(path);
   current = path_capabilities(p);
   path_set_capabilities(p, (current &
                             (~(capabilities & VG_PATH_CAPABILITY_ALL))));
}

VGbitfield vegaGetPathCapabilities(VGPath path)
{
   struct vg_context *ctx = vg_current_context();
   struct path *p = 0;

   if (path == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return 0;
   }
   p = handle_to_path(path);
   return path_capabilities(p);
}

void vegaAppendPath(VGPath dstPath, VGPath srcPath)
{
   struct vg_context *ctx = vg_current_context();
   struct path *src, *dst;

   if (dstPath == VG_INVALID_HANDLE || srcPath == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }
   src = handle_to_path(srcPath);
   dst = handle_to_path(dstPath);

   if (!(path_capabilities(src) & VG_PATH_CAPABILITY_APPEND_FROM) ||
       !(path_capabilities(dst) & VG_PATH_CAPABILITY_APPEND_TO)) {
      vg_set_error(ctx, VG_PATH_CAPABILITY_ERROR);
      return;
   }
   path_append_path(dst, src);
}

void vegaAppendPathData(VGPath dstPath,
                        VGint numSegments,
                        const VGubyte * pathSegments,
                        const void * pathData)
{
   struct vg_context *ctx = vg_current_context();
   struct path *p = 0;
   VGint i;

   if (dstPath == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }
   if (!pathSegments) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }
   if (numSegments <= 0) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }
   for (i = 0; i < numSegments; ++i) {
      if (pathSegments[i] > VG_LCWARC_TO_REL) {
         vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
         return;
      }
   }

   p = handle_to_path(dstPath);

   if (!p || !is_aligned_to(p, path_datatype_size(p))) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   if (!(path_capabilities(p)&VG_PATH_CAPABILITY_APPEND_TO)) {
      vg_set_error(ctx, VG_PATH_CAPABILITY_ERROR);
      return;
   }

   path_append_data(p, numSegments, pathSegments, pathData);
}

void vegaModifyPathCoords(VGPath dstPath,
                          VGint startIndex,
                          VGint numSegments,
                          const void * pathData)
{
   struct vg_context *ctx = vg_current_context();
   struct path *p = 0;

   if (dstPath == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }
   if (startIndex < 0 || numSegments <= 0) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   p = handle_to_path(dstPath);

   if (!pathData || !is_aligned_to(pathData, path_datatype_size(p))) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   if (startIndex + numSegments > path_num_segments(p)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }
   if (!(path_capabilities(p)&VG_PATH_CAPABILITY_MODIFY)) {
      vg_set_error(ctx, VG_PATH_CAPABILITY_ERROR);
      return;
   }
   path_modify_coords(p, startIndex, numSegments, pathData);
}

void vegaTransformPath(VGPath dstPath, VGPath srcPath)
{
   struct vg_context *ctx = vg_current_context();
   struct path *src = 0, *dst = 0;

   if (dstPath == VG_INVALID_HANDLE || srcPath == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }
   src = handle_to_path(srcPath);
   dst = handle_to_path(dstPath);

   if (!(path_capabilities(src) & VG_PATH_CAPABILITY_TRANSFORM_FROM) ||
       !(path_capabilities(dst) & VG_PATH_CAPABILITY_TRANSFORM_TO)) {
      vg_set_error(ctx, VG_PATH_CAPABILITY_ERROR);
      return;
   }
   path_transform(dst, src);
}

VGboolean vegaInterpolatePath(VGPath dstPath,
                              VGPath startPath,
                              VGPath endPath,
                              VGfloat amount)
{
   struct vg_context *ctx = vg_current_context();
   struct path *start = 0, *dst = 0, *end = 0;

   if (dstPath == VG_INVALID_HANDLE ||
       startPath == VG_INVALID_HANDLE ||
       endPath == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return VG_FALSE;
   }
   dst = handle_to_path(dstPath);
   start = handle_to_path(startPath);
   end = handle_to_path(endPath);

   if (!(path_capabilities(dst) & VG_PATH_CAPABILITY_INTERPOLATE_TO) ||
       !(path_capabilities(start) & VG_PATH_CAPABILITY_INTERPOLATE_FROM) ||
       !(path_capabilities(end) & VG_PATH_CAPABILITY_INTERPOLATE_FROM)) {
      vg_set_error(ctx, VG_PATH_CAPABILITY_ERROR);
      return VG_FALSE;
   }

   return path_interpolate(dst,
                           start, end, amount);
}

VGfloat vegaPathLength(VGPath path,
                       VGint startSegment,
                       VGint numSegments)
{
   struct vg_context *ctx = vg_current_context();
   struct path *p = 0;

   if (path == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return -1;
   }
   if (startSegment < 0) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return -1;
   }
   if (numSegments <= 0) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return -1;
   }
   p = handle_to_path(path);

   if (!(path_capabilities(p) & VG_PATH_CAPABILITY_PATH_LENGTH)) {
      vg_set_error(ctx, VG_PATH_CAPABILITY_ERROR);
      return -1;
   }
   if (startSegment + numSegments > path_num_segments(p)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return -1;
   }

   return path_length(p, startSegment, numSegments);
}

void vegaPointAlongPath(VGPath path,
                        VGint startSegment,
                        VGint numSegments,
                        VGfloat distance,
                        VGfloat * x, VGfloat * y,
                        VGfloat * tangentX,
                        VGfloat * tangentY)
{
   struct vg_context *ctx = vg_current_context();
   struct path *p = 0;
   VGbitfield caps;

   if (path == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }
   if (startSegment < 0) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }
   if (numSegments <= 0) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   if (!is_aligned(x) || !is_aligned(y) ||
       !is_aligned(tangentX) || !is_aligned(tangentY)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   p = handle_to_path(path);

   caps = path_capabilities(p);
   if (!(caps & VG_PATH_CAPABILITY_POINT_ALONG_PATH) ||
       !(caps & VG_PATH_CAPABILITY_TANGENT_ALONG_PATH)) {
      vg_set_error(ctx, VG_PATH_CAPABILITY_ERROR);
      return;
   }

   if (startSegment + numSegments > path_num_segments(p)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   {
      VGfloat point[2], normal[2];
      path_point(p, startSegment, numSegments, distance,
                 point, normal);
      if (x)
         *x = point[0];
      if (y)
         *y = point[1];
      if (tangentX)
         *tangentX = -normal[1];
      if (tangentY)
         *tangentY = normal[0];
   }
}

void vegaPathBounds(VGPath path,
                    VGfloat * minX,
                    VGfloat * minY,
                    VGfloat * width,
                    VGfloat * height)
{
   struct vg_context *ctx = vg_current_context();
   struct path *p = 0;
   VGbitfield caps;

   if (path == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   if (!minX || !minY || !width || !height) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   if (!is_aligned(minX) || !is_aligned(minY) ||
       !is_aligned(width) || !is_aligned(height)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   p = handle_to_path(path);

   caps = path_capabilities(p);
   if (!(caps & VG_PATH_CAPABILITY_PATH_BOUNDS)) {
      vg_set_error(ctx, VG_PATH_CAPABILITY_ERROR);
      return;
   }

   path_bounding_rect(p, minX, minY, width, height);
}

void vegaPathTransformedBounds(VGPath path,
                               VGfloat * minX,
                               VGfloat * minY,
                               VGfloat * width,
                               VGfloat * height)
{
   struct vg_context *ctx = vg_current_context();
   struct path *p = 0;
   VGbitfield caps;

   if (path == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   if (!minX || !minY || !width || !height) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   if (!is_aligned(minX) || !is_aligned(minY) ||
       !is_aligned(width) || !is_aligned(height)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   p = handle_to_path(path);

   caps = path_capabilities(p);
   if (!(caps & VG_PATH_CAPABILITY_PATH_TRANSFORMED_BOUNDS)) {
      vg_set_error(ctx, VG_PATH_CAPABILITY_ERROR);
      return;
   }

#if 0
   /* faster, but seems to have precision problems... */
   path_bounding_rect(p, minX, minY, width, height);
   if (*width > 0 && *height > 0) {
      VGfloat pts[] = {*minX,          *minY,
                       *minX + *width, *minY,
                       *minX + *width, *minY + *height,
                       *minX,          *minY + *height};
      struct matrix *matrix = &ctx->state.vg.path_user_to_surface_matrix;
      VGfloat maxX, maxY;
      matrix_map_point(matrix, pts[0], pts[1], pts + 0, pts + 1);
      matrix_map_point(matrix, pts[2], pts[3], pts + 2, pts + 3);
      matrix_map_point(matrix, pts[4], pts[5], pts + 4, pts + 5);
      matrix_map_point(matrix, pts[6], pts[7], pts + 6, pts + 7);
      *minX = MIN2(pts[0], MIN2(pts[2], MIN2(pts[4], pts[6])));
      *minY = MIN2(pts[1], MIN2(pts[3], MIN2(pts[5], pts[7])));
      maxX = MAX2(pts[0], MAX2(pts[2], MAX2(pts[4], pts[6])));
      maxY = MAX2(pts[1], MAX2(pts[3], MAX2(pts[5], pts[7])));
      *width  = maxX - *minX;
      *height = maxY - *minY;
   }
#else
   {
      struct path *dst = path_create(VG_PATH_DATATYPE_F, 1.0, 0,
                                     0, 0, VG_PATH_CAPABILITY_ALL);
      path_transform(dst, p);
      path_bounding_rect(dst, minX, minY, width, height);
      path_destroy(dst);
   }
#endif
}


void vegaDrawPath(VGPath path, VGbitfield paintModes)
{
   struct vg_context *ctx = vg_current_context();
   struct path *p = handle_to_path(path);

   if (path == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   if (!(paintModes & (VG_STROKE_PATH | VG_FILL_PATH))) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   if (path_is_empty(p))
      return;
   path_render(p, paintModes,
         &ctx->state.vg.path_user_to_surface_matrix);
}

