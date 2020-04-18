/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.
 
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
 
 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */
         

#include <assert.h>

#include "main/mtypes.h"
#include "program/prog_parameter.h"
#include "brw_util.h"
#include "brw_defines.h"

GLuint brw_translate_blend_equation( GLenum mode )
{
   switch (mode) {
   case GL_FUNC_ADD: 
      return BRW_BLENDFUNCTION_ADD; 
   case GL_MIN: 
      return BRW_BLENDFUNCTION_MIN; 
   case GL_MAX: 
      return BRW_BLENDFUNCTION_MAX; 
   case GL_FUNC_SUBTRACT: 
      return BRW_BLENDFUNCTION_SUBTRACT; 
   case GL_FUNC_REVERSE_SUBTRACT: 
      return BRW_BLENDFUNCTION_REVERSE_SUBTRACT; 
   default: 
      assert(0);
      return BRW_BLENDFUNCTION_ADD;
   }
}

GLuint brw_translate_blend_factor( GLenum factor )
{
   switch(factor) {
   case GL_ZERO: 
      return BRW_BLENDFACTOR_ZERO; 
   case GL_SRC_ALPHA: 
      return BRW_BLENDFACTOR_SRC_ALPHA; 
   case GL_ONE: 
      return BRW_BLENDFACTOR_ONE; 
   case GL_SRC_COLOR: 
      return BRW_BLENDFACTOR_SRC_COLOR; 
   case GL_ONE_MINUS_SRC_COLOR: 
      return BRW_BLENDFACTOR_INV_SRC_COLOR; 
   case GL_DST_COLOR: 
      return BRW_BLENDFACTOR_DST_COLOR; 
   case GL_ONE_MINUS_DST_COLOR: 
      return BRW_BLENDFACTOR_INV_DST_COLOR; 
   case GL_ONE_MINUS_SRC_ALPHA:
      return BRW_BLENDFACTOR_INV_SRC_ALPHA; 
   case GL_DST_ALPHA: 
      return BRW_BLENDFACTOR_DST_ALPHA; 
   case GL_ONE_MINUS_DST_ALPHA:
      return BRW_BLENDFACTOR_INV_DST_ALPHA; 
   case GL_SRC_ALPHA_SATURATE: 
      return BRW_BLENDFACTOR_SRC_ALPHA_SATURATE;
   case GL_CONSTANT_COLOR:
      return BRW_BLENDFACTOR_CONST_COLOR; 
   case GL_ONE_MINUS_CONSTANT_COLOR:
      return BRW_BLENDFACTOR_INV_CONST_COLOR;
   case GL_CONSTANT_ALPHA:
      return BRW_BLENDFACTOR_CONST_ALPHA; 
   case GL_ONE_MINUS_CONSTANT_ALPHA:
      return BRW_BLENDFACTOR_INV_CONST_ALPHA;

   case GL_SRC1_COLOR:
      return BRW_BLENDFACTOR_SRC1_COLOR;
   case GL_SRC1_ALPHA:
      return BRW_BLENDFACTOR_SRC1_ALPHA;
   case GL_ONE_MINUS_SRC1_COLOR:
      return BRW_BLENDFACTOR_INV_SRC1_COLOR;
   case GL_ONE_MINUS_SRC1_ALPHA:
      return BRW_BLENDFACTOR_INV_SRC1_ALPHA;

   default:
      assert(0);
      return BRW_BLENDFACTOR_ZERO;
   }   
}
