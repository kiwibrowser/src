/**************************************************************************

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

**************************************************************************/

/*
 * Authors:
 *   Kevin E. Martin <martin@valinux.com>
 *   Gareth Hughes <gareth@valinux.com>
 */

#include "main/glheader.h"
#include "main/imports.h"
#include "main/colormac.h"
#include "main/context.h"
#include "main/macros.h"
#include "main/teximage.h"
#include "main/texstate.h"
#include "main/texobj.h"
#include "main/enums.h"
#include "main/samplerobj.h"

#include "radeon_context.h"
#include "radeon_mipmap_tree.h"
#include "radeon_state.h"
#include "radeon_ioctl.h"
#include "radeon_swtcl.h"
#include "radeon_tex.h"
#include "radeon_tcl.h"


#define RADEON_TXFORMAT_A8        RADEON_TXFORMAT_I8
#define RADEON_TXFORMAT_L8        RADEON_TXFORMAT_I8
#define RADEON_TXFORMAT_AL88      RADEON_TXFORMAT_AI88
#define RADEON_TXFORMAT_YCBCR     RADEON_TXFORMAT_YVYU422
#define RADEON_TXFORMAT_YCBCR_REV RADEON_TXFORMAT_VYUY422
#define RADEON_TXFORMAT_RGB_DXT1  RADEON_TXFORMAT_DXT1
#define RADEON_TXFORMAT_RGBA_DXT1 RADEON_TXFORMAT_DXT1
#define RADEON_TXFORMAT_RGBA_DXT3 RADEON_TXFORMAT_DXT23
#define RADEON_TXFORMAT_RGBA_DXT5 RADEON_TXFORMAT_DXT45

#define _COLOR(f) \
    [ MESA_FORMAT_ ## f ] = { RADEON_TXFORMAT_ ## f, 0 }
#define _COLOR_REV(f) \
    [ MESA_FORMAT_ ## f ## _REV ] = { RADEON_TXFORMAT_ ## f, 0 }
#define _ALPHA(f) \
    [ MESA_FORMAT_ ## f ] = { RADEON_TXFORMAT_ ## f | RADEON_TXFORMAT_ALPHA_IN_MAP, 0 }
#define _ALPHA_REV(f) \
    [ MESA_FORMAT_ ## f ## _REV ] = { RADEON_TXFORMAT_ ## f | RADEON_TXFORMAT_ALPHA_IN_MAP, 0 }
#define _YUV(f) \
   [ MESA_FORMAT_ ## f ] = { RADEON_TXFORMAT_ ## f, RADEON_YUV_TO_RGB }
#define _INVALID(f) \
    [ MESA_FORMAT_ ## f ] = { 0xffffffff, 0 }
#define VALID_FORMAT(f) ( ((f) <= MESA_FORMAT_RGBA_DXT5) \
			     && (tx_table[f].format != 0xffffffff) )

struct tx_table {
   GLuint format, filter;
};

/* XXX verify this table against MESA_FORMAT_x values */
static const struct tx_table tx_table[] =
{
   _INVALID(NONE), /* MESA_FORMAT_NONE */
   _ALPHA(RGBA8888),
   _ALPHA_REV(RGBA8888),
   _ALPHA(ARGB8888),
   _ALPHA_REV(ARGB8888),
   [ MESA_FORMAT_RGB888 ] = { RADEON_TXFORMAT_ARGB8888, 0 },
   _COLOR(RGB565),
   _COLOR_REV(RGB565),
   _ALPHA(ARGB4444),
   _ALPHA_REV(ARGB4444),
   _ALPHA(ARGB1555),
   _ALPHA_REV(ARGB1555),
   _ALPHA(AL88),
   _ALPHA_REV(AL88),
   _ALPHA(A8),
   _COLOR(L8),
   _ALPHA(I8),
   _YUV(YCBCR),
   _YUV(YCBCR_REV),
   _INVALID(RGB_FXT1),
   _INVALID(RGBA_FXT1),
   _COLOR(RGB_DXT1),
   _ALPHA(RGBA_DXT1),
   _ALPHA(RGBA_DXT3),
   _ALPHA(RGBA_DXT5),
};

#undef _COLOR
#undef _ALPHA
#undef _INVALID

/* ================================================================
 * Texture combine functions
 */

/* GL_ARB_texture_env_combine support
 */

/* The color tables have combine functions for GL_SRC_COLOR,
 * GL_ONE_MINUS_SRC_COLOR, GL_SRC_ALPHA and GL_ONE_MINUS_SRC_ALPHA.
 */
static GLuint radeon_texture_color[][RADEON_MAX_TEXTURE_UNITS] =
{
   {
      RADEON_COLOR_ARG_A_T0_COLOR,
      RADEON_COLOR_ARG_A_T1_COLOR,
      RADEON_COLOR_ARG_A_T2_COLOR
   },
   {
      RADEON_COLOR_ARG_A_T0_COLOR | RADEON_COMP_ARG_A,
      RADEON_COLOR_ARG_A_T1_COLOR | RADEON_COMP_ARG_A,
      RADEON_COLOR_ARG_A_T2_COLOR | RADEON_COMP_ARG_A
   },
   {
      RADEON_COLOR_ARG_A_T0_ALPHA,
      RADEON_COLOR_ARG_A_T1_ALPHA,
      RADEON_COLOR_ARG_A_T2_ALPHA
   },
   {
      RADEON_COLOR_ARG_A_T0_ALPHA | RADEON_COMP_ARG_A,
      RADEON_COLOR_ARG_A_T1_ALPHA | RADEON_COMP_ARG_A,
      RADEON_COLOR_ARG_A_T2_ALPHA | RADEON_COMP_ARG_A
   },
};

static GLuint radeon_tfactor_color[] =
{
   RADEON_COLOR_ARG_A_TFACTOR_COLOR,
   RADEON_COLOR_ARG_A_TFACTOR_COLOR | RADEON_COMP_ARG_A,
   RADEON_COLOR_ARG_A_TFACTOR_ALPHA,
   RADEON_COLOR_ARG_A_TFACTOR_ALPHA | RADEON_COMP_ARG_A
};

static GLuint radeon_primary_color[] =
{
   RADEON_COLOR_ARG_A_DIFFUSE_COLOR,
   RADEON_COLOR_ARG_A_DIFFUSE_COLOR | RADEON_COMP_ARG_A,
   RADEON_COLOR_ARG_A_DIFFUSE_ALPHA,
   RADEON_COLOR_ARG_A_DIFFUSE_ALPHA | RADEON_COMP_ARG_A
};

static GLuint radeon_previous_color[] =
{
   RADEON_COLOR_ARG_A_CURRENT_COLOR,
   RADEON_COLOR_ARG_A_CURRENT_COLOR | RADEON_COMP_ARG_A,
   RADEON_COLOR_ARG_A_CURRENT_ALPHA,
   RADEON_COLOR_ARG_A_CURRENT_ALPHA | RADEON_COMP_ARG_A
};

/* GL_ZERO table - indices 0-3
 * GL_ONE  table - indices 1-4
 */
static GLuint radeon_zero_color[] =
{
   RADEON_COLOR_ARG_A_ZERO,
   RADEON_COLOR_ARG_A_ZERO | RADEON_COMP_ARG_A,
   RADEON_COLOR_ARG_A_ZERO,
   RADEON_COLOR_ARG_A_ZERO | RADEON_COMP_ARG_A,
   RADEON_COLOR_ARG_A_ZERO
};


/* The alpha tables only have GL_SRC_ALPHA and GL_ONE_MINUS_SRC_ALPHA.
 */
static GLuint radeon_texture_alpha[][RADEON_MAX_TEXTURE_UNITS] =
{
   {
      RADEON_ALPHA_ARG_A_T0_ALPHA,
      RADEON_ALPHA_ARG_A_T1_ALPHA,
      RADEON_ALPHA_ARG_A_T2_ALPHA
   },
   {
      RADEON_ALPHA_ARG_A_T0_ALPHA | RADEON_COMP_ARG_A,
      RADEON_ALPHA_ARG_A_T1_ALPHA | RADEON_COMP_ARG_A,
      RADEON_ALPHA_ARG_A_T2_ALPHA | RADEON_COMP_ARG_A
   },
};

static GLuint radeon_tfactor_alpha[] =
{
   RADEON_ALPHA_ARG_A_TFACTOR_ALPHA,
   RADEON_ALPHA_ARG_A_TFACTOR_ALPHA | RADEON_COMP_ARG_A
};

static GLuint radeon_primary_alpha[] =
{
   RADEON_ALPHA_ARG_A_DIFFUSE_ALPHA,
   RADEON_ALPHA_ARG_A_DIFFUSE_ALPHA | RADEON_COMP_ARG_A
};

static GLuint radeon_previous_alpha[] =
{
   RADEON_ALPHA_ARG_A_CURRENT_ALPHA,
   RADEON_ALPHA_ARG_A_CURRENT_ALPHA | RADEON_COMP_ARG_A
};

/* GL_ZERO table - indices 0-1
 * GL_ONE  table - indices 1-2
 */
static GLuint radeon_zero_alpha[] =
{
   RADEON_ALPHA_ARG_A_ZERO,
   RADEON_ALPHA_ARG_A_ZERO | RADEON_COMP_ARG_A,
   RADEON_ALPHA_ARG_A_ZERO
};


/* Extract the arg from slot A, shift it into the correct argument slot
 * and set the corresponding complement bit.
 */
#define RADEON_COLOR_ARG( n, arg )			\
do {							\
   color_combine |=					\
      ((color_arg[n] & RADEON_COLOR_ARG_MASK)		\
       << RADEON_COLOR_ARG_##arg##_SHIFT);		\
   color_combine |=					\
      ((color_arg[n] >> RADEON_COMP_ARG_SHIFT)		\
       << RADEON_COMP_ARG_##arg##_SHIFT);		\
} while (0)

#define RADEON_ALPHA_ARG( n, arg )			\
do {							\
   alpha_combine |=					\
      ((alpha_arg[n] & RADEON_ALPHA_ARG_MASK)		\
       << RADEON_ALPHA_ARG_##arg##_SHIFT);		\
   alpha_combine |=					\
      ((alpha_arg[n] >> RADEON_COMP_ARG_SHIFT)		\
       << RADEON_COMP_ARG_##arg##_SHIFT);		\
} while (0)


/* ================================================================
 * Texture unit state management
 */

static GLboolean radeonUpdateTextureEnv( struct gl_context *ctx, int unit )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
   GLuint color_combine, alpha_combine;
   const GLuint color_combine0 = RADEON_COLOR_ARG_A_ZERO | RADEON_COLOR_ARG_B_ZERO
         | RADEON_COLOR_ARG_C_CURRENT_COLOR | RADEON_BLEND_CTL_ADD
         | RADEON_SCALE_1X | RADEON_CLAMP_TX;
   const GLuint alpha_combine0 = RADEON_ALPHA_ARG_A_ZERO | RADEON_ALPHA_ARG_B_ZERO
         | RADEON_ALPHA_ARG_C_CURRENT_ALPHA | RADEON_BLEND_CTL_ADD
         | RADEON_SCALE_1X | RADEON_CLAMP_TX;


   /* texUnit->_Current can be NULL if and only if the texture unit is
    * not actually enabled.
    */
   assert( (texUnit->_ReallyEnabled == 0)
	   || (texUnit->_Current != NULL) );

   if ( RADEON_DEBUG & RADEON_TEXTURE ) {
      fprintf( stderr, "%s( %p, %d )\n", __FUNCTION__, (void *)ctx, unit );
   }

   /* Set the texture environment state.  Isn't this nice and clean?
    * The chip will automagically set the texture alpha to 0xff when
    * the texture format does not include an alpha component. This
    * reduces the amount of special-casing we have to do, alpha-only
    * textures being a notable exception. Doesn't work for luminance
    * textures realized with I8 and ALPHA_IN_MAP not set neither (on r100).
    */
    /* Don't cache these results.
    */
   rmesa->state.texture.unit[unit].format = 0;
   rmesa->state.texture.unit[unit].envMode = 0;

   if ( !texUnit->_ReallyEnabled ) {
      color_combine = color_combine0;
      alpha_combine = alpha_combine0;
   }
   else {
      GLuint color_arg[3], alpha_arg[3];
      GLuint i;
      const GLuint numColorArgs = texUnit->_CurrentCombine->_NumArgsRGB;
      const GLuint numAlphaArgs = texUnit->_CurrentCombine->_NumArgsA;
      GLuint RGBshift = texUnit->_CurrentCombine->ScaleShiftRGB;
      GLuint Ashift = texUnit->_CurrentCombine->ScaleShiftA;


      /* Step 1:
       * Extract the color and alpha combine function arguments.
       */
      for ( i = 0 ; i < numColorArgs ; i++ ) {
	 const GLint op = texUnit->_CurrentCombine->OperandRGB[i] - GL_SRC_COLOR;
	 const GLuint srcRGBi = texUnit->_CurrentCombine->SourceRGB[i];
	 assert(op >= 0);
	 assert(op <= 3);
	 switch ( srcRGBi ) {
	 case GL_TEXTURE:
	    if (texUnit->_Current->Image[0][0]->_BaseFormat == GL_ALPHA)
	       color_arg[i] = radeon_zero_color[op];
	    else
	       color_arg[i] = radeon_texture_color[op][unit];
	    break;
	 case GL_CONSTANT:
	    color_arg[i] = radeon_tfactor_color[op];
	    break;
	 case GL_PRIMARY_COLOR:
	    color_arg[i] = radeon_primary_color[op];
	    break;
	 case GL_PREVIOUS:
	    color_arg[i] = radeon_previous_color[op];
	    break;
	 case GL_ZERO:
	    color_arg[i] = radeon_zero_color[op];
	    break;
	 case GL_ONE:
	    color_arg[i] = radeon_zero_color[op+1];
	    break;
	 case GL_TEXTURE0:
	 case GL_TEXTURE1:
	 case GL_TEXTURE2: {
	    GLuint txunit = srcRGBi - GL_TEXTURE0;
	    if (ctx->Texture.Unit[txunit]._Current->Image[0][0]->_BaseFormat == GL_ALPHA)
	       color_arg[i] = radeon_zero_color[op];
	    else
	 /* implement ogl 1.4/1.5 core spec here, not specification of
	  * GL_ARB_texture_env_crossbar (which would require disabling blending
	  * instead of undefined results when referencing not enabled texunit) */
	      color_arg[i] = radeon_texture_color[op][txunit];
	    }
	    break;
	 default:
	    return GL_FALSE;
	 }
      }

      for ( i = 0 ; i < numAlphaArgs ; i++ ) {
	 const GLint op = texUnit->_CurrentCombine->OperandA[i] - GL_SRC_ALPHA;
	 const GLuint srcAi = texUnit->_CurrentCombine->SourceA[i];
	 assert(op >= 0);
	 assert(op <= 1);
	 switch ( srcAi ) {
	 case GL_TEXTURE:
	    if (texUnit->_Current->Image[0][0]->_BaseFormat == GL_LUMINANCE)
	       alpha_arg[i] = radeon_zero_alpha[op+1];
	    else
	       alpha_arg[i] = radeon_texture_alpha[op][unit];
	    break;
	 case GL_CONSTANT:
	    alpha_arg[i] = radeon_tfactor_alpha[op];
	    break;
	 case GL_PRIMARY_COLOR:
	    alpha_arg[i] = radeon_primary_alpha[op];
	    break;
	 case GL_PREVIOUS:
	    alpha_arg[i] = radeon_previous_alpha[op];
	    break;
	 case GL_ZERO:
	    alpha_arg[i] = radeon_zero_alpha[op];
	    break;
	 case GL_ONE:
	    alpha_arg[i] = radeon_zero_alpha[op+1];
	    break;
	 case GL_TEXTURE0:
	 case GL_TEXTURE1:
	 case GL_TEXTURE2: {    
	    GLuint txunit = srcAi - GL_TEXTURE0;
	    if (ctx->Texture.Unit[txunit]._Current->Image[0][0]->_BaseFormat == GL_LUMINANCE)
	       alpha_arg[i] = radeon_zero_alpha[op+1];
	    else
	       alpha_arg[i] = radeon_texture_alpha[op][txunit];
	    }
	    break;
	 default:
	    return GL_FALSE;
	 }
      }

      /* Step 2:
       * Build up the color and alpha combine functions.
       */
      switch ( texUnit->_CurrentCombine->ModeRGB ) {
      case GL_REPLACE:
	 color_combine = (RADEON_COLOR_ARG_A_ZERO |
			  RADEON_COLOR_ARG_B_ZERO |
			  RADEON_BLEND_CTL_ADD |
			  RADEON_CLAMP_TX);
	 RADEON_COLOR_ARG( 0, C );
	 break;
      case GL_MODULATE:
	 color_combine = (RADEON_COLOR_ARG_C_ZERO |
			  RADEON_BLEND_CTL_ADD |
			  RADEON_CLAMP_TX);
	 RADEON_COLOR_ARG( 0, A );
	 RADEON_COLOR_ARG( 1, B );
	 break;
      case GL_ADD:
	 color_combine = (RADEON_COLOR_ARG_B_ZERO |
			  RADEON_COMP_ARG_B |
			  RADEON_BLEND_CTL_ADD |
			  RADEON_CLAMP_TX);
	 RADEON_COLOR_ARG( 0, A );
	 RADEON_COLOR_ARG( 1, C );
	 break;
      case GL_ADD_SIGNED:
	 color_combine = (RADEON_COLOR_ARG_B_ZERO |
			  RADEON_COMP_ARG_B |
			  RADEON_BLEND_CTL_ADDSIGNED |
			  RADEON_CLAMP_TX);
	 RADEON_COLOR_ARG( 0, A );
	 RADEON_COLOR_ARG( 1, C );
	 break;
      case GL_SUBTRACT:
	 color_combine = (RADEON_COLOR_ARG_B_ZERO |
			  RADEON_COMP_ARG_B |
			  RADEON_BLEND_CTL_SUBTRACT |
			  RADEON_CLAMP_TX);
	 RADEON_COLOR_ARG( 0, A );
	 RADEON_COLOR_ARG( 1, C );
	 break;
      case GL_INTERPOLATE:
	 color_combine = (RADEON_BLEND_CTL_BLEND |
			  RADEON_CLAMP_TX);
	 RADEON_COLOR_ARG( 0, B );
	 RADEON_COLOR_ARG( 1, A );
	 RADEON_COLOR_ARG( 2, C );
	 break;

      case GL_DOT3_RGB_EXT:
      case GL_DOT3_RGBA_EXT:
	 /* The EXT version of the DOT3 extension does not support the
	  * scale factor, but the ARB version (and the version in OpenGL
	  * 1.3) does.
	  */
	 RGBshift = 0;
	 /* FALLTHROUGH */

      case GL_DOT3_RGB:
      case GL_DOT3_RGBA:
	 /* The R100 / RV200 only support a 1X multiplier in hardware
	  * w/the ARB version.
	  */
	 if ( RGBshift != (RADEON_SCALE_1X >> RADEON_SCALE_SHIFT) ) {
	    return GL_FALSE;
	 }

	 RGBshift += 2;
	 if ( (texUnit->_CurrentCombine->ModeRGB == GL_DOT3_RGBA_EXT)
	    || (texUnit->_CurrentCombine->ModeRGB == GL_DOT3_RGBA) ) {
            /* is it necessary to set this or will it be ignored anyway? */
	    Ashift = RGBshift;
	 }

	 color_combine = (RADEON_COLOR_ARG_C_ZERO |
			  RADEON_BLEND_CTL_DOT3 |
			  RADEON_CLAMP_TX);
	 RADEON_COLOR_ARG( 0, A );
	 RADEON_COLOR_ARG( 1, B );
	 break;

      case GL_MODULATE_ADD_ATI:
	 color_combine = (RADEON_BLEND_CTL_ADD |
			  RADEON_CLAMP_TX);
	 RADEON_COLOR_ARG( 0, A );
	 RADEON_COLOR_ARG( 1, C );
	 RADEON_COLOR_ARG( 2, B );
	 break;
      case GL_MODULATE_SIGNED_ADD_ATI:
	 color_combine = (RADEON_BLEND_CTL_ADDSIGNED |
			  RADEON_CLAMP_TX);
	 RADEON_COLOR_ARG( 0, A );
	 RADEON_COLOR_ARG( 1, C );
	 RADEON_COLOR_ARG( 2, B );
	 break;
      case GL_MODULATE_SUBTRACT_ATI:
	 color_combine = (RADEON_BLEND_CTL_SUBTRACT |
			  RADEON_CLAMP_TX);
	 RADEON_COLOR_ARG( 0, A );
	 RADEON_COLOR_ARG( 1, C );
	 RADEON_COLOR_ARG( 2, B );
	 break;
      default:
	 return GL_FALSE;
      }

      switch ( texUnit->_CurrentCombine->ModeA ) {
      case GL_REPLACE:
	 alpha_combine = (RADEON_ALPHA_ARG_A_ZERO |
			  RADEON_ALPHA_ARG_B_ZERO |
			  RADEON_BLEND_CTL_ADD |
			  RADEON_CLAMP_TX);
	 RADEON_ALPHA_ARG( 0, C );
	 break;
      case GL_MODULATE:
	 alpha_combine = (RADEON_ALPHA_ARG_C_ZERO |
			  RADEON_BLEND_CTL_ADD |
			  RADEON_CLAMP_TX);
	 RADEON_ALPHA_ARG( 0, A );
	 RADEON_ALPHA_ARG( 1, B );
	 break;
      case GL_ADD:
	 alpha_combine = (RADEON_ALPHA_ARG_B_ZERO |
			  RADEON_COMP_ARG_B |
			  RADEON_BLEND_CTL_ADD |
			  RADEON_CLAMP_TX);
	 RADEON_ALPHA_ARG( 0, A );
	 RADEON_ALPHA_ARG( 1, C );
	 break;
      case GL_ADD_SIGNED:
	 alpha_combine = (RADEON_ALPHA_ARG_B_ZERO |
			  RADEON_COMP_ARG_B |
			  RADEON_BLEND_CTL_ADDSIGNED |
			  RADEON_CLAMP_TX);
	 RADEON_ALPHA_ARG( 0, A );
	 RADEON_ALPHA_ARG( 1, C );
	 break;
      case GL_SUBTRACT:
	 alpha_combine = (RADEON_COLOR_ARG_B_ZERO |
			  RADEON_COMP_ARG_B |
			  RADEON_BLEND_CTL_SUBTRACT |
			  RADEON_CLAMP_TX);
	 RADEON_ALPHA_ARG( 0, A );
	 RADEON_ALPHA_ARG( 1, C );
	 break;
      case GL_INTERPOLATE:
	 alpha_combine = (RADEON_BLEND_CTL_BLEND |
			  RADEON_CLAMP_TX);
	 RADEON_ALPHA_ARG( 0, B );
	 RADEON_ALPHA_ARG( 1, A );
	 RADEON_ALPHA_ARG( 2, C );
	 break;

      case GL_MODULATE_ADD_ATI:
	 alpha_combine = (RADEON_BLEND_CTL_ADD |
			  RADEON_CLAMP_TX);
	 RADEON_ALPHA_ARG( 0, A );
	 RADEON_ALPHA_ARG( 1, C );
	 RADEON_ALPHA_ARG( 2, B );
	 break;
      case GL_MODULATE_SIGNED_ADD_ATI:
	 alpha_combine = (RADEON_BLEND_CTL_ADDSIGNED |
			  RADEON_CLAMP_TX);
	 RADEON_ALPHA_ARG( 0, A );
	 RADEON_ALPHA_ARG( 1, C );
	 RADEON_ALPHA_ARG( 2, B );
	 break;
      case GL_MODULATE_SUBTRACT_ATI:
	 alpha_combine = (RADEON_BLEND_CTL_SUBTRACT |
			  RADEON_CLAMP_TX);
	 RADEON_ALPHA_ARG( 0, A );
	 RADEON_ALPHA_ARG( 1, C );
	 RADEON_ALPHA_ARG( 2, B );
	 break;
      default:
	 return GL_FALSE;
      }

      if ( (texUnit->_CurrentCombine->ModeRGB == GL_DOT3_RGB_EXT)
	   || (texUnit->_CurrentCombine->ModeRGB == GL_DOT3_RGB) ) {
	 alpha_combine |= RADEON_DOT_ALPHA_DONT_REPLICATE;
      }

      /* Step 3:
       * Apply the scale factor.
       */
      color_combine |= (RGBshift << RADEON_SCALE_SHIFT);
      alpha_combine |= (Ashift   << RADEON_SCALE_SHIFT);

      /* All done!
       */
   }

   if ( rmesa->hw.tex[unit].cmd[TEX_PP_TXCBLEND] != color_combine ||
	rmesa->hw.tex[unit].cmd[TEX_PP_TXABLEND] != alpha_combine ) {
      RADEON_STATECHANGE( rmesa, tex[unit] );
      rmesa->hw.tex[unit].cmd[TEX_PP_TXCBLEND] = color_combine;
      rmesa->hw.tex[unit].cmd[TEX_PP_TXABLEND] = alpha_combine;
   }

   return GL_TRUE;
}

void radeonSetTexBuffer2(__DRIcontext *pDRICtx, GLint target, GLint texture_format,
			 __DRIdrawable *dPriv)
{
	struct gl_texture_unit *texUnit;
	struct gl_texture_object *texObj;
	struct gl_texture_image *texImage;
	struct radeon_renderbuffer *rb;
	radeon_texture_image *rImage;
	radeonContextPtr radeon;
	struct radeon_framebuffer *rfb;
	radeonTexObjPtr t;
	uint32_t pitch_val;
	gl_format texFormat;

	radeon = pDRICtx->driverPrivate;

	rfb = dPriv->driverPrivate;
        texUnit = _mesa_get_current_tex_unit(radeon->glCtx);
	texObj = _mesa_select_tex_object(radeon->glCtx, texUnit, target);
        texImage = _mesa_get_tex_image(radeon->glCtx, texObj, target, 0);

	rImage = get_radeon_texture_image(texImage);
	t = radeon_tex_obj(texObj);
        if (t == NULL) {
    	    return;
    	}

	radeon_update_renderbuffers(pDRICtx, dPriv, GL_TRUE);
	rb = rfb->color_rb[0];
	if (rb->bo == NULL) {
		/* Failed to BO for the buffer */
		return;
	}

	_mesa_lock_texture(radeon->glCtx, texObj);
	if (t->bo) {
		radeon_bo_unref(t->bo);
		t->bo = NULL;
	}
	if (rImage->bo) {
		radeon_bo_unref(rImage->bo);
		rImage->bo = NULL;
	}

	radeon_miptree_unreference(&t->mt);
	radeon_miptree_unreference(&rImage->mt);

	rImage->bo = rb->bo;
	radeon_bo_ref(rImage->bo);
	t->bo = rb->bo;
	radeon_bo_ref(t->bo);
	t->tile_bits = 0;
	t->image_override = GL_TRUE;
	t->override_offset = 0;
	switch (rb->cpp) {
	case 4:
		if (texture_format == __DRI_TEXTURE_FORMAT_RGB) {
			t->pp_txformat = tx_table[MESA_FORMAT_RGB888].format;
			texFormat = MESA_FORMAT_RGB888;
		}
		else {
			t->pp_txformat = tx_table[MESA_FORMAT_ARGB8888].format;
			texFormat = MESA_FORMAT_ARGB8888;
		}
		t->pp_txfilter |= tx_table[MESA_FORMAT_ARGB8888].filter;
		break;
	case 3:
	default:
		texFormat = MESA_FORMAT_RGB888;
		t->pp_txformat = tx_table[MESA_FORMAT_RGB888].format;
		t->pp_txfilter |= tx_table[MESA_FORMAT_RGB888].filter;
		break;
	case 2:
		texFormat = MESA_FORMAT_RGB565;
		t->pp_txformat = tx_table[MESA_FORMAT_RGB565].format;
		t->pp_txfilter |= tx_table[MESA_FORMAT_RGB565].filter;
		break;
	}

	_mesa_init_teximage_fields(radeon->glCtx, texImage,
				   rb->base.Base.Width, rb->base.Base.Height,
				   1, 0,
				   rb->cpp, texFormat);
	rImage->base.RowStride = rb->pitch / rb->cpp;

	t->pp_txpitch &= (1 << 13) -1;
	pitch_val = rb->pitch;

        t->pp_txsize = ((rb->base.Base.Width - 1) << RADEON_TEX_USIZE_SHIFT)
		| ((rb->base.Base.Height - 1) << RADEON_TEX_VSIZE_SHIFT);
	if (target == GL_TEXTURE_RECTANGLE_NV) {
		t->pp_txformat |= RADEON_TXFORMAT_NON_POWER2;
		t->pp_txpitch = pitch_val;
		t->pp_txpitch -= 32;
	} else {
	  t->pp_txformat &= ~(RADEON_TXFORMAT_WIDTH_MASK |
			      RADEON_TXFORMAT_HEIGHT_MASK |
			      RADEON_TXFORMAT_CUBIC_MAP_ENABLE |
			      RADEON_TXFORMAT_F5_WIDTH_MASK |
			      RADEON_TXFORMAT_F5_HEIGHT_MASK);
	  t->pp_txformat |= ((texImage->WidthLog2 << RADEON_TXFORMAT_WIDTH_SHIFT) |
			     (texImage->HeightLog2 << RADEON_TXFORMAT_HEIGHT_SHIFT));
	}
	t->validated = GL_TRUE;
	_mesa_unlock_texture(radeon->glCtx, texObj);
	return;
}


void radeonSetTexBuffer(__DRIcontext *pDRICtx, GLint target, __DRIdrawable *dPriv)
{
        radeonSetTexBuffer2(pDRICtx, target, __DRI_TEXTURE_FORMAT_RGBA, dPriv);
}


#define TEXOBJ_TXFILTER_MASK (RADEON_MAX_MIP_LEVEL_MASK |	\
			      RADEON_MIN_FILTER_MASK | 		\
			      RADEON_MAG_FILTER_MASK |		\
			      RADEON_MAX_ANISO_MASK |		\
			      RADEON_YUV_TO_RGB |		\
			      RADEON_YUV_TEMPERATURE_MASK |	\
			      RADEON_CLAMP_S_MASK | 		\
			      RADEON_CLAMP_T_MASK | 		\
			      RADEON_BORDER_MODE_D3D )

#define TEXOBJ_TXFORMAT_MASK (RADEON_TXFORMAT_WIDTH_MASK |	\
			      RADEON_TXFORMAT_HEIGHT_MASK |	\
			      RADEON_TXFORMAT_FORMAT_MASK |	\
                              RADEON_TXFORMAT_F5_WIDTH_MASK |	\
                              RADEON_TXFORMAT_F5_HEIGHT_MASK |	\
			      RADEON_TXFORMAT_ALPHA_IN_MAP |	\
			      RADEON_TXFORMAT_CUBIC_MAP_ENABLE |	\
                              RADEON_TXFORMAT_NON_POWER2)


static void disable_tex_obj_state( r100ContextPtr rmesa, 
				   int unit )
{
   RADEON_STATECHANGE( rmesa, tex[unit] );

   RADEON_STATECHANGE( rmesa, tcl );
   rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXFMT] &= ~(RADEON_ST_BIT(unit) |
					     RADEON_Q_BIT(unit));
   
   if (rmesa->radeon.TclFallback & (RADEON_TCL_FALLBACK_TEXGEN_0<<unit)) {
     TCL_FALLBACK( rmesa->radeon.glCtx, (RADEON_TCL_FALLBACK_TEXGEN_0<<unit), GL_FALSE);
     rmesa->recheck_texgen[unit] = GL_TRUE;
   }

   if (rmesa->hw.tex[unit].cmd[TEX_PP_TXFORMAT] & RADEON_TXFORMAT_CUBIC_MAP_ENABLE) {
     /* this seems to be a genuine (r100 only?) hw bug. Need to remove the
	cubic_map bit on unit 2 when the unit is disabled, otherwise every
	2nd (2d) mipmap on unit 0 will be broken (may not be needed for other
	units, better be safe than sorry though).*/
     RADEON_STATECHANGE( rmesa, tex[unit] );
     rmesa->hw.tex[unit].cmd[TEX_PP_TXFORMAT] &= ~RADEON_TXFORMAT_CUBIC_MAP_ENABLE;
   }

   {
      GLuint inputshift = RADEON_TEXGEN_0_INPUT_SHIFT + unit*4;
      GLuint tmp = rmesa->TexGenEnabled;

      rmesa->TexGenEnabled &= ~(RADEON_TEXGEN_TEXMAT_0_ENABLE<<unit);
      rmesa->TexGenEnabled &= ~(RADEON_TEXMAT_0_ENABLE<<unit);
      rmesa->TexGenEnabled &= ~(RADEON_TEXGEN_INPUT_MASK<<inputshift);
      rmesa->TexGenNeedNormals[unit] = 0;
      rmesa->TexGenEnabled |= 
	(RADEON_TEXGEN_INPUT_TEXCOORD_0+unit) << inputshift;

      if (tmp != rmesa->TexGenEnabled) {
	rmesa->recheck_texgen[unit] = GL_TRUE;
	rmesa->radeon.NewGLState |= _NEW_TEXTURE_MATRIX;
      }
   }
}

static void import_tex_obj_state( r100ContextPtr rmesa,
				  int unit,
				  radeonTexObjPtr texobj )
{
/* do not use RADEON_DB_STATE to avoid stale texture caches */
   uint32_t *cmd = &rmesa->hw.tex[unit].cmd[TEX_CMD_0];
   GLuint se_coord_fmt = rmesa->hw.set.cmd[SET_SE_COORDFMT];

   RADEON_STATECHANGE( rmesa, tex[unit] );

   cmd[TEX_PP_TXFILTER] &= ~TEXOBJ_TXFILTER_MASK;
   cmd[TEX_PP_TXFILTER] |= texobj->pp_txfilter & TEXOBJ_TXFILTER_MASK;
   cmd[TEX_PP_TXFORMAT] &= ~TEXOBJ_TXFORMAT_MASK;
   cmd[TEX_PP_TXFORMAT] |= texobj->pp_txformat & TEXOBJ_TXFORMAT_MASK;
   cmd[TEX_PP_BORDER_COLOR] = texobj->pp_border_color;

   if (texobj->pp_txformat & RADEON_TXFORMAT_NON_POWER2) {
      uint32_t *txr_cmd = &rmesa->hw.txr[unit].cmd[TXR_CMD_0];
      txr_cmd[TXR_PP_TEX_SIZE] = texobj->pp_txsize; /* NPOT only! */
      txr_cmd[TXR_PP_TEX_PITCH] = texobj->pp_txpitch; /* NPOT only! */
      RADEON_STATECHANGE( rmesa, txr[unit] );
   }

   if (texobj->base.Target == GL_TEXTURE_RECTANGLE_NV) {
      se_coord_fmt |= RADEON_VTX_ST0_NONPARAMETRIC << unit;
   }
   else {
      se_coord_fmt &= ~(RADEON_VTX_ST0_NONPARAMETRIC << unit);

      if (texobj->base.Target == GL_TEXTURE_CUBE_MAP) {
	 uint32_t *cube_cmd = &rmesa->hw.cube[unit].cmd[CUBE_CMD_0];

	 RADEON_STATECHANGE( rmesa, cube[unit] );
	 cube_cmd[CUBE_PP_CUBIC_FACES] = texobj->pp_cubic_faces;
	 /* state filled out in the cube_emit */
      }
   }

   if (se_coord_fmt != rmesa->hw.set.cmd[SET_SE_COORDFMT]) {
      RADEON_STATECHANGE( rmesa, set );
      rmesa->hw.set.cmd[SET_SE_COORDFMT] = se_coord_fmt;
   }

   rmesa->radeon.NewGLState |= _NEW_TEXTURE_MATRIX;
}


static void set_texgen_matrix( r100ContextPtr rmesa, 
			       GLuint unit,
			       const GLfloat *s_plane,
			       const GLfloat *t_plane,
			       const GLfloat *r_plane,
			       const GLfloat *q_plane )
{
   rmesa->TexGenMatrix[unit].m[0]  = s_plane[0];
   rmesa->TexGenMatrix[unit].m[4]  = s_plane[1];
   rmesa->TexGenMatrix[unit].m[8]  = s_plane[2];
   rmesa->TexGenMatrix[unit].m[12] = s_plane[3];

   rmesa->TexGenMatrix[unit].m[1]  = t_plane[0];
   rmesa->TexGenMatrix[unit].m[5]  = t_plane[1];
   rmesa->TexGenMatrix[unit].m[9]  = t_plane[2];
   rmesa->TexGenMatrix[unit].m[13] = t_plane[3];

   rmesa->TexGenMatrix[unit].m[2]  = r_plane[0];
   rmesa->TexGenMatrix[unit].m[6]  = r_plane[1];
   rmesa->TexGenMatrix[unit].m[10] = r_plane[2];
   rmesa->TexGenMatrix[unit].m[14] = r_plane[3];

   rmesa->TexGenMatrix[unit].m[3]  = q_plane[0];
   rmesa->TexGenMatrix[unit].m[7]  = q_plane[1];
   rmesa->TexGenMatrix[unit].m[11] = q_plane[2];
   rmesa->TexGenMatrix[unit].m[15] = q_plane[3];

   rmesa->TexGenEnabled |= RADEON_TEXMAT_0_ENABLE << unit;
   rmesa->radeon.NewGLState |= _NEW_TEXTURE_MATRIX;
}

/* Returns GL_FALSE if fallback required.
 */
static GLboolean radeon_validate_texgen( struct gl_context *ctx, GLuint unit )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
   GLuint inputshift = RADEON_TEXGEN_0_INPUT_SHIFT + unit*4;
   GLuint tmp = rmesa->TexGenEnabled;
   static const GLfloat reflect[16] = {
      -1,  0,  0,  0,
       0, -1,  0,  0,
       0,  0,  -1, 0,
       0,  0,  0,  1 };

   rmesa->TexGenEnabled &= ~(RADEON_TEXGEN_TEXMAT_0_ENABLE << unit);
   rmesa->TexGenEnabled &= ~(RADEON_TEXMAT_0_ENABLE << unit);
   rmesa->TexGenEnabled &= ~(RADEON_TEXGEN_INPUT_MASK << inputshift);
   rmesa->TexGenNeedNormals[unit] = 0;

   if ((texUnit->TexGenEnabled & (S_BIT|T_BIT|R_BIT|Q_BIT)) == 0) {
      /* Disabled, no fallback:
       */
      rmesa->TexGenEnabled |=
	 (RADEON_TEXGEN_INPUT_TEXCOORD_0 + unit) << inputshift;
      return GL_TRUE;
   }
   /* the r100 cannot do texgen for some coords and not for others
    * we do not detect such cases (certainly can't do it here) and just
    * ASSUME that when S and T are texgen enabled we do not need other
    * non-texgen enabled coords, no matter if the R and Q bits are texgen
    * enabled. Still check for mixed mode texgen for all coords.
    */
   else if ( (texUnit->TexGenEnabled & S_BIT) &&
	     (texUnit->TexGenEnabled & T_BIT) &&
	     (texUnit->GenS.Mode == texUnit->GenT.Mode) ) {
      if ( ((texUnit->TexGenEnabled & R_BIT) &&
	    (texUnit->GenS.Mode != texUnit->GenR.Mode)) ||
	   ((texUnit->TexGenEnabled & Q_BIT) &&
	    (texUnit->GenS.Mode != texUnit->GenQ.Mode)) ) {
	 /* Mixed modes, fallback:
	  */
	 if (RADEON_DEBUG & RADEON_FALLBACKS)
	    fprintf(stderr, "fallback mixed texgen\n");
	 return GL_FALSE;
      }
      rmesa->TexGenEnabled |= RADEON_TEXGEN_TEXMAT_0_ENABLE << unit;
   }
   else {
   /* some texgen mode not including both S and T bits */
      if (RADEON_DEBUG & RADEON_FALLBACKS)
	 fprintf(stderr, "fallback mixed texgen/nontexgen\n");
      return GL_FALSE;
   }

   if ((texUnit->TexGenEnabled & (R_BIT | Q_BIT)) != 0) {
      /* need this here for vtxfmt presumably. Argh we need to set
         this from way too many places, would be much easier if we could leave
         tcl q coord always enabled as on r200) */
      RADEON_STATECHANGE( rmesa, tcl );
      rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXFMT] |= RADEON_Q_BIT(unit);
   }

   switch (texUnit->GenS.Mode) {
   case GL_OBJECT_LINEAR:
      rmesa->TexGenEnabled |= RADEON_TEXGEN_INPUT_OBJ << inputshift;
      set_texgen_matrix( rmesa, unit,
			 texUnit->GenS.ObjectPlane,
			 texUnit->GenT.ObjectPlane,
			 texUnit->GenR.ObjectPlane,
			 texUnit->GenQ.ObjectPlane);
      break;

   case GL_EYE_LINEAR:
      rmesa->TexGenEnabled |= RADEON_TEXGEN_INPUT_EYE << inputshift;
      set_texgen_matrix( rmesa, unit,
			 texUnit->GenS.EyePlane,
			 texUnit->GenT.EyePlane,
			 texUnit->GenR.EyePlane,
			 texUnit->GenQ.EyePlane);
      break;

   case GL_REFLECTION_MAP_NV:
      rmesa->TexGenNeedNormals[unit] = GL_TRUE;
      rmesa->TexGenEnabled |= RADEON_TEXGEN_INPUT_EYE_REFLECT << inputshift;
      /* TODO: unknown if this is needed/correct */
      set_texgen_matrix( rmesa, unit, reflect, reflect + 4,
			reflect + 8, reflect + 12 );
      break;

   case GL_NORMAL_MAP_NV:
      rmesa->TexGenNeedNormals[unit] = GL_TRUE;
      rmesa->TexGenEnabled |= RADEON_TEXGEN_INPUT_EYE_NORMAL << inputshift;
      break;

   case GL_SPHERE_MAP:
      /* the mode which everyone uses :-( */
   default:
      /* Unsupported mode, fallback:
       */
      if (RADEON_DEBUG & RADEON_FALLBACKS)
	 fprintf(stderr, "fallback GL_SPHERE_MAP\n");
      return GL_FALSE;
   }

   if (tmp != rmesa->TexGenEnabled) {
      rmesa->radeon.NewGLState |= _NEW_TEXTURE_MATRIX;
   }

   return GL_TRUE;
}

/**
 * Compute the cached hardware register values for the given texture object.
 *
 * \param rmesa Context pointer
 * \param t the r300 texture object
 */
static GLboolean setup_hardware_state(r100ContextPtr rmesa, radeonTexObj *t, int unit)
{
   const struct gl_texture_image *firstImage;
   GLint log2Width, log2Height, texelBytes;

   if ( t->bo ) {
	return GL_TRUE;
   }

   firstImage = t->base.Image[0][t->minLod];

   log2Width  = firstImage->WidthLog2;
   log2Height = firstImage->HeightLog2;
   texelBytes = _mesa_get_format_bytes(firstImage->TexFormat);

   if (!t->image_override) {
      if (VALID_FORMAT(firstImage->TexFormat)) {
	const struct tx_table *table = tx_table;

	 t->pp_txformat &= ~(RADEON_TXFORMAT_FORMAT_MASK |
			     RADEON_TXFORMAT_ALPHA_IN_MAP);
	 t->pp_txfilter &= ~RADEON_YUV_TO_RGB;	 
	 
	 t->pp_txformat |= table[ firstImage->TexFormat ].format;
	 t->pp_txfilter |= table[ firstImage->TexFormat ].filter;
      } else {
	 _mesa_problem(NULL, "unexpected texture format in %s",
		       __FUNCTION__);
	 return GL_FALSE;
      }
   }

   t->pp_txfilter &= ~RADEON_MAX_MIP_LEVEL_MASK;
   t->pp_txfilter |= (t->maxLod - t->minLod) << RADEON_MAX_MIP_LEVEL_SHIFT;
	
   t->pp_txformat &= ~(RADEON_TXFORMAT_WIDTH_MASK |
		       RADEON_TXFORMAT_HEIGHT_MASK |
		       RADEON_TXFORMAT_CUBIC_MAP_ENABLE |
		       RADEON_TXFORMAT_F5_WIDTH_MASK |
		       RADEON_TXFORMAT_F5_HEIGHT_MASK);
   t->pp_txformat |= ((log2Width << RADEON_TXFORMAT_WIDTH_SHIFT) |
		      (log2Height << RADEON_TXFORMAT_HEIGHT_SHIFT));

   t->tile_bits = 0;

   if (t->base.Target == GL_TEXTURE_CUBE_MAP) {
      ASSERT(log2Width == log2Height);
      t->pp_txformat |= ((log2Width << RADEON_TXFORMAT_F5_WIDTH_SHIFT) |
			 (log2Height << RADEON_TXFORMAT_F5_HEIGHT_SHIFT) |
			 /* don't think we need this bit, if it exists at all - fglrx does not set it */
			 (RADEON_TXFORMAT_CUBIC_MAP_ENABLE));
      t->pp_cubic_faces = ((log2Width << RADEON_FACE_WIDTH_1_SHIFT) |
                           (log2Height << RADEON_FACE_HEIGHT_1_SHIFT) |
                           (log2Width << RADEON_FACE_WIDTH_2_SHIFT) |
                           (log2Height << RADEON_FACE_HEIGHT_2_SHIFT) |
                           (log2Width << RADEON_FACE_WIDTH_3_SHIFT) |
                           (log2Height << RADEON_FACE_HEIGHT_3_SHIFT) |
                           (log2Width << RADEON_FACE_WIDTH_4_SHIFT) |
                           (log2Height << RADEON_FACE_HEIGHT_4_SHIFT));
   }

   t->pp_txsize = (((firstImage->Width - 1) << RADEON_TEX_USIZE_SHIFT)
		   | ((firstImage->Height - 1) << RADEON_TEX_VSIZE_SHIFT));

   if ( !t->image_override ) {
      if (_mesa_is_format_compressed(firstImage->TexFormat))
         t->pp_txpitch = (firstImage->Width + 63) & ~(63);
      else
         t->pp_txpitch = ((firstImage->Width * texelBytes) + 63) & ~(63);
      t->pp_txpitch -= 32;
   }

   if (t->base.Target == GL_TEXTURE_RECTANGLE_NV) {
      t->pp_txformat |= RADEON_TXFORMAT_NON_POWER2;
   }

   return GL_TRUE;
}

static GLboolean radeon_validate_texture(struct gl_context *ctx, struct gl_texture_object *texObj, int unit)
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   radeonTexObj *t = radeon_tex_obj(texObj);
   int ret;

   if (!radeon_validate_texture_miptree(ctx, _mesa_get_samplerobj(ctx, unit), texObj))
      return GL_FALSE;

   ret = setup_hardware_state(rmesa, t, unit);
   if (ret == GL_FALSE)
     return GL_FALSE;

   /* yuv conversion only works in first unit */
   if (unit != 0 && (t->pp_txfilter & RADEON_YUV_TO_RGB))
      return GL_FALSE;

   RADEON_STATECHANGE( rmesa, ctx );
   rmesa->hw.ctx.cmd[CTX_PP_CNTL] |= 
     (RADEON_TEX_0_ENABLE | RADEON_TEX_BLEND_0_ENABLE) << unit;
   RADEON_STATECHANGE( rmesa, tcl );
   rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXFMT] |= RADEON_ST_BIT(unit);

   rmesa->recheck_texgen[unit] = GL_TRUE;

   radeonTexUpdateParameters(ctx, unit);
   import_tex_obj_state( rmesa, unit, t );

   if (rmesa->recheck_texgen[unit]) {
      GLboolean fallback = !radeon_validate_texgen( ctx, unit );
      TCL_FALLBACK( ctx, (RADEON_TCL_FALLBACK_TEXGEN_0<<unit), fallback);
      rmesa->recheck_texgen[unit] = 0;
      rmesa->radeon.NewGLState |= _NEW_TEXTURE_MATRIX;
   }

   if ( ! radeonUpdateTextureEnv( ctx, unit ) ) {
     return GL_FALSE;
   }
   FALLBACK( rmesa, RADEON_FALLBACK_BORDER_MODE, t->border_fallback );

   t->validated = GL_TRUE;
   return !t->border_fallback;
}

static GLboolean radeonUpdateTextureUnit( struct gl_context *ctx, int unit )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);

   if (ctx->Texture.Unit[unit]._ReallyEnabled & TEXTURE_3D_BIT) {
     disable_tex_obj_state(rmesa, unit);
     rmesa->state.texture.unit[unit].texobj = NULL;
     return GL_FALSE;
   }

   if (!ctx->Texture.Unit[unit]._ReallyEnabled) {
     /* disable the unit */
     disable_tex_obj_state(rmesa, unit);
     rmesa->state.texture.unit[unit].texobj = NULL;
     return GL_TRUE;
   }

   if (!radeon_validate_texture(ctx, ctx->Texture.Unit[unit]._Current, unit)) {
    _mesa_warning(ctx,
		  "failed to validate texture for unit %d.\n",
		  unit);
     rmesa->state.texture.unit[unit].texobj = NULL;
     return GL_FALSE;
   }
   rmesa->state.texture.unit[unit].texobj = radeon_tex_obj(ctx->Texture.Unit[unit]._Current);
   return GL_TRUE;
}

void radeonUpdateTextureState( struct gl_context *ctx )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   GLboolean ok;

   /* set the ctx all textures off */
   RADEON_STATECHANGE( rmesa, ctx );
   rmesa->hw.ctx.cmd[CTX_PP_CNTL] &= ~((RADEON_TEX_ENABLE_MASK) | (RADEON_TEX_BLEND_ENABLE_MASK));

   ok = (radeonUpdateTextureUnit( ctx, 0 ) &&
	 radeonUpdateTextureUnit( ctx, 1 ) &&
	 radeonUpdateTextureUnit( ctx, 2 ));

   FALLBACK( rmesa, RADEON_FALLBACK_TEXTURE, !ok );

   if (rmesa->radeon.TclFallback)
      radeonChooseVertexState( ctx );
}
