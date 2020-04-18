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

#include "testlib.h"
#include <stdio.h>

/*
void test(int pred, const char *pred_string, const char *doc_string, const char *file, unsigned int line)
{
	fputs(doc_string, stderr);
	if (!pred)
		fprintf(stderr, " FAIL!\n\t\"%s\" at %s:%u\n", pred_string, file, line);
	else
		fputs(" PASS!\n", stderr);
}
*/

int GetPort
(
	Display *display,
	unsigned int width,
	unsigned int height,
	unsigned int chroma_format,
	const unsigned int *mc_types,
	unsigned int num_mc_types,
	XvPortID *port_id,
	int *surface_type_id,
	unsigned int *is_overlay,
	unsigned int *intra_unsigned
)
{
	unsigned int	found_port = 0;
	XvAdaptorInfo	*adaptor_info;
	unsigned int	num_adaptors;
	int		num_types;
	int		ev_base, err_base;
	unsigned int	i, j, k, l;

	if (!XvMCQueryExtension(display, &ev_base, &err_base))
		return 0;
	if (XvQueryAdaptors(display, XDefaultRootWindow(display), &num_adaptors, &adaptor_info) != Success)
		return 0;

	for (i = 0; i < num_adaptors && !found_port; ++i)
	{
		if (adaptor_info[i].type & XvImageMask)
		{
			XvMCSurfaceInfo *surface_info = XvMCListSurfaceTypes(display, adaptor_info[i].base_id, &num_types);

			if (surface_info)
			{
				for (j = 0; j < num_types && !found_port; ++j)
				{
					if
					(
						surface_info[j].chroma_format == chroma_format &&
						surface_info[j].max_width >= width &&
						surface_info[j].max_height >= height
					)
					{
						for (k = 0; k < num_mc_types && !found_port; ++k)
						{
							if (surface_info[j].mc_type == mc_types[k])
							{
								for (l = 0; l < adaptor_info[i].num_ports && !found_port; ++l)
								{
									if (XvGrabPort(display, adaptor_info[i].base_id + l, CurrentTime) == Success)
									{
										*port_id = adaptor_info[i].base_id + l;
										*surface_type_id = surface_info[j].surface_type_id;
										*is_overlay = surface_info[j].flags & XVMC_OVERLAID_SURFACE;
										*intra_unsigned = surface_info[j].flags & XVMC_INTRA_UNSIGNED;
										found_port = 1;
									}
								}
							}
						}
					}
				}

				XFree(surface_info);
			}
		}
	}

	XvFreeAdaptorInfo(adaptor_info);

	return found_port;
}

unsigned int align(unsigned int value, unsigned int alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}

/* From the glibc manual */
int timeval_subtract(struct timeval *result, struct timeval *x, struct timeval *y)
{
	/* Perform the carry for the later subtraction by updating y. */
	if (x->tv_usec < y->tv_usec)
	{
		int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
		y->tv_usec -= 1000000 * nsec;
		y->tv_sec += nsec;
	}
	if (x->tv_usec - y->tv_usec > 1000000)
	{
		int nsec = (x->tv_usec - y->tv_usec) / 1000000;
		y->tv_usec += 1000000 * nsec;
		y->tv_sec -= nsec;
	}

	/*
	 * Compute the time remaining to wait.
	 * tv_usec is certainly positive.
	 */
	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_usec = x->tv_usec - y->tv_usec;

	/* Return 1 if result is negative. */
	return x->tv_sec < y->tv_sec;
}
