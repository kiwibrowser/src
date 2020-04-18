/*
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#include "main/glheader.h"
#include "main/imports.h"
#include "main/colormac.h"
#include "main/context.h"
#include "main/enums.h"
#include "main/image.h"
#include "main/mfeatures.h"
#include "main/simple_list.h"
#include "main/teximage.h"
#include "main/texobj.h"
#include "main/samplerobj.h"

#include "radeon_mipmap_tree.h"
#include "r200_context.h"
#include "r200_ioctl.h"
#include "r200_tex.h"

#include "xmlpool.h"



/**
 * Set the texture wrap modes.
 * 
 * \param t Texture object whose wrap modes are to be set
 * \param swrap Wrap mode for the \a s texture coordinate
 * \param twrap Wrap mode for the \a t texture coordinate
 */

static void r200SetTexWrap( radeonTexObjPtr t, GLenum swrap, GLenum twrap, GLenum rwrap )
{
   GLboolean  is_clamp = GL_FALSE;
   GLboolean  is_clamp_to_border = GL_FALSE;
   struct gl_texture_object *tObj = &t->base;

   radeon_print(RADEON_TEXTURE, RADEON_TRACE,
		"%s(tex %p) sw %s, tw %s, rw %s\n",
		__func__, t,
		_mesa_lookup_enum_by_nr(swrap),
		_mesa_lookup_enum_by_nr(twrap),
		_mesa_lookup_enum_by_nr(rwrap));

   t->pp_txfilter &= ~(R200_CLAMP_S_MASK | R200_CLAMP_T_MASK | R200_BORDER_MODE_D3D);

   switch ( swrap ) {
   case GL_REPEAT:
      t->pp_txfilter |= R200_CLAMP_S_WRAP;
      break;
   case GL_CLAMP:
      t->pp_txfilter |= R200_CLAMP_S_CLAMP_GL;
      is_clamp = GL_TRUE;
      break;
   case GL_CLAMP_TO_EDGE:
      t->pp_txfilter |= R200_CLAMP_S_CLAMP_LAST;
      break;
   case GL_CLAMP_TO_BORDER:
      t->pp_txfilter |= R200_CLAMP_S_CLAMP_GL;
      is_clamp_to_border = GL_TRUE;
      break;
   case GL_MIRRORED_REPEAT:
      t->pp_txfilter |= R200_CLAMP_S_MIRROR;
      break;
   case GL_MIRROR_CLAMP_EXT:
      t->pp_txfilter |= R200_CLAMP_S_MIRROR_CLAMP_GL;
      is_clamp = GL_TRUE;
      break;
   case GL_MIRROR_CLAMP_TO_EDGE_EXT:
      t->pp_txfilter |= R200_CLAMP_S_MIRROR_CLAMP_LAST;
      break;
   case GL_MIRROR_CLAMP_TO_BORDER_EXT:
      t->pp_txfilter |= R200_CLAMP_S_MIRROR_CLAMP_GL;
      is_clamp_to_border = GL_TRUE;
      break;
   default:
      _mesa_problem(NULL, "bad S wrap mode in %s", __FUNCTION__);
   }

   if (tObj->Target != GL_TEXTURE_1D) {
      switch ( twrap ) {
      case GL_REPEAT:
         t->pp_txfilter |= R200_CLAMP_T_WRAP;
         break;
      case GL_CLAMP:
         t->pp_txfilter |= R200_CLAMP_T_CLAMP_GL;
         is_clamp = GL_TRUE;
         break;
      case GL_CLAMP_TO_EDGE:
         t->pp_txfilter |= R200_CLAMP_T_CLAMP_LAST;
         break;
      case GL_CLAMP_TO_BORDER:
         t->pp_txfilter |= R200_CLAMP_T_CLAMP_GL;
         is_clamp_to_border = GL_TRUE;
         break;
      case GL_MIRRORED_REPEAT:
         t->pp_txfilter |= R200_CLAMP_T_MIRROR;
         break;
      case GL_MIRROR_CLAMP_EXT:
         t->pp_txfilter |= R200_CLAMP_T_MIRROR_CLAMP_GL;
         is_clamp = GL_TRUE;
         break;
      case GL_MIRROR_CLAMP_TO_EDGE_EXT:
         t->pp_txfilter |= R200_CLAMP_T_MIRROR_CLAMP_LAST;
         break;
      case GL_MIRROR_CLAMP_TO_BORDER_EXT:
         t->pp_txfilter |= R200_CLAMP_T_MIRROR_CLAMP_GL;
         is_clamp_to_border = GL_TRUE;
         break;
      default:
         _mesa_problem(NULL, "bad T wrap mode in %s", __FUNCTION__);
      }
   }

   t->pp_txformat_x &= ~R200_CLAMP_Q_MASK;

   switch ( rwrap ) {
   case GL_REPEAT:
      t->pp_txformat_x |= R200_CLAMP_Q_WRAP;
      break;
   case GL_CLAMP:
      t->pp_txformat_x |= R200_CLAMP_Q_CLAMP_GL;
      is_clamp = GL_TRUE;
      break;
   case GL_CLAMP_TO_EDGE:
      t->pp_txformat_x |= R200_CLAMP_Q_CLAMP_LAST;
      break;
   case GL_CLAMP_TO_BORDER:
      t->pp_txformat_x |= R200_CLAMP_Q_CLAMP_GL;
      is_clamp_to_border = GL_TRUE;
      break;
   case GL_MIRRORED_REPEAT:
      t->pp_txformat_x |= R200_CLAMP_Q_MIRROR;
      break;
   case GL_MIRROR_CLAMP_EXT:
      t->pp_txformat_x |= R200_CLAMP_Q_MIRROR_CLAMP_GL;
      is_clamp = GL_TRUE;
      break;
   case GL_MIRROR_CLAMP_TO_EDGE_EXT:
      t->pp_txformat_x |= R200_CLAMP_Q_MIRROR_CLAMP_LAST;
      break;
   case GL_MIRROR_CLAMP_TO_BORDER_EXT:
      t->pp_txformat_x |= R200_CLAMP_Q_MIRROR_CLAMP_GL;
      is_clamp_to_border = GL_TRUE;
      break;
   default:
      _mesa_problem(NULL, "bad R wrap mode in %s", __FUNCTION__);
   }

   if ( is_clamp_to_border ) {
      t->pp_txfilter |= R200_BORDER_MODE_D3D;
   }

   t->border_fallback = (is_clamp && is_clamp_to_border);
}

static void r200SetTexMaxAnisotropy( radeonTexObjPtr t, GLfloat max )
{
   t->pp_txfilter &= ~R200_MAX_ANISO_MASK;
   radeon_print(RADEON_TEXTURE, RADEON_TRACE,
	"%s(tex %p) max %f.\n",
	__func__, t, max);

   if ( max <= 1.0 ) {
      t->pp_txfilter |= R200_MAX_ANISO_1_TO_1;
   } else if ( max <= 2.0 ) {
      t->pp_txfilter |= R200_MAX_ANISO_2_TO_1;
   } else if ( max <= 4.0 ) {
      t->pp_txfilter |= R200_MAX_ANISO_4_TO_1;
   } else if ( max <= 8.0 ) {
      t->pp_txfilter |= R200_MAX_ANISO_8_TO_1;
   } else {
      t->pp_txfilter |= R200_MAX_ANISO_16_TO_1;
   }
}

/**
 * Set the texture magnification and minification modes.
 * 
 * \param t Texture whose filter modes are to be set
 * \param minf Texture minification mode
 * \param magf Texture magnification mode
 */

static void r200SetTexFilter( radeonTexObjPtr t, GLenum minf, GLenum magf )
{
   GLuint anisotropy = (t->pp_txfilter & R200_MAX_ANISO_MASK);

   /* Force revalidation to account for switches from/to mipmapping. */
   t->validated = GL_FALSE;

   t->pp_txfilter &= ~(R200_MIN_FILTER_MASK | R200_MAG_FILTER_MASK);
   t->pp_txformat_x &= ~R200_VOLUME_FILTER_MASK;

   radeon_print(RADEON_TEXTURE, RADEON_TRACE,
	"%s(tex %p) minf %s, maxf %s, anisotropy %d.\n",
	__func__, t,
	_mesa_lookup_enum_by_nr(minf),
	_mesa_lookup_enum_by_nr(magf),
	anisotropy);

   if ( anisotropy == R200_MAX_ANISO_1_TO_1 ) {
      switch ( minf ) {
      case GL_NEAREST:
	 t->pp_txfilter |= R200_MIN_FILTER_NEAREST;
	 break;
      case GL_LINEAR:
	 t->pp_txfilter |= R200_MIN_FILTER_LINEAR;
	 break;
      case GL_NEAREST_MIPMAP_NEAREST:
	 t->pp_txfilter |= R200_MIN_FILTER_NEAREST_MIP_NEAREST;
	 break;
      case GL_NEAREST_MIPMAP_LINEAR:
	 t->pp_txfilter |= R200_MIN_FILTER_LINEAR_MIP_NEAREST;
	 break;
      case GL_LINEAR_MIPMAP_NEAREST:
	 t->pp_txfilter |= R200_MIN_FILTER_NEAREST_MIP_LINEAR;
	 break;
      case GL_LINEAR_MIPMAP_LINEAR:
	 t->pp_txfilter |= R200_MIN_FILTER_LINEAR_MIP_LINEAR;
	 break;
      }
   } else {
      switch ( minf ) {
      case GL_NEAREST:
	 t->pp_txfilter |= R200_MIN_FILTER_ANISO_NEAREST;
	 break;
      case GL_LINEAR:
	 t->pp_txfilter |= R200_MIN_FILTER_ANISO_LINEAR;
	 break;
      case GL_NEAREST_MIPMAP_NEAREST:
      case GL_LINEAR_MIPMAP_NEAREST:
	 t->pp_txfilter |= R200_MIN_FILTER_ANISO_NEAREST_MIP_NEAREST;
	 break;
      case GL_NEAREST_MIPMAP_LINEAR:
      case GL_LINEAR_MIPMAP_LINEAR:
	 t->pp_txfilter |= R200_MIN_FILTER_ANISO_NEAREST_MIP_LINEAR;
	 break;
      }
   }

   /* Note we don't have 3D mipmaps so only use the mag filter setting
    * to set the 3D texture filter mode.
    */
   switch ( magf ) {
   case GL_NEAREST:
      t->pp_txfilter |= R200_MAG_FILTER_NEAREST;
      t->pp_txformat_x |= R200_VOLUME_FILTER_NEAREST;
      break;
   case GL_LINEAR:
      t->pp_txfilter |= R200_MAG_FILTER_LINEAR;
      t->pp_txformat_x |= R200_VOLUME_FILTER_LINEAR;
      break;
   }
}

static void r200SetTexBorderColor( radeonTexObjPtr t, const GLfloat color[4] )
{
   GLubyte c[4];
   CLAMPED_FLOAT_TO_UBYTE(c[0], color[0]);
   CLAMPED_FLOAT_TO_UBYTE(c[1], color[1]);
   CLAMPED_FLOAT_TO_UBYTE(c[2], color[2]);
   CLAMPED_FLOAT_TO_UBYTE(c[3], color[3]);
   t->pp_border_color = radeonPackColor( 4, c[0], c[1], c[2], c[3] );
}

static void r200TexEnv( struct gl_context *ctx, GLenum target,
			  GLenum pname, const GLfloat *param )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLuint unit = ctx->Texture.CurrentUnit;
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];

   radeon_print(RADEON_TEXTURE | RADEON_STATE, RADEON_VERBOSE, "%s( %s )\n",
	       __FUNCTION__, _mesa_lookup_enum_by_nr( pname ) );

   /* This is incorrect: Need to maintain this data for each of
    * GL_TEXTURE_{123}D, GL_TEXTURE_RECTANGLE_NV, etc, and switch
    * between them according to _ReallyEnabled.
    */
   switch ( pname ) {
   case GL_TEXTURE_ENV_COLOR: {
      GLubyte c[4];
      GLuint envColor;
      _mesa_unclamped_float_rgba_to_ubyte(c, texUnit->EnvColor);
      envColor = radeonPackColor( 4, c[0], c[1], c[2], c[3] );
      if ( rmesa->hw.tf.cmd[TF_TFACTOR_0 + unit] != envColor ) {
	 R200_STATECHANGE( rmesa, tf );
	 rmesa->hw.tf.cmd[TF_TFACTOR_0 + unit] = envColor;
      }
      break;
   }

   case GL_TEXTURE_LOD_BIAS_EXT: {
      GLfloat bias, min;
      GLuint b;
      const int fixed_one = R200_LOD_BIAS_FIXED_ONE;

      /* The R200's LOD bias is a signed 2's complement value with a
       * range of -16.0 <= bias < 16.0. 
       *
       * NOTE: Add a small bias to the bias for conform mipsel.c test.
       */
      bias = *param;
      min = driQueryOptionb (&rmesa->radeon.optionCache, "no_neg_lod_bias") ?
	  0.0 : -16.0;
      bias = CLAMP( bias, min, 16.0 );
      b = ((int)(bias * fixed_one)
		+ R200_LOD_BIAS_CORRECTION) & R200_LOD_BIAS_MASK;
      
      if ( (rmesa->hw.tex[unit].cmd[TEX_PP_TXFORMAT_X] & R200_LOD_BIAS_MASK) != b ) {
	 R200_STATECHANGE( rmesa, tex[unit] );
	 rmesa->hw.tex[unit].cmd[TEX_PP_TXFORMAT_X] &= ~R200_LOD_BIAS_MASK;
	 rmesa->hw.tex[unit].cmd[TEX_PP_TXFORMAT_X] |= b;
      }
      break;
   }
   case GL_COORD_REPLACE_ARB:
      if (ctx->Point.PointSprite) {
	 R200_STATECHANGE( rmesa, spr );
	 if ((GLenum)param[0]) {
	    rmesa->hw.spr.cmd[SPR_POINT_SPRITE_CNTL] |= R200_PS_GEN_TEX_0 << unit;
	 } else {
	    rmesa->hw.spr.cmd[SPR_POINT_SPRITE_CNTL] &= ~(R200_PS_GEN_TEX_0 << unit);
	 }
      }
      break;
   default:
      return;
   }
}

void r200TexUpdateParameters(struct gl_context *ctx, GLuint unit)
{
   struct gl_sampler_object *samp = _mesa_get_samplerobj(ctx, unit);
   radeonTexObj* t = radeon_tex_obj(ctx->Texture.Unit[unit]._Current);

   r200SetTexMaxAnisotropy(t , samp->MaxAnisotropy);
   r200SetTexFilter(t, samp->MinFilter, samp->MagFilter);
   r200SetTexWrap(t, samp->WrapS, samp->WrapT, samp->WrapR);
   r200SetTexBorderColor(t, samp->BorderColor.f);
}

/**
 * Changes variables and flags for a state update, which will happen at the
 * next UpdateTextureState
 */
static void r200TexParameter( struct gl_context *ctx, GLenum target,
				struct gl_texture_object *texObj,
				GLenum pname, const GLfloat *params )
{
   radeonTexObj* t = radeon_tex_obj(texObj);

   radeon_print(RADEON_TEXTURE | RADEON_STATE, RADEON_VERBOSE,
		"%s(%p, tex %p)  target %s, pname %s\n",
		__FUNCTION__, ctx, texObj,
		_mesa_lookup_enum_by_nr( target ),
	       _mesa_lookup_enum_by_nr( pname ) );

   switch ( pname ) {
   case GL_TEXTURE_MIN_FILTER:
   case GL_TEXTURE_MAG_FILTER:
   case GL_TEXTURE_MAX_ANISOTROPY_EXT:
   case GL_TEXTURE_WRAP_S:
   case GL_TEXTURE_WRAP_T:
   case GL_TEXTURE_WRAP_R:
   case GL_TEXTURE_BORDER_COLOR:
   case GL_TEXTURE_BASE_LEVEL:
   case GL_TEXTURE_MAX_LEVEL:
   case GL_TEXTURE_MIN_LOD:
   case GL_TEXTURE_MAX_LOD:
      t->validated = GL_FALSE;
      break;

   default:
      return;
   }
}


static void r200DeleteTexture(struct gl_context * ctx, struct gl_texture_object *texObj)
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   radeonTexObj* t = radeon_tex_obj(texObj);

   radeon_print(RADEON_TEXTURE | RADEON_STATE, RADEON_NORMAL,
           "%s( %p (target = %s) )\n", __FUNCTION__,
	   (void *)texObj,
	   _mesa_lookup_enum_by_nr(texObj->Target));

   if (rmesa) {
      int i;
      radeon_firevertices(&rmesa->radeon);
      for ( i = 0 ; i < rmesa->radeon.glCtx->Const.MaxTextureUnits ; i++ ) {
	 if ( t == rmesa->state.texture.unit[i].texobj ) {
	    rmesa->state.texture.unit[i].texobj = NULL;
	    rmesa->hw.tex[i].dirty = GL_FALSE;
	    rmesa->hw.cube[i].dirty = GL_FALSE;
	 }
      }      
   }

   radeon_miptree_unreference(&t->mt);

   _mesa_delete_texture_object(ctx, texObj);
}

/* Need:  
 *  - Same GEN_MODE for all active bits
 *  - Same EyePlane/ObjPlane for all active bits when using Eye/Obj
 *  - STRQ presumably all supported (matrix means incoming R values
 *    can end up in STQ, this has implications for vertex support,
 *    presumably ok if maos is used, though?)
 *  
 * Basically impossible to do this on the fly - just collect some
 * basic info & do the checks from ValidateState().
 */
static void r200TexGen( struct gl_context *ctx,
			  GLenum coord,
			  GLenum pname,
			  const GLfloat *params )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLuint unit = ctx->Texture.CurrentUnit;
   rmesa->recheck_texgen[unit] = GL_TRUE;
}


/**
 * Allocate a new texture object.
 * Called via ctx->Driver.NewTextureObject.
 * Note: this function will be called during context creation to
 * allocate the default texture objects.
 * Fixup MaxAnisotropy according to user preference.
 */
static struct gl_texture_object *r200NewTextureObject(struct gl_context * ctx,
						      GLuint name,
						      GLenum target)
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   radeonTexObj* t = CALLOC_STRUCT(radeon_tex_obj);


   radeon_print(RADEON_STATE | RADEON_TEXTURE, RADEON_NORMAL,
           "%s(%p) target %s, new texture %p.\n",
	   __FUNCTION__, ctx,
	   _mesa_lookup_enum_by_nr(target), t);

   _mesa_initialize_texture_object(&t->base, name, target);
   t->base.Sampler.MaxAnisotropy = rmesa->radeon.initialMaxAnisotropy;

   /* Initialize hardware state */
   r200SetTexWrap( t, t->base.Sampler.WrapS, t->base.Sampler.WrapT, t->base.Sampler.WrapR );
   r200SetTexMaxAnisotropy( t, t->base.Sampler.MaxAnisotropy );
   r200SetTexFilter(t, t->base.Sampler.MinFilter, t->base.Sampler.MagFilter);
   r200SetTexBorderColor(t, t->base.Sampler.BorderColor.f);

   return &t->base;
}

static struct gl_sampler_object *
r200NewSamplerObject(struct gl_context *ctx, GLuint name)
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   struct gl_sampler_object *samp = _mesa_new_sampler_object(ctx, name);
   if (samp)
      samp->MaxAnisotropy = rmesa->radeon.initialMaxAnisotropy;
   return samp;
}



void r200InitTextureFuncs( radeonContextPtr radeon, struct dd_function_table *functions )
{
   /* Note: we only plug in the functions we implement in the driver
    * since _mesa_init_driver_functions() was already called.
    */

   radeon_init_common_texture_funcs(radeon, functions);

   functions->NewTextureObject		= r200NewTextureObject;
   //   functions->BindTexture		= r200BindTexture;
   functions->DeleteTexture		= r200DeleteTexture;

   functions->TexEnv			= r200TexEnv;
   functions->TexParameter		= r200TexParameter;
   functions->TexGen			= r200TexGen;
   functions->NewSamplerObject		= r200NewSamplerObject;
}
