#ifndef DRI2_BUFFER_H
#define DRI2_BUFFER_H

#include "dri_util.h"

struct pipe_surface;

struct dri2_buffer
{
   __DRIbuffer base;
   struct pipe_resource *resource;
};

static INLINE struct dri2_buffer *
dri2_buffer(__DRIbuffer * driBufferPriv)
{
   return (struct dri2_buffer *) driBufferPriv;
}

#endif

/* vim: set sw=3 ts=8 sts=3 expandtab: */
