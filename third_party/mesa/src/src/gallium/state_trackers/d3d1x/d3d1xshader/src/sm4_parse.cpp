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

#include "sm4.h"
#include "utils.h"

#if 1
#define check(x) assert(x)
#define fail(x) assert(0 && (x))
#else
#define check(x) do {if(!(x)) throw(#x);} while(0)
#define fail(x) throw(x)
#endif

struct sm4_parser
{
	unsigned* tokens;
	unsigned* tokens_end;
	sm4_program& program;

	sm4_parser(sm4_program& program, void* p_tokens, unsigned size)
	: program(program)
	{
		tokens = (unsigned*)p_tokens;
		tokens_end = (unsigned*)((char*)p_tokens + size);
	}

	/* TODO: byteswap if machine is big endian */
	uint32_t read32()
	{
		check(tokens < tokens_end);
		return bswap_le32(*tokens++);
	}

	template<typename T>
	void read_token(T* tok)
	{
		*(unsigned*)tok = read32();
	}

	uint64_t read64()
	{
		unsigned a = read32();
		unsigned b = read32();
		return (uint64_t)a | ((uint64_t)b << 32);
	}

	void skip(unsigned toskip)
	{
		tokens += toskip;
	}

	void read_op(sm4_op* pop)
	{
		sm4_op& op = *pop;
		sm4_token_operand optok;
		read_token(&optok);
		assert(optok.file < SM4_FILE_COUNT);
		op.swizzle[0] = 0;
		op.swizzle[1] = 1;
		op.swizzle[2] = 2;
		op.swizzle[3] = 3;
		op.mask = 0xf;
		switch(optok.comps_enum)
		{
		case SM4_OPERAND_COMPNUM_0:
			op.comps = 0;
			break;
		case SM4_OPERAND_COMPNUM_1:
			op.comps = 1;
			op.swizzle[1] = op.swizzle[2] = op.swizzle[3] = 0;
			break;
		case SM4_OPERAND_COMPNUM_4:
			op.comps = 4;
			op.mode = optok.mode;
			switch(optok.mode)
			{
			case SM4_OPERAND_MODE_MASK:
				op.mask = SM4_OPERAND_SEL_MASK(optok.sel);
				break;
			case SM4_OPERAND_MODE_SWIZZLE:
				op.swizzle[0] = SM4_OPERAND_SEL_SWZ(optok.sel, 0);
				op.swizzle[1] = SM4_OPERAND_SEL_SWZ(optok.sel, 1);
				op.swizzle[2] = SM4_OPERAND_SEL_SWZ(optok.sel, 2);
				op.swizzle[3] = SM4_OPERAND_SEL_SWZ(optok.sel, 3);
				break;
			case SM4_OPERAND_MODE_SCALAR:
				op.swizzle[0] = op.swizzle[1] = op.swizzle[2] = op.swizzle[3] = SM4_OPERAND_SEL_SCALAR(optok.sel);
				break;
			}
			break;
		case SM4_OPERAND_COMPNUM_N:
			fail("Unhandled operand component type");
		}
		op.file = (sm4_file)optok.file;
		op.num_indices = optok.num_indices;

		if(optok.extended)
		{
			sm4_token_operand_extended optokext;
			read_token(&optokext);
			if(optokext.type == 0)
			{}
			else if(optokext.type == 1)
			{
				op.neg = optokext.neg;
				op.abs= optokext.abs;
			}
			else
				fail("Unhandled extended operand token type");
		}

		for(unsigned i = 0; i < op.num_indices; ++i)
		{
			unsigned repr;
			if(i == 0)
				repr = optok.index0_repr;
			else if(i == 1)
				repr = optok.index1_repr;
			else if(i == 2)
				repr = optok.index2_repr;
			else
				fail("Unhandled operand index representation");
			op.indices[i].disp = 0;
			// TODO: is disp supposed to be signed here??
			switch(repr)
			{
			case SM4_OPERAND_INDEX_REPR_IMM32:
				op.indices[i].disp = (int32_t)read32();
				break;
			case SM4_OPERAND_INDEX_REPR_IMM64:
				op.indices[i].disp = read64();
				break;
			case SM4_OPERAND_INDEX_REPR_REG:
relative:
				op.indices[i].reg.reset(new sm4_op());
				read_op(&*op.indices[i].reg);
				break;
			case SM4_OPERAND_INDEX_REPR_REG_IMM32:
				op.indices[i].disp = (int32_t)read32();
				goto relative;
			case SM4_OPERAND_INDEX_REPR_REG_IMM64:
				op.indices[i].disp = read64();
				goto relative;
			}
		}

		if(op.file == SM4_FILE_IMMEDIATE32)
		{
			for(unsigned i = 0; i < op.comps; ++i)
				op.imm_values[i].i32 = read32();
		}
		else if(op.file == SM4_FILE_IMMEDIATE64)
		{
			for(unsigned i = 0; i < op.comps; ++i)
				op.imm_values[i].i64 = read64();
		}
	}

	void do_parse()
	{
		read_token(&program.version);

		unsigned lentok = read32();
		tokens_end = tokens - 2 + lentok;

		while(tokens != tokens_end)
		{
			sm4_token_instruction insntok;
			read_token(&insntok);
			unsigned* insn_end = tokens - 1 + insntok.length;
			sm4_opcode opcode = (sm4_opcode)insntok.opcode;
			check(opcode < SM4_OPCODE_COUNT);

			if(opcode == SM4_OPCODE_CUSTOMDATA)
			{
				// immediate constant buffer data
				unsigned customlen = read32() - 2;

				sm4_dcl& dcl = *new sm4_dcl;
				program.dcls.push_back(&dcl);

				dcl.opcode = SM4_OPCODE_CUSTOMDATA;
				dcl.num = customlen;
				dcl.data = malloc(customlen * sizeof(tokens[0]));

				memcpy(dcl.data, &tokens[0], customlen * sizeof(tokens[0]));

				skip(customlen);
				continue;
			}

			if(opcode == SM4_OPCODE_HS_FORK_PHASE || opcode == SM4_OPCODE_HS_JOIN_PHASE)
			{
				// need to interleave these with the declarations or we cannot
				// assign fork/join phase instance counts to phases
				sm4_dcl& dcl = *new sm4_dcl;
				program.dcls.push_back(&dcl);
				dcl.opcode = opcode;
			}

			if((opcode >= SM4_OPCODE_DCL_RESOURCE && opcode <= SM4_OPCODE_DCL_GLOBAL_FLAGS)
				|| (opcode >= SM4_OPCODE_DCL_STREAM && opcode <= SM4_OPCODE_DCL_RESOURCE_STRUCTURED))
			{
				sm4_dcl& dcl = *new sm4_dcl;
				program.dcls.push_back(&dcl);
				(sm4_token_instruction&)dcl = insntok;

				sm4_token_instruction_extended exttok;
				memcpy(&exttok, &insntok, sizeof(exttok));
				while(exttok.extended)
				{
					read_token(&exttok);
				}

#define READ_OP_ANY dcl.op.reset(new sm4_op()); read_op(&*dcl.op);
#define READ_OP(FILE) READ_OP_ANY
				//check(dcl.op->file == SM4_FILE_##FILE);

				switch(opcode)
				{
				case SM4_OPCODE_DCL_GLOBAL_FLAGS:
					break;
				case SM4_OPCODE_DCL_RESOURCE:
					READ_OP(RESOURCE);
					read_token(&dcl.rrt);
					break;
				case SM4_OPCODE_DCL_SAMPLER:
					READ_OP(SAMPLER);
					break;
				case SM4_OPCODE_DCL_INPUT:
				case SM4_OPCODE_DCL_INPUT_PS:
					READ_OP(INPUT);
					break;
				case SM4_OPCODE_DCL_INPUT_SIV:
				case SM4_OPCODE_DCL_INPUT_SGV:
				case SM4_OPCODE_DCL_INPUT_PS_SIV:
				case SM4_OPCODE_DCL_INPUT_PS_SGV:
					READ_OP(INPUT);
					dcl.sv = (sm4_sv)(uint16_t)read32();
					break;
				case SM4_OPCODE_DCL_OUTPUT:
					READ_OP(OUTPUT);
					break;
				case SM4_OPCODE_DCL_OUTPUT_SIV:
				case SM4_OPCODE_DCL_OUTPUT_SGV:
					READ_OP(OUTPUT);
					dcl.sv = (sm4_sv)(uint16_t)read32();
					break;
				case SM4_OPCODE_DCL_INDEX_RANGE:
					READ_OP_ANY;
					check(dcl.op->file == SM4_FILE_INPUT || dcl.op->file == SM4_FILE_OUTPUT);
					dcl.num = read32();
					break;
				case SM4_OPCODE_DCL_TEMPS:
					dcl.num = read32();
					break;
				case SM4_OPCODE_DCL_INDEXABLE_TEMP:
					READ_OP(INDEXABLE_TEMP);
					dcl.indexable_temp.num = read32();
					dcl.indexable_temp.comps = read32();
					break;
				case SM4_OPCODE_DCL_CONSTANT_BUFFER:
					READ_OP(CONSTANT_BUFFER);
					break;
				case SM4_OPCODE_DCL_GS_INPUT_PRIMITIVE:
				case SM4_OPCODE_DCL_GS_OUTPUT_PRIMITIVE_TOPOLOGY:
					break;
				case SM4_OPCODE_DCL_MAX_OUTPUT_VERTEX_COUNT:
					dcl.num = read32();
					break;
				case SM4_OPCODE_DCL_GS_INSTANCE_COUNT:
					dcl.num = read32();
					break;
				case SM4_OPCODE_DCL_INPUT_CONTROL_POINT_COUNT:
				case SM4_OPCODE_DCL_OUTPUT_CONTROL_POINT_COUNT:
				case SM4_OPCODE_DCL_TESS_DOMAIN:
				case SM4_OPCODE_DCL_TESS_PARTITIONING:
				case SM4_OPCODE_DCL_TESS_OUTPUT_PRIMITIVE:
					break;
				case SM4_OPCODE_DCL_HS_MAX_TESSFACTOR:
					dcl.f32 = read32();
					break;
				case SM4_OPCODE_DCL_HS_FORK_PHASE_INSTANCE_COUNT:
					dcl.num = read32();
					break;
				case SM4_OPCODE_DCL_FUNCTION_BODY:
					dcl.num = read32();
					break;
				case SM4_OPCODE_DCL_FUNCTION_TABLE:
					dcl.num = read32();
					dcl.data = malloc(dcl.num * sizeof(uint32_t));
					for(unsigned i = 0; i < dcl.num; ++i)
						((uint32_t*)dcl.data)[i] = read32();
					break;
				case SM4_OPCODE_DCL_INTERFACE:
					dcl.intf.id = read32();
					dcl.intf.expected_function_table_length = read32();
					{
						uint32_t v = read32();
						dcl.intf.table_length = v & 0xffff;
						dcl.intf.array_length = v >> 16;
					}
					dcl.data = malloc(dcl.intf.table_length * sizeof(uint32_t));
					for(unsigned i = 0; i < dcl.intf.table_length; ++i)
						((uint32_t*)dcl.data)[i] = read32();
					break;
				case SM4_OPCODE_DCL_THREAD_GROUP:
					dcl.thread_group_size[0] = read32();
					dcl.thread_group_size[1] = read32();
					dcl.thread_group_size[2] = read32();
					break;
				case SM4_OPCODE_DCL_UNORDERED_ACCESS_VIEW_TYPED:
					READ_OP(UNORDERED_ACCESS_VIEW);
					read_token(&dcl.rrt);
					break;
				case SM4_OPCODE_DCL_UNORDERED_ACCESS_VIEW_RAW:
					READ_OP(UNORDERED_ACCESS_VIEW);
					break;
				case SM4_OPCODE_DCL_UNORDERED_ACCESS_VIEW_STRUCTURED:
					READ_OP(UNORDERED_ACCESS_VIEW);
					dcl.structured.stride = read32();
					break;
				case SM4_OPCODE_DCL_THREAD_GROUP_SHARED_MEMORY_RAW:
					READ_OP(THREAD_GROUP_SHARED_MEMORY);
					dcl.num = read32();
					break;
				case SM4_OPCODE_DCL_THREAD_GROUP_SHARED_MEMORY_STRUCTURED:
					READ_OP(THREAD_GROUP_SHARED_MEMORY);
					dcl.structured.stride = read32();
					dcl.structured.count = read32();
					break;
				case SM4_OPCODE_DCL_RESOURCE_RAW:
					READ_OP(RESOURCE);
					break;
				case SM4_OPCODE_DCL_RESOURCE_STRUCTURED:
					READ_OP(RESOURCE);
					dcl.structured.stride = read32();
					break;
				case SM4_OPCODE_DCL_STREAM:
					/* TODO: dcl_stream is undocumented: what is it? */
					fail("Unhandled dcl_stream since it's undocumented");
				default:
					fail("Unhandled declaration");
				}

				check(tokens == insn_end);
			}
			else
			{
				sm4_insn& insn = *new sm4_insn;
				program.insns.push_back(&insn);
				(sm4_token_instruction&)insn = insntok;

				sm4_token_instruction_extended exttok;
				memcpy(&exttok, &insntok, sizeof(exttok));
				while(exttok.extended)
				{
					read_token(&exttok);
					if(exttok.type == SM4_TOKEN_INSTRUCTION_EXTENDED_TYPE_SAMPLE_CONTROLS)
					{
						insn.sample_offset[0] = exttok.sample_controls.offset_u;
						insn.sample_offset[1] = exttok.sample_controls.offset_v;
						insn.sample_offset[2] = exttok.sample_controls.offset_w;
					}
					else if(exttok.type == SM4_TOKEN_INSTRUCTION_EXTENDED_TYPE_RESOURCE_DIM)
						insn.resource_target = exttok.resource_target.target;
					else if(exttok.type == SM4_TOKEN_INSTRUCTION_EXTENDED_TYPE_RESOURCE_RETURN_TYPE)
					{
						insn.resource_return_type[0] = exttok.resource_return_type.x;
						insn.resource_return_type[1] = exttok.resource_return_type.y;
						insn.resource_return_type[2] = exttok.resource_return_type.z;
						insn.resource_return_type[3] = exttok.resource_return_type.w;
					}
				}

				switch(opcode)
				{
				case SM4_OPCODE_INTERFACE_CALL:
					insn.num = read32();
					break;
				default:
					break;
				}

				unsigned op_num = 0;
				while(tokens != insn_end)
				{
					check(tokens < insn_end);
					check(op_num < SM4_MAX_OPS);
					insn.ops[op_num].reset(new sm4_op);
					read_op(&*insn.ops[op_num]);
					++op_num;
				}
				insn.num_ops = op_num;
			}
		}
	}

	const char* parse()
	{
		try
		{
			do_parse();
			return 0;
		}
		catch(const char* error)
		{
			return error;
		}
	}
};

sm4_program* sm4_parse(void* tokens, int size)
{
	sm4_program* program = new sm4_program;
	sm4_parser parser(*program, tokens, size);
	if(!parser.parse())
		return program;
	delete program;
	return 0;
}
