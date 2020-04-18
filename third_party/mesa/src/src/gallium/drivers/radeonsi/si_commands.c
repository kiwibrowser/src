/*
 * Copyright 2012 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *      Christian König <christian.koenig@amd.com>
 */

#include "radeonsi_pipe.h"
#include "radeonsi_pm4.h"
#include "sid.h"

void si_cmd_surface_sync(struct si_pm4_state *pm4, uint32_t cp_coher_cntl)
{
	si_pm4_cmd_begin(pm4, PKT3_SURFACE_SYNC);
	si_pm4_cmd_add(pm4, cp_coher_cntl);	/* CP_COHER_CNTL */
	si_pm4_cmd_add(pm4, 0xffffffff);	/* CP_COHER_SIZE */
	si_pm4_cmd_add(pm4, 0);			/* CP_COHER_BASE */
	si_pm4_cmd_add(pm4, 0x0000000A);	/* POLL_INTERVAL */
	si_pm4_cmd_end(pm4, false);
}
