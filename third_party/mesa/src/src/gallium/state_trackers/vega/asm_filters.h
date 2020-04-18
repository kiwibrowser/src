/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.  All Rights Reserved.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef ASM_FILTERS_H
#define ASM_FILTERS_H

static const char color_matrix_asm[] =
   "FRAG\n"
   "DCL IN[0], GENERIC[0], PERSPECTIVE\n"
   "DCL OUT[0], COLOR, CONSTANT\n"
   "DCL CONST[0..4], CONSTANT\n"
   "DCL TEMP[0..4], CONSTANT\n"
   "DCL SAMP[0], CONSTANT\n"
   "TEX TEMP[0], IN[0], SAMP[0], 2D\n"
   "MOV TEMP[1], TEMP[0].xxxx\n"
   "MOV TEMP[2], TEMP[0].yyyy\n"
   "MOV TEMP[3], TEMP[0].zzzz\n"
   "MOV TEMP[4], TEMP[0].wwww\n"
   "MUL TEMP[1], TEMP[1], CONST[0]\n"
   "MUL TEMP[2], TEMP[2], CONST[1]\n"
   "MUL TEMP[3], TEMP[3], CONST[2]\n"
   "MUL TEMP[4], TEMP[4], CONST[3]\n"
   "ADD TEMP[0], TEMP[1], CONST[4]\n"
   "ADD TEMP[0], TEMP[0], TEMP[2]\n"
   "ADD TEMP[0], TEMP[0], TEMP[3]\n"
   "ADD TEMP[0], TEMP[0], TEMP[4]\n"
   "MOV OUT[0], TEMP[0]\n"
   "END\n";

static const char convolution_asm[] =
   "FRAG\n"
   "DCL IN[0], GENERIC[0], PERSPECTIVE\n"
   "DCL OUT[0], COLOR, CONSTANT\n"
   "DCL TEMP[0..4], CONSTANT\n"
   "DCL ADDR[0], CONSTANT\n"
   "DCL CONST[0..%d], CONSTANT\n"
   "DCL SAMP[0], CONSTANT\n"
   "0: MOV TEMP[0], CONST[0].xxxx\n"
   "1: MOV TEMP[1], CONST[0].xxxx\n"
   "2: BGNLOOP :14\n"
   "3: SGE TEMP[0].z, TEMP[0].yyyy, CONST[1].xxxx\n"
   "4: IF TEMP[0].zzzz :7\n"
   "5: BRK\n"
   "6: ENDIF\n"
   "7: ARL ADDR[0].x, TEMP[0].yyyy\n"
   "8: MOV TEMP[3], CONST[ADDR[0]+2]\n"
   "9: ADD TEMP[4].xy, IN[0], TEMP[3]\n"
   "10: TEX TEMP[2], TEMP[4], SAMP[0], 2D\n"
   "11: MOV TEMP[3], CONST[ADDR[0]+%d]\n"
   "12: MAD TEMP[1], TEMP[2], TEMP[3], TEMP[1]\n"
   "13: ADD TEMP[0].y, TEMP[0].yyyy, CONST[0].yyyy\n"
   "14: ENDLOOP :2\n"
   "15: MAD OUT[0], TEMP[1], CONST[1].yyyy, CONST[1].zzzz\n"
   "16: END\n";


static const char lookup_asm[] =
   "FRAG\n"
   "DCL IN[0], GENERIC[0], PERSPECTIVE\n"
   "DCL OUT[0], COLOR, CONSTANT\n"
   "DCL TEMP[0..2], CONSTANT\n"
   "DCL CONST[0], CONSTANT\n"
   "DCL SAMP[0..1], CONSTANT\n"
   "TEX TEMP[0], IN[0], SAMP[0], 2D\n"
   "MOV TEMP[1], TEMP[0]\n"
   /* do red */
   "TEX TEMP[2], TEMP[1].xxxx, SAMP[1], 1D\n"
   "MOV TEMP[0].x, TEMP[2].xxxx\n"
   /* do blue */
   "TEX TEMP[2], TEMP[1].yyyy, SAMP[1], 1D\n"
   "MOV TEMP[0].y, TEMP[2].yyyy\n"
   /* do green */
   "TEX TEMP[2], TEMP[1].zzzz, SAMP[1], 1D\n"
   "MOV TEMP[0].z, TEMP[2].zzzz\n"
   /* do alpha */
   "TEX TEMP[2], TEMP[1].wwww, SAMP[1], 1D\n"
   "MOV TEMP[0].w, TEMP[2].wwww\n"
   "MOV OUT[0], TEMP[0]\n"
   "END\n";


static const char lookup_single_asm[] =
   "FRAG\n"
   "DCL IN[0], GENERIC[0], PERSPECTIVE\n"
   "DCL OUT[0], COLOR, CONSTANT\n"
   "DCL TEMP[0..2], CONSTANT\n"
   "DCL CONST[0], CONSTANT\n"
   "DCL SAMP[0..1], CONSTANT\n"
   "TEX TEMP[0], IN[0], SAMP[0], 2D\n"
   "TEX TEMP[1], TEMP[0].%s, SAMP[1], 1D\n"
   "MOV OUT[0], TEMP[1]\n"
   "END\n";

#endif
