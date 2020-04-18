/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/


#include "main/glheader.h"
#include "main/context.h"
#include "main/macros.h"
#include "main/enums.h"
#include "main/colormac.h"
#include "main/dd.h"

#include "intel_screen.h"
#include "intel_context.h"

int
intel_translate_shadow_compare_func(GLenum func)
{
   switch (func) {
   case GL_NEVER: 
       return COMPAREFUNC_ALWAYS;
   case GL_LESS: 
       return COMPAREFUNC_LEQUAL;
   case GL_LEQUAL: 
       return COMPAREFUNC_LESS;
   case GL_GREATER: 
       return COMPAREFUNC_GEQUAL;
   case GL_GEQUAL: 
      return COMPAREFUNC_GREATER;
   case GL_NOTEQUAL: 
      return COMPAREFUNC_EQUAL;
   case GL_EQUAL: 
      return COMPAREFUNC_NOTEQUAL;
   case GL_ALWAYS: 
       return COMPAREFUNC_NEVER;
   }

   fprintf(stderr, "Unknown value in %s: %x\n", __FUNCTION__, func);
   return COMPAREFUNC_NEVER;
}

int
intel_translate_compare_func(GLenum func)
{
   switch (func) {
   case GL_NEVER:
      return COMPAREFUNC_NEVER;
   case GL_LESS:
      return COMPAREFUNC_LESS;
   case GL_LEQUAL:
      return COMPAREFUNC_LEQUAL;
   case GL_GREATER:
      return COMPAREFUNC_GREATER;
   case GL_GEQUAL:
      return COMPAREFUNC_GEQUAL;
   case GL_NOTEQUAL:
      return COMPAREFUNC_NOTEQUAL;
   case GL_EQUAL:
      return COMPAREFUNC_EQUAL;
   case GL_ALWAYS:
      return COMPAREFUNC_ALWAYS;
   }

   fprintf(stderr, "Unknown value in %s: %x\n", __FUNCTION__, func);
   return COMPAREFUNC_ALWAYS;
}

int
intel_translate_stencil_op(GLenum op)
{
   switch (op) {
   case GL_KEEP:
      return STENCILOP_KEEP;
   case GL_ZERO:
      return STENCILOP_ZERO;
   case GL_REPLACE:
      return STENCILOP_REPLACE;
   case GL_INCR:
      return STENCILOP_INCRSAT;
   case GL_DECR:
      return STENCILOP_DECRSAT;
   case GL_INCR_WRAP:
      return STENCILOP_INCR;
   case GL_DECR_WRAP:
      return STENCILOP_DECR;
   case GL_INVERT:
      return STENCILOP_INVERT;
   default:
      return STENCILOP_ZERO;
   }
}

int
intel_translate_blend_factor(GLenum factor)
{
   switch (factor) {
   case GL_ZERO:
      return BLENDFACT_ZERO;
   case GL_SRC_ALPHA:
      return BLENDFACT_SRC_ALPHA;
   case GL_ONE:
      return BLENDFACT_ONE;
   case GL_SRC_COLOR:
      return BLENDFACT_SRC_COLR;
   case GL_ONE_MINUS_SRC_COLOR:
      return BLENDFACT_INV_SRC_COLR;
   case GL_DST_COLOR:
      return BLENDFACT_DST_COLR;
   case GL_ONE_MINUS_DST_COLOR:
      return BLENDFACT_INV_DST_COLR;
   case GL_ONE_MINUS_SRC_ALPHA:
      return BLENDFACT_INV_SRC_ALPHA;
   case GL_DST_ALPHA:
      return BLENDFACT_DST_ALPHA;
   case GL_ONE_MINUS_DST_ALPHA:
      return BLENDFACT_INV_DST_ALPHA;
   case GL_SRC_ALPHA_SATURATE:
      return BLENDFACT_SRC_ALPHA_SATURATE;
   case GL_CONSTANT_COLOR:
      return BLENDFACT_CONST_COLOR;
   case GL_ONE_MINUS_CONSTANT_COLOR:
      return BLENDFACT_INV_CONST_COLOR;
   case GL_CONSTANT_ALPHA:
      return BLENDFACT_CONST_ALPHA;
   case GL_ONE_MINUS_CONSTANT_ALPHA:
      return BLENDFACT_INV_CONST_ALPHA;
   }

   fprintf(stderr, "Unknown value in %s: %x\n", __FUNCTION__, factor);
   return BLENDFACT_ZERO;
}

int
intel_translate_logic_op(GLenum opcode)
{
   switch (opcode) {
   case GL_CLEAR:
      return LOGICOP_CLEAR;
   case GL_AND:
      return LOGICOP_AND;
   case GL_AND_REVERSE:
      return LOGICOP_AND_RVRSE;
   case GL_COPY:
      return LOGICOP_COPY;
   case GL_COPY_INVERTED:
      return LOGICOP_COPY_INV;
   case GL_AND_INVERTED:
      return LOGICOP_AND_INV;
   case GL_NOOP:
      return LOGICOP_NOOP;
   case GL_XOR:
      return LOGICOP_XOR;
   case GL_OR:
      return LOGICOP_OR;
   case GL_OR_INVERTED:
      return LOGICOP_OR_INV;
   case GL_NOR:
      return LOGICOP_NOR;
   case GL_EQUIV:
      return LOGICOP_EQUIV;
   case GL_INVERT:
      return LOGICOP_INV;
   case GL_OR_REVERSE:
      return LOGICOP_OR_RVRSE;
   case GL_NAND:
      return LOGICOP_NAND;
   case GL_SET:
      return LOGICOP_SET;
   default:
      return LOGICOP_SET;
   }
}
