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

#include <X11/Xlib.h>
#include <X11/extensions/XvMClib.h>

#include "util/u_memory.h"

#include "xvmc_private.h"

PUBLIC
Status XvMCCreateBlocks(Display *dpy, XvMCContext *context, unsigned int num_blocks, XvMCBlockArray *blocks)
{
   assert(dpy);

   if (!context)
      return XvMCBadContext;
   if (num_blocks == 0)
      return BadValue;

   assert(blocks);

   blocks->context_id = context->context_id;
   blocks->num_blocks = num_blocks;
   blocks->blocks = MALLOC(BLOCK_SIZE_BYTES * num_blocks);
   blocks->privData = NULL;

   return Success;
}

PUBLIC
Status XvMCDestroyBlocks(Display *dpy, XvMCBlockArray *blocks)
{
   assert(dpy);
   assert(blocks);
   FREE(blocks->blocks);

   return Success;
}

PUBLIC
Status XvMCCreateMacroBlocks(Display *dpy, XvMCContext *context, unsigned int num_blocks, XvMCMacroBlockArray *blocks)
{
   assert(dpy);

   if (!context)
      return XvMCBadContext;
   if (num_blocks == 0)
      return BadValue;

   assert(blocks);

   blocks->context_id = context->context_id;
   blocks->num_blocks = num_blocks;
   blocks->macro_blocks = MALLOC(sizeof(XvMCMacroBlock) * num_blocks);
   blocks->privData = NULL;

   return Success;
}

PUBLIC
Status XvMCDestroyMacroBlocks(Display *dpy, XvMCMacroBlockArray *blocks)
{
   assert(dpy);
   assert(blocks);
   FREE(blocks->macro_blocks);

   return Success;
}
