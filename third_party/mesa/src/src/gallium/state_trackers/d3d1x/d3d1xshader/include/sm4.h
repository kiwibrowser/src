/**************************************************************************
 *
 * Copyright 2010 Luca Barbieri
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/* Header for Shader Model 4.0, 4.1 and 5.0 */

#ifndef SM4_H_
#define SM4_H_

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <memory>
#include <vector>
#include <map>
#include <iostream>
#include "le32.h"

#include "sm4_defs.h"

extern const char* sm4_opcode_names[];
extern const char* sm4_file_names[];
extern const char* sm4_shortfile_names[];
extern const char* sm4_target_names[];
extern const char* sm4_interpolation_names[];
extern const char* sm4_sv_names[];

struct sm4_token_version
{
	unsigned minor : 4;
	unsigned major : 4;
	unsigned format : 8;
	unsigned type : 16;
};

struct sm4_token_instruction
{
	// we don't make it an union directly because unions can't be inherited from
	union
	{
		// length and extended are always present, but they are only here to reduce duplication
		struct
		{
			unsigned opcode : 11;
			unsigned _11_23 : 13;
			unsigned length : 7;
			unsigned extended : 1;
		};
		struct
		{
			unsigned opcode : 11;
			unsigned resinfo_return_type : 2;
			unsigned sat : 1;
			unsigned _14_17 : 4;
			unsigned test_nz : 1; // bit 18
			unsigned precise_mask : 4;
			unsigned _23 : 1;
			unsigned length : 7;
			unsigned extended : 1;
		} insn;
		struct
		{
			unsigned opcode : 11;
			unsigned threads_in_group : 1;
			unsigned shared_memory : 1;
			unsigned uav_group : 1;
			unsigned uav_global : 1;
			unsigned _15_17 : 3;
		} sync;
		struct
		{
			unsigned opcode : 11;
			unsigned allow_refactoring : 1;
			unsigned fp64 : 1;
			unsigned early_depth_stencil : 1;
			unsigned enable_raw_and_structured_in_non_cs : 1;
		} dcl_global_flags;
		struct
		{
			unsigned opcode : 11;
			unsigned target : 5;
			unsigned nr_samples : 7;
		} dcl_resource;
		struct
		{
			unsigned opcode : 11;
			unsigned shadow : 1;
			unsigned mono : 1;
		} dcl_sampler;
		struct
		{
			unsigned opcode : 11;
			unsigned interpolation : 5;
		} dcl_input_ps;
		struct
		{
			unsigned opcode : 11;
			unsigned dynamic : 1;
		} dcl_constant_buffer;
		struct
		{
			unsigned opcode : 11;
			unsigned primitive : 6;
		} dcl_gs_input_primitive;
		struct
		{
			unsigned opcode : 11;
			unsigned primitive_topology : 7;
		} dcl_gs_output_primitive_topology;
		struct
		{
			unsigned opcode : 11;
			unsigned control_points : 6;
		} dcl_input_control_point_count;
		struct
		{
			unsigned opcode : 11;
			unsigned control_points : 6;
		} dcl_output_control_point_count;
		struct
		{
			unsigned opcode : 11;
			unsigned domain : 3; /* D3D_TESSELLATOR_DOMAIN */
		} dcl_tess_domain;
		struct
		{
			unsigned opcode : 11;
			unsigned partitioning : 3; /* D3D_TESSELLATOR_PARTITIONING */
		} dcl_tess_partitioning;
		struct
		{
			unsigned opcode : 11;
			unsigned primitive : 3; /* D3D_TESSELLATOR_OUTPUT_PRIMITIVE */
		} dcl_tess_output_primitive;
	};
};

union sm4_token_instruction_extended
{
	struct
	{
		unsigned type : 6;
		unsigned _6_30 : 25;
		unsigned extended :1;
	};
	struct
	{
		unsigned type : 6;
		unsigned _6_8 : 3;
		int offset_u : 4;
		int offset_v : 4;
		int offset_w : 4;
	} sample_controls;
	struct
	{
		unsigned type : 6;
		unsigned target : 5;
	} resource_target;
	struct
	{
		unsigned type : 6;
		unsigned x : 4;
		unsigned y : 4;
		unsigned z : 4;
		unsigned w : 4;
	} resource_return_type;
};

struct sm4_token_resource_return_type
{
	unsigned x : 4;
	unsigned y : 4;
	unsigned z : 4;
	unsigned w : 4;
};

struct sm4_token_operand
{
	unsigned comps_enum : 2; /* sm4_operands_comps */
	unsigned mode : 2; /* sm4_operand_mode */
	unsigned sel : 8;
	unsigned file : 8; /* sm4_file */
	unsigned num_indices : 2;
	unsigned index0_repr : 3; /* sm4_operand_index_repr */
	unsigned index1_repr : 3; /* sm4_operand_index_repr */
	unsigned index2_repr : 3; /* sm4_operand_index_repr */
	unsigned extended : 1;
};

#define SM4_OPERAND_SEL_MASK(sel) ((sel) & 0xf)
#define SM4_OPERAND_SEL_SWZ(sel, i) (((sel) >> ((i) * 2)) & 3)
#define SM4_OPERAND_SEL_SCALAR(sel) ((sel) & 3)

struct sm4_token_operand_extended
{
	unsigned type : 6;
	unsigned neg : 1;
	unsigned abs : 1;
};

union sm4_any
{
	double f64;
	float f32;
	int64_t i64;
	int32_t i32;
	uint64_t u64;
	int64_t u32;
};

struct sm4_op;
struct sm4_insn;
struct sm4_dcl;
struct sm4_program;
std::ostream& operator <<(std::ostream& out, const sm4_op& op);
std::ostream& operator <<(std::ostream& out, const sm4_insn& op);
std::ostream& operator <<(std::ostream& out, const sm4_dcl& op);
std::ostream& operator <<(std::ostream& out, const sm4_program& op);

struct sm4_op
{
	uint8_t mode;
	uint8_t comps;
	uint8_t mask;
	uint8_t num_indices;
	uint8_t swizzle[4];
	sm4_file file;
	sm4_any imm_values[4];
	bool neg;
	bool abs;
	struct
	{
		int64_t disp;
		std::auto_ptr<sm4_op> reg;
	} indices[3];

	bool is_index_simple(unsigned i) const
	{
		 return !indices[i].reg.get() && indices[i].disp >= 0 && (int64_t)(int32_t)indices[i].disp == indices[i].disp;
	}

	bool has_simple_index() const
	{
		return num_indices == 1 && is_index_simple(0);
	}

	sm4_op()
	{
		memset(this, 0, sizeof(*this));
	}

	void dump();

private:
	sm4_op(const sm4_op& op)
	{}
};

/* for sample_d */
#define SM4_MAX_OPS 6

struct sm4_insn : public sm4_token_instruction
{
	int8_t sample_offset[3];
	uint8_t resource_target;
	uint8_t resource_return_type[4];

	unsigned num;
	unsigned num_ops;
	std::auto_ptr<sm4_op> ops[SM4_MAX_OPS];

	sm4_insn()
	{
		memset(this, 0, sizeof(*this));
	}

	void dump();

private:
	sm4_insn(const sm4_insn& op)
	{}
};

struct sm4_dcl : public sm4_token_instruction
{
	std::auto_ptr<sm4_op> op;
	union
	{
		unsigned num;
		float f32;
		sm4_sv sv;
		struct
		{
			unsigned id;
			unsigned expected_function_table_length;
			unsigned table_length;
			unsigned array_length;
		} intf;
		unsigned thread_group_size[3];
		sm4_token_resource_return_type rrt;
		struct
		{
			unsigned num;
			unsigned comps;
		} indexable_temp;
		struct
		{
			unsigned stride;
			unsigned count;
		} structured;
	};

	void* data;

	sm4_dcl()
	{
		memset(this, 0, sizeof(*this));
	}

	~sm4_dcl()
	{
		free(data);
	}

	void dump();

private:
	sm4_dcl(const sm4_dcl& op)
	{}
};

struct _D3D11_SIGNATURE_PARAMETER_DESC;

struct sm4_program
{
	sm4_token_version version;
	std::vector<sm4_dcl*> dcls;
	std::vector<sm4_insn*> insns;

	_D3D11_SIGNATURE_PARAMETER_DESC* params_in;
	_D3D11_SIGNATURE_PARAMETER_DESC* params_out;
	_D3D11_SIGNATURE_PARAMETER_DESC* params_patch;
	unsigned num_params_in;
	unsigned num_params_out;
	unsigned num_params_patch;

	/* for ifs, the insn number of the else or endif if there is no else
	 * for elses, the insn number of the endif
	 * for endifs, the insn number of the if
	 * for loops, the insn number of the endloop
	 * for endloops, the insn number of the loop
	 * for all others, -1
	 */
	std::vector<int> cf_insn_linked;

	bool labels_found;
	std::vector<int> label_to_insn_num;

	sm4_program()
	{
		memset(&version, 0, sizeof(version));
		labels_found = false;
		num_params_in = num_params_out = num_params_patch = 0;
	}

	~sm4_program()
	{
		for(std::vector<sm4_dcl*>::iterator i = dcls.begin(), e = dcls.end(); i != e; ++i)
			delete *i;
		for(std::vector<sm4_insn*>::iterator i = insns.begin(), e = insns.end(); i != e; ++i)
			delete *i;

		if(num_params_in)
			free(params_in);
		if(num_params_out)
			free(params_out);
		if(num_params_patch)
			free(params_patch);
	}

	void dump();

private:
	sm4_program(const sm4_dcl& op)
	{}
};

sm4_program* sm4_parse(void* tokens, int size);

bool sm4_link_cf_insns(sm4_program& program);
bool sm4_find_labels(sm4_program& program);

#endif /* SM4_H_ */

