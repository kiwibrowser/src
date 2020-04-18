/**************************************************************************
 *
 * Copyright 2009 Younes Manton.
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

#include <assert.h>
#include <error.h>
#include "testlib.h"

int main(int argc, char **argv)
{
	const unsigned int	width = 16, height = 16;
	const unsigned int	mc_types[2] = {XVMC_MOCOMP | XVMC_MPEG_2, XVMC_IDCT | XVMC_MPEG_2};

	Display			*display;
	XvPortID		port_num;
	int			surface_type_id;
	unsigned int		is_overlay, intra_unsigned;
	int			colorkey;
	XvMCContext		context;
	XvMCSurface		surface = {0};

	display = XOpenDisplay(NULL);

	if (!GetPort
	(
		display,
		width,
		height,
		XVMC_CHROMA_FORMAT_420,
		mc_types,
		2,
		&port_num,
		&surface_type_id,
		&is_overlay,
		&intra_unsigned
	))
	{
		XCloseDisplay(display);
		error(1, 0, "Error, unable to find a good port.\n");
	}

	if (is_overlay)
	{
		Atom xv_colorkey = XInternAtom(display, "XV_COLORKEY", 0);
		XvGetPortAttribute(display, port_num, xv_colorkey, &colorkey);
	}

	assert(XvMCCreateContext(display, port_num, surface_type_id, width, height, XVMC_DIRECT, &context) == Success);

	/* Test NULL context */
	assert(XvMCCreateSurface(display, NULL, &surface) == XvMCBadContext);
	/* Test NULL surface */
	assert(XvMCCreateSurface(display, &context, NULL) == XvMCBadSurface);
	/* Test valid params */
	assert(XvMCCreateSurface(display, &context, &surface) == Success);
	/* Test surface id assigned */
	assert(surface.surface_id != 0);
	/* Test context id assigned and correct */
	assert(surface.context_id == context.context_id);
	/* Test surface type id assigned and correct */
	assert(surface.surface_type_id == surface_type_id);
	/* Test width & height assigned and correct */
	assert(surface.width == width && surface.height == height);
	/* Test valid params */
	assert(XvMCDestroySurface(display, &surface) == Success);
	/* Test NULL surface */
	assert(XvMCDestroySurface(display, NULL) == XvMCBadSurface);

	assert(XvMCDestroyContext(display, &context) == Success);

	XvUngrabPort(display, port_num, CurrentTime);
	XCloseDisplay(display);

	return 0;
}
