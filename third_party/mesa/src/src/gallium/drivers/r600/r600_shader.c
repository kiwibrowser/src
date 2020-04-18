/*
 * Copyright 2010 Jerome Glisse <glisse@freedesktop.org>
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
#include "r600_sq.h"
#include "r600_llvm.h"
#include "r600_formats.h"
#include "r600_opcodes.h"
#include "r600d.h"

#include "pipe/p_shader_tokens.h"
#include "tgsi/tgsi_info.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_scan.h"
#include "tgsi/tgsi_dump.h"
#include "util/u_memory.h"
#include <stdio.h>
#include <errno.h>
#include <byteswap.h>

/* CAYMAN notes 
Why CAYMAN got loops for lots of instructions is explained here.

-These 8xx t-slot only ops are implemented in all vector slots.
MUL_LIT, FLT_TO_UINT, INT_TO_FLT, UINT_TO_FLT
These 8xx t-slot only opcodes become vector ops, with all four 
slots expecting the arguments on sources a and b. Result is 
broadcast to all channels.
MULLO_INT, MULHI_INT, MULLO_UINT, MULHI_UINT
These 8xx t-slot only opcodes become vector ops in the z, y, and 
x slots.
EXP_IEEE, LOG_IEEE/CLAMPED, RECIP_IEEE/CLAMPED/FF/INT/UINT/_64/CLAMPED_64
RECIPSQRT_IEEE/CLAMPED/FF/_64/CLAMPED_64
SQRT_IEEE/_64
SIN/COS
The w slot may have an independent co-issued operation, or if the 
result is required to be in the w slot, the opcode above may be 
issued in the w slot as well.
The compiler must issue the source argument to slots z, y, and x
*/

static int r600_pipe_shader(struct pipe_context *ctx, struct r600_pipe_shader *shader)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_shader *rshader = &shader->shader;
	uint32_t *ptr;
	int	i;

	/* copy new shader */
	if (shader->bo == NULL) {
		shader->bo = (struct r600_resource*)
			pipe_buffer_create(ctx->screen, PIPE_BIND_CUSTOM, PIPE_USAGE_IMMUTABLE, rshader->bc.ndw * 4);
		if (shader->bo == NULL) {
			return -ENOMEM;
		}
		ptr = (uint32_t*)rctx->ws->buffer_map(shader->bo->cs_buf, rctx->cs, PIPE_TRANSFER_WRITE);
		if (R600_BIG_ENDIAN) {
			for (i = 0; i < rshader->bc.ndw; ++i) {
				ptr[i] = bswap_32(rshader->bc.bytecode[i]);
			}
		} else {
			memcpy(ptr, rshader->bc.bytecode, rshader->bc.ndw * sizeof(*ptr));
		}
		rctx->ws->buffer_unmap(shader->bo->cs_buf);
	}
	/* build state */
	switch (rshader->processor_type) {
	case TGSI_PROCESSOR_VERTEX:
		if (rctx->chip_class >= EVERGREEN) {
			evergreen_pipe_shader_vs(ctx, shader);
		} else {
			r600_pipe_shader_vs(ctx, shader);
		}
		break;
	case TGSI_PROCESSOR_FRAGMENT:
		if (rctx->chip_class >= EVERGREEN) {
			evergreen_pipe_shader_ps(ctx, shader);
		} else {
			r600_pipe_shader_ps(ctx, shader);
		}
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int r600_shader_from_tgsi(struct r600_context * rctx, struct r600_pipe_shader *pipeshader);

int r600_pipe_shader_create(struct pipe_context *ctx, struct r600_pipe_shader *shader)
{
	static int dump_shaders = -1;
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_pipe_shader_selector *sel = shader->selector;
	int r;

	/* Would like some magic "get_bool_option_once" routine.
	*/
	if (dump_shaders == -1)
		dump_shaders = debug_get_bool_option("R600_DUMP_SHADERS", FALSE);

	if (dump_shaders) {
		fprintf(stderr, "--------------------------------------------------------------\n");
		tgsi_dump(sel->tokens, 0);

		if (sel->so.num_outputs) {
			unsigned i;
			fprintf(stderr, "STREAMOUT\n");
			for (i = 0; i < sel->so.num_outputs; i++) {
				unsigned mask = ((1 << sel->so.output[i].num_components) - 1) <<
						sel->so.output[i].start_component;
				fprintf(stderr, "  %i: MEM_STREAM0_BUF%i OUT[%i].%s%s%s%s\n", i,
					sel->so.output[i].output_buffer, sel->so.output[i].register_index,
				        mask & 1 ? "x" : "_",
				        (mask >> 1) & 1 ? "y" : "_",
				        (mask >> 2) & 1 ? "z" : "_",
				        (mask >> 3) & 1 ? "w" : "_");
			}
		}
	}
	r = r600_shader_from_tgsi(rctx, shader);
	if (r) {
		R600_ERR("translation from TGSI failed !\n");
		return r;
	}
	r = r600_bytecode_build(&shader->shader.bc);
	if (r) {
		R600_ERR("building bytecode failed !\n");
		return r;
	}
	if (dump_shaders) {
		r600_bytecode_dump(&shader->shader.bc);
		fprintf(stderr, "______________________________________________________________\n");
	}
	return r600_pipe_shader(ctx, shader);
}

void r600_pipe_shader_destroy(struct pipe_context *ctx, struct r600_pipe_shader *shader)
{
	pipe_resource_reference((struct pipe_resource**)&shader->bo, NULL);
	r600_bytecode_clear(&shader->shader.bc);
}

/*
 * tgsi -> r600 shader
 */
struct r600_shader_tgsi_instruction;

struct r600_shader_src {
	unsigned				sel;
	unsigned				swizzle[4];
	unsigned				neg;
	unsigned				abs;
	unsigned				rel;
	uint32_t				value[4];
};

struct r600_shader_ctx {
	struct tgsi_shader_info			info;
	struct tgsi_parse_context		parse;
	const struct tgsi_token			*tokens;
	unsigned				type;
	unsigned				file_offset[TGSI_FILE_COUNT];
	unsigned				temp_reg;
	struct r600_shader_tgsi_instruction	*inst_info;
	struct r600_bytecode			*bc;
	struct r600_shader			*shader;
	struct r600_shader_src			src[4];
	uint32_t				*literals;
	uint32_t				nliterals;
	uint32_t				max_driver_temp_used;
	/* needed for evergreen interpolation */
	boolean                                 input_centroid;
	boolean                                 input_linear;
	boolean                                 input_perspective;
	int					num_interp_gpr;
	int					face_gpr;
	int					colors_used;
	boolean                 clip_vertex_write;
	unsigned                cv_output;
	int					fragcoord_input;
	int					native_integers;
};

struct r600_shader_tgsi_instruction {
	unsigned	tgsi_opcode;
	unsigned	is_op3;
	unsigned	r600_opcode;
	int (*process)(struct r600_shader_ctx *ctx);
};

static struct r600_shader_tgsi_instruction r600_shader_tgsi_instruction[], eg_shader_tgsi_instruction[], cm_shader_tgsi_instruction[];
static int tgsi_helper_tempx_replicate(struct r600_shader_ctx *ctx);
static inline void callstack_check_depth(struct r600_shader_ctx *ctx, unsigned reason, unsigned check_max_only);
static void fc_pushlevel(struct r600_shader_ctx *ctx, int type);
static int tgsi_else(struct r600_shader_ctx *ctx);
static int tgsi_endif(struct r600_shader_ctx *ctx);
static int tgsi_bgnloop(struct r600_shader_ctx *ctx);
static int tgsi_endloop(struct r600_shader_ctx *ctx);
static int tgsi_loop_brk_cont(struct r600_shader_ctx *ctx);

/*
 * bytestream -> r600 shader
 *
 * These functions are used to transform the output of the LLVM backend into
 * struct r600_bytecode.
 */

static void r600_bytecode_from_byte_stream(struct r600_shader_ctx *ctx,
				unsigned char * bytes,	unsigned num_bytes);

#ifdef HAVE_OPENCL
int r600_compute_shader_create(struct pipe_context * ctx,
	LLVMModuleRef mod,  struct r600_bytecode * bytecode)
{
	struct r600_context *r600_ctx = (struct r600_context *)ctx;
	unsigned char * bytes;
	unsigned byte_count;
	struct r600_shader_ctx shader_ctx;
	unsigned dump = 0;

	if (debug_get_bool_option("R600_DUMP_SHADERS", FALSE)) {
		dump = 1;
	}

	r600_llvm_compile(mod, &bytes, &byte_count, r600_ctx->family , dump);
	shader_ctx.bc = bytecode;
	r600_bytecode_init(shader_ctx.bc, r600_ctx->chip_class, r600_ctx->family);
	shader_ctx.bc->type = TGSI_PROCESSOR_COMPUTE;
	r600_bytecode_from_byte_stream(&shader_ctx, bytes, byte_count);
	if (shader_ctx.bc->chip_class == CAYMAN) {
		cm_bytecode_add_cf_end(shader_ctx.bc);
	}
	r600_bytecode_build(shader_ctx.bc);
	if (dump) {
		r600_bytecode_dump(shader_ctx.bc);
	}
	return 1;
}

#endif /* HAVE_OPENCL */

static uint32_t i32_from_byte_stream(unsigned char * bytes,
		unsigned * bytes_read)
{
	unsigned i;
	uint32_t out = 0;
	for (i = 0; i < 4; i++) {
		out |= bytes[(*bytes_read)++] << (8 * i);
	}
	return out;
}

static unsigned r600_src_from_byte_stream(unsigned char * bytes,
		unsigned bytes_read, struct r600_bytecode_alu * alu, unsigned src_idx)
{
	unsigned i;
	unsigned sel0, sel1;
	sel0 = bytes[bytes_read++];
	sel1 = bytes[bytes_read++];
	alu->src[src_idx].sel = sel0 | (sel1 << 8);
	alu->src[src_idx].chan = bytes[bytes_read++];
	alu->src[src_idx].neg = bytes[bytes_read++];
	alu->src[src_idx].abs = bytes[bytes_read++];
	alu->src[src_idx].rel = bytes[bytes_read++];
	alu->src[src_idx].kc_bank = bytes[bytes_read++];
	for (i = 0; i < 4; i++) {
		alu->src[src_idx].value |= bytes[bytes_read++] << (i * 8);
	}
	return bytes_read;
}

static unsigned r600_alu_from_byte_stream(struct r600_shader_ctx *ctx,
				unsigned char * bytes, unsigned bytes_read)
{
	unsigned src_idx;
	unsigned inst0, inst1;
	unsigned push_modifier;
	struct r600_bytecode_alu alu;
	memset(&alu, 0, sizeof(alu));
	for(src_idx = 0; src_idx < 3; src_idx++) {
		bytes_read = r600_src_from_byte_stream(bytes, bytes_read,
								&alu, src_idx);
	}

	alu.dst.sel = bytes[bytes_read++];
	alu.dst.chan = bytes[bytes_read++];
	alu.dst.clamp = bytes[bytes_read++];
	alu.dst.write = bytes[bytes_read++];
	alu.dst.rel = bytes[bytes_read++];
	inst0 = bytes[bytes_read++];
	inst1 = bytes[bytes_read++];
	alu.inst = inst0 | (inst1 << 8);
	alu.last = bytes[bytes_read++];
	alu.is_op3 = bytes[bytes_read++];
	push_modifier = bytes[bytes_read++];
	alu.pred_sel = bytes[bytes_read++];
	alu.bank_swizzle = bytes[bytes_read++];
	alu.bank_swizzle_force = bytes[bytes_read++];
	alu.omod = bytes[bytes_read++];
	alu.index_mode = bytes[bytes_read++];


	if (alu.inst == CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETNE) ||
	    alu.inst == CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETE) ||
	    alu.inst == CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETE_INT) ||
	    alu.inst == CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETNE_INT)) {
		alu.update_pred = 1;
		alu.dst.write = 0;
		alu.src[1].sel = V_SQ_ALU_SRC_0;
		alu.src[1].chan = 0;
		alu.last = 1;
    }

    if (push_modifier) {
        alu.pred_sel = 0;
		alu.execute_mask = 1;
		r600_bytecode_add_alu_type(ctx->bc, &alu, CTX_INST(V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU_PUSH_BEFORE));
	} else
		r600_bytecode_add_alu(ctx->bc, &alu);


	/* XXX: Handle other KILL instructions */
	if (alu.inst == CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLGT)) {
		ctx->shader->uses_kill = 1;
		/* XXX: This should be enforced in the LLVM backend. */
		ctx->bc->force_add_cf = 1;
	}
	return bytes_read;
}

static void llvm_if(struct r600_shader_ctx *ctx, struct r600_bytecode_alu * alu,
	unsigned pred_inst)
{
	r600_bytecode_add_cfinst(ctx->bc, CTX_INST(V_SQ_CF_WORD1_SQ_CF_INST_JUMP));
	fc_pushlevel(ctx, FC_IF);
	callstack_check_depth(ctx, FC_PUSH_VPM, 0);
}

static void r600_break_from_byte_stream(struct r600_shader_ctx *ctx,
			struct r600_bytecode_alu *alu, unsigned compare_opcode)
{
	unsigned opcode = TGSI_OPCODE_BRK;
	if (ctx->bc->chip_class == CAYMAN)
		ctx->inst_info = &cm_shader_tgsi_instruction[opcode];
	else if (ctx->bc->chip_class >= EVERGREEN)
		ctx->inst_info = &eg_shader_tgsi_instruction[opcode];
	else
		ctx->inst_info = &r600_shader_tgsi_instruction[opcode];
	llvm_if(ctx, alu, compare_opcode);
	tgsi_loop_brk_cont(ctx);
	tgsi_endif(ctx);
}

static unsigned r600_fc_from_byte_stream(struct r600_shader_ctx *ctx,
				unsigned char * bytes, unsigned bytes_read)
{
	struct r600_bytecode_alu alu;
	unsigned inst;
	memset(&alu, 0, sizeof(alu));
	bytes_read = r600_src_from_byte_stream(bytes, bytes_read, &alu, 0);
	inst = bytes[bytes_read++];
	switch (inst) {
	case 0: /* FC_IF */
		llvm_if(ctx, &alu,
			CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETNE));
		break;
	case 1: /* FC_IF_INT */
		llvm_if(ctx, &alu,
			CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETNE_INT));
		break;
	case 2: /* FC_ELSE */
		tgsi_else(ctx);
		break;
	case 3: /* FC_ENDIF */
		tgsi_endif(ctx);
		break;
	case 4: /* FC_BGNLOOP */
		tgsi_bgnloop(ctx);
		break;
	case 5: /* FC_ENDLOOP */
		tgsi_endloop(ctx);
		break;
	case 6: /* FC_BREAK */
		r600_break_from_byte_stream(ctx, &alu,
			CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETE_INT));
		break;
	case 7: /* FC_BREAK_NZ_INT */
		r600_break_from_byte_stream(ctx, &alu,
			CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETNE_INT));
		break;
	case 8: /* FC_CONTINUE */
		{
			unsigned opcode = TGSI_OPCODE_CONT;
			if (ctx->bc->chip_class == CAYMAN) {
				ctx->inst_info =
					&cm_shader_tgsi_instruction[opcode];
			} else if (ctx->bc->chip_class >= EVERGREEN) {
				ctx->inst_info =
					&eg_shader_tgsi_instruction[opcode];
			} else {
				ctx->inst_info =
					&r600_shader_tgsi_instruction[opcode];
			}
			tgsi_loop_brk_cont(ctx);
		}
		break;
	case 9: /* FC_BREAK_Z_INT */
		r600_break_from_byte_stream(ctx, &alu,
			CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETE_INT));
		break;
	case 10: /* FC_BREAK_NZ */
		r600_break_from_byte_stream(ctx, &alu,
			CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETNE));
		break;
	}

	return bytes_read;
}

static unsigned r600_tex_from_byte_stream(struct r600_shader_ctx *ctx,
				unsigned char * bytes, unsigned bytes_read)
{
	struct r600_bytecode_tex tex;

	tex.inst = bytes[bytes_read++];
	tex.resource_id = bytes[bytes_read++];
	tex.src_gpr = bytes[bytes_read++];
	tex.src_rel = bytes[bytes_read++];
	tex.dst_gpr = bytes[bytes_read++];
	tex.dst_rel = bytes[bytes_read++];
	tex.dst_sel_x = bytes[bytes_read++];
	tex.dst_sel_y = bytes[bytes_read++];
	tex.dst_sel_z = bytes[bytes_read++];
	tex.dst_sel_w = bytes[bytes_read++];
	tex.lod_bias = bytes[bytes_read++];
	tex.coord_type_x = bytes[bytes_read++];
	tex.coord_type_y = bytes[bytes_read++];
	tex.coord_type_z = bytes[bytes_read++];
	tex.coord_type_w = bytes[bytes_read++];
	tex.offset_x = bytes[bytes_read++];
	tex.offset_y = bytes[bytes_read++];
	tex.offset_z = bytes[bytes_read++];
	tex.sampler_id = bytes[bytes_read++];
	tex.src_sel_x = bytes[bytes_read++];
	tex.src_sel_y = bytes[bytes_read++];
	tex.src_sel_z = bytes[bytes_read++];
	tex.src_sel_w = bytes[bytes_read++];

	r600_bytecode_add_tex(ctx->bc, &tex);

	return bytes_read;
}

static int r600_vtx_from_byte_stream(struct r600_shader_ctx *ctx,
	unsigned char * bytes, unsigned bytes_read)
{
	struct r600_bytecode_vtx vtx;

	uint32_t word0 = i32_from_byte_stream(bytes, &bytes_read);
        uint32_t word1 = i32_from_byte_stream(bytes, &bytes_read);
	uint32_t word2 = i32_from_byte_stream(bytes, &bytes_read);

	memset(&vtx, 0, sizeof(vtx));

	/* WORD0 */
	vtx.inst = G_SQ_VTX_WORD0_VTX_INST(word0);
	vtx.fetch_type = G_SQ_VTX_WORD0_FETCH_TYPE(word0);
	vtx.buffer_id = G_SQ_VTX_WORD0_BUFFER_ID(word0);
	vtx.src_gpr = G_SQ_VTX_WORD0_SRC_GPR(word0);
	vtx.src_sel_x = G_SQ_VTX_WORD0_SRC_SEL_X(word0);
	vtx.mega_fetch_count = G_SQ_VTX_WORD0_MEGA_FETCH_COUNT(word0);

	/* WORD1 */
	vtx.dst_gpr = G_SQ_VTX_WORD1_GPR_DST_GPR(word1);
	vtx.dst_sel_x = G_SQ_VTX_WORD1_DST_SEL_X(word1);
	vtx.dst_sel_y = G_SQ_VTX_WORD1_DST_SEL_Y(word1);
	vtx.dst_sel_z = G_SQ_VTX_WORD1_DST_SEL_Z(word1);
	vtx.dst_sel_w = G_SQ_VTX_WORD1_DST_SEL_W(word1);
	vtx.use_const_fields = G_SQ_VTX_WORD1_USE_CONST_FIELDS(word1);
	vtx.data_format = G_SQ_VTX_WORD1_DATA_FORMAT(word1);
	vtx.num_format_all = G_SQ_VTX_WORD1_NUM_FORMAT_ALL(word1);
	vtx.format_comp_all = G_SQ_VTX_WORD1_FORMAT_COMP_ALL(word1);
	vtx.srf_mode_all = G_SQ_VTX_WORD1_SRF_MODE_ALL(word1);

	/* WORD 2*/
	vtx.offset = G_SQ_VTX_WORD2_OFFSET(word2);
	vtx.endian = G_SQ_VTX_WORD2_ENDIAN_SWAP(word2);

	if (r600_bytecode_add_vtx(ctx->bc, &vtx)) {
		fprintf(stderr, "Error adding vtx\n");
	}
	/* Use the Texture Cache */
	ctx->bc->cf_last->inst = EG_V_SQ_CF_WORD1_SQ_CF_INST_TEX;
	return bytes_read;
}

static void r600_bytecode_from_byte_stream(struct r600_shader_ctx *ctx,
				unsigned char * bytes,	unsigned num_bytes)
{
	unsigned bytes_read = 0;
	unsigned i, byte;
	while (bytes_read < num_bytes) {
		char inst_type = bytes[bytes_read++];
		switch (inst_type) {
		case 0:
			bytes_read = r600_alu_from_byte_stream(ctx, bytes,
								bytes_read);
			break;
		case 1:
			bytes_read = r600_tex_from_byte_stream(ctx, bytes,
								bytes_read);
			break;
		case 2:
			bytes_read = r600_fc_from_byte_stream(ctx, bytes,
								bytes_read);
			break;
		case 3:
			r600_bytecode_add_cfinst(ctx->bc, CF_NATIVE);
			for (i = 0; i < 2; i++) {
				for (byte = 0 ; byte < 4; byte++) {
					ctx->bc->cf_last->isa[i] |=
					(bytes[bytes_read++] << (byte * 8));
				}
			}
			break;

		case 4:
			bytes_read = r600_vtx_from_byte_stream(ctx, bytes,
								bytes_read);
			break;
		default:
			/* XXX: Error here */
			break;
		}
	}
}

/* End bytestream -> r600 shader functions*/

static int tgsi_is_supported(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *i = &ctx->parse.FullToken.FullInstruction;
	int j;

	if (i->Instruction.NumDstRegs > 1) {
		R600_ERR("too many dst (%d)\n", i->Instruction.NumDstRegs);
		return -EINVAL;
	}
	if (i->Instruction.Predicate) {
		R600_ERR("predicate unsupported\n");
		return -EINVAL;
	}
#if 0
	if (i->Instruction.Label) {
		R600_ERR("label unsupported\n");
		return -EINVAL;
	}
#endif
	for (j = 0; j < i->Instruction.NumSrcRegs; j++) {
		if (i->Src[j].Register.Dimension) {
			R600_ERR("unsupported src %d (dimension %d)\n", j,
				 i->Src[j].Register.Dimension);
			return -EINVAL;
		}
	}
	for (j = 0; j < i->Instruction.NumDstRegs; j++) {
		if (i->Dst[j].Register.Dimension) {
			R600_ERR("unsupported dst (dimension)\n");
			return -EINVAL;
		}
	}
	return 0;
}

static int evergreen_interp_alu(struct r600_shader_ctx *ctx, int input)
{
	int i, r;
	struct r600_bytecode_alu alu;
	int gpr = 0, base_chan = 0;
	int ij_index = 0;

	if (ctx->shader->input[input].interpolate == TGSI_INTERPOLATE_PERSPECTIVE) {
		ij_index = 0;
		if (ctx->shader->input[input].centroid)
			ij_index++;
	} else if (ctx->shader->input[input].interpolate == TGSI_INTERPOLATE_LINEAR) {
		ij_index = 0;
		/* if we have perspective add one */
		if (ctx->input_perspective)  {
			ij_index++;
			/* if we have perspective centroid */
			if (ctx->input_centroid)
				ij_index++;
		}
		if (ctx->shader->input[input].centroid)
			ij_index++;
	}

	/* work out gpr and base_chan from index */
	gpr = ij_index / 2;
	base_chan = (2 * (ij_index % 2)) + 1;

	for (i = 0; i < 8; i++) {
		memset(&alu, 0, sizeof(struct r600_bytecode_alu));

		if (i < 4)
			alu.inst = EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_INTERP_ZW;
		else
			alu.inst = EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_INTERP_XY;

		if ((i > 1) && (i < 6)) {
			alu.dst.sel = ctx->shader->input[input].gpr;
			alu.dst.write = 1;
		}

		alu.dst.chan = i % 4;

		alu.src[0].sel = gpr;
		alu.src[0].chan = (base_chan - (i % 2));

		alu.src[1].sel = V_SQ_ALU_SRC_PARAM_BASE + ctx->shader->input[input].lds_pos;

		alu.bank_swizzle_force = SQ_ALU_VEC_210;
		if ((i % 4) == 3)
			alu.last = 1;
		r = r600_bytecode_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}
	return 0;
}

static int evergreen_interp_flat(struct r600_shader_ctx *ctx, int input)
{
	int i, r;
	struct r600_bytecode_alu alu;

	for (i = 0; i < 4; i++) {
		memset(&alu, 0, sizeof(struct r600_bytecode_alu));

		alu.inst = EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_INTERP_LOAD_P0;

		alu.dst.sel = ctx->shader->input[input].gpr;
		alu.dst.write = 1;

		alu.dst.chan = i;

		alu.src[0].sel = V_SQ_ALU_SRC_PARAM_BASE + ctx->shader->input[input].lds_pos;
		alu.src[0].chan = i;

		if (i == 3)
			alu.last = 1;
		r = r600_bytecode_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}
	return 0;
}

/*
 * Special export handling in shaders
 *
 * shader export ARRAY_BASE for EXPORT_POS:
 * 60 is position
 * 61 is misc vector
 * 62, 63 are clip distance vectors
 *
 * The use of the values exported in 61-63 are controlled by PA_CL_VS_OUT_CNTL:
 * VS_OUT_MISC_VEC_ENA - enables the use of all fields in export 61
 * USE_VTX_POINT_SIZE - point size in the X channel of export 61
 * USE_VTX_EDGE_FLAG - edge flag in the Y channel of export 61
 * USE_VTX_RENDER_TARGET_INDX - render target index in the Z channel of export 61
 * USE_VTX_VIEWPORT_INDX - viewport index in the W channel of export 61
 * USE_VTX_KILL_FLAG - kill flag in the Z channel of export 61 (mutually
 * exclusive from render target index)
 * VS_OUT_CCDIST0_VEC_ENA/VS_OUT_CCDIST1_VEC_ENA - enable clip distance vectors
 *
 *
 * shader export ARRAY_BASE for EXPORT_PIXEL:
 * 0-7 CB targets
 * 61 computed Z vector
 *
 * The use of the values exported in the computed Z vector are controlled
 * by DB_SHADER_CONTROL:
 * Z_EXPORT_ENABLE - Z as a float in RED
 * STENCIL_REF_EXPORT_ENABLE - stencil ref as int in GREEN
 * COVERAGE_TO_MASK_ENABLE - alpha to mask in ALPHA
 * MASK_EXPORT_ENABLE - pixel sample mask in BLUE
 * DB_SOURCE_FORMAT - export control restrictions
 *
 */


/* Map name/sid pair from tgsi to the 8-bit semantic index for SPI setup */
static int r600_spi_sid(struct r600_shader_io * io)
{
	int index, name = io->name;

	/* These params are handled differently, they don't need
	 * semantic indices, so we'll use 0 for them.
	 */
	if (name == TGSI_SEMANTIC_POSITION ||
		name == TGSI_SEMANTIC_PSIZE ||
		name == TGSI_SEMANTIC_FACE)
		index = 0;
	else {
		if (name == TGSI_SEMANTIC_GENERIC) {
			/* For generic params simply use sid from tgsi */
			index = io->sid;
		} else {
			/* For non-generic params - pack name and sid into 8 bits */
			index = 0x80 | (name<<3) | (io->sid);
		}

		/* Make sure that all really used indices have nonzero value, so
		 * we can just compare it to 0 later instead of comparing the name
		 * with different values to detect special cases. */
		index++;
	}

	return index;
};

/* turn input into interpolate on EG */
static int evergreen_interp_input(struct r600_shader_ctx *ctx, int index)
{
	int r = 0;

	if (ctx->shader->input[index].spi_sid) {
		ctx->shader->input[index].lds_pos = ctx->shader->nlds++;
		if (ctx->shader->input[index].interpolate > 0) {
			r = evergreen_interp_alu(ctx, index);
		} else {
			r = evergreen_interp_flat(ctx, index);
		}
	}
	return r;
}

static int select_twoside_color(struct r600_shader_ctx *ctx, int front, int back)
{
	struct r600_bytecode_alu alu;
	int i, r;
	int gpr_front = ctx->shader->input[front].gpr;
	int gpr_back = ctx->shader->input[back].gpr;

	for (i = 0; i < 4; i++) {
		memset(&alu, 0, sizeof(alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_CNDGT);
		alu.is_op3 = 1;
		alu.dst.write = 1;
		alu.dst.sel = gpr_front;
		alu.src[0].sel = ctx->face_gpr;
		alu.src[1].sel = gpr_front;
		alu.src[2].sel = gpr_back;

		alu.dst.chan = i;
		alu.src[1].chan = i;
		alu.src[2].chan = i;
		alu.last = (i==3);

		if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
			return r;
	}

	return 0;
}

static int tgsi_declaration(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_declaration *d = &ctx->parse.FullToken.FullDeclaration;
	unsigned i;
	int r;

	switch (d->Declaration.File) {
	case TGSI_FILE_INPUT:
		i = ctx->shader->ninput++;
		ctx->shader->input[i].name = d->Semantic.Name;
		ctx->shader->input[i].sid = d->Semantic.Index;
		ctx->shader->input[i].spi_sid = r600_spi_sid(&ctx->shader->input[i]);
		ctx->shader->input[i].interpolate = d->Interp.Interpolate;
		ctx->shader->input[i].centroid = d->Interp.Centroid;
		ctx->shader->input[i].gpr = ctx->file_offset[TGSI_FILE_INPUT] + d->Range.First;
		if (ctx->type == TGSI_PROCESSOR_FRAGMENT) {
			switch (ctx->shader->input[i].name) {
			case TGSI_SEMANTIC_FACE:
				ctx->face_gpr = ctx->shader->input[i].gpr;
				break;
			case TGSI_SEMANTIC_COLOR:
				ctx->colors_used++;
				break;
			case TGSI_SEMANTIC_POSITION:
				ctx->fragcoord_input = i;
				break;
			}
			if (ctx->bc->chip_class >= EVERGREEN) {
				if ((r = evergreen_interp_input(ctx, i)))
					return r;
			}
		}
		break;
	case TGSI_FILE_OUTPUT:
		i = ctx->shader->noutput++;
		ctx->shader->output[i].name = d->Semantic.Name;
		ctx->shader->output[i].sid = d->Semantic.Index;
		ctx->shader->output[i].spi_sid = r600_spi_sid(&ctx->shader->output[i]);
		ctx->shader->output[i].gpr = ctx->file_offset[TGSI_FILE_OUTPUT] + d->Range.First;
		ctx->shader->output[i].interpolate = d->Interp.Interpolate;
		ctx->shader->output[i].write_mask = d->Declaration.UsageMask;
		if (ctx->type == TGSI_PROCESSOR_VERTEX) {
			switch (d->Semantic.Name) {
			case TGSI_SEMANTIC_CLIPDIST:
				ctx->shader->clip_dist_write |= d->Declaration.UsageMask << (d->Semantic.Index << 2);
				break;
			case TGSI_SEMANTIC_PSIZE:
				ctx->shader->vs_out_misc_write = 1;
				ctx->shader->vs_out_point_size = 1;
				break;
			case TGSI_SEMANTIC_CLIPVERTEX:
				ctx->clip_vertex_write = TRUE;
				ctx->cv_output = i;
				break;
			}
		} else if (ctx->type == TGSI_PROCESSOR_FRAGMENT) {
			switch (d->Semantic.Name) {
			case TGSI_SEMANTIC_COLOR:
				ctx->shader->nr_ps_max_color_exports++;
				break;
			}
		}
		break;
	case TGSI_FILE_CONSTANT:
	case TGSI_FILE_TEMPORARY:
	case TGSI_FILE_SAMPLER:
	case TGSI_FILE_ADDRESS:
		break;

	case TGSI_FILE_SYSTEM_VALUE:
		if (d->Semantic.Name == TGSI_SEMANTIC_INSTANCEID) {
			if (!ctx->native_integers) {
				struct r600_bytecode_alu alu;
				memset(&alu, 0, sizeof(struct r600_bytecode_alu));

				alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_INT_TO_FLT);
				alu.src[0].sel = 0;
				alu.src[0].chan = 3;

				alu.dst.sel = 0;
				alu.dst.chan = 3;
				alu.dst.write = 1;
				alu.last = 1;

				if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
					return r;
			}
			break;
		} else if (d->Semantic.Name == TGSI_SEMANTIC_VERTEXID)
			break;
	default:
		R600_ERR("unsupported file %d declaration\n", d->Declaration.File);
		return -EINVAL;
	}
	return 0;
}

static int r600_get_temp(struct r600_shader_ctx *ctx)
{
	return ctx->temp_reg + ctx->max_driver_temp_used++;
}

/*
 * for evergreen we need to scan the shader to find the number of GPRs we need to
 * reserve for interpolation.
 *
 * we need to know if we are going to emit
 * any centroid inputs
 * if perspective and linear are required
*/
static int evergreen_gpr_count(struct r600_shader_ctx *ctx)
{
	int i;
	int num_baryc;

	ctx->input_linear = FALSE;
	ctx->input_perspective = FALSE;
	ctx->input_centroid = FALSE;
	ctx->num_interp_gpr = 1;

	/* any centroid inputs */
	for (i = 0; i < ctx->info.num_inputs; i++) {
		/* skip position/face */
		if (ctx->info.input_semantic_name[i] == TGSI_SEMANTIC_POSITION ||
		    ctx->info.input_semantic_name[i] == TGSI_SEMANTIC_FACE)
			continue;
		if (ctx->info.input_interpolate[i] == TGSI_INTERPOLATE_LINEAR)
			ctx->input_linear = TRUE;
		if (ctx->info.input_interpolate[i] == TGSI_INTERPOLATE_PERSPECTIVE)
			ctx->input_perspective = TRUE;
		if (ctx->info.input_centroid[i])
			ctx->input_centroid = TRUE;
	}

	num_baryc = 0;
	/* ignoring sample for now */
	if (ctx->input_perspective)
		num_baryc++;
	if (ctx->input_linear)
		num_baryc++;
	if (ctx->input_centroid)
		num_baryc *= 2;

	ctx->num_interp_gpr += (num_baryc + 1) >> 1;

	/* XXX PULL MODEL and LINE STIPPLE, FIXED PT POS */
	return ctx->num_interp_gpr;
}

static void tgsi_src(struct r600_shader_ctx *ctx,
		     const struct tgsi_full_src_register *tgsi_src,
		     struct r600_shader_src *r600_src)
{
	memset(r600_src, 0, sizeof(*r600_src));
	r600_src->swizzle[0] = tgsi_src->Register.SwizzleX;
	r600_src->swizzle[1] = tgsi_src->Register.SwizzleY;
	r600_src->swizzle[2] = tgsi_src->Register.SwizzleZ;
	r600_src->swizzle[3] = tgsi_src->Register.SwizzleW;
	r600_src->neg = tgsi_src->Register.Negate;
	r600_src->abs = tgsi_src->Register.Absolute;

	if (tgsi_src->Register.File == TGSI_FILE_IMMEDIATE) {
		int index;
		if ((tgsi_src->Register.SwizzleX == tgsi_src->Register.SwizzleY) &&
			(tgsi_src->Register.SwizzleX == tgsi_src->Register.SwizzleZ) &&
			(tgsi_src->Register.SwizzleX == tgsi_src->Register.SwizzleW)) {

			index = tgsi_src->Register.Index * 4 + tgsi_src->Register.SwizzleX;
			r600_bytecode_special_constants(ctx->literals[index], &r600_src->sel, &r600_src->neg);
			if (r600_src->sel != V_SQ_ALU_SRC_LITERAL)
				return;
		}
		index = tgsi_src->Register.Index;
		r600_src->sel = V_SQ_ALU_SRC_LITERAL;
		memcpy(r600_src->value, ctx->literals + index * 4, sizeof(r600_src->value));
	} else if (tgsi_src->Register.File == TGSI_FILE_SYSTEM_VALUE) {
		if (ctx->info.system_value_semantic_name[tgsi_src->Register.Index] == TGSI_SEMANTIC_INSTANCEID) {
			r600_src->swizzle[0] = 3;
			r600_src->swizzle[1] = 3;
			r600_src->swizzle[2] = 3;
			r600_src->swizzle[3] = 3;
			r600_src->sel = 0;
		} else if (ctx->info.system_value_semantic_name[tgsi_src->Register.Index] == TGSI_SEMANTIC_VERTEXID) {
			r600_src->swizzle[0] = 0;
			r600_src->swizzle[1] = 0;
			r600_src->swizzle[2] = 0;
			r600_src->swizzle[3] = 0;
			r600_src->sel = 0;
		}
	} else {
		if (tgsi_src->Register.Indirect)
			r600_src->rel = V_SQ_REL_RELATIVE;
		r600_src->sel = tgsi_src->Register.Index;
		r600_src->sel += ctx->file_offset[tgsi_src->Register.File];
	}
}

static int tgsi_fetch_rel_const(struct r600_shader_ctx *ctx, unsigned int offset, unsigned int dst_reg)
{
	struct r600_bytecode_vtx vtx;
	unsigned int ar_reg;
	int r;

	if (offset) {
		struct r600_bytecode_alu alu;

		memset(&alu, 0, sizeof(alu));

		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_ADD_INT);
		alu.src[0].sel = ctx->bc->ar_reg;

		alu.src[1].sel = V_SQ_ALU_SRC_LITERAL;
		alu.src[1].value = offset;

		alu.dst.sel = dst_reg;
		alu.dst.write = 1;
		alu.last = 1;

		if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
			return r;

		ar_reg = dst_reg;
	} else {
		ar_reg = ctx->bc->ar_reg;
	}

	memset(&vtx, 0, sizeof(vtx));
	vtx.fetch_type = 2;		/* VTX_FETCH_NO_INDEX_OFFSET */
	vtx.src_gpr = ar_reg;
	vtx.mega_fetch_count = 16;
	vtx.dst_gpr = dst_reg;
	vtx.dst_sel_x = 0;		/* SEL_X */
	vtx.dst_sel_y = 1;		/* SEL_Y */
	vtx.dst_sel_z = 2;		/* SEL_Z */
	vtx.dst_sel_w = 3;		/* SEL_W */
	vtx.data_format = FMT_32_32_32_32_FLOAT;
	vtx.num_format_all = 2;		/* NUM_FORMAT_SCALED */
	vtx.format_comp_all = 1;	/* FORMAT_COMP_SIGNED */
	vtx.srf_mode_all = 1;		/* SRF_MODE_NO_ZERO */
	vtx.endian = r600_endian_swap(32);

	if ((r = r600_bytecode_add_vtx(ctx->bc, &vtx)))
		return r;

	return 0;
}

static int tgsi_split_constant(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bytecode_alu alu;
	int i, j, k, nconst, r;

	for (i = 0, nconst = 0; i < inst->Instruction.NumSrcRegs; i++) {
		if (inst->Src[i].Register.File == TGSI_FILE_CONSTANT) {
			nconst++;
		}
		tgsi_src(ctx, &inst->Src[i], &ctx->src[i]);
	}
	for (i = 0, j = nconst - 1; i < inst->Instruction.NumSrcRegs; i++) {
		if (inst->Src[i].Register.File != TGSI_FILE_CONSTANT) {
			continue;
		}

		if (ctx->src[i].rel) {
			int treg = r600_get_temp(ctx);
			if ((r = tgsi_fetch_rel_const(ctx, ctx->src[i].sel - 512, treg)))
				return r;

			ctx->src[i].sel = treg;
			ctx->src[i].rel = 0;
			j--;
		} else if (j > 0) {
			int treg = r600_get_temp(ctx);
			for (k = 0; k < 4; k++) {
				memset(&alu, 0, sizeof(struct r600_bytecode_alu));
				alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV);
				alu.src[0].sel = ctx->src[i].sel;
				alu.src[0].chan = k;
				alu.src[0].rel = ctx->src[i].rel;
				alu.dst.sel = treg;
				alu.dst.chan = k;
				alu.dst.write = 1;
				if (k == 3)
					alu.last = 1;
				r = r600_bytecode_add_alu(ctx->bc, &alu);
				if (r)
					return r;
			}
			ctx->src[i].sel = treg;
			ctx->src[i].rel =0;
			j--;
		}
	}
	return 0;
}

/* need to move any immediate into a temp - for trig functions which use literal for PI stuff */
static int tgsi_split_literal_constant(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bytecode_alu alu;
	int i, j, k, nliteral, r;

	for (i = 0, nliteral = 0; i < inst->Instruction.NumSrcRegs; i++) {
		if (ctx->src[i].sel == V_SQ_ALU_SRC_LITERAL) {
			nliteral++;
		}
	}
	for (i = 0, j = nliteral - 1; i < inst->Instruction.NumSrcRegs; i++) {
		if (j > 0 && ctx->src[i].sel == V_SQ_ALU_SRC_LITERAL) {
			int treg = r600_get_temp(ctx);
			for (k = 0; k < 4; k++) {
				memset(&alu, 0, sizeof(struct r600_bytecode_alu));
				alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV);
				alu.src[0].sel = ctx->src[i].sel;
				alu.src[0].chan = k;
				alu.src[0].value = ctx->src[i].value[k];
				alu.dst.sel = treg;
				alu.dst.chan = k;
				alu.dst.write = 1;
				if (k == 3)
					alu.last = 1;
				r = r600_bytecode_add_alu(ctx->bc, &alu);
				if (r)
					return r;
			}
			ctx->src[i].sel = treg;
			j--;
		}
	}
	return 0;
}

static int process_twoside_color_inputs(struct r600_shader_ctx *ctx)
{
	int i, r, count = ctx->shader->ninput;

	/* additional inputs will be allocated right after the existing inputs,
	 * we won't need them after the color selection, so we don't need to
	 * reserve these gprs for the rest of the shader code and to adjust
	 * output offsets etc. */
	int gpr = ctx->file_offset[TGSI_FILE_INPUT] +
			ctx->info.file_max[TGSI_FILE_INPUT] + 1;

	if (ctx->face_gpr == -1) {
		i = ctx->shader->ninput++;
		ctx->shader->input[i].name = TGSI_SEMANTIC_FACE;
		ctx->shader->input[i].spi_sid = 0;
		ctx->shader->input[i].gpr = gpr++;
		ctx->face_gpr = ctx->shader->input[i].gpr;
	}

	for (i = 0; i < count; i++) {
		if (ctx->shader->input[i].name == TGSI_SEMANTIC_COLOR) {
			int ni = ctx->shader->ninput++;
			memcpy(&ctx->shader->input[ni],&ctx->shader->input[i], sizeof(struct r600_shader_io));
			ctx->shader->input[ni].name = TGSI_SEMANTIC_BCOLOR;
			ctx->shader->input[ni].spi_sid = r600_spi_sid(&ctx->shader->input[ni]);
			ctx->shader->input[ni].gpr = gpr++;

			if (ctx->bc->chip_class >= EVERGREEN) {
				r = evergreen_interp_input(ctx, ni);
				if (r)
					return r;
			}

			r = select_twoside_color(ctx, i, ni);
			if (r)
				return r;
		}
	}
	return 0;
}

static int r600_shader_from_tgsi(struct r600_context * rctx, struct r600_pipe_shader *pipeshader)
{
	struct r600_shader *shader = &pipeshader->shader;
	struct tgsi_token *tokens = pipeshader->selector->tokens;
	struct pipe_stream_output_info so = pipeshader->selector->so;
	struct tgsi_full_immediate *immediate;
	struct tgsi_full_property *property;
	struct r600_shader_ctx ctx;
	struct r600_bytecode_output output[32];
	unsigned output_done, noutput;
	unsigned opcode;
	int i, j, k, r = 0;
	int next_pixel_base = 0, next_pos_base = 60, next_param_base = 0;
	/* Declarations used by llvm code */
	bool use_llvm = false;
	unsigned char * inst_bytes = NULL;
	unsigned inst_byte_count = 0;

#ifdef R600_USE_LLVM
	use_llvm = debug_get_bool_option("R600_LLVM", TRUE);
#endif
	ctx.bc = &shader->bc;
	ctx.shader = shader;
	ctx.native_integers = true;

	r600_bytecode_init(ctx.bc, rctx->chip_class, rctx->family);
	ctx.tokens = tokens;
	tgsi_scan_shader(tokens, &ctx.info);
	tgsi_parse_init(&ctx.parse, tokens);
	ctx.type = ctx.parse.FullHeader.Processor.Processor;
	shader->processor_type = ctx.type;
	ctx.bc->type = shader->processor_type;

	ctx.face_gpr = -1;
	ctx.fragcoord_input = -1;
	ctx.colors_used = 0;
	ctx.clip_vertex_write = 0;

	shader->nr_ps_color_exports = 0;
	shader->nr_ps_max_color_exports = 0;

	shader->two_side = (ctx.type == TGSI_PROCESSOR_FRAGMENT) && rctx->two_side;

	/* register allocations */
	/* Values [0,127] correspond to GPR[0..127].
	 * Values [128,159] correspond to constant buffer bank 0
	 * Values [160,191] correspond to constant buffer bank 1
	 * Values [256,511] correspond to cfile constants c[0..255]. (Gone on EG)
	 * Values [256,287] correspond to constant buffer bank 2 (EG)
	 * Values [288,319] correspond to constant buffer bank 3 (EG)
	 * Other special values are shown in the list below.
	 * 244  ALU_SRC_1_DBL_L: special constant 1.0 double-float, LSW. (RV670+)
	 * 245  ALU_SRC_1_DBL_M: special constant 1.0 double-float, MSW. (RV670+)
	 * 246  ALU_SRC_0_5_DBL_L: special constant 0.5 double-float, LSW. (RV670+)
	 * 247  ALU_SRC_0_5_DBL_M: special constant 0.5 double-float, MSW. (RV670+)
	 * 248	SQ_ALU_SRC_0: special constant 0.0.
	 * 249	SQ_ALU_SRC_1: special constant 1.0 float.
	 * 250	SQ_ALU_SRC_1_INT: special constant 1 integer.
	 * 251	SQ_ALU_SRC_M_1_INT: special constant -1 integer.
	 * 252	SQ_ALU_SRC_0_5: special constant 0.5 float.
	 * 253	SQ_ALU_SRC_LITERAL: literal constant.
	 * 254	SQ_ALU_SRC_PV: previous vector result.
	 * 255	SQ_ALU_SRC_PS: previous scalar result.
	 */
	for (i = 0; i < TGSI_FILE_COUNT; i++) {
		ctx.file_offset[i] = 0;
	}
	if (ctx.type == TGSI_PROCESSOR_VERTEX) {
		ctx.file_offset[TGSI_FILE_INPUT] = 1;
		if (ctx.bc->chip_class >= EVERGREEN) {
			r600_bytecode_add_cfinst(ctx.bc, EG_V_SQ_CF_WORD1_SQ_CF_INST_CALL_FS);
		} else {
			r600_bytecode_add_cfinst(ctx.bc, V_SQ_CF_WORD1_SQ_CF_INST_CALL_FS);
		}
	}
	if (ctx.type == TGSI_PROCESSOR_FRAGMENT && ctx.bc->chip_class >= EVERGREEN) {
		ctx.file_offset[TGSI_FILE_INPUT] = evergreen_gpr_count(&ctx);
	}

	/* LLVM backend setup */
#ifdef R600_USE_LLVM
	if (use_llvm && ctx.info.indirect_files) {
		fprintf(stderr, "Warning: R600 LLVM backend does not support "
				"indirect adressing.  Falling back to TGSI "
				"backend.\n");
		use_llvm = 0;
	}
	if (use_llvm) {
		struct radeon_llvm_context radeon_llvm_ctx;
		LLVMModuleRef mod;
		unsigned dump = 0;
		memset(&radeon_llvm_ctx, 0, sizeof(radeon_llvm_ctx));
		radeon_llvm_ctx.reserved_reg_count = ctx.file_offset[TGSI_FILE_INPUT];
		mod = r600_tgsi_llvm(&radeon_llvm_ctx, tokens);
		if (debug_get_bool_option("R600_DUMP_SHADERS", FALSE)) {
			dump = 1;
		}
		if (r600_llvm_compile(mod, &inst_bytes, &inst_byte_count,
							rctx->family, dump)) {
			FREE(inst_bytes);
			radeon_llvm_dispose(&radeon_llvm_ctx);
			use_llvm = 0;
			fprintf(stderr, "R600 LLVM backend failed to compile "
				"shader.  Falling back to TGSI\n");
		} else {
			ctx.file_offset[TGSI_FILE_OUTPUT] =
					ctx.file_offset[TGSI_FILE_INPUT];
		}
		radeon_llvm_dispose(&radeon_llvm_ctx);
	}
#endif
	/* End of LLVM backend setup */

	if (!use_llvm) {
		ctx.file_offset[TGSI_FILE_OUTPUT] =
			ctx.file_offset[TGSI_FILE_INPUT] +
			ctx.info.file_max[TGSI_FILE_INPUT] + 1;
	}
	ctx.file_offset[TGSI_FILE_TEMPORARY] = ctx.file_offset[TGSI_FILE_OUTPUT] +
						ctx.info.file_max[TGSI_FILE_OUTPUT] + 1;

	/* Outside the GPR range. This will be translated to one of the
	 * kcache banks later. */
	ctx.file_offset[TGSI_FILE_CONSTANT] = 512;

	ctx.file_offset[TGSI_FILE_IMMEDIATE] = V_SQ_ALU_SRC_LITERAL;
	ctx.bc->ar_reg = ctx.file_offset[TGSI_FILE_TEMPORARY] +
			ctx.info.file_max[TGSI_FILE_TEMPORARY] + 1;
	ctx.temp_reg = ctx.bc->ar_reg + 1;

	ctx.nliterals = 0;
	ctx.literals = NULL;
	shader->fs_write_all = FALSE;
	while (!tgsi_parse_end_of_tokens(&ctx.parse)) {
		tgsi_parse_token(&ctx.parse);
		switch (ctx.parse.FullToken.Token.Type) {
		case TGSI_TOKEN_TYPE_IMMEDIATE:
			immediate = &ctx.parse.FullToken.FullImmediate;
			ctx.literals = realloc(ctx.literals, (ctx.nliterals + 1) * 16);
			if(ctx.literals == NULL) {
				r = -ENOMEM;
				goto out_err;
			}
			ctx.literals[ctx.nliterals * 4 + 0] = immediate->u[0].Uint;
			ctx.literals[ctx.nliterals * 4 + 1] = immediate->u[1].Uint;
			ctx.literals[ctx.nliterals * 4 + 2] = immediate->u[2].Uint;
			ctx.literals[ctx.nliterals * 4 + 3] = immediate->u[3].Uint;
			ctx.nliterals++;
			break;
		case TGSI_TOKEN_TYPE_DECLARATION:
			r = tgsi_declaration(&ctx);
			if (r)
				goto out_err;
			break;
		case TGSI_TOKEN_TYPE_INSTRUCTION:
			break;
		case TGSI_TOKEN_TYPE_PROPERTY:
			property = &ctx.parse.FullToken.FullProperty;
			switch (property->Property.PropertyName) {
			case TGSI_PROPERTY_FS_COLOR0_WRITES_ALL_CBUFS:
				if (property->u[0].Data == 1)
					shader->fs_write_all = TRUE;
				break;
			case TGSI_PROPERTY_VS_PROHIBIT_UCPS:
				if (property->u[0].Data == 1)
					shader->vs_prohibit_ucps = TRUE;
				break;
			}
			break;
		default:
			R600_ERR("unsupported token type %d\n", ctx.parse.FullToken.Token.Type);
			r = -EINVAL;
			goto out_err;
		}
	}

	if (shader->fs_write_all && rctx->chip_class >= EVERGREEN)
		shader->nr_ps_max_color_exports = 8;

	if (ctx.fragcoord_input >= 0) {
		if (ctx.bc->chip_class == CAYMAN) {
			for (j = 0 ; j < 4; j++) {
				struct r600_bytecode_alu alu;
				memset(&alu, 0, sizeof(struct r600_bytecode_alu));
				alu.inst = BC_INST(ctx.bc, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIP_IEEE);
				alu.src[0].sel = shader->input[ctx.fragcoord_input].gpr;
				alu.src[0].chan = 3;

				alu.dst.sel = shader->input[ctx.fragcoord_input].gpr;
				alu.dst.chan = j;
				alu.dst.write = (j == 3);
				alu.last = 1;
				if ((r = r600_bytecode_add_alu(ctx.bc, &alu)))
					return r;
			}
		} else {
			struct r600_bytecode_alu alu;
			memset(&alu, 0, sizeof(struct r600_bytecode_alu));
			alu.inst = BC_INST(ctx.bc, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIP_IEEE);
			alu.src[0].sel = shader->input[ctx.fragcoord_input].gpr;
			alu.src[0].chan = 3;

			alu.dst.sel = shader->input[ctx.fragcoord_input].gpr;
			alu.dst.chan = 3;
			alu.dst.write = 1;
			alu.last = 1;
			if ((r = r600_bytecode_add_alu(ctx.bc, &alu)))
				return r;
		}
	}

	if (shader->two_side && ctx.colors_used) {
		if ((r = process_twoside_color_inputs(&ctx)))
			return r;
	}

	tgsi_parse_init(&ctx.parse, tokens);
	while (!tgsi_parse_end_of_tokens(&ctx.parse)) {
		tgsi_parse_token(&ctx.parse);
		switch (ctx.parse.FullToken.Token.Type) {
		case TGSI_TOKEN_TYPE_INSTRUCTION:
			if (use_llvm) {
				continue;
			}
			r = tgsi_is_supported(&ctx);
			if (r)
				goto out_err;
			ctx.max_driver_temp_used = 0;
			/* reserve first tmp for everyone */
			r600_get_temp(&ctx);

			opcode = ctx.parse.FullToken.FullInstruction.Instruction.Opcode;
			if ((r = tgsi_split_constant(&ctx)))
				goto out_err;
			if ((r = tgsi_split_literal_constant(&ctx)))
				goto out_err;
			if (ctx.bc->chip_class == CAYMAN)
				ctx.inst_info = &cm_shader_tgsi_instruction[opcode];
			else if (ctx.bc->chip_class >= EVERGREEN)
				ctx.inst_info = &eg_shader_tgsi_instruction[opcode];
			else
				ctx.inst_info = &r600_shader_tgsi_instruction[opcode];
			r = ctx.inst_info->process(&ctx);
			if (r)
				goto out_err;
			break;
		default:
			break;
		}
	}

	/* Get instructions if we are using the LLVM backend. */
	if (use_llvm) {
		r600_bytecode_from_byte_stream(&ctx, inst_bytes, inst_byte_count);
		FREE(inst_bytes);
	}

	noutput = shader->noutput;

	if (ctx.clip_vertex_write) {
		/* need to convert a clipvertex write into clipdistance writes and not export
		   the clip vertex anymore */

		memset(&shader->output[noutput], 0, 2*sizeof(struct r600_shader_io));
		shader->output[noutput].name = TGSI_SEMANTIC_CLIPDIST;
		shader->output[noutput].gpr = ctx.temp_reg;
		noutput++;
		shader->output[noutput].name = TGSI_SEMANTIC_CLIPDIST;
		shader->output[noutput].gpr = ctx.temp_reg+1;
		noutput++;

		/* reset spi_sid for clipvertex output to avoid confusing spi */
		shader->output[ctx.cv_output].spi_sid = 0;

		shader->clip_dist_write = 0xFF;

		for (i = 0; i < 8; i++) {
			int oreg = i >> 2;
			int ochan = i & 3;

			for (j = 0; j < 4; j++) {
				struct r600_bytecode_alu alu;
				memset(&alu, 0, sizeof(struct r600_bytecode_alu));
				alu.inst = BC_INST(ctx.bc, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_DOT4);
				alu.src[0].sel = shader->output[ctx.cv_output].gpr;
				alu.src[0].chan = j;

				alu.src[1].sel = 512 + i;
				alu.src[1].kc_bank = 1;
				alu.src[1].chan = j;

				alu.dst.sel = ctx.temp_reg + oreg;
				alu.dst.chan = j;
				alu.dst.write = (j == ochan);
				if (j == 3)
					alu.last = 1;
				r = r600_bytecode_add_alu(ctx.bc, &alu);
				if (r)
					return r;
			}
		}
	}

	/* Add stream outputs. */
	if (ctx.type == TGSI_PROCESSOR_VERTEX && so.num_outputs) {
		for (i = 0; i < so.num_outputs; i++) {
			struct r600_bytecode_output output;

			if (so.output[i].output_buffer >= 4) {
				R600_ERR("exceeded the max number of stream output buffers, got: %d\n",
					 so.output[i].output_buffer);
				r = -EINVAL;
				goto out_err;
			}
			if (so.output[i].dst_offset < so.output[i].start_component) {
			   R600_ERR("stream_output - dst_offset cannot be less than start_component\n");
			   r = -EINVAL;
			   goto out_err;
			}

			memset(&output, 0, sizeof(struct r600_bytecode_output));
			output.gpr = shader->output[so.output[i].register_index].gpr;
			output.elem_size = 0;
			output.array_base = so.output[i].dst_offset - so.output[i].start_component;
			output.type = V_SQ_CF_ALLOC_EXPORT_WORD0_SQ_EXPORT_WRITE;
			output.burst_count = 1;
			output.barrier = 1;
			/* array_size is an upper limit for the burst_count
			 * with MEM_STREAM instructions */
			output.array_size = 0xFFF;
			output.comp_mask = ((1 << so.output[i].num_components) - 1) << so.output[i].start_component;
			if (ctx.bc->chip_class >= EVERGREEN) {
				switch (so.output[i].output_buffer) {
				case 0:
					output.inst = EG_V_SQ_CF_ALLOC_EXPORT_WORD1_SQ_CF_INST_MEM_STREAM0_BUF0;
					break;
				case 1:
					output.inst = EG_V_SQ_CF_ALLOC_EXPORT_WORD1_SQ_CF_INST_MEM_STREAM0_BUF1;
					break;
				case 2:
					output.inst = EG_V_SQ_CF_ALLOC_EXPORT_WORD1_SQ_CF_INST_MEM_STREAM0_BUF2;
					break;
				case 3:
					output.inst = EG_V_SQ_CF_ALLOC_EXPORT_WORD1_SQ_CF_INST_MEM_STREAM0_BUF3;
					break;
				}
			} else {
				switch (so.output[i].output_buffer) {
				case 0:
					output.inst = V_SQ_CF_ALLOC_EXPORT_WORD1_SQ_CF_INST_MEM_STREAM0;
					break;
				case 1:
					output.inst = V_SQ_CF_ALLOC_EXPORT_WORD1_SQ_CF_INST_MEM_STREAM1;
					break;
				case 2:
					output.inst = V_SQ_CF_ALLOC_EXPORT_WORD1_SQ_CF_INST_MEM_STREAM2;
					break;
				case 3:
					output.inst = V_SQ_CF_ALLOC_EXPORT_WORD1_SQ_CF_INST_MEM_STREAM3;
					break;
				}
			}
			r = r600_bytecode_add_output(ctx.bc, &output);
			if (r)
				goto out_err;
		}
	}

	/* export output */
	for (i = 0, j = 0; i < noutput; i++, j++) {
		memset(&output[j], 0, sizeof(struct r600_bytecode_output));
		output[j].gpr = shader->output[i].gpr;
		output[j].elem_size = 3;
		output[j].swizzle_x = 0;
		output[j].swizzle_y = 1;
		output[j].swizzle_z = 2;
		output[j].swizzle_w = 3;
		output[j].burst_count = 1;
		output[j].barrier = 1;
		output[j].type = -1;
		output[j].inst = BC_INST(ctx.bc, V_SQ_CF_ALLOC_EXPORT_WORD1_SQ_CF_INST_EXPORT);
		switch (ctx.type) {
		case TGSI_PROCESSOR_VERTEX:
			switch (shader->output[i].name) {
			case TGSI_SEMANTIC_POSITION:
				output[j].array_base = next_pos_base++;
				output[j].type = V_SQ_CF_ALLOC_EXPORT_WORD0_SQ_EXPORT_POS;
				break;

			case TGSI_SEMANTIC_PSIZE:
				output[j].array_base = next_pos_base++;
				output[j].type = V_SQ_CF_ALLOC_EXPORT_WORD0_SQ_EXPORT_POS;
				break;
			case TGSI_SEMANTIC_CLIPVERTEX:
				j--;
				break;
			case TGSI_SEMANTIC_CLIPDIST:
				output[j].array_base = next_pos_base++;
				output[j].type = V_SQ_CF_ALLOC_EXPORT_WORD0_SQ_EXPORT_POS;
				/* spi_sid is 0 for clipdistance outputs that were generated
				 * for clipvertex - we don't need to pass them to PS */
				if (shader->output[i].spi_sid) {
					j++;
					/* duplicate it as PARAM to pass to the pixel shader */
					memcpy(&output[j], &output[j-1], sizeof(struct r600_bytecode_output));
					output[j].array_base = next_param_base++;
					output[j].type = V_SQ_CF_ALLOC_EXPORT_WORD0_SQ_EXPORT_PARAM;
				}
				break;
			case TGSI_SEMANTIC_FOG:
				output[j].swizzle_y = 4; /* 0 */
				output[j].swizzle_z = 4; /* 0 */
				output[j].swizzle_w = 5; /* 1 */
				break;
			}
			break;
		case TGSI_PROCESSOR_FRAGMENT:
			if (shader->output[i].name == TGSI_SEMANTIC_COLOR) {
				/* never export more colors than the number of CBs */
				if (next_pixel_base && next_pixel_base >= (rctx->nr_cbufs + rctx->dual_src_blend * 1)) {
					/* skip export */
					j--;
					continue;
				}
				output[j].swizzle_w = rctx->alpha_to_one && rctx->multisample_enable && !rctx->cb0_is_integer ? 5 : 3;
				output[j].array_base = next_pixel_base++;
				output[j].type = V_SQ_CF_ALLOC_EXPORT_WORD0_SQ_EXPORT_PIXEL;
				shader->nr_ps_color_exports++;
				if (shader->fs_write_all && (rctx->chip_class >= EVERGREEN)) {
					for (k = 1; k < rctx->nr_cbufs; k++) {
						j++;
						memset(&output[j], 0, sizeof(struct r600_bytecode_output));
						output[j].gpr = shader->output[i].gpr;
						output[j].elem_size = 3;
						output[j].swizzle_x = 0;
						output[j].swizzle_y = 1;
						output[j].swizzle_z = 2;
						output[j].swizzle_w = rctx->alpha_to_one && rctx->multisample_enable && !rctx->cb0_is_integer ? 5 : 3;
						output[j].burst_count = 1;
						output[j].barrier = 1;
						output[j].array_base = next_pixel_base++;
						output[j].inst = BC_INST(ctx.bc, V_SQ_CF_ALLOC_EXPORT_WORD1_SQ_CF_INST_EXPORT);
						output[j].type = V_SQ_CF_ALLOC_EXPORT_WORD0_SQ_EXPORT_PIXEL;
						shader->nr_ps_color_exports++;
					}
				}
			} else if (shader->output[i].name == TGSI_SEMANTIC_POSITION) {
				output[j].array_base = 61;
				output[j].swizzle_x = 2;
				output[j].swizzle_y = 7;
				output[j].swizzle_z = output[j].swizzle_w = 7;
				output[j].type = V_SQ_CF_ALLOC_EXPORT_WORD0_SQ_EXPORT_PIXEL;
			} else if (shader->output[i].name == TGSI_SEMANTIC_STENCIL) {
				output[j].array_base = 61;
				output[j].swizzle_x = 7;
				output[j].swizzle_y = 1;
				output[j].swizzle_z = output[j].swizzle_w = 7;
				output[j].type = V_SQ_CF_ALLOC_EXPORT_WORD0_SQ_EXPORT_PIXEL;
			} else {
				R600_ERR("unsupported fragment output name %d\n", shader->output[i].name);
				r = -EINVAL;
				goto out_err;
			}
			break;
		default:
			R600_ERR("unsupported processor type %d\n", ctx.type);
			r = -EINVAL;
			goto out_err;
		}

		if (output[j].type==-1) {
			output[j].type = V_SQ_CF_ALLOC_EXPORT_WORD0_SQ_EXPORT_PARAM;
			output[j].array_base = next_param_base++;
		}
	}

	/* add fake param output for vertex shader if no param is exported */
	if (ctx.type == TGSI_PROCESSOR_VERTEX && next_param_base == 0) {
			memset(&output[j], 0, sizeof(struct r600_bytecode_output));
			output[j].gpr = 0;
			output[j].elem_size = 3;
			output[j].swizzle_x = 7;
			output[j].swizzle_y = 7;
			output[j].swizzle_z = 7;
			output[j].swizzle_w = 7;
			output[j].burst_count = 1;
			output[j].barrier = 1;
			output[j].type = V_SQ_CF_ALLOC_EXPORT_WORD0_SQ_EXPORT_PARAM;
			output[j].array_base = 0;
			output[j].inst = BC_INST(ctx.bc, V_SQ_CF_ALLOC_EXPORT_WORD1_SQ_CF_INST_EXPORT);
			j++;
	}

	/* add fake pixel export */
	if (ctx.type == TGSI_PROCESSOR_FRAGMENT && next_pixel_base == 0) {
		memset(&output[j], 0, sizeof(struct r600_bytecode_output));
		output[j].gpr = 0;
		output[j].elem_size = 3;
		output[j].swizzle_x = 7;
		output[j].swizzle_y = 7;
		output[j].swizzle_z = 7;
		output[j].swizzle_w = 7;
		output[j].burst_count = 1;
		output[j].barrier = 1;
		output[j].type = V_SQ_CF_ALLOC_EXPORT_WORD0_SQ_EXPORT_PIXEL;
		output[j].array_base = 0;
		output[j].inst = BC_INST(ctx.bc, V_SQ_CF_ALLOC_EXPORT_WORD1_SQ_CF_INST_EXPORT);
		j++;
	}

	noutput = j;

	/* set export done on last export of each type */
	for (i = noutput - 1, output_done = 0; i >= 0; i--) {
		if (ctx.bc->chip_class < CAYMAN) {
			if (i == (noutput - 1)) {
				output[i].end_of_program = 1;
			}
		}
		if (!(output_done & (1 << output[i].type))) {
			output_done |= (1 << output[i].type);
			output[i].inst = BC_INST(ctx.bc, V_SQ_CF_ALLOC_EXPORT_WORD1_SQ_CF_INST_EXPORT_DONE);
		}
	}
	/* add output to bytecode */
	for (i = 0; i < noutput; i++) {
		r = r600_bytecode_add_output(ctx.bc, &output[i]);
		if (r)
			goto out_err;
	}
	/* add program end */
	if (ctx.bc->chip_class == CAYMAN)
		cm_bytecode_add_cf_end(ctx.bc);

	/* check GPR limit - we have 124 = 128 - 4
	 * (4 are reserved as alu clause temporary registers) */
	if (ctx.bc->ngpr > 124) {
		R600_ERR("GPR limit exceeded - shader requires %d registers\n", ctx.bc->ngpr);
		r = -ENOMEM;
		goto out_err;
	}

	free(ctx.literals);
	tgsi_parse_free(&ctx.parse);
	return 0;
out_err:
	free(ctx.literals);
	tgsi_parse_free(&ctx.parse);
	return r;
}

static int tgsi_unsupported(struct r600_shader_ctx *ctx)
{
	R600_ERR("%s tgsi opcode unsupported\n",
		 tgsi_get_opcode_name(ctx->inst_info->tgsi_opcode));
	return -EINVAL;
}

static int tgsi_end(struct r600_shader_ctx *ctx)
{
	return 0;
}

static void r600_bytecode_src(struct r600_bytecode_alu_src *bc_src,
			const struct r600_shader_src *shader_src,
			unsigned chan)
{
	bc_src->sel = shader_src->sel;
	bc_src->chan = shader_src->swizzle[chan];
	bc_src->neg = shader_src->neg;
	bc_src->abs = shader_src->abs;
	bc_src->rel = shader_src->rel;
	bc_src->value = shader_src->value[bc_src->chan];
}

static void r600_bytecode_src_set_abs(struct r600_bytecode_alu_src *bc_src)
{
	bc_src->abs = 1;
	bc_src->neg = 0;
}

static void r600_bytecode_src_toggle_neg(struct r600_bytecode_alu_src *bc_src)
{
	bc_src->neg = !bc_src->neg;
}

static void tgsi_dst(struct r600_shader_ctx *ctx,
		     const struct tgsi_full_dst_register *tgsi_dst,
		     unsigned swizzle,
		     struct r600_bytecode_alu_dst *r600_dst)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;

	r600_dst->sel = tgsi_dst->Register.Index;
	r600_dst->sel += ctx->file_offset[tgsi_dst->Register.File];
	r600_dst->chan = swizzle;
	r600_dst->write = 1;
	if (tgsi_dst->Register.Indirect)
		r600_dst->rel = V_SQ_REL_RELATIVE;
	if (inst->Instruction.Saturate) {
		r600_dst->clamp = 1;
	}
}

static int tgsi_last_instruction(unsigned writemask)
{
	int i, lasti = 0;

	for (i = 0; i < 4; i++) {
		if (writemask & (1 << i)) {
			lasti = i;
		}
	}
	return lasti;
}

static int tgsi_op2_s(struct r600_shader_ctx *ctx, int swap, int trans_only)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bytecode_alu alu;
	int i, j, r;
	int lasti = tgsi_last_instruction(inst->Dst[0].Register.WriteMask);

	for (i = 0; i < lasti + 1; i++) {
		if (!(inst->Dst[0].Register.WriteMask & (1 << i)))
			continue;

		memset(&alu, 0, sizeof(struct r600_bytecode_alu));
		tgsi_dst(ctx, &inst->Dst[0], i, &alu.dst);

		alu.inst = ctx->inst_info->r600_opcode;
		if (!swap) {
			for (j = 0; j < inst->Instruction.NumSrcRegs; j++) {
				r600_bytecode_src(&alu.src[j], &ctx->src[j], i);
			}
		} else {
			r600_bytecode_src(&alu.src[0], &ctx->src[1], i);
			r600_bytecode_src(&alu.src[1], &ctx->src[0], i);
		}
		/* handle some special cases */
		switch (ctx->inst_info->tgsi_opcode) {
		case TGSI_OPCODE_SUB:
			r600_bytecode_src_toggle_neg(&alu.src[1]);
			break;
		case TGSI_OPCODE_ABS:
			r600_bytecode_src_set_abs(&alu.src[0]);
			break;
		default:
			break;
		}
		if (i == lasti || trans_only) {
			alu.last = 1;
		}
		r = r600_bytecode_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}
	return 0;
}

static int tgsi_op2(struct r600_shader_ctx *ctx)
{
	return tgsi_op2_s(ctx, 0, 0);
}

static int tgsi_op2_swap(struct r600_shader_ctx *ctx)
{
	return tgsi_op2_s(ctx, 1, 0);
}

static int tgsi_op2_trans(struct r600_shader_ctx *ctx)
{
	return tgsi_op2_s(ctx, 0, 1);
}

static int tgsi_ineg(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bytecode_alu alu;
	int i, r;
	int lasti = tgsi_last_instruction(inst->Dst[0].Register.WriteMask);

	for (i = 0; i < lasti + 1; i++) {

		if (!(inst->Dst[0].Register.WriteMask & (1 << i)))
			continue;
		memset(&alu, 0, sizeof(struct r600_bytecode_alu));
		alu.inst = ctx->inst_info->r600_opcode;

		alu.src[0].sel = V_SQ_ALU_SRC_0;

		r600_bytecode_src(&alu.src[1], &ctx->src[0], i);

		tgsi_dst(ctx, &inst->Dst[0], i, &alu.dst);

		if (i == lasti) {
			alu.last = 1;
		}
		r = r600_bytecode_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}
	return 0;

}

static int cayman_emit_float_instr(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	int i, j, r;
	struct r600_bytecode_alu alu;
	int last_slot = (inst->Dst[0].Register.WriteMask & 0x8) ? 4 : 3;
	
	for (i = 0 ; i < last_slot; i++) {
		memset(&alu, 0, sizeof(struct r600_bytecode_alu));
		alu.inst = ctx->inst_info->r600_opcode;
		for (j = 0; j < inst->Instruction.NumSrcRegs; j++) {
			r600_bytecode_src(&alu.src[j], &ctx->src[j], 0);

			/* RSQ should take the absolute value of src */
			if (ctx->inst_info->tgsi_opcode == TGSI_OPCODE_RSQ) {
				r600_bytecode_src_set_abs(&alu.src[j]);
			}
		}
		tgsi_dst(ctx, &inst->Dst[0], i, &alu.dst);
		alu.dst.write = (inst->Dst[0].Register.WriteMask >> i) & 1;

		if (i == last_slot - 1)
			alu.last = 1;
		r = r600_bytecode_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}
	return 0;
}

static int cayman_mul_int_instr(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	int i, j, k, r;
	struct r600_bytecode_alu alu;
	int last_slot = (inst->Dst[0].Register.WriteMask & 0x8) ? 4 : 3;
	for (k = 0; k < last_slot; k++) {
		if (!(inst->Dst[0].Register.WriteMask & (1 << k)))
			continue;

		for (i = 0 ; i < 4; i++) {
			memset(&alu, 0, sizeof(struct r600_bytecode_alu));
			alu.inst = ctx->inst_info->r600_opcode;
			for (j = 0; j < inst->Instruction.NumSrcRegs; j++) {
				r600_bytecode_src(&alu.src[j], &ctx->src[j], k);
			}
			tgsi_dst(ctx, &inst->Dst[0], i, &alu.dst);
			alu.dst.write = (i == k);
			if (i == 3)
				alu.last = 1;
			r = r600_bytecode_add_alu(ctx->bc, &alu);
			if (r)
				return r;
		}
	}
	return 0;
}

/*
 * r600 - trunc to -PI..PI range
 * r700 - normalize by dividing by 2PI
 * see fdo bug 27901
 */
static int tgsi_setup_trig(struct r600_shader_ctx *ctx)
{
	static float half_inv_pi = 1.0 /(3.1415926535 * 2);
	static float double_pi = 3.1415926535 * 2;
	static float neg_pi = -3.1415926535;

	int r;
	struct r600_bytecode_alu alu;

	memset(&alu, 0, sizeof(struct r600_bytecode_alu));
	alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_MULADD);
	alu.is_op3 = 1;

	alu.dst.chan = 0;
	alu.dst.sel = ctx->temp_reg;
	alu.dst.write = 1;

	r600_bytecode_src(&alu.src[0], &ctx->src[0], 0);

	alu.src[1].sel = V_SQ_ALU_SRC_LITERAL;
	alu.src[1].chan = 0;
	alu.src[1].value = *(uint32_t *)&half_inv_pi;
	alu.src[2].sel = V_SQ_ALU_SRC_0_5;
	alu.src[2].chan = 0;
	alu.last = 1;
	r = r600_bytecode_add_alu(ctx->bc, &alu);
	if (r)
		return r;

	memset(&alu, 0, sizeof(struct r600_bytecode_alu));
	alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FRACT);

	alu.dst.chan = 0;
	alu.dst.sel = ctx->temp_reg;
	alu.dst.write = 1;

	alu.src[0].sel = ctx->temp_reg;
	alu.src[0].chan = 0;
	alu.last = 1;
	r = r600_bytecode_add_alu(ctx->bc, &alu);
	if (r)
		return r;

	memset(&alu, 0, sizeof(struct r600_bytecode_alu));
	alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_MULADD);
	alu.is_op3 = 1;

	alu.dst.chan = 0;
	alu.dst.sel = ctx->temp_reg;
	alu.dst.write = 1;

	alu.src[0].sel = ctx->temp_reg;
	alu.src[0].chan = 0;

	alu.src[1].sel = V_SQ_ALU_SRC_LITERAL;
	alu.src[1].chan = 0;
	alu.src[2].sel = V_SQ_ALU_SRC_LITERAL;
	alu.src[2].chan = 0;

	if (ctx->bc->chip_class == R600) {
		alu.src[1].value = *(uint32_t *)&double_pi;
		alu.src[2].value = *(uint32_t *)&neg_pi;
	} else {
		alu.src[1].sel = V_SQ_ALU_SRC_1;
		alu.src[2].sel = V_SQ_ALU_SRC_0_5;
		alu.src[2].neg = 1;
	}

	alu.last = 1;
	r = r600_bytecode_add_alu(ctx->bc, &alu);
	if (r)
		return r;
	return 0;
}

static int cayman_trig(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bytecode_alu alu;
	int last_slot = (inst->Dst[0].Register.WriteMask & 0x8) ? 4 : 3;
	int i, r;

	r = tgsi_setup_trig(ctx);
	if (r)
		return r;


	for (i = 0; i < last_slot; i++) {
		memset(&alu, 0, sizeof(struct r600_bytecode_alu));
		alu.inst = ctx->inst_info->r600_opcode;
		alu.dst.chan = i;

		tgsi_dst(ctx, &inst->Dst[0], i, &alu.dst);
		alu.dst.write = (inst->Dst[0].Register.WriteMask >> i) & 1;

		alu.src[0].sel = ctx->temp_reg;
		alu.src[0].chan = 0;
		if (i == last_slot - 1)
			alu.last = 1;
		r = r600_bytecode_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}
	return 0;
}

static int tgsi_trig(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bytecode_alu alu;
	int i, r;
	int lasti = tgsi_last_instruction(inst->Dst[0].Register.WriteMask);

	r = tgsi_setup_trig(ctx);
	if (r)
		return r;

	memset(&alu, 0, sizeof(struct r600_bytecode_alu));
	alu.inst = ctx->inst_info->r600_opcode;
	alu.dst.chan = 0;
	alu.dst.sel = ctx->temp_reg;
	alu.dst.write = 1;

	alu.src[0].sel = ctx->temp_reg;
	alu.src[0].chan = 0;
	alu.last = 1;
	r = r600_bytecode_add_alu(ctx->bc, &alu);
	if (r)
		return r;

	/* replicate result */
	for (i = 0; i < lasti + 1; i++) {
		if (!(inst->Dst[0].Register.WriteMask & (1 << i)))
			continue;

		memset(&alu, 0, sizeof(struct r600_bytecode_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV);

		alu.src[0].sel = ctx->temp_reg;
		tgsi_dst(ctx, &inst->Dst[0], i, &alu.dst);
		if (i == lasti)
			alu.last = 1;
		r = r600_bytecode_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}
	return 0;
}

static int tgsi_scs(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bytecode_alu alu;
	int i, r;

	/* We'll only need the trig stuff if we are going to write to the
	 * X or Y components of the destination vector.
	 */
	if (likely(inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_XY)) {
		r = tgsi_setup_trig(ctx);
		if (r)
			return r;
	}

	/* dst.x = COS */
	if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_X) {
		if (ctx->bc->chip_class == CAYMAN) {
			for (i = 0 ; i < 3; i++) {
				memset(&alu, 0, sizeof(struct r600_bytecode_alu));
				alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_COS);
				tgsi_dst(ctx, &inst->Dst[0], i, &alu.dst);

				if (i == 0)
					alu.dst.write = 1;
				else
					alu.dst.write = 0;
				alu.src[0].sel = ctx->temp_reg;
				alu.src[0].chan = 0;
				if (i == 2)
					alu.last = 1;
				r = r600_bytecode_add_alu(ctx->bc, &alu);
				if (r)
					return r;
			}
		} else {
			memset(&alu, 0, sizeof(struct r600_bytecode_alu));
			alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_COS);
			tgsi_dst(ctx, &inst->Dst[0], 0, &alu.dst);

			alu.src[0].sel = ctx->temp_reg;
			alu.src[0].chan = 0;
			alu.last = 1;
			r = r600_bytecode_add_alu(ctx->bc, &alu);
			if (r)
				return r;
		}
	}

	/* dst.y = SIN */
	if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_Y) {
		if (ctx->bc->chip_class == CAYMAN) {
			for (i = 0 ; i < 3; i++) {
				memset(&alu, 0, sizeof(struct r600_bytecode_alu));
				alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SIN);
				tgsi_dst(ctx, &inst->Dst[0], i, &alu.dst);
				if (i == 1)
					alu.dst.write = 1;
				else
					alu.dst.write = 0;
				alu.src[0].sel = ctx->temp_reg;
				alu.src[0].chan = 0;
				if (i == 2)
					alu.last = 1;
				r = r600_bytecode_add_alu(ctx->bc, &alu);
				if (r)
					return r;
			}
		} else {
			memset(&alu, 0, sizeof(struct r600_bytecode_alu));
			alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SIN);
			tgsi_dst(ctx, &inst->Dst[0], 1, &alu.dst);

			alu.src[0].sel = ctx->temp_reg;
			alu.src[0].chan = 0;
			alu.last = 1;
			r = r600_bytecode_add_alu(ctx->bc, &alu);
			if (r)
				return r;
		}
	}

	/* dst.z = 0.0; */
	if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_Z) {
		memset(&alu, 0, sizeof(struct r600_bytecode_alu));

		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV);

		tgsi_dst(ctx, &inst->Dst[0], 2, &alu.dst);

		alu.src[0].sel = V_SQ_ALU_SRC_0;
		alu.src[0].chan = 0;

		alu.last = 1;

		r = r600_bytecode_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}

	/* dst.w = 1.0; */
	if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_W) {
		memset(&alu, 0, sizeof(struct r600_bytecode_alu));

		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV);

		tgsi_dst(ctx, &inst->Dst[0], 3, &alu.dst);

		alu.src[0].sel = V_SQ_ALU_SRC_1;
		alu.src[0].chan = 0;

		alu.last = 1;

		r = r600_bytecode_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}

	return 0;
}

static int tgsi_kill(struct r600_shader_ctx *ctx)
{
	struct r600_bytecode_alu alu;
	int i, r;

	for (i = 0; i < 4; i++) {
		memset(&alu, 0, sizeof(struct r600_bytecode_alu));
		alu.inst = ctx->inst_info->r600_opcode;

		alu.dst.chan = i;

		alu.src[0].sel = V_SQ_ALU_SRC_0;

		if (ctx->inst_info->tgsi_opcode == TGSI_OPCODE_KILP) {
			alu.src[1].sel = V_SQ_ALU_SRC_1;
			alu.src[1].neg = 1;
		} else {
			r600_bytecode_src(&alu.src[1], &ctx->src[0], i);
		}
		if (i == 3) {
			alu.last = 1;
		}
		r = r600_bytecode_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}

	/* kill must be last in ALU */
	ctx->bc->force_add_cf = 1;
	ctx->shader->uses_kill = TRUE;
	return 0;
}

static int tgsi_lit(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bytecode_alu alu;
	int r;

	/* tmp.x = max(src.y, 0.0) */
	memset(&alu, 0, sizeof(struct r600_bytecode_alu));
	alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MAX);
	r600_bytecode_src(&alu.src[0], &ctx->src[0], 1);
	alu.src[1].sel  = V_SQ_ALU_SRC_0; /*0.0*/
	alu.src[1].chan = 1;

	alu.dst.sel = ctx->temp_reg;
	alu.dst.chan = 0;
	alu.dst.write = 1;

	alu.last = 1;
	r = r600_bytecode_add_alu(ctx->bc, &alu);
	if (r)
		return r;

	if (inst->Dst[0].Register.WriteMask & (1 << 2))
	{
		int chan;
		int sel;
		int i;

		if (ctx->bc->chip_class == CAYMAN) {
			for (i = 0; i < 3; i++) {
				/* tmp.z = log(tmp.x) */
				memset(&alu, 0, sizeof(struct r600_bytecode_alu));
				alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LOG_CLAMPED);
				alu.src[0].sel = ctx->temp_reg;
				alu.src[0].chan = 0;
				alu.dst.sel = ctx->temp_reg;
				alu.dst.chan = i;
				if (i == 2) {
					alu.dst.write = 1;
					alu.last = 1;
				} else
					alu.dst.write = 0;
				
				r = r600_bytecode_add_alu(ctx->bc, &alu);
				if (r)
					return r;
			}
		} else {
			/* tmp.z = log(tmp.x) */
			memset(&alu, 0, sizeof(struct r600_bytecode_alu));
			alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LOG_CLAMPED);
			alu.src[0].sel = ctx->temp_reg;
			alu.src[0].chan = 0;
			alu.dst.sel = ctx->temp_reg;
			alu.dst.chan = 2;
			alu.dst.write = 1;
			alu.last = 1;
			r = r600_bytecode_add_alu(ctx->bc, &alu);
			if (r)
				return r;
		}

		chan = alu.dst.chan;
		sel = alu.dst.sel;

		/* tmp.x = amd MUL_LIT(tmp.z, src.w, src.x ) */
		memset(&alu, 0, sizeof(struct r600_bytecode_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_MUL_LIT);
		alu.src[0].sel  = sel;
		alu.src[0].chan = chan;
		r600_bytecode_src(&alu.src[1], &ctx->src[0], 3);
		r600_bytecode_src(&alu.src[2], &ctx->src[0], 0);
		alu.dst.sel = ctx->temp_reg;
		alu.dst.chan = 0;
		alu.dst.write = 1;
		alu.is_op3 = 1;
		alu.last = 1;
		r = r600_bytecode_add_alu(ctx->bc, &alu);
		if (r)
			return r;

		if (ctx->bc->chip_class == CAYMAN) {
			for (i = 0; i < 3; i++) {
				/* dst.z = exp(tmp.x) */
				memset(&alu, 0, sizeof(struct r600_bytecode_alu));
				alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_EXP_IEEE);
				alu.src[0].sel = ctx->temp_reg;
				alu.src[0].chan = 0;
				tgsi_dst(ctx, &inst->Dst[0], i, &alu.dst);
				if (i == 2) {
					alu.dst.write = 1;
					alu.last = 1;
				} else
					alu.dst.write = 0;
				r = r600_bytecode_add_alu(ctx->bc, &alu);
				if (r)
					return r;
			}
		} else {
			/* dst.z = exp(tmp.x) */
			memset(&alu, 0, sizeof(struct r600_bytecode_alu));
			alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_EXP_IEEE);
			alu.src[0].sel = ctx->temp_reg;
			alu.src[0].chan = 0;
			tgsi_dst(ctx, &inst->Dst[0], 2, &alu.dst);
			alu.last = 1;
			r = r600_bytecode_add_alu(ctx->bc, &alu);
			if (r)
				return r;
		}
	}

	/* dst.x, <- 1.0  */
	memset(&alu, 0, sizeof(struct r600_bytecode_alu));
	alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV);
	alu.src[0].sel  = V_SQ_ALU_SRC_1; /*1.0*/
	alu.src[0].chan = 0;
	tgsi_dst(ctx, &inst->Dst[0], 0, &alu.dst);
	alu.dst.write = (inst->Dst[0].Register.WriteMask >> 0) & 1;
	r = r600_bytecode_add_alu(ctx->bc, &alu);
	if (r)
		return r;

	/* dst.y = max(src.x, 0.0) */
	memset(&alu, 0, sizeof(struct r600_bytecode_alu));
	alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MAX);
	r600_bytecode_src(&alu.src[0], &ctx->src[0], 0);
	alu.src[1].sel  = V_SQ_ALU_SRC_0; /*0.0*/
	alu.src[1].chan = 0;
	tgsi_dst(ctx, &inst->Dst[0], 1, &alu.dst);
	alu.dst.write = (inst->Dst[0].Register.WriteMask >> 1) & 1;
	r = r600_bytecode_add_alu(ctx->bc, &alu);
	if (r)
		return r;

	/* dst.w, <- 1.0  */
	memset(&alu, 0, sizeof(struct r600_bytecode_alu));
	alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV);
	alu.src[0].sel  = V_SQ_ALU_SRC_1;
	alu.src[0].chan = 0;
	tgsi_dst(ctx, &inst->Dst[0], 3, &alu.dst);
	alu.dst.write = (inst->Dst[0].Register.WriteMask >> 3) & 1;
	alu.last = 1;
	r = r600_bytecode_add_alu(ctx->bc, &alu);
	if (r)
		return r;

	return 0;
}

static int tgsi_rsq(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bytecode_alu alu;
	int i, r;

	memset(&alu, 0, sizeof(struct r600_bytecode_alu));

	/* XXX:
	 * For state trackers other than OpenGL, we'll want to use
	 * _RECIPSQRT_IEEE instead.
	 */
	alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIPSQRT_CLAMPED);

	for (i = 0; i < inst->Instruction.NumSrcRegs; i++) {
		r600_bytecode_src(&alu.src[i], &ctx->src[i], 0);
		r600_bytecode_src_set_abs(&alu.src[i]);
	}
	alu.dst.sel = ctx->temp_reg;
	alu.dst.write = 1;
	alu.last = 1;
	r = r600_bytecode_add_alu(ctx->bc, &alu);
	if (r)
		return r;
	/* replicate result */
	return tgsi_helper_tempx_replicate(ctx);
}

static int tgsi_helper_tempx_replicate(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bytecode_alu alu;
	int i, r;

	for (i = 0; i < 4; i++) {
		memset(&alu, 0, sizeof(struct r600_bytecode_alu));
		alu.src[0].sel = ctx->temp_reg;
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV);
		alu.dst.chan = i;
		tgsi_dst(ctx, &inst->Dst[0], i, &alu.dst);
		alu.dst.write = (inst->Dst[0].Register.WriteMask >> i) & 1;
		if (i == 3)
			alu.last = 1;
		r = r600_bytecode_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}
	return 0;
}

static int tgsi_trans_srcx_replicate(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bytecode_alu alu;
	int i, r;

	memset(&alu, 0, sizeof(struct r600_bytecode_alu));
	alu.inst = ctx->inst_info->r600_opcode;
	for (i = 0; i < inst->Instruction.NumSrcRegs; i++) {
		r600_bytecode_src(&alu.src[i], &ctx->src[i], 0);
	}
	alu.dst.sel = ctx->temp_reg;
	alu.dst.write = 1;
	alu.last = 1;
	r = r600_bytecode_add_alu(ctx->bc, &alu);
	if (r)
		return r;
	/* replicate result */
	return tgsi_helper_tempx_replicate(ctx);
}

static int cayman_pow(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	int i, r;
	struct r600_bytecode_alu alu;
	int last_slot = (inst->Dst[0].Register.WriteMask & 0x8) ? 4 : 3;

	for (i = 0; i < 3; i++) {
		memset(&alu, 0, sizeof(struct r600_bytecode_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LOG_IEEE);
		r600_bytecode_src(&alu.src[0], &ctx->src[0], 0);
		alu.dst.sel = ctx->temp_reg;
		alu.dst.chan = i;
		alu.dst.write = 1;
		if (i == 2)
			alu.last = 1;
		r = r600_bytecode_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}

	/* b * LOG2(a) */
	memset(&alu, 0, sizeof(struct r600_bytecode_alu));
	alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MUL);
	r600_bytecode_src(&alu.src[0], &ctx->src[1], 0);
	alu.src[1].sel = ctx->temp_reg;
	alu.dst.sel = ctx->temp_reg;
	alu.dst.write = 1;
	alu.last = 1;
	r = r600_bytecode_add_alu(ctx->bc, &alu);
	if (r)
		return r;

	for (i = 0; i < last_slot; i++) {
		/* POW(a,b) = EXP2(b * LOG2(a))*/
		memset(&alu, 0, sizeof(struct r600_bytecode_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_EXP_IEEE);
		alu.src[0].sel = ctx->temp_reg;

		tgsi_dst(ctx, &inst->Dst[0], i, &alu.dst);
		alu.dst.write = (inst->Dst[0].Register.WriteMask >> i) & 1;
		if (i == last_slot - 1)
			alu.last = 1;
		r = r600_bytecode_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}
	return 0;
}

static int tgsi_pow(struct r600_shader_ctx *ctx)
{
	struct r600_bytecode_alu alu;
	int r;

	/* LOG2(a) */
	memset(&alu, 0, sizeof(struct r600_bytecode_alu));
	alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LOG_IEEE);
	r600_bytecode_src(&alu.src[0], &ctx->src[0], 0);
	alu.dst.sel = ctx->temp_reg;
	alu.dst.write = 1;
	alu.last = 1;
	r = r600_bytecode_add_alu(ctx->bc, &alu);
	if (r)
		return r;
	/* b * LOG2(a) */
	memset(&alu, 0, sizeof(struct r600_bytecode_alu));
	alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MUL);
	r600_bytecode_src(&alu.src[0], &ctx->src[1], 0);
	alu.src[1].sel = ctx->temp_reg;
	alu.dst.sel = ctx->temp_reg;
	alu.dst.write = 1;
	alu.last = 1;
	r = r600_bytecode_add_alu(ctx->bc, &alu);
	if (r)
		return r;
	/* POW(a,b) = EXP2(b * LOG2(a))*/
	memset(&alu, 0, sizeof(struct r600_bytecode_alu));
	alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_EXP_IEEE);
	alu.src[0].sel = ctx->temp_reg;
	alu.dst.sel = ctx->temp_reg;
	alu.dst.write = 1;
	alu.last = 1;
	r = r600_bytecode_add_alu(ctx->bc, &alu);
	if (r)
		return r;
	return tgsi_helper_tempx_replicate(ctx);
}

static int tgsi_divmod(struct r600_shader_ctx *ctx, int mod, int signed_op)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bytecode_alu alu;
	int i, r, j;
	unsigned write_mask = inst->Dst[0].Register.WriteMask;
	int tmp0 = ctx->temp_reg;
	int tmp1 = r600_get_temp(ctx);
	int tmp2 = r600_get_temp(ctx);
	int tmp3 = r600_get_temp(ctx);
	/* Unsigned path:
	 *
	 * we need to represent src1 as src2*q + r, where q - quotient, r - remainder
	 *
	 * 1. tmp0.x = rcp (src2)     = 2^32/src2 + e, where e is rounding error
	 * 2. tmp0.z = lo (tmp0.x * src2)
	 * 3. tmp0.w = -tmp0.z
	 * 4. tmp0.y = hi (tmp0.x * src2)
	 * 5. tmp0.z = (tmp0.y == 0 ? tmp0.w : tmp0.z)      = abs(lo(rcp*src2))
	 * 6. tmp0.w = hi (tmp0.z * tmp0.x)    = e, rounding error
	 * 7. tmp1.x = tmp0.x - tmp0.w
	 * 8. tmp1.y = tmp0.x + tmp0.w
	 * 9. tmp0.x = (tmp0.y == 0 ? tmp1.y : tmp1.x)
	 * 10. tmp0.z = hi(tmp0.x * src1)     = q
	 * 11. tmp0.y = lo (tmp0.z * src2)     = src2*q = src1 - r
	 *
	 * 12. tmp0.w = src1 - tmp0.y       = r
	 * 13. tmp1.x = tmp0.w >= src2		= r >= src2 (uint comparison)
	 * 14. tmp1.y = src1 >= tmp0.y      = r >= 0 (uint comparison)
	 *
	 * if DIV
	 *
	 *   15. tmp1.z = tmp0.z + 1			= q + 1
	 *   16. tmp1.w = tmp0.z - 1			= q - 1
	 *
	 * else MOD
	 *
	 *   15. tmp1.z = tmp0.w - src2			= r - src2
	 *   16. tmp1.w = tmp0.w + src2			= r + src2
	 *
	 * endif
	 *
	 * 17. tmp1.x = tmp1.x & tmp1.y
	 *
	 * DIV: 18. tmp0.z = tmp1.x==0 ? tmp0.z : tmp1.z
	 * MOD: 18. tmp0.z = tmp1.x==0 ? tmp0.w : tmp1.z
	 *
	 * 19. tmp0.z = tmp1.y==0 ? tmp1.w : tmp0.z
	 * 20. dst = src2==0 ? MAX_UINT : tmp0.z
	 *
	 * Signed path:
	 *
	 * Same as unsigned, using abs values of the operands,
	 * and fixing the sign of the result in the end.
	 */

	for (i = 0; i < 4; i++) {
		if (!(write_mask & (1<<i)))
			continue;

		if (signed_op) {

			/* tmp2.x = -src0 */
			memset(&alu, 0, sizeof(struct r600_bytecode_alu));
			alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SUB_INT);

			alu.dst.sel = tmp2;
			alu.dst.chan = 0;
			alu.dst.write = 1;

			alu.src[0].sel = V_SQ_ALU_SRC_0;

			r600_bytecode_src(&alu.src[1], &ctx->src[0], i);

			alu.last = 1;
			if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
				return r;

			/* tmp2.y = -src1 */
			memset(&alu, 0, sizeof(struct r600_bytecode_alu));
			alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SUB_INT);

			alu.dst.sel = tmp2;
			alu.dst.chan = 1;
			alu.dst.write = 1;

			alu.src[0].sel = V_SQ_ALU_SRC_0;

			r600_bytecode_src(&alu.src[1], &ctx->src[1], i);

			alu.last = 1;
			if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
				return r;

			/* tmp2.z sign bit is set if src0 and src2 signs are different */
			/* it will be a sign of the quotient */
			if (!mod) {

				memset(&alu, 0, sizeof(struct r600_bytecode_alu));
				alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_XOR_INT);

				alu.dst.sel = tmp2;
				alu.dst.chan = 2;
				alu.dst.write = 1;

				r600_bytecode_src(&alu.src[0], &ctx->src[0], i);
				r600_bytecode_src(&alu.src[1], &ctx->src[1], i);

				alu.last = 1;
				if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
					return r;
			}

			/* tmp2.x = |src0| */
			memset(&alu, 0, sizeof(struct r600_bytecode_alu));
			alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_CNDGE_INT);
			alu.is_op3 = 1;

			alu.dst.sel = tmp2;
			alu.dst.chan = 0;
			alu.dst.write = 1;

			r600_bytecode_src(&alu.src[0], &ctx->src[0], i);
			r600_bytecode_src(&alu.src[1], &ctx->src[0], i);
			alu.src[2].sel = tmp2;
			alu.src[2].chan = 0;

			alu.last = 1;
			if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
				return r;

			/* tmp2.y = |src1| */
			memset(&alu, 0, sizeof(struct r600_bytecode_alu));
			alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_CNDGE_INT);
			alu.is_op3 = 1;

			alu.dst.sel = tmp2;
			alu.dst.chan = 1;
			alu.dst.write = 1;

			r600_bytecode_src(&alu.src[0], &ctx->src[1], i);
			r600_bytecode_src(&alu.src[1], &ctx->src[1], i);
			alu.src[2].sel = tmp2;
			alu.src[2].chan = 1;

			alu.last = 1;
			if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
				return r;

		}

		/* 1. tmp0.x = rcp_u (src2)     = 2^32/src2 + e, where e is rounding error */
		if (ctx->bc->chip_class == CAYMAN) {
			/* tmp3.x = u2f(src2) */
			memset(&alu, 0, sizeof(struct r600_bytecode_alu));
			alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_UINT_TO_FLT);

			alu.dst.sel = tmp3;
			alu.dst.chan = 0;
			alu.dst.write = 1;

			if (signed_op) {
				alu.src[0].sel = tmp2;
				alu.src[0].chan = 1;
			} else {
				r600_bytecode_src(&alu.src[0], &ctx->src[1], i);
			}

			alu.last = 1;
			if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
				return r;

			/* tmp0.x = recip(tmp3.x) */
			for (j = 0 ; j < 3; j++) {
				memset(&alu, 0, sizeof(struct r600_bytecode_alu));
				alu.inst = EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIP_IEEE;

				alu.dst.sel = tmp0;
				alu.dst.chan = j;
				alu.dst.write = (j == 0);

				alu.src[0].sel = tmp3;
				alu.src[0].chan = 0;

				if (j == 2)
					alu.last = 1;
				if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
					return r;
			}

			memset(&alu, 0, sizeof(struct r600_bytecode_alu));
			alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MUL);

			alu.src[0].sel = tmp0;
			alu.src[0].chan = 0;

			alu.src[1].sel = V_SQ_ALU_SRC_LITERAL;
			alu.src[1].value = 0x4f800000;

			alu.dst.sel = tmp3;
			alu.dst.write = 1;
			alu.last = 1;
			r = r600_bytecode_add_alu(ctx->bc, &alu);
			if (r)
				return r;

			memset(&alu, 0, sizeof(struct r600_bytecode_alu));
			alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FLT_TO_UINT);
		  
			alu.dst.sel = tmp0;
			alu.dst.chan = 0;
			alu.dst.write = 1;

			alu.src[0].sel = tmp3;
			alu.src[0].chan = 0;

			alu.last = 1;
			if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
				return r;

		} else {
			memset(&alu, 0, sizeof(struct r600_bytecode_alu));
			alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIP_UINT);

			alu.dst.sel = tmp0;
			alu.dst.chan = 0;
			alu.dst.write = 1;

			if (signed_op) {
				alu.src[0].sel = tmp2;
				alu.src[0].chan = 1;
			} else {
				r600_bytecode_src(&alu.src[0], &ctx->src[1], i);
			}

			alu.last = 1;
			if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
				return r;
		}

		/* 2. tmp0.z = lo (tmp0.x * src2) */
		if (ctx->bc->chip_class == CAYMAN) {
			for (j = 0 ; j < 4; j++) {
				memset(&alu, 0, sizeof(struct r600_bytecode_alu));
				alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MULLO_UINT);

				alu.dst.sel = tmp0;
				alu.dst.chan = j;
				alu.dst.write = (j == 2);

				alu.src[0].sel = tmp0;
				alu.src[0].chan = 0;
				if (signed_op) {
					alu.src[1].sel = tmp2;
					alu.src[1].chan = 1;
				} else {
					r600_bytecode_src(&alu.src[1], &ctx->src[1], i);
				}

				alu.last = (j == 3);
				if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
					return r;
			}
		} else {
			memset(&alu, 0, sizeof(struct r600_bytecode_alu));
			alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MULLO_UINT);

			alu.dst.sel = tmp0;
			alu.dst.chan = 2;
			alu.dst.write = 1;

			alu.src[0].sel = tmp0;
			alu.src[0].chan = 0;
			if (signed_op) {
				alu.src[1].sel = tmp2;
				alu.src[1].chan = 1;
			} else {
				r600_bytecode_src(&alu.src[1], &ctx->src[1], i);
			}
			
			alu.last = 1;
			if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
				return r;
		}

		/* 3. tmp0.w = -tmp0.z */
		memset(&alu, 0, sizeof(struct r600_bytecode_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SUB_INT);

		alu.dst.sel = tmp0;
		alu.dst.chan = 3;
		alu.dst.write = 1;

		alu.src[0].sel = V_SQ_ALU_SRC_0;
		alu.src[1].sel = tmp0;
		alu.src[1].chan = 2;

		alu.last = 1;
		if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
			return r;

		/* 4. tmp0.y = hi (tmp0.x * src2) */
		if (ctx->bc->chip_class == CAYMAN) {
			for (j = 0 ; j < 4; j++) {
				memset(&alu, 0, sizeof(struct r600_bytecode_alu));
				alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MULHI_UINT);

				alu.dst.sel = tmp0;
				alu.dst.chan = j;
				alu.dst.write = (j == 1);

				alu.src[0].sel = tmp0;
				alu.src[0].chan = 0;

				if (signed_op) {
					alu.src[1].sel = tmp2;
					alu.src[1].chan = 1;
				} else {
					r600_bytecode_src(&alu.src[1], &ctx->src[1], i);
				}
				alu.last = (j == 3);
				if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
					return r;
			}
		} else {
			memset(&alu, 0, sizeof(struct r600_bytecode_alu));
			alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MULHI_UINT);

			alu.dst.sel = tmp0;
			alu.dst.chan = 1;
			alu.dst.write = 1;

			alu.src[0].sel = tmp0;
			alu.src[0].chan = 0;

			if (signed_op) {
				alu.src[1].sel = tmp2;
				alu.src[1].chan = 1;
			} else {
				r600_bytecode_src(&alu.src[1], &ctx->src[1], i);
			}

			alu.last = 1;
			if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
				return r;
		}

		/* 5. tmp0.z = (tmp0.y == 0 ? tmp0.w : tmp0.z)      = abs(lo(rcp*src)) */
		memset(&alu, 0, sizeof(struct r600_bytecode_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_CNDE_INT);
		alu.is_op3 = 1;

		alu.dst.sel = tmp0;
		alu.dst.chan = 2;
		alu.dst.write = 1;

		alu.src[0].sel = tmp0;
		alu.src[0].chan = 1;
		alu.src[1].sel = tmp0;
		alu.src[1].chan = 3;
		alu.src[2].sel = tmp0;
		alu.src[2].chan = 2;

		alu.last = 1;
		if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
			return r;

		/* 6. tmp0.w = hi (tmp0.z * tmp0.x)    = e, rounding error */
		if (ctx->bc->chip_class == CAYMAN) {
			for (j = 0 ; j < 4; j++) {
				memset(&alu, 0, sizeof(struct r600_bytecode_alu));
				alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MULHI_UINT);

				alu.dst.sel = tmp0;
				alu.dst.chan = j;
				alu.dst.write = (j == 3);

				alu.src[0].sel = tmp0;
				alu.src[0].chan = 2;

				alu.src[1].sel = tmp0;
				alu.src[1].chan = 0;

				alu.last = (j == 3);
				if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
					return r;
			}
		} else {
			memset(&alu, 0, sizeof(struct r600_bytecode_alu));
			alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MULHI_UINT);

			alu.dst.sel = tmp0;
			alu.dst.chan = 3;
			alu.dst.write = 1;

			alu.src[0].sel = tmp0;
			alu.src[0].chan = 2;

			alu.src[1].sel = tmp0;
			alu.src[1].chan = 0;

			alu.last = 1;
			if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
				return r;
		}

		/* 7. tmp1.x = tmp0.x - tmp0.w */
		memset(&alu, 0, sizeof(struct r600_bytecode_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SUB_INT);

		alu.dst.sel = tmp1;
		alu.dst.chan = 0;
		alu.dst.write = 1;

		alu.src[0].sel = tmp0;
		alu.src[0].chan = 0;
		alu.src[1].sel = tmp0;
		alu.src[1].chan = 3;

		alu.last = 1;
		if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
			return r;

		/* 8. tmp1.y = tmp0.x + tmp0.w */
		memset(&alu, 0, sizeof(struct r600_bytecode_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_ADD_INT);

		alu.dst.sel = tmp1;
		alu.dst.chan = 1;
		alu.dst.write = 1;

		alu.src[0].sel = tmp0;
		alu.src[0].chan = 0;
		alu.src[1].sel = tmp0;
		alu.src[1].chan = 3;

		alu.last = 1;
		if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
			return r;

		/* 9. tmp0.x = (tmp0.y == 0 ? tmp1.y : tmp1.x) */
		memset(&alu, 0, sizeof(struct r600_bytecode_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_CNDE_INT);
		alu.is_op3 = 1;

		alu.dst.sel = tmp0;
		alu.dst.chan = 0;
		alu.dst.write = 1;

		alu.src[0].sel = tmp0;
		alu.src[0].chan = 1;
		alu.src[1].sel = tmp1;
		alu.src[1].chan = 1;
		alu.src[2].sel = tmp1;
		alu.src[2].chan = 0;

		alu.last = 1;
		if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
			return r;

		/* 10. tmp0.z = hi(tmp0.x * src1)     = q */
		if (ctx->bc->chip_class == CAYMAN) {
			for (j = 0 ; j < 4; j++) {
				memset(&alu, 0, sizeof(struct r600_bytecode_alu));
				alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MULHI_UINT);

				alu.dst.sel = tmp0;
				alu.dst.chan = j;
				alu.dst.write = (j == 2);

				alu.src[0].sel = tmp0;
				alu.src[0].chan = 0;

				if (signed_op) {
					alu.src[1].sel = tmp2;
					alu.src[1].chan = 0;
				} else {
					r600_bytecode_src(&alu.src[1], &ctx->src[0], i);
				}

				alu.last = (j == 3);
				if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
					return r;
			}
		} else {
			memset(&alu, 0, sizeof(struct r600_bytecode_alu));
			alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MULHI_UINT);

			alu.dst.sel = tmp0;
			alu.dst.chan = 2;
			alu.dst.write = 1;

			alu.src[0].sel = tmp0;
			alu.src[0].chan = 0;

			if (signed_op) {
				alu.src[1].sel = tmp2;
				alu.src[1].chan = 0;
			} else {
				r600_bytecode_src(&alu.src[1], &ctx->src[0], i);
			}

			alu.last = 1;
			if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
				return r;
		}

		/* 11. tmp0.y = lo (src2 * tmp0.z)     = src2*q = src1 - r */
		if (ctx->bc->chip_class == CAYMAN) {
			for (j = 0 ; j < 4; j++) {
				memset(&alu, 0, sizeof(struct r600_bytecode_alu));
				alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MULLO_UINT);

				alu.dst.sel = tmp0;
				alu.dst.chan = j;
				alu.dst.write = (j == 1);

				if (signed_op) {
					alu.src[0].sel = tmp2;
					alu.src[0].chan = 1;
				} else {
					r600_bytecode_src(&alu.src[0], &ctx->src[1], i);
				}

				alu.src[1].sel = tmp0;
				alu.src[1].chan = 2;

				alu.last = (j == 3);
				if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
					return r;
			}
		} else {
			memset(&alu, 0, sizeof(struct r600_bytecode_alu));
			alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MULLO_UINT);

			alu.dst.sel = tmp0;
			alu.dst.chan = 1;
			alu.dst.write = 1;

			if (signed_op) {
				alu.src[0].sel = tmp2;
				alu.src[0].chan = 1;
			} else {
				r600_bytecode_src(&alu.src[0], &ctx->src[1], i);
			}
			
			alu.src[1].sel = tmp0;
			alu.src[1].chan = 2;

			alu.last = 1;
			if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
				return r;
		}

		/* 12. tmp0.w = src1 - tmp0.y       = r */
		memset(&alu, 0, sizeof(struct r600_bytecode_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SUB_INT);

		alu.dst.sel = tmp0;
		alu.dst.chan = 3;
		alu.dst.write = 1;

		if (signed_op) {
			alu.src[0].sel = tmp2;
			alu.src[0].chan = 0;
		} else {
			r600_bytecode_src(&alu.src[0], &ctx->src[0], i);
		}

		alu.src[1].sel = tmp0;
		alu.src[1].chan = 1;

		alu.last = 1;
		if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
			return r;

		/* 13. tmp1.x = tmp0.w >= src2		= r >= src2 */
		memset(&alu, 0, sizeof(struct r600_bytecode_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGE_UINT);

		alu.dst.sel = tmp1;
		alu.dst.chan = 0;
		alu.dst.write = 1;

		alu.src[0].sel = tmp0;
		alu.src[0].chan = 3;
		if (signed_op) {
			alu.src[1].sel = tmp2;
			alu.src[1].chan = 1;
		} else {
			r600_bytecode_src(&alu.src[1], &ctx->src[1], i);
		}

		alu.last = 1;
		if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
			return r;

		/* 14. tmp1.y = src1 >= tmp0.y       = r >= 0 */
		memset(&alu, 0, sizeof(struct r600_bytecode_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGE_UINT);

		alu.dst.sel = tmp1;
		alu.dst.chan = 1;
		alu.dst.write = 1;

		if (signed_op) {
			alu.src[0].sel = tmp2;
			alu.src[0].chan = 0;
		} else {
			r600_bytecode_src(&alu.src[0], &ctx->src[0], i);
		}

		alu.src[1].sel = tmp0;
		alu.src[1].chan = 1;

		alu.last = 1;
		if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
			return r;

		if (mod) { /* UMOD */

			/* 15. tmp1.z = tmp0.w - src2			= r - src2 */
			memset(&alu, 0, sizeof(struct r600_bytecode_alu));
			alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SUB_INT);

			alu.dst.sel = tmp1;
			alu.dst.chan = 2;
			alu.dst.write = 1;

			alu.src[0].sel = tmp0;
			alu.src[0].chan = 3;

			if (signed_op) {
				alu.src[1].sel = tmp2;
				alu.src[1].chan = 1;
			} else {
				r600_bytecode_src(&alu.src[1], &ctx->src[1], i);
			}

			alu.last = 1;
			if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
				return r;

			/* 16. tmp1.w = tmp0.w + src2			= r + src2 */
			memset(&alu, 0, sizeof(struct r600_bytecode_alu));
			alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_ADD_INT);

			alu.dst.sel = tmp1;
			alu.dst.chan = 3;
			alu.dst.write = 1;

			alu.src[0].sel = tmp0;
			alu.src[0].chan = 3;
			if (signed_op) {
				alu.src[1].sel = tmp2;
				alu.src[1].chan = 1;
			} else {
				r600_bytecode_src(&alu.src[1], &ctx->src[1], i);
			}

			alu.last = 1;
			if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
				return r;

		} else { /* UDIV */

			/* 15. tmp1.z = tmp0.z + 1       = q + 1       DIV */
			memset(&alu, 0, sizeof(struct r600_bytecode_alu));
			alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_ADD_INT);

			alu.dst.sel = tmp1;
			alu.dst.chan = 2;
			alu.dst.write = 1;

			alu.src[0].sel = tmp0;
			alu.src[0].chan = 2;
			alu.src[1].sel = V_SQ_ALU_SRC_1_INT;

			alu.last = 1;
			if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
				return r;

			/* 16. tmp1.w = tmp0.z - 1			= q - 1 */
			memset(&alu, 0, sizeof(struct r600_bytecode_alu));
			alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_ADD_INT);

			alu.dst.sel = tmp1;
			alu.dst.chan = 3;
			alu.dst.write = 1;

			alu.src[0].sel = tmp0;
			alu.src[0].chan = 2;
			alu.src[1].sel = V_SQ_ALU_SRC_M_1_INT;

			alu.last = 1;
			if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
				return r;

		}

		/* 17. tmp1.x = tmp1.x & tmp1.y */
		memset(&alu, 0, sizeof(struct r600_bytecode_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_AND_INT);

		alu.dst.sel = tmp1;
		alu.dst.chan = 0;
		alu.dst.write = 1;

		alu.src[0].sel = tmp1;
		alu.src[0].chan = 0;
		alu.src[1].sel = tmp1;
		alu.src[1].chan = 1;

		alu.last = 1;
		if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
			return r;

		/* 18. tmp0.z = tmp1.x==0 ? tmp0.z : tmp1.z    DIV */
		/* 18. tmp0.z = tmp1.x==0 ? tmp0.w : tmp1.z    MOD */
		memset(&alu, 0, sizeof(struct r600_bytecode_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_CNDE_INT);
		alu.is_op3 = 1;

		alu.dst.sel = tmp0;
		alu.dst.chan = 2;
		alu.dst.write = 1;

		alu.src[0].sel = tmp1;
		alu.src[0].chan = 0;
		alu.src[1].sel = tmp0;
		alu.src[1].chan = mod ? 3 : 2;
		alu.src[2].sel = tmp1;
		alu.src[2].chan = 2;

		alu.last = 1;
		if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
			return r;

		/* 19. tmp0.z = tmp1.y==0 ? tmp1.w : tmp0.z */
		memset(&alu, 0, sizeof(struct r600_bytecode_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_CNDE_INT);
		alu.is_op3 = 1;

		if (signed_op) {
			alu.dst.sel = tmp0;
			alu.dst.chan = 2;
			alu.dst.write = 1;
		} else {
			tgsi_dst(ctx, &inst->Dst[0], i, &alu.dst);
		}

		alu.src[0].sel = tmp1;
		alu.src[0].chan = 1;
		alu.src[1].sel = tmp1;
		alu.src[1].chan = 3;
		alu.src[2].sel = tmp0;
		alu.src[2].chan = 2;

		alu.last = 1;
		if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
			return r;

		if (signed_op) {

			/* fix the sign of the result */

			if (mod) {

				/* tmp0.x = -tmp0.z */
				memset(&alu, 0, sizeof(struct r600_bytecode_alu));
				alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SUB_INT);

				alu.dst.sel = tmp0;
				alu.dst.chan = 0;
				alu.dst.write = 1;

				alu.src[0].sel = V_SQ_ALU_SRC_0;
				alu.src[1].sel = tmp0;
				alu.src[1].chan = 2;

				alu.last = 1;
				if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
					return r;

				/* sign of the remainder is the same as the sign of src0 */
				/* tmp0.x = src0>=0 ? tmp0.z : tmp0.x */
				memset(&alu, 0, sizeof(struct r600_bytecode_alu));
				alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_CNDGE_INT);
				alu.is_op3 = 1;

				tgsi_dst(ctx, &inst->Dst[0], i, &alu.dst);

				r600_bytecode_src(&alu.src[0], &ctx->src[0], i);
				alu.src[1].sel = tmp0;
				alu.src[1].chan = 2;
				alu.src[2].sel = tmp0;
				alu.src[2].chan = 0;

				alu.last = 1;
				if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
					return r;

			} else {

				/* tmp0.x = -tmp0.z */
				memset(&alu, 0, sizeof(struct r600_bytecode_alu));
				alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SUB_INT);

				alu.dst.sel = tmp0;
				alu.dst.chan = 0;
				alu.dst.write = 1;

				alu.src[0].sel = V_SQ_ALU_SRC_0;
				alu.src[1].sel = tmp0;
				alu.src[1].chan = 2;

				alu.last = 1;
				if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
					return r;

				/* fix the quotient sign (same as the sign of src0*src1) */
				/* tmp0.x = tmp2.z>=0 ? tmp0.z : tmp0.x */
				memset(&alu, 0, sizeof(struct r600_bytecode_alu));
				alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_CNDGE_INT);
				alu.is_op3 = 1;

				tgsi_dst(ctx, &inst->Dst[0], i, &alu.dst);

				alu.src[0].sel = tmp2;
				alu.src[0].chan = 2;
				alu.src[1].sel = tmp0;
				alu.src[1].chan = 2;
				alu.src[2].sel = tmp0;
				alu.src[2].chan = 0;

				alu.last = 1;
				if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
					return r;
			}
		}
	}
	return 0;
}

static int tgsi_udiv(struct r600_shader_ctx *ctx)
{
	return tgsi_divmod(ctx, 0, 0);
}

static int tgsi_umod(struct r600_shader_ctx *ctx)
{
	return tgsi_divmod(ctx, 1, 0);
}

static int tgsi_idiv(struct r600_shader_ctx *ctx)
{
	return tgsi_divmod(ctx, 0, 1);
}

static int tgsi_imod(struct r600_shader_ctx *ctx)
{
	return tgsi_divmod(ctx, 1, 1);
}


static int tgsi_f2i(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bytecode_alu alu;
	int i, r;
	unsigned write_mask = inst->Dst[0].Register.WriteMask;
	int last_inst = tgsi_last_instruction(write_mask);

	for (i = 0; i < 4; i++) {
		if (!(write_mask & (1<<i)))
			continue;

		memset(&alu, 0, sizeof(struct r600_bytecode_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_TRUNC);

		alu.dst.sel = ctx->temp_reg;
		alu.dst.chan = i;
		alu.dst.write = 1;

		r600_bytecode_src(&alu.src[0], &ctx->src[0], i);
		if (i == last_inst)
			alu.last = 1;
		r = r600_bytecode_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}

	for (i = 0; i < 4; i++) {
		if (!(write_mask & (1<<i)))
			continue;

		memset(&alu, 0, sizeof(struct r600_bytecode_alu));
		alu.inst = ctx->inst_info->r600_opcode;

		tgsi_dst(ctx, &inst->Dst[0], i, &alu.dst);

		alu.src[0].sel = ctx->temp_reg;
		alu.src[0].chan = i;

		if (i == last_inst || alu.inst == EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FLT_TO_UINT)
			alu.last = 1;
		r = r600_bytecode_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}

	return 0;
}

static int tgsi_iabs(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bytecode_alu alu;
	int i, r;
	unsigned write_mask = inst->Dst[0].Register.WriteMask;
	int last_inst = tgsi_last_instruction(write_mask);

	/* tmp = -src */
	for (i = 0; i < 4; i++) {
		if (!(write_mask & (1<<i)))
			continue;

		memset(&alu, 0, sizeof(struct r600_bytecode_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SUB_INT);

		alu.dst.sel = ctx->temp_reg;
		alu.dst.chan = i;
		alu.dst.write = 1;

		r600_bytecode_src(&alu.src[1], &ctx->src[0], i);
		alu.src[0].sel = V_SQ_ALU_SRC_0;

		if (i == last_inst)
			alu.last = 1;
		r = r600_bytecode_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}

	/* dst = (src >= 0 ? src : tmp) */
	for (i = 0; i < 4; i++) {
		if (!(write_mask & (1<<i)))
			continue;

		memset(&alu, 0, sizeof(struct r600_bytecode_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_CNDGE_INT);
		alu.is_op3 = 1;
		alu.dst.write = 1;

		tgsi_dst(ctx, &inst->Dst[0], i, &alu.dst);

		r600_bytecode_src(&alu.src[0], &ctx->src[0], i);
		r600_bytecode_src(&alu.src[1], &ctx->src[0], i);
		alu.src[2].sel = ctx->temp_reg;
		alu.src[2].chan = i;

		if (i == last_inst)
			alu.last = 1;
		r = r600_bytecode_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}
	return 0;
}

static int tgsi_issg(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bytecode_alu alu;
	int i, r;
	unsigned write_mask = inst->Dst[0].Register.WriteMask;
	int last_inst = tgsi_last_instruction(write_mask);

	/* tmp = (src >= 0 ? src : -1) */
	for (i = 0; i < 4; i++) {
		if (!(write_mask & (1<<i)))
			continue;

		memset(&alu, 0, sizeof(struct r600_bytecode_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_CNDGE_INT);
		alu.is_op3 = 1;

		alu.dst.sel = ctx->temp_reg;
		alu.dst.chan = i;
		alu.dst.write = 1;

		r600_bytecode_src(&alu.src[0], &ctx->src[0], i);
		r600_bytecode_src(&alu.src[1], &ctx->src[0], i);
		alu.src[2].sel = V_SQ_ALU_SRC_M_1_INT;

		if (i == last_inst)
			alu.last = 1;
		r = r600_bytecode_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}

	/* dst = (tmp > 0 ? 1 : tmp) */
	for (i = 0; i < 4; i++) {
		if (!(write_mask & (1<<i)))
			continue;

		memset(&alu, 0, sizeof(struct r600_bytecode_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_CNDGT_INT);
		alu.is_op3 = 1;
		alu.dst.write = 1;

		tgsi_dst(ctx, &inst->Dst[0], i, &alu.dst);

		alu.src[0].sel = ctx->temp_reg;
		alu.src[0].chan = i;

		alu.src[1].sel = V_SQ_ALU_SRC_1_INT;

		alu.src[2].sel = ctx->temp_reg;
		alu.src[2].chan = i;

		if (i == last_inst)
			alu.last = 1;
		r = r600_bytecode_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}
	return 0;
}



static int tgsi_ssg(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bytecode_alu alu;
	int i, r;

	/* tmp = (src > 0 ? 1 : src) */
	for (i = 0; i < 4; i++) {
		memset(&alu, 0, sizeof(struct r600_bytecode_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_CNDGT);
		alu.is_op3 = 1;

		alu.dst.sel = ctx->temp_reg;
		alu.dst.chan = i;

		r600_bytecode_src(&alu.src[0], &ctx->src[0], i);
		alu.src[1].sel = V_SQ_ALU_SRC_1;
		r600_bytecode_src(&alu.src[2], &ctx->src[0], i);

		if (i == 3)
			alu.last = 1;
		r = r600_bytecode_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}

	/* dst = (-tmp > 0 ? -1 : tmp) */
	for (i = 0; i < 4; i++) {
		memset(&alu, 0, sizeof(struct r600_bytecode_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_CNDGT);
		alu.is_op3 = 1;
		tgsi_dst(ctx, &inst->Dst[0], i, &alu.dst);

		alu.src[0].sel = ctx->temp_reg;
		alu.src[0].chan = i;
		alu.src[0].neg = 1;

		alu.src[1].sel = V_SQ_ALU_SRC_1;
		alu.src[1].neg = 1;

		alu.src[2].sel = ctx->temp_reg;
		alu.src[2].chan = i;

		if (i == 3)
			alu.last = 1;
		r = r600_bytecode_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}
	return 0;
}

static int tgsi_helper_copy(struct r600_shader_ctx *ctx, struct tgsi_full_instruction *inst)
{
	struct r600_bytecode_alu alu;
	int i, r;

	for (i = 0; i < 4; i++) {
		memset(&alu, 0, sizeof(struct r600_bytecode_alu));
		if (!(inst->Dst[0].Register.WriteMask & (1 << i))) {
			alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP);
			alu.dst.chan = i;
		} else {
			alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV);
			tgsi_dst(ctx, &inst->Dst[0], i, &alu.dst);
			alu.src[0].sel = ctx->temp_reg;
			alu.src[0].chan = i;
		}
		if (i == 3) {
			alu.last = 1;
		}
		r = r600_bytecode_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}
	return 0;
}

static int tgsi_op3(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bytecode_alu alu;
	int i, j, r;
	int lasti = tgsi_last_instruction(inst->Dst[0].Register.WriteMask);

	for (i = 0; i < lasti + 1; i++) {
		if (!(inst->Dst[0].Register.WriteMask & (1 << i)))
			continue;

		memset(&alu, 0, sizeof(struct r600_bytecode_alu));
		alu.inst = ctx->inst_info->r600_opcode;
		for (j = 0; j < inst->Instruction.NumSrcRegs; j++) {
			r600_bytecode_src(&alu.src[j], &ctx->src[j], i);
		}

		tgsi_dst(ctx, &inst->Dst[0], i, &alu.dst);
		alu.dst.chan = i;
		alu.dst.write = 1;
		alu.is_op3 = 1;
		if (i == lasti) {
			alu.last = 1;
		}
		r = r600_bytecode_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}
	return 0;
}

static int tgsi_dp(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bytecode_alu alu;
	int i, j, r;

	for (i = 0; i < 4; i++) {
		memset(&alu, 0, sizeof(struct r600_bytecode_alu));
		alu.inst = ctx->inst_info->r600_opcode;
		for (j = 0; j < inst->Instruction.NumSrcRegs; j++) {
			r600_bytecode_src(&alu.src[j], &ctx->src[j], i);
		}

		tgsi_dst(ctx, &inst->Dst[0], i, &alu.dst);
		alu.dst.chan = i;
		alu.dst.write = (inst->Dst[0].Register.WriteMask >> i) & 1;
		/* handle some special cases */
		switch (ctx->inst_info->tgsi_opcode) {
		case TGSI_OPCODE_DP2:
			if (i > 1) {
				alu.src[0].sel = alu.src[1].sel = V_SQ_ALU_SRC_0;
				alu.src[0].chan = alu.src[1].chan = 0;
			}
			break;
		case TGSI_OPCODE_DP3:
			if (i > 2) {
				alu.src[0].sel = alu.src[1].sel = V_SQ_ALU_SRC_0;
				alu.src[0].chan = alu.src[1].chan = 0;
			}
			break;
		case TGSI_OPCODE_DPH:
			if (i == 3) {
				alu.src[0].sel = V_SQ_ALU_SRC_1;
				alu.src[0].chan = 0;
				alu.src[0].neg = 0;
			}
			break;
		default:
			break;
		}
		if (i == 3) {
			alu.last = 1;
		}
		r = r600_bytecode_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}
	return 0;
}

static inline boolean tgsi_tex_src_requires_loading(struct r600_shader_ctx *ctx,
						    unsigned index)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	return 	(inst->Src[index].Register.File != TGSI_FILE_TEMPORARY &&
		inst->Src[index].Register.File != TGSI_FILE_INPUT &&
		inst->Src[index].Register.File != TGSI_FILE_OUTPUT) ||
		ctx->src[index].neg || ctx->src[index].abs;
}

static inline unsigned tgsi_tex_get_src_gpr(struct r600_shader_ctx *ctx,
					unsigned index)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	return ctx->file_offset[inst->Src[index].Register.File] + inst->Src[index].Register.Index;
}

static int tgsi_tex(struct r600_shader_ctx *ctx)
{
	static float one_point_five = 1.5f;
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bytecode_tex tex;
	struct r600_bytecode_alu alu;
	unsigned src_gpr;
	int r, i, j;
	int opcode;
	/* Texture fetch instructions can only use gprs as source.
	 * Also they cannot negate the source or take the absolute value */
	const boolean src_requires_loading = inst->Instruction.Opcode != TGSI_OPCODE_TXQ_LZ &&
                                             tgsi_tex_src_requires_loading(ctx, 0);
	boolean src_loaded = FALSE;
	unsigned sampler_src_reg = inst->Instruction.Opcode == TGSI_OPCODE_TXQ_LZ ? 0 : 1;
	uint8_t offset_x = 0, offset_y = 0, offset_z = 0;

	src_gpr = tgsi_tex_get_src_gpr(ctx, 0);

	if (inst->Instruction.Opcode == TGSI_OPCODE_TXF) {
		/* get offset values */
		if (inst->Texture.NumOffsets) {
			assert(inst->Texture.NumOffsets == 1);

			offset_x = ctx->literals[inst->TexOffsets[0].Index + inst->TexOffsets[0].SwizzleX] << 1;
			offset_y = ctx->literals[inst->TexOffsets[0].Index + inst->TexOffsets[0].SwizzleY] << 1;
			offset_z = ctx->literals[inst->TexOffsets[0].Index + inst->TexOffsets[0].SwizzleZ] << 1;
		}
	} else if (inst->Instruction.Opcode == TGSI_OPCODE_TXD) {
		/* TGSI moves the sampler to src reg 3 for TXD */
		sampler_src_reg = 3;

		for (i = 1; i < 3; i++) {
			/* set gradients h/v */
			memset(&tex, 0, sizeof(struct r600_bytecode_tex));
			tex.inst = (i == 1) ? SQ_TEX_INST_SET_GRADIENTS_H :
				SQ_TEX_INST_SET_GRADIENTS_V;
			tex.sampler_id = tgsi_tex_get_src_gpr(ctx, sampler_src_reg);
			tex.resource_id = tex.sampler_id + R600_MAX_CONST_BUFFERS;

			if (tgsi_tex_src_requires_loading(ctx, i)) {
				tex.src_gpr = r600_get_temp(ctx);
				tex.src_sel_x = 0;
				tex.src_sel_y = 1;
				tex.src_sel_z = 2;
				tex.src_sel_w = 3;

				for (j = 0; j < 4; j++) {
					memset(&alu, 0, sizeof(struct r600_bytecode_alu));
					alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV);
                                        r600_bytecode_src(&alu.src[0], &ctx->src[i], j);
                                        alu.dst.sel = tex.src_gpr;
                                        alu.dst.chan = j;
                                        if (j == 3)
                                                alu.last = 1;
                                        alu.dst.write = 1;
                                        r = r600_bytecode_add_alu(ctx->bc, &alu);
                                        if (r)
                                                return r;
				}

			} else {
				tex.src_gpr = tgsi_tex_get_src_gpr(ctx, i);
				tex.src_sel_x = ctx->src[i].swizzle[0];
				tex.src_sel_y = ctx->src[i].swizzle[1];
				tex.src_sel_z = ctx->src[i].swizzle[2];
				tex.src_sel_w = ctx->src[i].swizzle[3];
				tex.src_rel = ctx->src[i].rel;
			}
			tex.dst_gpr = ctx->temp_reg; /* just to avoid confusing the asm scheduler */
			tex.dst_sel_x = tex.dst_sel_y = tex.dst_sel_z = tex.dst_sel_w = 7;
			if (inst->Texture.Texture != TGSI_TEXTURE_RECT) {
				tex.coord_type_x = 1;
				tex.coord_type_y = 1;
				tex.coord_type_z = 1;
				tex.coord_type_w = 1;
			}
			r = r600_bytecode_add_tex(ctx->bc, &tex);
			if (r)
				return r;
		}
	} else if (inst->Instruction.Opcode == TGSI_OPCODE_TXP) {
		int out_chan;
		/* Add perspective divide */
		if (ctx->bc->chip_class == CAYMAN) {
			out_chan = 2;
			for (i = 0; i < 3; i++) {
				memset(&alu, 0, sizeof(struct r600_bytecode_alu));
				alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIP_IEEE);
				r600_bytecode_src(&alu.src[0], &ctx->src[0], 3);

				alu.dst.sel = ctx->temp_reg;
				alu.dst.chan = i;
				if (i == 2)
					alu.last = 1;
				if (out_chan == i)
					alu.dst.write = 1;
				r = r600_bytecode_add_alu(ctx->bc, &alu);
				if (r)
					return r;
			}

		} else {
			out_chan = 3;
			memset(&alu, 0, sizeof(struct r600_bytecode_alu));
			alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIP_IEEE);
			r600_bytecode_src(&alu.src[0], &ctx->src[0], 3);

			alu.dst.sel = ctx->temp_reg;
			alu.dst.chan = out_chan;
			alu.last = 1;
			alu.dst.write = 1;
			r = r600_bytecode_add_alu(ctx->bc, &alu);
			if (r)
				return r;
		}

		for (i = 0; i < 3; i++) {
			memset(&alu, 0, sizeof(struct r600_bytecode_alu));
			alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MUL);
			alu.src[0].sel = ctx->temp_reg;
			alu.src[0].chan = out_chan;
			r600_bytecode_src(&alu.src[1], &ctx->src[0], i);
			alu.dst.sel = ctx->temp_reg;
			alu.dst.chan = i;
			alu.dst.write = 1;
			r = r600_bytecode_add_alu(ctx->bc, &alu);
			if (r)
				return r;
		}
		memset(&alu, 0, sizeof(struct r600_bytecode_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV);
		alu.src[0].sel = V_SQ_ALU_SRC_1;
		alu.src[0].chan = 0;
		alu.dst.sel = ctx->temp_reg;
		alu.dst.chan = 3;
		alu.last = 1;
		alu.dst.write = 1;
		r = r600_bytecode_add_alu(ctx->bc, &alu);
		if (r)
			return r;
		src_loaded = TRUE;
		src_gpr = ctx->temp_reg;
	}

	if ((inst->Texture.Texture == TGSI_TEXTURE_CUBE ||
	     inst->Texture.Texture == TGSI_TEXTURE_SHADOWCUBE) &&
	    inst->Instruction.Opcode != TGSI_OPCODE_TXQ &&
	    inst->Instruction.Opcode != TGSI_OPCODE_TXQ_LZ) {

		static const unsigned src0_swizzle[] = {2, 2, 0, 1};
		static const unsigned src1_swizzle[] = {1, 0, 2, 2};

		/* tmp1.xyzw = CUBE(R0.zzxy, R0.yxzz) */
		for (i = 0; i < 4; i++) {
			memset(&alu, 0, sizeof(struct r600_bytecode_alu));
			alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_CUBE);
			r600_bytecode_src(&alu.src[0], &ctx->src[0], src0_swizzle[i]);
			r600_bytecode_src(&alu.src[1], &ctx->src[0], src1_swizzle[i]);
			alu.dst.sel = ctx->temp_reg;
			alu.dst.chan = i;
			if (i == 3)
				alu.last = 1;
			alu.dst.write = 1;
			r = r600_bytecode_add_alu(ctx->bc, &alu);
			if (r)
				return r;
		}

		/* tmp1.z = RCP_e(|tmp1.z|) */
		if (ctx->bc->chip_class == CAYMAN) {
			for (i = 0; i < 3; i++) {
				memset(&alu, 0, sizeof(struct r600_bytecode_alu));
				alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIP_IEEE);
				alu.src[0].sel = ctx->temp_reg;
				alu.src[0].chan = 2;
				alu.src[0].abs = 1;
				alu.dst.sel = ctx->temp_reg;
				alu.dst.chan = i;
				if (i == 2)
					alu.dst.write = 1;
				if (i == 2)
					alu.last = 1;
				r = r600_bytecode_add_alu(ctx->bc, &alu);
				if (r)
					return r;
			}
		} else {
			memset(&alu, 0, sizeof(struct r600_bytecode_alu));
			alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIP_IEEE);
			alu.src[0].sel = ctx->temp_reg;
			alu.src[0].chan = 2;
			alu.src[0].abs = 1;
			alu.dst.sel = ctx->temp_reg;
			alu.dst.chan = 2;
			alu.dst.write = 1;
			alu.last = 1;
			r = r600_bytecode_add_alu(ctx->bc, &alu);
			if (r)
				return r;
		}

		/* MULADD R0.x,  R0.x,  PS1,  (0x3FC00000, 1.5f).x
		 * MULADD R0.y,  R0.y,  PS1,  (0x3FC00000, 1.5f).x
		 * muladd has no writemask, have to use another temp
		 */
		memset(&alu, 0, sizeof(struct r600_bytecode_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_MULADD);
		alu.is_op3 = 1;

		alu.src[0].sel = ctx->temp_reg;
		alu.src[0].chan = 0;
		alu.src[1].sel = ctx->temp_reg;
		alu.src[1].chan = 2;

		alu.src[2].sel = V_SQ_ALU_SRC_LITERAL;
		alu.src[2].chan = 0;
		alu.src[2].value = *(uint32_t *)&one_point_five;

		alu.dst.sel = ctx->temp_reg;
		alu.dst.chan = 0;
		alu.dst.write = 1;

		r = r600_bytecode_add_alu(ctx->bc, &alu);
		if (r)
			return r;

		memset(&alu, 0, sizeof(struct r600_bytecode_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_MULADD);
		alu.is_op3 = 1;

		alu.src[0].sel = ctx->temp_reg;
		alu.src[0].chan = 1;
		alu.src[1].sel = ctx->temp_reg;
		alu.src[1].chan = 2;

		alu.src[2].sel = V_SQ_ALU_SRC_LITERAL;
		alu.src[2].chan = 0;
		alu.src[2].value = *(uint32_t *)&one_point_five;

		alu.dst.sel = ctx->temp_reg;
		alu.dst.chan = 1;
		alu.dst.write = 1;

		alu.last = 1;
		r = r600_bytecode_add_alu(ctx->bc, &alu);
		if (r)
			return r;
		/* write initial W value into Z component */
		if (inst->Texture.Texture == TGSI_TEXTURE_SHADOWCUBE) {
			memset(&alu, 0, sizeof(struct r600_bytecode_alu));
			alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV);
			r600_bytecode_src(&alu.src[0], &ctx->src[0], 3);
			alu.dst.sel = ctx->temp_reg;
			alu.dst.chan = 2;
			alu.dst.write = 1;
			alu.last = 1;
			r = r600_bytecode_add_alu(ctx->bc, &alu);
			if (r)
				return r;
		}

		/* for cube forms of lod and bias we need to route the lod
		   value into Z */
		if (inst->Instruction.Opcode == TGSI_OPCODE_TXB ||
		    inst->Instruction.Opcode == TGSI_OPCODE_TXL) {
			memset(&alu, 0, sizeof(struct r600_bytecode_alu));
			alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV);
			r600_bytecode_src(&alu.src[0], &ctx->src[0], 3);
			alu.dst.sel = ctx->temp_reg;
			alu.dst.chan = 2;
			alu.last = 1;
			alu.dst.write = 1;
			r = r600_bytecode_add_alu(ctx->bc, &alu);
			if (r)
				return r;
		}

		src_loaded = TRUE;
		src_gpr = ctx->temp_reg;
	}

	if (src_requires_loading && !src_loaded) {
		for (i = 0; i < 4; i++) {
			memset(&alu, 0, sizeof(struct r600_bytecode_alu));
			alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV);
			r600_bytecode_src(&alu.src[0], &ctx->src[0], i);
			alu.dst.sel = ctx->temp_reg;
			alu.dst.chan = i;
			if (i == 3)
				alu.last = 1;
			alu.dst.write = 1;
			r = r600_bytecode_add_alu(ctx->bc, &alu);
			if (r)
				return r;
		}
		src_loaded = TRUE;
		src_gpr = ctx->temp_reg;
	}

	opcode = ctx->inst_info->r600_opcode;
	if (inst->Texture.Texture == TGSI_TEXTURE_SHADOW1D ||
	    inst->Texture.Texture == TGSI_TEXTURE_SHADOW2D ||
	    inst->Texture.Texture == TGSI_TEXTURE_SHADOWRECT ||
	    inst->Texture.Texture == TGSI_TEXTURE_SHADOWCUBE ||
	    inst->Texture.Texture == TGSI_TEXTURE_SHADOW1D_ARRAY ||
	    inst->Texture.Texture == TGSI_TEXTURE_SHADOW2D_ARRAY) {
		switch (opcode) {
		case SQ_TEX_INST_SAMPLE:
			opcode = SQ_TEX_INST_SAMPLE_C;
			break;
		case SQ_TEX_INST_SAMPLE_L:
			opcode = SQ_TEX_INST_SAMPLE_C_L;
			break;
		case SQ_TEX_INST_SAMPLE_LB:
			opcode = SQ_TEX_INST_SAMPLE_C_LB;
			break;
		case SQ_TEX_INST_SAMPLE_G:
			opcode = SQ_TEX_INST_SAMPLE_C_G;
			break;
		}
	}

	memset(&tex, 0, sizeof(struct r600_bytecode_tex));
	tex.inst = opcode;

	tex.sampler_id = tgsi_tex_get_src_gpr(ctx, sampler_src_reg);
	tex.resource_id = tex.sampler_id + R600_MAX_CONST_BUFFERS;
	tex.src_gpr = src_gpr;
	tex.dst_gpr = ctx->file_offset[inst->Dst[0].Register.File] + inst->Dst[0].Register.Index;
	tex.dst_sel_x = (inst->Dst[0].Register.WriteMask & 1) ? 0 : 7;
	tex.dst_sel_y = (inst->Dst[0].Register.WriteMask & 2) ? 1 : 7;
	tex.dst_sel_z = (inst->Dst[0].Register.WriteMask & 4) ? 2 : 7;
	tex.dst_sel_w = (inst->Dst[0].Register.WriteMask & 8) ? 3 : 7;

	if (inst->Instruction.Opcode == TGSI_OPCODE_TXQ_LZ) {
		tex.src_sel_x = 4;
		tex.src_sel_y = 4;
		tex.src_sel_z = 4;
		tex.src_sel_w = 4;
	} else if (src_loaded) {
		tex.src_sel_x = 0;
		tex.src_sel_y = 1;
		tex.src_sel_z = 2;
		tex.src_sel_w = 3;
	} else {
		tex.src_sel_x = ctx->src[0].swizzle[0];
		tex.src_sel_y = ctx->src[0].swizzle[1];
		tex.src_sel_z = ctx->src[0].swizzle[2];
		tex.src_sel_w = ctx->src[0].swizzle[3];
		tex.src_rel = ctx->src[0].rel;
	}

	if (inst->Texture.Texture == TGSI_TEXTURE_CUBE ||
	    inst->Texture.Texture == TGSI_TEXTURE_SHADOWCUBE) {
		tex.src_sel_x = 1;
		tex.src_sel_y = 0;
		tex.src_sel_z = 3;
		tex.src_sel_w = 2; /* route Z compare or Lod value into W */
	}

	if (inst->Texture.Texture != TGSI_TEXTURE_RECT &&
	    inst->Texture.Texture != TGSI_TEXTURE_SHADOWRECT) {
		tex.coord_type_x = 1;
		tex.coord_type_y = 1;
	}
	tex.coord_type_z = 1;
	tex.coord_type_w = 1;

	tex.offset_x = offset_x;
	tex.offset_y = offset_y;
	tex.offset_z = offset_z;

	/* Put the depth for comparison in W.
	 * TGSI_TEXTURE_SHADOW2D_ARRAY already has the depth in W.
	 * Some instructions expect the depth in Z. */
	if ((inst->Texture.Texture == TGSI_TEXTURE_SHADOW1D ||
	     inst->Texture.Texture == TGSI_TEXTURE_SHADOW2D ||
	     inst->Texture.Texture == TGSI_TEXTURE_SHADOWRECT ||
	     inst->Texture.Texture == TGSI_TEXTURE_SHADOW1D_ARRAY) &&
	    opcode != SQ_TEX_INST_SAMPLE_C_L &&
	    opcode != SQ_TEX_INST_SAMPLE_C_LB) {
		tex.src_sel_w = tex.src_sel_z;
	}

	if (inst->Texture.Texture == TGSI_TEXTURE_1D_ARRAY ||
	    inst->Texture.Texture == TGSI_TEXTURE_SHADOW1D_ARRAY) {
		if (opcode == SQ_TEX_INST_SAMPLE_C_L ||
		    opcode == SQ_TEX_INST_SAMPLE_C_LB) {
			/* the array index is read from Y */
			tex.coord_type_y = 0;
		} else {
			/* the array index is read from Z */
			tex.coord_type_z = 0;
			tex.src_sel_z = tex.src_sel_y;
		}
	} else if (inst->Texture.Texture == TGSI_TEXTURE_2D_ARRAY ||
		   inst->Texture.Texture == TGSI_TEXTURE_SHADOW2D_ARRAY)
		/* the array index is read from Z */
		tex.coord_type_z = 0;

	r = r600_bytecode_add_tex(ctx->bc, &tex);
	if (r)
		return r;

	/* add shadow ambient support  - gallium doesn't do it yet */
	return 0;
}

static int tgsi_lrp(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bytecode_alu alu;
	int lasti = tgsi_last_instruction(inst->Dst[0].Register.WriteMask);
	unsigned i;
	int r;

	/* optimize if it's just an equal balance */
	if (ctx->src[0].sel == V_SQ_ALU_SRC_0_5) {
		for (i = 0; i < lasti + 1; i++) {
			if (!(inst->Dst[0].Register.WriteMask & (1 << i)))
				continue;

			memset(&alu, 0, sizeof(struct r600_bytecode_alu));
			alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_ADD);
			r600_bytecode_src(&alu.src[0], &ctx->src[1], i);
			r600_bytecode_src(&alu.src[1], &ctx->src[2], i);
			alu.omod = 3;
			tgsi_dst(ctx, &inst->Dst[0], i, &alu.dst);
			alu.dst.chan = i;
			if (i == lasti) {
				alu.last = 1;
			}
			r = r600_bytecode_add_alu(ctx->bc, &alu);
			if (r)
				return r;
		}
		return 0;
	}

	/* 1 - src0 */
	for (i = 0; i < lasti + 1; i++) {
		if (!(inst->Dst[0].Register.WriteMask & (1 << i)))
			continue;

		memset(&alu, 0, sizeof(struct r600_bytecode_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_ADD);
		alu.src[0].sel = V_SQ_ALU_SRC_1;
		alu.src[0].chan = 0;
		r600_bytecode_src(&alu.src[1], &ctx->src[0], i);
		r600_bytecode_src_toggle_neg(&alu.src[1]);
		alu.dst.sel = ctx->temp_reg;
		alu.dst.chan = i;
		if (i == lasti) {
			alu.last = 1;
		}
		alu.dst.write = 1;
		r = r600_bytecode_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}

	/* (1 - src0) * src2 */
	for (i = 0; i < lasti + 1; i++) {
		if (!(inst->Dst[0].Register.WriteMask & (1 << i)))
			continue;

		memset(&alu, 0, sizeof(struct r600_bytecode_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MUL);
		alu.src[0].sel = ctx->temp_reg;
		alu.src[0].chan = i;
		r600_bytecode_src(&alu.src[1], &ctx->src[2], i);
		alu.dst.sel = ctx->temp_reg;
		alu.dst.chan = i;
		if (i == lasti) {
			alu.last = 1;
		}
		alu.dst.write = 1;
		r = r600_bytecode_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}

	/* src0 * src1 + (1 - src0) * src2 */
	for (i = 0; i < lasti + 1; i++) {
		if (!(inst->Dst[0].Register.WriteMask & (1 << i)))
			continue;

		memset(&alu, 0, sizeof(struct r600_bytecode_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_MULADD);
		alu.is_op3 = 1;
		r600_bytecode_src(&alu.src[0], &ctx->src[0], i);
		r600_bytecode_src(&alu.src[1], &ctx->src[1], i);
		alu.src[2].sel = ctx->temp_reg;
		alu.src[2].chan = i;

		tgsi_dst(ctx, &inst->Dst[0], i, &alu.dst);
		alu.dst.chan = i;
		if (i == lasti) {
			alu.last = 1;
		}
		r = r600_bytecode_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}
	return 0;
}

static int tgsi_cmp(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bytecode_alu alu;
	int i, r;
	int lasti = tgsi_last_instruction(inst->Dst[0].Register.WriteMask);

	for (i = 0; i < lasti + 1; i++) {
		if (!(inst->Dst[0].Register.WriteMask & (1 << i)))
			continue;

		memset(&alu, 0, sizeof(struct r600_bytecode_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_CNDGE);
		r600_bytecode_src(&alu.src[0], &ctx->src[0], i);
		r600_bytecode_src(&alu.src[1], &ctx->src[2], i);
		r600_bytecode_src(&alu.src[2], &ctx->src[1], i);
		tgsi_dst(ctx, &inst->Dst[0], i, &alu.dst);
		alu.dst.chan = i;
		alu.dst.write = 1;
		alu.is_op3 = 1;
		if (i == lasti)
			alu.last = 1;
		r = r600_bytecode_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}
	return 0;
}

static int tgsi_xpd(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	static const unsigned int src0_swizzle[] = {2, 0, 1};
	static const unsigned int src1_swizzle[] = {1, 2, 0};
	struct r600_bytecode_alu alu;
	uint32_t use_temp = 0;
	int i, r;

	if (inst->Dst[0].Register.WriteMask != 0xf)
		use_temp = 1;

	for (i = 0; i < 4; i++) {
		memset(&alu, 0, sizeof(struct r600_bytecode_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MUL);
		if (i < 3) {
			r600_bytecode_src(&alu.src[0], &ctx->src[0], src0_swizzle[i]);
			r600_bytecode_src(&alu.src[1], &ctx->src[1], src1_swizzle[i]);
		} else {
			alu.src[0].sel = V_SQ_ALU_SRC_0;
			alu.src[0].chan = i;
			alu.src[1].sel = V_SQ_ALU_SRC_0;
			alu.src[1].chan = i;
		}

		alu.dst.sel = ctx->temp_reg;
		alu.dst.chan = i;
		alu.dst.write = 1;

		if (i == 3)
			alu.last = 1;
		r = r600_bytecode_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}

	for (i = 0; i < 4; i++) {
		memset(&alu, 0, sizeof(struct r600_bytecode_alu));
		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_MULADD);

		if (i < 3) {
			r600_bytecode_src(&alu.src[0], &ctx->src[0], src1_swizzle[i]);
			r600_bytecode_src(&alu.src[1], &ctx->src[1], src0_swizzle[i]);
		} else {
			alu.src[0].sel = V_SQ_ALU_SRC_0;
			alu.src[0].chan = i;
			alu.src[1].sel = V_SQ_ALU_SRC_0;
			alu.src[1].chan = i;
		}

		alu.src[2].sel = ctx->temp_reg;
		alu.src[2].neg = 1;
		alu.src[2].chan = i;

		if (use_temp)
			alu.dst.sel = ctx->temp_reg;
		else
			tgsi_dst(ctx, &inst->Dst[0], i, &alu.dst);
		alu.dst.chan = i;
		alu.dst.write = 1;
		alu.is_op3 = 1;
		if (i == 3)
			alu.last = 1;
		r = r600_bytecode_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}
	if (use_temp)
		return tgsi_helper_copy(ctx, inst);
	return 0;
}

static int tgsi_exp(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bytecode_alu alu;
	int r;
	int i;

	/* result.x = 2^floor(src); */
	if (inst->Dst[0].Register.WriteMask & 1) {
		memset(&alu, 0, sizeof(struct r600_bytecode_alu));

		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FLOOR);
		r600_bytecode_src(&alu.src[0], &ctx->src[0], 0);

		alu.dst.sel = ctx->temp_reg;
		alu.dst.chan = 0;
		alu.dst.write = 1;
		alu.last = 1;
		r = r600_bytecode_add_alu(ctx->bc, &alu);
		if (r)
			return r;

		if (ctx->bc->chip_class == CAYMAN) {
			for (i = 0; i < 3; i++) {
				alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_EXP_IEEE);
				alu.src[0].sel = ctx->temp_reg;
				alu.src[0].chan = 0;

				alu.dst.sel = ctx->temp_reg;
				alu.dst.chan = i;
				alu.dst.write = i == 0;
				alu.last = i == 2;
				r = r600_bytecode_add_alu(ctx->bc, &alu);
				if (r)
					return r;
			}
		} else {
			alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_EXP_IEEE);
			alu.src[0].sel = ctx->temp_reg;
			alu.src[0].chan = 0;

			alu.dst.sel = ctx->temp_reg;
			alu.dst.chan = 0;
			alu.dst.write = 1;
			alu.last = 1;
			r = r600_bytecode_add_alu(ctx->bc, &alu);
			if (r)
				return r;
		}
	}

	/* result.y = tmp - floor(tmp); */
	if ((inst->Dst[0].Register.WriteMask >> 1) & 1) {
		memset(&alu, 0, sizeof(struct r600_bytecode_alu));

		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FRACT);
		r600_bytecode_src(&alu.src[0], &ctx->src[0], 0);

		alu.dst.sel = ctx->temp_reg;
#if 0
		r = tgsi_dst(ctx, &inst->Dst[0], i, &alu.dst);
		if (r)
			return r;
#endif
		alu.dst.write = 1;
		alu.dst.chan = 1;

		alu.last = 1;

		r = r600_bytecode_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}

	/* result.z = RoughApprox2ToX(tmp);*/
	if ((inst->Dst[0].Register.WriteMask >> 2) & 0x1) {
		if (ctx->bc->chip_class == CAYMAN) {
			for (i = 0; i < 3; i++) {
				memset(&alu, 0, sizeof(struct r600_bytecode_alu));
				alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_EXP_IEEE);
				r600_bytecode_src(&alu.src[0], &ctx->src[0], 0);

				alu.dst.sel = ctx->temp_reg;
				alu.dst.chan = i;
				if (i == 2) {
					alu.dst.write = 1;
					alu.last = 1;
				}

				r = r600_bytecode_add_alu(ctx->bc, &alu);
				if (r)
					return r;
			}
		} else {
			memset(&alu, 0, sizeof(struct r600_bytecode_alu));
			alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_EXP_IEEE);
			r600_bytecode_src(&alu.src[0], &ctx->src[0], 0);

			alu.dst.sel = ctx->temp_reg;
			alu.dst.write = 1;
			alu.dst.chan = 2;

			alu.last = 1;

			r = r600_bytecode_add_alu(ctx->bc, &alu);
			if (r)
				return r;
		}
	}

	/* result.w = 1.0;*/
	if ((inst->Dst[0].Register.WriteMask >> 3) & 0x1) {
		memset(&alu, 0, sizeof(struct r600_bytecode_alu));

		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV);
		alu.src[0].sel = V_SQ_ALU_SRC_1;
		alu.src[0].chan = 0;

		alu.dst.sel = ctx->temp_reg;
		alu.dst.chan = 3;
		alu.dst.write = 1;
		alu.last = 1;
		r = r600_bytecode_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}
	return tgsi_helper_copy(ctx, inst);
}

static int tgsi_log(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bytecode_alu alu;
	int r;
	int i;

	/* result.x = floor(log2(|src|)); */
	if (inst->Dst[0].Register.WriteMask & 1) {
		if (ctx->bc->chip_class == CAYMAN) {
			for (i = 0; i < 3; i++) {
				memset(&alu, 0, sizeof(struct r600_bytecode_alu));

				alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LOG_IEEE);
				r600_bytecode_src(&alu.src[0], &ctx->src[0], 0);
				r600_bytecode_src_set_abs(&alu.src[0]);
			
				alu.dst.sel = ctx->temp_reg;
				alu.dst.chan = i;
				if (i == 0) 
					alu.dst.write = 1;
				if (i == 2)
					alu.last = 1;
				r = r600_bytecode_add_alu(ctx->bc, &alu);
				if (r)
					return r;
			}

		} else {
			memset(&alu, 0, sizeof(struct r600_bytecode_alu));

			alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LOG_IEEE);
			r600_bytecode_src(&alu.src[0], &ctx->src[0], 0);
			r600_bytecode_src_set_abs(&alu.src[0]);
			
			alu.dst.sel = ctx->temp_reg;
			alu.dst.chan = 0;
			alu.dst.write = 1;
			alu.last = 1;
			r = r600_bytecode_add_alu(ctx->bc, &alu);
			if (r)
				return r;
		}

		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FLOOR);
		alu.src[0].sel = ctx->temp_reg;
		alu.src[0].chan = 0;

		alu.dst.sel = ctx->temp_reg;
		alu.dst.chan = 0;
		alu.dst.write = 1;
		alu.last = 1;

		r = r600_bytecode_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}

	/* result.y = |src.x| / (2 ^ floor(log2(|src.x|))); */
	if ((inst->Dst[0].Register.WriteMask >> 1) & 1) {

		if (ctx->bc->chip_class == CAYMAN) {
			for (i = 0; i < 3; i++) {
				memset(&alu, 0, sizeof(struct r600_bytecode_alu));

				alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LOG_IEEE);
				r600_bytecode_src(&alu.src[0], &ctx->src[0], 0);
				r600_bytecode_src_set_abs(&alu.src[0]);

				alu.dst.sel = ctx->temp_reg;
				alu.dst.chan = i;
				if (i == 1)
					alu.dst.write = 1;
				if (i == 2)
					alu.last = 1;
				
				r = r600_bytecode_add_alu(ctx->bc, &alu);
				if (r)
					return r;	
			}
		} else {
			memset(&alu, 0, sizeof(struct r600_bytecode_alu));

			alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LOG_IEEE);
			r600_bytecode_src(&alu.src[0], &ctx->src[0], 0);
			r600_bytecode_src_set_abs(&alu.src[0]);

			alu.dst.sel = ctx->temp_reg;
			alu.dst.chan = 1;
			alu.dst.write = 1;
			alu.last = 1;

			r = r600_bytecode_add_alu(ctx->bc, &alu);
			if (r)
				return r;
		}

		memset(&alu, 0, sizeof(struct r600_bytecode_alu));

		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FLOOR);
		alu.src[0].sel = ctx->temp_reg;
		alu.src[0].chan = 1;

		alu.dst.sel = ctx->temp_reg;
		alu.dst.chan = 1;
		alu.dst.write = 1;
		alu.last = 1;

		r = r600_bytecode_add_alu(ctx->bc, &alu);
		if (r)
			return r;

		if (ctx->bc->chip_class == CAYMAN) {
			for (i = 0; i < 3; i++) {
				memset(&alu, 0, sizeof(struct r600_bytecode_alu));
				alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_EXP_IEEE);
				alu.src[0].sel = ctx->temp_reg;
				alu.src[0].chan = 1;

				alu.dst.sel = ctx->temp_reg;
				alu.dst.chan = i;
				if (i == 1)
					alu.dst.write = 1;
				if (i == 2)
					alu.last = 1;

				r = r600_bytecode_add_alu(ctx->bc, &alu);
				if (r)
					return r;
			}
		} else {
			memset(&alu, 0, sizeof(struct r600_bytecode_alu));
			alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_EXP_IEEE);
			alu.src[0].sel = ctx->temp_reg;
			alu.src[0].chan = 1;

			alu.dst.sel = ctx->temp_reg;
			alu.dst.chan = 1;
			alu.dst.write = 1;
			alu.last = 1;

			r = r600_bytecode_add_alu(ctx->bc, &alu);
			if (r)
				return r;
		}

		if (ctx->bc->chip_class == CAYMAN) {
			for (i = 0; i < 3; i++) {
				memset(&alu, 0, sizeof(struct r600_bytecode_alu));
				alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIP_IEEE);
				alu.src[0].sel = ctx->temp_reg;
				alu.src[0].chan = 1;

				alu.dst.sel = ctx->temp_reg;
				alu.dst.chan = i;
				if (i == 1)
					alu.dst.write = 1;
				if (i == 2)
					alu.last = 1;
				
				r = r600_bytecode_add_alu(ctx->bc, &alu);
				if (r)
					return r;
			}
		} else {
			memset(&alu, 0, sizeof(struct r600_bytecode_alu));
			alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIP_IEEE);
			alu.src[0].sel = ctx->temp_reg;
			alu.src[0].chan = 1;

			alu.dst.sel = ctx->temp_reg;
			alu.dst.chan = 1;
			alu.dst.write = 1;
			alu.last = 1;

			r = r600_bytecode_add_alu(ctx->bc, &alu);
			if (r)
				return r;
		}

		memset(&alu, 0, sizeof(struct r600_bytecode_alu));

		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MUL);

		r600_bytecode_src(&alu.src[0], &ctx->src[0], 0);
		r600_bytecode_src_set_abs(&alu.src[0]);

		alu.src[1].sel = ctx->temp_reg;
		alu.src[1].chan = 1;

		alu.dst.sel = ctx->temp_reg;
		alu.dst.chan = 1;
		alu.dst.write = 1;
		alu.last = 1;

		r = r600_bytecode_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}

	/* result.z = log2(|src|);*/
	if ((inst->Dst[0].Register.WriteMask >> 2) & 1) {
		if (ctx->bc->chip_class == CAYMAN) {
			for (i = 0; i < 3; i++) {
				memset(&alu, 0, sizeof(struct r600_bytecode_alu));

				alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LOG_IEEE);
				r600_bytecode_src(&alu.src[0], &ctx->src[0], 0);
				r600_bytecode_src_set_abs(&alu.src[0]);

				alu.dst.sel = ctx->temp_reg;
				if (i == 2)
					alu.dst.write = 1;
				alu.dst.chan = i;
				if (i == 2)
					alu.last = 1;

				r = r600_bytecode_add_alu(ctx->bc, &alu);
				if (r)
					return r;
			}
		} else {
			memset(&alu, 0, sizeof(struct r600_bytecode_alu));

			alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LOG_IEEE);
			r600_bytecode_src(&alu.src[0], &ctx->src[0], 0);
			r600_bytecode_src_set_abs(&alu.src[0]);

			alu.dst.sel = ctx->temp_reg;
			alu.dst.write = 1;
			alu.dst.chan = 2;
			alu.last = 1;

			r = r600_bytecode_add_alu(ctx->bc, &alu);
			if (r)
				return r;
		}
	}

	/* result.w = 1.0; */
	if ((inst->Dst[0].Register.WriteMask >> 3) & 1) {
		memset(&alu, 0, sizeof(struct r600_bytecode_alu));

		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV);
		alu.src[0].sel = V_SQ_ALU_SRC_1;
		alu.src[0].chan = 0;

		alu.dst.sel = ctx->temp_reg;
		alu.dst.chan = 3;
		alu.dst.write = 1;
		alu.last = 1;

		r = r600_bytecode_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}

	return tgsi_helper_copy(ctx, inst);
}

static int tgsi_eg_arl(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bytecode_alu alu;
	int r;

	memset(&alu, 0, sizeof(struct r600_bytecode_alu));

	switch (inst->Instruction.Opcode) {
	case TGSI_OPCODE_ARL:
		alu.inst = EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FLT_TO_INT_FLOOR;
		break;
	case TGSI_OPCODE_ARR:
		alu.inst = EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FLT_TO_INT;
		break;
	case TGSI_OPCODE_UARL:
		alu.inst = EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV;
		break;
	default:
		assert(0);
		return -1;
	}

	r600_bytecode_src(&alu.src[0], &ctx->src[0], 0);
	alu.last = 1;
	alu.dst.sel = ctx->bc->ar_reg;
	alu.dst.write = 1;
	r = r600_bytecode_add_alu(ctx->bc, &alu);
	if (r)
		return r;

	ctx->bc->ar_loaded = 0;
	return 0;
}
static int tgsi_r600_arl(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bytecode_alu alu;
	int r;

	switch (inst->Instruction.Opcode) {
	case TGSI_OPCODE_ARL:
		memset(&alu, 0, sizeof(alu));
		alu.inst = V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FLOOR;
		r600_bytecode_src(&alu.src[0], &ctx->src[0], 0);
		alu.dst.sel = ctx->bc->ar_reg;
		alu.dst.write = 1;
		alu.last = 1;

		if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
			return r;

		memset(&alu, 0, sizeof(alu));
		alu.inst = V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FLT_TO_INT;
		alu.src[0].sel = ctx->bc->ar_reg;
		alu.dst.sel = ctx->bc->ar_reg;
		alu.dst.write = 1;
		alu.last = 1;

		if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
			return r;
		break;
	case TGSI_OPCODE_ARR:
		memset(&alu, 0, sizeof(alu));
		alu.inst = V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FLT_TO_INT;
		r600_bytecode_src(&alu.src[0], &ctx->src[0], 0);
		alu.dst.sel = ctx->bc->ar_reg;
		alu.dst.write = 1;
		alu.last = 1;

		if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
			return r;
		break;
	case TGSI_OPCODE_UARL:
		memset(&alu, 0, sizeof(alu));
		alu.inst = V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV;
		r600_bytecode_src(&alu.src[0], &ctx->src[0], 0);
		alu.dst.sel = ctx->bc->ar_reg;
		alu.dst.write = 1;
		alu.last = 1;

		if ((r = r600_bytecode_add_alu(ctx->bc, &alu)))
			return r;
		break;
	default:
		assert(0);
		return -1;
	}

	ctx->bc->ar_loaded = 0;
	return 0;
}

static int tgsi_opdst(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bytecode_alu alu;
	int i, r = 0;

	for (i = 0; i < 4; i++) {
		memset(&alu, 0, sizeof(struct r600_bytecode_alu));

		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MUL);
		tgsi_dst(ctx, &inst->Dst[0], i, &alu.dst);

		if (i == 0 || i == 3) {
			alu.src[0].sel = V_SQ_ALU_SRC_1;
		} else {
			r600_bytecode_src(&alu.src[0], &ctx->src[0], i);
		}

		if (i == 0 || i == 2) {
			alu.src[1].sel = V_SQ_ALU_SRC_1;
		} else {
			r600_bytecode_src(&alu.src[1], &ctx->src[1], i);
		}
		if (i == 3)
			alu.last = 1;
		r = r600_bytecode_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}
	return 0;
}

static int emit_logic_pred(struct r600_shader_ctx *ctx, int opcode)
{
	struct r600_bytecode_alu alu;
	int r;

	memset(&alu, 0, sizeof(struct r600_bytecode_alu));
	alu.inst = opcode;
	alu.execute_mask = 1;
	alu.update_pred = 1;

	alu.dst.sel = ctx->temp_reg;
	alu.dst.write = 1;
	alu.dst.chan = 0;

	r600_bytecode_src(&alu.src[0], &ctx->src[0], 0);
	alu.src[1].sel = V_SQ_ALU_SRC_0;
	alu.src[1].chan = 0;

	alu.last = 1;

	r = r600_bytecode_add_alu_type(ctx->bc, &alu, CTX_INST(V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU_PUSH_BEFORE));
	if (r)
		return r;
	return 0;
}

static int pops(struct r600_shader_ctx *ctx, int pops)
{
	unsigned force_pop = ctx->bc->force_add_cf;

	if (!force_pop) {
		int alu_pop = 3;
		if (ctx->bc->cf_last) {
			if (ctx->bc->cf_last->inst == CTX_INST(V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU))
				alu_pop = 0;
			else if (ctx->bc->cf_last->inst == CTX_INST(V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU_POP_AFTER))
				alu_pop = 1;
		}
		alu_pop += pops;
		if (alu_pop == 1) {
			ctx->bc->cf_last->inst = CTX_INST(V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU_POP_AFTER);
			ctx->bc->force_add_cf = 1;
		} else if (alu_pop == 2) {
			ctx->bc->cf_last->inst = CTX_INST(V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU_POP2_AFTER);
			ctx->bc->force_add_cf = 1;
		} else {
			force_pop = 1;
		}
	}

	if (force_pop) {
		r600_bytecode_add_cfinst(ctx->bc, CTX_INST(V_SQ_CF_WORD1_SQ_CF_INST_POP));
		ctx->bc->cf_last->pop_count = pops;
		ctx->bc->cf_last->cf_addr = ctx->bc->cf_last->id + 2;
	}

	return 0;
}

static inline void callstack_decrease_current(struct r600_shader_ctx *ctx, unsigned reason)
{
	switch(reason) {
	case FC_PUSH_VPM:
		ctx->bc->callstack[ctx->bc->call_sp].current--;
		break;
	case FC_PUSH_WQM:
	case FC_LOOP:
		ctx->bc->callstack[ctx->bc->call_sp].current -= 4;
		break;
	case FC_REP:
		/* TOODO : for 16 vp asic should -= 2; */
		ctx->bc->callstack[ctx->bc->call_sp].current --;
		break;
	}
}

static inline void callstack_check_depth(struct r600_shader_ctx *ctx, unsigned reason, unsigned check_max_only)
{
	if (check_max_only) {
		int diff;
		switch (reason) {
		case FC_PUSH_VPM:
			diff = 1;
			break;
		case FC_PUSH_WQM:
			diff = 4;
			break;
		default:
			assert(0);
			diff = 0;
		}
		if ((ctx->bc->callstack[ctx->bc->call_sp].current + diff) >
		    ctx->bc->callstack[ctx->bc->call_sp].max) {
			ctx->bc->callstack[ctx->bc->call_sp].max =
				ctx->bc->callstack[ctx->bc->call_sp].current + diff;
		}
		return;
	}
	switch (reason) {
	case FC_PUSH_VPM:
		ctx->bc->callstack[ctx->bc->call_sp].current++;
		break;
	case FC_PUSH_WQM:
	case FC_LOOP:
		ctx->bc->callstack[ctx->bc->call_sp].current += 4;
		break;
	case FC_REP:
		ctx->bc->callstack[ctx->bc->call_sp].current++;
		break;
	}

	if ((ctx->bc->callstack[ctx->bc->call_sp].current) >
	    ctx->bc->callstack[ctx->bc->call_sp].max) {
		ctx->bc->callstack[ctx->bc->call_sp].max =
			ctx->bc->callstack[ctx->bc->call_sp].current;
	}
}

static void fc_set_mid(struct r600_shader_ctx *ctx, int fc_sp)
{
	struct r600_cf_stack_entry *sp = &ctx->bc->fc_stack[fc_sp];

	sp->mid = (struct r600_bytecode_cf **)realloc((void *)sp->mid,
						sizeof(struct r600_bytecode_cf *) * (sp->num_mid + 1));
	sp->mid[sp->num_mid] = ctx->bc->cf_last;
	sp->num_mid++;
}

static void fc_pushlevel(struct r600_shader_ctx *ctx, int type)
{
	ctx->bc->fc_sp++;
	ctx->bc->fc_stack[ctx->bc->fc_sp].type = type;
	ctx->bc->fc_stack[ctx->bc->fc_sp].start = ctx->bc->cf_last;
}

static void fc_poplevel(struct r600_shader_ctx *ctx)
{
	struct r600_cf_stack_entry *sp = &ctx->bc->fc_stack[ctx->bc->fc_sp];
	if (sp->mid) {
		free(sp->mid);
		sp->mid = NULL;
	}
	sp->num_mid = 0;
	sp->start = NULL;
	sp->type = 0;
	ctx->bc->fc_sp--;
}

#if 0
static int emit_return(struct r600_shader_ctx *ctx)
{
	r600_bytecode_add_cfinst(ctx->bc, CTX_INST(V_SQ_CF_WORD1_SQ_CF_INST_RETURN));
	return 0;
}

static int emit_jump_to_offset(struct r600_shader_ctx *ctx, int pops, int offset)
{

	r600_bytecode_add_cfinst(ctx->bc, CTX_INST(V_SQ_CF_WORD1_SQ_CF_INST_JUMP));
	ctx->bc->cf_last->pop_count = pops;
	/* XXX work out offset */
	return 0;
}

static int emit_setret_in_loop_flag(struct r600_shader_ctx *ctx, unsigned flag_value)
{
	return 0;
}

static void emit_testflag(struct r600_shader_ctx *ctx)
{

}

static void emit_return_on_flag(struct r600_shader_ctx *ctx, unsigned ifidx)
{
	emit_testflag(ctx);
	emit_jump_to_offset(ctx, 1, 4);
	emit_setret_in_loop_flag(ctx, V_SQ_ALU_SRC_0);
	pops(ctx, ifidx + 1);
	emit_return(ctx);
}

static void break_loop_on_flag(struct r600_shader_ctx *ctx, unsigned fc_sp)
{
	emit_testflag(ctx);

	r600_bytecode_add_cfinst(ctx->bc, ctx->inst_info->r600_opcode);
	ctx->bc->cf_last->pop_count = 1;

	fc_set_mid(ctx, fc_sp);

	pops(ctx, 1);
}
#endif

static int tgsi_if(struct r600_shader_ctx *ctx)
{
	emit_logic_pred(ctx, CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETNE_INT));

	r600_bytecode_add_cfinst(ctx->bc, CTX_INST(V_SQ_CF_WORD1_SQ_CF_INST_JUMP));

	fc_pushlevel(ctx, FC_IF);

	callstack_check_depth(ctx, FC_PUSH_VPM, 0);
	return 0;
}

static int tgsi_else(struct r600_shader_ctx *ctx)
{
	r600_bytecode_add_cfinst(ctx->bc, CTX_INST(V_SQ_CF_WORD1_SQ_CF_INST_ELSE));
	ctx->bc->cf_last->pop_count = 1;

	fc_set_mid(ctx, ctx->bc->fc_sp);
	ctx->bc->fc_stack[ctx->bc->fc_sp].start->cf_addr = ctx->bc->cf_last->id;
	return 0;
}

static int tgsi_endif(struct r600_shader_ctx *ctx)
{
	pops(ctx, 1);
	if (ctx->bc->fc_stack[ctx->bc->fc_sp].type != FC_IF) {
		R600_ERR("if/endif unbalanced in shader\n");
		return -1;
	}

	if (ctx->bc->fc_stack[ctx->bc->fc_sp].mid == NULL) {
		ctx->bc->fc_stack[ctx->bc->fc_sp].start->cf_addr = ctx->bc->cf_last->id + 2;
		ctx->bc->fc_stack[ctx->bc->fc_sp].start->pop_count = 1;
	} else {
		ctx->bc->fc_stack[ctx->bc->fc_sp].mid[0]->cf_addr = ctx->bc->cf_last->id + 2;
	}
	fc_poplevel(ctx);

	callstack_decrease_current(ctx, FC_PUSH_VPM);
	return 0;
}

static int tgsi_bgnloop(struct r600_shader_ctx *ctx)
{
	/* LOOP_START_DX10 ignores the LOOP_CONFIG* registers, so it is not
	 * limited to 4096 iterations, like the other LOOP_* instructions. */
	r600_bytecode_add_cfinst(ctx->bc, CTX_INST(V_SQ_CF_WORD1_SQ_CF_INST_LOOP_START_DX10));

	fc_pushlevel(ctx, FC_LOOP);

	/* check stack depth */
	callstack_check_depth(ctx, FC_LOOP, 0);
	return 0;
}

static int tgsi_endloop(struct r600_shader_ctx *ctx)
{
	int i;

	r600_bytecode_add_cfinst(ctx->bc, CTX_INST(V_SQ_CF_WORD1_SQ_CF_INST_LOOP_END));

	if (ctx->bc->fc_stack[ctx->bc->fc_sp].type != FC_LOOP) {
		R600_ERR("loop/endloop in shader code are not paired.\n");
		return -EINVAL;
	}

	/* fixup loop pointers - from r600isa
	   LOOP END points to CF after LOOP START,
	   LOOP START point to CF after LOOP END
	   BRK/CONT point to LOOP END CF
	*/
	ctx->bc->cf_last->cf_addr = ctx->bc->fc_stack[ctx->bc->fc_sp].start->id + 2;

	ctx->bc->fc_stack[ctx->bc->fc_sp].start->cf_addr = ctx->bc->cf_last->id + 2;

	for (i = 0; i < ctx->bc->fc_stack[ctx->bc->fc_sp].num_mid; i++) {
		ctx->bc->fc_stack[ctx->bc->fc_sp].mid[i]->cf_addr = ctx->bc->cf_last->id;
	}
	/* XXX add LOOPRET support */
	fc_poplevel(ctx);
	callstack_decrease_current(ctx, FC_LOOP);
	return 0;
}

static int tgsi_loop_brk_cont(struct r600_shader_ctx *ctx)
{
	unsigned int fscp;

	for (fscp = ctx->bc->fc_sp; fscp > 0; fscp--)
	{
		if (FC_LOOP == ctx->bc->fc_stack[fscp].type)
			break;
	}

	if (fscp == 0) {
		R600_ERR("Break not inside loop/endloop pair\n");
		return -EINVAL;
	}

	r600_bytecode_add_cfinst(ctx->bc, ctx->inst_info->r600_opcode);

	fc_set_mid(ctx, fscp);

	callstack_check_depth(ctx, FC_PUSH_VPM, 1);
	return 0;
}

static int tgsi_umad(struct r600_shader_ctx *ctx)
{
	struct tgsi_full_instruction *inst = &ctx->parse.FullToken.FullInstruction;
	struct r600_bytecode_alu alu;
	int i, j, r;
	int lasti = tgsi_last_instruction(inst->Dst[0].Register.WriteMask);

	/* src0 * src1 */
	for (i = 0; i < lasti + 1; i++) {
		if (!(inst->Dst[0].Register.WriteMask & (1 << i)))
			continue;

		memset(&alu, 0, sizeof(struct r600_bytecode_alu));

		alu.dst.chan = i;
		alu.dst.sel = ctx->temp_reg;
		alu.dst.write = 1;

		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MULLO_UINT);
		for (j = 0; j < 2; j++) {
		        r600_bytecode_src(&alu.src[j], &ctx->src[j], i);
		}

		alu.last = 1;
		r = r600_bytecode_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}


	for (i = 0; i < lasti + 1; i++) {
		if (!(inst->Dst[0].Register.WriteMask & (1 << i)))
			continue;

		memset(&alu, 0, sizeof(struct r600_bytecode_alu));
		tgsi_dst(ctx, &inst->Dst[0], i, &alu.dst);

		alu.inst = CTX_INST(V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_ADD_INT);

		alu.src[0].sel = ctx->temp_reg;
		alu.src[0].chan = i;
		
		r600_bytecode_src(&alu.src[1], &ctx->src[2], i);
		if (i == lasti) {
			alu.last = 1;
		}
		r = r600_bytecode_add_alu(ctx->bc, &alu);
		if (r)
			return r;
	}
	return 0;
}

static struct r600_shader_tgsi_instruction r600_shader_tgsi_instruction[] = {
	{TGSI_OPCODE_ARL,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_r600_arl},
	{TGSI_OPCODE_MOV,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV, tgsi_op2},
	{TGSI_OPCODE_LIT,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_lit},

	/* XXX:
	 * For state trackers other than OpenGL, we'll want to use
	 * _RECIP_IEEE instead.
	 */
	{TGSI_OPCODE_RCP,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIP_CLAMPED, tgsi_trans_srcx_replicate},

	{TGSI_OPCODE_RSQ,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_rsq},
	{TGSI_OPCODE_EXP,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_exp},
	{TGSI_OPCODE_LOG,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_log},
	{TGSI_OPCODE_MUL,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MUL, tgsi_op2},
	{TGSI_OPCODE_ADD,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_ADD, tgsi_op2},
	{TGSI_OPCODE_DP3,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_DOT4, tgsi_dp},
	{TGSI_OPCODE_DP4,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_DOT4, tgsi_dp},
	{TGSI_OPCODE_DST,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_opdst},
	{TGSI_OPCODE_MIN,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MIN, tgsi_op2},
	{TGSI_OPCODE_MAX,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MAX, tgsi_op2},
	{TGSI_OPCODE_SLT,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGT, tgsi_op2_swap},
	{TGSI_OPCODE_SGE,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGE, tgsi_op2},
	{TGSI_OPCODE_MAD,	1, V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_MULADD, tgsi_op3},
	{TGSI_OPCODE_SUB,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_ADD, tgsi_op2},
	{TGSI_OPCODE_LRP,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_lrp},
	{TGSI_OPCODE_CND,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	/* gap */
	{20,			0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_DP2A,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	/* gap */
	{22,			0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{23,			0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_FRC,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FRACT, tgsi_op2},
	{TGSI_OPCODE_CLAMP,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_FLR,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FLOOR, tgsi_op2},
	{TGSI_OPCODE_ROUND,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RNDNE, tgsi_op2},
	{TGSI_OPCODE_EX2,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_EXP_IEEE, tgsi_trans_srcx_replicate},
	{TGSI_OPCODE_LG2,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LOG_IEEE, tgsi_trans_srcx_replicate},
	{TGSI_OPCODE_POW,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_pow},
	{TGSI_OPCODE_XPD,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_xpd},
	/* gap */
	{32,			0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_ABS,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV, tgsi_op2},
	{TGSI_OPCODE_RCC,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_DPH,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_DOT4, tgsi_dp},
	{TGSI_OPCODE_COS,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_COS, tgsi_trig},
	{TGSI_OPCODE_DDX,	0, SQ_TEX_INST_GET_GRADIENTS_H, tgsi_tex},
	{TGSI_OPCODE_DDY,	0, SQ_TEX_INST_GET_GRADIENTS_V, tgsi_tex},
	{TGSI_OPCODE_KILP,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLGT, tgsi_kill},  /* predicated kill */
	{TGSI_OPCODE_PK2H,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_PK2US,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_PK4B,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_PK4UB,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_RFL,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_SEQ,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETE, tgsi_op2},
	{TGSI_OPCODE_SFL,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_SGT,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGT, tgsi_op2},
	{TGSI_OPCODE_SIN,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SIN, tgsi_trig},
	{TGSI_OPCODE_SLE,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGE, tgsi_op2_swap},
	{TGSI_OPCODE_SNE,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETNE, tgsi_op2},
	{TGSI_OPCODE_STR,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_TEX,	0, SQ_TEX_INST_SAMPLE, tgsi_tex},
	{TGSI_OPCODE_TXD,	0, SQ_TEX_INST_SAMPLE_G, tgsi_tex},
	{TGSI_OPCODE_TXP,	0, SQ_TEX_INST_SAMPLE, tgsi_tex},
	{TGSI_OPCODE_UP2H,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_UP2US,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_UP4B,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_UP4UB,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_X2D,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_ARA,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_ARR,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_r600_arl},
	{TGSI_OPCODE_BRA,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_CAL,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_RET,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_SSG,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_ssg},
	{TGSI_OPCODE_CMP,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_cmp},
	{TGSI_OPCODE_SCS,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_scs},
	{TGSI_OPCODE_TXB,	0, SQ_TEX_INST_SAMPLE_LB, tgsi_tex},
	{TGSI_OPCODE_NRM,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_DIV,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_DP2,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_DOT4, tgsi_dp},
	{TGSI_OPCODE_TXL,	0, SQ_TEX_INST_SAMPLE_L, tgsi_tex},
	{TGSI_OPCODE_BRK,	0, V_SQ_CF_WORD1_SQ_CF_INST_LOOP_BREAK, tgsi_loop_brk_cont},
	{TGSI_OPCODE_IF,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_if},
	/* gap */
	{75,			0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{76,			0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_ELSE,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_else},
	{TGSI_OPCODE_ENDIF,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_endif},
	/* gap */
	{79,			0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{80,			0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_PUSHA,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_POPA,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_CEIL,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_CEIL, tgsi_op2},
	{TGSI_OPCODE_I2F,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_INT_TO_FLT, tgsi_op2_trans},
	{TGSI_OPCODE_NOT,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOT_INT, tgsi_op2},
	{TGSI_OPCODE_TRUNC,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_TRUNC, tgsi_op2},
	{TGSI_OPCODE_SHL,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LSHL_INT, tgsi_op2_trans},
	/* gap */
	{88,			0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_AND,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_AND_INT, tgsi_op2},
	{TGSI_OPCODE_OR,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_OR_INT, tgsi_op2},
	{TGSI_OPCODE_MOD,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_imod},
	{TGSI_OPCODE_XOR,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_XOR_INT, tgsi_op2},
	{TGSI_OPCODE_SAD,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_TXF,	0, SQ_TEX_INST_LD, tgsi_tex},
	{TGSI_OPCODE_TXQ,	0, SQ_TEX_INST_GET_TEXTURE_RESINFO, tgsi_tex},
	{TGSI_OPCODE_CONT,	0, V_SQ_CF_WORD1_SQ_CF_INST_LOOP_CONTINUE, tgsi_loop_brk_cont},
	{TGSI_OPCODE_EMIT,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_ENDPRIM,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_BGNLOOP,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_bgnloop},
	{TGSI_OPCODE_BGNSUB,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_ENDLOOP,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_endloop},
	{TGSI_OPCODE_ENDSUB,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_TXQ_LZ,	0, SQ_TEX_INST_GET_TEXTURE_RESINFO, tgsi_tex},
	/* gap */
	{104,			0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{105,			0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{106,			0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_NOP,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	/* gap */
	{108,			0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{109,			0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{110,			0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{111,			0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_NRM4,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_CALLNZ,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_IFC,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_BREAKC,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_KIL,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLGT, tgsi_kill},  /* conditional kill */
	{TGSI_OPCODE_END,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_end},  /* aka HALT */
	/* gap */
	{118,			0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_F2I,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FLT_TO_INT, tgsi_op2_trans},
	{TGSI_OPCODE_IDIV,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_idiv},
	{TGSI_OPCODE_IMAX,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MAX_INT, tgsi_op2},
	{TGSI_OPCODE_IMIN,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MIN_INT, tgsi_op2},
	{TGSI_OPCODE_INEG,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SUB_INT, tgsi_ineg},
	{TGSI_OPCODE_ISGE,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGE_INT, tgsi_op2},
	{TGSI_OPCODE_ISHR,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_ASHR_INT, tgsi_op2_trans},
	{TGSI_OPCODE_ISLT,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGT_INT, tgsi_op2_swap},
	{TGSI_OPCODE_F2U,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FLT_TO_UINT, tgsi_op2_trans},
	{TGSI_OPCODE_U2F,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_UINT_TO_FLT, tgsi_op2_trans},
	{TGSI_OPCODE_UADD,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_ADD_INT, tgsi_op2},
	{TGSI_OPCODE_UDIV,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_udiv},
	{TGSI_OPCODE_UMAD,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_umad},
	{TGSI_OPCODE_UMAX,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MAX_UINT, tgsi_op2},
	{TGSI_OPCODE_UMIN,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MIN_UINT, tgsi_op2},
	{TGSI_OPCODE_UMOD,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_umod},
	{TGSI_OPCODE_UMUL,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MULLO_UINT, tgsi_op2_trans},
	{TGSI_OPCODE_USEQ,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETE_INT, tgsi_op2},
	{TGSI_OPCODE_USGE,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGE_UINT, tgsi_op2},
	{TGSI_OPCODE_USHR,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LSHR_INT, tgsi_op2_trans},
	{TGSI_OPCODE_USLT,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGT_UINT, tgsi_op2_swap},
	{TGSI_OPCODE_USNE,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETNE_INT, tgsi_op2_swap},
	{TGSI_OPCODE_SWITCH,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_CASE,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_DEFAULT,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_ENDSWITCH,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_SAMPLE,    0, 0, tgsi_unsupported},
	{TGSI_OPCODE_SAMPLE_I,  0, 0, tgsi_unsupported},
	{TGSI_OPCODE_SAMPLE_I_MS, 0, 0, tgsi_unsupported},
	{TGSI_OPCODE_SAMPLE_B,  0, 0, tgsi_unsupported},
	{TGSI_OPCODE_SAMPLE_C,  0, 0, tgsi_unsupported},
	{TGSI_OPCODE_SAMPLE_C_LZ, 0, 0, tgsi_unsupported},
	{TGSI_OPCODE_SAMPLE_D,  0, 0, tgsi_unsupported},
	{TGSI_OPCODE_SAMPLE_L,  0, 0, tgsi_unsupported},
	{TGSI_OPCODE_GATHER4,   0, 0, tgsi_unsupported},
	{TGSI_OPCODE_SVIEWINFO,	0, 0, tgsi_unsupported},
	{TGSI_OPCODE_SAMPLE_POS, 0, 0, tgsi_unsupported},
	{TGSI_OPCODE_SAMPLE_INFO, 0, 0, tgsi_unsupported},
	{TGSI_OPCODE_UARL,      0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOVA_INT, tgsi_r600_arl},
	{TGSI_OPCODE_UCMP,      0, 0, tgsi_unsupported},
	{TGSI_OPCODE_IABS,      0, 0, tgsi_iabs},
	{TGSI_OPCODE_ISSG,      0, 0, tgsi_issg},
	{TGSI_OPCODE_LAST,	0, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
};

static struct r600_shader_tgsi_instruction eg_shader_tgsi_instruction[] = {
	{TGSI_OPCODE_ARL,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_eg_arl},
	{TGSI_OPCODE_MOV,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV, tgsi_op2},
	{TGSI_OPCODE_LIT,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_lit},
	{TGSI_OPCODE_RCP,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIP_IEEE, tgsi_trans_srcx_replicate},
	{TGSI_OPCODE_RSQ,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIPSQRT_IEEE, tgsi_rsq},
	{TGSI_OPCODE_EXP,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_exp},
	{TGSI_OPCODE_LOG,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_log},
	{TGSI_OPCODE_MUL,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MUL, tgsi_op2},
	{TGSI_OPCODE_ADD,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_ADD, tgsi_op2},
	{TGSI_OPCODE_DP3,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_DOT4, tgsi_dp},
	{TGSI_OPCODE_DP4,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_DOT4, tgsi_dp},
	{TGSI_OPCODE_DST,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_opdst},
	{TGSI_OPCODE_MIN,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MIN, tgsi_op2},
	{TGSI_OPCODE_MAX,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MAX, tgsi_op2},
	{TGSI_OPCODE_SLT,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGT, tgsi_op2_swap},
	{TGSI_OPCODE_SGE,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGE, tgsi_op2},
	{TGSI_OPCODE_MAD,	1, EG_V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_MULADD, tgsi_op3},
	{TGSI_OPCODE_SUB,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_ADD, tgsi_op2},
	{TGSI_OPCODE_LRP,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_lrp},
	{TGSI_OPCODE_CND,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	/* gap */
	{20,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_DP2A,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	/* gap */
	{22,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{23,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_FRC,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FRACT, tgsi_op2},
	{TGSI_OPCODE_CLAMP,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_FLR,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FLOOR, tgsi_op2},
	{TGSI_OPCODE_ROUND,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RNDNE, tgsi_op2},
	{TGSI_OPCODE_EX2,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_EXP_IEEE, tgsi_trans_srcx_replicate},
	{TGSI_OPCODE_LG2,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LOG_IEEE, tgsi_trans_srcx_replicate},
	{TGSI_OPCODE_POW,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_pow},
	{TGSI_OPCODE_XPD,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_xpd},
	/* gap */
	{32,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_ABS,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV, tgsi_op2},
	{TGSI_OPCODE_RCC,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_DPH,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_DOT4, tgsi_dp},
	{TGSI_OPCODE_COS,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_COS, tgsi_trig},
	{TGSI_OPCODE_DDX,	0, SQ_TEX_INST_GET_GRADIENTS_H, tgsi_tex},
	{TGSI_OPCODE_DDY,	0, SQ_TEX_INST_GET_GRADIENTS_V, tgsi_tex},
	{TGSI_OPCODE_KILP,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLGT, tgsi_kill},  /* predicated kill */
	{TGSI_OPCODE_PK2H,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_PK2US,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_PK4B,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_PK4UB,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_RFL,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_SEQ,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETE, tgsi_op2},
	{TGSI_OPCODE_SFL,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_SGT,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGT, tgsi_op2},
	{TGSI_OPCODE_SIN,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SIN, tgsi_trig},
	{TGSI_OPCODE_SLE,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGE, tgsi_op2_swap},
	{TGSI_OPCODE_SNE,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETNE, tgsi_op2},
	{TGSI_OPCODE_STR,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_TEX,	0, SQ_TEX_INST_SAMPLE, tgsi_tex},
	{TGSI_OPCODE_TXD,	0, SQ_TEX_INST_SAMPLE_G, tgsi_tex},
	{TGSI_OPCODE_TXP,	0, SQ_TEX_INST_SAMPLE, tgsi_tex},
	{TGSI_OPCODE_UP2H,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_UP2US,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_UP4B,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_UP4UB,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_X2D,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_ARA,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_ARR,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_eg_arl},
	{TGSI_OPCODE_BRA,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_CAL,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_RET,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_SSG,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_ssg},
	{TGSI_OPCODE_CMP,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_cmp},
	{TGSI_OPCODE_SCS,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_scs},
	{TGSI_OPCODE_TXB,	0, SQ_TEX_INST_SAMPLE_LB, tgsi_tex},
	{TGSI_OPCODE_NRM,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_DIV,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_DP2,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_DOT4, tgsi_dp},
	{TGSI_OPCODE_TXL,	0, SQ_TEX_INST_SAMPLE_L, tgsi_tex},
	{TGSI_OPCODE_BRK,	0, EG_V_SQ_CF_WORD1_SQ_CF_INST_LOOP_BREAK, tgsi_loop_brk_cont},
	{TGSI_OPCODE_IF,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_if},
	/* gap */
	{75,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{76,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_ELSE,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_else},
	{TGSI_OPCODE_ENDIF,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_endif},
	/* gap */
	{79,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{80,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_PUSHA,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_POPA,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_CEIL,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_CEIL, tgsi_op2},
	{TGSI_OPCODE_I2F,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_INT_TO_FLT, tgsi_op2_trans},
	{TGSI_OPCODE_NOT,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOT_INT, tgsi_op2},
	{TGSI_OPCODE_TRUNC,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_TRUNC, tgsi_op2},
	{TGSI_OPCODE_SHL,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LSHL_INT, tgsi_op2},
	/* gap */
	{88,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_AND,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_AND_INT, tgsi_op2},
	{TGSI_OPCODE_OR,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_OR_INT, tgsi_op2},
	{TGSI_OPCODE_MOD,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_imod},
	{TGSI_OPCODE_XOR,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_XOR_INT, tgsi_op2},
	{TGSI_OPCODE_SAD,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_TXF,	0, SQ_TEX_INST_LD, tgsi_tex},
	{TGSI_OPCODE_TXQ,	0, SQ_TEX_INST_GET_TEXTURE_RESINFO, tgsi_tex},
	{TGSI_OPCODE_CONT,	0, EG_V_SQ_CF_WORD1_SQ_CF_INST_LOOP_CONTINUE, tgsi_loop_brk_cont},
	{TGSI_OPCODE_EMIT,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_ENDPRIM,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_BGNLOOP,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_bgnloop},
	{TGSI_OPCODE_BGNSUB,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_ENDLOOP,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_endloop},
	{TGSI_OPCODE_ENDSUB,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_TXQ_LZ,	0, SQ_TEX_INST_GET_TEXTURE_RESINFO, tgsi_tex},
	/* gap */
	{104,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{105,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{106,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_NOP,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	/* gap */
	{108,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{109,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{110,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{111,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_NRM4,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_CALLNZ,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_IFC,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_BREAKC,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_KIL,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLGT, tgsi_kill},  /* conditional kill */
	{TGSI_OPCODE_END,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_end},  /* aka HALT */
	/* gap */
	{118,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_F2I,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FLT_TO_INT, tgsi_f2i},
	{TGSI_OPCODE_IDIV,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_idiv},
	{TGSI_OPCODE_IMAX,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MAX_INT, tgsi_op2},
	{TGSI_OPCODE_IMIN,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MIN_INT, tgsi_op2},
	{TGSI_OPCODE_INEG,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SUB_INT, tgsi_ineg},
	{TGSI_OPCODE_ISGE,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGE_INT, tgsi_op2},
	{TGSI_OPCODE_ISHR,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_ASHR_INT, tgsi_op2},
	{TGSI_OPCODE_ISLT,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGT_INT, tgsi_op2_swap},
	{TGSI_OPCODE_F2U,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FLT_TO_UINT, tgsi_f2i},
	{TGSI_OPCODE_U2F,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_UINT_TO_FLT, tgsi_op2_trans},
	{TGSI_OPCODE_UADD,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_ADD_INT, tgsi_op2},
	{TGSI_OPCODE_UDIV,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_udiv},
	{TGSI_OPCODE_UMAD,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_umad},
	{TGSI_OPCODE_UMAX,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MAX_UINT, tgsi_op2},
	{TGSI_OPCODE_UMIN,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MIN_UINT, tgsi_op2},
	{TGSI_OPCODE_UMOD,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_umod},
	{TGSI_OPCODE_UMUL,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MULLO_UINT, tgsi_op2_trans},
	{TGSI_OPCODE_USEQ,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETE_INT, tgsi_op2},
	{TGSI_OPCODE_USGE,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGE_UINT, tgsi_op2},
	{TGSI_OPCODE_USHR,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LSHR_INT, tgsi_op2},
	{TGSI_OPCODE_USLT,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGT_UINT, tgsi_op2_swap},
	{TGSI_OPCODE_USNE,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETNE_INT, tgsi_op2},
	{TGSI_OPCODE_SWITCH,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_CASE,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_DEFAULT,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_ENDSWITCH,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_SAMPLE,    0, 0, tgsi_unsupported},
	{TGSI_OPCODE_SAMPLE_I,      0, 0, tgsi_unsupported},
	{TGSI_OPCODE_SAMPLE_I_MS,   0, 0, tgsi_unsupported},
	{TGSI_OPCODE_SAMPLE_B,  0, 0, tgsi_unsupported},
	{TGSI_OPCODE_SAMPLE_C,  0, 0, tgsi_unsupported},
	{TGSI_OPCODE_SAMPLE_C_LZ, 0, 0, tgsi_unsupported},
	{TGSI_OPCODE_SAMPLE_D,  0, 0, tgsi_unsupported},
	{TGSI_OPCODE_SAMPLE_L,  0, 0, tgsi_unsupported},
	{TGSI_OPCODE_GATHER4,   0, 0, tgsi_unsupported},
	{TGSI_OPCODE_SVIEWINFO,	0, 0, tgsi_unsupported},
	{TGSI_OPCODE_SAMPLE_POS, 0, 0, tgsi_unsupported},
	{TGSI_OPCODE_SAMPLE_INFO, 0, 0, tgsi_unsupported},
	{TGSI_OPCODE_UARL,      0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOVA_INT, tgsi_eg_arl},
	{TGSI_OPCODE_UCMP,      0, 0, tgsi_unsupported},
	{TGSI_OPCODE_IABS,      0, 0, tgsi_iabs},
	{TGSI_OPCODE_ISSG,      0, 0, tgsi_issg},
	{TGSI_OPCODE_LAST,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
};

static struct r600_shader_tgsi_instruction cm_shader_tgsi_instruction[] = {
	{TGSI_OPCODE_ARL,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_eg_arl},
	{TGSI_OPCODE_MOV,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV, tgsi_op2},
	{TGSI_OPCODE_LIT,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_lit},
	{TGSI_OPCODE_RCP,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIP_IEEE, cayman_emit_float_instr},
	{TGSI_OPCODE_RSQ,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIPSQRT_IEEE, cayman_emit_float_instr},
	{TGSI_OPCODE_EXP,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_exp},
	{TGSI_OPCODE_LOG,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_log},
	{TGSI_OPCODE_MUL,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MUL, tgsi_op2},
	{TGSI_OPCODE_ADD,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_ADD, tgsi_op2},
	{TGSI_OPCODE_DP3,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_DOT4, tgsi_dp},
	{TGSI_OPCODE_DP4,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_DOT4, tgsi_dp},
	{TGSI_OPCODE_DST,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_opdst},
	{TGSI_OPCODE_MIN,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MIN, tgsi_op2},
	{TGSI_OPCODE_MAX,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MAX, tgsi_op2},
	{TGSI_OPCODE_SLT,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGT, tgsi_op2_swap},
	{TGSI_OPCODE_SGE,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGE, tgsi_op2},
	{TGSI_OPCODE_MAD,	1, EG_V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_MULADD, tgsi_op3},
	{TGSI_OPCODE_SUB,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_ADD, tgsi_op2},
	{TGSI_OPCODE_LRP,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_lrp},
	{TGSI_OPCODE_CND,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	/* gap */
	{20,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_DP2A,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	/* gap */
	{22,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{23,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_FRC,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FRACT, tgsi_op2},
	{TGSI_OPCODE_CLAMP,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_FLR,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FLOOR, tgsi_op2},
	{TGSI_OPCODE_ROUND,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RNDNE, tgsi_op2},
	{TGSI_OPCODE_EX2,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_EXP_IEEE, cayman_emit_float_instr},
	{TGSI_OPCODE_LG2,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LOG_IEEE, cayman_emit_float_instr},
	{TGSI_OPCODE_POW,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, cayman_pow},
	{TGSI_OPCODE_XPD,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_xpd},
	/* gap */
	{32,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_ABS,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV, tgsi_op2},
	{TGSI_OPCODE_RCC,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_DPH,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_DOT4, tgsi_dp},
	{TGSI_OPCODE_COS,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_COS, cayman_trig},
	{TGSI_OPCODE_DDX,	0, SQ_TEX_INST_GET_GRADIENTS_H, tgsi_tex},
	{TGSI_OPCODE_DDY,	0, SQ_TEX_INST_GET_GRADIENTS_V, tgsi_tex},
	{TGSI_OPCODE_KILP,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLGT, tgsi_kill},  /* predicated kill */
	{TGSI_OPCODE_PK2H,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_PK2US,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_PK4B,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_PK4UB,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_RFL,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_SEQ,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETE, tgsi_op2},
	{TGSI_OPCODE_SFL,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_SGT,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGT, tgsi_op2},
	{TGSI_OPCODE_SIN,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SIN, cayman_trig},
	{TGSI_OPCODE_SLE,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGE, tgsi_op2_swap},
	{TGSI_OPCODE_SNE,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETNE, tgsi_op2},
	{TGSI_OPCODE_STR,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_TEX,	0, SQ_TEX_INST_SAMPLE, tgsi_tex},
	{TGSI_OPCODE_TXD,	0, SQ_TEX_INST_SAMPLE_G, tgsi_tex},
	{TGSI_OPCODE_TXP,	0, SQ_TEX_INST_SAMPLE, tgsi_tex},
	{TGSI_OPCODE_UP2H,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_UP2US,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_UP4B,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_UP4UB,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_X2D,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_ARA,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_ARR,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_eg_arl},
	{TGSI_OPCODE_BRA,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_CAL,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_RET,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_SSG,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_ssg},
	{TGSI_OPCODE_CMP,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_cmp},
	{TGSI_OPCODE_SCS,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_scs},
	{TGSI_OPCODE_TXB,	0, SQ_TEX_INST_SAMPLE_LB, tgsi_tex},
	{TGSI_OPCODE_NRM,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_DIV,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_DP2,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_DOT4, tgsi_dp},
	{TGSI_OPCODE_TXL,	0, SQ_TEX_INST_SAMPLE_L, tgsi_tex},
	{TGSI_OPCODE_BRK,	0, EG_V_SQ_CF_WORD1_SQ_CF_INST_LOOP_BREAK, tgsi_loop_brk_cont},
	{TGSI_OPCODE_IF,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_if},
	/* gap */
	{75,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{76,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_ELSE,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_else},
	{TGSI_OPCODE_ENDIF,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_endif},
	/* gap */
	{79,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{80,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_PUSHA,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_POPA,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_CEIL,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_CEIL, tgsi_op2},
	{TGSI_OPCODE_I2F,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_INT_TO_FLT, tgsi_op2},
	{TGSI_OPCODE_NOT,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOT_INT, tgsi_op2},
	{TGSI_OPCODE_TRUNC,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_TRUNC, tgsi_op2},
	{TGSI_OPCODE_SHL,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LSHL_INT, tgsi_op2},
	/* gap */
	{88,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_AND,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_AND_INT, tgsi_op2},
	{TGSI_OPCODE_OR,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_OR_INT, tgsi_op2},
	{TGSI_OPCODE_MOD,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_imod},
	{TGSI_OPCODE_XOR,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_XOR_INT, tgsi_op2},
	{TGSI_OPCODE_SAD,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_TXF,	0, SQ_TEX_INST_LD, tgsi_tex},
	{TGSI_OPCODE_TXQ,	0, SQ_TEX_INST_GET_TEXTURE_RESINFO, tgsi_tex},
	{TGSI_OPCODE_CONT,	0, EG_V_SQ_CF_WORD1_SQ_CF_INST_LOOP_CONTINUE, tgsi_loop_brk_cont},
	{TGSI_OPCODE_EMIT,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_ENDPRIM,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_BGNLOOP,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_bgnloop},
	{TGSI_OPCODE_BGNSUB,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_ENDLOOP,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_endloop},
	{TGSI_OPCODE_ENDSUB,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_TXQ_LZ,	0, SQ_TEX_INST_GET_TEXTURE_RESINFO, tgsi_tex},
	/* gap */
	{104,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{105,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{106,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_NOP,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	/* gap */
	{108,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{109,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{110,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{111,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_NRM4,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_CALLNZ,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_IFC,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_BREAKC,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_KIL,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLGT, tgsi_kill},  /* conditional kill */
	{TGSI_OPCODE_END,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_end},  /* aka HALT */
	/* gap */
	{118,			0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_F2I,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FLT_TO_INT, tgsi_op2},
	{TGSI_OPCODE_IDIV,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_idiv},
	{TGSI_OPCODE_IMAX,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MAX_INT, tgsi_op2},
	{TGSI_OPCODE_IMIN,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MIN_INT, tgsi_op2},
	{TGSI_OPCODE_INEG,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SUB_INT, tgsi_ineg},
	{TGSI_OPCODE_ISGE,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGE_INT, tgsi_op2},
	{TGSI_OPCODE_ISHR,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_ASHR_INT, tgsi_op2},
	{TGSI_OPCODE_ISLT,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGT_INT, tgsi_op2_swap},
	{TGSI_OPCODE_F2U,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FLT_TO_UINT, tgsi_op2},
	{TGSI_OPCODE_U2F,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_UINT_TO_FLT, tgsi_op2},
	{TGSI_OPCODE_UADD,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_ADD_INT, tgsi_op2},
	{TGSI_OPCODE_UDIV,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_udiv},
	{TGSI_OPCODE_UMAD,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_umad},
	{TGSI_OPCODE_UMAX,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MAX_UINT, tgsi_op2},
	{TGSI_OPCODE_UMIN,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MIN_UINT, tgsi_op2},
	{TGSI_OPCODE_UMOD,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_umod},
	{TGSI_OPCODE_UMUL,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MULLO_INT, cayman_mul_int_instr},
	{TGSI_OPCODE_USEQ,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETE_INT, tgsi_op2},
	{TGSI_OPCODE_USGE,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGE_UINT, tgsi_op2},
	{TGSI_OPCODE_USHR,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LSHR_INT, tgsi_op2},
	{TGSI_OPCODE_USLT,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGT_UINT, tgsi_op2_swap},
	{TGSI_OPCODE_USNE,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETNE_INT, tgsi_op2},
	{TGSI_OPCODE_SWITCH,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_CASE,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_DEFAULT,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_ENDSWITCH,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
	{TGSI_OPCODE_SAMPLE,    0, 0, tgsi_unsupported},
	{TGSI_OPCODE_SAMPLE_I,      0, 0, tgsi_unsupported},
	{TGSI_OPCODE_SAMPLE_I_MS,   0, 0, tgsi_unsupported},
	{TGSI_OPCODE_SAMPLE_B,  0, 0, tgsi_unsupported},
	{TGSI_OPCODE_SAMPLE_C,  0, 0, tgsi_unsupported},
	{TGSI_OPCODE_SAMPLE_C_LZ, 0, 0, tgsi_unsupported},
	{TGSI_OPCODE_SAMPLE_D,  0, 0, tgsi_unsupported},
	{TGSI_OPCODE_SAMPLE_L,  0, 0, tgsi_unsupported},
	{TGSI_OPCODE_GATHER4,   0, 0, tgsi_unsupported},
	{TGSI_OPCODE_SVIEWINFO,	0, 0, tgsi_unsupported},
	{TGSI_OPCODE_SAMPLE_POS, 0, 0, tgsi_unsupported},
	{TGSI_OPCODE_SAMPLE_INFO, 0, 0, tgsi_unsupported},
	{TGSI_OPCODE_UARL,      0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOVA_INT, tgsi_eg_arl},
	{TGSI_OPCODE_UCMP,      0, 0, tgsi_unsupported},
	{TGSI_OPCODE_IABS,      0, 0, tgsi_iabs},
	{TGSI_OPCODE_ISSG,      0, 0, tgsi_issg},
	{TGSI_OPCODE_LAST,	0, EG_V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP, tgsi_unsupported},
};
