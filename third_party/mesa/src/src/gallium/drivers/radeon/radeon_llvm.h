/*
 * Copyright 2011 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors: Tom Stellard <thomas.stellard@amd.com>
 *
 */

#ifndef RADEON_LLVM_H
#define RADEON_LLVM_H

#include <llvm-c/Core.h>
#include "gallivm/lp_bld_init.h"
#include "gallivm/lp_bld_tgsi.h"

#define RADEON_LLVM_MAX_INPUTS 16 * 4
#define RADEON_LLVM_MAX_OUTPUTS 16 * 4
#define RADEON_LLVM_MAX_BRANCH_DEPTH 16
#define RADEON_LLVM_MAX_LOOP_DEPTH 16

#define RADEON_LLVM_MAX_SYSTEM_VALUES 4

struct radeon_llvm_branch {
	LLVMBasicBlockRef endif_block;
	LLVMBasicBlockRef if_block;
	LLVMBasicBlockRef else_block;
	unsigned has_else;
};

struct radeon_llvm_loop {
	LLVMBasicBlockRef loop_block;
	LLVMBasicBlockRef endloop_block;
};

struct radeon_llvm_context {

	struct lp_build_tgsi_soa_context soa;

	/*=== Front end configuration ===*/

	/* Special Intrinsics */

	/** Write to an output register: float store_output(float, i32) */
	const char * store_output_intr;

	/** Swizzle a vector value: <4 x float> swizzle(<4 x float>, i32)
	 * The swizzle is an unsigned integer that encodes a TGSI_SWIZZLE_* value
	 * in 2-bits.
	 * Swizzle{0-1} = X Channel
	 * Swizzle{2-3} = Y Channel
	 * Swizzle{4-5} = Z Channel
	 * Swizzle{6-7} = W Channel
	 */
	const char * swizzle_intr;

	/* Instructions that are not described by any of the TGSI opcodes. */

	/** This function is responsible for initilizing the inputs array and will be
	  * called once for each input declared in the TGSI shader.
	  */
	void (*load_input)(struct radeon_llvm_context *,
			unsigned input_index,
			const struct tgsi_full_declaration *decl);

	void (*load_system_value)(struct radeon_llvm_context *,
			unsigned index,
			const struct tgsi_full_declaration *decl);

	/** User data to use with the callbacks */
	void * userdata;

	/** This array contains the input values for the shader.  Typically these
	  * values will be in the form of a target intrinsic that will inform the
	  * backend how to load the actual inputs to the shader. 
	  */
	LLVMValueRef inputs[RADEON_LLVM_MAX_INPUTS];
	LLVMValueRef outputs[RADEON_LLVM_MAX_OUTPUTS][TGSI_NUM_CHANNELS];
	unsigned output_reg_count;

	LLVMValueRef system_values[RADEON_LLVM_MAX_SYSTEM_VALUES];

	unsigned reserved_reg_count;
	/*=== Private Members ===*/

	struct radeon_llvm_branch branch[RADEON_LLVM_MAX_BRANCH_DEPTH];
	struct radeon_llvm_loop loop[RADEON_LLVM_MAX_LOOP_DEPTH];

	unsigned branch_depth;
	unsigned loop_depth;


	LLVMValueRef main_fn;

	struct gallivm_state gallivm;
};

static inline LLVMValueRef bitcast(
		struct lp_build_tgsi_context * bld_base,
		enum tgsi_opcode_type type,
		LLVMValueRef value
)
{
	LLVMBuilderRef builder = bld_base->base.gallivm->builder;
	LLVMContextRef ctx = bld_base->base.gallivm->context;
	LLVMTypeRef dst_type;

	switch (type) {
	case TGSI_TYPE_UNSIGNED:
	case TGSI_TYPE_SIGNED:
		dst_type = LLVMInt32TypeInContext(ctx);
		break;
	case TGSI_TYPE_UNTYPED:
	case TGSI_TYPE_FLOAT:
		dst_type = LLVMFloatTypeInContext(ctx);
		break;
	default:
		dst_type = 0;
		break;
	}

	if (dst_type)
		return LLVMBuildBitCast(builder, value, dst_type, "");
	else
		return value;
}


void radeon_llvm_context_init(struct radeon_llvm_context * ctx);

void radeon_llvm_dispose(struct radeon_llvm_context * ctx);

inline static struct radeon_llvm_context * radeon_llvm_context(
	struct lp_build_tgsi_context * bld_base)
{
	return (struct radeon_llvm_context*)bld_base;
}

unsigned radeon_llvm_reg_index_soa(unsigned index, unsigned chan);

void radeon_llvm_finalize_module(struct radeon_llvm_context * ctx);

LLVMValueRef
build_intrinsic(LLVMBuilderRef builder,
		const char *name,
		LLVMTypeRef ret_type,
		LLVMValueRef *args,
		unsigned num_args,
		LLVMAttribute attr);

void
build_tgsi_intrinsic_nomem(
		const struct lp_build_tgsi_action * action,
		struct lp_build_tgsi_context * bld_base,
		struct lp_build_emit_data * emit_data);



#endif /* RADEON_LLVM_H */
