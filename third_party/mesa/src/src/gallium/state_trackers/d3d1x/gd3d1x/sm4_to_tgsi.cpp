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

#include <d3d11shader.h>
#include "d3d1xstutil.h"
#include "sm4.h"
#include "tgsi/tgsi_ureg.h"
#include <vector>

#if 1
#define check(x) assert(x)
#define fail(x) assert(0 && (x))
#else
#define check(x) do {if(!(x)) throw(#x);} while(0)
#define fail(x) throw(x)
#endif

struct tgsi_interpolation
{
	unsigned interpolation;
	bool centroid;
};

static tgsi_interpolation sm4_to_pipe_interpolation[] =
{
	{TGSI_INTERPOLATE_PERSPECTIVE, false}, /* UNDEFINED */
	{TGSI_INTERPOLATE_CONSTANT, false},
	{TGSI_INTERPOLATE_PERSPECTIVE, false}, /* LINEAR */
	{TGSI_INTERPOLATE_PERSPECTIVE, true}, /* LINEAR_CENTROID */
	{TGSI_INTERPOLATE_LINEAR, false}, /* LINEAR_NOPERSPECTIVE */
	{TGSI_INTERPOLATE_LINEAR, true}, /* LINEAR_NOPERSPECTIVE_CENTROID */

	// Added in D3D10.1
	{TGSI_INTERPOLATE_PERSPECTIVE, true}, /* LINEAR_SAMPLE */
	{TGSI_INTERPOLATE_LINEAR, true}, /* LINEAR_NOPERSPECTIVE_SAMPLE */
};

static int sm4_to_pipe_sv[] =
{
	-1,
	TGSI_SEMANTIC_POSITION,
	-1, /*TGSI_SEMANTIC_CLIP_DISTANCE */
	-1, /*TGSI_SEMANTIC_CULL_DISTANCE */
	-1, /*TGSI_SEMANTIC_RENDER_TARGET_ARRAY_INDEX */
	-1, /*TGSI_SEMANTIC_VIEWPORT_ARRAY_INDEX */
	-1, /*TGSI_SEMANTIC_VERTEXID,*/
	TGSI_SEMANTIC_PRIMID,
	TGSI_SEMANTIC_INSTANCEID,
	TGSI_SEMANTIC_FACE,
	-1, /*TGSI_SEMANTIC_SAMPLE_INDEX*/
};

struct sm4_to_tgsi_converter
{
	struct ureg_program* ureg;
	std::vector<struct ureg_dst> temps;
	std::vector<struct ureg_dst> outputs;
	std::vector<struct ureg_src> inputs;
	std::vector<struct ureg_src> resources;
	std::vector<struct ureg_src> samplers;
	std::vector<std::pair<unsigned, unsigned> > targets; // first is normal, second shadow/comparison
	std::vector<unsigned> sampler_modes; // 0 = normal, 1 = shadow/comparison
	std::vector<std::pair<unsigned, unsigned> > loops;
	sm4_insn* insn;
	struct sm4_program& program;
	std::vector<unsigned> sm4_to_tgsi_insn_num;
	std::vector<std::pair<unsigned, unsigned> > label_to_sm4_insn_num;
	bool in_sub;
	bool avoid_txf;
	bool avoid_int;

	sm4_to_tgsi_converter(struct sm4_program& program)
	: program(program)
	{
		avoid_txf = true;
		avoid_int = false;
	}

	struct ureg_dst _reg(sm4_op& op)
	{
		switch(op.file)
		{
		case SM4_FILE_NULL:
		{
			struct ureg_dst d;
			memset(&d, 0, sizeof(d));
			d.File = TGSI_FILE_NULL;
			return d;
		}
		case SM4_FILE_TEMP:
			check(op.has_simple_index());
			check(op.indices[0].disp < temps.size());
			return temps[op.indices[0].disp];
		case SM4_FILE_OUTPUT:
			check(op.has_simple_index());
			check(op.indices[0].disp < outputs.size());
			return outputs[op.indices[0].disp];
		default:
			check(0);
			return ureg_dst_undef();
		}
	}

	struct ureg_dst _dst(unsigned i = 0)
	{
		check(i < insn->num_ops);
		sm4_op& op = *insn->ops[i];
		check(op.mode == SM4_OPERAND_MODE_MASK || op.mode == SM4_OPERAND_MODE_SCALAR);
		struct ureg_dst d = ureg_writemask(_reg(op), op.mask);
		if(insn->insn.sat)
			d = ureg_saturate(d);
		return d;
	}

	struct ureg_src _src(unsigned i)
	{
		check(i < insn->num_ops);
		sm4_op& op = *insn->ops[i];
		struct ureg_src s;
		switch(op.file)
		{
		case SM4_FILE_IMMEDIATE32:
			s = ureg_imm4f(ureg, op.imm_values[0].f32, op.imm_values[1].f32, op.imm_values[2].f32, op.imm_values[3].f32);
			break;
		case SM4_FILE_INPUT:
			check(op.is_index_simple(0));
			check(op.num_indices == 1 || op.num_indices == 2);
			// TODO: is this correct, or are incorrectly swapping the two indices in the GS case?
			check(op.indices[op.num_indices - 1].disp < inputs.size());
			s = inputs[op.indices[op.num_indices - 1].disp];
			if(op.num_indices == 2)
			{
				s.Dimension = 1;
				s.DimensionIndex = op.indices[0].disp;
			}
			break;
		case SM4_FILE_CONSTANT_BUFFER:
			// TODO: indirect addressing
			check(op.num_indices == 2);
			check(op.is_index_simple(0));
			check(op.is_index_simple(1));
			s = ureg_src_register(TGSI_FILE_CONSTANT, (unsigned)op.indices[1].disp);
			s.Dimension = 1;
			s.DimensionIndex = op.indices[0].disp;
			break;
		default:
			s = ureg_src(_reg(op));
			break;
		}
		if(op.mode == SM4_OPERAND_MODE_SWIZZLE || op.mode == SM4_OPERAND_MODE_SCALAR)
			s = ureg_swizzle(s, op.swizzle[0], op.swizzle[1], op.swizzle[2], op.swizzle[3]);
		else
		{
			/* immediates are masked to show needed values */
			check(op.file == SM4_FILE_IMMEDIATE32 || op.file == SM4_FILE_IMMEDIATE64);
		}
		if(op.abs)
			s = ureg_abs(s);
		if(op.neg)
			s = ureg_negate(s);
		return s;
	};

	int _idx(sm4_file file, unsigned i = 0)
	{
		check(i < insn->num_ops);
		sm4_op& op = *insn->ops[i];
		check(op.file == file);
		check(op.has_simple_index());
		return (int)op.indices[0].disp;
	}

	unsigned tex_target(unsigned resource, unsigned sampler)
	{
		unsigned shadow = sampler_modes[sampler];
		unsigned target = shadow ? targets[resource].second : targets[resource].first;
		check(target);
		return target;
	}

	enum pipe_type res_return_type(unsigned type)
	{
		switch(type)
		{
		case D3D_RETURN_TYPE_UNORM: return PIPE_TYPE_UNORM;
		case D3D_RETURN_TYPE_SNORM: return PIPE_TYPE_SNORM;
		case D3D_RETURN_TYPE_SINT:  return PIPE_TYPE_SINT;
		case D3D_RETURN_TYPE_UINT:  return PIPE_TYPE_UINT;
		case D3D_RETURN_TYPE_FLOAT: return PIPE_TYPE_FLOAT;
		default:
			fail("invalid resource return type");
			return PIPE_TYPE_FLOAT;
		}
	}

	std::vector<struct ureg_dst> insn_tmps;

	struct ureg_dst _tmp()
	{
		struct ureg_dst t = ureg_DECL_temporary(ureg);
		insn_tmps.push_back(t);
		return t;
	}

	struct ureg_dst _tmp(struct ureg_dst d)
	{
		if(d.File == TGSI_FILE_TEMPORARY)
			return d;
		else
			return ureg_writemask(_tmp(), d.WriteMask);
	}

#define OP1_(d, g) case SM4_OPCODE_##d: ureg_##g(ureg, _dst(), _src(1)); break
#define OP2_(d, g) case SM4_OPCODE_##d: ureg_##g(ureg, _dst(), _src(1), _src(2)); break
#define OP3_(d, g) case SM4_OPCODE_##d: ureg_##g(ureg, _dst(), _src(1), _src(2), _src(3)); break
#define OP1(n) OP1_(n, n)
#define OP2(n) OP2_(n, n)
#define OP3(n) OP3_(n, n)
#define OP_CF(d, g) case SM4_OPCODE_##d: ureg_##g(ureg, &label); label_to_sm4_insn_num.push_back(std::make_pair(label, program.cf_insn_linked[insn_num])); break;

	void translate_insns(unsigned begin, unsigned end)
	{
		for(unsigned insn_num = begin; insn_num < end; ++insn_num)
		{
			sm4_to_tgsi_insn_num[insn_num] = ureg_get_instruction_number(ureg);
			unsigned label;
			insn = program.insns[insn_num];
			bool ok;
			ok = true;
			switch(insn->opcode)
			{
			// trivial instructions
			case SM4_OPCODE_NOP:
				break;
			OP1(MOV);

			// float
			OP2(ADD);
			OP2(MUL);
			OP3(MAD);
			OP2(DIV);
			OP1(FRC);
			OP1(RCP);
			OP2(MIN);
			OP2(MAX);
			OP2_(LT, SLT);
			OP2_(GE, SGE);
			OP2_(EQ, SEQ);
			OP2_(NE, SNE);

			// bitwise
			OP1(NOT);
			OP2(AND);
			OP2(OR);
			OP2(XOR);

			// special mathematical
			OP2(DP2);
			OP2(DP3);
			OP2(DP4);
			OP1(RSQ);
			OP1_(LOG, LG2);
			OP1_(EXP, EX2);

			// rounding
			OP1_(ROUND_NE, ROUND);
			OP1_(ROUND_Z, TRUNC);
			OP1_(ROUND_PI, CEIL);
			OP1_(ROUND_NI, FLR);

			// cross-thread
			OP1_(DERIV_RTX, DDX);
			OP1_(DERIV_RTX_COARSE, DDX);
			OP1_(DERIV_RTX_FINE, DDX);
			OP1_(DERIV_RTY, DDY);
			OP1_(DERIV_RTY_COARSE, DDY);
			OP1_(DERIV_RTY_FINE, DDY);
			case SM4_OPCODE_EMIT:
				ureg_EMIT(ureg);
				break;
			case SM4_OPCODE_CUT:
				ureg_ENDPRIM(ureg);
				break;
			case SM4_OPCODE_EMITTHENCUT:
				ureg_EMIT(ureg);
				ureg_ENDPRIM(ureg);
				break;

			// non-trivial instructions
			case SM4_OPCODE_MOVC:
				/* CMP checks for < 0, but MOVC checks for != 0
				 * but fortunately, x != 0 is equivalent to -abs(x) < 0
				 * XXX: can test_nz apply to this?!
				 */
				ureg_CMP(ureg, _dst(), ureg_negate(ureg_abs(_src(1))), _src(2), _src(3));
				break;
			case SM4_OPCODE_SQRT:
			{
				struct ureg_dst d = _dst();
				struct ureg_dst t = _tmp(d);
				ureg_RSQ(ureg, t, _src(1));
				ureg_RCP(ureg, d, ureg_src(t));
				break;
			}
			case SM4_OPCODE_SINCOS:
			{
				struct ureg_dst s = _dst(0);
				struct ureg_dst c = _dst(1);
				struct ureg_src v = _src(2);
				if(s.File != TGSI_FILE_NULL)
					ureg_SIN(ureg, s, v);
				if(c.File != TGSI_FILE_NULL)
					ureg_COS(ureg, c, v);
				break;
			}

			// control flow
			case SM4_OPCODE_DISCARD:
				ureg_KIL(ureg, _src(0));
				break;
			OP_CF(LOOP, BGNLOOP);
			OP_CF(ENDLOOP, ENDLOOP);
			case SM4_OPCODE_BREAK:
				ureg_BRK(ureg);
				break;
			case SM4_OPCODE_BREAKC:
				// XXX: can test_nz apply to this?!
				ureg_BREAKC(ureg, _src(0));
				break;
			case SM4_OPCODE_CONTINUE:
				ureg_CONT(ureg);
				break;
			case SM4_OPCODE_CONTINUEC:
				// XXX: can test_nz apply to this?!
				ureg_IF(ureg, _src(0), &label);
				ureg_CONT(ureg);
				ureg_fixup_label(ureg, label, ureg_get_instruction_number(ureg));
				ureg_ENDIF(ureg);
				break;
			case SM4_OPCODE_SWITCH:
				ureg_SWITCH(ureg, _src(0));
				break;
			case SM4_OPCODE_CASE:
				ureg_CASE(ureg, _src(0));
				break;
			case SM4_OPCODE_DEFAULT:
				ureg_DEFAULT(ureg);
				break;
			case SM4_OPCODE_ENDSWITCH:
				ureg_ENDSWITCH(ureg);
				break;
			case SM4_OPCODE_CALL:
				ureg_CAL(ureg, &label);
				label_to_sm4_insn_num.push_back(std::make_pair(label, program.label_to_insn_num[_idx(SM4_FILE_LABEL)]));
				break;
			case SM4_OPCODE_LABEL:
				if(in_sub)
					ureg_ENDSUB(ureg);
				else
					ureg_END(ureg);
				ureg_BGNSUB(ureg);
				in_sub = true;
				break;
			case SM4_OPCODE_RET:
				if(in_sub || insn_num != (program.insns.size() - 1))
					ureg_RET(ureg);
				break;
			case SM4_OPCODE_RETC:
				ureg_IF(ureg, _src(0), &label);
				if(insn->insn.test_nz)
					ureg_RET(ureg);
				ureg_fixup_label(ureg, label, ureg_get_instruction_number(ureg));
				if(!insn->insn.test_nz)
				{
					ureg_ELSE(ureg, &label);
					ureg_RET(ureg);
					ureg_fixup_label(ureg, label, ureg_get_instruction_number(ureg));
				}
				ureg_ENDIF(ureg);
				break;
			OP_CF(ELSE, ELSE);
			case SM4_OPCODE_ENDIF:
				ureg_ENDIF(ureg);
				break;
			case SM4_OPCODE_IF:
				if(insn->insn.test_nz)
				{
					ureg_IF(ureg, _src(0), &label);
					label_to_sm4_insn_num.push_back(std::make_pair(label, program.cf_insn_linked[insn_num]));
				}
				else
				{
					unsigned linked = program.cf_insn_linked[insn_num];
					if(program.insns[linked]->opcode == SM4_OPCODE_ENDIF)
					{
						ureg_IF(ureg, _src(0), &label);
						ureg_fixup_label(ureg, label, ureg_get_instruction_number(ureg));
						ureg_ELSE(ureg, &label);
						label_to_sm4_insn_num.push_back(std::make_pair(label, linked));
					}
					else
					{
						/* we have to swap the branches in this case (fun!)
						 * TODO: maybe just emit a SEQ 0?
						 * */
						unsigned endif = program.cf_insn_linked[linked];

						ureg_IF(ureg, _src(0), &label);
						label_to_sm4_insn_num.push_back(std::make_pair(label, linked));

						translate_insns(linked + 1, endif);

						sm4_to_tgsi_insn_num[linked] = ureg_get_instruction_number(ureg);
						ureg_ELSE(ureg, &label);
						label_to_sm4_insn_num.push_back(std::make_pair(label, endif));

						translate_insns(insn_num + 1, linked);

						insn_num = endif - 1;
						goto next;
					}
				}
				break;
			case SM4_OPCODE_RESINFO:
				// TODO: return type
				ureg_SVIEWINFO(ureg, _dst(), _src(1), resources[_idx(SM4_FILE_RESOURCE, 2)]);
				break;
			// TODO: sample index, texture offset
			case SM4_OPCODE_LD: // dst, coord_int, res; mipmap level in last coord_int arg
				ureg_LOAD(ureg, _dst(), _src(1), resources[_idx(SM4_FILE_RESOURCE, 2)]);
				break;
			case SM4_OPCODE_LD_MS:
				ureg_LOAD_MS(ureg, _dst(), _src(1), resources[_idx(SM4_FILE_RESOURCE, 2)]);
				break;
			case SM4_OPCODE_SAMPLE: // dst, coord, res, samp
				ureg_SAMPLE(ureg, _dst(), _src(1), resources[_idx(SM4_FILE_RESOURCE, 2)], samplers[_idx(SM4_FILE_SAMPLER, 3)]);
				break;
			case SM4_OPCODE_SAMPLE_B: // dst, coord, res, samp, bias.x
				ureg_SAMPLE_B(ureg, _dst(), _src(1), resources[_idx(SM4_FILE_RESOURCE, 2)], samplers[_idx(SM4_FILE_SAMPLER, 3)], _src(4));
				break;
			case SM4_OPCODE_SAMPLE_C: // dst, coord, res, samp, comp.x
				ureg_SAMPLE_C(ureg, _dst(), _src(1), resources[_idx(SM4_FILE_RESOURCE, 2)], samplers[_idx(SM4_FILE_SAMPLER, 3)], _src(4));
				break;
			case SM4_OPCODE_SAMPLE_C_LZ: // dst, coord, res, samp, comp.x
				ureg_SAMPLE_C_LZ(ureg, _dst(), _src(1), resources[_idx(SM4_FILE_RESOURCE, 2)], samplers[_idx(SM4_FILE_SAMPLER, 3)], _src(4));
				break;
			case SM4_OPCODE_SAMPLE_D: // dst, coord, res, samp, ddx, ddy
				ureg_SAMPLE_D(ureg, _dst(), _src(1), resources[_idx(SM4_FILE_RESOURCE, 2)], samplers[_idx(SM4_FILE_SAMPLER, 3)], _src(4), _src(5));
				break;
			case SM4_OPCODE_SAMPLE_L: // dst, coord, res, samp, bias.x
			{
				struct ureg_dst tmp = _tmp();
				ureg_MOV(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_XYZ), _src(1));
				ureg_MOV(ureg, ureg_writemask(tmp, TGSI_WRITEMASK_W), ureg_swizzle(_src(4), 0, 0, 0, 0));
				ureg_SAMPLE_L(ureg, _dst(), ureg_src(tmp), resources[_idx(SM4_FILE_RESOURCE, 2)], samplers[_idx(SM4_FILE_SAMPLER, 3)]);
				break;
			}
			default:
				ok = false;
				break;
			}

			if(!ok && !avoid_int)
			{
				ok = true;
				switch(insn->opcode)
				{
				// integer
				OP1_(ITOF, I2F);
				OP1_(FTOI, F2I);
				OP2_(IADD, UADD);
				OP1(INEG);
				OP2_(IMUL, UMUL);
				OP3_(IMAD, UMAD);
				OP2_(ISHL, SHL);
				OP2_(ISHR, ISHR);
				OP2(IMIN);
				OP2(IMAX);
				OP2_(ILT, ISLT);
				OP2_(IGE, ISGE);
				OP2_(IEQ, USEQ);
				OP2_(INE, USNE);

				// unsigned
				OP1_(UTOF, U2F);
				OP1_(FTOU, F2U);
				OP2(UMUL);
				OP3(UMAD);
				OP2(UMIN);
				OP2(UMAX);
				OP2_(ULT, USLT);
				OP2_(UGE, USGE);
				OP2(USHR);

				case SM4_OPCODE_UDIV:
				{
					struct ureg_dst q = _dst(0);
					struct ureg_dst r = _dst(1);
					struct ureg_src a = _src(2);
					struct ureg_src b = _src(3);
					if(q.File != TGSI_FILE_NULL)
						ureg_UDIV(ureg, q, a, b);
					if(r.File != TGSI_FILE_NULL)
						ureg_UMOD(ureg, r, a, b);
					break;
				}
				default:
					ok = false;
				}
			}

			if(!ok && avoid_int)
			{
				ok = true;
				switch(insn->opcode)
				{
				case SM4_OPCODE_ITOF:
				case SM4_OPCODE_UTOF:
					break;
				OP1_(FTOI, TRUNC);
				OP1_(FTOU, FLR);
				// integer
				OP2_(IADD, ADD);
				OP2_(IMUL, MUL);
				OP3_(IMAD, MAD);
				OP2_(MIN, MIN);
				OP2_(MAX, MAX);
				OP2_(ILT, SLT);
				OP2_(IGE, SGE);
				OP2_(IEQ, SEQ);
				OP2_(INE, SNE);

				// unsigned
				OP2_(UMUL, MUL);
				OP3_(UMAD, MAD);
				OP2_(UMIN, MIN);
				OP2_(UMAX, MAX);
				OP2_(ULT, SLT);
				OP2_(UGE, SGE);

				case SM4_OPCODE_INEG:
					ureg_MOV(ureg, _dst(), ureg_negate(_src(1)));
					break;
				case SM4_OPCODE_ISHL:
				{
					struct ureg_dst d = _dst();
					struct ureg_dst t = _tmp(d);
					ureg_EX2(ureg, t, _src(2));
					ureg_MUL(ureg, d, ureg_src(t), _src(1));
					break;
				}
				case SM4_OPCODE_ISHR:
				case SM4_OPCODE_USHR:
				{
					struct ureg_dst d = _dst();
					struct ureg_dst t = _tmp(d);
					ureg_EX2(ureg, t, ureg_negate(_src(2)));
					ureg_MUL(ureg, t, ureg_src(t), _src(1));
					ureg_FLR(ureg, d, ureg_src(t));
					break;
				}
				case SM4_OPCODE_UDIV:
				{
					struct ureg_dst q = _dst(0);
					struct ureg_dst r = _dst(1);
					struct ureg_src a = _src(2);
					struct ureg_src b = _src(3);
					struct ureg_dst f = _tmp();
					ureg_DIV(ureg, f, a, b);
					if(q.File != TGSI_FILE_NULL)
						ureg_FLR(ureg, q, ureg_src(f));
					if(r.File != TGSI_FILE_NULL)
					{
						ureg_FRC(ureg, f, ureg_src(f));
						ureg_MUL(ureg, r, ureg_src(f), b);
					}
					break;
				}
				default:
					ok = false;
				}
			}

			check(ok);

			if(!insn_tmps.empty())
			{
				for(unsigned i = 0; i < insn_tmps.size(); ++i)
					ureg_release_temporary(ureg, insn_tmps[i]);
				insn_tmps.clear();
			}
next:;
		}
	}

	void* do_translate()
	{
		unsigned processor;
		switch(program.version.type)
		{
		case 0:
			processor = TGSI_PROCESSOR_FRAGMENT;
			break;
		case 1:
			processor = TGSI_PROCESSOR_VERTEX;
			break;
		case 2:
			processor = TGSI_PROCESSOR_GEOMETRY;
			break;
		default:
			fail("Tessellation and compute shaders not yet supported");
			return 0;
		}

		if(!sm4_link_cf_insns(program))
			fail("Malformed control flow");
		if(!sm4_find_labels(program))
			fail("Failed to locate labels");

		ureg = ureg_create(processor);

		in_sub = false;

		sm4_to_tgsi_insn_num.resize(program.insns.size());
		for(unsigned insn_num = 0; insn_num < program.dcls.size(); ++insn_num)
		{
			sm4_dcl& dcl = *program.dcls[insn_num];
			int idx = -1;
			if(dcl.op.get() && dcl.op->is_index_simple(0))
				idx = dcl.op->indices[0].disp;
			switch(dcl.opcode)
			{
			case SM4_OPCODE_DCL_GLOBAL_FLAGS:
				break;
			case SM4_OPCODE_DCL_TEMPS:
				for(unsigned i = 0; i < dcl.num; ++i)
					temps.push_back(ureg_DECL_temporary(ureg));
				break;
			case SM4_OPCODE_DCL_INPUT:
				check(idx >= 0);
				if(processor == TGSI_PROCESSOR_VERTEX)
				{
					if(inputs.size() <= (unsigned)idx)
						inputs.resize(idx + 1);
					inputs[idx] = ureg_DECL_vs_input(ureg, idx);
				}
				else if(processor == TGSI_PROCESSOR_GEOMETRY)
				{
					// TODO: is this correct?
					unsigned gsidx = dcl.op->indices[1].disp;
					if(inputs.size() <= (unsigned)gsidx)
						inputs.resize(gsidx + 1);
					inputs[gsidx] = ureg_DECL_gs_input(ureg, gsidx, TGSI_SEMANTIC_GENERIC, gsidx);
				}
				else
					check(0);
				break;
			case SM4_OPCODE_DCL_INPUT_PS:
				check(idx >= 0);
				if(inputs.size() <= (unsigned)idx)
					inputs.resize(idx + 1);
				inputs[idx] = ureg_DECL_fs_input_cyl_centroid(ureg, TGSI_SEMANTIC_GENERIC, idx, sm4_to_pipe_interpolation[dcl.dcl_input_ps.interpolation].interpolation, 0, sm4_to_pipe_interpolation[dcl.dcl_input_ps.interpolation].centroid);
				break;
			case SM4_OPCODE_DCL_OUTPUT:
				check(idx >= 0);
				if(outputs.size() <= (unsigned)idx)
					outputs.resize(idx + 1);
				if(processor == TGSI_PROCESSOR_FRAGMENT)
					outputs[idx] = ureg_DECL_output(ureg, TGSI_SEMANTIC_COLOR, idx);
				else
					outputs[idx] = ureg_DECL_output(ureg, TGSI_SEMANTIC_GENERIC, idx);
				break;
			case SM4_OPCODE_DCL_INPUT_SIV:
			case SM4_OPCODE_DCL_INPUT_SGV:
			case SM4_OPCODE_DCL_INPUT_PS_SIV:
			case SM4_OPCODE_DCL_INPUT_PS_SGV:
				check(idx >= 0);
				if(inputs.size() <= (unsigned)idx)
					inputs.resize(idx + 1);
				// TODO: is this correct?
				inputs[idx] = ureg_DECL_system_value(ureg, idx, sm4_to_pipe_sv[dcl.sv], 0);
				break;
			case SM4_OPCODE_DCL_OUTPUT_SIV:
			case SM4_OPCODE_DCL_OUTPUT_SGV:
				check(idx >= 0);
				if(outputs.size() <= (unsigned)idx)
					outputs.resize(idx + 1);
				check(sm4_to_pipe_sv[dcl.sv] >= 0);
				outputs[idx] = ureg_DECL_output(ureg, sm4_to_pipe_sv[dcl.sv], 0);
				break;
			case SM4_OPCODE_DCL_RESOURCE:
				check(idx >= 0);
				if(targets.size() <= (unsigned)idx)
					targets.resize(idx + 1);
				switch(dcl.dcl_resource.target)
				{
				case SM4_TARGET_TEXTURE1D:
					targets[idx].first = TGSI_TEXTURE_1D;
					targets[idx].second = TGSI_TEXTURE_SHADOW1D;
					break;
				case SM4_TARGET_TEXTURE1DARRAY:
					targets[idx].first = TGSI_TEXTURE_1D_ARRAY;
					targets[idx].second = TGSI_TEXTURE_SHADOW1D_ARRAY;
					break;
				case SM4_TARGET_TEXTURE2D:
					targets[idx].first = TGSI_TEXTURE_2D;
					targets[idx].second = TGSI_TEXTURE_SHADOW2D;
					break;
				case SM4_TARGET_TEXTURE2DARRAY:
					targets[idx].first = TGSI_TEXTURE_2D_ARRAY;
					targets[idx].second = TGSI_TEXTURE_SHADOW2D_ARRAY;
					break;
				case SM4_TARGET_TEXTURE3D:
					targets[idx].first = TGSI_TEXTURE_3D;
					targets[idx].second = 0;
					break;
				case SM4_TARGET_TEXTURECUBE:
					targets[idx].first = TGSI_TEXTURE_CUBE;
					targets[idx].second = 0;
					break;
				default:
					// HACK to make SimpleSample10 work
					//check(0);
					targets[idx].first = TGSI_TEXTURE_2D;
					targets[idx].second = TGSI_TEXTURE_SHADOW2D;
					break;
				}
				if(resources.size() <= (unsigned)idx)
					resources.resize(idx + 1);
				resources[idx] = ureg_DECL_sampler_view(
                                   ureg, idx, targets[idx].first,
                                   res_return_type(dcl.rrt.x),
                                   res_return_type(dcl.rrt.y),
                                   res_return_type(dcl.rrt.z),
                                   res_return_type(dcl.rrt.w));
				break;
			case SM4_OPCODE_DCL_SAMPLER:
				check(idx >= 0);
				if(sampler_modes.size() <= (unsigned)idx)
					sampler_modes.resize(idx + 1);
				check(!dcl.dcl_sampler.mono);
				sampler_modes[idx] = dcl.dcl_sampler.shadow;
				if(samplers.size() <= (unsigned)idx)
					samplers.resize(idx + 1);
				samplers[idx] = ureg_DECL_sampler(ureg, idx);
				break;
			case SM4_OPCODE_DCL_CONSTANT_BUFFER:
				check(dcl.op->num_indices == 2);
				check(dcl.op->is_index_simple(0));
				check(dcl.op->is_index_simple(1));
				idx = dcl.op->indices[0].disp;
				ureg_DECL_constant2D(ureg, 0, (unsigned)dcl.op->indices[1].disp - 1, idx);
				break;
			case SM4_OPCODE_DCL_GS_INPUT_PRIMITIVE:
				ureg_property_gs_input_prim(ureg, d3d_to_pipe_prim_type[dcl.dcl_gs_input_primitive.primitive]);
				break;
			case SM4_OPCODE_DCL_GS_OUTPUT_PRIMITIVE_TOPOLOGY:
				ureg_property_gs_output_prim(ureg, d3d_to_pipe_prim[dcl.dcl_gs_output_primitive_topology.primitive_topology]);
				break;
			case SM4_OPCODE_DCL_MAX_OUTPUT_VERTEX_COUNT:
				ureg_property_gs_max_vertices(ureg, dcl.num);
				break;
			default:
				check(0);
			}
		}

		translate_insns(0, program.insns.size());
		sm4_to_tgsi_insn_num.push_back(ureg_get_instruction_number(ureg));
		if(in_sub)
			ureg_ENDSUB(ureg);
		else
			ureg_END(ureg);

		for(unsigned i = 0; i < label_to_sm4_insn_num.size(); ++i)
			ureg_fixup_label(ureg, label_to_sm4_insn_num[i].first, sm4_to_tgsi_insn_num[label_to_sm4_insn_num[i].second]);

		const struct tgsi_token * tokens = ureg_get_tokens(ureg, 0);
		ureg_destroy(ureg);
		return (void*)tokens;
	}

	void* translate()
	{
		try
		{
			return do_translate();
		}
		catch(const char*)
		{
			return 0;
		}
	}
};

void* sm4_to_tgsi(struct sm4_program& program)
{
	sm4_to_tgsi_converter conv(program);
	return conv.translate();
}

void* sm4_to_tgsi_linkage_only(struct sm4_program& prog)
{
	struct ureg_program* ureg = ureg_create(TGSI_PROCESSOR_GEOMETRY);

	uint64_t already = 0;
	for(unsigned n = 0, i = 0; i < prog.num_params_out; ++i)
	{
		unsigned sn, si;

		if(already & (1ULL << prog.params_out[i].Register))
			continue;
		already |= 1ULL << prog.params_out[i].Register;

		switch(prog.params_out[i].SystemValueType)
		{
		case D3D_NAME_UNDEFINED:
			sn = TGSI_SEMANTIC_GENERIC;
			si = n++;
			break;
		case D3D_NAME_CULL_DISTANCE:
		case D3D_NAME_CLIP_DISTANCE:
			// FIXME
			sn = 0;
			si = prog.params_out[i].SemanticIndex;
			assert(0);
			break;
		default:
			continue;
		}

		ureg_DECL_output(ureg, sn, si);
	}

	const struct tgsi_token* tokens = ureg_get_tokens(ureg, 0);
	ureg_destroy(ureg);
	return (void*)tokens;
}
