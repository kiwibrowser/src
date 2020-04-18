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

#ifndef ASM_UTIL_H
#define ASM_UTIL_H

/* μnew = μmask */
static const char set_mask_asm[] =
   "FRAG\n"
   "DCL IN[0], GENERIC[0], PERSPECTIVE\n"
   "DCL SAMP[0], CONSTANT\n"
   "DCL OUT[0], COLOR, CONSTANT\n"
   "0: TEX OUT[0], IN[0], SAMP[0], 2D\n"/*umask*/
   "1: END\n";

/* μnew = 1 – (1 – μmask)*(1 – μprev) */
static const char union_mask_asm[] =
   "FRAG\n"
   "DCL IN[0], GENERIC[0], PERSPECTIVE\n"
   "DCL IN[1], POSITION, LINEAR\n"
   "DCL CONST[0], CONSTANT\n"
   "DCL SAMP[0..1], CONSTANT\n"
   "DCL TEMP[0..3], CONSTANT\n"
   "DCL OUT[0], COLOR, CONSTANT\n"
   "0: TEX TEMP[1], IN[0], SAMP[0], 2D\n"/*umask*/
   "1: TEX TEMP[0], IN[1], SAMP[1], 2D\n"/*uprev*/
   "2: SUB TEMP[2], CONST[0], TEMP[0]\n"
   "3: SUB TEMP[3], CONST[0], TEMP[1]\n"
   "4: MUL TEMP[0].w, TEMP[2].wwww, TEMP[3].wwww\n"
   "5: SUB OUT[0], CONST[0], TEMP[0]\n"
   "6: END\n";

/* μnew = μmask *μprev */
static const char intersect_mask_asm[] =
   "FRAG\n"
   "DCL IN[0], GENERIC[0], PERSPECTIVE\n"
   "DCL IN[1], POSITION, LINEAR\n"
   "DCL CONST[0], CONSTANT\n"
   "DCL SAMP[0..1], CONSTANT\n"
   "DCL TEMP[0..1], CONSTANT\n"
   "DCL OUT[0], COLOR, CONSTANT\n"
   "0: TEX TEMP[0], IN[1], SAMP[1], 2D\n"/*uprev*/
   "1: TEX TEMP[1], IN[0], SAMP[0], 2D\n"/*umask*/
   "2: MUL OUT[0], TEMP[0].wwww, TEMP[1].wwww\n"
   "3: END\n";

/* μnew = μprev*(1 – μmask) */
static const char subtract_mask_asm[] =
   "FRAG\n"
   "DCL IN[0], GENERIC[0], PERSPECTIVE\n"
   "DCL IN[1], POSITION, LINEAR\n"
   "DCL CONST[0], CONSTANT\n"
   "DCL SAMP[0..1], CONSTANT\n"
   "DCL TEMP[0..2], CONSTANT\n"
   "DCL OUT[0], COLOR, CONSTANT\n"
   "0: TEX TEMP[1], IN[0], SAMP[0], 2D\n"/*umask*/
   "1: TEX TEMP[0], IN[1], SAMP[1], 2D\n"/*uprev*/
   "2: SUB TEMP[2], CONST[0], TEMP[1]\n"
   "3: MUL OUT[0], TEMP[2].wwww, TEMP[0].wwww\n"
   "4: END\n";

#endif
