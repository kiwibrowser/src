/**************************************************************************

Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
                     Tungsten Graphics Inc., Austin, Texas.

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
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#include "main/glheader.h"
#include "main/imports.h"
#include "main/context.h"
#include "main/mtypes.h"
#include "main/enums.h"
#include "main/macros.h"

#include "radeon_fog.h"

/**********************************************************************/
/*             Fog blend factor computation for hw tcl                */
/*             same calculation used as in t_vb_fog.c                 */
/**********************************************************************/

#define FOG_EXP_TABLE_SIZE 256
#define FOG_MAX (10.0)
#define EXP_FOG_MAX .0006595
#define FOG_INCR (FOG_MAX/FOG_EXP_TABLE_SIZE)
static GLfloat exp_table[FOG_EXP_TABLE_SIZE];

#if 1
#define NEG_EXP( result, narg )						\
do {									\
   GLfloat f = (GLfloat) (narg * (1.0/FOG_INCR));			\
   GLint k = (GLint) f;							\
   if (k > FOG_EXP_TABLE_SIZE-2) 					\
      result = (GLfloat) EXP_FOG_MAX;					\
   else									\
      result = exp_table[k] + (f-k)*(exp_table[k+1]-exp_table[k]);	\
} while (0)
#else
#define NEG_EXP( result, narg )					\
do {								\
   result = exp(-narg);						\
} while (0)
#endif


/**
 * Initialize the exp_table[] lookup table for approximating exp().
 */
void
radeonInitStaticFogData( void )
{
   GLfloat f = 0.0F;
   GLint i = 0;
   for ( ; i < FOG_EXP_TABLE_SIZE ; i++, f += FOG_INCR) {
      exp_table[i] = (GLfloat) exp(-f);
   }
}

/**
 * Compute per-vertex fog blend factors from fog coordinates by
 * evaluating the GL_LINEAR, GL_EXP or GL_EXP2 fog function.
 * Fog coordinates are distances from the eye (typically between the
 * near and far clip plane distances).
 * Note the fog (eye Z) coords may be negative so we use ABS(z) below.
 * Fog blend factors are in the range [0,1].
 */
float
radeonComputeFogBlendFactor( struct gl_context *ctx, GLfloat fogcoord )
{
	GLfloat end  = ctx->Fog.End;
	GLfloat d, temp;
	const GLfloat z = FABSF(fogcoord);

	switch (ctx->Fog.Mode) {
	case GL_LINEAR:
		if (ctx->Fog.Start == ctx->Fog.End)
			d = 1.0F;
		else
			d = 1.0F / (ctx->Fog.End - ctx->Fog.Start);
		temp = (end - z) * d;
		return CLAMP(temp, 0.0F, 1.0F);
		break;
	case GL_EXP:
		d = ctx->Fog.Density;
		NEG_EXP( temp, d * z );
		return temp;
		break;
	case GL_EXP2:
		d = ctx->Fog.Density*ctx->Fog.Density;
		NEG_EXP( temp, d * z * z );
		return temp;
		break;
	default:
		_mesa_problem(ctx, "Bad fog mode in make_fog_coord");
		return 0;
	}
}

