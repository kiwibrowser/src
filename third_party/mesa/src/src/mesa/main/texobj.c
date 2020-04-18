/**
 * \file texobj.c
 * Texture object management.
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


#include "mfeatures.h"
#include "bufferobj.h"
#include "colortab.h"
#include "context.h"
#include "enums.h"
#include "fbobject.h"
#include "formats.h"
#include "hash.h"
#include "imports.h"
#include "macros.h"
#include "teximage.h"
#include "texobj.h"
#include "texstate.h"
#include "mtypes.h"
#include "program/prog_instruction.h"



/**********************************************************************/
/** \name Internal functions */
/*@{*/


/**
 * Return the gl_texture_object for a given ID.
 */
struct gl_texture_object *
_mesa_lookup_texture(struct gl_context *ctx, GLuint id)
{
   return (struct gl_texture_object *)
      _mesa_HashLookup(ctx->Shared->TexObjects, id);
}



/**
 * Allocate and initialize a new texture object.  But don't put it into the
 * texture object hash table.
 *
 * Called via ctx->Driver.NewTextureObject, unless overridden by a device
 * driver.
 * 
 * \param shared the shared GL state structure to contain the texture object
 * \param name integer name for the texture object
 * \param target either GL_TEXTURE_1D, GL_TEXTURE_2D, GL_TEXTURE_3D,
 * GL_TEXTURE_CUBE_MAP_ARB or GL_TEXTURE_RECTANGLE_NV.  zero is ok for the sake
 * of GenTextures()
 *
 * \return pointer to new texture object.
 */
struct gl_texture_object *
_mesa_new_texture_object( struct gl_context *ctx, GLuint name, GLenum target )
{
   struct gl_texture_object *obj;
   (void) ctx;
   obj = MALLOC_STRUCT(gl_texture_object);
   _mesa_initialize_texture_object(obj, name, target);
   return obj;
}


/**
 * Initialize a new texture object to default values.
 * \param obj  the texture object
 * \param name  the texture name
 * \param target  the texture target
 */
void
_mesa_initialize_texture_object( struct gl_texture_object *obj,
                                 GLuint name, GLenum target )
{
   ASSERT(target == 0 ||
          target == GL_TEXTURE_1D ||
          target == GL_TEXTURE_2D ||
          target == GL_TEXTURE_3D ||
          target == GL_TEXTURE_CUBE_MAP_ARB ||
          target == GL_TEXTURE_RECTANGLE_NV ||
          target == GL_TEXTURE_1D_ARRAY_EXT ||
          target == GL_TEXTURE_2D_ARRAY_EXT ||
          target == GL_TEXTURE_EXTERNAL_OES ||
          target == GL_TEXTURE_BUFFER);

   memset(obj, 0, sizeof(*obj));
   /* init the non-zero fields */
   _glthread_INIT_MUTEX(obj->Mutex);
   obj->RefCount = 1;
   obj->Name = name;
   obj->Target = target;
   obj->Priority = 1.0F;
   obj->BaseLevel = 0;
   obj->MaxLevel = 1000;

   /* must be one; no support for (YUV) planes in separate buffers */
   obj->RequiredTextureImageUnits = 1;

   /* sampler state */
   if (target == GL_TEXTURE_RECTANGLE_NV ||
       target == GL_TEXTURE_EXTERNAL_OES) {
      obj->Sampler.WrapS = GL_CLAMP_TO_EDGE;
      obj->Sampler.WrapT = GL_CLAMP_TO_EDGE;
      obj->Sampler.WrapR = GL_CLAMP_TO_EDGE;
      obj->Sampler.MinFilter = GL_LINEAR;
   }
   else {
      obj->Sampler.WrapS = GL_REPEAT;
      obj->Sampler.WrapT = GL_REPEAT;
      obj->Sampler.WrapR = GL_REPEAT;
      obj->Sampler.MinFilter = GL_NEAREST_MIPMAP_LINEAR;
   }
   obj->Sampler.MagFilter = GL_LINEAR;
   obj->Sampler.MinLod = -1000.0;
   obj->Sampler.MaxLod = 1000.0;
   obj->Sampler.LodBias = 0.0;
   obj->Sampler.MaxAnisotropy = 1.0;
   obj->Sampler.CompareMode = GL_NONE;         /* ARB_shadow */
   obj->Sampler.CompareFunc = GL_LEQUAL;       /* ARB_shadow */
   obj->DepthMode = GL_LUMINANCE;
   obj->Sampler.CubeMapSeamless = GL_FALSE;
   obj->Swizzle[0] = GL_RED;
   obj->Swizzle[1] = GL_GREEN;
   obj->Swizzle[2] = GL_BLUE;
   obj->Swizzle[3] = GL_ALPHA;
   obj->_Swizzle = SWIZZLE_NOOP;
   obj->Sampler.sRGBDecode = GL_DECODE_EXT;
   obj->BufferObjectFormat = GL_LUMINANCE8;
   obj->_BufferObjectFormat = MESA_FORMAT_L8;
}


/**
 * Some texture initialization can't be finished until we know which
 * target it's getting bound to (GL_TEXTURE_1D/2D/etc).
 */
static void
finish_texture_init(struct gl_context *ctx, GLenum target,
                    struct gl_texture_object *obj)
{
   assert(obj->Target == 0);

   if (target == GL_TEXTURE_RECTANGLE_NV ||
       target == GL_TEXTURE_EXTERNAL_OES) {
      /* have to init wrap and filter state here - kind of klunky */
      obj->Sampler.WrapS = GL_CLAMP_TO_EDGE;
      obj->Sampler.WrapT = GL_CLAMP_TO_EDGE;
      obj->Sampler.WrapR = GL_CLAMP_TO_EDGE;
      obj->Sampler.MinFilter = GL_LINEAR;
      if (ctx->Driver.TexParameter) {
         static const GLfloat fparam_wrap[1] = {(GLfloat) GL_CLAMP_TO_EDGE};
         static const GLfloat fparam_filter[1] = {(GLfloat) GL_LINEAR};
         ctx->Driver.TexParameter(ctx, target, obj, GL_TEXTURE_WRAP_S, fparam_wrap);
         ctx->Driver.TexParameter(ctx, target, obj, GL_TEXTURE_WRAP_T, fparam_wrap);
         ctx->Driver.TexParameter(ctx, target, obj, GL_TEXTURE_WRAP_R, fparam_wrap);
         ctx->Driver.TexParameter(ctx, target, obj, GL_TEXTURE_MIN_FILTER, fparam_filter);
      }
   }
}


/**
 * Deallocate a texture object struct.  It should have already been
 * removed from the texture object pool.
 * Called via ctx->Driver.DeleteTexture() if not overriden by a driver.
 *
 * \param shared the shared GL state to which the object belongs.
 * \param texObj the texture object to delete.
 */
void
_mesa_delete_texture_object(struct gl_context *ctx,
                            struct gl_texture_object *texObj)
{
   GLuint i, face;

   /* Set Target to an invalid value.  With some assertions elsewhere
    * we can try to detect possible use of deleted textures.
    */
   texObj->Target = 0x99;

   /* free the texture images */
   for (face = 0; face < 6; face++) {
      for (i = 0; i < MAX_TEXTURE_LEVELS; i++) {
         if (texObj->Image[face][i]) {
            ctx->Driver.DeleteTextureImage(ctx, texObj->Image[face][i]);
         }
      }
   }

   _mesa_reference_buffer_object(ctx, &texObj->BufferObject, NULL);

   /* destroy the mutex -- it may have allocated memory (eg on bsd) */
   _glthread_DESTROY_MUTEX(texObj->Mutex);

   /* free this object */
   free(texObj);
}



/**
 * Copy texture object state from one texture object to another.
 * Use for glPush/PopAttrib.
 *
 * \param dest destination texture object.
 * \param src source texture object.
 */
void
_mesa_copy_texture_object( struct gl_texture_object *dest,
                           const struct gl_texture_object *src )
{
   dest->Target = src->Target;
   dest->Name = src->Name;
   dest->Priority = src->Priority;
   dest->Sampler.BorderColor.f[0] = src->Sampler.BorderColor.f[0];
   dest->Sampler.BorderColor.f[1] = src->Sampler.BorderColor.f[1];
   dest->Sampler.BorderColor.f[2] = src->Sampler.BorderColor.f[2];
   dest->Sampler.BorderColor.f[3] = src->Sampler.BorderColor.f[3];
   dest->Sampler.WrapS = src->Sampler.WrapS;
   dest->Sampler.WrapT = src->Sampler.WrapT;
   dest->Sampler.WrapR = src->Sampler.WrapR;
   dest->Sampler.MinFilter = src->Sampler.MinFilter;
   dest->Sampler.MagFilter = src->Sampler.MagFilter;
   dest->Sampler.MinLod = src->Sampler.MinLod;
   dest->Sampler.MaxLod = src->Sampler.MaxLod;
   dest->Sampler.LodBias = src->Sampler.LodBias;
   dest->BaseLevel = src->BaseLevel;
   dest->MaxLevel = src->MaxLevel;
   dest->Sampler.MaxAnisotropy = src->Sampler.MaxAnisotropy;
   dest->Sampler.CompareMode = src->Sampler.CompareMode;
   dest->Sampler.CompareFunc = src->Sampler.CompareFunc;
   dest->Sampler.CubeMapSeamless = src->Sampler.CubeMapSeamless;
   dest->DepthMode = src->DepthMode;
   dest->Sampler.sRGBDecode = src->Sampler.sRGBDecode;
   dest->_MaxLevel = src->_MaxLevel;
   dest->_MaxLambda = src->_MaxLambda;
   dest->GenerateMipmap = src->GenerateMipmap;
   dest->_BaseComplete = src->_BaseComplete;
   dest->_MipmapComplete = src->_MipmapComplete;
   COPY_4V(dest->Swizzle, src->Swizzle);
   dest->_Swizzle = src->_Swizzle;

   dest->RequiredTextureImageUnits = src->RequiredTextureImageUnits;
}


/**
 * Free all texture images of the given texture object.
 *
 * \param ctx GL context.
 * \param t texture object.
 *
 * \sa _mesa_clear_texture_image().
 */
void
_mesa_clear_texture_object(struct gl_context *ctx,
                           struct gl_texture_object *texObj)
{
   GLuint i, j;

   if (texObj->Target == 0)
      return;

   for (i = 0; i < MAX_FACES; i++) {
      for (j = 0; j < MAX_TEXTURE_LEVELS; j++) {
         struct gl_texture_image *texImage = texObj->Image[i][j];
         if (texImage)
            _mesa_clear_texture_image(ctx, texImage);
      }
   }
}


/**
 * Check if the given texture object is valid by examining its Target field.
 * For debugging only.
 */
static GLboolean
valid_texture_object(const struct gl_texture_object *tex)
{
   switch (tex->Target) {
   case 0:
   case GL_TEXTURE_1D:
   case GL_TEXTURE_2D:
   case GL_TEXTURE_3D:
   case GL_TEXTURE_CUBE_MAP_ARB:
   case GL_TEXTURE_RECTANGLE_NV:
   case GL_TEXTURE_1D_ARRAY_EXT:
   case GL_TEXTURE_2D_ARRAY_EXT:
   case GL_TEXTURE_BUFFER:
   case GL_TEXTURE_EXTERNAL_OES:
      return GL_TRUE;
   case 0x99:
      _mesa_problem(NULL, "invalid reference to a deleted texture object");
      return GL_FALSE;
   default:
      _mesa_problem(NULL, "invalid texture object Target 0x%x, Id = %u",
                    tex->Target, tex->Name);
      return GL_FALSE;
   }
}


/**
 * Reference (or unreference) a texture object.
 * If '*ptr', decrement *ptr's refcount (and delete if it becomes zero).
 * If 'tex' is non-null, increment its refcount.
 * This is normally only called from the _mesa_reference_texobj() macro
 * when there's a real pointer change.
 */
void
_mesa_reference_texobj_(struct gl_texture_object **ptr,
                        struct gl_texture_object *tex)
{
   assert(ptr);

   if (*ptr) {
      /* Unreference the old texture */
      GLboolean deleteFlag = GL_FALSE;
      struct gl_texture_object *oldTex = *ptr;

      ASSERT(valid_texture_object(oldTex));
      (void) valid_texture_object; /* silence warning in release builds */

      _glthread_LOCK_MUTEX(oldTex->Mutex);
      ASSERT(oldTex->RefCount > 0);
      oldTex->RefCount--;

      deleteFlag = (oldTex->RefCount == 0);
      _glthread_UNLOCK_MUTEX(oldTex->Mutex);

      if (deleteFlag) {
         GET_CURRENT_CONTEXT(ctx);
         if (ctx)
            ctx->Driver.DeleteTexture(ctx, oldTex);
         else
            _mesa_problem(NULL, "Unable to delete texture, no context");
      }

      *ptr = NULL;
   }
   assert(!*ptr);

   if (tex) {
      /* reference new texture */
      ASSERT(valid_texture_object(tex));
      _glthread_LOCK_MUTEX(tex->Mutex);
      if (tex->RefCount == 0) {
         /* this texture's being deleted (look just above) */
         /* Not sure this can every really happen.  Warn if it does. */
         _mesa_problem(NULL, "referencing deleted texture object");
         *ptr = NULL;
      }
      else {
         tex->RefCount++;
         *ptr = tex;
      }
      _glthread_UNLOCK_MUTEX(tex->Mutex);
   }
}


enum base_mipmap { BASE, MIPMAP };


/**
 * Mark a texture object as incomplete.  There are actually three kinds of
 * (in)completeness:
 * 1. "base incomplete": the base level of the texture is invalid so no
 *    texturing is possible.
 * 2. "mipmap incomplete": a non-base level of the texture is invalid so
 *    mipmap filtering isn't possible, but non-mipmap filtering is.
 * 3. "texture incompleteness": some combination of texture state and
 *    sampler state renders the texture incomplete.
 *
 * \param t  texture object
 * \param bm  either BASE or MIPMAP to indicate what's incomplete
 * \param fmt...  string describing why it's incomplete (for debugging).
 */
static void
incomplete(struct gl_texture_object *t, enum base_mipmap bm,
           const char *fmt, ...)
{
   if (MESA_DEBUG_FLAGS & DEBUG_INCOMPLETE_TEXTURE) {
      va_list args;
      char s[100];

      va_start(args, fmt);
      vsnprintf(s, sizeof(s), fmt, args);
      va_end(args);

      _mesa_debug(NULL, "Texture Obj %d incomplete because: %s\n", t->Name, s);
   }

   if (bm == BASE)
      t->_BaseComplete = GL_FALSE;
   t->_MipmapComplete = GL_FALSE;
}


/**
 * Examine a texture object to determine if it is complete.
 *
 * The gl_texture_object::Complete flag will be set to GL_TRUE or GL_FALSE
 * accordingly.
 *
 * \param ctx GL context.
 * \param t texture object.
 *
 * According to the texture target, verifies that each of the mipmaps is
 * present and has the expected size.
 */
void
_mesa_test_texobj_completeness( const struct gl_context *ctx,
                                struct gl_texture_object *t )
{
   const GLint baseLevel = t->BaseLevel;
   const struct gl_texture_image *baseImage;
   GLint maxLog2 = 0, maxLevels = 0;

   /* We'll set these to FALSE if tests fail below */
   t->_BaseComplete = GL_TRUE;
   t->_MipmapComplete = GL_TRUE;

   if (t->Target == GL_TEXTURE_BUFFER) {
      /* Buffer textures are always considered complete.  The obvious case where
       * they would be incomplete (no BO attached) is actually specced to be
       * undefined rendering results.
       */
      return;
   }

   /* Detect cases where the application set the base level to an invalid
    * value.
    */
   if ((baseLevel < 0) || (baseLevel >= MAX_TEXTURE_LEVELS)) {
      incomplete(t, BASE, "base level = %d is invalid", baseLevel);
      return;
   }

   if (t->MaxLevel < baseLevel) {
      incomplete(t, BASE, "MAX_LEVEL (%d) < BASE_LEVEL (%d)",
		 t->MaxLevel, baseLevel);
      return;
   }

   baseImage = t->Image[0][baseLevel];

   /* Always need the base level image */
   if (!baseImage) {
      incomplete(t, BASE, "Image[baseLevel=%d] == NULL", baseLevel);
      return;
   }

   /* Check width/height/depth for zero */
   if (baseImage->Width == 0 ||
       baseImage->Height == 0 ||
       baseImage->Depth == 0) {
      incomplete(t, BASE, "texture width or height or depth = 0");
      return;
   }

   /* Check if the texture values are integer */
   {
      GLenum datatype = _mesa_get_format_datatype(baseImage->TexFormat);
      t->_IsIntegerFormat = datatype == GL_INT || datatype == GL_UNSIGNED_INT;
   }

   /* Compute _MaxLevel (the maximum mipmap level we'll sample from given the
    * mipmap image sizes and GL_TEXTURE_MAX_LEVEL state).
    */
   switch (t->Target) {
   case GL_TEXTURE_1D:
   case GL_TEXTURE_1D_ARRAY_EXT:
      maxLog2 = baseImage->WidthLog2;
      maxLevels = ctx->Const.MaxTextureLevels;
      break;
   case GL_TEXTURE_2D:
   case GL_TEXTURE_2D_ARRAY_EXT:
      maxLog2 = MAX2(baseImage->WidthLog2,
                     baseImage->HeightLog2);
      maxLevels = ctx->Const.MaxTextureLevels;
      break;
   case GL_TEXTURE_3D:
      maxLog2 = MAX3(baseImage->WidthLog2,
                     baseImage->HeightLog2,
                     baseImage->DepthLog2);
      maxLevels = ctx->Const.Max3DTextureLevels;
      break;
   case GL_TEXTURE_CUBE_MAP_ARB:
      maxLog2 = MAX2(baseImage->WidthLog2,
                     baseImage->HeightLog2);
      maxLevels = ctx->Const.MaxCubeTextureLevels;
      break;
   case GL_TEXTURE_RECTANGLE_NV:
   case GL_TEXTURE_BUFFER:
   case GL_TEXTURE_EXTERNAL_OES:
      maxLog2 = 0;  /* not applicable */
      maxLevels = 1;  /* no mipmapping */
      break;
   default:
      _mesa_problem(ctx, "Bad t->Target in _mesa_test_texobj_completeness");
      return;
   }

   ASSERT(maxLevels > 0);

   t->_MaxLevel = baseLevel + maxLog2;  /* 'p' in the GL spec */
   t->_MaxLevel = MIN2(t->_MaxLevel, t->MaxLevel);
   t->_MaxLevel = MIN2(t->_MaxLevel, maxLevels - 1); /* 'q' in the GL spec */

   /* Compute _MaxLambda = q - b (see the 1.2 spec) used during mipmapping */
   t->_MaxLambda = (GLfloat) (t->_MaxLevel - baseLevel);

   if (t->Immutable) {
      /* This texture object was created with glTexStorage1/2/3D() so we
       * know that all the mipmap levels are the right size and all cube
       * map faces are the same size.
       * We don't need to do any of the additional checks below.
       */
      return;
   }

   if (t->Target == GL_TEXTURE_CUBE_MAP_ARB) {
      /* Make sure that all six cube map level 0 images are the same size.
       * Note:  we know that the image's width==height (we enforce that
       * at glTexImage time) so we only need to test the width here.
       */
      GLuint face;
      assert(baseImage->Width2 == baseImage->Height);
      for (face = 1; face < 6; face++) {
         assert(t->Image[face][baseLevel] == NULL ||
                t->Image[face][baseLevel]->Width2 ==
                t->Image[face][baseLevel]->Height2);
         if (t->Image[face][baseLevel] == NULL ||
             t->Image[face][baseLevel]->Width2 != baseImage->Width2) {
            incomplete(t, BASE, "Cube face missing or mismatched size");
            return;
         }
      }
   }

   /*
    * Do mipmap consistency checking.
    * Note: we don't care about the current texture sampler state here.
    * To determine texture completeness we'll either look at _BaseComplete
    * or _MipmapComplete depending on the current minification filter mode.
    */
   {
      GLint i;
      const GLint minLevel = baseLevel;
      const GLint maxLevel = t->_MaxLevel;
      const GLuint numFaces = _mesa_num_tex_faces(t->Target);
      GLuint width, height, depth, face;

      if (minLevel > maxLevel) {
         incomplete(t, BASE, "minLevel > maxLevel");
         return;
      }

      /* Get the base image's dimensions */
      width = baseImage->Width2;
      height = baseImage->Height2;
      depth = baseImage->Depth2;

      /* Note: this loop will be a no-op for RECT, BUFFER, EXTERNAL textures */
      for (i = baseLevel + 1; i < maxLevels; i++) {
         /* Compute the expected size of image at level[i] */
         if (width > 1) {
            width /= 2;
         }
         if (height > 1 && t->Target != GL_TEXTURE_1D_ARRAY) {
            height /= 2;
         }
         if (depth > 1 && t->Target != GL_TEXTURE_2D_ARRAY) {
            depth /= 2;
         }

         /* loop over cube faces (or single face otherwise) */
         for (face = 0; face < numFaces; face++) {
            if (i >= minLevel && i <= maxLevel) {
               const struct gl_texture_image *img = t->Image[face][i];

               if (!img) {
                  incomplete(t, MIPMAP, "TexImage[%d] is missing", i);
                  return;
               }
               if (img->TexFormat != baseImage->TexFormat) {
                  incomplete(t, MIPMAP, "Format[i] != Format[baseLevel]");
                  return;
               }
               if (img->Border != baseImage->Border) {
                  incomplete(t, MIPMAP, "Border[i] != Border[baseLevel]");
                  return;
               }
               if (img->Width2 != width) {
                  incomplete(t, MIPMAP, "TexImage[%d] bad width %u", i, img->Width2);
                  return;
               }
               if (img->Height2 != height) {
                  incomplete(t, MIPMAP, "TexImage[%d] bad height %u", i, img->Height2);
                  return;
               }
               if (img->Depth2 != depth) {
                  incomplete(t, MIPMAP, "TexImage[%d] bad depth %u", i, img->Depth2);
                  return;
               }

               /* Extra checks for cube textures */
               if (face > 0) {
                  /* check that cube faces are the same size */
                  if (img->Width2 != t->Image[0][i]->Width2 || 
                      img->Height2 != t->Image[0][i]->Height2) {
		     incomplete(t, MIPMAP, "CubeMap Image[n][i] bad size");
		     return;
		  }
               }
            }
         }
         
         if (width == 1 && height == 1 && depth == 1) {
            return;  /* found smallest needed mipmap, all done! */
         }
      }
   }
}


/**
 * Check if the given cube map texture is "cube complete" as defined in
 * the OpenGL specification.
 */
GLboolean
_mesa_cube_complete(const struct gl_texture_object *texObj)
{
   const GLint baseLevel = texObj->BaseLevel;
   const struct gl_texture_image *img0, *img;
   GLuint face;

   if (texObj->Target != GL_TEXTURE_CUBE_MAP)
      return GL_FALSE;

   if ((baseLevel < 0) || (baseLevel >= MAX_TEXTURE_LEVELS))
      return GL_FALSE;

   /* check first face */
   img0 = texObj->Image[0][baseLevel];
   if (!img0 ||
       img0->Width < 1 ||
       img0->Width != img0->Height)
      return GL_FALSE;

   /* check remaining faces vs. first face */
   for (face = 1; face < 6; face++) {
      img = texObj->Image[face][baseLevel];
      if (!img ||
          img->Width != img0->Width ||
          img->Height != img0->Height ||
          img->TexFormat != img0->TexFormat)
         return GL_FALSE;
   }

   return GL_TRUE;
}


/**
 * Mark a texture object dirty.  It forces the object to be incomplete
 * and optionally forces the context to re-validate its state.
 *
 * \param ctx GL context.
 * \param texObj texture object.
 * \param invalidate_state also invalidate context state.
 */
void
_mesa_dirty_texobj(struct gl_context *ctx, struct gl_texture_object *texObj,
                   GLboolean invalidate_state)
{
   texObj->_BaseComplete = GL_FALSE;
   texObj->_MipmapComplete = GL_FALSE;
   if (invalidate_state)
      ctx->NewState |= _NEW_TEXTURE;
}


/**
 * Return pointer to a default/fallback texture of the given type/target.
 * The texture is an RGBA texture with all texels = (0,0,0,1).
 * That's the value a GLSL sampler should get when sampling from an
 * incomplete texture.
 */
struct gl_texture_object *
_mesa_get_fallback_texture(struct gl_context *ctx, gl_texture_index tex)
{
   if (!ctx->Shared->FallbackTex[tex]) {
      /* create fallback texture now */
      const GLsizei width = 1, height = 1, depth = 1;
      GLubyte texel[4];
      struct gl_texture_object *texObj;
      struct gl_texture_image *texImage;
      gl_format texFormat;
      GLuint dims, face, numFaces = 1;
      GLenum target;

      texel[0] =
      texel[1] =
      texel[2] = 0x0;
      texel[3] = 0xff;

      switch (tex) {
      case TEXTURE_2D_ARRAY_INDEX:
         dims = 3;
         target = GL_TEXTURE_2D_ARRAY;
         break;
      case TEXTURE_1D_ARRAY_INDEX:
         dims = 2;
         target = GL_TEXTURE_1D_ARRAY;
         break;
      case TEXTURE_CUBE_INDEX:
         dims = 2;
         target = GL_TEXTURE_CUBE_MAP;
         numFaces = 6;
         break;
      case TEXTURE_3D_INDEX:
         dims = 3;
         target = GL_TEXTURE_3D;
         break;
      case TEXTURE_RECT_INDEX:
         dims = 2;
         target = GL_TEXTURE_RECTANGLE;
         break;
      case TEXTURE_2D_INDEX:
         dims = 2;
         target = GL_TEXTURE_2D;
         break;
      case TEXTURE_1D_INDEX:
         dims = 1;
         target = GL_TEXTURE_1D;
         break;
      case TEXTURE_BUFFER_INDEX:
         dims = 0;
         target = GL_TEXTURE_BUFFER;
         break;
      case TEXTURE_EXTERNAL_INDEX:
         dims = 2;
         target = GL_TEXTURE_EXTERNAL_OES;
         break;
      default:
         /* no-op */
         return NULL;
      }

      /* create texture object */
      texObj = ctx->Driver.NewTextureObject(ctx, 0, target);
      if (!texObj)
         return NULL;

      assert(texObj->RefCount == 1);
      texObj->Sampler.MinFilter = GL_NEAREST;
      texObj->Sampler.MagFilter = GL_NEAREST;

      texFormat = ctx->Driver.ChooseTextureFormat(ctx, target,
                                                  GL_RGBA, GL_RGBA,
                                                  GL_UNSIGNED_BYTE);

      /* need a loop here just for cube maps */
      for (face = 0; face < numFaces; face++) {
         GLenum faceTarget;

         if (target == GL_TEXTURE_CUBE_MAP)
            faceTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X + face;
         else
            faceTarget = target;

         /* initialize level[0] texture image */
         texImage = _mesa_get_tex_image(ctx, texObj, faceTarget, 0);

         _mesa_init_teximage_fields(ctx, texImage,
                                    width,
                                    (dims > 1) ? height : 1,
                                    (dims > 2) ? depth : 1,
                                    0, /* border */
                                    GL_RGBA, texFormat);

         ctx->Driver.TexImage(ctx, dims, texImage,
                              GL_RGBA, GL_UNSIGNED_BYTE, texel,
                              &ctx->DefaultPacking);
      }

      _mesa_test_texobj_completeness(ctx, texObj);
      assert(texObj->_BaseComplete);
      assert(texObj->_MipmapComplete);

      ctx->Shared->FallbackTex[tex] = texObj;
   }
   return ctx->Shared->FallbackTex[tex];
}


/**
 * Compute the size of the given texture object, in bytes.
 */
static GLuint
texture_size(const struct gl_texture_object *texObj)
{
   const GLuint numFaces = _mesa_num_tex_faces(texObj->Target);
   GLuint face, level, size = 0;

   for (face = 0; face < numFaces; face++) {
      for (level = 0; level < MAX_TEXTURE_LEVELS; level++) {
         const struct gl_texture_image *img = texObj->Image[face][level];
         if (img) {
            GLuint sz = _mesa_format_image_size(img->TexFormat, img->Width,
                                                img->Height, img->Depth);
            size += sz;
         }
      }
   }

   return size;
}


/**
 * Callback called from _mesa_HashWalk()
 */
static void
count_tex_size(GLuint key, void *data, void *userData)
{
   const struct gl_texture_object *texObj =
      (const struct gl_texture_object *) data;
   GLuint *total = (GLuint *) userData;

   *total = *total + texture_size(texObj);
}


/**
 * Compute total size (in bytes) of all textures for the given context.
 * For debugging purposes.
 */
GLuint
_mesa_total_texture_memory(struct gl_context *ctx)
{
   GLuint tgt, total = 0;

   _mesa_HashWalk(ctx->Shared->TexObjects, count_tex_size, &total);

   /* plus, the default texture objects */
   for (tgt = 0; tgt < NUM_TEXTURE_TARGETS; tgt++) {
      total += texture_size(ctx->Shared->DefaultTex[tgt]);
   }

   return total;
}

static struct gl_texture_object *
invalidate_tex_image_error_check(struct gl_context *ctx, GLuint texture,
                                 GLint level, const char *name)
{
   /* The GL_ARB_invalidate_subdata spec says:
    *
    *     "If <texture> is zero or is not the name of a texture, the error
    *     INVALID_VALUE is generated."
    *
    * This performs the error check in a different order than listed in the
    * spec.  We have to get the texture object before we can validate the
    * other parameters against values in the texture object.
    */
   struct gl_texture_object *const t = _mesa_lookup_texture(ctx, texture);
   if (texture == 0 || t == NULL) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(texture)", name);
      return NULL;
   }

   /* The GL_ARB_invalidate_subdata spec says:
    *
    *     "If <level> is less than zero or greater than the base 2 logarithm
    *     of the maximum texture width, height, or depth, the error
    *     INVALID_VALUE is generated."
    */
   if (level < 0 || level > t->MaxLevel) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(level)", name);
      return NULL;
   }

   /* The GL_ARB_invalidate_subdata spec says:
    *
    *     "If the target of <texture> is TEXTURE_RECTANGLE, TEXTURE_BUFFER,
    *     TEXTURE_2D_MULTISAMPLE, or TEXTURE_2D_MULTISAMPLE_ARRAY, and <level>
    *     is not zero, the error INVALID_VALUE is generated."
    */
   if (level != 0) {
      switch (t->Target) {
      case GL_TEXTURE_RECTANGLE:
      case GL_TEXTURE_BUFFER:
      case GL_TEXTURE_2D_MULTISAMPLE:
      case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
         _mesa_error(ctx, GL_INVALID_VALUE, "%s(level)", name);
         return NULL;

      default:
         break;
      }
   }

   return t;
}

/*@}*/


/***********************************************************************/
/** \name API functions */
/*@{*/


/**
 * Generate texture names.
 *
 * \param n number of texture names to be generated.
 * \param textures an array in which will hold the generated texture names.
 *
 * \sa glGenTextures().
 *
 * Calls _mesa_HashFindFreeKeyBlock() to find a block of free texture
 * IDs which are stored in \p textures.  Corresponding empty texture
 * objects are also generated.
 */ 
void GLAPIENTRY
_mesa_GenTextures( GLsizei n, GLuint *textures )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint first;
   GLint i;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (n < 0) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glGenTextures" );
      return;
   }

   if (!textures)
      return;

   /*
    * This must be atomic (generation and allocation of texture IDs)
    */
   _glthread_LOCK_MUTEX(ctx->Shared->Mutex);

   first = _mesa_HashFindFreeKeyBlock(ctx->Shared->TexObjects, n);

   /* Allocate new, empty texture objects */
   for (i = 0; i < n; i++) {
      struct gl_texture_object *texObj;
      GLuint name = first + i;
      GLenum target = 0;
      texObj = ctx->Driver.NewTextureObject(ctx, name, target);
      if (!texObj) {
         _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGenTextures");
         return;
      }

      /* insert into hash table */
      _mesa_HashInsert(ctx->Shared->TexObjects, texObj->Name, texObj);

      textures[i] = name;
   }

   _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);
}


/**
 * Check if the given texture object is bound to the current draw or
 * read framebuffer.  If so, Unbind it.
 */
static void
unbind_texobj_from_fbo(struct gl_context *ctx,
                       struct gl_texture_object *texObj)
{
   const GLuint n = (ctx->DrawBuffer == ctx->ReadBuffer) ? 1 : 2;
   GLuint i;

   for (i = 0; i < n; i++) {
      struct gl_framebuffer *fb = (i == 0) ? ctx->DrawBuffer : ctx->ReadBuffer;
      if (_mesa_is_user_fbo(fb)) {
         GLuint j;
         for (j = 0; j < BUFFER_COUNT; j++) {
            if (fb->Attachment[j].Type == GL_TEXTURE &&
                fb->Attachment[j].Texture == texObj) {
	       /* Vertices are already flushed by _mesa_DeleteTextures */
	       ctx->NewState |= _NEW_BUFFERS;
               _mesa_remove_attachment(ctx, fb->Attachment + j);         
            }
         }
      }
   }
}


/**
 * Check if the given texture object is bound to any texture image units and
 * unbind it if so (revert to default textures).
 */
static void
unbind_texobj_from_texunits(struct gl_context *ctx,
                            struct gl_texture_object *texObj)
{
   GLuint u, tex;

   for (u = 0; u < Elements(ctx->Texture.Unit); u++) {
      struct gl_texture_unit *unit = &ctx->Texture.Unit[u];
      for (tex = 0; tex < NUM_TEXTURE_TARGETS; tex++) {
         if (texObj == unit->CurrentTex[tex]) {
            _mesa_reference_texobj(&unit->CurrentTex[tex],
                                   ctx->Shared->DefaultTex[tex]);
            ASSERT(unit->CurrentTex[tex]);
            break;
         }
      }
   }
}


/**
 * Delete named textures.
 *
 * \param n number of textures to be deleted.
 * \param textures array of texture IDs to be deleted.
 *
 * \sa glDeleteTextures().
 *
 * If we're about to delete a texture that's currently bound to any
 * texture unit, unbind the texture first.  Decrement the reference
 * count on the texture object and delete it if it's zero.
 * Recall that texture objects can be shared among several rendering
 * contexts.
 */
void GLAPIENTRY
_mesa_DeleteTextures( GLsizei n, const GLuint *textures)
{
   GET_CURRENT_CONTEXT(ctx);
   GLint i;
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx); /* too complex */

   if (!textures)
      return;

   for (i = 0; i < n; i++) {
      if (textures[i] > 0) {
         struct gl_texture_object *delObj
            = _mesa_lookup_texture(ctx, textures[i]);

         if (delObj) {
            _mesa_lock_texture(ctx, delObj);

            /* Check if texture is bound to any framebuffer objects.
             * If so, unbind.
             * See section 4.4.2.3 of GL_EXT_framebuffer_object.
             */
            unbind_texobj_from_fbo(ctx, delObj);

            /* Check if this texture is currently bound to any texture units.
             * If so, unbind it.
             */
            unbind_texobj_from_texunits(ctx, delObj);

            _mesa_unlock_texture(ctx, delObj);

            ctx->NewState |= _NEW_TEXTURE;

            /* The texture _name_ is now free for re-use.
             * Remove it from the hash table now.
             */
            _glthread_LOCK_MUTEX(ctx->Shared->Mutex);
            _mesa_HashRemove(ctx->Shared->TexObjects, delObj->Name);
            _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);

            /* Unreference the texobj.  If refcount hits zero, the texture
             * will be deleted.
             */
            _mesa_reference_texobj(&delObj, NULL);
         }
      }
   }
}


/**
 * Convert a GL texture target enum such as GL_TEXTURE_2D or GL_TEXTURE_3D
 * into the corresponding Mesa texture target index.
 * Note that proxy targets are not valid here.
 * \return TEXTURE_x_INDEX or -1 if target is invalid
 */
static GLint
target_enum_to_index(struct gl_context *ctx, GLenum target)
{
   switch (target) {
   case GL_TEXTURE_1D:
      return _mesa_is_desktop_gl(ctx) ? TEXTURE_1D_INDEX : -1;
   case GL_TEXTURE_2D:
      return TEXTURE_2D_INDEX;
   case GL_TEXTURE_3D:
      return TEXTURE_3D_INDEX;
   case GL_TEXTURE_CUBE_MAP_ARB:
      return ctx->Extensions.ARB_texture_cube_map
         ? TEXTURE_CUBE_INDEX : -1;
   case GL_TEXTURE_RECTANGLE_NV:
      return _mesa_is_desktop_gl(ctx) && ctx->Extensions.NV_texture_rectangle
         ? TEXTURE_RECT_INDEX : -1;
   case GL_TEXTURE_1D_ARRAY_EXT:
      return _mesa_is_desktop_gl(ctx)
         && (ctx->Extensions.EXT_texture_array
             || ctx->Extensions.MESA_texture_array)
         ? TEXTURE_1D_ARRAY_INDEX : -1;
   case GL_TEXTURE_2D_ARRAY_EXT:
      return (_mesa_is_desktop_gl(ctx)
              && (ctx->Extensions.EXT_texture_array
                  || ctx->Extensions.MESA_texture_array))
         || _mesa_is_gles3(ctx)
         ? TEXTURE_2D_ARRAY_INDEX : -1;
   case GL_TEXTURE_BUFFER_ARB:
      return _mesa_is_desktop_gl(ctx)
         && ctx->Extensions.ARB_texture_buffer_object
         ? TEXTURE_BUFFER_INDEX : -1;
   case GL_TEXTURE_EXTERNAL_OES:
      return _mesa_is_gles(ctx) && ctx->Extensions.OES_EGL_image_external
         ? TEXTURE_EXTERNAL_INDEX : -1;
   default:
      return -1;
   }
}


/**
 * Bind a named texture to a texturing target.
 * 
 * \param target texture target.
 * \param texName texture name.
 * 
 * \sa glBindTexture().
 *
 * Determines the old texture object bound and returns immediately if rebinding
 * the same texture.  Get the current texture which is either a default texture
 * if name is null, a named texture from the hash, or a new texture if the
 * given texture name is new. Increments its reference count, binds it, and
 * calls dd_function_table::BindTexture. Decrements the old texture reference
 * count and deletes it if it reaches zero.
 */
void GLAPIENTRY
_mesa_BindTexture( GLenum target, GLuint texName )
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = _mesa_get_current_tex_unit(ctx);
   struct gl_texture_object *newTexObj = NULL;
   GLint targetIndex;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      _mesa_debug(ctx, "glBindTexture %s %d\n",
                  _mesa_lookup_enum_by_nr(target), (GLint) texName);

   targetIndex = target_enum_to_index(ctx, target);
   if (targetIndex < 0) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBindTexture(target)");
      return;
   }
   assert(targetIndex < NUM_TEXTURE_TARGETS);

   /*
    * Get pointer to new texture object (newTexObj)
    */
   if (texName == 0) {
      /* Use a default texture object */
      newTexObj = ctx->Shared->DefaultTex[targetIndex];
   }
   else {
      /* non-default texture object */
      newTexObj = _mesa_lookup_texture(ctx, texName);
      if (newTexObj) {
         /* error checking */
         if (newTexObj->Target != 0 && newTexObj->Target != target) {
            /* the named texture object's target doesn't match the given target */
            _mesa_error( ctx, GL_INVALID_OPERATION,
                         "glBindTexture(target mismatch)" );
            return;
         }
         if (newTexObj->Target == 0) {
            finish_texture_init(ctx, target, newTexObj);
         }
      }
      else {
         if (ctx->API == API_OPENGL_CORE) {
            _mesa_error(ctx, GL_INVALID_OPERATION, "glBindTexture");
            return;
         }

         /* if this is a new texture id, allocate a texture object now */
         newTexObj = ctx->Driver.NewTextureObject(ctx, texName, target);
         if (!newTexObj) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBindTexture");
            return;
         }

         /* and insert it into hash table */
         _glthread_LOCK_MUTEX(ctx->Shared->Mutex);
         _mesa_HashInsert(ctx->Shared->TexObjects, texName, newTexObj);
         _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);
      }
      newTexObj->Target = target;
   }

   assert(valid_texture_object(newTexObj));

   /* Check if this texture is only used by this context and is already bound.
    * If so, just return.
    */
   {
      GLboolean early_out;
      _glthread_LOCK_MUTEX(ctx->Shared->Mutex);
      early_out = ((ctx->Shared->RefCount == 1)
                   && (newTexObj == texUnit->CurrentTex[targetIndex]));
      _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);
      if (early_out) {
         return;
      }
   }

   /* flush before changing binding */
   FLUSH_VERTICES(ctx, _NEW_TEXTURE);

   /* Do the actual binding.  The refcount on the previously bound
    * texture object will be decremented.  It'll be deleted if the
    * count hits zero.
    */
   _mesa_reference_texobj(&texUnit->CurrentTex[targetIndex], newTexObj);
   ASSERT(texUnit->CurrentTex[targetIndex]);

   /* Pass BindTexture call to device driver */
   if (ctx->Driver.BindTexture)
      ctx->Driver.BindTexture(ctx, target, newTexObj);
}


/**
 * Set texture priorities.
 * 
 * \param n number of textures.
 * \param texName texture names.
 * \param priorities corresponding texture priorities.
 * 
 * \sa glPrioritizeTextures().
 * 
 * Looks up each texture in the hash, clamps the corresponding priority between
 * 0.0 and 1.0, and calls dd_function_table::PrioritizeTexture.
 */
void GLAPIENTRY
_mesa_PrioritizeTextures( GLsizei n, const GLuint *texName,
                          const GLclampf *priorities )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint i;
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (n < 0) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glPrioritizeTextures" );
      return;
   }

   if (!priorities)
      return;

   for (i = 0; i < n; i++) {
      if (texName[i] > 0) {
         struct gl_texture_object *t = _mesa_lookup_texture(ctx, texName[i]);
         if (t) {
            t->Priority = CLAMP( priorities[i], 0.0F, 1.0F );
         }
      }
   }

   ctx->NewState |= _NEW_TEXTURE;
}



/**
 * See if textures are loaded in texture memory.
 * 
 * \param n number of textures to query.
 * \param texName array with the texture names.
 * \param residences array which will hold the residence status.
 *
 * \return GL_TRUE if all textures are resident and \p residences is left unchanged, 
 * 
 * Note: we assume all textures are always resident
 */
GLboolean GLAPIENTRY
_mesa_AreTexturesResident(GLsizei n, const GLuint *texName,
                          GLboolean *residences)
{
   GET_CURRENT_CONTEXT(ctx);
   GLboolean allResident = GL_TRUE;
   GLint i;
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glAreTexturesResident(n)");
      return GL_FALSE;
   }

   if (!texName || !residences)
      return GL_FALSE;

   /* We only do error checking on the texture names */
   for (i = 0; i < n; i++) {
      struct gl_texture_object *t;
      if (texName[i] == 0) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glAreTexturesResident");
         return GL_FALSE;
      }
      t = _mesa_lookup_texture(ctx, texName[i]);
      if (!t) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glAreTexturesResident");
         return GL_FALSE;
      }
   }
   
   return allResident;
}


/**
 * See if a name corresponds to a texture.
 *
 * \param texture texture name.
 *
 * \return GL_TRUE if texture name corresponds to a texture, or GL_FALSE
 * otherwise.
 * 
 * \sa glIsTexture().
 *
 * Calls _mesa_HashLookup().
 */
GLboolean GLAPIENTRY
_mesa_IsTexture( GLuint texture )
{
   struct gl_texture_object *t;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   if (!texture)
      return GL_FALSE;

   t = _mesa_lookup_texture(ctx, texture);

   /* IsTexture is true only after object has been bound once. */
   return t && t->Target;
}


/**
 * Simplest implementation of texture locking: grab the shared tex
 * mutex.  Examine the shared context state timestamp and if there has
 * been a change, set the appropriate bits in ctx->NewState.
 *
 * This is used to deal with synchronizing things when a texture object
 * is used/modified by different contexts (or threads) which are sharing
 * the texture.
 *
 * See also _mesa_lock/unlock_texture() in teximage.h
 */
void
_mesa_lock_context_textures( struct gl_context *ctx )
{
   _glthread_LOCK_MUTEX(ctx->Shared->TexMutex);

   if (ctx->Shared->TextureStateStamp != ctx->TextureStateTimestamp) {
      ctx->NewState |= _NEW_TEXTURE;
      ctx->TextureStateTimestamp = ctx->Shared->TextureStateStamp;
   }
}


void
_mesa_unlock_context_textures( struct gl_context *ctx )
{
   assert(ctx->Shared->TextureStateStamp == ctx->TextureStateTimestamp);
   _glthread_UNLOCK_MUTEX(ctx->Shared->TexMutex);
}

void GLAPIENTRY
_mesa_InvalidateTexSubImage(GLuint texture, GLint level, GLint xoffset,
                            GLint yoffset, GLint zoffset, GLsizei width,
                            GLsizei height, GLsizei depth)
{
   struct gl_texture_object *t;
   struct gl_texture_image *image;
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   t = invalidate_tex_image_error_check(ctx, texture, level,
                                        "glInvalidateTexSubImage");

   /* The GL_ARB_invalidate_subdata spec says:
    *
    *     "...the specified subregion must be between -<b> and <dim>+<b> where
    *     <dim> is the size of the dimension of the texture image, and <b> is
    *     the size of the border of that texture image, otherwise
    *     INVALID_VALUE is generated (border is not applied to dimensions that
    *     don't exist in a given texture target)."
    */
   image = t->Image[0][level];
   if (image) {
      int xBorder;
      int yBorder;
      int zBorder;
      int imageWidth;
      int imageHeight;
      int imageDepth;

      /* The GL_ARB_invalidate_subdata spec says:
       *
       *     "For texture targets that don't have certain dimensions, this
       *     command treats those dimensions as having a size of 1. For
       *     example, to invalidate a portion of a two-dimensional texture,
       *     the application would use <zoffset> equal to zero and <depth>
       *     equal to one."
       */
      switch (t->Target) {
      case GL_TEXTURE_BUFFER:
         xBorder = 0;
         yBorder = 0;
         zBorder = 0;
         imageWidth = 1;
         imageHeight = 1;
         imageDepth = 1;
         break;
      case GL_TEXTURE_1D:
         xBorder = image->Border;
         yBorder = 0;
         zBorder = 0;
         imageWidth = image->Width;
         imageHeight = 1;
         imageDepth = 1;
         break;
      case GL_TEXTURE_1D_ARRAY:
         xBorder = image->Border;
         yBorder = 0;
         zBorder = 0;
         imageWidth = image->Width;
         imageHeight = image->Height;
         imageDepth = 1;
         break;
      case GL_TEXTURE_2D:
      case GL_TEXTURE_CUBE_MAP:
      case GL_TEXTURE_RECTANGLE:
      case GL_TEXTURE_2D_MULTISAMPLE:
         xBorder = image->Border;
         yBorder = image->Border;
         zBorder = 0;
         imageWidth = image->Width;
         imageHeight = image->Height;
         imageDepth = 1;
         break;
      case GL_TEXTURE_2D_ARRAY:
      case GL_TEXTURE_CUBE_MAP_ARRAY:
         xBorder = image->Border;
         yBorder = image->Border;
         zBorder = 0;
         imageWidth = image->Width;
         imageHeight = image->Height;
         imageDepth = image->Depth;
         break;
      case GL_TEXTURE_3D:
         xBorder = image->Border;
         yBorder = image->Border;
         zBorder = image->Border;
         imageWidth = image->Width;
         imageHeight = image->Height;
         imageDepth = image->Depth;
         break;
      default:
         assert(!"Should not get here.");
         xBorder = 0;
         yBorder = 0;
         zBorder = 0;
         imageWidth = 0;
         imageHeight = 0;
         imageDepth = 0;
         break;
      }

      if (xoffset < -xBorder) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glInvalidateSubTexImage(xoffset)");
         return;
      }

      if (xoffset + width > imageWidth + xBorder) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glInvalidateSubTexImage(xoffset+width)");
         return;
      }

      if (yoffset < -yBorder) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glInvalidateSubTexImage(yoffset)");
         return;
      }

      if (yoffset + height > imageHeight + yBorder) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glInvalidateSubTexImage(yoffset+height)");
         return;
      }

      if (zoffset < -zBorder) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glInvalidateSubTexImage(zoffset)");
         return;
      }

      if (zoffset + depth  > imageDepth + zBorder) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glInvalidateSubTexImage(zoffset+depth)");
         return;
      }
   }

   /* We don't actually do anything for this yet.  Just return after
    * validating the parameters and generating the required errors.
    */
   return;
}

void GLAPIENTRY
_mesa_InvalidateTexImage(GLuint texture, GLint level)
{
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   invalidate_tex_image_error_check(ctx, texture, level,
                                    "glInvalidateTexImage");

   /* We don't actually do anything for this yet.  Just return after
    * validating the parameters and generating the required errors.
    */
   return;
}

/*@}*/
