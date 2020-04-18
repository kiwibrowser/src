// Copyright 2016 The SwiftShader Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "VertexProgram.hpp"

#include "VertexShader.hpp"
#include "SamplerCore.hpp"
#include "Renderer/Renderer.hpp"
#include "Renderer/Vertex.hpp"
#include "Common/Half.hpp"
#include "Common/Debug.hpp"

namespace sw
{
	VertexProgram::VertexProgram(const VertexProcessor::State &state, const VertexShader *shader)
		: VertexRoutine(state, shader), shader(shader), r(shader->dynamicallyIndexedTemporaries)
	{
		ifDepth = 0;
		loopRepDepth = 0;
		currentLabel = -1;
		whileTest = false;

		for(int i = 0; i < 2048; i++)
		{
			labelBlock[i] = 0;
		}

		loopDepth = -1;
		enableStack[0] = Int4(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);

		if(shader->containsBreakInstruction())
		{
			enableBreak = Int4(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);
		}

		if(shader->containsContinueInstruction())
		{
			enableContinue = Int4(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);
		}

		if(shader->isInstanceIdDeclared())
		{
			instanceID = *Pointer<Int>(data + OFFSET(DrawData,instanceID));
		}
	}

	VertexProgram::~VertexProgram()
	{
	}

	void VertexProgram::pipeline(UInt& index)
	{
		if(!state.preTransformed)
		{
			program(index);
		}
		else
		{
			passThrough();
		}
	}

	void VertexProgram::program(UInt& index)
	{
	//	shader->print("VertexShader-%0.8X.txt", state.shaderID);

		unsigned short shaderModel = shader->getShaderModel();

		enableIndex = 0;
		stackIndex = 0;

		if(shader->containsLeaveInstruction())
		{
			enableLeave = Int4(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);
		}

		if(shader->isVertexIdDeclared())
		{
			if(state.textureSampling)
			{
				vertexID = Int4(index);
			}
			else
			{
				vertexID = Insert(vertexID, As<Int>(index), 0);
				vertexID = Insert(vertexID, As<Int>(index + 1), 1);
				vertexID = Insert(vertexID, As<Int>(index + 2), 2);
				vertexID = Insert(vertexID, As<Int>(index + 3), 3);
			}
		}

		// Create all call site return blocks up front
		for(size_t i = 0; i < shader->getLength(); i++)
		{
			const Shader::Instruction *instruction = shader->getInstruction(i);
			Shader::Opcode opcode = instruction->opcode;

			if(opcode == Shader::OPCODE_CALL || opcode == Shader::OPCODE_CALLNZ)
			{
				const Dst &dst = instruction->dst;

				ASSERT(callRetBlock[dst.label].size() == dst.callSite);
				callRetBlock[dst.label].push_back(Nucleus::createBasicBlock());
			}
		}

		for(size_t i = 0; i < shader->getLength(); i++)
		{
			const Shader::Instruction *instruction = shader->getInstruction(i);
			Shader::Opcode opcode = instruction->opcode;

			if(opcode == Shader::OPCODE_DCL || opcode == Shader::OPCODE_DEF || opcode == Shader::OPCODE_DEFI || opcode == Shader::OPCODE_DEFB)
			{
				continue;
			}

			Dst dst = instruction->dst;
			Src src0 = instruction->src[0];
			Src src1 = instruction->src[1];
			Src src2 = instruction->src[2];
			Src src3 = instruction->src[3];
			Src src4 = instruction->src[4];

			bool predicate = instruction->predicate;
			Control control = instruction->control;
			bool integer = dst.type == Shader::PARAMETER_ADDR;
			bool pp = dst.partialPrecision;

			Vector4f d;
			Vector4f s0;
			Vector4f s1;
			Vector4f s2;
			Vector4f s3;
			Vector4f s4;

			if(src0.type != Shader::PARAMETER_VOID) s0 = fetchRegister(src0);
			if(src1.type != Shader::PARAMETER_VOID) s1 = fetchRegister(src1);
			if(src2.type != Shader::PARAMETER_VOID) s2 = fetchRegister(src2);
			if(src3.type != Shader::PARAMETER_VOID) s3 = fetchRegister(src3);
			if(src4.type != Shader::PARAMETER_VOID) s4 = fetchRegister(src4);

			switch(opcode)
			{
			case Shader::OPCODE_VS_1_0:                                     break;
			case Shader::OPCODE_VS_1_1:                                     break;
			case Shader::OPCODE_VS_2_0:                                     break;
			case Shader::OPCODE_VS_2_x:                                     break;
			case Shader::OPCODE_VS_2_sw:                                    break;
			case Shader::OPCODE_VS_3_0:                                     break;
			case Shader::OPCODE_VS_3_sw:                                    break;
			case Shader::OPCODE_DCL:                                        break;
			case Shader::OPCODE_DEF:                                        break;
			case Shader::OPCODE_DEFI:                                       break;
			case Shader::OPCODE_DEFB:                                       break;
			case Shader::OPCODE_NOP:                                        break;
			case Shader::OPCODE_ABS:        abs(d, s0);                     break;
			case Shader::OPCODE_IABS:       iabs(d, s0);                    break;
			case Shader::OPCODE_ADD:        add(d, s0, s1);                 break;
			case Shader::OPCODE_IADD:       iadd(d, s0, s1);                break;
			case Shader::OPCODE_CRS:        crs(d, s0, s1);                 break;
			case Shader::OPCODE_FORWARD1:   forward1(d, s0, s1, s2);        break;
			case Shader::OPCODE_FORWARD2:   forward2(d, s0, s1, s2);        break;
			case Shader::OPCODE_FORWARD3:   forward3(d, s0, s1, s2);        break;
			case Shader::OPCODE_FORWARD4:   forward4(d, s0, s1, s2);        break;
			case Shader::OPCODE_REFLECT1:   reflect1(d, s0, s1);            break;
			case Shader::OPCODE_REFLECT2:   reflect2(d, s0, s1);            break;
			case Shader::OPCODE_REFLECT3:   reflect3(d, s0, s1);            break;
			case Shader::OPCODE_REFLECT4:   reflect4(d, s0, s1);            break;
			case Shader::OPCODE_REFRACT1:   refract1(d, s0, s1, s2.x);      break;
			case Shader::OPCODE_REFRACT2:   refract2(d, s0, s1, s2.x);      break;
			case Shader::OPCODE_REFRACT3:   refract3(d, s0, s1, s2.x);      break;
			case Shader::OPCODE_REFRACT4:   refract4(d, s0, s1, s2.x);      break;
			case Shader::OPCODE_DP1:        dp1(d, s0, s1);                 break;
			case Shader::OPCODE_DP2:        dp2(d, s0, s1);                 break;
			case Shader::OPCODE_DP3:        dp3(d, s0, s1);                 break;
			case Shader::OPCODE_DP4:        dp4(d, s0, s1);                 break;
			case Shader::OPCODE_DET2:       det2(d, s0, s1);                break;
			case Shader::OPCODE_DET3:       det3(d, s0, s1, s2);            break;
			case Shader::OPCODE_DET4:       det4(d, s0, s1, s2, s3);        break;
			case Shader::OPCODE_ATT:        att(d, s0, s1);                 break;
			case Shader::OPCODE_EXP2X:      exp2x(d, s0, pp);               break;
			case Shader::OPCODE_EXP2:       exp2(d, s0, pp);                break;
			case Shader::OPCODE_EXPP:       expp(d, s0, shaderModel);       break;
			case Shader::OPCODE_EXP:        exp(d, s0, pp);                 break;
			case Shader::OPCODE_FRC:        frc(d, s0);                     break;
			case Shader::OPCODE_TRUNC:      trunc(d, s0);                   break;
			case Shader::OPCODE_FLOOR:      floor(d, s0);                   break;
			case Shader::OPCODE_ROUND:      round(d, s0);                   break;
			case Shader::OPCODE_ROUNDEVEN:  roundEven(d, s0);               break;
			case Shader::OPCODE_CEIL:       ceil(d, s0);                    break;
			case Shader::OPCODE_LIT:        lit(d, s0);                     break;
			case Shader::OPCODE_LOG2X:      log2x(d, s0, pp);               break;
			case Shader::OPCODE_LOG2:       log2(d, s0, pp);                break;
			case Shader::OPCODE_LOGP:       logp(d, s0, shaderModel);       break;
			case Shader::OPCODE_LOG:        log(d, s0, pp);                 break;
			case Shader::OPCODE_LRP:        lrp(d, s0, s1, s2);             break;
			case Shader::OPCODE_STEP:       step(d, s0, s1);                break;
			case Shader::OPCODE_SMOOTH:     smooth(d, s0, s1, s2);          break;
			case Shader::OPCODE_ISINF:      isinf(d, s0);                   break;
			case Shader::OPCODE_ISNAN:      isnan(d, s0);                   break;
			case Shader::OPCODE_FLOATBITSTOINT:
			case Shader::OPCODE_FLOATBITSTOUINT:
			case Shader::OPCODE_INTBITSTOFLOAT:
			case Shader::OPCODE_UINTBITSTOFLOAT: d = s0;                    break;
			case Shader::OPCODE_PACKSNORM2x16:   packSnorm2x16(d, s0);      break;
			case Shader::OPCODE_PACKUNORM2x16:   packUnorm2x16(d, s0);      break;
			case Shader::OPCODE_PACKHALF2x16:    packHalf2x16(d, s0);       break;
			case Shader::OPCODE_UNPACKSNORM2x16: unpackSnorm2x16(d, s0);    break;
			case Shader::OPCODE_UNPACKUNORM2x16: unpackUnorm2x16(d, s0);    break;
			case Shader::OPCODE_UNPACKHALF2x16:  unpackHalf2x16(d, s0);     break;
			case Shader::OPCODE_M3X2:       M3X2(d, s0, src1);              break;
			case Shader::OPCODE_M3X3:       M3X3(d, s0, src1);              break;
			case Shader::OPCODE_M3X4:       M3X4(d, s0, src1);              break;
			case Shader::OPCODE_M4X3:       M4X3(d, s0, src1);              break;
			case Shader::OPCODE_M4X4:       M4X4(d, s0, src1);              break;
			case Shader::OPCODE_MAD:        mad(d, s0, s1, s2);             break;
			case Shader::OPCODE_IMAD:       imad(d, s0, s1, s2);            break;
			case Shader::OPCODE_MAX:        max(d, s0, s1);                 break;
			case Shader::OPCODE_IMAX:       imax(d, s0, s1);                break;
			case Shader::OPCODE_UMAX:       umax(d, s0, s1);                break;
			case Shader::OPCODE_MIN:        min(d, s0, s1);                 break;
			case Shader::OPCODE_IMIN:       imin(d, s0, s1);                break;
			case Shader::OPCODE_UMIN:       umin(d, s0, s1);                break;
			case Shader::OPCODE_MOV:        mov(d, s0, integer);            break;
			case Shader::OPCODE_MOVA:       mov(d, s0, true);               break;
			case Shader::OPCODE_NEG:        neg(d, s0);                     break;
			case Shader::OPCODE_INEG:       ineg(d, s0);                    break;
			case Shader::OPCODE_F2B:        f2b(d, s0);                     break;
			case Shader::OPCODE_B2F:        b2f(d, s0);                     break;
			case Shader::OPCODE_F2I:        f2i(d, s0);                     break;
			case Shader::OPCODE_I2F:        i2f(d, s0);                     break;
			case Shader::OPCODE_F2U:        f2u(d, s0);                     break;
			case Shader::OPCODE_U2F:        u2f(d, s0);                     break;
			case Shader::OPCODE_I2B:        i2b(d, s0);                     break;
			case Shader::OPCODE_B2I:        b2i(d, s0);                     break;
			case Shader::OPCODE_MUL:        mul(d, s0, s1);                 break;
			case Shader::OPCODE_IMUL:       imul(d, s0, s1);                break;
			case Shader::OPCODE_NRM2:       nrm2(d, s0, pp);                break;
			case Shader::OPCODE_NRM3:       nrm3(d, s0, pp);                break;
			case Shader::OPCODE_NRM4:       nrm4(d, s0, pp);                break;
			case Shader::OPCODE_POWX:       powx(d, s0, s1, pp);            break;
			case Shader::OPCODE_POW:        pow(d, s0, s1, pp);             break;
			case Shader::OPCODE_RCPX:       rcpx(d, s0, pp);                break;
			case Shader::OPCODE_DIV:        div(d, s0, s1);                 break;
			case Shader::OPCODE_IDIV:       idiv(d, s0, s1);                break;
			case Shader::OPCODE_UDIV:       udiv(d, s0, s1);                break;
			case Shader::OPCODE_MOD:        mod(d, s0, s1);                 break;
			case Shader::OPCODE_IMOD:       imod(d, s0, s1);                break;
			case Shader::OPCODE_UMOD:       umod(d, s0, s1);                break;
			case Shader::OPCODE_SHL:        shl(d, s0, s1);                 break;
			case Shader::OPCODE_ISHR:       ishr(d, s0, s1);                break;
			case Shader::OPCODE_USHR:       ushr(d, s0, s1);                break;
			case Shader::OPCODE_RSQX:       rsqx(d, s0, pp);                break;
			case Shader::OPCODE_SQRT:       sqrt(d, s0, pp);                break;
			case Shader::OPCODE_RSQ:        rsq(d, s0, pp);                 break;
			case Shader::OPCODE_LEN2:       len2(d.x, s0, pp);              break;
			case Shader::OPCODE_LEN3:       len3(d.x, s0, pp);              break;
			case Shader::OPCODE_LEN4:       len4(d.x, s0, pp);              break;
			case Shader::OPCODE_DIST1:      dist1(d.x, s0, s1, pp);         break;
			case Shader::OPCODE_DIST2:      dist2(d.x, s0, s1, pp);         break;
			case Shader::OPCODE_DIST3:      dist3(d.x, s0, s1, pp);         break;
			case Shader::OPCODE_DIST4:      dist4(d.x, s0, s1, pp);         break;
			case Shader::OPCODE_SGE:        step(d, s1, s0);                break;
			case Shader::OPCODE_SGN:        sgn(d, s0);                     break;
			case Shader::OPCODE_ISGN:       isgn(d, s0);                    break;
			case Shader::OPCODE_SINCOS:     sincos(d, s0, pp);              break;
			case Shader::OPCODE_COS:        cos(d, s0, pp);                 break;
			case Shader::OPCODE_SIN:        sin(d, s0, pp);                 break;
			case Shader::OPCODE_TAN:        tan(d, s0);                     break;
			case Shader::OPCODE_ACOS:       acos(d, s0);                    break;
			case Shader::OPCODE_ASIN:       asin(d, s0);                    break;
			case Shader::OPCODE_ATAN:       atan(d, s0);                    break;
			case Shader::OPCODE_ATAN2:      atan2(d, s0, s1);               break;
			case Shader::OPCODE_COSH:       cosh(d, s0, pp);                break;
			case Shader::OPCODE_SINH:       sinh(d, s0, pp);                break;
			case Shader::OPCODE_TANH:       tanh(d, s0, pp);                break;
			case Shader::OPCODE_ACOSH:      acosh(d, s0, pp);               break;
			case Shader::OPCODE_ASINH:      asinh(d, s0, pp);               break;
			case Shader::OPCODE_ATANH:      atanh(d, s0, pp);               break;
			case Shader::OPCODE_SLT:        slt(d, s0, s1);                 break;
			case Shader::OPCODE_SUB:        sub(d, s0, s1);                 break;
			case Shader::OPCODE_ISUB:       isub(d, s0, s1);                break;
			case Shader::OPCODE_BREAK:      BREAK();                        break;
			case Shader::OPCODE_BREAKC:     BREAKC(s0, s1, control);        break;
			case Shader::OPCODE_BREAKP:     BREAKP(src0);                   break;
			case Shader::OPCODE_CONTINUE:   CONTINUE();                     break;
			case Shader::OPCODE_TEST:       TEST();                         break;
			case Shader::OPCODE_CALL:       CALL(dst.label, dst.callSite);  break;
			case Shader::OPCODE_CALLNZ:     CALLNZ(dst.label, dst.callSite, src0); break;
			case Shader::OPCODE_ELSE:       ELSE();                         break;
			case Shader::OPCODE_ENDIF:      ENDIF();                        break;
			case Shader::OPCODE_ENDLOOP:    ENDLOOP();                      break;
			case Shader::OPCODE_ENDREP:     ENDREP();                       break;
			case Shader::OPCODE_ENDWHILE:   ENDWHILE();                     break;
			case Shader::OPCODE_ENDSWITCH:  ENDSWITCH();                    break;
			case Shader::OPCODE_IF:         IF(src0);                       break;
			case Shader::OPCODE_IFC:        IFC(s0, s1, control);           break;
			case Shader::OPCODE_LABEL:      LABEL(dst.index);               break;
			case Shader::OPCODE_LOOP:       LOOP(src1);                     break;
			case Shader::OPCODE_REP:        REP(src0);                      break;
			case Shader::OPCODE_WHILE:      WHILE(src0);                    break;
			case Shader::OPCODE_SWITCH:     SWITCH();                       break;
			case Shader::OPCODE_RET:        RET();                          break;
			case Shader::OPCODE_LEAVE:      LEAVE();                        break;
			case Shader::OPCODE_CMP:        cmp(d, s0, s1, control);        break;
			case Shader::OPCODE_ICMP:       icmp(d, s0, s1, control);       break;
			case Shader::OPCODE_UCMP:       ucmp(d, s0, s1, control);       break;
			case Shader::OPCODE_SELECT:     select(d, s0, s1, s2);          break;
			case Shader::OPCODE_EXTRACT:    extract(d.x, s0, s1.x);         break;
			case Shader::OPCODE_INSERT:     insert(d, s0, s1.x, s2.x);      break;
			case Shader::OPCODE_ALL:        all(d.x, s0);                   break;
			case Shader::OPCODE_ANY:        any(d.x, s0);                   break;
			case Shader::OPCODE_NOT:        bitwise_not(d, s0);             break;
			case Shader::OPCODE_OR:         bitwise_or(d, s0, s1);          break;
			case Shader::OPCODE_XOR:        bitwise_xor(d, s0, s1);         break;
			case Shader::OPCODE_AND:        bitwise_and(d, s0, s1);         break;
			case Shader::OPCODE_EQ:         equal(d, s0, s1);               break;
			case Shader::OPCODE_NE:         notEqual(d, s0, s1);            break;
			case Shader::OPCODE_TEXLDL:     TEXLOD(d, s0, src1, s0.w);      break;
			case Shader::OPCODE_TEXLOD:     TEXLOD(d, s0, src1, s2.x);      break;
			case Shader::OPCODE_TEX:        TEX(d, s0, src1);               break;
			case Shader::OPCODE_TEXOFFSET:  TEXOFFSET(d, s0, src1, s2);     break;
			case Shader::OPCODE_TEXLODOFFSET: TEXLODOFFSET(d, s0, src1, s2, s3.x); break;
			case Shader::OPCODE_TEXELFETCH: TEXELFETCH(d, s0, src1, s2.x);  break;
			case Shader::OPCODE_TEXELFETCHOFFSET: TEXELFETCHOFFSET(d, s0, src1, s2, s3.x); break;
			case Shader::OPCODE_TEXGRAD:    TEXGRAD(d, s0, src1, s2, s3);   break;
			case Shader::OPCODE_TEXGRADOFFSET: TEXGRADOFFSET(d, s0, src1, s2, s3, s4); break;
			case Shader::OPCODE_TEXSIZE:    TEXSIZE(d, s0.x, src1);         break;
			case Shader::OPCODE_END:                                        break;
			default:
				ASSERT(false);
			}

			if(dst.type != Shader::PARAMETER_VOID && dst.type != Shader::PARAMETER_LABEL && opcode != Shader::OPCODE_NOP)
			{
				if(dst.saturate)
				{
					if(dst.x) d.x = Max(d.x, Float4(0.0f));
					if(dst.y) d.y = Max(d.y, Float4(0.0f));
					if(dst.z) d.z = Max(d.z, Float4(0.0f));
					if(dst.w) d.w = Max(d.w, Float4(0.0f));

					if(dst.x) d.x = Min(d.x, Float4(1.0f));
					if(dst.y) d.y = Min(d.y, Float4(1.0f));
					if(dst.z) d.z = Min(d.z, Float4(1.0f));
					if(dst.w) d.w = Min(d.w, Float4(1.0f));
				}

				if(instruction->isPredicated())
				{
					Vector4f pDst;   // FIXME: Rename

					switch(dst.type)
					{
					case Shader::PARAMETER_VOID: break;
					case Shader::PARAMETER_TEMP:
						if(dst.rel.type == Shader::PARAMETER_VOID)
						{
							if(dst.x) pDst.x = r[dst.index].x;
							if(dst.y) pDst.y = r[dst.index].y;
							if(dst.z) pDst.z = r[dst.index].z;
							if(dst.w) pDst.w = r[dst.index].w;
						}
						else
						{
							Int a = relativeAddress(dst);

							if(dst.x) pDst.x = r[dst.index + a].x;
							if(dst.y) pDst.y = r[dst.index + a].y;
							if(dst.z) pDst.z = r[dst.index + a].z;
							if(dst.w) pDst.w = r[dst.index + a].w;
						}
						break;
					case Shader::PARAMETER_ADDR: pDst = a0; break;
					case Shader::PARAMETER_RASTOUT:
						switch(dst.index)
						{
						case 0:
							if(dst.x) pDst.x = o[Pos].x;
							if(dst.y) pDst.y = o[Pos].y;
							if(dst.z) pDst.z = o[Pos].z;
							if(dst.w) pDst.w = o[Pos].w;
							break;
						case 1:
							pDst.x = o[Fog].x;
							break;
						case 2:
							pDst.x = o[Pts].y;
							break;
						default:
							ASSERT(false);
						}
						break;
					case Shader::PARAMETER_ATTROUT:
						if(dst.x) pDst.x = o[C0 + dst.index].x;
						if(dst.y) pDst.y = o[C0 + dst.index].y;
						if(dst.z) pDst.z = o[C0 + dst.index].z;
						if(dst.w) pDst.w = o[C0 + dst.index].w;
						break;
					case Shader::PARAMETER_TEXCRDOUT:
				//	case Shader::PARAMETER_OUTPUT:
						if(shaderModel < 0x0300)
						{
							if(dst.x) pDst.x = o[T0 + dst.index].x;
							if(dst.y) pDst.y = o[T0 + dst.index].y;
							if(dst.z) pDst.z = o[T0 + dst.index].z;
							if(dst.w) pDst.w = o[T0 + dst.index].w;
						}
						else
						{
							if(dst.rel.type == Shader::PARAMETER_VOID)   // Not relative
							{
								if(dst.x) pDst.x = o[dst.index].x;
								if(dst.y) pDst.y = o[dst.index].y;
								if(dst.z) pDst.z = o[dst.index].z;
								if(dst.w) pDst.w = o[dst.index].w;
							}
							else
							{
								Int a = relativeAddress(dst);

								if(dst.x) pDst.x = o[dst.index + a].x;
								if(dst.y) pDst.y = o[dst.index + a].y;
								if(dst.z) pDst.z = o[dst.index + a].z;
								if(dst.w) pDst.w = o[dst.index + a].w;
							}
						}
						break;
					case Shader::PARAMETER_LABEL:                break;
					case Shader::PARAMETER_PREDICATE: pDst = p0; break;
					case Shader::PARAMETER_INPUT:                break;
					default:
						ASSERT(false);
					}

					Int4 enable = enableMask(instruction);

					Int4 xEnable = enable;
					Int4 yEnable = enable;
					Int4 zEnable = enable;
					Int4 wEnable = enable;

					if(predicate)
					{
						unsigned char pSwizzle = instruction->predicateSwizzle;

						Float4 xPredicate = p0[(pSwizzle >> 0) & 0x03];
						Float4 yPredicate = p0[(pSwizzle >> 2) & 0x03];
						Float4 zPredicate = p0[(pSwizzle >> 4) & 0x03];
						Float4 wPredicate = p0[(pSwizzle >> 6) & 0x03];

						if(!instruction->predicateNot)
						{
							if(dst.x) xEnable = xEnable & As<Int4>(xPredicate);
							if(dst.y) yEnable = yEnable & As<Int4>(yPredicate);
							if(dst.z) zEnable = zEnable & As<Int4>(zPredicate);
							if(dst.w) wEnable = wEnable & As<Int4>(wPredicate);
						}
						else
						{
							if(dst.x) xEnable = xEnable & ~As<Int4>(xPredicate);
							if(dst.y) yEnable = yEnable & ~As<Int4>(yPredicate);
							if(dst.z) zEnable = zEnable & ~As<Int4>(zPredicate);
							if(dst.w) wEnable = wEnable & ~As<Int4>(wPredicate);
						}
					}

					if(dst.x) d.x = As<Float4>(As<Int4>(d.x) & xEnable);
					if(dst.y) d.y = As<Float4>(As<Int4>(d.y) & yEnable);
					if(dst.z) d.z = As<Float4>(As<Int4>(d.z) & zEnable);
					if(dst.w) d.w = As<Float4>(As<Int4>(d.w) & wEnable);

					if(dst.x) d.x = As<Float4>(As<Int4>(d.x) | (As<Int4>(pDst.x) & ~xEnable));
					if(dst.y) d.y = As<Float4>(As<Int4>(d.y) | (As<Int4>(pDst.y) & ~yEnable));
					if(dst.z) d.z = As<Float4>(As<Int4>(d.z) | (As<Int4>(pDst.z) & ~zEnable));
					if(dst.w) d.w = As<Float4>(As<Int4>(d.w) | (As<Int4>(pDst.w) & ~wEnable));
				}

				switch(dst.type)
				{
				case Shader::PARAMETER_VOID:
					break;
				case Shader::PARAMETER_TEMP:
					if(dst.rel.type == Shader::PARAMETER_VOID)
					{
						if(dst.x) r[dst.index].x = d.x;
						if(dst.y) r[dst.index].y = d.y;
						if(dst.z) r[dst.index].z = d.z;
						if(dst.w) r[dst.index].w = d.w;
					}
					else
					{
						Int a = relativeAddress(dst);

						if(dst.x) r[dst.index + a].x = d.x;
						if(dst.y) r[dst.index + a].y = d.y;
						if(dst.z) r[dst.index + a].z = d.z;
						if(dst.w) r[dst.index + a].w = d.w;
					}
					break;
				case Shader::PARAMETER_ADDR:
					if(dst.x) a0.x = d.x;
					if(dst.y) a0.y = d.y;
					if(dst.z) a0.z = d.z;
					if(dst.w) a0.w = d.w;
					break;
				case Shader::PARAMETER_RASTOUT:
					switch(dst.index)
					{
					case 0:
						if(dst.x) o[Pos].x = d.x;
						if(dst.y) o[Pos].y = d.y;
						if(dst.z) o[Pos].z = d.z;
						if(dst.w) o[Pos].w = d.w;
						break;
					case 1:
						o[Fog].x = d.x;
						break;
					case 2:
						o[Pts].y = d.x;
						break;
					default:	ASSERT(false);
					}
					break;
				case Shader::PARAMETER_ATTROUT:
					if(dst.x) o[C0 + dst.index].x = d.x;
					if(dst.y) o[C0 + dst.index].y = d.y;
					if(dst.z) o[C0 + dst.index].z = d.z;
					if(dst.w) o[C0 + dst.index].w = d.w;
					break;
				case Shader::PARAMETER_TEXCRDOUT:
			//	case Shader::PARAMETER_OUTPUT:
					if(shaderModel < 0x0300)
					{
						if(dst.x) o[T0 + dst.index].x = d.x;
						if(dst.y) o[T0 + dst.index].y = d.y;
						if(dst.z) o[T0 + dst.index].z = d.z;
						if(dst.w) o[T0 + dst.index].w = d.w;
					}
					else
					{
						if(dst.rel.type == Shader::PARAMETER_VOID)   // Not relative
						{
							if(dst.x) o[dst.index].x = d.x;
							if(dst.y) o[dst.index].y = d.y;
							if(dst.z) o[dst.index].z = d.z;
							if(dst.w) o[dst.index].w = d.w;
						}
						else
						{
							Int a = relativeAddress(dst);

							if(dst.x) o[dst.index + a].x = d.x;
							if(dst.y) o[dst.index + a].y = d.y;
							if(dst.z) o[dst.index + a].z = d.z;
							if(dst.w) o[dst.index + a].w = d.w;
						}
					}
					break;
				case Shader::PARAMETER_LABEL:             break;
				case Shader::PARAMETER_PREDICATE: p0 = d; break;
				case Shader::PARAMETER_INPUT:             break;
				default:
					ASSERT(false);
				}
			}
		}

		if(currentLabel != -1)
		{
			Nucleus::setInsertBlock(returnBlock);
		}
	}

	void VertexProgram::passThrough()
	{
		if(shader)
		{
			for(int i = 0; i < MAX_VERTEX_OUTPUTS; i++)
			{
				unsigned char usage = shader->getOutput(i, 0).usage;

				switch(usage)
				{
				case 0xFF:
					continue;
				case Shader::USAGE_PSIZE:
					o[i].y = v[i].x;
					break;
				case Shader::USAGE_TEXCOORD:
					o[i].x = v[i].x;
					o[i].y = v[i].y;
					o[i].z = v[i].z;
					o[i].w = v[i].w;
					break;
				case Shader::USAGE_POSITION:
					o[i].x = v[i].x;
					o[i].y = v[i].y;
					o[i].z = v[i].z;
					o[i].w = v[i].w;
					break;
				case Shader::USAGE_COLOR:
					o[i].x = v[i].x;
					o[i].y = v[i].y;
					o[i].z = v[i].z;
					o[i].w = v[i].w;
					break;
				case Shader::USAGE_FOG:
					o[i].x = v[i].x;
					break;
				default:
					ASSERT(false);
				}
			}
		}
		else
		{
			o[Pos].x = v[PositionT].x;
			o[Pos].y = v[PositionT].y;
			o[Pos].z = v[PositionT].z;
			o[Pos].w = v[PositionT].w;

			for(int i = 0; i < 2; i++)
			{
				o[C0 + i].x = v[Color0 + i].x;
				o[C0 + i].y = v[Color0 + i].y;
				o[C0 + i].z = v[Color0 + i].z;
				o[C0 + i].w = v[Color0 + i].w;
			}

			for(int i = 0; i < 8; i++)
			{
				o[T0 + i].x = v[TexCoord0 + i].x;
				o[T0 + i].y = v[TexCoord0 + i].y;
				o[T0 + i].z = v[TexCoord0 + i].z;
				o[T0 + i].w = v[TexCoord0 + i].w;
			}

			o[Pts].y = v[PointSize].x;
		}
	}

	Vector4f VertexProgram::fetchRegister(const Src &src, unsigned int offset)
	{
		Vector4f reg;
		unsigned int i = src.index + offset;

		switch(src.type)
		{
		case Shader::PARAMETER_TEMP:
			if(src.rel.type == Shader::PARAMETER_VOID)
			{
				reg = r[i];
			}
			else
			{
				reg = r[i + relativeAddress(src, src.bufferIndex)];
			}
			break;
		case Shader::PARAMETER_CONST:
			reg = readConstant(src, offset);
			break;
		case Shader::PARAMETER_INPUT:
			if(src.rel.type == Shader::PARAMETER_VOID)
			{
				reg = v[i];
			}
			else
			{
				reg = v[i + relativeAddress(src, src.bufferIndex)];
			}
			break;
		case Shader::PARAMETER_VOID: return r[0];   // Dummy
		case Shader::PARAMETER_FLOAT4LITERAL:
			reg.x = Float4(src.value[0]);
			reg.y = Float4(src.value[1]);
			reg.z = Float4(src.value[2]);
			reg.w = Float4(src.value[3]);
			break;
		case Shader::PARAMETER_ADDR:      reg = a0; break;
		case Shader::PARAMETER_CONSTBOOL: return r[0];   // Dummy
		case Shader::PARAMETER_CONSTINT:  return r[0];   // Dummy
		case Shader::PARAMETER_LOOP:      return r[0];   // Dummy
		case Shader::PARAMETER_PREDICATE: return r[0];   // Dummy
		case Shader::PARAMETER_SAMPLER:
			if(src.rel.type == Shader::PARAMETER_VOID)
			{
				reg.x = As<Float4>(Int4(i));
			}
			else if(src.rel.type == Shader::PARAMETER_TEMP)
			{
				reg.x = As<Float4>(Int4(i) + As<Int4>(r[src.rel.index].x));
			}
			return reg;
		case Shader::PARAMETER_OUTPUT:
			if(src.rel.type == Shader::PARAMETER_VOID)
			{
				reg = o[i];
			}
			else
			{
				reg = o[i + relativeAddress(src, src.bufferIndex)];
			}
			break;
		case Shader::PARAMETER_MISCTYPE:
			if(src.index == Shader::InstanceIDIndex)
			{
				reg.x = As<Float>(instanceID);
			}
			else if(src.index == Shader::VertexIDIndex)
			{
				reg.x = As<Float4>(vertexID);
			}
			else ASSERT(false);
			return reg;
		default:
			ASSERT(false);
		}

		const Float4 &x = reg[(src.swizzle >> 0) & 0x3];
		const Float4 &y = reg[(src.swizzle >> 2) & 0x3];
		const Float4 &z = reg[(src.swizzle >> 4) & 0x3];
		const Float4 &w = reg[(src.swizzle >> 6) & 0x3];

		Vector4f mod;

		switch(src.modifier)
		{
		case Shader::MODIFIER_NONE:
			mod.x = x;
			mod.y = y;
			mod.z = z;
			mod.w = w;
			break;
		case Shader::MODIFIER_NEGATE:
			mod.x = -x;
			mod.y = -y;
			mod.z = -z;
			mod.w = -w;
			break;
		case Shader::MODIFIER_ABS:
			mod.x = Abs(x);
			mod.y = Abs(y);
			mod.z = Abs(z);
			mod.w = Abs(w);
			break;
		case Shader::MODIFIER_ABS_NEGATE:
			mod.x = -Abs(x);
			mod.y = -Abs(y);
			mod.z = -Abs(z);
			mod.w = -Abs(w);
			break;
		case Shader::MODIFIER_NOT:
			mod.x = As<Float4>(As<Int4>(x) ^ Int4(0xFFFFFFFF));
			mod.y = As<Float4>(As<Int4>(y) ^ Int4(0xFFFFFFFF));
			mod.z = As<Float4>(As<Int4>(z) ^ Int4(0xFFFFFFFF));
			mod.w = As<Float4>(As<Int4>(w) ^ Int4(0xFFFFFFFF));
			break;
		default:
			ASSERT(false);
		}

		return mod;
	}

	RValue<Pointer<Byte>> VertexProgram::uniformAddress(int bufferIndex, unsigned int index)
	{
		if(bufferIndex == -1)
		{
			return data + OFFSET(DrawData, vs.c[index]);
		}
		else
		{
			return *Pointer<Pointer<Byte>>(data + OFFSET(DrawData, vs.u[bufferIndex])) + index;
		}
	}

	RValue<Pointer<Byte>> VertexProgram::uniformAddress(int bufferIndex, unsigned int index, Int& offset)
	{
		return uniformAddress(bufferIndex, index) + offset * sizeof(float4);
	}

	Vector4f VertexProgram::readConstant(const Src &src, unsigned int offset)
	{
		Vector4f c;
		unsigned int i = src.index + offset;

		if(src.rel.type == Shader::PARAMETER_VOID)   // Not relative
		{
			c.x = c.y = c.z = c.w = *Pointer<Float4>(uniformAddress(src.bufferIndex, i));

			c.x = c.x.xxxx;
			c.y = c.y.yyyy;
			c.z = c.z.zzzz;
			c.w = c.w.wwww;

			if(shader->containsDefineInstruction())   // Constant may be known at compile time
			{
				for(size_t j = 0; j < shader->getLength(); j++)
				{
					const Shader::Instruction &instruction = *shader->getInstruction(j);

					if(instruction.opcode == Shader::OPCODE_DEF)
					{
						if(instruction.dst.index == i)
						{
							c.x = Float4(instruction.src[0].value[0]);
							c.y = Float4(instruction.src[0].value[1]);
							c.z = Float4(instruction.src[0].value[2]);
							c.w = Float4(instruction.src[0].value[3]);

							break;
						}
					}
				}
			}
		}
		else if(src.rel.type == Shader::PARAMETER_LOOP)
		{
			Int loopCounter = aL[loopDepth];

			c.x = c.y = c.z = c.w = *Pointer<Float4>(uniformAddress(src.bufferIndex, i, loopCounter));

			c.x = c.x.xxxx;
			c.y = c.y.yyyy;
			c.z = c.z.zzzz;
			c.w = c.w.wwww;
		}
		else
		{
			if(src.rel.deterministic)
			{
				Int a = relativeAddress(src, src.bufferIndex);

				c.x = c.y = c.z = c.w = *Pointer<Float4>(uniformAddress(src.bufferIndex, i, a));

				c.x = c.x.xxxx;
				c.y = c.y.yyyy;
				c.z = c.z.zzzz;
				c.w = c.w.wwww;
			}
			else
			{
				int component = src.rel.swizzle & 0x03;
				Float4 a;

				switch(src.rel.type)
				{
				case Shader::PARAMETER_ADDR:     a = a0[component]; break;
				case Shader::PARAMETER_TEMP:     a = r[src.rel.index][component]; break;
				case Shader::PARAMETER_INPUT:    a = v[src.rel.index][component]; break;
				case Shader::PARAMETER_OUTPUT:   a = o[src.rel.index][component]; break;
				case Shader::PARAMETER_CONST:    a = *Pointer<Float>(uniformAddress(src.bufferIndex, src.rel.index) + component * sizeof(float)); break;
				case Shader::PARAMETER_MISCTYPE:
					if(src.rel.index == Shader::InstanceIDIndex)
					{
						a = As<Float4>(Int4(instanceID)); break;
					}
					else if(src.rel.index == Shader::VertexIDIndex)
					{
						a = As<Float4>(vertexID); break;
					}
					else ASSERT(false);
					break;
				default: ASSERT(false);
				}

				Int4 index = Int4(i) + As<Int4>(a) * Int4(src.rel.scale);

				index = Min(As<UInt4>(index), UInt4(VERTEX_UNIFORM_VECTORS));   // Clamp to constant register range, c[VERTEX_UNIFORM_VECTORS] = {0, 0, 0, 0}

				Int index0 = Extract(index, 0);
				Int index1 = Extract(index, 1);
				Int index2 = Extract(index, 2);
				Int index3 = Extract(index, 3);

				c.x = *Pointer<Float4>(uniformAddress(src.bufferIndex, 0, index0), 16);
				c.y = *Pointer<Float4>(uniformAddress(src.bufferIndex, 0, index1), 16);
				c.z = *Pointer<Float4>(uniformAddress(src.bufferIndex, 0, index2), 16);
				c.w = *Pointer<Float4>(uniformAddress(src.bufferIndex, 0, index3), 16);

				transpose4x4(c.x, c.y, c.z, c.w);
			}
		}

		return c;
	}

	Int VertexProgram::relativeAddress(const Shader::Parameter &var, int bufferIndex)
	{
		ASSERT(var.rel.deterministic);

		if(var.rel.type == Shader::PARAMETER_TEMP)
		{
			return As<Int>(Extract(r[var.rel.index].x, 0)) * var.rel.scale;
		}
		else if(var.rel.type == Shader::PARAMETER_INPUT)
		{
			return As<Int>(Extract(v[var.rel.index].x, 0)) * var.rel.scale;
		}
		else if(var.rel.type == Shader::PARAMETER_OUTPUT)
		{
			return As<Int>(Extract(o[var.rel.index].x, 0)) * var.rel.scale;
		}
		else if(var.rel.type == Shader::PARAMETER_CONST)
		{
			return *Pointer<Int>(uniformAddress(bufferIndex, var.rel.index)) * var.rel.scale;
		}
		else if(var.rel.type == Shader::PARAMETER_LOOP)
		{
			return aL[loopDepth];
		}
		else ASSERT(false);

		return 0;
	}

	Int4 VertexProgram::enableMask(const Shader::Instruction *instruction)
	{
		Int4 enable = instruction->analysisBranch ? Int4(enableStack[enableIndex]) : Int4(0xFFFFFFFF);

		if(!whileTest)
		{
			if(shader->containsBreakInstruction() && instruction->analysisBreak)
			{
				enable &= enableBreak;
			}

			if(shader->containsContinueInstruction() && instruction->analysisContinue)
			{
				enable &= enableContinue;
			}

			if(shader->containsLeaveInstruction() && instruction->analysisLeave)
			{
				enable &= enableLeave;
			}
		}

		return enable;
	}

	void VertexProgram::M3X2(Vector4f &dst, Vector4f &src0, Src &src1)
	{
		Vector4f row0 = fetchRegister(src1, 0);
		Vector4f row1 = fetchRegister(src1, 1);

		dst.x = dot3(src0, row0);
		dst.y = dot3(src0, row1);
	}

	void VertexProgram::M3X3(Vector4f &dst, Vector4f &src0, Src &src1)
	{
		Vector4f row0 = fetchRegister(src1, 0);
		Vector4f row1 = fetchRegister(src1, 1);
		Vector4f row2 = fetchRegister(src1, 2);

		dst.x = dot3(src0, row0);
		dst.y = dot3(src0, row1);
		dst.z = dot3(src0, row2);
	}

	void VertexProgram::M3X4(Vector4f &dst, Vector4f &src0, Src &src1)
	{
		Vector4f row0 = fetchRegister(src1, 0);
		Vector4f row1 = fetchRegister(src1, 1);
		Vector4f row2 = fetchRegister(src1, 2);
		Vector4f row3 = fetchRegister(src1, 3);

		dst.x = dot3(src0, row0);
		dst.y = dot3(src0, row1);
		dst.z = dot3(src0, row2);
		dst.w = dot3(src0, row3);
	}

	void VertexProgram::M4X3(Vector4f &dst, Vector4f &src0, Src &src1)
	{
		Vector4f row0 = fetchRegister(src1, 0);
		Vector4f row1 = fetchRegister(src1, 1);
		Vector4f row2 = fetchRegister(src1, 2);

		dst.x = dot4(src0, row0);
		dst.y = dot4(src0, row1);
		dst.z = dot4(src0, row2);
	}

	void VertexProgram::M4X4(Vector4f &dst, Vector4f &src0, Src &src1)
	{
		Vector4f row0 = fetchRegister(src1, 0);
		Vector4f row1 = fetchRegister(src1, 1);
		Vector4f row2 = fetchRegister(src1, 2);
		Vector4f row3 = fetchRegister(src1, 3);

		dst.x = dot4(src0, row0);
		dst.y = dot4(src0, row1);
		dst.z = dot4(src0, row2);
		dst.w = dot4(src0, row3);
	}

	void VertexProgram::BREAK()
	{
		enableBreak = enableBreak & ~enableStack[enableIndex];
	}

	void VertexProgram::BREAKC(Vector4f &src0, Vector4f &src1, Control control)
	{
		Int4 condition;

		switch(control)
		{
		case Shader::CONTROL_GT: condition = CmpNLE(src0.x, src1.x); break;
		case Shader::CONTROL_EQ: condition = CmpEQ(src0.x, src1.x);  break;
		case Shader::CONTROL_GE: condition = CmpNLT(src0.x, src1.x); break;
		case Shader::CONTROL_LT: condition = CmpLT(src0.x, src1.x);  break;
		case Shader::CONTROL_NE: condition = CmpNEQ(src0.x, src1.x); break;
		case Shader::CONTROL_LE: condition = CmpLE(src0.x, src1.x);  break;
		default:
			ASSERT(false);
		}

		BREAK(condition);
	}

	void VertexProgram::BREAKP(const Src &predicateRegister)   // FIXME: Factor out parts common with BREAKC
	{
		Int4 condition = As<Int4>(p0[predicateRegister.swizzle & 0x3]);

		if(predicateRegister.modifier == Shader::MODIFIER_NOT)
		{
			condition = ~condition;
		}

		BREAK(condition);
	}

	void VertexProgram::BREAK(Int4 &condition)
	{
		condition &= enableStack[enableIndex];

		enableBreak = enableBreak & ~condition;
	}

	void VertexProgram::CONTINUE()
	{
		enableContinue = enableContinue & ~enableStack[enableIndex];
	}

	void VertexProgram::TEST()
	{
		whileTest = true;
	}

	void VertexProgram::CALL(int labelIndex, int callSiteIndex)
	{
		if(!labelBlock[labelIndex])
		{
			labelBlock[labelIndex] = Nucleus::createBasicBlock();
		}

		if(callRetBlock[labelIndex].size() > 1)
		{
			callStack[stackIndex++] = UInt(callSiteIndex);
		}

		Int4 restoreLeave = enableLeave;

		Nucleus::createBr(labelBlock[labelIndex]);
		Nucleus::setInsertBlock(callRetBlock[labelIndex][callSiteIndex]);

		enableLeave = restoreLeave;
	}

	void VertexProgram::CALLNZ(int labelIndex, int callSiteIndex, const Src &src)
	{
		if(src.type == Shader::PARAMETER_CONSTBOOL)
		{
			CALLNZb(labelIndex, callSiteIndex, src);
		}
		else if(src.type == Shader::PARAMETER_PREDICATE)
		{
			CALLNZp(labelIndex, callSiteIndex, src);
		}
		else ASSERT(false);
	}

	void VertexProgram::CALLNZb(int labelIndex, int callSiteIndex, const Src &boolRegister)
	{
		Bool condition = (*Pointer<Byte>(data + OFFSET(DrawData,vs.b[boolRegister.index])) != Byte(0));   // FIXME

		if(boolRegister.modifier == Shader::MODIFIER_NOT)
		{
			condition = !condition;
		}

		if(!labelBlock[labelIndex])
		{
			labelBlock[labelIndex] = Nucleus::createBasicBlock();
		}

		if(callRetBlock[labelIndex].size() > 1)
		{
			callStack[stackIndex++] = UInt(callSiteIndex);
		}

		Int4 restoreLeave = enableLeave;

		branch(condition, labelBlock[labelIndex], callRetBlock[labelIndex][callSiteIndex]);
		Nucleus::setInsertBlock(callRetBlock[labelIndex][callSiteIndex]);

		enableLeave = restoreLeave;
	}

	void VertexProgram::CALLNZp(int labelIndex, int callSiteIndex, const Src &predicateRegister)
	{
		Int4 condition = As<Int4>(p0[predicateRegister.swizzle & 0x3]);

		if(predicateRegister.modifier == Shader::MODIFIER_NOT)
		{
			condition = ~condition;
		}

		condition &= enableStack[enableIndex];

		if(!labelBlock[labelIndex])
		{
			labelBlock[labelIndex] = Nucleus::createBasicBlock();
		}

		if(callRetBlock[labelIndex].size() > 1)
		{
			callStack[stackIndex++] = UInt(callSiteIndex);
		}

		enableIndex++;
		enableStack[enableIndex] = condition;
		Int4 restoreLeave = enableLeave;

		Bool notAllFalse = SignMask(condition) != 0;
		branch(notAllFalse, labelBlock[labelIndex], callRetBlock[labelIndex][callSiteIndex]);
		Nucleus::setInsertBlock(callRetBlock[labelIndex][callSiteIndex]);

		enableIndex--;
		enableLeave = restoreLeave;
	}

	void VertexProgram::ELSE()
	{
		ifDepth--;

		BasicBlock *falseBlock = ifFalseBlock[ifDepth];
		BasicBlock *endBlock = Nucleus::createBasicBlock();

		if(isConditionalIf[ifDepth])
		{
			Int4 condition = ~enableStack[enableIndex] & enableStack[enableIndex - 1];
			Bool notAllFalse = SignMask(condition) != 0;

			branch(notAllFalse, falseBlock, endBlock);

			enableStack[enableIndex] = ~enableStack[enableIndex] & enableStack[enableIndex - 1];
		}
		else
		{
			Nucleus::createBr(endBlock);
			Nucleus::setInsertBlock(falseBlock);
		}

		ifFalseBlock[ifDepth] = endBlock;

		ifDepth++;
	}

	void VertexProgram::ENDIF()
	{
		ifDepth--;

		BasicBlock *endBlock = ifFalseBlock[ifDepth];

		Nucleus::createBr(endBlock);
		Nucleus::setInsertBlock(endBlock);

		if(isConditionalIf[ifDepth])
		{
			enableIndex--;
		}
	}

	void VertexProgram::ENDLOOP()
	{
		loopRepDepth--;

		aL[loopDepth] = aL[loopDepth] + increment[loopDepth];   // FIXME: +=

		BasicBlock *testBlock = loopRepTestBlock[loopRepDepth];
		BasicBlock *endBlock = loopRepEndBlock[loopRepDepth];

		Nucleus::createBr(testBlock);
		Nucleus::setInsertBlock(endBlock);

		loopDepth--;
		enableBreak = Int4(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);
	}

	void VertexProgram::ENDREP()
	{
		loopRepDepth--;

		BasicBlock *testBlock = loopRepTestBlock[loopRepDepth];
		BasicBlock *endBlock = loopRepEndBlock[loopRepDepth];

		Nucleus::createBr(testBlock);
		Nucleus::setInsertBlock(endBlock);

		loopDepth--;
		enableBreak = Int4(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);
	}

	void VertexProgram::ENDWHILE()
	{
		loopRepDepth--;

		BasicBlock *testBlock = loopRepTestBlock[loopRepDepth];
		BasicBlock *endBlock = loopRepEndBlock[loopRepDepth];

		Nucleus::createBr(testBlock);
		Nucleus::setInsertBlock(endBlock);

		enableIndex--;
		whileTest = false;
	}

	void VertexProgram::ENDSWITCH()
	{
		loopRepDepth--;

		BasicBlock *endBlock = loopRepEndBlock[loopRepDepth];

		Nucleus::createBr(endBlock);
		Nucleus::setInsertBlock(endBlock);
	}

	void VertexProgram::IF(const Src &src)
	{
		if(src.type == Shader::PARAMETER_CONSTBOOL)
		{
			IFb(src);
		}
		else if(src.type == Shader::PARAMETER_PREDICATE)
		{
			IFp(src);
		}
		else
		{
			Int4 condition = As<Int4>(fetchRegister(src).x);
			IF(condition);
		}
	}

	void VertexProgram::IFb(const Src &boolRegister)
	{
		ASSERT(ifDepth < 24 + 4);

		Bool condition = (*Pointer<Byte>(data + OFFSET(DrawData,vs.b[boolRegister.index])) != Byte(0));   // FIXME

		if(boolRegister.modifier == Shader::MODIFIER_NOT)
		{
			condition = !condition;
		}

		BasicBlock *trueBlock = Nucleus::createBasicBlock();
		BasicBlock *falseBlock = Nucleus::createBasicBlock();

		branch(condition, trueBlock, falseBlock);

		isConditionalIf[ifDepth] = false;
		ifFalseBlock[ifDepth] = falseBlock;

		ifDepth++;
	}

	void VertexProgram::IFp(const Src &predicateRegister)
	{
		Int4 condition = As<Int4>(p0[predicateRegister.swizzle & 0x3]);

		if(predicateRegister.modifier == Shader::MODIFIER_NOT)
		{
			condition = ~condition;
		}

		IF(condition);
	}

	void VertexProgram::IFC(Vector4f &src0, Vector4f &src1, Control control)
	{
		Int4 condition;

		switch(control)
		{
		case Shader::CONTROL_GT: condition = CmpNLE(src0.x, src1.x); break;
		case Shader::CONTROL_EQ: condition = CmpEQ(src0.x, src1.x);  break;
		case Shader::CONTROL_GE: condition = CmpNLT(src0.x, src1.x); break;
		case Shader::CONTROL_LT: condition = CmpLT(src0.x, src1.x);  break;
		case Shader::CONTROL_NE: condition = CmpNEQ(src0.x, src1.x); break;
		case Shader::CONTROL_LE: condition = CmpLE(src0.x, src1.x);  break;
		default:
			ASSERT(false);
		}

		IF(condition);
	}

	void VertexProgram::IF(Int4 &condition)
	{
		condition &= enableStack[enableIndex];

		enableIndex++;
		enableStack[enableIndex] = condition;

		BasicBlock *trueBlock = Nucleus::createBasicBlock();
		BasicBlock *falseBlock = Nucleus::createBasicBlock();

		Bool notAllFalse = SignMask(condition) != 0;

		branch(notAllFalse, trueBlock, falseBlock);

		isConditionalIf[ifDepth] = true;
		ifFalseBlock[ifDepth] = falseBlock;

		ifDepth++;
	}

	void VertexProgram::LABEL(int labelIndex)
	{
		if(!labelBlock[labelIndex])
		{
			labelBlock[labelIndex] = Nucleus::createBasicBlock();
		}

		Nucleus::setInsertBlock(labelBlock[labelIndex]);
		currentLabel = labelIndex;
	}

	void VertexProgram::LOOP(const Src &integerRegister)
	{
		loopDepth++;

		iteration[loopDepth] = *Pointer<Int>(data + OFFSET(DrawData,vs.i[integerRegister.index][0]));
		aL[loopDepth] = *Pointer<Int>(data + OFFSET(DrawData,vs.i[integerRegister.index][1]));
		increment[loopDepth] = *Pointer<Int>(data + OFFSET(DrawData,vs.i[integerRegister.index][2]));

		// FIXME: Compiles to two instructions?
		If(increment[loopDepth] == 0)
		{
			increment[loopDepth] = 1;
		}

		BasicBlock *loopBlock = Nucleus::createBasicBlock();
		BasicBlock *testBlock = Nucleus::createBasicBlock();
		BasicBlock *endBlock = Nucleus::createBasicBlock();

		loopRepTestBlock[loopRepDepth] = testBlock;
		loopRepEndBlock[loopRepDepth] = endBlock;

		// FIXME: jump(testBlock)
		Nucleus::createBr(testBlock);
		Nucleus::setInsertBlock(testBlock);

		branch(iteration[loopDepth] > 0, loopBlock, endBlock);
		Nucleus::setInsertBlock(loopBlock);

		iteration[loopDepth] = iteration[loopDepth] - 1;   // FIXME: --

		loopRepDepth++;
	}

	void VertexProgram::REP(const Src &integerRegister)
	{
		loopDepth++;

		iteration[loopDepth] = *Pointer<Int>(data + OFFSET(DrawData,vs.i[integerRegister.index][0]));
		aL[loopDepth] = aL[loopDepth - 1];

		BasicBlock *loopBlock = Nucleus::createBasicBlock();
		BasicBlock *testBlock = Nucleus::createBasicBlock();
		BasicBlock *endBlock = Nucleus::createBasicBlock();

		loopRepTestBlock[loopRepDepth] = testBlock;
		loopRepEndBlock[loopRepDepth] = endBlock;

		// FIXME: jump(testBlock)
		Nucleus::createBr(testBlock);
		Nucleus::setInsertBlock(testBlock);

		branch(iteration[loopDepth] > 0, loopBlock, endBlock);
		Nucleus::setInsertBlock(loopBlock);

		iteration[loopDepth] = iteration[loopDepth] - 1;   // FIXME: --

		loopRepDepth++;
	}

	void VertexProgram::WHILE(const Src &temporaryRegister)
	{
		enableIndex++;

		BasicBlock *loopBlock = Nucleus::createBasicBlock();
		BasicBlock *testBlock = Nucleus::createBasicBlock();
		BasicBlock *endBlock = Nucleus::createBasicBlock();

		loopRepTestBlock[loopRepDepth] = testBlock;
		loopRepEndBlock[loopRepDepth] = endBlock;

		Int4 restoreBreak = enableBreak;
		Int4 restoreContinue = enableContinue;

		// TODO: jump(testBlock)
		Nucleus::createBr(testBlock);
		Nucleus::setInsertBlock(testBlock);
		enableContinue = restoreContinue;

		const Vector4f &src = fetchRegister(temporaryRegister);
		Int4 condition = As<Int4>(src.x);
		condition &= enableStack[enableIndex - 1];
		if(shader->containsLeaveInstruction()) condition &= enableLeave;
		if(shader->containsBreakInstruction()) condition &= enableBreak;
		enableStack[enableIndex] = condition;

		Bool notAllFalse = SignMask(condition) != 0;
		branch(notAllFalse, loopBlock, endBlock);

		Nucleus::setInsertBlock(endBlock);
		enableBreak = restoreBreak;

		Nucleus::setInsertBlock(loopBlock);

		loopRepDepth++;
	}

	void VertexProgram::SWITCH()
	{
		BasicBlock *endBlock = Nucleus::createBasicBlock();

		loopRepTestBlock[loopRepDepth] = nullptr;
		loopRepEndBlock[loopRepDepth] = endBlock;

		Int4 restoreBreak = enableBreak;

		BasicBlock *currentBlock = Nucleus::getInsertBlock();

		Nucleus::setInsertBlock(endBlock);
		enableBreak = restoreBreak;

		Nucleus::setInsertBlock(currentBlock);

		loopRepDepth++;
	}

	void VertexProgram::RET()
	{
		if(currentLabel == -1)
		{
			returnBlock = Nucleus::createBasicBlock();
			Nucleus::createBr(returnBlock);
		}
		else
		{
			BasicBlock *unreachableBlock = Nucleus::createBasicBlock();

			if(callRetBlock[currentLabel].size() > 1)   // Pop the return destination from the call stack
			{
				// FIXME: Encapsulate
				UInt index = callStack[--stackIndex];

				Value *value = index.loadValue();
				SwitchCases *switchCases = Nucleus::createSwitch(value, unreachableBlock, (int)callRetBlock[currentLabel].size());

				for(unsigned int i = 0; i < callRetBlock[currentLabel].size(); i++)
				{
					Nucleus::addSwitchCase(switchCases, i, callRetBlock[currentLabel][i]);
				}
			}
			else if(callRetBlock[currentLabel].size() == 1)   // Jump directly to the unique return destination
			{
				Nucleus::createBr(callRetBlock[currentLabel][0]);
			}
			else   // Function isn't called
			{
				Nucleus::createBr(unreachableBlock);
			}

			Nucleus::setInsertBlock(unreachableBlock);
			Nucleus::createUnreachable();
		}
	}

	void VertexProgram::LEAVE()
	{
		enableLeave = enableLeave & ~enableStack[enableIndex];

		// FIXME: Return from function if all instances left
		// FIXME: Use enableLeave in other control-flow constructs
	}

	void VertexProgram::TEX(Vector4f &dst, Vector4f &src0, const Src &src1)
	{
		dst = sampleTexture(src1, src0, (src0.x), (src0), (src0), (src0), Base);
	}

	void VertexProgram::TEXOFFSET(Vector4f &dst, Vector4f &src0, const Src& src1, Vector4f &offset)
	{
		dst = sampleTexture(src1, src0, (src0.x), (src0), (src0), offset, {Base, Offset});
	}

	void VertexProgram::TEXLOD(Vector4f &dst, Vector4f &src0, const Src& src1, Float4 &lod)
	{
		dst = sampleTexture(src1, src0, lod, (src0), (src0), (src0), Lod);
	}

	void VertexProgram::TEXLODOFFSET(Vector4f &dst, Vector4f &src0, const Src& src1, Vector4f &offset, Float4 &lod)
	{
		dst = sampleTexture(src1, src0, lod, (src0), (src0), offset, {Lod, Offset});
	}

	void VertexProgram::TEXELFETCH(Vector4f &dst, Vector4f &src0, const Src& src1, Float4 &lod)
	{
		dst = sampleTexture(src1, src0, lod, (src0), (src0), (src0), Fetch);
	}

	void VertexProgram::TEXELFETCHOFFSET(Vector4f &dst, Vector4f &src0, const Src& src1, Vector4f &offset, Float4 &lod)
	{
		dst = sampleTexture(src1, src0, lod, (src0), (src0), offset, {Fetch, Offset});
	}

	void VertexProgram::TEXGRAD(Vector4f &dst, Vector4f &src0, const Src& src1, Vector4f &dsx, Vector4f &dsy)
	{
		dst = sampleTexture(src1, src0, (src0.x), dsx, dsy, src0, Grad);
	}

	void VertexProgram::TEXGRADOFFSET(Vector4f &dst, Vector4f &src0, const Src& src1, Vector4f &dsx, Vector4f &dsy, Vector4f &offset)
	{
		dst = sampleTexture(src1, src0, (src0.x), dsx, dsy, offset, {Grad, Offset});
	}

	void VertexProgram::TEXSIZE(Vector4f &dst, Float4 &lod, const Src &src1)
	{
		Pointer<Byte> texture = data + OFFSET(DrawData, mipmap[TEXTURE_IMAGE_UNITS]) + src1.index * sizeof(Texture);
		dst = SamplerCore::textureSize(texture, lod);
	}

	Vector4f VertexProgram::sampleTexture(const Src &s, Vector4f &uvwq, Float4 &lod, Vector4f &dsx, Vector4f &dsy, Vector4f &offset, SamplerFunction function)
	{
		Vector4f tmp;

		if(s.type == Shader::PARAMETER_SAMPLER && s.rel.type == Shader::PARAMETER_VOID)
		{
			tmp = sampleTexture(s.index, uvwq, lod, dsx, dsy, offset, function);
		}
		else
		{
			Int index = As<Int>(Float(fetchRegister(s).x.x));

			for(int i = 0; i < VERTEX_TEXTURE_IMAGE_UNITS; i++)
			{
				if(shader->usesSampler(i))
				{
					If(index == i)
					{
						tmp = sampleTexture(i, uvwq, lod, dsx, dsy, offset, function);
						// FIXME: When the sampler states are the same, we could use one sampler and just index the texture
					}
				}
			}
		}

		Vector4f c;
		c.x = tmp[(s.swizzle >> 0) & 0x3];
		c.y = tmp[(s.swizzle >> 2) & 0x3];
		c.z = tmp[(s.swizzle >> 4) & 0x3];
		c.w = tmp[(s.swizzle >> 6) & 0x3];

		return c;
	}

	Vector4f VertexProgram::sampleTexture(int sampler, Vector4f &uvwq, Float4 &lod, Vector4f &dsx, Vector4f &dsy, Vector4f &offset, SamplerFunction function)
	{
		Pointer<Byte> texture = data + OFFSET(DrawData, mipmap[TEXTURE_IMAGE_UNITS]) + sampler * sizeof(Texture);
		return SamplerCore(constants, state.sampler[sampler]).sampleTexture(texture, uvwq.x, uvwq.y, uvwq.z, uvwq.w, lod, dsx, dsy, offset, function);
	}
}
