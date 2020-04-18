/**************************************************************************
 *
 * Copyright 2010 Thomas Balling Sørensen & Orasanu Lucian.
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

#include <va/va.h>
#include <va/va_backend.h>

#include "va_private.h"

static struct VADriverVTable vtable =
{
   &vlVaTerminate, /* VAStatus (*vaTerminate) ( VADriverContextP ctx ); */
   &vlVaQueryConfigProfiles, /* VAStatus (*vaQueryConfigProfiles) ( VADriverContextP ctx, VAProfile *profile_list,int *num_profiles); */
   &vlVaQueryConfigEntrypoints, /* VAStatus (*vaQueryConfigEntrypoints) ( VADriverContextP ctx,	VAProfile profile, VAEntrypoint  *entrypoint_list, int *num_entrypoints	); */
   &vlVaGetConfigAttributes, /* VAStatus (*vaGetConfigAttributes) ( VADriverContextP ctx, VAProfile profile, VAEntrypoint entrypoint, VAConfigAttrib *attrib_list, int num_attribs ); */
   &vlVaCreateConfig, /* VAStatus (*vaCreateConfig) ( VADriverContextP ctx, VAProfile profile, VAEntrypoint entrypoint,	VAConfigAttrib *attrib_list, int num_attribs, VAConfigID *config_id); */
   &vlVaDestroyConfig, /* VAStatus (*vaDestroyConfig) ( VADriverContextP ctx, VAConfigID config_id); */
   &vlVaQueryConfigAttributes, /* VAStatus (*vaQueryConfigAttributes) ( VADriverContextP ctx, VAConfigID config_id, VAProfile *profile, VAEntrypoint *entrypoint, VAConfigAttrib *attrib_list, int *num_attribs); */
   &vlVaCreateSurfaces, /* VAStatus (*vaCreateSurfaces) ( VADriverContextP ctx,int width,int height,int format,int num_surfaces,VASurfaceID *surfaces); */
   &vlVaDestroySurfaces, /* VAStatus (*vaDestroySurfaces) ( VADriverContextP ctx, VASurfaceID *surface_list, int num_surfaces ); */
   &vlVaCreateContext, /* VAStatus (*vaCreateContext) (VADriverContextP ctx,VAConfigID config_id,int picture_width,int picture_height,int flag,VASurfaceID *render_targets,int num_render_targets,VAContextID *context); */
   &vlVaDestroyContext, /* VAStatus (*vaDestroyContext) (VADriverContextP ctx,VAContextID context); */
   &vlVaCreateBuffer, /* VAStatus (*vaCreateBuffer) (VADriverContextP ctx,VAContextID context,VABufferType type,unsigned int size,unsigned int num_elements,void *data,VABufferID *buf_id); */
   &vlVaBufferSetNumElements, /* VAStatus (*vaBufferSetNumElements) (VADriverContextP ctx,VABufferID buf_id,unsigned int num_elements); */
   &vlVaMapBuffer, /* VAStatus (*vaMapBuffer) (VADriverContextP ctx,VABufferID buf_id,void **pbuf); */
   &vlVaUnmapBuffer, /* VAStatus (*vaUnmapBuffer) (VADriverContextP ctx,VABufferID buf_id); */
   &vlVaDestroyBuffer, /* VAStatus (*vaDestroyBuffer) (VADriverContextP ctx,VABufferID buffer_id); */
   &vlVaBeginPicture, /* VAStatus (*vaBeginPicture) (VADriverContextP ctx,VAContextID context,VASurfaceID render_target); */
   &vlVaRenderPicture, /* VAStatus (*vaRenderPicture) (VADriverContextP ctx,VAContextID context,VABufferID *buffers,int num_buffers); */
   &vlVaEndPicture, /* VAStatus (*vaEndPicture) (VADriverContextP ctx,VAContextID context); */
   &vlVaSyncSurface, /* VAStatus (*vaSyncSurface) (VADriverContextP ctx,VASurfaceID render_target); */
   &vlVaQuerySurfaceStatus, /* VAStatus (*vaQuerySurfaceStatus) (VADriverContextP ctx,VASurfaceID render_target,VASurfaceStatus *status); */
   &vlVaPutSurface, /* VAStatus (*vaPutSurface) (
      VADriverContextP ctx,
      VASurfaceID surface,
      void* draw,
      short srcx,
      short srcy,
      unsigned short srcw,
      unsigned short srch,
      short destx,
      short desty,
      unsigned short destw,
      unsigned short desth,
      VARectangle *cliprects,
      unsigned int number_cliprects,
      unsigned int flags); */
   &vlVaQueryImageFormats, /* VAStatus (*vaQueryImageFormats) ( VADriverContextP ctx, VAImageFormat *format_list,int *num_formats); */
   &vlVaCreateImage, /* VAStatus (*vaCreateImage) (VADriverContextP ctx,VAImageFormat *format,int width,int height,VAImage *image); */
   &vlVaDeriveImage, /* VAStatus (*vaDeriveImage) (VADriverContextP ctx,VASurfaceID surface,VAImage *image); */
   &vlVaDestroyImage, /* VAStatus (*vaDestroyImage) (VADriverContextP ctx,VAImageID image); */
   &vlVaSetImagePalette, /* VAStatus (*vaSetImagePalette) (VADriverContextP ctx,VAImageID image, unsigned char *palette); */
   &vlVaGetImage, /* VAStatus (*vaGetImage) (VADriverContextP ctx,VASurfaceID surface,int x,int y,unsigned int width,unsigned int height,VAImageID image); */
   &vlVaPutImage, /* VAStatus (*vaPutImage) (
      VADriverContextP ctx,
      VASurfaceID surface,
      VAImageID image,
      int src_x,
      int src_y,
      unsigned int src_width,
      unsigned int src_height,
      int dest_x,
      int dest_y,
      unsigned int dest_width,
      unsigned int dest_height
   ); */
   &vlVaQuerySubpictureFormats,	/* VAStatus (*vaQuerySubpictureFormats) (VADriverContextP ctx,VAImageFormat *format_list,unsigned int *flags,unsigned int *num_formats); */
   &vlVaCreateSubpicture, /* VAStatus (*vaCreateSubpicture) (VADriverContextP ctx,VAImageID image,VASubpictureID *subpicture); */
   &vlVaDestroySubpicture, /* VAStatus (*vaDestroySubpicture) (VADriverContextP ctx,VASubpictureID subpicture); */
   &vlVaSubpictureImage, /* VAStatus (*vaSetSubpictureImage) (VADriverContextP ctx,VASubpictureID subpicture,VAImageID image); */
   &vlVaSetSubpictureChromakey, /* VAStatus (*vaSetSubpictureChromakey) (VADriverContextP ctx,VASubpictureID subpicture,unsigned int chromakey_min,unsigned int chromakey_max,unsigned int chromakey_mask); */
   &vlVaSetSubpictureGlobalAlpha, /* VAStatus (*vaSetSubpictureGlobalAlpha) (VADriverContextP ctx,VASubpictureID subpicture,float global_alpha); */
   &vlVaAssociateSubpicture, /* VAStatus (*vaAssociateSubpicture) (
      VADriverContextP ctx,
      VASubpictureID subpicture,
      VASurfaceID *target_surfaces,
      int num_surfaces,
      short src_x,
      short src_y,
      unsigned short src_width,
      unsigned short src_height,
      short dest_x,
      short dest_y,
      unsigned short dest_width,
      unsigned short dest_height,
      unsigned int flags); */
   &vlVaDeassociateSubpicture, /* VAStatus (*vaDeassociateSubpicture) (VADriverContextP ctx,VASubpictureID subpicture,VASurfaceID *target_surfaces,int num_surfaces); */
   &vlVaQueryDisplayAttributes, /* VAStatus (*vaQueryDisplayAttributes) (VADriverContextP ctx,VADisplayAttribute *attr_list,int *num_attributes); */
   &vlVaGetDisplayAttributes, /* VAStatus (*vaGetDisplayAttributes) (VADriverContextP ctx,VADisplayAttribute *attr_list,int num_attributes); */
   &vlVaSetDisplayAttributes, /* VAStatus (*vaSetDisplayAttributes) (VADriverContextP ctx,VADisplayAttribute *attr_list,int num_attributes); */
   &vlVaBufferInfo, /* VAStatus (*vaBufferInfo) (VADriverContextP ctx,VAContextID context,VABufferID buf_id,VABufferType *type,unsigned int *size,unsigned int *num_elements); */
   &vlVaLockSurface, /* VAStatus (*vaLockSurface) (
      VADriverContextP ctx,
      VASurfaceID surface,
      unsigned int *fourcc,
      unsigned int *luma_stride,
      unsigned int *chroma_u_stride,
      unsigned int *chroma_v_stride,
      unsigned int *luma_offset,
      unsigned int *chroma_u_offset,
      unsigned int *chroma_v_offset,
      unsigned int *buffer_name,
      void **buffer); */
   &vlVaUnlockSurface, /* VAStatus (*vaUnlockSurface) (VADriverContextP ctx,VASurfaceID surface); */
   NULL /* struct VADriverVTableGLX *glx; "Optional" */
};

struct VADriverVTable vlVaGetVtable()
{
   return vtable;
}
