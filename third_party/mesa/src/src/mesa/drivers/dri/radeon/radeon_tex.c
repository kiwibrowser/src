/*
Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
                     VA Linux Systems Inc., Fremont, California.

All Rights Reserved.

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
 *    Gareth Hughes <gareth@valinux.com>
 *    Brian Paul <brianp@valinux.com>
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

#include "radeon_context.h"
#include "radeon_mipmap_tree.h"
#include "radeon_ioctl.h"
#include "radeon_tex.h"

#include "xmlpool.h"



/**
 * Set the texture wrap modes.
 * 
 * \param t Texture object whose wrap modes are to be set
 * \param swrap Wrap mode for the \a s texture coordinate
 * \param twrap Wrap mode for the \a t texture coordinate
 */

static void radeonSetTexWrap( radeonTexObjPtr t, GLenum swrap, GLenum twrap )
{
   GLboolean  is_clamp = GL_FALSE;
   GLboolean  is_clamp_to_border = GL_FALSE;

   t->pp_txfilter &= ~(RADEON_CLAMP_S_MASK | RADEON_CLAMP_T_MASK | RADEON_BORDER_MODE_D3D);

   switch ( swrap ) {
   case GL_REPEAT:
      t->pp_txfilter |= RADEON_CLAMP_S_WRAP;
      break;
   case GL_CLAMP:
      t->pp_txfilter |= RADEON_CLAMP_S_CLAMP_GL;
      is_clamp = GL_TRUE;
      break;
   case GL_CLAMP_TO_EDGE:
      t->pp_txfilter |= RADEON_CLAMP_S_CLAMP_LAST;
      break;
   case GL_CLAMP_TO_BORDER:
      t->pp_txfilter |= RADEON_CLAMP_S_CLAMP_GL;
      is_clamp_to_border = GL_TRUE;
      break;
   case GL_MIRRORED_REPEAT:
      t->pp_txfilter |= RADEON_CLAMP_S_MIRROR;
      break;
   case GL_MIRROR_CLAMP_EXT:
      t->pp_txfilter |= RADEON_CLAMP_S_MIRROR_CLAMP_GL;
      is_clamp = GL_TRUE;
      break;
   case GL_MIRROR_CLAMP_TO_EDGE_EXT:
      t->pp_txfilter |= RADEON_CLAMP_S_MIRROR_CLAMP_LAST;
      break;
   case GL_MIRROR_CLAMP_TO_BORDER_EXT:
      t->pp_txfilter |= RADEON_CLAMP_S_MIRROR_CLAMP_GL;
      is_clamp_to_border = GL_TRUE;
      break;
   default:
      _mesa_problem(NULL, "bad S wrap mode in %s", __FUNCTION__);
   }

   if (t->base.Target != GL_TEXTURE_1D) {
      switch ( twrap ) {
      case GL_REPEAT:
	 t->pp_txfilter |= RADEON_CLAMP_T_WRAP;
	 break;
      case GL_CLAMP:
	 t->pp_txfilter |= RADEON_CLAMP_T_CLAMP_GL;
	 is_clamp = GL_TRUE;
	 break;
      case GL_CLAMP_TO_EDGE:
	 t->pp_txfilter |= RADEON_CLAMP_T_CLAMP_LAST;
	 break;
      case GL_CLAMP_TO_BORDER:
	 t->pp_txfilter |= RADEON_CLAMP_T_CLAMP_GL;
	 is_clamp_to_border = GL_TRUE;
	 break;
      case GL_MIRRORED_REPEAT:
	 t->pp_txfilter |= RADEON_CLAMP_T_MIRROR;
	 break;
      case GL_MIRROR_CLAMP_EXT:
	 t->pp_txfilter |= RADEON_CLAMP_T_MIRROR_CLAMP_GL;
	 is_clamp = GL_TRUE;
	 break;
      case GL_MIRROR_CLAMP_TO_EDGE_EXT:
	 t->pp_txfilter |= RADEON_CLAMP_T_MIRROR_CLAMP_LAST;
	 break;
      case GL_MIRROR_CLAMP_TO_BORDER_EXT:
	 t->pp_txfilter |= RADEON_CLAMP_T_MIRROR_CLAMP_GL;
	 is_clamp_to_border = GL_TRUE;
	 break;
      default:
	 _mesa_problem(NULL, "bad T wrap mode in %s", __FUNCTION__);
      }
   }

   if ( is_clamp_to_border ) {
      t->pp_txfilter |= RADEON_BORDER_MODE_D3D;
   }

   t->border_fallback = (is_clamp && is_clamp_to_border);
}

static void radeonSetTexMaxAnisotropy( radeonTexObjPtr t, GLfloat max )
{
   t->pp_txfilter &= ~RADEON_MAX_ANISO_MASK;

   if ( max == 1.0 ) {
      t->pp_txfilter |= RADEON_MAX_ANISO_1_TO_1;
   } else if ( max <= 2.0 ) {
      t->pp_txfilter |= RADEON_MAX_ANISO_2_TO_1;
   } else if ( max <= 4.0 ) {
      t->pp_txfilter |= RADEON_MAX_ANISO_4_TO_1;
   } else if ( max <= 8.0 ) {
      t->pp_txfilter |= RADEON_MAX_ANISO_8_TO_1;
   } else {
      t->pp_txfilter |= RADEON_MAX_ANISO_16_TO_1;
   }
}

/**
 * Set the texture magnification and minification modes.
 * 
 * \param t Texture whose filter modes are to be set
 * \param minf Texture minification mode
 * \param magf Texture magnification mode
 */

static void radeonSetTexFilter( radeonTexObjPtr t, GLenum minf, GLenum magf )
{
   GLuint anisotropy = (t->pp_txfilter & RADEON_MAX_ANISO_MASK);

   /* Force revalidation to account for switches from/to mipmapping. */
   t->validated = GL_FALSE;

   t->pp_txfilter &= ~(RADEON_MIN_FILTER_MASK | RADEON_MAG_FILTER_MASK);

   /* r100 chips can't handle mipmaps/aniso for cubemap/volume textures */
   if ( t->base.Target == GL_TEXTURE_CUBE_MAP ) {
      switch ( minf ) {
      case GL_NEAREST:
      case GL_NEAREST_MIPMAP_NEAREST:
      case GL_NEAREST_MIPMAP_LINEAR:
	 t->pp_txfilter |= RADEON_MIN_FILTER_NEAREST;
	 break;
      case GL_LINEAR:
      case GL_LINEAR_MIPMAP_NEAREST:
      case GL_LINEAR_MIPMAP_LINEAR:
	 t->pp_txfilter |= RADEON_MIN_FILTER_LINEAR;
	 break;
      default:
	 break;
      }
   }
   else if ( anisotropy == RADEON_MAX_ANISO_1_TO_1 ) {
      switch ( minf ) {
      case GL_NEAREST:
	 t->pp_txfilter |= RADEON_MIN_FILTER_NEAREST;
	 break;
      case GL_LINEAR:
	 t->pp_txfilter |= RADEON_MIN_FILTER_LINEAR;
	 break;
      case GL_NEAREST_MIPMAP_NEAREST:
	 t->pp_txfilter |= RADEON_MIN_FILTER_NEAREST_MIP_NEAREST;
	 break;
      case GL_NEAREST_MIPMAP_LINEAR:
	 t->pp_txfilter |= RADEON_MIN_FILTER_LINEAR_MIP_NEAREST;
	 break;
      case GL_LINEAR_MIPMAP_NEAREST:
	 t->pp_txfilter |= RADEON_MIN_FILTER_NEAREST_MIP_LINEAR;
	 break;
      case GL_LINEAR_MIPMAP_LINEAR:
	 t->pp_txfilter |= RADEON_MIN_FILTER_LINEAR_MIP_LINEAR;
	 break;
      }
   } else {
      switch ( minf ) {
      case GL_NEAREST:
	 t->pp_txfilter |= RADEON_MIN_FILTER_ANISO_NEAREST;
	 break;
      case GL_LINEAR:
	 t->pp_txfilter |= RADEON_MIN_FILTER_ANISO_LINEAR;
	 break;
      case GL_NEAREST_MIPMAP_NEAREST:
      case GL_LINEAR_MIPMAP_NEAREST:
	 t->pp_txfilter |= RADEON_MIN_FILTER_ANISO_NEAREST_MIP_NEAREST;
	 break;
      case GL_NEAREST_MIPMAP_LINEAR:
      case GL_LINEAR_MIPMAP_LINEAR:
	 t->pp_txfilter |= RADEON_MIN_FILTER_ANISO_NEAREST_MIP_LINEAR;
	 break;
      }
   }

   switch ( magf ) {
   case GL_NEAREST:
      t->pp_txfilter |= RADEON_MAG_FILTER_NEAREST;
      break;
   case GL_LINEAR:
      t->pp_txfilter |= RADEON_MAG_FILTER_LINEAR;
      break;
   }
}

static void radeonSetTexBorderColor( radeonTexObjPtr t, const GLfloat color[4] )
{
   GLubyte c[4];
   CLAMPED_FLOAT_TO_UBYTE(c[0], color[0]);
   CLAMPED_FLOAT_TO_UBYTE(c[1], color[1]);
   CLAMPED_FLOAT_TO_UBYTE(c[2], color[2]);
   CLAMPED_FLOAT_TO_UBYTE(c[3], color[3]);
   t->pp_border_color = radeonPackColor( 4, c[0], c[1], c[2], c[3] );
}

#define SCALED_FLOAT_TO_BYTE( x, scale ) \
		(((GLuint)((255.0F / scale) * (x))) / 2)

static void radeonTexEnv( struct gl_context *ctx, GLenum target,
			  GLenum pname, const GLfloat *param )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   GLuint unit = ctx->Texture.CurrentUnit;
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];

   if ( RADEON_DEBUG & RADEON_STATE ) {
      fprintf( stderr, "%s( %s )\n",
	       __FUNCTION__, _mesa_lookup_enum_by_nr( pname ) );
   }

   switch ( pname ) {
   case GL_TEXTURE_ENV_COLOR: {
      GLubyte c[4];
      GLuint envColor;
      _mesa_unclamped_float_rgba_to_ubyte(c, texUnit->EnvColor);
      envColor = radeonPackColor( 4, c[0], c[1], c[2], c[3] );
      if ( rmesa->hw.tex[unit].cmd[TEX_PP_TFACTOR] != envColor ) {
	 RADEON_STATECHANGE( rmesa, tex[unit] );
	 rmesa->hw.tex[unit].cmd[TEX_PP_TFACTOR] = envColor;
      }
      break;
   }

   case GL_TEXTURE_LOD_BIAS_EXT: {
      GLfloat bias, min;
      GLuint b;

      /* The Radeon's LOD bias is a signed 2's complement value with a
       * range of -1.0 <= bias < 4.0.  We break this into two linear
       * functions, one mapping [-1.0,0.0] to [-128,0] and one mapping
       * [0.0,4.0] to [0,127].
       */
      min = driQueryOptionb (&rmesa->radeon.optionCache, "no_neg_lod_bias") ?
	  0.0 : -1.0;
      bias = CLAMP( *param, min, 4.0 );
      if ( bias == 0 ) {
	 b = 0;
      } else if ( bias > 0 ) {
	 b = ((GLuint)SCALED_FLOAT_TO_BYTE( bias, 4.0 )) << RADEON_LOD_BIAS_SHIFT;
      } else {
	 b = ((GLuint)SCALED_FLOAT_TO_BYTE( bias, 1.0 )) << RADEON_LOD_BIAS_SHIFT;
      }
      if ( (rmesa->hw.tex[unit].cmd[TEX_PP_TXFILTER] & RADEON_LOD_BIAS_MASK) != b ) {
	 RADEON_STATECHANGE( rmesa, tex[unit] );
	 rmesa->hw.tex[unit].cmd[TEX_PP_TXFILTER] &= ~RADEON_LOD_BIAS_MASK;
	 rmesa->hw.tex[unit].cmd[TEX_PP_TXFILTER] |= (b & RADEON_LOD_BIAS_MASK);
      }
      break;
   }

   default:
      return;
   }
}

void radeonTexUpdateParameters(struct gl_context *ctx, GLuint unit)
{
   struct gl_sampler_object *samp = _mesa_get_samplerobj(ctx, unit);
   radeonTexObj* t = radeon_tex_obj(ctx->Texture.Unit[unit]._Current);

   radeonSetTexMaxAnisotropy(t , samp->MaxAnisotropy);
   radeonSetTexFilter(t, samp->MinFilter, samp->MagFilter);
   radeonSetTexWrap(t, samp->WrapS, samp->WrapT);
   radeonSetTexBorderColor(t, samp->BorderColor.f);
}


/**
 * Changes variables and flags for a state update, which will happen at the
 * next UpdateTextureState
 */

static void radeonTexParameter( struct gl_context *ctx, GLenum target,
				struct gl_texture_object *texObj,
				GLenum pname, const GLfloat *params )
{
   radeonTexObj* t = radeon_tex_obj(texObj);

   radeon_print(RADEON_TEXTURE, RADEON_VERBOSE, "%s( %s )\n", __FUNCTION__,
	       _mesa_lookup_enum_by_nr( pname ) );

   switch ( pname ) {
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

static void radeonDeleteTexture( struct gl_context *ctx,
				 struct gl_texture_object *texObj )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   radeonTexObj* t = radeon_tex_obj(texObj);
   int i;

   radeon_print(RADEON_TEXTURE, RADEON_NORMAL,
	 "%s( %p (target = %s) )\n", __FUNCTION__, (void *)texObj,
	       _mesa_lookup_enum_by_nr( texObj->Target ) );

   if ( rmesa ) {
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

   /* Free mipmap images and the texture object itself */
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
static void radeonTexGen( struct gl_context *ctx,
			  GLenum coord,
			  GLenum pname,
			  const GLfloat *params )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   GLuint unit = ctx->Texture.CurrentUnit;
   rmesa->recheck_texgen[unit] = GL_TRUE;
}

/**
 * Allocate a new texture object.
 * Called via ctx->Driver.NewTextureObject.
 * Note: we could use containment here to 'derive' the driver-specific
 * texture object from the core mesa gl_texture_object.  Not done at this time.
 */
static struct gl_texture_object *
radeonNewTextureObject( struct gl_context *ctx, GLuint name, GLenum target )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   radeonTexObj* t = CALLOC_STRUCT(radeon_tex_obj);

   _mesa_initialize_texture_object(&t->base, name, target);
   t->base.Sampler.MaxAnisotropy = rmesa->radeon.initialMaxAnisotropy;

   t->border_fallback = GL_FALSE;

   t->pp_txfilter = RADEON_BORDER_MODE_OGL;
   t->pp_txformat = (RADEON_TXFORMAT_ENDIAN_NO_SWAP |
		     RADEON_TXFORMAT_PERSPECTIVE_ENABLE);
   
   radeonSetTexWrap( t, t->base.Sampler.WrapS, t->base.Sampler.WrapT );
   radeonSetTexMaxAnisotropy( t, t->base.Sampler.MaxAnisotropy );
   radeonSetTexFilter( t, t->base.Sampler.MinFilter, t->base.Sampler.MagFilter );
   radeonSetTexBorderColor( t, t->base.Sampler.BorderColor.f );
   return &t->base;
}


static struct gl_sampler_object *
radeonNewSamplerObject(struct gl_context *ctx, GLuint name)
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   struct gl_sampler_object *samp = _mesa_new_sampler_object(ctx, name);
   if (samp)
      samp->MaxAnisotropy = rmesa->radeon.initialMaxAnisotropy;
   return samp;
}


void radeonInitTextureFuncs( radeonContextPtr radeon, struct dd_function_table *functions )
{
   radeon_init_common_texture_funcs(radeon, functions);

   functions->NewTextureObject		= radeonNewTextureObject;
   //   functions->BindTexture		= radeonBindTexture;
   functions->DeleteTexture		= radeonDeleteTexture;

   functions->TexEnv			= radeonTexEnv;
   functions->TexParameter		= radeonTexParameter;
   functions->TexGen			= radeonTexGen;
   functions->NewSamplerObject		= radeonNewSamplerObject;
}
