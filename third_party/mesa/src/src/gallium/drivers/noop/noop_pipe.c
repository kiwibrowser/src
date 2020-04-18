/*
 * Copyright 2010 Red Hat Inc.
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
 */
#include <stdio.h>
#include <errno.h>
#include "pipe/p_defines.h"
#include "pipe/p_state.h"
#include "pipe/p_context.h"
#include "pipe/p_screen.h"
#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "noop_public.h"

DEBUG_GET_ONCE_BOOL_OPTION(noop, "GALLIUM_NOOP", FALSE)

void noop_init_state_functions(struct pipe_context *ctx);

struct noop_pipe_screen {
	struct pipe_screen	pscreen;
	struct pipe_screen	*oscreen;
};

/*
 * query
 */
struct noop_query {
	unsigned	query;
};
static struct pipe_query *noop_create_query(struct pipe_context *ctx, unsigned query_type)
{
	struct noop_query *query = CALLOC_STRUCT(noop_query);

	return (struct pipe_query *)query;
}

static void noop_destroy_query(struct pipe_context *ctx, struct pipe_query *query)
{
	FREE(query);
}

static void noop_begin_query(struct pipe_context *ctx, struct pipe_query *query)
{
}

static void noop_end_query(struct pipe_context *ctx, struct pipe_query *query)
{
}

static boolean noop_get_query_result(struct pipe_context *ctx,
					struct pipe_query *query,
					boolean wait,
					union pipe_query_result *vresult)
{
	uint64_t *result = (uint64_t*)vresult;

	*result = 0;
	return TRUE;
}


/*
 * resource
 */
struct noop_resource {
	struct pipe_resource	base;
	unsigned		size;
	char			*data;
	struct sw_displaytarget	*dt;
};

static struct pipe_resource *noop_resource_create(struct pipe_screen *screen,
						const struct pipe_resource *templ)
{
	struct noop_resource *nresource;
	unsigned stride;

	nresource = CALLOC_STRUCT(noop_resource);
	if (nresource == NULL)
		return NULL;

	stride = util_format_get_stride(templ->format, templ->width0);
	nresource->base = *templ;
	nresource->base.screen = screen;
	nresource->size = stride * templ->height0 * templ->depth0;
	nresource->data = MALLOC(nresource->size);
	pipe_reference_init(&nresource->base.reference, 1);
	if (nresource->data == NULL) {
		FREE(nresource);
		return NULL;
	}
	return &nresource->base;
}

static struct pipe_resource *noop_resource_from_handle(struct pipe_screen *screen,
							const struct pipe_resource *templ,
							struct winsys_handle *handle)
{
	struct noop_pipe_screen *noop_screen = (struct noop_pipe_screen*)screen;
	struct pipe_screen *oscreen = noop_screen->oscreen;
	struct pipe_resource *result;
	struct pipe_resource *noop_resource;

	result = oscreen->resource_from_handle(oscreen, templ, handle);
	noop_resource = noop_resource_create(screen, result);
	pipe_resource_reference(&result, NULL);
	return noop_resource;
}

static boolean noop_resource_get_handle(struct pipe_screen *screen,
					struct pipe_resource *resource,
					struct winsys_handle *handle)
{
	return FALSE;
}

static void noop_resource_destroy(struct pipe_screen *screen,
					struct pipe_resource *resource)
{
	struct noop_resource *nresource = (struct noop_resource *)resource;

	FREE(nresource->data);
	FREE(resource);
}


/*
 * transfer
 */
static struct pipe_transfer *noop_get_transfer(struct pipe_context *context,
						struct pipe_resource *resource,
						unsigned level,
						enum pipe_transfer_usage usage,
						const struct pipe_box *box)
{
	struct pipe_transfer *transfer;

	transfer = CALLOC_STRUCT(pipe_transfer);
	if (transfer == NULL)
		return NULL;
	pipe_resource_reference(&transfer->resource, resource);
	transfer->level = level;
	transfer->usage = usage;
	transfer->box = *box;
	transfer->stride = 1;
	transfer->layer_stride = 1;
	return transfer;
}

static void *noop_transfer_map(struct pipe_context *pipe,
				struct pipe_transfer *transfer)
{
	struct noop_resource *nresource = (struct noop_resource *)transfer->resource;

	return nresource->data;
}

static void noop_transfer_flush_region(struct pipe_context *pipe,
					struct pipe_transfer *transfer,
					const struct pipe_box *box)
{
}

static void noop_transfer_unmap(struct pipe_context *pipe,
				struct pipe_transfer *transfer)
{
}

static void noop_transfer_destroy(struct pipe_context *pipe,
					struct pipe_transfer *transfer)
{
	pipe_resource_reference(&transfer->resource, NULL);
	FREE(transfer);
}

static void noop_transfer_inline_write(struct pipe_context *pipe,
					struct pipe_resource *resource,
					unsigned level,
					unsigned usage,
					const struct pipe_box *box,
					const void *data,
					unsigned stride,
					unsigned layer_stride)
{
}


/*
 * clear/copy
 */
static void noop_clear(struct pipe_context *ctx, unsigned buffers,
		       const union pipe_color_union *color, double depth, unsigned stencil)
{
}

static void noop_clear_render_target(struct pipe_context *ctx,
				     struct pipe_surface *dst,
				     const union pipe_color_union *color,
				     unsigned dstx, unsigned dsty,
				     unsigned width, unsigned height)
{
}

static void noop_clear_depth_stencil(struct pipe_context *ctx,
				     struct pipe_surface *dst,
				     unsigned clear_flags,
				     double depth,
				     unsigned stencil,
				     unsigned dstx, unsigned dsty,
				     unsigned width, unsigned height)
{
}

static void noop_resource_copy_region(struct pipe_context *ctx,
				      struct pipe_resource *dst,
				      unsigned dst_level,
				      unsigned dstx, unsigned dsty, unsigned dstz,
				      struct pipe_resource *src,
				      unsigned src_level,
				      const struct pipe_box *src_box)
{
}


/*
 * context
 */
static void noop_flush(struct pipe_context *ctx,
			struct pipe_fence_handle **fence)
{
}

static void noop_destroy_context(struct pipe_context *ctx)
{
	FREE(ctx);
}

static struct pipe_context *noop_create_context(struct pipe_screen *screen, void *priv)
{
	struct pipe_context *ctx = CALLOC_STRUCT(pipe_context);

	if (ctx == NULL)
		return NULL;
	ctx->screen = screen;
	ctx->priv = priv;
	ctx->destroy = noop_destroy_context;
	ctx->flush = noop_flush;
	ctx->clear = noop_clear;
	ctx->clear_render_target = noop_clear_render_target;
	ctx->clear_depth_stencil = noop_clear_depth_stencil;
	ctx->resource_copy_region = noop_resource_copy_region;
	ctx->create_query = noop_create_query;
	ctx->destroy_query = noop_destroy_query;
	ctx->begin_query = noop_begin_query;
	ctx->end_query = noop_end_query;
	ctx->get_query_result = noop_get_query_result;
	ctx->get_transfer = noop_get_transfer;
	ctx->transfer_map = noop_transfer_map;
	ctx->transfer_flush_region = noop_transfer_flush_region;
	ctx->transfer_unmap = noop_transfer_unmap;
	ctx->transfer_destroy = noop_transfer_destroy;
	ctx->transfer_inline_write = noop_transfer_inline_write;
	noop_init_state_functions(ctx);

	return ctx;
}


/*
 * pipe_screen
 */
static void noop_flush_frontbuffer(struct pipe_screen *_screen,
				   struct pipe_resource *resource,
				   unsigned level, unsigned layer,
				   void *context_private)
{
}

static const char *noop_get_vendor(struct pipe_screen* pscreen)
{
	return "X.Org";
}

static const char *noop_get_name(struct pipe_screen* pscreen)
{
	return "NOOP";
}

static int noop_get_param(struct pipe_screen* pscreen, enum pipe_cap param)
{
	struct pipe_screen *screen = ((struct noop_pipe_screen*)pscreen)->oscreen;

	return screen->get_param(screen, param);
}

static float noop_get_paramf(struct pipe_screen* pscreen,
			     enum pipe_capf param)
{
	struct pipe_screen *screen = ((struct noop_pipe_screen*)pscreen)->oscreen;

	return screen->get_paramf(screen, param);
}

static int noop_get_shader_param(struct pipe_screen* pscreen, unsigned shader, enum pipe_shader_cap param)
{
	struct pipe_screen *screen = ((struct noop_pipe_screen*)pscreen)->oscreen;

	return screen->get_shader_param(screen, shader, param);
}

static boolean noop_is_format_supported(struct pipe_screen* pscreen,
					enum pipe_format format,
					enum pipe_texture_target target,
					unsigned sample_count,
                                        unsigned usage)
{
	struct pipe_screen *screen = ((struct noop_pipe_screen*)pscreen)->oscreen;

	return screen->is_format_supported(screen, format, target, sample_count, usage);
}

static uint64_t noop_get_timestamp(struct pipe_screen *pscreen)
{
	return 0;
}

static void noop_destroy_screen(struct pipe_screen *screen)
{
	struct noop_pipe_screen *noop_screen = (struct noop_pipe_screen*)screen;
	struct pipe_screen *oscreen = noop_screen->oscreen;

	oscreen->destroy(oscreen);
	FREE(screen);
}

struct pipe_screen *noop_screen_create(struct pipe_screen *oscreen)
{
	struct noop_pipe_screen *noop_screen;
	struct pipe_screen *screen;

	if (!debug_get_option_noop()) {
		return oscreen;
	}

	noop_screen = CALLOC_STRUCT(noop_pipe_screen);
	if (noop_screen == NULL) {
		return NULL;
	}
	noop_screen->oscreen = oscreen;
	screen = &noop_screen->pscreen;

	screen->destroy = noop_destroy_screen;
	screen->get_name = noop_get_name;
	screen->get_vendor = noop_get_vendor;
	screen->get_param = noop_get_param;
	screen->get_shader_param = noop_get_shader_param;
	screen->get_paramf = noop_get_paramf;
	screen->is_format_supported = noop_is_format_supported;
	screen->context_create = noop_create_context;
	screen->resource_create = noop_resource_create;
	screen->resource_from_handle = noop_resource_from_handle;
	screen->resource_get_handle = noop_resource_get_handle;
	screen->resource_destroy = noop_resource_destroy;
	screen->flush_frontbuffer = noop_flush_frontbuffer;
        screen->get_timestamp = noop_get_timestamp;

	return screen;
}
