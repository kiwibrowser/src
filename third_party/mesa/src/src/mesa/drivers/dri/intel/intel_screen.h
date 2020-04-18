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

#ifndef _INTEL_INIT_H_
#define _INTEL_INIT_H_

#include <stdbool.h>
#include <sys/time.h>
#include "dri_util.h"
#include "intel_bufmgr.h"
#include "i915_drm.h"
#include "xmlconfig.h"

struct intel_screen
{
   int deviceID;
   int gen;

   int logTextureGranularity;

   __DRIscreen *driScrnPriv;

   bool no_hw;
   GLuint relaxed_relocations;

   /*
    * The hardware hiz and separate stencil fields are needed in intel_screen,
    * rather than solely in intel_context, because glXCreatePbuffer and
    * glXCreatePixmap are not passed a GLXContext.
    */
   bool hw_has_separate_stencil;
   bool hw_must_use_separate_stencil;

   bool kernel_has_gen7_sol_reset;

   bool hw_has_llc;
   bool hw_has_swizzling;

   bool no_vbo;
   dri_bufmgr *bufmgr;
   struct _mesa_HashTable *named_regions;

   /**
   * Configuration cache with default values for all contexts
   */
   driOptionCache optionCache;
};

extern bool intelMapScreenRegions(__DRIscreen * sPriv);

extern void intelDestroyContext(__DRIcontext * driContextPriv);

extern GLboolean intelUnbindContext(__DRIcontext * driContextPriv);

extern GLboolean
intelMakeCurrent(__DRIcontext * driContextPriv,
                 __DRIdrawable * driDrawPriv,
                 __DRIdrawable * driReadPriv);

double get_time(void);
void aub_dump_bmp(struct gl_context *ctx);

#endif
