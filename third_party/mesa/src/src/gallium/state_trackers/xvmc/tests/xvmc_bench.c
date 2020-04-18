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
#include <stdio.h>
#include <string.h>
#include <error.h>
#include <sys/time.h>
#include "testlib.h"

#define MACROBLOCK_WIDTH		16
#define MACROBLOCK_HEIGHT		16
#define BLOCKS_PER_MACROBLOCK		6

#define DEFAULT_INPUT_WIDTH		720
#define DEFAULT_INPUT_HEIGHT		480
#define DEFAULT_REPS			100

#define PIPELINE_STEP_MC		1
#define PIPELINE_STEP_CSC		2
#define PIPELINE_STEP_SWAP		4

#define MB_TYPE_I			1
#define MB_TYPE_P			2
#define MB_TYPE_B			4

struct Config
{
	unsigned int input_width;
	unsigned int input_height;
	unsigned int output_width;
	unsigned int output_height;
	unsigned int pipeline;
	unsigned int mb_types;
	unsigned int reps;
};

void ParseArgs(int argc, char **argv, struct Config *config);

void ParseArgs(int argc, char **argv, struct Config *config)
{
	int fail = 0;
	int i;

	config->input_width = DEFAULT_INPUT_WIDTH;
	config->input_height = DEFAULT_INPUT_HEIGHT;
	config->output_width = 0;
	config->output_height = 0;
	config->pipeline = 0;
	config->mb_types = 0;
	config->reps = DEFAULT_REPS;

	for (i = 1; i < argc && !fail; ++i)
	{
		if (!strcmp(argv[i], "-iw"))
		{
			if (sscanf(argv[++i], "%u", &config->input_width) != 1)
				fail = 1;
		}
		else if (!strcmp(argv[i], "-ih"))
		{
			if (sscanf(argv[++i], "%u", &config->input_height) != 1)
				fail = 1;
		}
		else if (!strcmp(argv[i], "-ow"))
		{
			if (sscanf(argv[++i], "%u", &config->output_width) != 1)
				fail = 1;
		}
		else if (!strcmp(argv[i], "-oh"))
		{
			if (sscanf(argv[++i], "%u", &config->output_height) != 1)
				fail = 1;
		}
		else if (!strcmp(argv[i], "-p"))
		{
			char *token = strtok(argv[++i], ",");

			while (token && !fail)
			{
				if (!strcmp(token, "mc"))
					config->pipeline |= PIPELINE_STEP_MC;
				else if (!strcmp(token, "csc"))
					config->pipeline |= PIPELINE_STEP_CSC;
				else if (!strcmp(token, "swp"))
					config->pipeline |= PIPELINE_STEP_SWAP;
				else
					fail = 1;

				if (!fail)
					token = strtok(NULL, ",");
			}
		}
		else if (!strcmp(argv[i], "-mb"))
		{
			char *token = strtok(argv[++i], ",");

			while (token && !fail)
			{
				if (strcmp(token, "i"))
					config->mb_types |= MB_TYPE_I;
				else if (strcmp(token, "p"))
					config->mb_types |= MB_TYPE_P;
				else if (strcmp(token, "b"))
					config->mb_types |= MB_TYPE_B;
				else
					fail = 1;

				if (!fail)
					token = strtok(NULL, ",");
			}
		}
		else if (!strcmp(argv[i], "-r"))
		{
			if (sscanf(argv[++i], "%u", &config->reps) != 1)
				fail = 1;
		}
		else
			fail = 1;
	}

	if (fail)
		error
		(
			1, 0,
			"Bad argument.\n"
			"\n"
			"Usage: %s [options]\n"
			"\t-iw <width>\tInput width\n"
			"\t-ih <height>\tInput height\n"
			"\t-ow <width>\tOutput width\n"
			"\t-oh <height>\tOutput height\n"
			"\t-p <pipeline>\tPipeline to test\n"
			"\t-mb <mb type>\tMacroBlock types to use\n"
			"\t-r <reps>\tRepetitions\n\n"
			"\tPipeline steps: mc,csc,swap\n"
			"\tMB types: i,p,b\n",
			argv[0]
		);

	if (config->output_width == 0)
		config->output_width = config->input_width;
	if (config->output_height == 0)
		config->output_height = config->input_height;
	if (!config->pipeline)
		config->pipeline = PIPELINE_STEP_MC | PIPELINE_STEP_CSC | PIPELINE_STEP_SWAP;
	if (!config->mb_types)
		config->mb_types = MB_TYPE_I | MB_TYPE_P | MB_TYPE_B;
}

int main(int argc, char **argv)
{
	struct Config		config;
	Display			*display;
	Window			root, window;
	const unsigned int	mc_types[2] = {XVMC_MOCOMP | XVMC_MPEG_2, XVMC_IDCT | XVMC_MPEG_2};
	XvPortID		port_num;
	int			surface_type_id;
	unsigned int		is_overlay, intra_unsigned;
	int			colorkey;
	XvMCContext		context;
	XvMCSurface		surface;
	XvMCBlockArray		block_array;
	XvMCMacroBlockArray	mb_array;
	unsigned int		mbw, mbh;
	unsigned int		mbx, mby;
	unsigned int		reps;
	struct timeval		start, stop, diff;
	double			diff_secs;

	ParseArgs(argc, argv, &config);

	mbw = align(config.input_width, MACROBLOCK_WIDTH) / MACROBLOCK_WIDTH;
	mbh = align(config.input_height, MACROBLOCK_HEIGHT) / MACROBLOCK_HEIGHT;

	display = XOpenDisplay(NULL);

	if (!GetPort
	(
		display,
		config.input_width,
		config.input_height,
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

	root = XDefaultRootWindow(display);
	window = XCreateSimpleWindow(display, root, 0, 0, config.output_width, config.output_height, 0, 0, colorkey);

	assert(XvMCCreateContext(display, port_num, surface_type_id, config.input_width, config.input_height, XVMC_DIRECT, &context) == Success);
	assert(XvMCCreateSurface(display, &context, &surface) == Success);
	assert(XvMCCreateBlocks(display, &context, mbw * mbh * BLOCKS_PER_MACROBLOCK, &block_array) == Success);
	assert(XvMCCreateMacroBlocks(display, &context, mbw * mbh, &mb_array) == Success);

	for (mby = 0; mby < mbh; ++mby)
		for (mbx = 0; mbx < mbw; ++mbx)
		{
			mb_array.macro_blocks[mby * mbw + mbx].x = mbx;
			mb_array.macro_blocks[mby * mbw + mbx].y = mby;
			mb_array.macro_blocks[mby * mbw + mbx].macroblock_type = XVMC_MB_TYPE_INTRA;
			/*mb->motion_type = ;*/
			/*mb->motion_vertical_field_select = ;*/
			mb_array.macro_blocks[mby * mbw + mbx].dct_type = XVMC_DCT_TYPE_FRAME;
			/*mb->PMV[0][0][0] = ;
			mb->PMV[0][0][1] = ;
			mb->PMV[0][1][0] = ;
			mb->PMV[0][1][1] = ;
			mb->PMV[1][0][0] = ;
			mb->PMV[1][0][1] = ;
			mb->PMV[1][1][0] = ;
			mb->PMV[1][1][1] = ;*/
			mb_array.macro_blocks[mby * mbw + mbx].index = (mby * mbw + mbx) * BLOCKS_PER_MACROBLOCK;
			mb_array.macro_blocks[mby * mbw + mbx].coded_block_pattern = 0x3F;
		}

	XSelectInput(display, window, ExposureMask | KeyPressMask);
	XMapWindow(display, window);
	XSync(display, 0);

	gettimeofday(&start, NULL);

	for (reps = 0; reps < config.reps; ++reps)
	{
		if (config.pipeline & PIPELINE_STEP_MC)
		{
			assert(XvMCRenderSurface(display, &context, XVMC_FRAME_PICTURE, &surface, NULL, NULL, 0, mbw * mbh, 0, &mb_array, &block_array) == Success);
			assert(XvMCFlushSurface(display, &surface) == Success);
		}
		if (config.pipeline & PIPELINE_STEP_CSC)
			assert(XvMCPutSurface(display, &surface, window, 0, 0, config.input_width, config.input_height, 0, 0, config.output_width, config.output_height, XVMC_FRAME_PICTURE) == Success);
	}

	gettimeofday(&stop, NULL);

	timeval_subtract(&diff, &stop, &start);
	diff_secs = (double)diff.tv_sec + (double)diff.tv_usec / 1000000.0;

	printf("XvMC Benchmark\n");
	printf("Input: %u,%u\nOutput: %u,%u\n", config.input_width, config.input_height, config.output_width, config.output_height);
	printf("Pipeline: ");
	if (config.pipeline & PIPELINE_STEP_MC)
		printf("|mc|");
	if (config.pipeline & PIPELINE_STEP_CSC)
		printf("|csc|");
	if (config.pipeline & PIPELINE_STEP_SWAP)
		printf("|swap|");
	printf("\n");
	printf("Reps: %u\n", config.reps);
	printf("Total time: %.2lf (%.2lf reps / sec)\n", diff_secs, config.reps / diff_secs);

	assert(XvMCDestroyBlocks(display, &block_array) == Success);
	assert(XvMCDestroyMacroBlocks(display, &mb_array) == Success);
	assert(XvMCDestroySurface(display, &surface) == Success);
	assert(XvMCDestroyContext(display, &context) == Success);

	XvUngrabPort(display, port_num, CurrentTime);
	XDestroyWindow(display, window);
	XCloseDisplay(display);

	return 0;
}
