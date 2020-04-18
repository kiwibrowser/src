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
#include "api.h"

/* Hardware Queries */
VGHardwareQueryResult vegaHardwareQuery(VGHardwareQueryType key,
                                        VGint setting)
{
   struct vg_context *ctx = vg_current_context();

   if (key < VG_IMAGE_FORMAT_QUERY ||
       key > VG_PATH_DATATYPE_QUERY) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return VG_HARDWARE_UNACCELERATED;
   }

   if (key == VG_IMAGE_FORMAT_QUERY) {
      if (setting < VG_sRGBX_8888 ||
          setting > VG_lABGR_8888_PRE) {
         vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
         return VG_HARDWARE_UNACCELERATED;
      }
   } else if (key == VG_PATH_DATATYPE_QUERY) {
      if (setting < VG_PATH_DATATYPE_S_8 ||
          setting > VG_PATH_DATATYPE_F) {
         vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
         return VG_HARDWARE_UNACCELERATED;
      }
   }
   /* we're supposed to accelerate everything */
   return VG_HARDWARE_ACCELERATED;
}

/* Renderer and Extension Information */
const VGubyte *vegaGetString(VGStringID name)
{
   struct vg_context *ctx = vg_current_context();
   static const VGubyte *vendor = (VGubyte *)"Tungsten Graphics, Inc";
   static const VGubyte *renderer = (VGubyte *)"Vega OpenVG 1.1";
   static const VGubyte *version = (VGubyte *)"1.1";

   if (!ctx)
      return NULL;

   switch(name) {
   case VG_VENDOR:
      return vendor;
   case VG_RENDERER:
      return renderer;
   case VG_VERSION:
      return version;
   case VG_EXTENSIONS:
      return NULL;
   default:
      return NULL;
   }
}
