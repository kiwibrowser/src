#include "r600_llvm.h"

#include "gallivm/lp_bld_const.h"
#include "gallivm/lp_bld_intr.h"
#include "gallivm/lp_bld_gather.h"
#include "tgsi/tgsi_parse.h"
#include "util/u_double_list.h"
#include "util/u_memory.h"

#include "r600.h"
#include "r600_asm.h"
#include "r600_opcodes.h"
#include "r600_shader.h"
#include "radeon_llvm.h"
#include "radeon_llvm_emit.h"

#include <stdio.h>

#if defined R600_USE_LLVM || defined HAVE_OPENCL

static LLVMValueRef llvm_fetch_const(
	struct lp_build_tgsi_context * bld_base,
	const struct tgsi_full_src_register *reg,
	enum tgsi_opcode_type type,
	unsigned swizzle)
{
	LLVMValueRef idx = lp_build_const_int32(bld_base->base.gallivm,
			radeon_llvm_reg_index_soa(reg->Register.Index, swizzle));
	LLVMValueRef cval = build_intrinsic(bld_base->base.gallivm->builder,
		"llvm.AMDGPU.load.const", bld_base->base.elem_type,
		&idx, 1, LLVMReadNoneAttribute);

	return bitcast(bld_base, type, cval);
}

static void llvm_load_system_value(
		struct radeon_llvm_context * ctx,
		unsigned index,
		const struct tgsi_full_declaration *decl)
{
	unsigned chan;

	switch (decl->Semantic.Name) {
	case TGSI_SEMANTIC_INSTANCEID: chan = 3; break;
	case TGSI_SEMANTIC_VERTEXID: chan = 0; break;
	default: assert(!"unknown system value");
	}

	LLVMValueRef reg = lp_build_const_int32(
			ctx->soa.bld_base.base.gallivm, chan);
	ctx->system_values[index] = build_intrinsic(
			ctx->soa.bld_base.base.gallivm->builder,
			"llvm.R600.load.input",
			ctx->soa.bld_base.base.elem_type, &reg, 1,
			LLVMReadNoneAttribute);
}

static LLVMValueRef llvm_fetch_system_value(
		struct lp_build_tgsi_context * bld_base,
		const struct tgsi_full_src_register *reg,
		enum tgsi_opcode_type type,
		unsigned swizzle)
{
	struct radeon_llvm_context * ctx = radeon_llvm_context(bld_base);
	LLVMValueRef cval = ctx->system_values[reg->Register.Index];
	return bitcast(bld_base, type, cval);
}

static void llvm_load_input(
	struct radeon_llvm_context * ctx,
	unsigned input_index,
	const struct tgsi_full_declaration *decl)
{
	unsigned chan;

	for (chan = 0; chan < 4; chan++) {
		unsigned soa_index = radeon_llvm_reg_index_soa(input_index,
								chan);

		/* The * 4 is assuming that we are in soa mode. */
		LLVMValueRef reg = lp_build_const_int32(
				ctx->soa.bld_base.base.gallivm,
				soa_index + (ctx->reserved_reg_count * 4));
		ctx->inputs[soa_index] = build_intrinsic(
				ctx->soa.bld_base.base.gallivm->builder,
				"llvm.R600.load.input",
				ctx->soa.bld_base.base.elem_type, &reg, 1,
				LLVMReadNoneAttribute);
	}
}

static void llvm_emit_prologue(struct lp_build_tgsi_context * bld_base)
{
	struct radeon_llvm_context * ctx = radeon_llvm_context(bld_base);
	struct lp_build_context * base = &bld_base->base;
	unsigned i;

	/* Reserve special input registers */
	for (i = 0; i < ctx->reserved_reg_count; i++) {
		unsigned chan;
		for (chan = 0; chan < TGSI_NUM_CHANNELS; chan++) {
			LLVMValueRef reg_index = lp_build_const_int32(
					base->gallivm,
					radeon_llvm_reg_index_soa(i, chan));
			lp_build_intrinsic_unary(base->gallivm->builder,
				"llvm.AMDGPU.reserve.reg",
				LLVMVoidTypeInContext(base->gallivm->context),
				reg_index);
		}
	}
}

static void llvm_emit_epilogue(struct lp_build_tgsi_context * bld_base)
{
	struct radeon_llvm_context * ctx = radeon_llvm_context(bld_base);
	struct lp_build_context * base = &bld_base->base;
	unsigned i;

	/* Add the necessary export instructions */
	for (i = 0; i < ctx->output_reg_count; i++) {
		unsigned chan;
		for (chan = 0; chan < TGSI_NUM_CHANNELS; chan++) {
			LLVMValueRef output;
			unsigned adjusted_reg_idx = i +
					ctx->reserved_reg_count;
			LLVMValueRef reg_index = lp_build_const_int32(
				base->gallivm,
				radeon_llvm_reg_index_soa(adjusted_reg_idx, chan));

			output = LLVMBuildLoad(base->gallivm->builder,
				ctx->soa.outputs[i][chan], "");

			lp_build_intrinsic_binary(
				base->gallivm->builder,
				"llvm.AMDGPU.store.output",
				LLVMVoidTypeInContext(base->gallivm->context),
				output, reg_index);
		}
	}
}

static void llvm_emit_tex(
	const struct lp_build_tgsi_action * action,
	struct lp_build_tgsi_context * bld_base,
	struct lp_build_emit_data * emit_data)
{
	struct gallivm_state * gallivm = bld_base->base.gallivm;
	LLVMValueRef args[6];
	unsigned c, sampler_src;

	assert(emit_data->arg_count + 2 <= Elements(args));

	for (c = 0; c < emit_data->arg_count; ++c)
		args[c] = emit_data->args[c];

	sampler_src = emit_data->inst->Instruction.NumSrcRegs-1;

	args[c++] = lp_build_const_int32(gallivm,
					emit_data->inst->Src[sampler_src].Register.Index);
	args[c++] = lp_build_const_int32(gallivm,
					emit_data->inst->Texture.Texture);

	emit_data->output[0] = build_intrinsic(gallivm->builder,
					action->intr_name,
					emit_data->dst_type, args, c, LLVMReadNoneAttribute);
}

static void dp_fetch_args(
	struct lp_build_tgsi_context * bld_base,
	struct lp_build_emit_data * emit_data)
{
	struct lp_build_context * base = &bld_base->base;
	unsigned chan;
	LLVMValueRef elements[2][4];
	unsigned opcode = emit_data->inst->Instruction.Opcode;
	unsigned dp_components = (opcode == TGSI_OPCODE_DP2 ? 2 :
					(opcode == TGSI_OPCODE_DP3 ? 3 : 4));
	for (chan = 0 ; chan < dp_components; chan++) {
		elements[0][chan] = lp_build_emit_fetch(bld_base,
						emit_data->inst, 0, chan);
		elements[1][chan] = lp_build_emit_fetch(bld_base,
						emit_data->inst, 1, chan);
	}

	for ( ; chan < 4; chan++) {
		elements[0][chan] = base->zero;
		elements[1][chan] = base->zero;
	}

	 /* Fix up for DPH */
	if (opcode == TGSI_OPCODE_DPH) {
		elements[0][TGSI_CHAN_W] = base->one;
	}

	emit_data->args[0] = lp_build_gather_values(bld_base->base.gallivm,
							elements[0], 4);
	emit_data->args[1] = lp_build_gather_values(bld_base->base.gallivm,
							elements[1], 4);
	emit_data->arg_count = 2;

	emit_data->dst_type = base->elem_type;
}

static struct lp_build_tgsi_action dot_action = {
	.fetch_args = dp_fetch_args,
	.emit = build_tgsi_intrinsic_nomem,
	.intr_name = "llvm.AMDGPU.dp4"
};



LLVMModuleRef r600_tgsi_llvm(
	struct radeon_llvm_context * ctx,
	const struct tgsi_token * tokens)
{
	struct tgsi_shader_info shader_info;
	struct lp_build_tgsi_context * bld_base = &ctx->soa.bld_base;
	radeon_llvm_context_init(ctx);
	tgsi_scan_shader(tokens, &shader_info);

	bld_base->info = &shader_info;
	bld_base->userdata = ctx;
	bld_base->emit_fetch_funcs[TGSI_FILE_CONSTANT] = llvm_fetch_const;
	bld_base->emit_fetch_funcs[TGSI_FILE_SYSTEM_VALUE] = llvm_fetch_system_value;
	bld_base->emit_prologue = llvm_emit_prologue;
	bld_base->emit_epilogue = llvm_emit_epilogue;
	ctx->userdata = ctx;
	ctx->load_input = llvm_load_input;
	ctx->load_system_value = llvm_load_system_value;

	bld_base->op_actions[TGSI_OPCODE_DP2] = dot_action;
	bld_base->op_actions[TGSI_OPCODE_DP3] = dot_action;
	bld_base->op_actions[TGSI_OPCODE_DP4] = dot_action;
	bld_base->op_actions[TGSI_OPCODE_DPH] = dot_action;
	bld_base->op_actions[TGSI_OPCODE_DDX].emit = llvm_emit_tex;
	bld_base->op_actions[TGSI_OPCODE_DDY].emit = llvm_emit_tex;
	bld_base->op_actions[TGSI_OPCODE_TEX].emit = llvm_emit_tex;
	bld_base->op_actions[TGSI_OPCODE_TXB].emit = llvm_emit_tex;
	bld_base->op_actions[TGSI_OPCODE_TXD].emit = llvm_emit_tex;
	bld_base->op_actions[TGSI_OPCODE_TXL].emit = llvm_emit_tex;
	bld_base->op_actions[TGSI_OPCODE_TXF].emit = llvm_emit_tex;
	bld_base->op_actions[TGSI_OPCODE_TXQ].emit = llvm_emit_tex;
	bld_base->op_actions[TGSI_OPCODE_TXP].emit = llvm_emit_tex;

	lp_build_tgsi_llvm(bld_base, tokens);

	radeon_llvm_finalize_module(ctx);

	return ctx->gallivm.module;
}

const char * r600_llvm_gpu_string(enum radeon_family family)
{
	const char * gpu_family;

	switch (family) {
	case CHIP_R600:
	case CHIP_RV610:
	case CHIP_RV630:
	case CHIP_RV620:
	case CHIP_RV635:
	case CHIP_RS780:
	case CHIP_RS880:
	case CHIP_RV710:
		gpu_family = "rv710";
		break;
	case CHIP_RV730:
		gpu_family = "rv730";
		break;
	case CHIP_RV670:
	case CHIP_RV740:
	case CHIP_RV770:
		gpu_family = "rv770";
		break;
	case CHIP_PALM:
	case CHIP_CEDAR:
		gpu_family = "cedar";
		break;
	case CHIP_SUMO:
	case CHIP_SUMO2:
	case CHIP_REDWOOD:
		gpu_family = "redwood";
		break;
	case CHIP_JUNIPER:
		gpu_family = "juniper";
		break;
	case CHIP_HEMLOCK:
	case CHIP_CYPRESS:
		gpu_family = "cypress";
		break;
	case CHIP_BARTS:
		gpu_family = "barts";
		break;
	case CHIP_TURKS:
		gpu_family = "turks";
		break;
	case CHIP_CAICOS:
		gpu_family = "caicos";
		break;
	case CHIP_CAYMAN:
        case CHIP_ARUBA:
		gpu_family = "cayman";
		break;
	default:
		gpu_family = "";
		fprintf(stderr, "Chip not supported by r600 llvm "
			"backend, please file a bug at bugs.freedesktop.org\n");
		break;
	}
	return gpu_family;
}

unsigned r600_llvm_compile(
	LLVMModuleRef mod,
	unsigned char ** inst_bytes,
	unsigned * inst_byte_count,
	enum radeon_family family,
	unsigned dump)
{
	const char * gpu_family = r600_llvm_gpu_string(family);
	return radeon_llvm_compile(mod, inst_bytes, inst_byte_count,
							gpu_family, dump);
}

#endif
