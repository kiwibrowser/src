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

#ifndef PATH_UTILS_H
#define PATH_UTILS_H

#include "VG/openvg.h"

#define SEGMENT_COMMAND(command) /* Extract segment type */     \
    ((command) & 0x1e)
#define SEGMENT_ABS_REL(command) /* Extract absolute/relative bit */ \
    ((command) & 0x1)

static INLINE VGint size_for_datatype(VGPathDatatype datatype)
{
   switch(datatype) {
   case VG_PATH_DATATYPE_S_8:
      return 1;
   case VG_PATH_DATATYPE_S_16:
      return 2;
   case VG_PATH_DATATYPE_S_32:
      return 4;
   case VG_PATH_DATATYPE_F:
      return 4;
   default:
      assert(!"unknown datatype");
   }
   return 0;
}

static INLINE VGint num_elements_for_segments(const VGubyte *segments,
                                              VGint num_segments)
{
   VGint i;
   VGint count = 0;

   for (i = 0; i < num_segments; ++i) {
      VGubyte segment = segments[i];
      VGint command = SEGMENT_COMMAND(segment);
      switch(command) {
      case VG_CLOSE_PATH:
         break;
      case VG_MOVE_TO:
         count += 2;
         break;
      case VG_LINE_TO:
         count += 2;
         break;
      case VG_HLINE_TO:
         count += 1;
         break;
      case VG_VLINE_TO:
         count += 1;
         break;
      case VG_QUAD_TO:
         count += 4;
         break;
      case VG_CUBIC_TO:
         count += 6;
         break;
      case VG_SQUAD_TO:
         count += 2;
         break;
      case VG_SCUBIC_TO:
         count += 4;
         break;
      case VG_SCCWARC_TO:
         count += 5;
         break;
      case VG_SCWARC_TO:
         count += 5;
         break;
      case VG_LCCWARC_TO:
         count += 5;
         break;
      case VG_LCWARC_TO:
         count += 5;
         break;
      default:
         assert(!"Unknown segment!");
      }
   }
   return count;
}

#endif
