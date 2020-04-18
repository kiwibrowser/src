/**
 * \file points.c
 * Point operations.
 */

/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
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


#include "glheader.h"
#include "context.h"
#include "macros.h"
#include "points.h"
#include "mtypes.h"


/**
 * Set current point size.
 * \param size  point diameter in pixels
 * \sa glPointSize().
 */
void GLAPIENTRY
_mesa_PointSize( GLfloat size )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (size <= 0.0) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glPointSize" );
      return;
   }

   if (ctx->Point.Size == size)
      return;

   FLUSH_VERTICES(ctx, _NEW_POINT);
   ctx->Point.Size = size;

   if (ctx->Driver.PointSize)
      ctx->Driver.PointSize(ctx, size);
}


#if _HAVE_FULL_GL


void GLAPIENTRY
_mesa_PointParameteri( GLenum pname, GLint param )
{
   GLfloat p[3];
   p[0] = (GLfloat) param;
   p[1] = p[2] = 0.0F;
   _mesa_PointParameterfv(pname, p);
}


void GLAPIENTRY
_mesa_PointParameteriv( GLenum pname, const GLint *params )
{
   GLfloat p[3];
   p[0] = (GLfloat) params[0];
   if (pname == GL_DISTANCE_ATTENUATION_EXT) {
      p[1] = (GLfloat) params[1];
      p[2] = (GLfloat) params[2];
   }
   _mesa_PointParameterfv(pname, p);
}


void GLAPIENTRY
_mesa_PointParameterf( GLenum pname, GLfloat param)
{
   GLfloat p[3];
   p[0] = param;
   p[1] = p[2] = 0.0F;
   _mesa_PointParameterfv(pname, p);
}


void GLAPIENTRY
_mesa_PointParameterfv( GLenum pname, const GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   /* Drivers that support point sprites must also support point parameters.
    * If point parameters aren't supported, then this function shouldn't even
    * exist.
    */
   ASSERT(!(ctx->Extensions.ARB_point_sprite
            || ctx->Extensions.NV_point_sprite)
          || ctx->Extensions.EXT_point_parameters);

   if (!ctx->Extensions.EXT_point_parameters) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "unsupported function called (unsupported extension)");
      return;
   }

   switch (pname) {
      case GL_DISTANCE_ATTENUATION_EXT:
         if (TEST_EQ_3V(ctx->Point.Params, params))
            return;
         FLUSH_VERTICES(ctx, _NEW_POINT);
         COPY_3V(ctx->Point.Params, params);
         ctx->Point._Attenuated = (ctx->Point.Params[0] != 1.0 ||
                                   ctx->Point.Params[1] != 0.0 ||
                                   ctx->Point.Params[2] != 0.0);

         if (ctx->Point._Attenuated)
            ctx->_TriangleCaps |= DD_POINT_ATTEN;
         else
            ctx->_TriangleCaps &= ~DD_POINT_ATTEN;
         break;
      case GL_POINT_SIZE_MIN_EXT:
         if (params[0] < 0.0F) {
            _mesa_error( ctx, GL_INVALID_VALUE,
                         "glPointParameterf[v]{EXT,ARB}(param)" );
            return;
         }
         if (ctx->Point.MinSize == params[0])
            return;
         FLUSH_VERTICES(ctx, _NEW_POINT);
         ctx->Point.MinSize = params[0];
         break;
      case GL_POINT_SIZE_MAX_EXT:
         if (params[0] < 0.0F) {
            _mesa_error( ctx, GL_INVALID_VALUE,
                         "glPointParameterf[v]{EXT,ARB}(param)" );
            return;
         }
         if (ctx->Point.MaxSize == params[0])
            return;
         FLUSH_VERTICES(ctx, _NEW_POINT);
         ctx->Point.MaxSize = params[0];
         break;
      case GL_POINT_FADE_THRESHOLD_SIZE_EXT:
         if (params[0] < 0.0F) {
            _mesa_error( ctx, GL_INVALID_VALUE,
                         "glPointParameterf[v]{EXT,ARB}(param)" );
            return;
         }
         if (ctx->Point.Threshold == params[0])
            return;
         FLUSH_VERTICES(ctx, _NEW_POINT);
         ctx->Point.Threshold = params[0];
         break;
      case GL_POINT_SPRITE_R_MODE_NV:
         /* This is one area where ARB_point_sprite and NV_point_sprite
	  * differ.  In ARB_point_sprite the POINT_SPRITE_R_MODE is
	  * always ZERO.  NV_point_sprite adds the S and R modes.
	  */
         if (_mesa_is_desktop_gl(ctx) && ctx->Extensions.NV_point_sprite) {
            GLenum value = (GLenum) params[0];
            if (value != GL_ZERO && value != GL_S && value != GL_R) {
               _mesa_error(ctx, GL_INVALID_VALUE,
                           "glPointParameterf[v]{EXT,ARB}(param)");
               return;
            }
            if (ctx->Point.SpriteRMode == value)
               return;
            FLUSH_VERTICES(ctx, _NEW_POINT);
            ctx->Point.SpriteRMode = value;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glPointParameterf[v]{EXT,ARB}(pname)");
            return;
         }
         break;
      case GL_POINT_SPRITE_COORD_ORIGIN:
	 /* GL_POINT_SPRITE_COORD_ORIGIN was added to point sprites when the
	  * extension was merged into OpenGL 2.0.
	  */
         if ((ctx->API == API_OPENGL && ctx->Version >= 20)
             || ctx->API == API_OPENGL_CORE) {
            GLenum value = (GLenum) params[0];
            if (value != GL_LOWER_LEFT && value != GL_UPPER_LEFT) {
               _mesa_error(ctx, GL_INVALID_VALUE,
                           "glPointParameterf[v]{EXT,ARB}(param)");
               return;
            }
            if (ctx->Point.SpriteOrigin == value)
               return;
            FLUSH_VERTICES(ctx, _NEW_POINT);
            ctx->Point.SpriteOrigin = value;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glPointParameterf[v]{EXT,ARB}(pname)");
            return;
         }
         break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM,
                      "glPointParameterf[v]{EXT,ARB}(pname)" );
         return;
   }

   if (ctx->Driver.PointParameterfv)
      (*ctx->Driver.PointParameterfv)(ctx, pname, params);
}
#endif



/**
 * Initialize the context point state.
 *
 * \param ctx GL context.
 *
 * Initializes __struct gl_contextRec::Point and point related constants in
 * __struct gl_contextRec::Const.
 */
void
_mesa_init_point(struct gl_context *ctx)
{
   GLuint i;

   ctx->Point.SmoothFlag = GL_FALSE;
   ctx->Point.Size = 1.0;
   ctx->Point.Params[0] = 1.0;
   ctx->Point.Params[1] = 0.0;
   ctx->Point.Params[2] = 0.0;
   ctx->Point._Attenuated = GL_FALSE;
   ctx->Point.MinSize = 0.0;
   ctx->Point.MaxSize
      = MAX2(ctx->Const.MaxPointSize, ctx->Const.MaxPointSizeAA);
   ctx->Point.Threshold = 1.0;

   /* Page 403 (page 423 of the PDF) of the OpenGL 3.0 spec says:
    *
    *     "Non-sprite points (section 3.4) - Enable/Disable targets
    *     POINT_SMOOTH and POINT_SPRITE, and all associated state. Point
    *     rasterization is always performed as though POINT_SPRITE were
    *     enabled."
    *
    * In a core context, the state will default to true, and the setters and
    * getters are disabled.
    */
   ctx->Point.PointSprite = (ctx->API == API_OPENGL_CORE);

   ctx->Point.SpriteRMode = GL_ZERO; /* GL_NV_point_sprite (only!) */
   ctx->Point.SpriteOrigin = GL_UPPER_LEFT; /* GL_ARB_point_sprite */
   for (i = 0; i < Elements(ctx->Point.CoordReplace); i++) {
      ctx->Point.CoordReplace[i] = GL_FALSE; /* GL_ARB/NV_point_sprite */
   }
}
