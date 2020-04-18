/*
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
 
#ifndef EVERGREEN_COMPUTE_INTERNAL_H
#define EVERGREEN_COMPUTE_INTERNAL_H

#include "compute_memory_pool.h"

enum evergreen_compute_resources
{
#define DECL_COMPUTE_RESOURCE(name, n) COMPUTE_RESOURCE_ ## name ,
#include "compute_resource.def"
#undef DECL_COMPUTE_RESOURCE
__COMPUTE_RESOURCE_END__
};

typedef unsigned u32;

#define COMPUTE_RES_TC_FLUSH      0xF0001
#define COMPUTE_RES_VC_FLUSH      0xF0002
#define COMPUTE_RES_SH_FLUSH      0xF0004
#define COMPUTE_RES_CB_FLUSH(x)  (0xF0008 | x << 8)
#define COMPUTE_RES_FULL_FLUSH    0xF0010

struct evergreen_compute_resource {
	int enabled;

	int do_reloc[256];
	u32 cs[256];
	int cs_end;

	struct r600_resource *bo;
	int coher_bo_size;
	enum radeon_bo_usage usage;
	int flags; ///flags for COMPUTE_RES_*_FLUSH
};

struct compute_sampler_state {
	struct r600_pipe_state base;
	struct pipe_sampler_state state;
};

struct number_type_and_format {
	unsigned format;
	unsigned number_type;
	unsigned num_format_all;
};

struct r600_pipe_compute {
	struct r600_context *ctx;
	struct r600_bytecode bc;
	struct tgsi_token *tokens;

	struct evergreen_compute_resource *resources;

	unsigned local_size;
	unsigned private_size;
	unsigned input_size;
#ifdef HAVE_OPENCL
	LLVMModuleRef mod;
#endif
	struct r600_resource *kernel_param;
	struct r600_resource *shader_code_bo;
};

int evergreen_compute_get_gpu_format(struct number_type_and_format* fmt, struct r600_resource *bo); ///get hw format from resource, return 0 on faliure, nonzero on success


void evergreen_emit_raw_reg_set(struct evergreen_compute_resource* res, unsigned index, int num);
void evergreen_emit_ctx_reg_set(struct r600_context *ctx, unsigned index, int num);
void evergreen_emit_raw_value(struct evergreen_compute_resource* res, unsigned value);
void evergreen_emit_ctx_value(struct r600_context *ctx, unsigned value);
void evergreen_mult_reg_set_(struct evergreen_compute_resource* res,  int index, u32* array, int size);
void evergreen_emit_ctx_reloc(struct r600_context *ctx, struct r600_resource *bo, enum radeon_bo_usage usage);
void evergreen_reg_set(struct evergreen_compute_resource* res, unsigned index, unsigned value);
void evergreen_emit_force_reloc(struct evergreen_compute_resource* res);

struct evergreen_compute_resource* get_empty_res(struct r600_pipe_compute*, enum evergreen_compute_resources res_code, int index);
int get_compute_resource_num(void);

#define evergreen_mult_reg_set(res, index, array) evergreen_mult_reg_set_(res, index, array, sizeof(array))

void evergreen_set_rat(struct r600_pipe_compute *pipe, int id, struct r600_resource* bo, int start, int size);
void evergreen_set_gds(struct r600_pipe_compute *pipe, uint32_t addr, uint32_t size);
void evergreen_set_export(struct r600_pipe_compute *pipe, struct r600_resource* bo, int offset, int size);
void evergreen_set_loop_const(struct r600_pipe_compute *pipe, int id, int count, int init, int inc);
void evergreen_set_tmp_ring(struct r600_pipe_compute *pipe, struct r600_resource* bo, int offset, int size, int se);
void evergreen_set_tex_resource(struct r600_pipe_compute *pipe, struct r600_pipe_sampler_view* view, int id);
void evergreen_set_sampler_resource(struct r600_pipe_compute *pipe, struct compute_sampler_state *sampler, int id);
void evergreen_set_const_cache(struct r600_pipe_compute *pipe, int cache_id, struct r600_resource* cbo, int size, int offset);

struct r600_resource* r600_compute_buffer_alloc_vram(struct r600_screen *screen, unsigned size);

#endif
