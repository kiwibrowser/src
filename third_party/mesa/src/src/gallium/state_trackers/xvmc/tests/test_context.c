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
	XvMCContext		context = {0};

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

	/* Test NULL context */
	/* XXX: XvMCBadContext not a valid return for XvMCCreateContext in the XvMC API, but openChrome driver returns it */
	assert(XvMCCreateContext(display, port_num, surface_type_id, width, height, XVMC_DIRECT, NULL) == XvMCBadContext);
	/* Test invalid port */
	/* XXX: Success and XvBadPort have the same value, if this call actually gets passed the validation step as of now we'll crash later */
	assert(XvMCCreateContext(display, -1, surface_type_id, width, height, XVMC_DIRECT, &context) == XvBadPort);
	/* Test invalid surface */
	assert(XvMCCreateContext(display, port_num, -1, width, height, XVMC_DIRECT, &context) == BadMatch);
	/* Test invalid flags */
	assert(XvMCCreateContext(display, port_num, surface_type_id, width, height, -1, &context) == BadValue);
	/* Test huge width */
	assert(XvMCCreateContext(display, port_num, surface_type_id, 16384, height, XVMC_DIRECT, &context) == BadValue);
	/* Test huge height */
	assert(XvMCCreateContext(display, port_num, surface_type_id, width, 16384, XVMC_DIRECT, &context) == BadValue);
	/* Test huge width & height */
	assert(XvMCCreateContext(display, port_num, surface_type_id, 16384, 16384, XVMC_DIRECT, &context) == BadValue);
	/* Test valid params */
	assert(XvMCCreateContext(display, port_num, surface_type_id, width, height, XVMC_DIRECT, &context) == Success);
	/* Test context id assigned */
	assert(context.context_id != 0);
	/* Test surface type id assigned and correct */
	assert(context.surface_type_id == surface_type_id);
	/* Test width & height assigned and correct */
	assert(context.width == width && context.height == height);
	/* Test port assigned and correct */
	assert(context.port == port_num);
	/* Test flags assigned and correct */
	assert(context.flags == XVMC_DIRECT);
	/* Test NULL context */
	assert(XvMCDestroyContext(display, NULL) == XvMCBadContext);
	/* Test valid params */
	assert(XvMCDestroyContext(display, &context) == Success);
	/* Test awkward but valid width */
	assert(XvMCCreateContext(display, port_num, surface_type_id, width + 1, height, XVMC_DIRECT, &context) == Success);
	assert(context.width >= width + 1);
	assert(XvMCDestroyContext(display, &context) == Success);
	/* Test awkward but valid height */
	assert(XvMCCreateContext(display, port_num, surface_type_id, width, height + 1, XVMC_DIRECT, &context) == Success);
	assert(context.height >= height + 1);
	assert(XvMCDestroyContext(display, &context) == Success);
	/* Test awkward but valid width & height */
	assert(XvMCCreateContext(display, port_num, surface_type_id, width + 1, height + 1, XVMC_DIRECT, &context) == Success);
	assert(context.width >= width + 1 && context.height >= height + 1);
	assert(XvMCDestroyContext(display, &context) == Success);

	XvUngrabPort(display, port_num, CurrentTime);
	XCloseDisplay(display);

	return 0;
}
