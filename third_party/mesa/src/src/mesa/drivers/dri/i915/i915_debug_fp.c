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

#include <stdio.h>

#include "i915_reg.h"
#include "i915_debug.h"
#include "main/imports.h"

static const char *opcodes[0x20] = {
   "NOP",
   "ADD",
   "MOV",
   "MUL",
   "MAD",
   "DP2ADD",
   "DP3",
   "DP4",
   "FRC",
   "RCP",
   "RSQ",
   "EXP",
   "LOG",
   "CMP",
   "MIN",
   "MAX",
   "FLR",
   "MOD",
   "TRC",
   "SGE",
   "SLT",
   "TEXLD",
   "TEXLDP",
   "TEXLDB",
   "TEXKILL",
   "DCL",
   "0x1a",
   "0x1b",
   "0x1c",
   "0x1d",
   "0x1e",
   "0x1f",
};


static const int args[0x20] = {
   0,                           /* 0 nop */
   2,                           /* 1 add */
   1,                           /* 2 mov */
   2,                           /* 3 m ul */
   3,                           /* 4 mad */
   3,                           /* 5 dp2add */
   2,                           /* 6 dp3 */
   2,                           /* 7 dp4 */
   1,                           /* 8 frc */
   1,                           /* 9 rcp */
   1,                           /* a rsq */
   1,                           /* b exp */
   1,                           /* c log */
   3,                           /* d cmp */
   2,                           /* e min */
   2,                           /* f max */
   1,                           /* 10 flr */
   1,                           /* 11 mod */
   1,                           /* 12 trc */
   2,                           /* 13 sge */
   2,                           /* 14 slt */
   1,
   1,
   1,
   1,
   0,
   0,
   0,
   0,
   0,
   0,
   0,
};


static const char *regname[0x8] = {
   "R",
   "T",
   "CONST",
   "S",
   "OC",
   "OD",
   "U",
   "UNKNOWN",
};

static void
print_reg_type_nr(GLuint type, GLuint nr)
{
   switch (type) {
   case REG_TYPE_T:
      switch (nr) {
      case T_DIFFUSE:
         printf("T_DIFFUSE");
         return;
      case T_SPECULAR:
         printf("T_SPECULAR");
         return;
      case T_FOG_W:
         printf("T_FOG_W");
         return;
      default:
         printf("T_TEX%d", nr);
         return;
      }
   case REG_TYPE_OC:
      if (nr == 0) {
         printf("oC");
         return;
      }
      break;
   case REG_TYPE_OD:
      if (nr == 0) {
         printf("oD");
         return;
      }
      break;
   default:
      break;
   }

   printf("%s[%d]", regname[type], nr);
}

#define REG_SWIZZLE_MASK 0x7777
#define REG_NEGATE_MASK 0x8888

#define REG_SWIZZLE_XYZW ((SRC_X << A2_SRC2_CHANNEL_X_SHIFT) |	\
		      (SRC_Y << A2_SRC2_CHANNEL_Y_SHIFT) |	\
		      (SRC_Z << A2_SRC2_CHANNEL_Z_SHIFT) |	\
		      (SRC_W << A2_SRC2_CHANNEL_W_SHIFT))


static void
print_reg_neg_swizzle(GLuint reg)
{
   int i;

   if ((reg & REG_SWIZZLE_MASK) == REG_SWIZZLE_XYZW &&
       (reg & REG_NEGATE_MASK) == 0)
      return;

   printf(".");

   for (i = 3; i >= 0; i--) {
      if (reg & (1 << ((i * 4) + 3)))
         printf("-");

      switch ((reg >> (i * 4)) & 0x7) {
      case 0:
         printf("x");
         break;
      case 1:
         printf("y");
         break;
      case 2:
         printf("z");
         break;
      case 3:
         printf("w");
         break;
      case 4:
         printf("0");
         break;
      case 5:
         printf("1");
         break;
      default:
         printf("?");
         break;
      }
   }
}


static void
print_src_reg(GLuint dword)
{
   GLuint nr = (dword >> A2_SRC2_NR_SHIFT) & REG_NR_MASK;
   GLuint type = (dword >> A2_SRC2_TYPE_SHIFT) & REG_TYPE_MASK;
   print_reg_type_nr(type, nr);
   print_reg_neg_swizzle(dword);
}


static void
print_dest_reg(GLuint dword)
{
   GLuint nr = (dword >> A0_DEST_NR_SHIFT) & REG_NR_MASK;
   GLuint type = (dword >> A0_DEST_TYPE_SHIFT) & REG_TYPE_MASK;
   print_reg_type_nr(type, nr);
   if ((dword & A0_DEST_CHANNEL_ALL) == A0_DEST_CHANNEL_ALL)
      return;
   printf(".");
   if (dword & A0_DEST_CHANNEL_X)
      printf("x");
   if (dword & A0_DEST_CHANNEL_Y)
      printf("y");
   if (dword & A0_DEST_CHANNEL_Z)
      printf("z");
   if (dword & A0_DEST_CHANNEL_W)
      printf("w");
}


#define GET_SRC0_REG(r0, r1) ((r0<<14)|(r1>>A1_SRC0_CHANNEL_W_SHIFT))
#define GET_SRC1_REG(r0, r1) ((r0<<8)|(r1>>A2_SRC1_CHANNEL_W_SHIFT))
#define GET_SRC2_REG(r)      (r)


static void
print_arith_op(GLuint opcode, const GLuint * program)
{
   if (opcode != A0_NOP) {
      print_dest_reg(program[0]);
      if (program[0] & A0_DEST_SATURATE)
         printf(" = SATURATE ");
      else
         printf(" = ");
   }

   printf("%s ", opcodes[opcode]);

   print_src_reg(GET_SRC0_REG(program[0], program[1]));
   if (args[opcode] == 1) {
      printf("\n");
      return;
   }

   printf(", ");
   print_src_reg(GET_SRC1_REG(program[1], program[2]));
   if (args[opcode] == 2) {
      printf("\n");
      return;
   }

   printf(", ");
   print_src_reg(GET_SRC2_REG(program[2]));
   printf("\n");
   return;
}


static void
print_tex_op(GLuint opcode, const GLuint * program)
{
   print_dest_reg(program[0] | A0_DEST_CHANNEL_ALL);
   printf(" = ");

   printf("%s ", opcodes[opcode]);

   printf("S[%d],", program[0] & T0_SAMPLER_NR_MASK);

   print_reg_type_nr((program[1] >> T1_ADDRESS_REG_TYPE_SHIFT) &
                     REG_TYPE_MASK,
                     (program[1] >> T1_ADDRESS_REG_NR_SHIFT) & REG_NR_MASK);
   printf("\n");
}

static void
print_dcl_op(GLuint opcode, const GLuint * program)
{
   printf("%s ", opcodes[opcode]);
   print_dest_reg(program[0] | A0_DEST_CHANNEL_ALL);
   printf("\n");
}


void
i915_disassemble_program(const GLuint * program, GLuint sz)
{
   GLuint size = program[0] & 0x1ff;
   GLint i;

   printf("\t\tBEGIN\n");

   assert(size + 2 == sz);

   program++;
   for (i = 1; i < sz; i += 3, program += 3) {
      GLuint opcode = program[0] & (0x1f << 24);

      printf("\t\t");

      if ((GLint) opcode >= A0_NOP && opcode <= A0_SLT)
         print_arith_op(opcode >> 24, program);
      else if (opcode >= T0_TEXLD && opcode <= T0_TEXKILL)
         print_tex_op(opcode >> 24, program);
      else if (opcode == D0_DCL)
         print_dcl_op(opcode >> 24, program);
      else
         printf("Unknown opcode 0x%x\n", opcode);
   }

   printf("\t\tEND\n\n");
}


