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

// TODO: we should fix this to output the same syntax as fxc, if sm4_dump_short_syntax is set

bool sm4_dump_short_syntax = true;

std::ostream& operator <<(std::ostream& out, const sm4_op& op)
{
	if(op.neg)
		out << '-';
	if(op.abs)
		out << '|';
	if(op.file == SM4_FILE_IMMEDIATE32)
	{
		out << "l(";
		for(unsigned i = 0; i < op.comps; ++i)
		{
			if(i)
				out << ", ";
			out << op.imm_values[i].f32;
		}
		out << ")";
	}
	else if(op.file == SM4_FILE_IMMEDIATE64)
	{
		out << "d(";
		for(unsigned i = 0; i < op.comps; ++i)
		{
			if(i)
				out << ", ";
			out << op.imm_values[i].f64;
		}
		out << ")";
		return out;
	}
	else
	{
		bool naked = false;
		if(sm4_dump_short_syntax)
		{
			switch(op.file)
			{
			case SM4_FILE_TEMP:
			case SM4_FILE_INPUT:
			case SM4_FILE_OUTPUT:
			case SM4_FILE_CONSTANT_BUFFER:
			case SM4_FILE_INDEXABLE_TEMP:
			case SM4_FILE_UNORDERED_ACCESS_VIEW:
			case SM4_FILE_THREAD_GROUP_SHARED_MEMORY:
				naked = true;
				break;
			default:
				naked = false;
				break;
			}
		}

		out << (sm4_dump_short_syntax ? sm4_shortfile_names : sm4_file_names)[op.file];

		if(op.indices[0].reg.get())
			naked = false;

		for(unsigned i = 0; i < op.num_indices; ++i)
		{
			if(!naked || i)
				out << '[';
			if(op.indices[i].reg.get())
			{
				out << *op.indices[i].reg;
				if(op.indices[i].disp)
					out << '+' << op.indices[i].disp;
			}
			else
				out << op.indices[i].disp;
			if(!naked || i)
				out << ']';
		}
		if(op.comps)
		{
			switch(op.mode)
			{
			case SM4_OPERAND_MODE_MASK:
				out << (sm4_dump_short_syntax ? '.' : '!');
				for(unsigned i = 0; i < op.comps; ++i)
				{
					if(op.mask & (1 << i))
						out << "xyzw"[i];
				}
				break;
			case SM4_OPERAND_MODE_SWIZZLE:
				out << '.';
				for(unsigned i = 0; i < op.comps; ++i)
					out << "xyzw"[op.swizzle[i]];
				break;
			case SM4_OPERAND_MODE_SCALAR:
				out << (sm4_dump_short_syntax ? '.' : ':');
				out << "xyzw"[op.swizzle[0]];
				break;
			}
		}
	}
	if(op.abs)
		out << '|';
	return out;
}

std::ostream& operator <<(std::ostream& out, const sm4_dcl& dcl)
{
	out << sm4_opcode_names[dcl.opcode];
	switch(dcl.opcode)
	{
	case SM4_OPCODE_DCL_GLOBAL_FLAGS:
		if(dcl.dcl_global_flags.allow_refactoring)
			out << " refactoringAllowed";
		if(dcl.dcl_global_flags.early_depth_stencil)
			out << " forceEarlyDepthStencil";
		if(dcl.dcl_global_flags.fp64)
			out << " enableDoublePrecisionFloatOps";
		if(dcl.dcl_global_flags.enable_raw_and_structured_in_non_cs)
			out << " enableRawAndStructuredBuffers";
		break;
	case SM4_OPCODE_DCL_INPUT_PS:
	case SM4_OPCODE_DCL_INPUT_PS_SIV:
	case SM4_OPCODE_DCL_INPUT_PS_SGV:
		out << ' ' << sm4_interpolation_names[dcl.dcl_input_ps.interpolation];
		break;
	case SM4_OPCODE_DCL_TEMPS:
		out << ' ' << dcl.num;
		break;
	default:
		break;
	}
	if(dcl.op.get())
		out << ' ' << *dcl.op;
	switch(dcl.opcode)
	{
	case SM4_OPCODE_DCL_CONSTANT_BUFFER:
		out << ", " << (dcl.dcl_constant_buffer.dynamic ? "dynamicIndexed" : "immediateIndexed");
		break;
	case SM4_OPCODE_DCL_INPUT_SIV:
	case SM4_OPCODE_DCL_INPUT_SGV:
	case SM4_OPCODE_DCL_OUTPUT_SIV:
	case SM4_OPCODE_DCL_OUTPUT_SGV:
	case SM4_OPCODE_DCL_INPUT_PS_SIV:
	case SM4_OPCODE_DCL_INPUT_PS_SGV:
		out << ", " << sm4_sv_names[dcl.num];
		break;
	}

	return out;
}

std::ostream& operator <<(std::ostream& out, const sm4_insn& insn)
{
	out << sm4_opcode_names[insn.opcode];
	if(insn.insn.sat)
		out << "_sat";
	for(unsigned i = 0; i < insn.num_ops; ++i)
	{
		if(i)
			out << ',';
		out << ' ' << *insn.ops[i];
	}
	return out;
}

std::ostream& operator <<(std::ostream& out, const sm4_program& program)
{
	out << "pvghdc"[program.version.type] << "s_" << program.version.major << "_" << program.version.minor << "\n";
	for(unsigned i = 0; i < program.dcls.size(); ++i)
		out << *program.dcls[i] << "\n";

	for(unsigned i = 0; i < program.insns.size(); ++i)
		out << *program.insns[i] << "\n";
	return out;
}

void sm4_op::dump()
{
	std::cout << *this;
}

void sm4_insn::dump()
{
	std::cout << *this;
}

void sm4_dcl::dump()
{
	std::cout << *this;
}

void sm4_program::dump()
{
	std::cout << *this;
}
