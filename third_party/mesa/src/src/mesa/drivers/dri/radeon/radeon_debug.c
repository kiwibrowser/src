/*
 * Copyright © 2009 Pauli Nieminen
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
 */
/*
 * Authors:
 *      Pauli Nieminen <suokkos@gmail.com>
 */

#include "utils.h"

#include "radeon_debug.h"
#include "radeon_common_context.h"

#include <stdarg.h>
#include <stdio.h>

static const struct dri_debug_control debug_control[] = {
	{"fall", RADEON_FALLBACKS},
	{"tex", RADEON_TEXTURE},
	{"ioctl", RADEON_IOCTL},
	{"verts", RADEON_VERTS},
	{"render", RADEON_RENDER},
	{"swrender", RADEON_SWRENDER},
	{"state", RADEON_STATE},
	{"shader", RADEON_SHADER},
	{"vfmt", RADEON_VFMT},
	{"vtxf", RADEON_VFMT},
	{"dri", RADEON_DRI},
	{"dma", RADEON_DMA},
	{"sanity", RADEON_SANITY},
	{"sync", RADEON_SYNC},
	{"pixel", RADEON_PIXEL},
	{"mem", RADEON_MEMORY},
	{"cs", RADEON_CS},
	{"allmsg", ~RADEON_SYNC}, /* avoid the term "sync" because the parser uses strstr */
	{NULL, 0}
};

radeon_debug_type_t radeon_enabled_debug_types;

void radeon_init_debug(void)
{
	radeon_enabled_debug_types = driParseDebugString(getenv("RADEON_DEBUG"), debug_control);

	radeon_enabled_debug_types |= RADEON_GENERAL;
}

void _radeon_debug_add_indent(void)
{
	GET_CURRENT_CONTEXT(ctx);
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);
	const size_t length = sizeof(radeon->debug.indent)
		/ sizeof(radeon->debug.indent[0]);
	if (radeon->debug.indent_depth < length - 1) {
		radeon->debug.indent[radeon->debug.indent_depth] = '\t';
		++radeon->debug.indent_depth;
	};
}

void _radeon_debug_remove_indent(void)
{
	GET_CURRENT_CONTEXT(ctx);
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);
	if (radeon->debug.indent_depth > 0) {
		radeon->debug.indent[radeon->debug.indent_depth] = '\0';
		--radeon->debug.indent_depth;
	}
}

void _radeon_print(const radeon_debug_type_t type,
	   const radeon_debug_level_t level,
	   const char* message,
	   ...)
{
	va_list values;

	GET_CURRENT_CONTEXT(ctx);
	if (ctx) {
		radeonContextPtr radeon = RADEON_CONTEXT(ctx);
		// FIXME: Make this multi thread safe
		if (radeon->debug.indent_depth)
			fprintf(stderr, "%s", radeon->debug.indent);
	}
	va_start( values, message );
	vfprintf(stderr, message, values);
	va_end( values );
}
