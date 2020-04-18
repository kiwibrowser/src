/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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


#ifndef LP_DEBUG_H
#define LP_DEBUG_H

#include "pipe/p_compiler.h"
#include "util/u_debug.h"

extern void
st_print_current(void);


#define DEBUG_PIPE      0x1
#define DEBUG_TGSI      0x2
#define DEBUG_TEX       0x4
#define DEBUG_SETUP     0x10
#define DEBUG_RAST      0x20
#define DEBUG_QUERY     0x40
#define DEBUG_SCREEN    0x80
#define DEBUG_SHOW_TILES    0x200
#define DEBUG_SHOW_SUBTILES 0x400
#define DEBUG_COUNTERS      0x800
#define DEBUG_SCENE         0x1000
#define DEBUG_FENCE         0x2000
#define DEBUG_MEM           0x4000
#define DEBUG_FS            0x8000

/* Performance flags.  These are active even on release builds.
 */
#define PERF_TEX_MEM        0x1  	/* minimize texture cache footprint */
#define PERF_NO_MIP_LINEAR  0x2  	/* MIP_FILTER_LINEAR ==> _NEAREST */
#define PERF_NO_MIPMAPS     0x4  	/* MIP_FILTER_NONE always */
#define PERF_NO_LINEAR      0x8  	/* FILTER_NEAREST always */
#define PERF_NO_TEX         0x10  	/* sample white always */
#define PERF_NO_BLEND       0x20  	/* disable blending */
#define PERF_NO_DEPTH       0x40  	/* disable depth buffering entirely */
#define PERF_NO_ALPHATEST   0x80  	/* disable alpha testing */


extern int LP_PERF;

#ifdef DEBUG
extern int LP_DEBUG;
#else
#define LP_DEBUG 0
#endif

void st_debug_init( void );

static INLINE void
LP_DBG( unsigned flag, const char *fmt, ... )
{
    if (LP_DEBUG & flag)
    {
        va_list args;

        va_start( args, fmt );
        debug_vprintf( fmt, args );
        va_end( args );
    }
}


#endif /* LP_DEBUG_H */
