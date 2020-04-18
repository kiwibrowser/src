/*
 * Copyright 2011 Adam Rak <adam.rak@streamnovation.com>
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
 *      Adam Rak <adam.rak@streamnovation.com>
 */

#include <stdio.h>
#include <errno.h>
#include "pipe/p_defines.h"
#include "pipe/p_state.h"
#include "pipe/p_context.h"
#include "util/u_blitter.h"
#include "util/u_double_list.h"
#include "util/u_transfer.h"
#include "util/u_surface.h"
#include "util/u_pack_color.h"
#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "util/u_framebuffer.h"
#include "pipebuffer/pb_buffer.h"
#include "r600.h"
#include "evergreend.h"
#include "r600_resource.h"
#include "r600_shader.h"
#include "r600_pipe.h"
#include "r600_formats.h"
#include "evergreen_compute.h"
#include "r600_hw_context_priv.h"
#include "evergreen_compute_internal.h"
#include "compute_memory_pool.h"
#ifdef HAVE_OPENCL
#include "llvm_wrapper.h"
#endif

/**
RAT0 is for global binding write
VTX1 is for global binding read

for wrting images RAT1...
for reading images TEX2...
  TEX2-RAT1 is paired

TEX2... consumes the same fetch resources, that VTX2... would consume

CONST0 and VTX0 is for parameters
  CONST0 is binding smaller input parameter buffer, and for constant indexing,
  also constant cached
  VTX0 is for indirect/non-constant indexing, or if the input is bigger than
  the constant cache can handle

RAT-s are limited to 12, so we can only bind at most 11 texture for writing
because we reserve RAT0 for global bindings. With byteaddressing enabled,
we should reserve another one too.=> 10 image binding for writing max.

from Nvidia OpenCL:
  CL_DEVICE_MAX_READ_IMAGE_ARGS:        128
  CL_DEVICE_MAX_WRITE_IMAGE_ARGS:       8 

so 10 for writing is enough. 176 is the max for reading according to the docs

writable images should be listed first < 10, so their id corresponds to RAT(id+1)
writable images will consume TEX slots, VTX slots too because of linear indexing

*/

static void evergreen_cs_set_vertex_buffer(
	struct r600_context * rctx,
	unsigned vb_index,
	unsigned offset,
	struct pipe_resource * buffer)
{
	struct r600_vertexbuf_state *state = &rctx->cs_vertex_buffer_state;
	struct pipe_vertex_buffer *vb = &state->vb[vb_index];
	vb->stride = 1;
	vb->buffer_offset = offset;
	vb->buffer = buffer;
	vb->user_buffer = NULL;

	r600_inval_vertex_cache(rctx);
	state->enabled_mask |= 1 << vb_index;
	state->dirty_mask |= 1 << vb_index;
	r600_atom_dirty(rctx, &state->atom);
}

const struct u_resource_vtbl r600_global_buffer_vtbl =
{
	u_default_resource_get_handle, /* get_handle */
	r600_compute_global_buffer_destroy, /* resource_destroy */
	r600_compute_global_get_transfer, /* get_transfer */
	r600_compute_global_transfer_destroy, /* transfer_destroy */
	r600_compute_global_transfer_map, /* transfer_map */
	r600_compute_global_transfer_flush_region,/* transfer_flush_region */
	r600_compute_global_transfer_unmap, /* transfer_unmap */
	r600_compute_global_transfer_inline_write /* transfer_inline_write */
};


void *evergreen_create_compute_state(
	struct pipe_context *ctx_,
	const const struct pipe_compute_state *cso)
{
	struct r600_context *ctx = (struct r600_context *)ctx_;
	struct r600_pipe_compute *shader = CALLOC_STRUCT(r600_pipe_compute);
	void *p;

#ifdef HAVE_OPENCL
	const struct pipe_llvm_program_header * header;
	const unsigned char * code;

	COMPUTE_DBG("*** evergreen_create_compute_state\n");

	header = cso->prog;
	code = cso->prog + sizeof(struct pipe_llvm_program_header);
#endif

	shader->ctx = (struct r600_context*)ctx;
	shader->resources = (struct evergreen_compute_resource*)
			CALLOC(sizeof(struct evergreen_compute_resource),
			get_compute_resource_num());
	shader->local_size = cso->req_local_mem; ///TODO: assert it
	shader->private_size = cso->req_private_mem;
	shader->input_size = cso->req_input_mem;

#ifdef HAVE_OPENCL 
	shader->mod = llvm_parse_bitcode(code, header->num_bytes);

	r600_compute_shader_create(ctx_, shader->mod, &shader->bc);
#endif
	shader->shader_code_bo = r600_compute_buffer_alloc_vram(ctx->screen,
							shader->bc.ndw * 4);

	p = ctx->ws->buffer_map(shader->shader_code_bo->cs_buf, ctx->cs,
							PIPE_TRANSFER_WRITE);

	memcpy(p, shader->bc.bytecode, shader->bc.ndw * 4);
	ctx->ws->buffer_unmap(shader->shader_code_bo->cs_buf);
	return shader;
}

void evergreen_delete_compute_state(struct pipe_context *ctx, void* state)
{
	struct r600_pipe_compute *shader = (struct r600_pipe_compute *)state;

	free(shader->resources);
	free(shader);
}

static void evergreen_bind_compute_state(struct pipe_context *ctx_, void *state)
{
	struct r600_context *ctx = (struct r600_context *)ctx_;

	COMPUTE_DBG("*** evergreen_bind_compute_state\n");

	ctx->cs_shader_state.shader = (struct r600_pipe_compute *)state;
}

/* The kernel parameters are stored a vtx buffer (ID=0), besides the explicit
 * kernel parameters there are inplicit parameters that need to be stored
 * in the vertex buffer as well.  Here is how these parameters are organized in
 * the buffer:
 *
 * DWORDS 0-2: Number of work groups in each dimension (x,y,z)
 * DWORDS 3-5: Number of global work items in each dimension (x,y,z)
 * DWORDS 6-8: Number of work items within each work group in each dimension
 *             (x,y,z)
 * DWORDS 9+ : Kernel parameters
 */
void evergreen_compute_upload_input(
	struct pipe_context *ctx_,
	const uint *block_layout,
	const uint *grid_layout,
	const void *input)
{
	struct r600_context *ctx = (struct r600_context *)ctx_;
	struct r600_pipe_compute *shader = ctx->cs_shader_state.shader;
	int i;
	unsigned kernel_parameters_offset_bytes = 36;
	uint32_t * num_work_groups_start;
	uint32_t * global_size_start;
	uint32_t * local_size_start;
	uint32_t * kernel_parameters_start;

	if (shader->input_size == 0) {
		return;
	}

	if (!shader->kernel_param) {
		unsigned buffer_size = shader->input_size;

		/* Add space for the grid dimensions */
		buffer_size += kernel_parameters_offset_bytes * sizeof(uint);
		shader->kernel_param = r600_compute_buffer_alloc_vram(
						ctx->screen, buffer_size);
	}

	num_work_groups_start = ctx->ws->buffer_map(
		shader->kernel_param->cs_buf, ctx->cs, PIPE_TRANSFER_WRITE);
	global_size_start = num_work_groups_start + (3 * (sizeof(uint) /4));
	local_size_start = global_size_start + (3 * (sizeof(uint)) / 4);
	kernel_parameters_start = local_size_start + (3 * (sizeof(uint)) / 4);

	/* Copy the work group size */
	memcpy(num_work_groups_start, grid_layout, 3 * sizeof(uint));

	/* Copy the global size */
	for (i = 0; i < 3; i++) {
		global_size_start[i] = grid_layout[i] * block_layout[i];
	}

	/* Copy the local dimensions */
	memcpy(local_size_start, block_layout, 3 * sizeof(uint));

	/* Copy the kernel inputs */
	memcpy(kernel_parameters_start, input, shader->input_size);

	for (i = 0; i < (kernel_parameters_offset_bytes / 4) +
					(shader->input_size / 4); i++) {
		COMPUTE_DBG("input %i : %i\n", i,
			((unsigned*)num_work_groups_start)[i]);
	}

	ctx->ws->buffer_unmap(shader->kernel_param->cs_buf);

	///ID=0 is reserved for the parameters
	evergreen_cs_set_vertex_buffer(ctx, 0, 0,
			(struct pipe_resource*)shader->kernel_param);
	///ID=0 is reserved for parameters
	evergreen_set_const_cache(shader, 0, shader->kernel_param,
						shader->input_size, 0);
}

static void evergreen_emit_direct_dispatch(
		struct r600_context *rctx,
		const uint *block_layout, const uint *grid_layout)
{
	int i;
	struct radeon_winsys_cs *cs = rctx->cs;
	unsigned num_waves;
	unsigned num_pipes = rctx->screen->info.r600_max_pipes;
	unsigned wave_divisor = (16 * num_pipes);
	int group_size = 1;
	int grid_size = 1;
	/* XXX: Enable lds and get size from cs_shader_state */
	unsigned lds_size = 0;

	/* Calculate group_size/grid_size */
	for (i = 0; i < 3; i++) {
		group_size *= block_layout[i];
	}

	for (i = 0; i < 3; i++)	{
		grid_size *= grid_layout[i];
	}

	/* num_waves = ceil((tg_size.x * tg_size.y, tg_size.z) / (16 * num_pipes)) */
	num_waves = (block_layout[0] * block_layout[1] * block_layout[2] +
			wave_divisor - 1) / wave_divisor;

	COMPUTE_DBG("Using %u pipes, there are %u wavefronts per thread block\n",
							num_pipes, num_waves);

	/* XXX: Partition the LDS between PS/CS.  By default half (4096 dwords
	 * on Evergreen) oes to Pixel Shaders and half goes to Compute Shaders.
	 * We may need to allocat the entire LDS space for Compute Shaders.
	 *
	 * EG: R_008E2C_SQ_LDS_RESOURCE_MGMT := S_008E2C_NUM_LS_LDS(lds_dwords)
	 * CM: CM_R_0286FC_SPI_LDS_MGMT :=  S_0286FC_NUM_LS_LDS(lds_dwords)
	 */

	r600_write_config_reg(cs, R_008970_VGT_NUM_INDICES, group_size);

	r600_write_config_reg_seq(cs, R_00899C_VGT_COMPUTE_START_X, 3);
	r600_write_value(cs, 0); /* R_00899C_VGT_COMPUTE_START_X */
	r600_write_value(cs, 0); /* R_0089A0_VGT_COMPUTE_START_Y */
	r600_write_value(cs, 0); /* R_0089A4_VGT_COMPUTE_START_Z */

	r600_write_config_reg(cs, R_0089AC_VGT_COMPUTE_THREAD_GROUP_SIZE,
								group_size);

	r600_write_compute_context_reg_seq(cs, R_0286EC_SPI_COMPUTE_NUM_THREAD_X, 3);
	r600_write_value(cs, block_layout[0]); /* R_0286EC_SPI_COMPUTE_NUM_THREAD_X */
	r600_write_value(cs, block_layout[1]); /* R_0286F0_SPI_COMPUTE_NUM_THREAD_Y */
	r600_write_value(cs, block_layout[2]); /* R_0286F4_SPI_COMPUTE_NUM_THREAD_Z */

	r600_write_compute_context_reg(cs, CM_R_0288E8_SQ_LDS_ALLOC,
					lds_size | (num_waves << 14));

	/* Dispatch packet */
	r600_write_value(cs, PKT3C(PKT3_DISPATCH_DIRECT, 3, 0));
	r600_write_value(cs, grid_layout[0]);
	r600_write_value(cs, grid_layout[1]);
	r600_write_value(cs, grid_layout[2]);
	/* VGT_DISPATCH_INITIATOR = COMPUTE_SHADER_EN */
	r600_write_value(cs, 1);
}

static void compute_emit_cs(struct r600_context *ctx, const uint *block_layout,
		const uint *grid_layout)
{
	struct radeon_winsys_cs *cs = ctx->cs;
	int i;

	struct r600_resource *onebo = NULL;
	struct r600_pipe_state *cb_state;
	struct evergreen_compute_resource *resources =
					ctx->cs_shader_state.shader->resources;

	/* Initialize all the compute-related registers.
	 *
	 * See evergreen_init_atom_start_compute_cs() in this file for the list
	 * of registers initialized by the start_compute_cs_cmd atom.
	 */
	r600_emit_atom(ctx, &ctx->start_compute_cs_cmd.atom);

	/* Emit cb_state */
        cb_state = ctx->states[R600_PIPE_STATE_FRAMEBUFFER];
	r600_context_pipe_state_emit(ctx, cb_state, RADEON_CP_PACKET3_COMPUTE_MODE);

	/* Set CB_TARGET_MASK  XXX: Use cb_misc_state */
	r600_write_compute_context_reg(cs, R_028238_CB_TARGET_MASK,
					ctx->compute_cb_target_mask);


	/* Emit vertex buffer state */
	ctx->cs_vertex_buffer_state.atom.num_dw = 12 * util_bitcount(ctx->cs_vertex_buffer_state.dirty_mask);
	r600_emit_atom(ctx, &ctx->cs_vertex_buffer_state.atom);

	/* Emit compute shader state */
	r600_emit_atom(ctx, &ctx->cs_shader_state.atom);

	for (i = 0; i < get_compute_resource_num(); i++) {
		if (resources[i].enabled) {
			int j;
			COMPUTE_DBG("resnum: %i, cdw: %i\n", i, cs->cdw);

			for (j = 0; j < resources[i].cs_end; j++) {
				if (resources[i].do_reloc[j]) {
					assert(resources[i].bo);
					evergreen_emit_ctx_reloc(ctx,
						resources[i].bo,
						resources[i].usage);
				}

				cs->buf[cs->cdw++] = resources[i].cs[j];
			}

			if (resources[i].bo) {
				onebo = resources[i].bo;
				evergreen_emit_ctx_reloc(ctx,
					resources[i].bo,
					resources[i].usage);

				///special case for textures
				if (resources[i].do_reloc
					[resources[i].cs_end] == 2) {
					evergreen_emit_ctx_reloc(ctx,
						resources[i].bo,
						resources[i].usage);
				}
			}
		}
	}

	/* Emit dispatch state and dispatch packet */
	evergreen_emit_direct_dispatch(ctx, block_layout, grid_layout);

	/* r600_flush_framebuffer() updates the cb_flush_flags and then
	 * calls r600_emit_atom() on the ctx->surface_sync_cmd.atom, which emits
	 * a SURFACE_SYNC packet via r600_emit_surface_sync().
	 *
	 * XXX r600_emit_surface_sync() hardcodes the CP_COHER_SIZE to
	 * 0xffffffff, so we will need to add a field to struct
	 * r600_surface_sync_cmd if we want to manually set this value.
	 */
	r600_flush_framebuffer(ctx, true /* Flush now */);

#if 0
	COMPUTE_DBG("cdw: %i\n", cs->cdw);
	for (i = 0; i < cs->cdw; i++) {
		COMPUTE_DBG("%4i : 0x%08X\n", i, ctx->cs->buf[i]);
	}
#endif

	ctx->ws->cs_flush(ctx->cs, RADEON_FLUSH_ASYNC | RADEON_FLUSH_COMPUTE);

	ctx->pm4_dirty_cdwords = 0;
	ctx->flags = 0;

	COMPUTE_DBG("shader started\n");

	ctx->ws->buffer_wait(onebo->buf, 0);

	COMPUTE_DBG("...\n");

	ctx->streamout_start = TRUE;
	ctx->streamout_append_bitmask = ~0;

}


/**
 * Emit function for r600_cs_shader_state atom
 */
void evergreen_emit_cs_shader(
		struct r600_context *rctx,
		struct r600_atom *atom)
{
	struct r600_cs_shader_state *state =
					(struct r600_cs_shader_state*)atom;
	struct r600_pipe_compute *shader = state->shader;
	struct radeon_winsys_cs *cs = rctx->cs;
	uint64_t va;

	va = r600_resource_va(&rctx->screen->screen, &shader->shader_code_bo->b.b);

	r600_write_compute_context_reg_seq(cs, R_0288D0_SQ_PGM_START_LS, 3);
	r600_write_value(cs, va >> 8); /* R_0288D0_SQ_PGM_START_LS */
	r600_write_value(cs,           /* R_0288D4_SQ_PGM_RESOURCES_LS */
			S_0288D4_NUM_GPRS(shader->bc.ngpr)
			| S_0288D4_STACK_SIZE(shader->bc.nstack));
	r600_write_value(cs, 0);	/* R_0288D8_SQ_PGM_RESOURCES_LS_2 */

	r600_write_value(cs, PKT3C(PKT3_NOP, 0, 0));
	r600_write_value(cs, r600_context_bo_reloc(rctx, shader->shader_code_bo,
							RADEON_USAGE_READ));

	r600_inval_shader_cache(rctx);
}

static void evergreen_launch_grid(
		struct pipe_context *ctx_,
		const uint *block_layout, const uint *grid_layout,
		uint32_t pc, const void *input)
{
	struct r600_context *ctx = (struct r600_context *)ctx_;

	COMPUTE_DBG("PC: %i\n", pc);

	evergreen_compute_upload_input(ctx_, block_layout, grid_layout, input);
	compute_emit_cs(ctx, block_layout, grid_layout);
}

static void evergreen_set_compute_resources(struct pipe_context * ctx_,
		unsigned start, unsigned count,
		struct pipe_surface ** surfaces)
{
	struct r600_context *ctx = (struct r600_context *)ctx_;
	struct r600_surface **resources = (struct r600_surface **)surfaces;

	COMPUTE_DBG("*** evergreen_set_compute_resources: start = %u count = %u\n",
			start, count);

	for (int i = 0; i < count; i++)	{
		/* The First two vertex buffers are reserved for parameters and
		 * global buffers. */
		unsigned vtx_id = 2 + i;
		if (resources[i]) {
			struct r600_resource_global *buffer =
				(struct r600_resource_global*)
				resources[i]->base.texture;
			if (resources[i]->base.writable) {
				assert(i+1 < 12);

				evergreen_set_rat(ctx->cs_shader_state.shader, i+1,
				(struct r600_resource *)resources[i]->base.texture,
				buffer->chunk->start_in_dw*4,
				resources[i]->base.texture->width0);
			}

			evergreen_cs_set_vertex_buffer(ctx, vtx_id,
					buffer->chunk->start_in_dw * 4,
					resources[i]->base.texture);
		}
	}
}

static void evergreen_set_cs_sampler_view(struct pipe_context *ctx_,
		unsigned start_slot, unsigned count,
		struct pipe_sampler_view **views)
{
	struct r600_context *ctx = (struct r600_context *)ctx_;
	struct r600_pipe_sampler_view **resource =
		(struct r600_pipe_sampler_view **)views;

	for (int i = 0; i < count; i++)	{
		if (resource[i]) {
			assert(i+1 < 12);
			///FETCH0 = VTX0 (param buffer),
			//FETCH1 = VTX1 (global buffer pool), FETCH2... = TEX
			evergreen_set_tex_resource(ctx->cs_shader_state.shader, resource[i], i+2);
		}
	}
}

static void evergreen_bind_compute_sampler_states(
	struct pipe_context *ctx_,
	unsigned start_slot,
	unsigned num_samplers,
	void **samplers_)
{
	struct r600_context *ctx = (struct r600_context *)ctx_;
	struct compute_sampler_state ** samplers =
		(struct compute_sampler_state **)samplers_;

	for (int i = 0; i < num_samplers; i++) {
		if (samplers[i]) {
			evergreen_set_sampler_resource(
				ctx->cs_shader_state.shader, samplers[i], i);
		}
	}
}

static void evergreen_set_global_binding(
	struct pipe_context *ctx_, unsigned first, unsigned n,
	struct pipe_resource **resources,
	uint32_t **handles)
{
	struct r600_context *ctx = (struct r600_context *)ctx_;
	struct compute_memory_pool *pool = ctx->screen->global_pool;
	struct r600_resource_global **buffers =
		(struct r600_resource_global **)resources;

	COMPUTE_DBG("*** evergreen_set_global_binding first = %u n = %u\n",
			first, n);

	if (!resources) {
		/* XXX: Unset */
		return;
	}

	compute_memory_finalize_pending(pool, ctx_);

	for (int i = 0; i < n; i++)
	{
		assert(resources[i]->target == PIPE_BUFFER);
		assert(resources[i]->bind & PIPE_BIND_GLOBAL);

		*(handles[i]) = buffers[i]->chunk->start_in_dw * 4;
	}

	evergreen_set_rat(ctx->cs_shader_state.shader, 0, pool->bo, 0, pool->size_in_dw * 4);
	evergreen_cs_set_vertex_buffer(ctx, 1, 0,
				(struct pipe_resource*)pool->bo);
}

/**
 * This function initializes all the compute specific registers that need to
 * be initialized for each compute command stream.  Registers that are common
 * to both compute and 3D will be initialized at the beginning of each compute
 * command stream by the start_cs_cmd atom.  However, since the SET_CONTEXT_REG
 * packet requires that the shader type bit be set, we must initialize all
 * context registers needed for compute in this function.  The registers
 * intialized by the start_cs_cmd atom can be found in evereen_state.c in the
 * functions evergreen_init_atom_start_cs or cayman_init_atom_start_cs depending
 * on the GPU family.
 */
void evergreen_init_atom_start_compute_cs(struct r600_context *ctx)
{
	struct r600_command_buffer *cb = &ctx->start_compute_cs_cmd;
	int num_threads;
	int num_stack_entries;

	/* since all required registers are initialised in the
	 * start_compute_cs_cmd atom, we can EMIT_EARLY here.
	 */
	r600_init_command_buffer(cb, 256, EMIT_EARLY);
	cb->pkt_flags = RADEON_CP_PACKET3_COMPUTE_MODE;

	switch (ctx->family) {
	case CHIP_CEDAR:
	default:
		num_threads = 128;
		num_stack_entries = 256;
		break;
	case CHIP_REDWOOD:
		num_threads = 128;
		num_stack_entries = 256;
		break;
	case CHIP_JUNIPER:
		num_threads = 128;
		num_stack_entries = 512;
		break;
	case CHIP_CYPRESS:
	case CHIP_HEMLOCK:
		num_threads = 128;
		num_stack_entries = 512;
		break;
	case CHIP_PALM:
		num_threads = 128;
		num_stack_entries = 256;
		break;
	case CHIP_SUMO:
		num_threads = 128;
		num_stack_entries = 256;
		break;
	case CHIP_SUMO2:
		num_threads = 128;
		num_stack_entries = 512;
		break;
	case CHIP_BARTS:
		num_threads = 128;
		num_stack_entries = 512;
		break;
	case CHIP_TURKS:
		num_threads = 128;
		num_stack_entries = 256;
		break;
	case CHIP_CAICOS:
		num_threads = 128;
		num_stack_entries = 256;
		break;
	}

	/* Config Registers */
	evergreen_init_common_regs(cb, ctx->chip_class
			, ctx->family, ctx->screen->info.drm_minor);

	/* The primitive type always needs to be POINTLIST for compute. */
	r600_store_config_reg(cb, R_008958_VGT_PRIMITIVE_TYPE,
						V_008958_DI_PT_POINTLIST);

	if (ctx->chip_class < CAYMAN) {

		/* These registers control which simds can be used by each stage.
		 * The default for these registers is 0xffffffff, which means
		 * all simds are available for each stage.  It's possible we may
		 * want to play around with these in the future, but for now
		 * the default value is fine.
		 *
		 * R_008E20_SQ_STATIC_THREAD_MGMT1
		 * R_008E24_SQ_STATIC_THREAD_MGMT2
		 * R_008E28_SQ_STATIC_THREAD_MGMT3
		 */

		/* XXX: We may need to adjust the thread and stack resouce
		 * values for 3D/compute interop */

		r600_store_config_reg_seq(cb, R_008C18_SQ_THREAD_RESOURCE_MGMT_1, 5);

		/* R_008C18_SQ_THREAD_RESOURCE_MGMT_1
		 * Set the number of threads used by the PS/VS/GS/ES stage to
		 * 0.
		 */
		r600_store_value(cb, 0);

		/* R_008C1C_SQ_THREAD_RESOURCE_MGMT_2
		 * Set the number of threads used by the CS (aka LS) stage to
		 * the maximum number of threads and set the number of threads
		 * for the HS stage to 0. */
		r600_store_value(cb, S_008C1C_NUM_LS_THREADS(num_threads));

		/* R_008C20_SQ_STACK_RESOURCE_MGMT_1
		 * Set the Control Flow stack entries to 0 for PS/VS stages */
		r600_store_value(cb, 0);

		/* R_008C24_SQ_STACK_RESOURCE_MGMT_2
		 * Set the Control Flow stack entries to 0 for GS/ES stages */
		r600_store_value(cb, 0);

		/* R_008C28_SQ_STACK_RESOURCE_MGMT_3
		 * Set the Contol Flow stack entries to 0 for the HS stage, and
		 * set it to the maximum value for the CS (aka LS) stage. */
		r600_store_value(cb,
			S_008C28_NUM_LS_STACK_ENTRIES(num_stack_entries));
	}

	/* Context Registers */

	if (ctx->chip_class < CAYMAN) {
		/* workaround for hw issues with dyn gpr - must set all limits
		 * to 240 instead of 0, 0x1e == 240 / 8
		 */
		r600_store_context_reg(cb, R_028838_SQ_DYN_GPR_RESOURCE_LIMIT_1,
				S_028838_PS_GPRS(0x1e) |
				S_028838_VS_GPRS(0x1e) |
				S_028838_GS_GPRS(0x1e) |
				S_028838_ES_GPRS(0x1e) |
				S_028838_HS_GPRS(0x1e) |
				S_028838_LS_GPRS(0x1e));
	}

	/* XXX: Investigate setting bit 15, which is FAST_COMPUTE_MODE */
	r600_store_context_reg(cb, R_028A40_VGT_GS_MODE,
		S_028A40_COMPUTE_MODE(1) | S_028A40_PARTIAL_THD_AT_EOI(1));

	r600_store_context_reg(cb, R_028B54_VGT_SHADER_STAGES_EN, 2/*CS_ON*/);

	r600_store_context_reg(cb, R_0286E8_SPI_COMPUTE_INPUT_CNTL,
						S_0286E8_TID_IN_GROUP_ENA
						| S_0286E8_TGID_ENA
						| S_0286E8_DISABLE_INDEX_PACK)
						;

	/* The LOOP_CONST registers are an optimizations for loops that allows
	 * you to store the initial counter, increment value, and maximum
	 * counter value in a register so that hardware can calculate the
	 * correct number of iterations for the loop, so that you don't need
	 * to have the loop counter in your shader code.  We don't currently use
	 * this optimization, so we must keep track of the counter in the
	 * shader and use a break instruction to exit loops.  However, the
	 * hardware will still uses this register to determine when to exit a
	 * loop, so we need to initialize the counter to 0, set the increment
	 * value to 1 and the maximum counter value to the 4095 (0xfff) which
	 * is the maximum value allowed.  This gives us a maximum of 4096
	 * iterations for our loops, but hopefully our break instruction will
	 * execute before some time before the 4096th iteration.
	 */
	eg_store_loop_const(cb, R_03A200_SQ_LOOP_CONST_0 + (160 * 4), 0x1000FFF);
}

void evergreen_init_compute_state_functions(struct r600_context *ctx)
{
	ctx->context.create_compute_state = evergreen_create_compute_state;
	ctx->context.delete_compute_state = evergreen_delete_compute_state;
	ctx->context.bind_compute_state = evergreen_bind_compute_state;
//	 ctx->context.create_sampler_view = evergreen_compute_create_sampler_view;
	ctx->context.set_compute_resources = evergreen_set_compute_resources;
	ctx->context.set_compute_sampler_views = evergreen_set_cs_sampler_view;
	ctx->context.bind_compute_sampler_states = evergreen_bind_compute_sampler_states;
	ctx->context.set_global_binding = evergreen_set_global_binding;
	ctx->context.launch_grid = evergreen_launch_grid;

	/* We always use at least two vertex buffers for compute, one for
         * parameters and one for global memory */
	ctx->cs_vertex_buffer_state.enabled_mask =
	ctx->cs_vertex_buffer_state.dirty_mask = 1 | 2;
}


struct pipe_resource *r600_compute_global_buffer_create(
	struct pipe_screen *screen,
	const struct pipe_resource *templ)
{
	assert(templ->target == PIPE_BUFFER);
	assert(templ->bind & PIPE_BIND_GLOBAL);
	assert(templ->array_size == 1 || templ->array_size == 0);
	assert(templ->depth0 == 1 || templ->depth0 == 0);
	assert(templ->height0 == 1 || templ->height0 == 0);

	struct r600_resource_global* result = (struct r600_resource_global*)
		CALLOC(sizeof(struct r600_resource_global), 1);
	struct r600_screen* rscreen = (struct r600_screen*)screen;

	COMPUTE_DBG("*** r600_compute_global_buffer_create\n");
	COMPUTE_DBG("width = %u array_size = %u\n", templ->width0,
			templ->array_size);

	result->base.b.vtbl = &r600_global_buffer_vtbl;
	result->base.b.b.screen = screen;
	result->base.b.b = *templ;
	pipe_reference_init(&result->base.b.b.reference, 1);

	int size_in_dw = (templ->width0+3) / 4;

	result->chunk = compute_memory_alloc(rscreen->global_pool, size_in_dw);

	if (result->chunk == NULL)
	{
		free(result);
		return NULL;
	}

	return &result->base.b.b;
}

void r600_compute_global_buffer_destroy(
	struct pipe_screen *screen,
	struct pipe_resource *res)
{
	assert(res->target == PIPE_BUFFER);
	assert(res->bind & PIPE_BIND_GLOBAL);

	struct r600_resource_global* buffer = (struct r600_resource_global*)res;
	struct r600_screen* rscreen = (struct r600_screen*)screen;

	compute_memory_free(rscreen->global_pool, buffer->chunk->id);

	buffer->chunk = NULL;
	free(res);
}

void* r600_compute_global_transfer_map(
	struct pipe_context *ctx_,
	struct pipe_transfer* transfer)
{
	assert(transfer->resource->target == PIPE_BUFFER);
	assert(transfer->resource->bind & PIPE_BIND_GLOBAL);
	assert(transfer->box.x >= 0);
	assert(transfer->box.y == 0);
	assert(transfer->box.z == 0);

	struct r600_context *ctx = (struct r600_context *)ctx_;
	struct r600_resource_global* buffer =
		(struct r600_resource_global*)transfer->resource;

	uint32_t* map;
	///TODO: do it better, mapping is not possible if the pool is too big

	if (!(map = ctx->ws->buffer_map(buffer->chunk->pool->bo->cs_buf,
						ctx->cs, transfer->usage))) {
		return NULL;
	}

	COMPUTE_DBG("buffer start: %lli\n", buffer->chunk->start_in_dw);
	return ((char*)(map + buffer->chunk->start_in_dw)) + transfer->box.x;
}

void r600_compute_global_transfer_unmap(
	struct pipe_context *ctx_,
	struct pipe_transfer* transfer)
{
	assert(transfer->resource->target == PIPE_BUFFER);
	assert(transfer->resource->bind & PIPE_BIND_GLOBAL);

	struct r600_context *ctx = (struct r600_context *)ctx_;
	struct r600_resource_global* buffer =
		(struct r600_resource_global*)transfer->resource;

	ctx->ws->buffer_unmap(buffer->chunk->pool->bo->cs_buf);
}

struct pipe_transfer * r600_compute_global_get_transfer(
	struct pipe_context *ctx_,
	struct pipe_resource *resource,
	unsigned level,
	unsigned usage,
	const struct pipe_box *box)
{
	struct r600_context *ctx = (struct r600_context *)ctx_;
	struct compute_memory_pool *pool = ctx->screen->global_pool;

	compute_memory_finalize_pending(pool, ctx_);

	assert(resource->target == PIPE_BUFFER);
	struct r600_context *rctx = (struct r600_context*)ctx_;
	struct pipe_transfer *transfer = util_slab_alloc(&rctx->pool_transfers);

	transfer->resource = resource;
	transfer->level = level;
	transfer->usage = usage;
	transfer->box = *box;
	transfer->stride = 0;
	transfer->layer_stride = 0;
	transfer->data = NULL;

	/* Note strides are zero, this is ok for buffers, but not for
	* textures 2d & higher at least.
	*/
	return transfer;
}

void r600_compute_global_transfer_destroy(
	struct pipe_context *ctx_,
	struct pipe_transfer *transfer)
{
	struct r600_context *rctx = (struct r600_context*)ctx_;
	util_slab_free(&rctx->pool_transfers, transfer);
}

void r600_compute_global_transfer_flush_region(
	struct pipe_context *ctx_,
	struct pipe_transfer *transfer,
	const struct pipe_box *box)
{
	assert(0 && "TODO");
}

void r600_compute_global_transfer_inline_write(
	struct pipe_context *pipe,
	struct pipe_resource *resource,
	unsigned level,
	unsigned usage,
	const struct pipe_box *box,
	const void *data,
	unsigned stride,
	unsigned layer_stride)
{
	assert(0 && "TODO");
}
