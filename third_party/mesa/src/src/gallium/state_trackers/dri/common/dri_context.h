/**************************************************************************
 *
 * Copyright (C) 2009 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/
/*
 * Author: Keith Whitwell <keithw@vmware.com>
 * Author: Jakob Bornecrantz <wallbraker@gmail.com>
 */

#ifndef DRI_CONTEXT_H
#define DRI_CONTEXT_H

#include "dri_util.h"
#include "pipe/p_compiler.h"
#include "postprocess/filters.h"

struct pipe_context;
struct pipe_fence;
struct st_api;
struct st_context_iface;
struct dri_drawable;

struct dri_context
{
   /* dri */
   __DRIscreen *sPriv;
   __DRIcontext *cPriv;
   __DRIdrawable *dPriv;
   __DRIdrawable *rPriv;

   driOptionCache optionCache;

   unsigned int bind_count;

   /* gallium */
   struct st_api *stapi;
   struct st_context_iface *st;
   struct pp_queue_t *pp;
   unsigned int pp_enabled[PP_FILTERS];
};

static INLINE struct dri_context *
dri_context(__DRIcontext * driContextPriv)
{
   if (!driContextPriv)
     return NULL;
   return (struct dri_context *)driContextPriv->driverPrivate;
}

/***********************************************************************
 * dri_context.c
 */
void dri_destroy_context(__DRIcontext * driContextPriv);

boolean dri_unbind_context(__DRIcontext * driContextPriv);

boolean
dri_make_current(__DRIcontext * driContextPriv,
		 __DRIdrawable * driDrawPriv,
		 __DRIdrawable * driReadPriv);

struct dri_context *
dri_get_current(__DRIscreen * driScreenPriv);

boolean
dri_create_context(gl_api api,
		   const struct gl_config * visual,
		   __DRIcontext * driContextPriv,
		   unsigned major_version,
		   unsigned minor_version,
		   uint32_t flags,
		   unsigned *error,
		   void *sharedContextPrivate);

#endif

/* vim: set sw=3 ts=8 sts=3 expandtab: */
