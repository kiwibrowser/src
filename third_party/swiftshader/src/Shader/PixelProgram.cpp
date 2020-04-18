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

#include "PixelProgram.hpp"

#include "SamplerCore.hpp"
#include "Renderer/Primitive.hpp"
#include "Renderer/Renderer.hpp"

namespace sw
{
	extern bool postBlendSRGB;
	extern bool booleanFaceRegister;
	extern bool halfIntegerCoordinates;     // Pixel centers are not at integer coordinates
	extern bool fullPixelPositionRegister;

	void PixelProgram::setBuiltins(Int &x, Int &y, Float4(&z)[4], Float4 &w)
	{
		if(shader->getShaderModel() >= 0x0300)
		{
			if(shader->isVPosDeclared())
			{
				if(!halfIntegerCoordinates)
				{
					vPos.x = Float4(Float(x)) + Float4(0, 1, 0, 1);
					vPos.y = Float4(Float(y)) + Float4(0, 0, 1, 1);
				}
				else
				{
					vPos.x = Float4(Float(x)) + Float4(0.5f, 1.5f, 0.5f, 1.5f);
					vPos.y = Float4(Float(y)) + Float4(0.5f, 0.5f, 1.5f, 1.5f);
				}

				if(fullPixelPositionRegister)
				{
					vPos.z = z[0]; // FIXME: Centroid?
					vPos.w = w;    // FIXME: Centroid?
				}
			}

			if(shader->isVFaceDeclared())
			{
				Float4 area = *Pointer<Float>(primitive + OFFSET(Primitive, area));
				Float4 face = booleanFaceRegister ? Float4(As<Float4>(CmpNLT(area, Float4(0.0f)))) : area;

				vFace.x = face;
				vFace.y = face;
				vFace.z = face;
				vFace.w = face;
			}
		}
	}

	void PixelProgram::applyShader(Int cMask[4])
	{
		enableIndex = 0;
		stackIndex = 0;

		if(shader->containsLeaveInstruction())
		{
			enableLeave = Int4(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);
		}

		for(int i = 0; i < RENDERTARGETS; i++)
		{
			if(state.targetFormat[i] != FORMAT_NULL)
			{
				oC[i] = Vector4f(0.0f, 0.0f, 0.0f, 0.0f);
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

		bool broadcastColor0 = true;

		for(size_t i = 0; i < shader->getLength(); i++)
		{
			const Shader::Instruction *instruction = shader->getInstruction(i);
			Shader::Opcode opcode = instruction->opcode;

			if(opcode == Shader::OPCODE_DCL || opcode == Shader::OPCODE_DEF || opcode == Shader::OPCODE_DEFI || opcode == Shader::OPCODE_DEFB)
			{
				continue;
			}

			const Dst &dst = instruction->dst;
			const Src &src0 = instruction->src[0];
			const Src &src1 = instruction->src[1];
			const Src &src2 = instruction->src[2];
			const Src &src3 = instruction->src[3];
			const Src &src4 = instruction->src[4];

			bool predicate = instruction->predicate;
			Control control = instruction->control;
			bool pp = dst.partialPrecision;
			bool project = instruction->project;
			bool bias = instruction->bias;

			Vector4f d;
			Vector4f s0;
			Vector4f s1;
			Vector4f s2;
			Vector4f s3;
			Vector4f s4;

			if(opcode == Shader::OPCODE_TEXKILL)   // Takes destination as input
			{
				if(dst.type == Shader::PARAMETER_TEXTURE)
				{
					d.x = v[2 + dst.index].x;
					d.y = v[2 + dst.index].y;
					d.z = v[2 + dst.index].z;
					d.w = v[2 + dst.index].w;
				}
				else
				{
					d = r[dst.index];
				}
			}

			if(src0.type != Shader::PARAMETER_VOID) s0 = fetchRegister(src0);
			if(src1.type != Shader::PARAMETER_VOID) s1 = fetchRegister(src1);
			if(src2.type != Shader::PARAMETER_VOID) s2 = fetchRegister(src2);
			if(src3.type != Shader::PARAMETER_VOID) s3 = fetchRegister(src3);
			if(src4.type != Shader::PARAMETER_VOID) s4 = fetchRegister(src4);

			switch(opcode)
			{
			case Shader::OPCODE_PS_2_0:                                                    break;
			case Shader::OPCODE_PS_2_x:                                                    break;
			case Shader::OPCODE_PS_3_0:                                                    break;
			case Shader::OPCODE_DEF:                                                       break;
			case Shader::OPCODE_DCL:                                                       break;
			case Shader::OPCODE_NOP:                                                       break;
			case Shader::OPCODE_MOV:        mov(d, s0);                                    break;
			case Shader::OPCODE_NEG:        neg(d, s0);                                    break;
			case Shader::OPCODE_INEG:       ineg(d, s0);                                   break;
			case Shader::OPCODE_F2B:        f2b(d, s0);                                    break;
			case Shader::OPCODE_B2F:        b2f(d, s0);                                    break;
			case Shader::OPCODE_F2I:        f2i(d, s0);                                    break;
			case Shader::OPCODE_I2F:        i2f(d, s0);                                    break;
			case Shader::OPCODE_F2U:        f2u(d, s0);                                    break;
			case Shader::OPCODE_U2F:        u2f(d, s0);                                    break;
			case Shader::OPCODE_I2B:        i2b(d, s0);                                    break;
			case Shader::OPCODE_B2I:        b2i(d, s0);                                    break;
			case Shader::OPCODE_ADD:        add(d, s0, s1);                                break;
			case Shader::OPCODE_IADD:       iadd(d, s0, s1);                               break;
			case Shader::OPCODE_SUB:        sub(d, s0, s1);                                break;
			case Shader::OPCODE_ISUB:       isub(d, s0, s1);                               break;
			case Shader::OPCODE_MUL:        mul(d, s0, s1);                                break;
			case Shader::OPCODE_IMUL:       imul(d, s0, s1);                               break;
			case Shader::OPCODE_MAD:        mad(d, s0, s1, s2);                            break;
			case Shader::OPCODE_IMAD:       imad(d, s0, s1, s2);                           break;
			case Shader::OPCODE_DP1:        dp1(d, s0, s1);                                break;
			case Shader::OPCODE_DP2:        dp2(d, s0, s1);                                break;
			case Shader::OPCODE_DP2ADD:     dp2add(d, s0, s1, s2);                         break;
			case Shader::OPCODE_DP3:        dp3(d, s0, s1);                                break;
			case Shader::OPCODE_DP4:        dp4(d, s0, s1);                                break;
			case Shader::OPCODE_DET2:       det2(d, s0, s1);                               break;
			case Shader::OPCODE_DET3:       det3(d, s0, s1, s2);                           break;
			case Shader::OPCODE_DET4:       det4(d, s0, s1, s2, s3);                       break;
			case Shader::OPCODE_CMP0:       cmp0(d, s0, s1, s2);                           break;
			case Shader::OPCODE_ICMP:       icmp(d, s0, s1, control);                      break;
			case Shader::OPCODE_UCMP:       ucmp(d, s0, s1, control);                      break;
			case Shader::OPCODE_SELECT:     select(d, s0, s1, s2);                         break;
			case Shader::OPCODE_EXTRACT:    extract(d.x, s0, s1.x);                        break;
			case Shader::OPCODE_INSERT:     insert(d, s0, s1.x, s2.x);                     break;
			case Shader::OPCODE_FRC:        frc(d, s0);                                    break;
			case Shader::OPCODE_TRUNC:      trunc(d, s0);                                  break;
			case Shader::OPCODE_FLOOR:      floor(d, s0);                                  break;
			case Shader::OPCODE_ROUND:      round(d, s0);                                  break;
			case Shader::OPCODE_ROUNDEVEN:  roundEven(d, s0);                              break;
			case Shader::OPCODE_CEIL:       ceil(d, s0);                                   break;
			case Shader::OPCODE_EXP2X:      exp2x(d, s0, pp);                              break;
			case Shader::OPCODE_EXP2:       exp2(d, s0, pp);                               break;
			case Shader::OPCODE_LOG2X:      log2x(d, s0, pp);                              break;
			case Shader::OPCODE_LOG2:       log2(d, s0, pp);                               break;
			case Shader::OPCODE_EXP:        exp(d, s0, pp);                                break;
			case Shader::OPCODE_LOG:        log(d, s0, pp);                                break;
			case Shader::OPCODE_RCPX:       rcpx(d, s0, pp);                               break;
			case Shader::OPCODE_DIV:        div(d, s0, s1);                                break;
			case Shader::OPCODE_IDIV:       idiv(d, s0, s1);                               break;
			case Shader::OPCODE_UDIV:       udiv(d, s0, s1);                               break;
			case Shader::OPCODE_MOD:        mod(d, s0, s1);                                break;
			case Shader::OPCODE_IMOD:       imod(d, s0, s1);                               break;
			case Shader::OPCODE_UMOD:       umod(d, s0, s1);                               break;
			case Shader::OPCODE_SHL:        shl(d, s0, s1);                                break;
			case Shader::OPCODE_ISHR:       ishr(d, s0, s1);                               break;
			case Shader::OPCODE_USHR:       ushr(d, s0, s1);                               break;
			case Shader::OPCODE_RSQX:       rsqx(d, s0, pp);                               break;
			case Shader::OPCODE_SQRT:       sqrt(d, s0, pp);                               break;
			case Shader::OPCODE_RSQ:        rsq(d, s0, pp);                                break;
			case Shader::OPCODE_LEN2:       len2(d.x, s0, pp);                             break;
			case Shader::OPCODE_LEN3:       len3(d.x, s0, pp);                             break;
			case Shader::OPCODE_LEN4:       len4(d.x, s0, pp);                             break;
			case Shader::OPCODE_DIST1:      dist1(d.x, s0, s1, pp);                        break;
			case Shader::OPCODE_DIST2:      dist2(d.x, s0, s1, pp);                        break;
			case Shader::OPCODE_DIST3:      dist3(d.x, s0, s1, pp);                        break;
			case Shader::OPCODE_DIST4:      dist4(d.x, s0, s1, pp);                        break;
			case Shader::OPCODE_MIN:        min(d, s0, s1);                                break;
			case Shader::OPCODE_IMIN:       imin(d, s0, s1);                               break;
			case Shader::OPCODE_UMIN:       umin(d, s0, s1);                               break;
			case Shader::OPCODE_MAX:        max(d, s0, s1);                                break;
			case Shader::OPCODE_IMAX:       imax(d, s0, s1);                               break;
			case Shader::OPCODE_UMAX:       umax(d, s0, s1);                               break;
			case Shader::OPCODE_LRP:        lrp(d, s0, s1, s2);                            break;
			case Shader::OPCODE_STEP:       step(d, s0, s1);                               break;
			case Shader::OPCODE_SMOOTH:     smooth(d, s0, s1, s2);                         break;
			case Shader::OPCODE_ISINF:      isinf(d, s0);                                  break;
			case Shader::OPCODE_ISNAN:      isnan(d, s0);                                  break;
			case Shader::OPCODE_FLOATBITSTOINT:
			case Shader::OPCODE_FLOATBITSTOUINT:
			case Shader::OPCODE_INTBITSTOFLOAT:
			case Shader::OPCODE_UINTBITSTOFLOAT: d = s0;                                   break;
			case Shader::OPCODE_PACKSNORM2x16:   packSnorm2x16(d, s0);                     break;
			case Shader::OPCODE_PACKUNORM2x16:   packUnorm2x16(d, s0);                     break;
			case Shader::OPCODE_PACKHALF2x16:    packHalf2x16(d, s0);                      break;
			case Shader::OPCODE_UNPACKSNORM2x16: unpackSnorm2x16(d, s0);                   break;
			case Shader::OPCODE_UNPACKUNORM2x16: unpackUnorm2x16(d, s0);                   break;
			case Shader::OPCODE_UNPACKHALF2x16:  unpackHalf2x16(d, s0);                    break;
			case Shader::OPCODE_POWX:       powx(d, s0, s1, pp);                           break;
			case Shader::OPCODE_POW:        pow(d, s0, s1, pp);                            break;
			case Shader::OPCODE_SGN:        sgn(d, s0);                                    break;
			case Shader::OPCODE_ISGN:       isgn(d, s0);                                   break;
			case Shader::OPCODE_CRS:        crs(d, s0, s1);                                break;
			case Shader::OPCODE_FORWARD1:   forward1(d, s0, s1, s2);                       break;
			case Shader::OPCODE_FORWARD2:   forward2(d, s0, s1, s2);                       break;
			case Shader::OPCODE_FORWARD3:   forward3(d, s0, s1, s2);                       break;
			case Shader::OPCODE_FORWARD4:   forward4(d, s0, s1, s2);                       break;
			case Shader::OPCODE_REFLECT1:   reflect1(d, s0, s1);                           break;
			case Shader::OPCODE_REFLECT2:   reflect2(d, s0, s1);                           break;
			case Shader::OPCODE_REFLECT3:   reflect3(d, s0, s1);                           break;
			case Shader::OPCODE_REFLECT4:   reflect4(d, s0, s1);                           break;
			case Shader::OPCODE_REFRACT1:   refract1(d, s0, s1, s2.x);                     break;
			case Shader::OPCODE_REFRACT2:   refract2(d, s0, s1, s2.x);                     break;
			case Shader::OPCODE_REFRACT3:   refract3(d, s0, s1, s2.x);                     break;
			case Shader::OPCODE_REFRACT4:   refract4(d, s0, s1, s2.x);                     break;
			case Shader::OPCODE_NRM2:       nrm2(d, s0, pp);                               break;
			case Shader::OPCODE_NRM3:       nrm3(d, s0, pp);                               break;
			case Shader::OPCODE_NRM4:       nrm4(d, s0, pp);                               break;
			case Shader::OPCODE_ABS:        abs(d, s0);                                    break;
			case Shader::OPCODE_IABS:       iabs(d, s0);                                   break;
			case Shader::OPCODE_SINCOS:     sincos(d, s0, pp);                             break;
			case Shader::OPCODE_COS:        cos(d, s0, pp);                                break;
			case Shader::OPCODE_SIN:        sin(d, s0, pp);                                break;
			case Shader::OPCODE_TAN:        tan(d, s0, pp);                                break;
			case Shader::OPCODE_ACOS:       acos(d, s0, pp);                               break;
			case Shader::OPCODE_ASIN:       asin(d, s0, pp);                               break;
			case Shader::OPCODE_ATAN:       atan(d, s0, pp);                               break;
			case Shader::OPCODE_ATAN2:      atan2(d, s0, s1, pp);                          break;
			case Shader::OPCODE_COSH:       cosh(d, s0, pp);                               break;
			case Shader::OPCODE_SINH:       sinh(d, s0, pp);                               break;
			case Shader::OPCODE_TANH:       tanh(d, s0, pp);                               break;
			case Shader::OPCODE_ACOSH:      acosh(d, s0, pp);                              break;
			case Shader::OPCODE_ASINH:      asinh(d, s0, pp);                              break;
			case Shader::OPCODE_ATANH:      atanh(d, s0, pp);                              break;
			case Shader::OPCODE_M4X4:       M4X4(d, s0, src1);                             break;
			case Shader::OPCODE_M4X3:       M4X3(d, s0, src1);                             break;
			case Shader::OPCODE_M3X4:       M3X4(d, s0, src1);                             break;
			case Shader::OPCODE_M3X3:       M3X3(d, s0, src1);                             break;
			case Shader::OPCODE_M3X2:       M3X2(d, s0, src1);                             break;
			case Shader::OPCODE_TEX:        TEX(d, s0, src1, project, bias);               break;
			case Shader::OPCODE_TEXLDD:     TEXGRAD(d, s0, src1, s2, s3);                  break;
			case Shader::OPCODE_TEXLDL:     TEXLOD(d, s0, src1, s0.w);                     break;
			case Shader::OPCODE_TEXLOD:     TEXLOD(d, s0, src1, s2.x);                     break;
			case Shader::OPCODE_TEXSIZE:    TEXSIZE(d, s0.x, src1);                        break;
			case Shader::OPCODE_TEXKILL:    TEXKILL(cMask, d, dst.mask);                   break;
			case Shader::OPCODE_TEXOFFSET:  TEXOFFSET(d, s0, src1, s2);                    break;
			case Shader::OPCODE_TEXLODOFFSET: TEXLODOFFSET(d, s0, src1, s2, s3.x);         break;
			case Shader::OPCODE_TEXELFETCH: TEXELFETCH(d, s0, src1, s2.x);                 break;
			case Shader::OPCODE_TEXELFETCHOFFSET: TEXELFETCHOFFSET(d, s0, src1, s2, s3.x); break;
			case Shader::OPCODE_TEXGRAD:    TEXGRAD(d, s0, src1, s2, s3);                  break;
			case Shader::OPCODE_TEXGRADOFFSET: TEXGRADOFFSET(d, s0, src1, s2, s3, s4);     break;
			case Shader::OPCODE_TEXBIAS:    TEXBIAS(d, s0, src1, s2.x);                    break;
			case Shader::OPCODE_TEXOFFSETBIAS: TEXOFFSETBIAS(d, s0, src1, s2, s3.x);       break;
			case Shader::OPCODE_DISCARD:    DISCARD(cMask, instruction);                   break;
			case Shader::OPCODE_DFDX:       DFDX(d, s0);                                   break;
			case Shader::OPCODE_DFDY:       DFDY(d, s0);                                   break;
			case Shader::OPCODE_FWIDTH:     FWIDTH(d, s0);                                 break;
			case Shader::OPCODE_BREAK:      BREAK();                                       break;
			case Shader::OPCODE_BREAKC:     BREAKC(s0, s1, control);                       break;
			case Shader::OPCODE_BREAKP:     BREAKP(src0);                                  break;
			case Shader::OPCODE_CONTINUE:   CONTINUE();                                    break;
			case Shader::OPCODE_TEST:       TEST();                                        break;
			case Shader::OPCODE_CALL:       CALL(dst.label, dst.callSite);                 break;
			case Shader::OPCODE_CALLNZ:     CALLNZ(dst.label, dst.callSite, src0);         break;
			case Shader::OPCODE_ELSE:       ELSE();                                        break;
			case Shader::OPCODE_ENDIF:      ENDIF();                                       break;
			case Shader::OPCODE_ENDLOOP:    ENDLOOP();                                     break;
			case Shader::OPCODE_ENDREP:     ENDREP();                                      break;
			case Shader::OPCODE_ENDWHILE:   ENDWHILE();                                    break;
			case Shader::OPCODE_ENDSWITCH:  ENDSWITCH();                                   break;
			case Shader::OPCODE_IF:         IF(src0);                                      break;
			case Shader::OPCODE_IFC:        IFC(s0, s1, control);                          break;
			case Shader::OPCODE_LABEL:      LABEL(dst.index);                              break;
			case Shader::OPCODE_LOOP:       LOOP(src1);                                    break;
			case Shader::OPCODE_REP:        REP(src0);                                     break;
			case Shader::OPCODE_WHILE:      WHILE(src0);                                   break;
			case Shader::OPCODE_SWITCH:     SWITCH();                                      break;
			case Shader::OPCODE_RET:        RET();                                         break;
			case Shader::OPCODE_LEAVE:      LEAVE();                                       break;
			case Shader::OPCODE_CMP:        cmp(d, s0, s1, control);                       break;
			case Shader::OPCODE_ALL:        all(d.x, s0);                                  break;
			case Shader::OPCODE_ANY:        any(d.x, s0);                                  break;
			case Shader::OPCODE_NOT:        bitwise_not(d, s0);                            break;
			case Shader::OPCODE_OR:         bitwise_or(d, s0, s1);                         break;
			case Shader::OPCODE_XOR:        bitwise_xor(d, s0, s1);                        break;
			case Shader::OPCODE_AND:        bitwise_and(d, s0, s1);                        break;
			case Shader::OPCODE_EQ:         equal(d, s0, s1);                              break;
			case Shader::OPCODE_NE:         notEqual(d, s0, s1);                           break;
			case Shader::OPCODE_END:                                                       break;
			default:
				ASSERT(false);
			}

			if(dst.type != Shader::PARAMETER_VOID && dst.type != Shader::PARAMETER_LABEL && opcode != Shader::OPCODE_TEXKILL && opcode != Shader::OPCODE_NOP)
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
					case Shader::PARAMETER_COLOROUT:
						if(dst.rel.type == Shader::PARAMETER_VOID)
						{
							if(dst.x) pDst.x = oC[dst.index].x;
							if(dst.y) pDst.y = oC[dst.index].y;
							if(dst.z) pDst.z = oC[dst.index].z;
							if(dst.w) pDst.w = oC[dst.index].w;
						}
						else
						{
							Int a = relativeAddress(dst) + dst.index;

							if(dst.x) pDst.x = oC[a].x;
							if(dst.y) pDst.y = oC[a].y;
							if(dst.z) pDst.z = oC[a].z;
							if(dst.w) pDst.w = oC[a].w;
						}
						break;
					case Shader::PARAMETER_PREDICATE:
						if(dst.x) pDst.x = p0.x;
						if(dst.y) pDst.y = p0.y;
						if(dst.z) pDst.z = p0.z;
						if(dst.w) pDst.w = p0.w;
						break;
					case Shader::PARAMETER_DEPTHOUT:
						pDst.x = oDepth;
						break;
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
				case Shader::PARAMETER_COLOROUT:
					if(dst.rel.type == Shader::PARAMETER_VOID)
					{
						broadcastColor0 = (dst.index == 0) && broadcastColor0;

						if(dst.x) { oC[dst.index].x = d.x; }
						if(dst.y) { oC[dst.index].y = d.y; }
						if(dst.z) { oC[dst.index].z = d.z; }
						if(dst.w) { oC[dst.index].w = d.w; }
					}
					else
					{
						broadcastColor0 = false;
						Int a = relativeAddress(dst) + dst.index;

						if(dst.x) { oC[a].x = d.x; }
						if(dst.y) { oC[a].y = d.y; }
						if(dst.z) { oC[a].z = d.z; }
						if(dst.w) { oC[a].w = d.w; }
					}
					break;
				case Shader::PARAMETER_PREDICATE:
					if(dst.x) p0.x = d.x;
					if(dst.y) p0.y = d.y;
					if(dst.z) p0.z = d.z;
					if(dst.w) p0.w = d.w;
					break;
				case Shader::PARAMETER_DEPTHOUT:
					oDepth = d.x;
					break;
				default:
					ASSERT(false);
				}
			}
		}

		if(currentLabel != -1)
		{
			Nucleus::setInsertBlock(returnBlock);
		}

		if(broadcastColor0)
		{
			for(int i = 0; i < RENDERTARGETS; i++)
			{
				c[i] = oC[0];
			}
		}
		else
		{
			for(int i = 0; i < RENDERTARGETS; i++)
			{
				c[i] = oC[i];
			}
		}

		clampColor(c);

		if(state.depthOverride)
		{
			oDepth = Min(Max(oDepth, Float4(0.0f)), Float4(1.0f));
		}
	}

	Bool PixelProgram::alphaTest(Int cMask[4])
	{
		if(!state.alphaTestActive())
		{
			return true;
		}

		Int aMask;

		if(state.transparencyAntialiasing == TRANSPARENCY_NONE)
		{
			Short4 alpha = RoundShort4(c[0].w * Float4(0x1000));

			PixelRoutine::alphaTest(aMask, alpha);

			for(unsigned int q = 0; q < state.multiSample; q++)
			{
				cMask[q] &= aMask;
			}
		}
		else if(state.transparencyAntialiasing == TRANSPARENCY_ALPHA_TO_COVERAGE)
		{
			alphaToCoverage(cMask, c[0].w);
		}
		else ASSERT(false);

		Int pass = cMask[0];

		for(unsigned int q = 1; q < state.multiSample; q++)
		{
			pass = pass | cMask[q];
		}

		return pass != 0x0;
	}

	void PixelProgram::rasterOperation(Float4 &fog, Pointer<Byte> cBuffer[4], Int &x, Int sMask[4], Int zMask[4], Int cMask[4])
	{
		for(int index = 0; index < RENDERTARGETS; index++)
		{
			if(!state.colorWriteActive(index))
			{
				continue;
			}

			if(!postBlendSRGB && state.writeSRGB && !isSRGB(index))
			{
				c[index].x = linearToSRGB(c[index].x);
				c[index].y = linearToSRGB(c[index].y);
				c[index].z = linearToSRGB(c[index].z);
			}

			if(index == 0)
			{
				fogBlend(c[index], fog);
			}

			switch(state.targetFormat[index])
			{
			case FORMAT_R5G6B5:
			case FORMAT_X8R8G8B8:
			case FORMAT_X8B8G8R8:
			case FORMAT_A8R8G8B8:
			case FORMAT_A8B8G8R8:
			case FORMAT_SRGB8_X8:
			case FORMAT_SRGB8_A8:
			case FORMAT_G8R8:
			case FORMAT_R8:
			case FORMAT_A8:
			case FORMAT_G16R16:
			case FORMAT_A16B16G16R16:
				for(unsigned int q = 0; q < state.multiSample; q++)
				{
					Pointer<Byte> buffer = cBuffer[index] + q * *Pointer<Int>(data + OFFSET(DrawData, colorSliceB[index]));
					Vector4s color;

					if(state.targetFormat[index] == FORMAT_R5G6B5)
					{
						color.x = UShort4(c[index].x * Float4(0xFBFF), false);
						color.y = UShort4(c[index].y * Float4(0xFDFF), false);
						color.z = UShort4(c[index].z * Float4(0xFBFF), false);
						color.w = UShort4(c[index].w * Float4(0xFFFF), false);
					}
					else
					{
						color.x = convertFixed16(c[index].x, false);
						color.y = convertFixed16(c[index].y, false);
						color.z = convertFixed16(c[index].z, false);
						color.w = convertFixed16(c[index].w, false);
					}

					if(state.multiSampleMask & (1 << q))
					{
						alphaBlend(index, buffer, color, x);
						logicOperation(index, buffer, color, x);
						writeColor(index, buffer, x, color, sMask[q], zMask[q], cMask[q]);
					}
				}
				break;
			case FORMAT_R32F:
			case FORMAT_G32R32F:
			case FORMAT_X32B32G32R32F:
			case FORMAT_A32B32G32R32F:
			case FORMAT_X32B32G32R32F_UNSIGNED:
			case FORMAT_R32I:
			case FORMAT_G32R32I:
			case FORMAT_A32B32G32R32I:
			case FORMAT_R32UI:
			case FORMAT_G32R32UI:
			case FORMAT_A32B32G32R32UI:
			case FORMAT_R16I:
			case FORMAT_G16R16I:
			case FORMAT_A16B16G16R16I:
			case FORMAT_R16UI:
			case FORMAT_G16R16UI:
			case FORMAT_A16B16G16R16UI:
			case FORMAT_R8I:
			case FORMAT_G8R8I:
			case FORMAT_A8B8G8R8I:
			case FORMAT_R8UI:
			case FORMAT_G8R8UI:
			case FORMAT_A8B8G8R8UI:
				for(unsigned int q = 0; q < state.multiSample; q++)
				{
					Pointer<Byte> buffer = cBuffer[index] + q * *Pointer<Int>(data + OFFSET(DrawData, colorSliceB[index]));
					Vector4f color = c[index];

					if(state.multiSampleMask & (1 << q))
					{
						alphaBlend(index, buffer, color, x);
						writeColor(index, buffer, x, color, sMask[q], zMask[q], cMask[q]);
					}
				}
				break;
			default:
				ASSERT(false);
			}
		}
	}

	Vector4f PixelProgram::sampleTexture(const Src &sampler, Vector4f &uvwq, Float4 &bias, Vector4f &dsx, Vector4f &dsy, Vector4f &offset, SamplerFunction function)
	{
		Vector4f tmp;

		if(sampler.type == Shader::PARAMETER_SAMPLER && sampler.rel.type == Shader::PARAMETER_VOID)
		{
			tmp = sampleTexture(sampler.index, uvwq, bias, dsx, dsy, offset, function);
		}
		else
		{
			Int index = As<Int>(Float(fetchRegister(sampler).x.x));

			for(int i = 0; i < TEXTURE_IMAGE_UNITS; i++)
			{
				if(shader->usesSampler(i))
				{
					If(index == i)
					{
						tmp = sampleTexture(i, uvwq, bias, dsx, dsy, offset, function);
						// FIXME: When the sampler states are the same, we could use one sampler and just index the texture
					}
				}
			}
		}

		Vector4f c;
		c.x = tmp[(sampler.swizzle >> 0) & 0x3];
		c.y = tmp[(sampler.swizzle >> 2) & 0x3];
		c.z = tmp[(sampler.swizzle >> 4) & 0x3];
		c.w = tmp[(sampler.swizzle >> 6) & 0x3];

		return c;
	}

	Vector4f PixelProgram::sampleTexture(int samplerIndex, Vector4f &uvwq, Float4 &bias, Vector4f &dsx, Vector4f &dsy, Vector4f &offset, SamplerFunction function)
	{
		#if PERF_PROFILE
			Long texTime = Ticks();
		#endif

		Pointer<Byte> texture = data + OFFSET(DrawData, mipmap) + samplerIndex * sizeof(Texture);
		Vector4f c = SamplerCore(constants, state.sampler[samplerIndex]).sampleTexture(texture, uvwq.x, uvwq.y, uvwq.z, uvwq.w, bias, dsx, dsy, offset, function);

		#if PERF_PROFILE
			cycles[PERF_TEX] += Ticks() - texTime;
		#endif

		return c;
	}

	void PixelProgram::clampColor(Vector4f oC[RENDERTARGETS])
	{
		for(int index = 0; index < RENDERTARGETS; index++)
		{
			if(!state.colorWriteActive(index) && !(index == 0 && state.alphaTestActive()))
			{
				continue;
			}

			switch(state.targetFormat[index])
			{
			case FORMAT_NULL:
				break;
			case FORMAT_R5G6B5:
			case FORMAT_A8R8G8B8:
			case FORMAT_A8B8G8R8:
			case FORMAT_X8R8G8B8:
			case FORMAT_X8B8G8R8:
			case FORMAT_SRGB8_X8:
			case FORMAT_SRGB8_A8:
			case FORMAT_G8R8:
			case FORMAT_R8:
			case FORMAT_A8:
			case FORMAT_G16R16:
			case FORMAT_A16B16G16R16:
				oC[index].x = Max(oC[index].x, Float4(0.0f)); oC[index].x = Min(oC[index].x, Float4(1.0f));
				oC[index].y = Max(oC[index].y, Float4(0.0f)); oC[index].y = Min(oC[index].y, Float4(1.0f));
				oC[index].z = Max(oC[index].z, Float4(0.0f)); oC[index].z = Min(oC[index].z, Float4(1.0f));
				oC[index].w = Max(oC[index].w, Float4(0.0f)); oC[index].w = Min(oC[index].w, Float4(1.0f));
				break;
			case FORMAT_R32F:
			case FORMAT_G32R32F:
			case FORMAT_X32B32G32R32F:
			case FORMAT_A32B32G32R32F:
			case FORMAT_R32I:
			case FORMAT_G32R32I:
			case FORMAT_A32B32G32R32I:
			case FORMAT_R32UI:
			case FORMAT_G32R32UI:
			case FORMAT_A32B32G32R32UI:
			case FORMAT_R16I:
			case FORMAT_G16R16I:
			case FORMAT_A16B16G16R16I:
			case FORMAT_R16UI:
			case FORMAT_G16R16UI:
			case FORMAT_A16B16G16R16UI:
			case FORMAT_R8I:
			case FORMAT_G8R8I:
			case FORMAT_A8B8G8R8I:
			case FORMAT_R8UI:
			case FORMAT_G8R8UI:
			case FORMAT_A8B8G8R8UI:
				break;
			case FORMAT_X32B32G32R32F_UNSIGNED:
				oC[index].x = Max(oC[index].x, Float4(0.0f));
				oC[index].y = Max(oC[index].y, Float4(0.0f));
				oC[index].z = Max(oC[index].z, Float4(0.0f));
				oC[index].w = Max(oC[index].w, Float4(0.0f));
				break;
			default:
				ASSERT(false);
			}
		}
	}

	Int4 PixelProgram::enableMask(const Shader::Instruction *instruction)
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

	Vector4f PixelProgram::fetchRegister(const Src &src, unsigned int offset)
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
				Int a = relativeAddress(src, src.bufferIndex);

				reg = r[i + a];
			}
			break;
		case Shader::PARAMETER_INPUT:
			{
				if(src.rel.type == Shader::PARAMETER_VOID)   // Not relative
				{
					reg = v[i];
				}
				else
				{
					Int a = relativeAddress(src, src.bufferIndex);

					reg = v[i + a];
				}
			}
			break;
		case Shader::PARAMETER_CONST:
			reg = readConstant(src, offset);
			break;
		case Shader::PARAMETER_TEXTURE:
			reg = v[2 + i];
			break;
		case Shader::PARAMETER_MISCTYPE:
			if(src.index == Shader::VPosIndex) reg = vPos;
			if(src.index == Shader::VFaceIndex) reg = vFace;
			break;
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
		case Shader::PARAMETER_PREDICATE:   return reg; // Dummy
		case Shader::PARAMETER_VOID:        return reg; // Dummy
		case Shader::PARAMETER_FLOAT4LITERAL:
			reg.x = Float4(src.value[0]);
			reg.y = Float4(src.value[1]);
			reg.z = Float4(src.value[2]);
			reg.w = Float4(src.value[3]);
			break;
		case Shader::PARAMETER_CONSTINT:    return reg; // Dummy
		case Shader::PARAMETER_CONSTBOOL:   return reg; // Dummy
		case Shader::PARAMETER_LOOP:        return reg; // Dummy
		case Shader::PARAMETER_COLOROUT:
			if(src.rel.type == Shader::PARAMETER_VOID)   // Not relative
			{
				reg = oC[i];
			}
			else
			{
				Int a = relativeAddress(src, src.bufferIndex);

				reg = oC[i + a];
			}
			break;
		case Shader::PARAMETER_DEPTHOUT:
			reg.x = oDepth;
			break;
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

	RValue<Pointer<Byte>> PixelProgram::uniformAddress(int bufferIndex, unsigned int index)
	{
		if(bufferIndex == -1)
		{
			return data + OFFSET(DrawData, ps.c[index]);
		}
		else
		{
			return *Pointer<Pointer<Byte>>(data + OFFSET(DrawData, ps.u[bufferIndex])) + index;
		}
	}

	RValue<Pointer<Byte>> PixelProgram::uniformAddress(int bufferIndex, unsigned int index, Int& offset)
	{
		return uniformAddress(bufferIndex, index) + offset * sizeof(float4);
	}

	Vector4f PixelProgram::readConstant(const Src &src, unsigned int offset)
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
			Int a = relativeAddress(src, src.bufferIndex);

			c.x = c.y = c.z = c.w = *Pointer<Float4>(uniformAddress(src.bufferIndex, i, a));

			c.x = c.x.xxxx;
			c.y = c.y.yyyy;
			c.z = c.z.zzzz;
			c.w = c.w.wwww;
		}

		return c;
	}

	Int PixelProgram::relativeAddress(const Shader::Parameter &var, int bufferIndex)
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
			return As<Int>(Extract(oC[var.rel.index].x, 0)) * var.rel.scale;
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

	Float4 PixelProgram::linearToSRGB(const Float4 &x)   // Approximates x^(1.0/2.2)
	{
		Float4 sqrtx = Rcp_pp(RcpSqrt_pp(x));
		Float4 sRGB = sqrtx * Float4(1.14f) - x * Float4(0.14f);

		return Min(Max(sRGB, Float4(0.0f)), Float4(1.0f));
	}

	void PixelProgram::M3X2(Vector4f &dst, Vector4f &src0, const Src &src1)
	{
		Vector4f row0 = fetchRegister(src1, 0);
		Vector4f row1 = fetchRegister(src1, 1);

		dst.x = dot3(src0, row0);
		dst.y = dot3(src0, row1);
	}

	void PixelProgram::M3X3(Vector4f &dst, Vector4f &src0, const Src &src1)
	{
		Vector4f row0 = fetchRegister(src1, 0);
		Vector4f row1 = fetchRegister(src1, 1);
		Vector4f row2 = fetchRegister(src1, 2);

		dst.x = dot3(src0, row0);
		dst.y = dot3(src0, row1);
		dst.z = dot3(src0, row2);
	}

	void PixelProgram::M3X4(Vector4f &dst, Vector4f &src0, const Src &src1)
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

	void PixelProgram::M4X3(Vector4f &dst, Vector4f &src0, const Src &src1)
	{
		Vector4f row0 = fetchRegister(src1, 0);
		Vector4f row1 = fetchRegister(src1, 1);
		Vector4f row2 = fetchRegister(src1, 2);

		dst.x = dot4(src0, row0);
		dst.y = dot4(src0, row1);
		dst.z = dot4(src0, row2);
	}

	void PixelProgram::M4X4(Vector4f &dst, Vector4f &src0, const Src &src1)
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

	void PixelProgram::TEX(Vector4f &dst, Vector4f &src0, const Src &src1, bool project, bool bias)
	{
		if(project)
		{
			Vector4f proj;
			Float4 rw = reciprocal(src0.w);
			proj.x = src0.x * rw;
			proj.y = src0.y * rw;
			proj.z = src0.z * rw;

			dst = sampleTexture(src1, proj, src0.x, (src0), (src0), (src0), Implicit);
		}
		else
		{
			dst = sampleTexture(src1, src0, src0.x, (src0), (src0), (src0), bias ? Bias : Implicit);
		}
	}

	void PixelProgram::TEXOFFSET(Vector4f &dst, Vector4f &src0, const Src &src1, Vector4f &offset)
	{
		dst = sampleTexture(src1, src0, (src0.x), (src0), (src0), offset, {Implicit, Offset});
	}

	void PixelProgram::TEXLODOFFSET(Vector4f &dst, Vector4f &src0, const Src &src1, Vector4f &offset, Float4 &lod)
	{
		dst = sampleTexture(src1, src0, lod, (src0), (src0), offset, {Lod, Offset});
	}

	void PixelProgram::TEXBIAS(Vector4f &dst, Vector4f &src0, const Src &src1, Float4 &bias)
	{
		dst = sampleTexture(src1, src0, bias, (src0), (src0), (src0), Bias);
	}

	void PixelProgram::TEXOFFSETBIAS(Vector4f &dst, Vector4f &src0, const Src &src1, Vector4f &offset, Float4 &bias)
	{
		dst = sampleTexture(src1, src0, bias, (src0), (src0), offset, {Bias, Offset});
	}

	void PixelProgram::TEXELFETCH(Vector4f &dst, Vector4f &src0, const Src& src1, Float4 &lod)
	{
		dst = sampleTexture(src1, src0, lod, (src0), (src0), (src0), Fetch);
	}

	void PixelProgram::TEXELFETCHOFFSET(Vector4f &dst, Vector4f &src0, const Src& src1, Vector4f &offset, Float4 &lod)
	{
		dst = sampleTexture(src1, src0, lod, (src0), (src0), offset, {Fetch, Offset});
	}

	void PixelProgram::TEXGRAD(Vector4f &dst, Vector4f &src0, const Src& src1, Vector4f &dsx, Vector4f &dsy)
	{
		dst = sampleTexture(src1, src0, (src0.x), dsx, dsy, (src0), Grad);
	}

	void PixelProgram::TEXGRADOFFSET(Vector4f &dst, Vector4f &src0, const Src& src1, Vector4f &dsx, Vector4f &dsy, Vector4f &offset)
	{
		dst = sampleTexture(src1, src0, (src0.x), dsx, dsy, offset, {Grad, Offset});
	}

	void PixelProgram::TEXLOD(Vector4f &dst, Vector4f &src0, const Src &src1, Float4 &lod)
	{
		dst = sampleTexture(src1, src0, lod, (src0), (src0), (src0), Lod);
	}

	void PixelProgram::TEXSIZE(Vector4f &dst, Float4 &lod, const Src &src1)
	{
		Pointer<Byte> texture = data + OFFSET(DrawData, mipmap) + src1.index * sizeof(Texture);
		dst = SamplerCore::textureSize(texture, lod);
	}

	void PixelProgram::TEXKILL(Int cMask[4], Vector4f &src, unsigned char mask)
	{
		Int kill = -1;

		if(mask & 0x1) kill &= SignMask(CmpNLT(src.x, Float4(0.0f)));
		if(mask & 0x2) kill &= SignMask(CmpNLT(src.y, Float4(0.0f)));
		if(mask & 0x4) kill &= SignMask(CmpNLT(src.z, Float4(0.0f)));
		if(mask & 0x8) kill &= SignMask(CmpNLT(src.w, Float4(0.0f)));

		// FIXME: Dynamic branching affects TEXKILL?
		//	if(shader->containsDynamicBranching())
		//	{
		//		kill = ~SignMask(enableMask());
		//	}

		for(unsigned int q = 0; q < state.multiSample; q++)
		{
			cMask[q] &= kill;
		}

		// FIXME: Branch to end of shader if all killed?
	}

	void PixelProgram::DISCARD(Int cMask[4], const Shader::Instruction *instruction)
	{
		Int kill = 0;

		if(shader->containsDynamicBranching())
		{
			kill = ~SignMask(enableMask(instruction));
		}

		for(unsigned int q = 0; q < state.multiSample; q++)
		{
			cMask[q] &= kill;
		}

		// FIXME: Branch to end of shader if all killed?
	}

	void PixelProgram::DFDX(Vector4f &dst, Vector4f &src)
	{
		dst.x = src.x.yyww - src.x.xxzz;
		dst.y = src.y.yyww - src.y.xxzz;
		dst.z = src.z.yyww - src.z.xxzz;
		dst.w = src.w.yyww - src.w.xxzz;
	}

	void PixelProgram::DFDY(Vector4f &dst, Vector4f &src)
	{
		dst.x = src.x.zwzw - src.x.xyxy;
		dst.y = src.y.zwzw - src.y.xyxy;
		dst.z = src.z.zwzw - src.z.xyxy;
		dst.w = src.w.zwzw - src.w.xyxy;
	}

	void PixelProgram::FWIDTH(Vector4f &dst, Vector4f &src)
	{
		// abs(dFdx(src)) + abs(dFdy(src));
		dst.x = Abs(src.x.yyww - src.x.xxzz) + Abs(src.x.zwzw - src.x.xyxy);
		dst.y = Abs(src.y.yyww - src.y.xxzz) + Abs(src.y.zwzw - src.y.xyxy);
		dst.z = Abs(src.z.yyww - src.z.xxzz) + Abs(src.z.zwzw - src.z.xyxy);
		dst.w = Abs(src.w.yyww - src.w.xxzz) + Abs(src.w.zwzw - src.w.xyxy);
	}

	void PixelProgram::BREAK()
	{
		enableBreak = enableBreak & ~enableStack[enableIndex];
	}

	void PixelProgram::BREAKC(Vector4f &src0, Vector4f &src1, Control control)
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

	void PixelProgram::BREAKP(const Src &predicateRegister)   // FIXME: Factor out parts common with BREAKC
	{
		Int4 condition = As<Int4>(p0[predicateRegister.swizzle & 0x3]);

		if(predicateRegister.modifier == Shader::MODIFIER_NOT)
		{
			condition = ~condition;
		}

		BREAK(condition);
	}

	void PixelProgram::BREAK(Int4 &condition)
	{
		condition &= enableStack[enableIndex];

		enableBreak = enableBreak & ~condition;
	}

	void PixelProgram::CONTINUE()
	{
		enableContinue = enableContinue & ~enableStack[enableIndex];
	}

	void PixelProgram::TEST()
	{
		whileTest = true;
	}

	void PixelProgram::CALL(int labelIndex, int callSiteIndex)
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

	void PixelProgram::CALLNZ(int labelIndex, int callSiteIndex, const Src &src)
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

	void PixelProgram::CALLNZb(int labelIndex, int callSiteIndex, const Src &boolRegister)
	{
		Bool condition = (*Pointer<Byte>(data + OFFSET(DrawData, ps.b[boolRegister.index])) != Byte(0));   // FIXME

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

	void PixelProgram::CALLNZp(int labelIndex, int callSiteIndex, const Src &predicateRegister)
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

	void PixelProgram::ELSE()
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

	void PixelProgram::ENDIF()
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

	void PixelProgram::ENDLOOP()
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

	void PixelProgram::ENDREP()
	{
		loopRepDepth--;

		BasicBlock *testBlock = loopRepTestBlock[loopRepDepth];
		BasicBlock *endBlock = loopRepEndBlock[loopRepDepth];

		Nucleus::createBr(testBlock);
		Nucleus::setInsertBlock(endBlock);

		loopDepth--;
		enableBreak = Int4(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);
	}

	void PixelProgram::ENDWHILE()
	{
		loopRepDepth--;

		BasicBlock *testBlock = loopRepTestBlock[loopRepDepth];
		BasicBlock *endBlock = loopRepEndBlock[loopRepDepth];

		Nucleus::createBr(testBlock);
		Nucleus::setInsertBlock(endBlock);

		enableIndex--;
		whileTest = false;
	}

	void PixelProgram::ENDSWITCH()
	{
		loopRepDepth--;

		BasicBlock *endBlock = loopRepEndBlock[loopRepDepth];

		Nucleus::createBr(endBlock);
		Nucleus::setInsertBlock(endBlock);
	}

	void PixelProgram::IF(const Src &src)
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

	void PixelProgram::IFb(const Src &boolRegister)
	{
		ASSERT(ifDepth < 24 + 4);

		Bool condition = (*Pointer<Byte>(data + OFFSET(DrawData, ps.b[boolRegister.index])) != Byte(0));   // FIXME

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

	void PixelProgram::IFp(const Src &predicateRegister)
	{
		Int4 condition = As<Int4>(p0[predicateRegister.swizzle & 0x3]);

		if(predicateRegister.modifier == Shader::MODIFIER_NOT)
		{
			condition = ~condition;
		}

		IF(condition);
	}

	void PixelProgram::IFC(Vector4f &src0, Vector4f &src1, Control control)
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

	void PixelProgram::IF(Int4 &condition)
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

	void PixelProgram::LABEL(int labelIndex)
	{
		if(!labelBlock[labelIndex])
		{
			labelBlock[labelIndex] = Nucleus::createBasicBlock();
		}

		Nucleus::setInsertBlock(labelBlock[labelIndex]);
		currentLabel = labelIndex;
	}

	void PixelProgram::LOOP(const Src &integerRegister)
	{
		loopDepth++;

		iteration[loopDepth] = *Pointer<Int>(data + OFFSET(DrawData, ps.i[integerRegister.index][0]));
		aL[loopDepth] = *Pointer<Int>(data + OFFSET(DrawData, ps.i[integerRegister.index][1]));
		increment[loopDepth] = *Pointer<Int>(data + OFFSET(DrawData, ps.i[integerRegister.index][2]));

		//	If(increment[loopDepth] == 0)
		//	{
		//		increment[loopDepth] = 1;
		//	}

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

	void PixelProgram::REP(const Src &integerRegister)
	{
		loopDepth++;

		iteration[loopDepth] = *Pointer<Int>(data + OFFSET(DrawData, ps.i[integerRegister.index][0]));
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

	void PixelProgram::WHILE(const Src &temporaryRegister)
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

	void PixelProgram::SWITCH()
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

	void PixelProgram::RET()
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

	void PixelProgram::LEAVE()
	{
		enableLeave = enableLeave & ~enableStack[enableIndex];

		// FIXME: Return from function if all instances left
		// FIXME: Use enableLeave in other control-flow constructs
	}
}
