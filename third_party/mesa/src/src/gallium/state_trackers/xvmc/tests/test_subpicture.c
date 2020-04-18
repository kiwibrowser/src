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
#include <stdio.h>
#include "testlib.h"

static void PrintGUID(const char *guid)
{
	int i;
	printf("\tguid: ");
	for (i = 0; i < 4; ++i)
		printf("%C,", guid[i] == 0 ? '0' : guid[i]);
	for (; i < 15; ++i)
		printf("%x,", (unsigned char)guid[i]);
	printf("%x\n", (unsigned int)guid[15]);
}

static void PrintComponentOrder(const char *co)
{
	int i;
	printf("\tcomponent_order:\n\t   ");
	for (i = 0; i < 4; ++i)
		printf("%C,", co[i] == 0 ? '0' : co[i]);
	for (; i < 31; ++i)
		printf("%x,", (unsigned int)co[i]);
	printf("%x\n", (unsigned int)co[31]);
}

int main(int argc, char **argv)
{
	const unsigned int	width = 16, height = 16;
	const unsigned int	mc_types[2] = {XVMC_MOCOMP | XVMC_MPEG_2, XVMC_IDCT | XVMC_MPEG_2};
	const unsigned int	subpic_width = 16, subpic_height = 16;

	Display			*display;
	XvPortID		port_num;
	int			surface_type_id;
	unsigned int		is_overlay, intra_unsigned;
	int			colorkey;
	XvMCContext		context;
	XvImageFormatValues	*subpics;
	int			num_subpics;
	XvMCSubpicture		subpicture = {0};
	int			i;

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

	subpics = XvMCListSubpictureTypes(display, port_num, surface_type_id, &num_subpics);
	assert((subpics && num_subpics) > 0 || (!subpics && num_subpics == 0));

	for (i = 0; i < num_subpics; ++i)
	{
		printf("Subpicture %d:\n", i);
		printf("\tid: 0x%08x\n", subpics[i].id);
		printf("\ttype: %s\n", subpics[i].type == XvRGB ? "XvRGB" : (subpics[i].type == XvYUV ? "XvYUV" : "Unknown"));
		printf("\tbyte_order: %s\n", subpics[i].byte_order == LSBFirst ? "LSB First" : (subpics[i].byte_order == MSBFirst ? "MSB First" : "Unknown"));
		PrintGUID(subpics[i].guid);
		printf("\tbpp: %u\n", subpics[i].bits_per_pixel);
		printf("\tformat: %s\n", subpics[i].format == XvPacked ? "XvPacked" : (subpics[i].format == XvPlanar ? "XvPlanar" : "Unknown"));
		printf("\tnum_planes: %u\n", subpics[i].num_planes);

		if (subpics[i].type == XvRGB)
		{
			printf("\tdepth: %u\n", subpics[i].depth);
			printf("\tred_mask: 0x%08x\n", subpics[i].red_mask);
			printf("\tgreen_mask: 0x%08x\n", subpics[i].green_mask);
			printf("\tblue_mask: 0x%08x\n", subpics[i].blue_mask);
		}
		else if (subpics[i].type == XvYUV)
		{
			printf("\ty_sample_bits: %u\n", subpics[i].y_sample_bits);
			printf("\tu_sample_bits: %u\n", subpics[i].u_sample_bits);
			printf("\tv_sample_bits: %u\n", subpics[i].v_sample_bits);
			printf("\thorz_y_period: %u\n", subpics[i].horz_y_period);
			printf("\thorz_u_period: %u\n", subpics[i].horz_u_period);
			printf("\thorz_v_period: %u\n", subpics[i].horz_v_period);
			printf("\tvert_y_period: %u\n", subpics[i].vert_y_period);
			printf("\tvert_u_period: %u\n", subpics[i].vert_u_period);
			printf("\tvert_v_period: %u\n", subpics[i].vert_v_period);
		}
		PrintComponentOrder(subpics[i].component_order);
		printf("\tscanline_order: %s\n", subpics[i].scanline_order == XvTopToBottom ? "XvTopToBottom" : (subpics[i].scanline_order == XvBottomToTop ? "XvBottomToTop" : "Unknown"));
	}

	if (num_subpics == 0)
	{
		printf("Subpictures not supported, nothing to test.\n");
		return 0;
	}

	/* Test NULL context */
	assert(XvMCCreateSubpicture(display, NULL, &subpicture, subpic_width, subpic_height, subpics[0].id) == XvMCBadContext);
	/* Test NULL subpicture */
	assert(XvMCCreateSubpicture(display, &context, NULL, subpic_width, subpic_height, subpics[0].id) == XvMCBadSubpicture);
	/* Test invalid subpicture */
	assert(XvMCCreateSubpicture(display, &context, &subpicture, subpic_width, subpic_height, -1) == BadMatch);
	/* Test huge width */
	assert(XvMCCreateSubpicture(display, &context, &subpicture, 16384, subpic_height, subpics[0].id) == BadValue);
	/* Test huge height */
	assert(XvMCCreateSubpicture(display, &context, &subpicture, subpic_width, 16384, subpics[0].id) == BadValue);
	/* Test huge width & height */
	assert(XvMCCreateSubpicture(display, &context, &subpicture, 16384, 16384, subpics[0].id) == BadValue);
	for (i = 0; i < num_subpics; ++i)
	{
		/* Test valid params */
		assert(XvMCCreateSubpicture(display, &context, &subpicture, subpic_width, subpic_height, subpics[i].id) == Success);
		/* Test subpicture id assigned */
		assert(subpicture.subpicture_id != 0);
		/* Test context id assigned and correct */
		assert(subpicture.context_id == context.context_id);
		/* Test subpicture type id assigned and correct */
		assert(subpicture.xvimage_id == subpics[i].id);
		/* Test width & height assigned and correct */
		assert(subpicture.width == width && subpicture.height == height);
		/* Test no palette support */
		assert(subpicture.num_palette_entries == 0 && subpicture.entry_bytes == 0);
		/* Test valid params */
		assert(XvMCDestroySubpicture(display, &subpicture) == Success);
	}
	/* Test NULL surface */
	assert(XvMCDestroySubpicture(display, NULL) == XvMCBadSubpicture);

	assert(XvMCDestroyContext(display, &context) == Success);

	XFree(subpics);
	XvUngrabPort(display, port_num, CurrentTime);
	XCloseDisplay(display);

	return 0;
}
