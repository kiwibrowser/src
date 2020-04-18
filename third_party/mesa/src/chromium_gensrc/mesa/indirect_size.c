/* DO NOT EDIT - This file generated automatically by glX_proto_size.py (from Mesa) script */

/*
 * (C) Copyright IBM Corporation 2004
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * IBM,
 * AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#include <X11/Xfuncproto.h>
#include <GL/gl.h>
#include "indirect_size.h"

#  if defined(__GNUC__) || (defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590))
#    define PURE __attribute__((pure))
#  else
#    define PURE
#  endif

#  if defined(__i386__) && defined(__GNUC__) && !defined(__CYGWIN__) && !defined(__MINGW32__)
#    define FASTCALL __attribute__((fastcall))
#  else
#    define FASTCALL
#  endif


#if defined(__CYGWIN__) || defined(__MINGW32__) || defined(GLX_USE_APPLEGL)
#  undef HAVE_ALIAS
#endif
#ifdef HAVE_ALIAS
#  define ALIAS2(from,to) \
    _X_INTERNAL PURE FASTCALL GLint __gl ## from ## _size( GLenum e ) \
        __attribute__ ((alias( # to )));
#  define ALIAS(from,to) ALIAS2( from, __gl ## to ## _size )
#else
#  define ALIAS(from,to) \
    _X_INTERNAL PURE FASTCALL GLint __gl ## from ## _size( GLenum e ) \
    { return __gl ## to ## _size( e ); }
#endif


_X_INTERNAL PURE FASTCALL GLint
__glCallLists_size( GLenum e )
{
    switch( e ) {
        case GL_BYTE:
        case GL_UNSIGNED_BYTE:
            return 1;
        case GL_SHORT:
        case GL_UNSIGNED_SHORT:
        case GL_2_BYTES:
        case GL_HALF_FLOAT:
            return 2;
        case GL_3_BYTES:
            return 3;
        case GL_INT:
        case GL_UNSIGNED_INT:
        case GL_FLOAT:
        case GL_4_BYTES:
            return 4;
        default: return 0;
    }
}

_X_INTERNAL PURE FASTCALL GLint
__glFogfv_size( GLenum e )
{
    switch( e ) {
        case GL_FOG_INDEX:
        case GL_FOG_DENSITY:
        case GL_FOG_START:
        case GL_FOG_END:
        case GL_FOG_MODE:
        case GL_FOG_OFFSET_VALUE_SGIX:
        case GL_FOG_DISTANCE_MODE_NV:
            return 1;
        case GL_FOG_COLOR:
            return 4;
        default: return 0;
    }
}

_X_INTERNAL PURE FASTCALL GLint
__glLightfv_size( GLenum e )
{
    switch( e ) {
        case GL_SPOT_EXPONENT:
        case GL_SPOT_CUTOFF:
        case GL_CONSTANT_ATTENUATION:
        case GL_LINEAR_ATTENUATION:
        case GL_QUADRATIC_ATTENUATION:
            return 1;
        case GL_SPOT_DIRECTION:
            return 3;
        case GL_AMBIENT:
        case GL_DIFFUSE:
        case GL_SPECULAR:
        case GL_POSITION:
            return 4;
        default: return 0;
    }
}

_X_INTERNAL PURE FASTCALL GLint
__glLightModelfv_size( GLenum e )
{
    switch( e ) {
        case GL_LIGHT_MODEL_LOCAL_VIEWER:
        case GL_LIGHT_MODEL_TWO_SIDE:
        case GL_LIGHT_MODEL_COLOR_CONTROL:
/*      case GL_LIGHT_MODEL_COLOR_CONTROL_EXT:*/
            return 1;
        case GL_LIGHT_MODEL_AMBIENT:
            return 4;
        default: return 0;
    }
}

_X_INTERNAL PURE FASTCALL GLint
__glMaterialfv_size( GLenum e )
{
    switch( e ) {
        case GL_SHININESS:
            return 1;
        case GL_COLOR_INDEXES:
            return 3;
        case GL_AMBIENT:
        case GL_DIFFUSE:
        case GL_SPECULAR:
        case GL_EMISSION:
        case GL_AMBIENT_AND_DIFFUSE:
            return 4;
        default: return 0;
    }
}

_X_INTERNAL PURE FASTCALL GLint
__glTexParameterfv_size( GLenum e )
{
    switch( e ) {
        case GL_TEXTURE_MAG_FILTER:
        case GL_TEXTURE_MIN_FILTER:
        case GL_TEXTURE_WRAP_S:
        case GL_TEXTURE_WRAP_T:
        case GL_TEXTURE_PRIORITY:
        case GL_TEXTURE_WRAP_R:
        case GL_TEXTURE_COMPARE_FAIL_VALUE_ARB:
/*      case GL_SHADOW_AMBIENT_SGIX:*/
        case GL_TEXTURE_MIN_LOD:
        case GL_TEXTURE_MAX_LOD:
        case GL_TEXTURE_BASE_LEVEL:
        case GL_TEXTURE_MAX_LEVEL:
        case GL_TEXTURE_CLIPMAP_FRAME_SGIX:
        case GL_TEXTURE_LOD_BIAS_S_SGIX:
        case GL_TEXTURE_LOD_BIAS_T_SGIX:
        case GL_TEXTURE_LOD_BIAS_R_SGIX:
        case GL_GENERATE_MIPMAP:
/*      case GL_GENERATE_MIPMAP_SGIS:*/
        case GL_TEXTURE_COMPARE_SGIX:
        case GL_TEXTURE_COMPARE_OPERATOR_SGIX:
        case GL_TEXTURE_MAX_CLAMP_S_SGIX:
        case GL_TEXTURE_MAX_CLAMP_T_SGIX:
        case GL_TEXTURE_MAX_CLAMP_R_SGIX:
        case GL_TEXTURE_MAX_ANISOTROPY_EXT:
        case GL_TEXTURE_LOD_BIAS:
/*      case GL_TEXTURE_LOD_BIAS_EXT:*/
        case GL_TEXTURE_STORAGE_HINT_APPLE:
        case GL_STORAGE_PRIVATE_APPLE:
        case GL_STORAGE_CACHED_APPLE:
        case GL_STORAGE_SHARED_APPLE:
        case GL_DEPTH_TEXTURE_MODE:
/*      case GL_DEPTH_TEXTURE_MODE_ARB:*/
        case GL_TEXTURE_COMPARE_MODE:
/*      case GL_TEXTURE_COMPARE_MODE_ARB:*/
        case GL_TEXTURE_COMPARE_FUNC:
/*      case GL_TEXTURE_COMPARE_FUNC_ARB:*/
        case GL_TEXTURE_UNSIGNED_REMAP_MODE_NV:
            return 1;
        case GL_TEXTURE_CLIPMAP_CENTER_SGIX:
        case GL_TEXTURE_CLIPMAP_OFFSET_SGIX:
            return 2;
        case GL_TEXTURE_CLIPMAP_VIRTUAL_DEPTH_SGIX:
            return 3;
        case GL_TEXTURE_BORDER_COLOR:
        case GL_POST_TEXTURE_FILTER_BIAS_SGIX:
        case GL_POST_TEXTURE_FILTER_SCALE_SGIX:
            return 4;
        default: return 0;
    }
}

_X_INTERNAL PURE FASTCALL GLint
__glTexEnvfv_size( GLenum e )
{
    switch( e ) {
        case GL_ALPHA_SCALE:
        case GL_TEXTURE_ENV_MODE:
        case GL_TEXTURE_LOD_BIAS:
        case GL_COMBINE_RGB:
        case GL_COMBINE_ALPHA:
        case GL_RGB_SCALE:
        case GL_SOURCE0_RGB:
        case GL_SOURCE1_RGB:
        case GL_SOURCE2_RGB:
        case GL_SOURCE3_RGB_NV:
        case GL_SOURCE0_ALPHA:
        case GL_SOURCE1_ALPHA:
        case GL_SOURCE2_ALPHA:
        case GL_SOURCE3_ALPHA_NV:
        case GL_OPERAND0_RGB:
        case GL_OPERAND1_RGB:
        case GL_OPERAND2_RGB:
        case GL_OPERAND3_RGB_NV:
        case GL_OPERAND0_ALPHA:
        case GL_OPERAND1_ALPHA:
        case GL_OPERAND2_ALPHA:
        case GL_OPERAND3_ALPHA_NV:
        case GL_BUMP_TARGET_ATI:
        case GL_COORD_REPLACE_ARB:
/*      case GL_COORD_REPLACE_NV:*/
            return 1;
        case GL_TEXTURE_ENV_COLOR:
            return 4;
        default: return 0;
    }
}

_X_INTERNAL PURE FASTCALL GLint
__glTexGendv_size( GLenum e )
{
    switch( e ) {
        case GL_TEXTURE_GEN_MODE:
            return 1;
        case GL_OBJECT_PLANE:
        case GL_EYE_PLANE:
            return 4;
        default: return 0;
    }
}

_X_INTERNAL PURE FASTCALL GLint
__glMap1d_size( GLenum e )
{
    switch( e ) {
        case GL_MAP1_INDEX:
        case GL_MAP1_TEXTURE_COORD_1:
            return 1;
        case GL_MAP1_TEXTURE_COORD_2:
            return 2;
        case GL_MAP1_NORMAL:
        case GL_MAP1_TEXTURE_COORD_3:
        case GL_MAP1_VERTEX_3:
            return 3;
        case GL_MAP1_COLOR_4:
        case GL_MAP1_TEXTURE_COORD_4:
        case GL_MAP1_VERTEX_4:
            return 4;
        default: return 0;
    }
}

_X_INTERNAL PURE FASTCALL GLint
__glMap2d_size( GLenum e )
{
    switch( e ) {
        case GL_MAP2_INDEX:
        case GL_MAP2_TEXTURE_COORD_1:
            return 1;
        case GL_MAP2_TEXTURE_COORD_2:
            return 2;
        case GL_MAP2_NORMAL:
        case GL_MAP2_TEXTURE_COORD_3:
        case GL_MAP2_VERTEX_3:
            return 3;
        case GL_MAP2_COLOR_4:
        case GL_MAP2_TEXTURE_COORD_4:
        case GL_MAP2_VERTEX_4:
            return 4;
        default: return 0;
    }
}

_X_INTERNAL PURE FASTCALL GLint
__glColorTableParameterfv_size( GLenum e )
{
    switch( e ) {
        case GL_COLOR_TABLE_SCALE:
        case GL_COLOR_TABLE_BIAS:
            return 4;
        default: return 0;
    }
}

_X_INTERNAL PURE FASTCALL GLint
__glConvolutionParameterfv_size( GLenum e )
{
    switch( e ) {
        case GL_CONVOLUTION_BORDER_MODE:
/*      case GL_CONVOLUTION_BORDER_MODE_EXT:*/
            return 1;
        case GL_CONVOLUTION_FILTER_SCALE:
/*      case GL_CONVOLUTION_FILTER_SCALE_EXT:*/
        case GL_CONVOLUTION_FILTER_BIAS:
/*      case GL_CONVOLUTION_FILTER_BIAS_EXT:*/
        case GL_CONVOLUTION_BORDER_COLOR:
/*      case GL_CONVOLUTION_BORDER_COLOR_HP:*/
            return 4;
        default: return 0;
    }
}

_X_INTERNAL PURE FASTCALL GLint
__glPointParameterfvEXT_size( GLenum e )
{
    switch( e ) {
        case GL_POINT_SIZE_MIN:
/*      case GL_POINT_SIZE_MIN_ARB:*/
/*      case GL_POINT_SIZE_MIN_SGIS:*/
        case GL_POINT_SIZE_MAX:
/*      case GL_POINT_SIZE_MAX_ARB:*/
/*      case GL_POINT_SIZE_MAX_SGIS:*/
        case GL_POINT_FADE_THRESHOLD_SIZE:
/*      case GL_POINT_FADE_THRESHOLD_SIZE_ARB:*/
/*      case GL_POINT_FADE_THRESHOLD_SIZE_SGIS:*/
        case GL_POINT_SPRITE_R_MODE_NV:
        case GL_POINT_SPRITE_COORD_ORIGIN:
            return 1;
        case GL_POINT_DISTANCE_ATTENUATION:
/*      case GL_POINT_DISTANCE_ATTENUATION_ARB:*/
/*      case GL_POINT_DISTANCE_ATTENUATION_SGIS:*/
            return 3;
        default: return 0;
    }
}

ALIAS( Fogiv, Fogfv )
ALIAS( Lightiv, Lightfv )
ALIAS( LightModeliv, LightModelfv )
ALIAS( Materialiv, Materialfv )
ALIAS( TexParameteriv, TexParameterfv )
ALIAS( TexEnviv, TexEnvfv )
ALIAS( TexGenfv, TexGendv )
ALIAS( TexGeniv, TexGendv )
ALIAS( Map1f, Map1d )
ALIAS( Map2f, Map2d )
ALIAS( ColorTableParameteriv, ColorTableParameterfv )
ALIAS( ConvolutionParameteriv, ConvolutionParameterfv )
ALIAS( PointParameterivNV, PointParameterfvEXT )

#  undef PURE
#  undef FASTCALL
