/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2011  VMware, Inc.  All Rights Reserved.
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



#ifndef SAMPLEROBJ_H
#define SAMPLEROBJ_H

struct dd_function_table;

static inline struct gl_sampler_object *
_mesa_get_samplerobj(struct gl_context *ctx, GLuint unit)
{
   if (ctx->Texture.Unit[unit].Sampler)
      return ctx->Texture.Unit[unit].Sampler;
   else if (ctx->Texture.Unit[unit]._Current)
      return &ctx->Texture.Unit[unit]._Current->Sampler;
   else
      return NULL;
}


/** Does the given filter state do mipmap filtering? */
static inline GLboolean
_mesa_is_mipmap_filter(const struct gl_sampler_object *samp)
{
   return samp->MinFilter != GL_NEAREST && samp->MinFilter != GL_LINEAR;
}


extern void
_mesa_reference_sampler_object_(struct gl_context *ctx,
                                struct gl_sampler_object **ptr,
                                struct gl_sampler_object *samp);

static inline void
_mesa_reference_sampler_object(struct gl_context *ctx,
                               struct gl_sampler_object **ptr,
                               struct gl_sampler_object *samp)
{
   if (*ptr != samp)
      _mesa_reference_sampler_object_(ctx, ptr, samp);
}


extern void
_mesa_init_sampler_object(struct gl_sampler_object *sampObj, GLuint name);

extern struct gl_sampler_object *
_mesa_new_sampler_object(struct gl_context *ctx, GLuint name);

extern void
_mesa_delete_sampler_object(struct gl_context *ctx,
                            struct gl_sampler_object *sampObj);

extern void
_mesa_init_sampler_object_functions(struct dd_function_table *driver);

extern void
_mesa_init_sampler_object_dispatch(struct _glapi_table *disp);

extern void GLAPIENTRY
_mesa_BindSampler(GLuint unit, GLuint sampler);

extern void GLAPIENTRY
_mesa_GenSamplers(GLsizei count, GLuint *samplers);

extern void GLAPIENTRY
_mesa_DeleteSamplers(GLsizei count, const GLuint *samplers);

extern void GLAPIENTRY
_mesa_SamplerParameteri(GLuint sampler, GLenum pname, GLint param);

#endif /* SAMPLEROBJ_H */
