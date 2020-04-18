/**************************************************************************
 *
 * Copyright 2009 Younes Manton.
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

#include <assert.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/extensions/Xvlib.h>
#include <X11/extensions/XvMClib.h>

#include "vl/vl_compositor.h"

#include "xvmc_private.h"

#define XV_BRIGHTNESS "XV_BRIGHTNESS"
#define XV_CONTRAST   "XV_CONTRAST"
#define XV_SATURATION "XV_SATURATION"
#define XV_HUE        "XV_HUE"
#define XV_COLORSPACE "XV_COLORSPACE"

static const XvAttribute attributes[] = {
   { XvGettable | XvSettable, -1000, 1000, XV_BRIGHTNESS },
   { XvGettable | XvSettable, -1000, 1000, XV_CONTRAST },
   { XvGettable | XvSettable, -1000, 1000, XV_SATURATION },
   { XvGettable | XvSettable, -1000, 1000, XV_HUE },
   { XvGettable | XvSettable, 0, 1, XV_COLORSPACE }
};

PUBLIC
XvAttribute* XvMCQueryAttributes(Display *dpy, XvMCContext *context, int *number)
{
   XvAttribute *result;

   assert(dpy && number);

   if (!context || !context->privData)
      return NULL;

   result = malloc(sizeof(attributes));
   if (!result)
      return NULL;

   memcpy(result, attributes, sizeof(attributes));
   *number = sizeof(attributes) / sizeof(XvAttribute);

   XVMC_MSG(XVMC_TRACE, "[XvMC] Returning %d attributes for context %p.\n", *number, context);

   return result;
}

PUBLIC
Status XvMCSetAttribute(Display *dpy, XvMCContext *context, Atom attribute, int value)
{
   XvMCContextPrivate *context_priv;
   const char *attr;
   vl_csc_matrix csc;

   assert(dpy);

   if (!context || !context->privData)
      return XvMCBadContext;

   context_priv = context->privData;

   attr = XGetAtomName(dpy, attribute);
   if (!attr)
      return XvMCBadContext;

   if (strcmp(attr, XV_BRIGHTNESS))
      context_priv->procamp.brightness = value / 1000.0f;
   else if (strcmp(attr, XV_CONTRAST))
      context_priv->procamp.contrast = value / 1000.0f + 1.0f;
   else if (strcmp(attr, XV_SATURATION))
      context_priv->procamp.saturation = value / 1000.0f + 1.0f;
   else if (strcmp(attr, XV_HUE))
      context_priv->procamp.hue = value / 1000.0f;
   else if (strcmp(attr, XV_COLORSPACE))
      context_priv->color_standard = value ?
         VL_CSC_COLOR_STANDARD_BT_601 :
         VL_CSC_COLOR_STANDARD_BT_709;
   else
      return BadName;

   vl_csc_get_matrix
   (
      context_priv->color_standard,
      &context_priv->procamp, true, &csc
   );
   vl_compositor_set_csc_matrix(&context_priv->cstate, (const vl_csc_matrix *)&csc);

   XVMC_MSG(XVMC_TRACE, "[XvMC] Set attribute %s to value %d.\n", attr, value);

   return Success;
}

PUBLIC
Status XvMCGetAttribute(Display *dpy, XvMCContext *context, Atom attribute, int *value)
{
   XvMCContextPrivate *context_priv;
   const char *attr;

   assert(dpy);

   if (!context || !context->privData)
      return XvMCBadContext;

   context_priv = context->privData;

   attr = XGetAtomName(dpy, attribute);
   if (!attr)
      return XvMCBadContext;

   if (strcmp(attr, XV_BRIGHTNESS))
      *value = context_priv->procamp.brightness * 1000;
   else if (strcmp(attr, XV_CONTRAST))
      *value = context_priv->procamp.contrast * 1000 - 1000;
   else if (strcmp(attr, XV_SATURATION))
      *value = context_priv->procamp.saturation * 1000 + 1000;
   else if (strcmp(attr, XV_HUE))
      *value = context_priv->procamp.hue * 1000;
   else if (strcmp(attr, XV_COLORSPACE))
      *value = context_priv->color_standard == VL_CSC_COLOR_STANDARD_BT_709;
   else
      return BadName;

   XVMC_MSG(XVMC_TRACE, "[XvMC] Got value %d for attribute %s.\n", *value, attr);

   return Success;
}
