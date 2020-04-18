/**
 * \file texobj.h
 * Texture object management.
 */

/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#ifndef TEXTOBJ_H
#define TEXTOBJ_H


#include "compiler.h"
#include "glheader.h"
#include "mtypes.h"
#include "samplerobj.h"


/**
 * \name Internal functions
 */
/*@{*/

extern struct gl_texture_object *
_mesa_lookup_texture(struct gl_context *ctx, GLuint id);

extern struct gl_texture_object *
_mesa_new_texture_object( struct gl_context *ctx, GLuint name, GLenum target );

extern void
_mesa_initialize_texture_object( struct gl_texture_object *obj,
                                 GLuint name, GLenum target );

extern void
_mesa_delete_texture_object( struct gl_context *ctx,
                             struct gl_texture_object *obj );

extern void
_mesa_copy_texture_object( struct gl_texture_object *dest,
                           const struct gl_texture_object *src );

extern void
_mesa_clear_texture_object(struct gl_context *ctx,
                           struct gl_texture_object *obj);

extern void
_mesa_reference_texobj_(struct gl_texture_object **ptr,
                        struct gl_texture_object *tex);

static inline void
_mesa_reference_texobj(struct gl_texture_object **ptr,
                       struct gl_texture_object *tex)
{
   if (*ptr != tex)
      _mesa_reference_texobj_(ptr, tex);
}


/**
 * Return number of faces for a texture target.  This will be 6 for
 * cube maps (and cube map arrays) and 1 otherwise.
 */
static inline GLuint
_mesa_num_tex_faces(GLenum target)
{
   return target == GL_TEXTURE_CUBE_MAP ? 6 : 1;
}


/** Is the texture "complete" with respect to the given sampler state? */
static inline GLboolean
_mesa_is_texture_complete(const struct gl_texture_object *texObj,
                          const struct gl_sampler_object *sampler)
{
   if (texObj->_IsIntegerFormat &&
       (sampler->MagFilter != GL_NEAREST ||
        (sampler->MinFilter != GL_NEAREST &&
         sampler->MinFilter != GL_NEAREST_MIPMAP_NEAREST))) {
      /* If the format is integer, only nearest filtering is allowed */
      return GL_FALSE;
   }

   if (_mesa_is_mipmap_filter(sampler))
      return texObj->_MipmapComplete;
   else
      return texObj->_BaseComplete;
}


extern void
_mesa_test_texobj_completeness( const struct gl_context *ctx,
                                struct gl_texture_object *obj );

extern GLboolean
_mesa_cube_complete(const struct gl_texture_object *texObj);

extern void
_mesa_dirty_texobj(struct gl_context *ctx, struct gl_texture_object *texObj,
                   GLboolean invalidate_state);

extern struct gl_texture_object *
_mesa_get_fallback_texture(struct gl_context *ctx, gl_texture_index tex);

extern GLuint
_mesa_total_texture_memory(struct gl_context *ctx);

extern void
_mesa_unlock_context_textures( struct gl_context *ctx );

extern void
_mesa_lock_context_textures( struct gl_context *ctx );

/*@}*/

/**
 * \name API functions
 */
/*@{*/

extern void GLAPIENTRY
_mesa_GenTextures( GLsizei n, GLuint *textures );


extern void GLAPIENTRY
_mesa_DeleteTextures( GLsizei n, const GLuint *textures );


extern void GLAPIENTRY
_mesa_BindTexture( GLenum target, GLuint texture );


extern void GLAPIENTRY
_mesa_PrioritizeTextures( GLsizei n, const GLuint *textures,
                          const GLclampf *priorities );


extern GLboolean GLAPIENTRY
_mesa_AreTexturesResident( GLsizei n, const GLuint *textures,
                           GLboolean *residences );

extern GLboolean GLAPIENTRY
_mesa_IsTexture( GLuint texture );

extern void GLAPIENTRY
_mesa_InvalidateTexSubImage(GLuint texture, GLint level, GLint xoffset,
                            GLint yoffset, GLint zoffset, GLsizei width,
                            GLsizei height, GLsizei depth);

extern void GLAPIENTRY
_mesa_InvalidateTexImage(GLuint texture, GLint level);

/*@}*/


#endif
