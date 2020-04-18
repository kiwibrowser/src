/**************************************************************************
 *
 * Copyright 2010-2012 VMware, Inc.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 **************************************************************************/


#ifndef LP_BLD_LIMITS_H_
#define LP_BLD_LIMITS_H_


#include <limits.h>

#include "pipe/p_state.h"
#include "pipe/p_defines.h"


/*
 * TGSI translation limits.
 *
 * Some are slightly above SM 3.0 requirements to give some wiggle room to
 * the state trackers.
 */

#define LP_MAX_TGSI_TEMPS 256

#define LP_MAX_TGSI_ADDRS 16

#define LP_MAX_TGSI_IMMEDIATES 256

#define LP_MAX_TGSI_PREDS 16

/**
 * Maximum control flow nesting
 *
 * SM3.0 requires 24
 */
#define LP_MAX_TGSI_NESTING 32

/**
 * Maximum iterations before loop termination
 * Shared between every loop in a TGSI shader
 */
#define LP_MAX_TGSI_LOOP_ITERATIONS 65535


/**
 * Some of these limits are actually infinite (i.e., only limited by available
 * memory), however advertising INT_MAX would cause some test problems to
 * actually try to allocate the maximum and run out of memory and crash.  So
 * stick with something reasonable here.
 */
static INLINE int
gallivm_get_shader_param(enum pipe_shader_cap param)
{
   switch(param) {
   case PIPE_SHADER_CAP_MAX_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS:
      return 1 * 1024 * 1024;
   case PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH:
      return LP_MAX_TGSI_NESTING;
   case PIPE_SHADER_CAP_MAX_INPUTS:
      return PIPE_MAX_SHADER_INPUTS;
   case PIPE_SHADER_CAP_MAX_CONSTS:
      return 16 * 2024;
   case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
      return PIPE_MAX_CONSTANT_BUFFERS;
   case PIPE_SHADER_CAP_MAX_TEMPS:
      return LP_MAX_TGSI_TEMPS;
   case PIPE_SHADER_CAP_MAX_ADDRS:
      return LP_MAX_TGSI_ADDRS;
   case PIPE_SHADER_CAP_MAX_PREDS:
      return LP_MAX_TGSI_PREDS;
   case PIPE_SHADER_CAP_TGSI_CONT_SUPPORTED:
      return 1;
   case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
   case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
   case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
   case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
      return 1;
   case PIPE_SHADER_CAP_SUBROUTINES:
      return 1;
   case PIPE_SHADER_CAP_INTEGERS:
      return 1;
   case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS:
      return PIPE_MAX_SAMPLERS;
   default:
      return 0;
   }
}


#endif /* LP_BLD_LIMITS_H_ */
