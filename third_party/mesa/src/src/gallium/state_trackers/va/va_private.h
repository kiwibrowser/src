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

#ifndef VA_PRIVATE_H
#define VA_PRIVATE_H

#include <va/va.h>
#include <va/va_backend.h>

#include "pipe/p_format.h"
#include "pipe/p_state.h"

#define VA_DEBUG(_str,...) debug_printf("[Gallium VA backend]: " _str,__VA_ARGS__)
#define VA_INFO(_str,...) VA_DEBUG("INFO: " _str,__VA_ARGS__)
#define VA_WARNING(_str,...) VA_DEBUG("WARNING: " _str,__VA_ARGS__)
#define VA_ERROR(_str,...) VA_DEBUG("ERROR: " _str,__VA_ARGS__)

#define VA_MAX_IMAGE_FORMATS_SUPPORTED 2
#define VA_MAX_SUBPIC_FORMATS_SUPPORTED 2
#define VA_MAX_ENTRYPOINTS 1

#define VL_HANDLES

typedef struct {
   struct vl_screen *vscreen;
   struct pipe_surface *backbuffer;
} vlVaDriverContextPriv;

typedef struct {
   unsigned int width;
   unsigned int height;
   enum pipe_video_chroma_format format;
   VADriverContextP ctx;
} vlVaSurfacePriv;

// Public functions:
VAStatus __vaDriverInit_0_31 (VADriverContextP ctx);

// Private functions:
struct VADriverVTable vlVaGetVtable();

bool vlCreateHTAB(void);
void vlDestroyHTAB(void);
VAGenericID vlAddDataHTAB(void *data);
void* vlGetDataHTAB(VAGenericID handle);

// Vtable functions:
VAStatus vlVaTerminate (VADriverContextP ctx);
VAStatus vlVaQueryConfigProfiles (VADriverContextP ctx, VAProfile *profile_list,int *num_profiles);
VAStatus vlVaQueryConfigEntrypoints (VADriverContextP ctx, VAProfile profile, VAEntrypoint  *entrypoint_list, int *num_entrypoints);
VAStatus vlVaGetConfigAttributes (VADriverContextP ctx, VAProfile profile, VAEntrypoint entrypoint, VAConfigAttrib *attrib_list, int num_attribs);
VAStatus vlVaCreateConfig (VADriverContextP ctx, VAProfile profile, VAEntrypoint entrypoint, VAConfigAttrib *attrib_list, int num_attribs, VAConfigID *config_id);
VAStatus vlVaDestroyConfig (VADriverContextP ctx, VAConfigID config_id);
VAStatus vlVaQueryConfigAttributes (VADriverContextP ctx, VAConfigID config_id, VAProfile *profile, VAEntrypoint *entrypoint, VAConfigAttrib *attrib_list, int *num_attribs);
VAStatus vlVaCreateSurfaces (VADriverContextP ctx,int width,int height,int format,int num_surfaces,VASurfaceID *surfaces);
VAStatus vlVaDestroySurfaces (VADriverContextP ctx, VASurfaceID *surface_list, int num_surfaces);
VAStatus vlVaCreateContext (VADriverContextP ctx,VAConfigID config_id,int picture_width,int picture_height,int flag,VASurfaceID *render_targets,int num_render_targets,VAContextID *context);
VAStatus vlVaDestroyContext (VADriverContextP ctx,VAContextID context);
VAStatus vlVaCreateBuffer (VADriverContextP ctx,VAContextID context,VABufferType type,unsigned int size,unsigned int num_elements,void *data,VABufferID *buf_id);
VAStatus vlVaBufferSetNumElements (VADriverContextP ctx,VABufferID buf_id,unsigned int num_elements);
VAStatus vlVaMapBuffer (VADriverContextP ctx,VABufferID buf_id,void **pbuf);
VAStatus vlVaUnmapBuffer (VADriverContextP ctx,VABufferID buf_id);
VAStatus vlVaDestroyBuffer (VADriverContextP ctx,VABufferID buffer_id);
VAStatus vlVaBeginPicture (VADriverContextP ctx,VAContextID context,VASurfaceID render_target);
VAStatus vlVaRenderPicture (VADriverContextP ctx,VAContextID context,VABufferID *buffers,int num_buffers);
VAStatus vlVaEndPicture (VADriverContextP ctx,VAContextID context);
VAStatus vlVaSyncSurface (VADriverContextP ctx,VASurfaceID render_target);
VAStatus vlVaQuerySurfaceStatus (VADriverContextP ctx,VASurfaceID render_target,VASurfaceStatus *status);
VAStatus vlVaPutSurface (VADriverContextP ctx,
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
                         unsigned int flags);
VAStatus vlVaQueryImageFormats (VADriverContextP ctx,VAImageFormat *format_list,int *num_formats);
VAStatus vlVaQuerySubpictureFormats(VADriverContextP ctx,VAImageFormat *format_list,unsigned int *flags,unsigned int *num_formats);
VAStatus vlVaCreateImage(VADriverContextP ctx,VAImageFormat *format,int width,int height,VAImage *image);
VAStatus vlVaDeriveImage(VADriverContextP ctx,VASurfaceID surface,VAImage *image);
VAStatus vlVaDestroyImage(VADriverContextP ctx,VAImageID image);
VAStatus vlVaSetImagePalette(VADriverContextP ctx,VAImageID image, unsigned char *palette);
VAStatus vlVaGetImage(VADriverContextP ctx,VASurfaceID surface,int x,int y,unsigned int width,unsigned int height,VAImageID image);
VAStatus vlVaPutImage(VADriverContextP ctx,
                      VASurfaceID surface,
                      VAImageID image,
                      int src_x,
                      int src_y,
                      unsigned int src_width,
                      unsigned int src_height,
                      int dest_x,
                      int dest_y,
                      unsigned int dest_width,
                      unsigned int dest_height);
VAStatus vlVaQuerySubpictureFormats(VADriverContextP ctx,VAImageFormat *format_list,unsigned int *flags,unsigned int *num_formats);
VAStatus vlVaCreateSubpicture(VADriverContextP ctx,VAImageID image,VASubpictureID *subpicture);
VAStatus vlVaDestroySubpicture(VADriverContextP ctx,VASubpictureID subpicture);
VAStatus vlVaSubpictureImage(VADriverContextP ctx,VASubpictureID subpicture,VAImageID image);
VAStatus vlVaSetSubpictureChromakey(VADriverContextP ctx,VASubpictureID subpicture,unsigned int chromakey_min,unsigned int chromakey_max,unsigned int chromakey_mask);
VAStatus vlVaSetSubpictureGlobalAlpha(VADriverContextP ctx,VASubpictureID subpicture,float global_alpha);
VAStatus vlVaAssociateSubpicture(VADriverContextP ctx,
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
                                 unsigned int flags);
VAStatus vlVaDeassociateSubpicture(VADriverContextP ctx,VASubpictureID subpicture,VASurfaceID *target_surfaces,int num_surfaces);
VAStatus vlVaQueryDisplayAttributes(VADriverContextP ctx,VADisplayAttribute *attr_list,int *num_attributes);
VAStatus vlVaGetDisplayAttributes(VADriverContextP ctx,VADisplayAttribute *attr_list,int num_attributes);
VAStatus vlVaSetDisplayAttributes(VADriverContextP ctx,VADisplayAttribute *attr_list,int num_attributes);
VAStatus vlVaBufferInfo(VADriverContextP ctx,VAContextID context,VABufferID buf_id,VABufferType *type,unsigned int *size,unsigned int *num_elements);
VAStatus vlVaLockSurface(VADriverContextP ctx,
                         VASurfaceID surface,
                         unsigned int *fourcc,
                         unsigned int *luma_stride,
                         unsigned int *chroma_u_stride,
                         unsigned int *chroma_v_stride,
                         unsigned int *luma_offset,
                         unsigned int *chroma_u_offset,
                         unsigned int *chroma_v_offset,
                         unsigned int *buffer_name,
                         void **buffer);
VAStatus vlVaUnlockSurface(VADriverContextP ctx,VASurfaceID surface);

#endif //VA_PRIVATE_H
