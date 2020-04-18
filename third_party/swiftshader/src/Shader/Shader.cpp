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

#include "Shader.hpp"

#include "VertexShader.hpp"
#include "PixelShader.hpp"
#include "Common/Math.hpp"
#include "Common/Debug.hpp"

#include <set>
#include <fstream>
#include <sstream>
#include <stdarg.h>

namespace sw
{
	volatile int Shader::serialCounter = 1;

	Shader::Opcode Shader::OPCODE_DP(int i)
	{
		switch(i)
		{
		default: ASSERT(false);
		case 1: return OPCODE_DP1;
		case 2: return OPCODE_DP2;
		case 3: return OPCODE_DP3;
		case 4: return OPCODE_DP4;
		}
	}

	Shader::Opcode Shader::OPCODE_LEN(int i)
	{
		switch(i)
		{
		default: ASSERT(false);
		case 1: return OPCODE_ABS;
		case 2: return OPCODE_LEN2;
		case 3: return OPCODE_LEN3;
		case 4: return OPCODE_LEN4;
		}
	}

	Shader::Opcode Shader::OPCODE_DIST(int i)
	{
		switch(i)
		{
		default: ASSERT(false);
		case 1: return OPCODE_DIST1;
		case 2: return OPCODE_DIST2;
		case 3: return OPCODE_DIST3;
		case 4: return OPCODE_DIST4;
		}
	}

	Shader::Opcode Shader::OPCODE_NRM(int i)
	{
		switch(i)
		{
		default: ASSERT(false);
		case 1: return OPCODE_SGN;
		case 2: return OPCODE_NRM2;
		case 3: return OPCODE_NRM3;
		case 4: return OPCODE_NRM4;
		}
	}

	Shader::Opcode Shader::OPCODE_FORWARD(int i)
	{
		switch(i)
		{
		default: ASSERT(false);
		case 1: return OPCODE_FORWARD1;
		case 2: return OPCODE_FORWARD2;
		case 3: return OPCODE_FORWARD3;
		case 4: return OPCODE_FORWARD4;
		}
	}

	Shader::Opcode Shader::OPCODE_REFLECT(int i)
	{
		switch(i)
		{
		default: ASSERT(false);
		case 1: return OPCODE_REFLECT1;
		case 2: return OPCODE_REFLECT2;
		case 3: return OPCODE_REFLECT3;
		case 4: return OPCODE_REFLECT4;
		}
	}

	Shader::Opcode Shader::OPCODE_REFRACT(int i)
	{
		switch(i)
		{
		default: ASSERT(false);
		case 1: return OPCODE_REFRACT1;
		case 2: return OPCODE_REFRACT2;
		case 3: return OPCODE_REFRACT3;
		case 4: return OPCODE_REFRACT4;
		}
	}

	Shader::Instruction::Instruction(Opcode opcode) : opcode(opcode), analysis(0)
	{
		control = CONTROL_RESERVED0;

		predicate = false;
		predicateNot = false;
		predicateSwizzle = 0xE4;

		coissue = false;
		samplerType = SAMPLER_UNKNOWN;
		usage = USAGE_POSITION;
		usageIndex = 0;
	}

	Shader::Instruction::Instruction(const unsigned long *token, int size, unsigned char majorVersion) : analysis(0)
	{
		parseOperationToken(*token++, majorVersion);

		samplerType = SAMPLER_UNKNOWN;
		usage = USAGE_POSITION;
		usageIndex = 0;

		if(opcode == OPCODE_IF ||
		   opcode == OPCODE_IFC ||
		   opcode == OPCODE_LOOP ||
		   opcode == OPCODE_REP ||
		   opcode == OPCODE_BREAKC ||
		   opcode == OPCODE_BREAKP)   // No destination operand
		{
			if(size > 0) parseSourceToken(0, token++, majorVersion);
			if(size > 1) parseSourceToken(1, token++, majorVersion);
			if(size > 2) parseSourceToken(2, token++, majorVersion);
			if(size > 3) ASSERT(false);
		}
		else if(opcode == OPCODE_DCL)
		{
			parseDeclarationToken(*token++);
			parseDestinationToken(token++, majorVersion);
		}
		else
		{
			if(size > 0)
			{
				parseDestinationToken(token, majorVersion);

				if(dst.rel.type != PARAMETER_VOID && majorVersion >= 3)
				{
					token++;
					size--;
				}

				token++;
				size--;
			}

			if(predicate)
			{
				ASSERT(size != 0);

				predicateNot = (Modifier)((*token & 0x0F000000) >> 24) == MODIFIER_NOT;
				predicateSwizzle = (unsigned char)((*token & 0x00FF0000) >> 16);

				token++;
				size--;
			}

			for(int i = 0; size > 0; i++)
			{
				parseSourceToken(i, token, majorVersion);

				token++;
				size--;

				if(src[i].rel.type != PARAMETER_VOID && majorVersion >= 2)
				{
					token++;
					size--;
				}
			}
		}
	}

	Shader::Instruction::~Instruction()
	{
	}

	std::string Shader::Instruction::string(ShaderType shaderType, unsigned short version) const
	{
		std::string instructionString;

		if(opcode != OPCODE_DCL)
		{
			instructionString += coissue ? "+ " : "";

			if(predicate)
			{
				instructionString += predicateNot ? "(!p0" : "(p0";
				instructionString += swizzleString(PARAMETER_PREDICATE, predicateSwizzle);
				instructionString += ") ";
			}

			instructionString += operationString(version) + controlString() + dst.shiftString() + dst.modifierString();

			if(dst.type != PARAMETER_VOID)
			{
				instructionString += " " + dst.string(shaderType, version) +
				                           dst.relativeString() +
				                           dst.maskString();
			}

			for(int i = 0; i < 4; i++)
			{
				if(src[i].type != PARAMETER_VOID)
				{
					instructionString += (dst.type != PARAMETER_VOID || i > 0) ? ", " : " ";
					instructionString += src[i].preModifierString() +
										 src[i].string(shaderType, version) +
										 src[i].relativeString() +
										 src[i].postModifierString() +
										 src[i].swizzleString();
				}
			}
		}
		else   // DCL
		{
			instructionString += "dcl";

			if(dst.type == PARAMETER_SAMPLER)
			{
				switch(samplerType)
				{
				case SAMPLER_UNKNOWN: instructionString += " ";        break;
				case SAMPLER_1D:      instructionString += "_1d ";     break;
				case SAMPLER_2D:      instructionString += "_2d ";     break;
				case SAMPLER_CUBE:    instructionString += "_cube ";   break;
				case SAMPLER_VOLUME:  instructionString += "_volume "; break;
				default:
					ASSERT(false);
				}

				instructionString += dst.string(shaderType, version);
			}
			else if(dst.type == PARAMETER_INPUT ||
				    dst.type == PARAMETER_OUTPUT ||
				    dst.type == PARAMETER_TEXTURE)
			{
				if(version >= 0x0300)
				{
					switch(usage)
					{
					case USAGE_POSITION:     instructionString += "_position";     break;
					case USAGE_BLENDWEIGHT:  instructionString += "_blendweight";  break;
					case USAGE_BLENDINDICES: instructionString += "_blendindices"; break;
					case USAGE_NORMAL:       instructionString += "_normal";       break;
					case USAGE_PSIZE:        instructionString += "_psize";        break;
					case USAGE_TEXCOORD:     instructionString += "_texcoord";     break;
					case USAGE_TANGENT:      instructionString += "_tangent";      break;
					case USAGE_BINORMAL:     instructionString += "_binormal";     break;
					case USAGE_TESSFACTOR:   instructionString += "_tessfactor";   break;
					case USAGE_POSITIONT:    instructionString += "_positiont";    break;
					case USAGE_COLOR:        instructionString += "_color";        break;
					case USAGE_FOG:          instructionString += "_fog";          break;
					case USAGE_DEPTH:        instructionString += "_depth";        break;
					case USAGE_SAMPLE:       instructionString += "_sample";       break;
					default:
						ASSERT(false);
					}

					if(usageIndex > 0)
					{
						std::ostringstream buffer;

						buffer << (int)usageIndex;

						instructionString += buffer.str();
					}
				}
				else ASSERT(dst.type != PARAMETER_OUTPUT);

				instructionString += " ";

				instructionString += dst.string(shaderType, version);
				instructionString += dst.maskString();
			}
			else if(dst.type == PARAMETER_MISCTYPE)   // vPos and vFace
			{
				instructionString += " ";

				instructionString += dst.string(shaderType, version);
			}
			else ASSERT(false);
		}

		return instructionString;
	}

	std::string Shader::DestinationParameter::modifierString() const
	{
		if(type == PARAMETER_VOID || type == PARAMETER_LABEL)
		{
			return "";
		}

		std::string modifierString;

		if(saturate)
		{
			modifierString += "_sat";
		}

		if(partialPrecision)
		{
			modifierString += "_pp";
		}

		if(centroid)
		{
			modifierString += "_centroid";
		}

		return modifierString;
	}

	std::string Shader::DestinationParameter::shiftString() const
	{
		if(type == PARAMETER_VOID || type == PARAMETER_LABEL)
		{
			return "";
		}

		switch(shift)
		{
		case 0:		return "";
		case 1:		return "_x2";
		case 2:		return "_x4";
		case 3:		return "_x8";
		case -1:	return "_d2";
		case -2:	return "_d4";
		case -3:	return "_d8";
		default:
			return "";
		//	ASSERT(false);   // FIXME
		}
	}

	std::string Shader::DestinationParameter::maskString() const
	{
		if(type == PARAMETER_VOID || type == PARAMETER_LABEL)
		{
			return "";
		}

		switch(mask)
		{
		case 0x0:	return "";
		case 0x1:	return ".x";
		case 0x2:	return ".y";
		case 0x3:	return ".xy";
		case 0x4:	return ".z";
		case 0x5:	return ".xz";
		case 0x6:	return ".yz";
		case 0x7:	return ".xyz";
		case 0x8:	return ".w";
		case 0x9:	return ".xw";
		case 0xA:	return ".yw";
		case 0xB:	return ".xyw";
		case 0xC:	return ".zw";
		case 0xD:	return ".xzw";
		case 0xE:	return ".yzw";
		case 0xF:	return "";
		default:
			ASSERT(false);
		}

		return "";
	}

	std::string Shader::SourceParameter::preModifierString() const
	{
		if(type == PARAMETER_VOID)
		{
			return "";
		}

		switch(modifier)
		{
		case MODIFIER_NONE:			return "";
		case MODIFIER_NEGATE:		return "-";
		case MODIFIER_BIAS:			return "";
		case MODIFIER_BIAS_NEGATE:	return "-";
		case MODIFIER_SIGN:			return "";
		case MODIFIER_SIGN_NEGATE:	return "-";
		case MODIFIER_COMPLEMENT:	return "1-";
		case MODIFIER_X2:			return "";
		case MODIFIER_X2_NEGATE:	return "-";
		case MODIFIER_DZ:			return "";
		case MODIFIER_DW:			return "";
		case MODIFIER_ABS:			return "";
		case MODIFIER_ABS_NEGATE:	return "-";
		case MODIFIER_NOT:			return "!";
		default:
			ASSERT(false);
		}

		return "";
	}

	std::string Shader::Parameter::relativeString() const
	{
		if(type == PARAMETER_CONST || type == PARAMETER_INPUT || type == PARAMETER_OUTPUT || type == PARAMETER_TEMP)
		{
			if(rel.type == PARAMETER_VOID)
			{
				return "";
			}
			else if(rel.type == PARAMETER_ADDR)
			{
				switch(rel.swizzle & 0x03)
				{
				case 0: return "[a0.x]";
				case 1: return "[a0.y]";
				case 2: return "[a0.z]";
				case 3: return "[a0.w]";
				}
			}
			else if(rel.type == PARAMETER_TEMP)
			{
				std::ostringstream buffer;
				buffer << rel.index;

				switch(rel.swizzle & 0x03)
				{
				case 0: return "[r" + buffer.str() + ".x]";
				case 1: return "[r" + buffer.str() + ".y]";
				case 2: return "[r" + buffer.str() + ".z]";
				case 3: return "[r" + buffer.str() + ".w]";
				}
			}
			else if(rel.type == PARAMETER_LOOP)
			{
				return "[aL]";
			}
			else if(rel.type == PARAMETER_CONST)
			{
				std::ostringstream buffer;
				buffer << rel.index;

				switch(rel.swizzle & 0x03)
				{
				case 0: return "[c" + buffer.str() + ".x]";
				case 1: return "[c" + buffer.str() + ".y]";
				case 2: return "[c" + buffer.str() + ".z]";
				case 3: return "[c" + buffer.str() + ".w]";
				}
			}
			else ASSERT(false);
		}

		return "";
	}

	std::string Shader::SourceParameter::postModifierString() const
	{
		if(type == PARAMETER_VOID)
		{
			return "";
		}

		switch(modifier)
		{
		case MODIFIER_NONE:			return "";
		case MODIFIER_NEGATE:		return "";
		case MODIFIER_BIAS:			return "_bias";
		case MODIFIER_BIAS_NEGATE:	return "_bias";
		case MODIFIER_SIGN:			return "_bx2";
		case MODIFIER_SIGN_NEGATE:	return "_bx2";
		case MODIFIER_COMPLEMENT:	return "";
		case MODIFIER_X2:			return "_x2";
		case MODIFIER_X2_NEGATE:	return "_x2";
		case MODIFIER_DZ:			return "_dz";
		case MODIFIER_DW:			return "_dw";
		case MODIFIER_ABS:			return "_abs";
		case MODIFIER_ABS_NEGATE:	return "_abs";
		case MODIFIER_NOT:			return "";
		default:
			ASSERT(false);
		}

		return "";
	}

	std::string Shader::SourceParameter::string(ShaderType shaderType, unsigned short version) const
	{
		if(type == PARAMETER_CONST && bufferIndex >= 0)
		{
			std::ostringstream buffer;
			buffer << bufferIndex;

			std::ostringstream offset;
			offset << index;

			return "cb" + buffer.str() + "[" + offset.str() + "]";
		}
		else
		{
			return Parameter::string(shaderType, version);
		}
	}

	std::string Shader::SourceParameter::swizzleString() const
	{
		return Instruction::swizzleString(type, swizzle);
	}

	void Shader::Instruction::parseOperationToken(unsigned long token, unsigned char majorVersion)
	{
		if((token & 0xFFFF0000) == 0xFFFF0000 || (token & 0xFFFF0000) == 0xFFFE0000)   // Version token
		{
			opcode = (Opcode)token;

			control = CONTROL_RESERVED0;
			predicate = false;
			coissue = false;
		}
		else
		{
			opcode = (Opcode)(token & 0x0000FFFF);
			control = (Control)((token & 0x00FF0000) >> 16);

			int size = (token & 0x0F000000) >> 24;

			predicate = (token & 0x10000000) != 0x00000000;
			coissue = (token & 0x40000000) != 0x00000000;

			if(majorVersion < 2)
			{
				if(size != 0)
				{
					ASSERT(false);   // Reserved
				}
			}

			if(majorVersion < 2)
			{
				if(predicate)
				{
					ASSERT(false);
				}
			}

			if((token & 0x20000000) != 0x00000000)
			{
				ASSERT(false);   // Reserved
			}

			if(majorVersion >= 2)
			{
				if(coissue)
				{
					ASSERT(false);   // Reserved
				}
			}

			if((token & 0x80000000) != 0x00000000)
			{
				ASSERT(false);
			}
		}
	}

	void Shader::Instruction::parseDeclarationToken(unsigned long token)
	{
		samplerType = (SamplerType)((token & 0x78000000) >> 27);
		usage = (Usage)(token & 0x0000001F);
		usageIndex = (unsigned char)((token & 0x000F0000) >> 16);
	}

	void Shader::Instruction::parseDestinationToken(const unsigned long *token, unsigned char majorVersion)
	{
		dst.index = (unsigned short)(token[0] & 0x000007FF);
		dst.type = (ParameterType)(((token[0] & 0x00001800) >> 8) | ((token[0] & 0x70000000) >> 28));

		// TODO: Check type and index range

		bool relative = (token[0] & 0x00002000) != 0x00000000;
		dst.rel.type = relative ? PARAMETER_ADDR : PARAMETER_VOID;
		dst.rel.swizzle = 0x00;
		dst.rel.scale = 1;

		if(relative && majorVersion >= 3)
		{
			dst.rel.type = (ParameterType)(((token[1] & 0x00001800) >> 8) | ((token[1] & 0x70000000) >> 28));
			dst.rel.swizzle = (unsigned char)((token[1] & 0x00FF0000) >> 16);
		}
		else if(relative) ASSERT(false);   // Reserved

		if((token[0] & 0x0000C000) != 0x00000000)
		{
			ASSERT(false);   // Reserved
		}

		dst.mask = (unsigned char)((token[0] & 0x000F0000) >> 16);
		dst.saturate = (token[0] & 0x00100000) != 0;
		dst.partialPrecision = (token[0] & 0x00200000) != 0;
		dst.centroid = (token[0] & 0x00400000) != 0;
		dst.shift = (signed char)((token[0] & 0x0F000000) >> 20) >> 4;

		if(majorVersion >= 2)
		{
			if(dst.shift)
			{
				ASSERT(false);   // Reserved
			}
		}

		if((token[0] & 0x80000000) != 0x80000000)
		{
			ASSERT(false);
		}
	}

	void Shader::Instruction::parseSourceToken(int i, const unsigned long *token, unsigned char majorVersion)
	{
		// Defaults
		src[i].index = 0;
		src[i].type = PARAMETER_VOID;
		src[i].modifier = MODIFIER_NONE;
		src[i].swizzle = 0xE4;
		src[i].rel.type = PARAMETER_VOID;
		src[i].rel.swizzle = 0x00;
		src[i].rel.scale = 1;

		switch(opcode)
		{
		case OPCODE_DEF:
			src[0].type = PARAMETER_FLOAT4LITERAL;
			src[0].value[i] = *(float*)token;
			break;
		case OPCODE_DEFB:
			src[0].type = PARAMETER_BOOL1LITERAL;
			src[0].boolean[0] = *(int*)token;
			break;
		case OPCODE_DEFI:
			src[0].type = PARAMETER_INT4LITERAL;
			src[0].integer[i] = *(int*)token;
			break;
		default:
			src[i].index = (unsigned short)(token[0] & 0x000007FF);
			src[i].type = (ParameterType)(((token[0] & 0x00001800) >> 8) | ((token[0] & 0x70000000) >> 28));

			// FIXME: Check type and index range

			bool relative = (token[0] & 0x00002000) != 0x00000000;
			src[i].rel.type = relative ? PARAMETER_ADDR : PARAMETER_VOID;

			if((token[0] & 0x0000C000) != 0x00000000)
			{
				if(opcode != OPCODE_DEF &&
				   opcode != OPCODE_DEFI &&
				   opcode != OPCODE_DEFB)
				{
					ASSERT(false);
				}
			}

			src[i].swizzle = (unsigned char)((token[0] & 0x00FF0000) >> 16);
			src[i].modifier = (Modifier)((token[0] & 0x0F000000) >> 24);

			if((token[0] & 0x80000000) != 0x80000000)
			{
				if(opcode != OPCODE_DEF &&
				   opcode != OPCODE_DEFI &&
				   opcode != OPCODE_DEFB)
				{
					ASSERT(false);
				}
			}

			if(relative && majorVersion >= 2)
			{
				src[i].rel.type = (ParameterType)(((token[1] & 0x00001800) >> 8) | ((token[1] & 0x70000000) >> 28));
				src[i].rel.swizzle = (unsigned char)((token[1] & 0x00FF0000) >> 16);
			}
		}
	}

	std::string Shader::Instruction::swizzleString(ParameterType type, unsigned char swizzle)
	{
		if(type == PARAMETER_VOID || type == PARAMETER_LABEL || swizzle == 0xE4)
		{
			return "";
		}

		int x = (swizzle & 0x03) >> 0;
		int y = (swizzle & 0x0C) >> 2;
		int z = (swizzle & 0x30) >> 4;
		int w = (swizzle & 0xC0) >> 6;

		std::string swizzleString = ".";

		switch(x)
		{
		case 0: swizzleString += "x"; break;
		case 1: swizzleString += "y"; break;
		case 2: swizzleString += "z"; break;
		case 3: swizzleString += "w"; break;
		}

		if(!(x == y && y == z && z == w))
		{
			switch(y)
			{
			case 0: swizzleString += "x"; break;
			case 1: swizzleString += "y"; break;
			case 2: swizzleString += "z"; break;
			case 3: swizzleString += "w"; break;
			}

			if(!(y == z && z == w))
			{
				switch(z)
				{
				case 0: swizzleString += "x"; break;
				case 1: swizzleString += "y"; break;
				case 2: swizzleString += "z"; break;
				case 3: swizzleString += "w"; break;
				}

				if(!(z == w))
				{
					switch(w)
					{
					case 0: swizzleString += "x"; break;
					case 1: swizzleString += "y"; break;
					case 2: swizzleString += "z"; break;
					case 3: swizzleString += "w"; break;
					}
				}
			}
		}

		return swizzleString;
	}

	std::string Shader::Instruction::operationString(unsigned short version) const
	{
		switch(opcode)
		{
		case OPCODE_NULL:            return "null";
		case OPCODE_NOP:             return "nop";
		case OPCODE_MOV:             return "mov";
		case OPCODE_ADD:             return "add";
		case OPCODE_IADD:            return "iadd";
		case OPCODE_SUB:             return "sub";
		case OPCODE_ISUB:            return "isub";
		case OPCODE_MAD:             return "mad";
		case OPCODE_IMAD:            return "imad";
		case OPCODE_MUL:             return "mul";
		case OPCODE_IMUL:            return "imul";
		case OPCODE_RCPX:            return "rcpx";
		case OPCODE_DIV:             return "div";
		case OPCODE_IDIV:            return "idiv";
		case OPCODE_UDIV:            return "udiv";
		case OPCODE_MOD:             return "mod";
		case OPCODE_IMOD:            return "imod";
		case OPCODE_UMOD:            return "umod";
		case OPCODE_SHL:             return "shl";
		case OPCODE_ISHR:            return "ishr";
		case OPCODE_USHR:            return "ushr";
		case OPCODE_RSQX:            return "rsqx";
		case OPCODE_SQRT:            return "sqrt";
		case OPCODE_RSQ:             return "rsq";
		case OPCODE_LEN2:            return "len2";
		case OPCODE_LEN3:            return "len3";
		case OPCODE_LEN4:            return "len4";
		case OPCODE_DIST1:           return "dist1";
		case OPCODE_DIST2:           return "dist2";
		case OPCODE_DIST3:           return "dist3";
		case OPCODE_DIST4:           return "dist4";
		case OPCODE_DP3:             return "dp3";
		case OPCODE_DP4:             return "dp4";
		case OPCODE_DET2:            return "det2";
		case OPCODE_DET3:            return "det3";
		case OPCODE_DET4:            return "det4";
		case OPCODE_MIN:             return "min";
		case OPCODE_IMIN:            return "imin";
		case OPCODE_UMIN:            return "umin";
		case OPCODE_MAX:             return "max";
		case OPCODE_IMAX:            return "imax";
		case OPCODE_UMAX:            return "umax";
		case OPCODE_SLT:             return "slt";
		case OPCODE_SGE:             return "sge";
		case OPCODE_EXP2X:           return "exp2x";
		case OPCODE_LOG2X:           return "log2x";
		case OPCODE_LIT:             return "lit";
		case OPCODE_ATT:             return "att";
		case OPCODE_LRP:             return "lrp";
		case OPCODE_STEP:            return "step";
		case OPCODE_SMOOTH:          return "smooth";
		case OPCODE_FLOATBITSTOINT:  return "floatBitsToInt";
		case OPCODE_FLOATBITSTOUINT: return "floatBitsToUInt";
		case OPCODE_INTBITSTOFLOAT:  return "intBitsToFloat";
		case OPCODE_UINTBITSTOFLOAT: return "uintBitsToFloat";
		case OPCODE_PACKSNORM2x16:   return "packSnorm2x16";
		case OPCODE_PACKUNORM2x16:   return "packUnorm2x16";
		case OPCODE_PACKHALF2x16:    return "packHalf2x16";
		case OPCODE_UNPACKSNORM2x16: return "unpackSnorm2x16";
		case OPCODE_UNPACKUNORM2x16: return "unpackUnorm2x16";
		case OPCODE_UNPACKHALF2x16:  return "unpackHalf2x16";
		case OPCODE_FRC:             return "frc";
		case OPCODE_M4X4:            return "m4x4";
		case OPCODE_M4X3:            return "m4x3";
		case OPCODE_M3X4:            return "m3x4";
		case OPCODE_M3X3:            return "m3x3";
		case OPCODE_M3X2:            return "m3x2";
		case OPCODE_CALL:            return "call";
		case OPCODE_CALLNZ:          return "callnz";
		case OPCODE_LOOP:            return "loop";
		case OPCODE_RET:             return "ret";
		case OPCODE_ENDLOOP:         return "endloop";
		case OPCODE_LABEL:           return "label";
		case OPCODE_DCL:             return "dcl";
		case OPCODE_POWX:            return "powx";
		case OPCODE_CRS:             return "crs";
		case OPCODE_SGN:             return "sgn";
		case OPCODE_ISGN:            return "isgn";
		case OPCODE_ABS:             return "abs";
		case OPCODE_IABS:            return "iabs";
		case OPCODE_NRM2:            return "nrm2";
		case OPCODE_NRM3:            return "nrm3";
		case OPCODE_NRM4:            return "nrm4";
		case OPCODE_SINCOS:          return "sincos";
		case OPCODE_REP:             return "rep";
		case OPCODE_ENDREP:          return "endrep";
		case OPCODE_IF:              return "if";
		case OPCODE_IFC:             return "ifc";
		case OPCODE_ELSE:            return "else";
		case OPCODE_ENDIF:           return "endif";
		case OPCODE_BREAK:           return "break";
		case OPCODE_BREAKC:          return "breakc";
		case OPCODE_MOVA:            return "mova";
		case OPCODE_DEFB:            return "defb";
		case OPCODE_DEFI:            return "defi";
		case OPCODE_TEXCOORD:        return "texcoord";
		case OPCODE_TEXKILL:         return "texkill";
		case OPCODE_DISCARD:         return "discard";
		case OPCODE_TEX:
			if(version < 0x0104)     return "tex";
			else                     return "texld";
		case OPCODE_TEXBEM:          return "texbem";
		case OPCODE_TEXBEML:         return "texbeml";
		case OPCODE_TEXREG2AR:       return "texreg2ar";
		case OPCODE_TEXREG2GB:       return "texreg2gb";
		case OPCODE_TEXM3X2PAD:      return "texm3x2pad";
		case OPCODE_TEXM3X2TEX:      return "texm3x2tex";
		case OPCODE_TEXM3X3PAD:      return "texm3x3pad";
		case OPCODE_TEXM3X3TEX:      return "texm3x3tex";
		case OPCODE_RESERVED0:       return "reserved0";
		case OPCODE_TEXM3X3SPEC:     return "texm3x3spec";
		case OPCODE_TEXM3X3VSPEC:    return "texm3x3vspec";
		case OPCODE_EXPP:            return "expp";
		case OPCODE_LOGP:            return "logp";
		case OPCODE_CND:             return "cnd";
		case OPCODE_DEF:             return "def";
		case OPCODE_TEXREG2RGB:      return "texreg2rgb";
		case OPCODE_TEXDP3TEX:       return "texdp3tex";
		case OPCODE_TEXM3X2DEPTH:    return "texm3x2depth";
		case OPCODE_TEXDP3:          return "texdp3";
		case OPCODE_TEXM3X3:         return "texm3x3";
		case OPCODE_TEXDEPTH:        return "texdepth";
		case OPCODE_CMP0:            return "cmp0";
		case OPCODE_ICMP:            return "icmp";
		case OPCODE_UCMP:            return "ucmp";
		case OPCODE_SELECT:          return "select";
		case OPCODE_EXTRACT:         return "extract";
		case OPCODE_INSERT:          return "insert";
		case OPCODE_BEM:             return "bem";
		case OPCODE_DP2ADD:          return "dp2add";
		case OPCODE_DFDX:            return "dFdx";
		case OPCODE_DFDY:            return "dFdy";
		case OPCODE_FWIDTH:          return "fwidth";
		case OPCODE_TEXLDD:          return "texldd";
		case OPCODE_CMP:             return "cmp";
		case OPCODE_TEXLDL:          return "texldl";
		case OPCODE_TEXBIAS:         return "texbias";
		case OPCODE_TEXOFFSET:       return "texoffset";
		case OPCODE_TEXOFFSETBIAS:   return "texoffsetbias";
		case OPCODE_TEXLODOFFSET:    return "texlodoffset";
		case OPCODE_TEXELFETCH:      return "texelfetch";
		case OPCODE_TEXELFETCHOFFSET: return "texelfetchoffset";
		case OPCODE_TEXGRAD:         return "texgrad";
		case OPCODE_TEXGRADOFFSET:   return "texgradoffset";
		case OPCODE_BREAKP:          return "breakp";
		case OPCODE_TEXSIZE:         return "texsize";
		case OPCODE_PHASE:           return "phase";
		case OPCODE_COMMENT:         return "comment";
		case OPCODE_END:             return "end";
		case OPCODE_PS_1_0:          return "ps_1_0";
		case OPCODE_PS_1_1:          return "ps_1_1";
		case OPCODE_PS_1_2:          return "ps_1_2";
		case OPCODE_PS_1_3:          return "ps_1_3";
		case OPCODE_PS_1_4:          return "ps_1_4";
		case OPCODE_PS_2_0:          return "ps_2_0";
		case OPCODE_PS_2_x:          return "ps_2_x";
		case OPCODE_PS_3_0:          return "ps_3_0";
		case OPCODE_VS_1_0:          return "vs_1_0";
		case OPCODE_VS_1_1:          return "vs_1_1";
		case OPCODE_VS_2_0:          return "vs_2_0";
		case OPCODE_VS_2_x:          return "vs_2_x";
		case OPCODE_VS_2_sw:         return "vs_2_sw";
		case OPCODE_VS_3_0:          return "vs_3_0";
		case OPCODE_VS_3_sw:         return "vs_3_sw";
		case OPCODE_WHILE:           return "while";
		case OPCODE_ENDWHILE:        return "endwhile";
		case OPCODE_COS:             return "cos";
		case OPCODE_SIN:             return "sin";
		case OPCODE_TAN:             return "tan";
		case OPCODE_ACOS:            return "acos";
		case OPCODE_ASIN:            return "asin";
		case OPCODE_ATAN:            return "atan";
		case OPCODE_ATAN2:           return "atan2";
		case OPCODE_COSH:            return "cosh";
		case OPCODE_SINH:            return "sinh";
		case OPCODE_TANH:            return "tanh";
		case OPCODE_ACOSH:           return "acosh";
		case OPCODE_ASINH:           return "asinh";
		case OPCODE_ATANH:           return "atanh";
		case OPCODE_DP1:             return "dp1";
		case OPCODE_DP2:             return "dp2";
		case OPCODE_TRUNC:           return "trunc";
		case OPCODE_FLOOR:           return "floor";
		case OPCODE_ROUND:           return "round";
		case OPCODE_ROUNDEVEN:       return "roundEven";
		case OPCODE_CEIL:            return "ceil";
		case OPCODE_EXP2:            return "exp2";
		case OPCODE_LOG2:            return "log2";
		case OPCODE_EXP:             return "exp";
		case OPCODE_LOG:             return "log";
		case OPCODE_POW:             return "pow";
		case OPCODE_F2B:             return "f2b";
		case OPCODE_B2F:             return "b2f";
		case OPCODE_F2I:             return "f2i";
		case OPCODE_I2F:             return "i2f";
		case OPCODE_F2U:             return "f2u";
		case OPCODE_U2F:             return "u2f";
		case OPCODE_B2I:             return "b2i";
		case OPCODE_I2B:             return "i2b";
		case OPCODE_ALL:             return "all";
		case OPCODE_ANY:             return "any";
		case OPCODE_NEG:             return "neg";
		case OPCODE_INEG:            return "ineg";
		case OPCODE_ISNAN:           return "isnan";
		case OPCODE_ISINF:           return "isinf";
		case OPCODE_NOT:             return "not";
		case OPCODE_OR:              return "or";
		case OPCODE_XOR:             return "xor";
		case OPCODE_AND:             return "and";
		case OPCODE_EQ:              return "eq";
		case OPCODE_NE:              return "neq";
		case OPCODE_FORWARD1:        return "forward1";
		case OPCODE_FORWARD2:        return "forward2";
		case OPCODE_FORWARD3:        return "forward3";
		case OPCODE_FORWARD4:        return "forward4";
		case OPCODE_REFLECT1:        return "reflect1";
		case OPCODE_REFLECT2:        return "reflect2";
		case OPCODE_REFLECT3:        return "reflect3";
		case OPCODE_REFLECT4:        return "reflect4";
		case OPCODE_REFRACT1:        return "refract1";
		case OPCODE_REFRACT2:        return "refract2";
		case OPCODE_REFRACT3:        return "refract3";
		case OPCODE_REFRACT4:        return "refract4";
		case OPCODE_LEAVE:           return "leave";
		case OPCODE_CONTINUE:        return "continue";
		case OPCODE_TEST:            return "test";
		case OPCODE_SWITCH:          return "switch";
		case OPCODE_ENDSWITCH:       return "endswitch";
		default:
			ASSERT(false);
		}

		return "<unknown>";
	}

	std::string Shader::Instruction::controlString() const
	{
		if(opcode != OPCODE_LOOP && opcode != OPCODE_BREAKC && opcode != OPCODE_IFC && opcode != OPCODE_CMP)
		{
			if(project) return "p";

			if(bias) return "b";

			// FIXME: LOD
		}

		switch(control)
		{
		case 1: return "_gt";
		case 2: return "_eq";
		case 3: return "_ge";
		case 4: return "_lt";
		case 5: return "_ne";
		case 6: return "_le";
		default:
			return "";
		//	ASSERT(false);   // FIXME
		}
	}

	std::string Shader::Parameter::string(ShaderType shaderType, unsigned short version) const
	{
		std::ostringstream buffer;

		if(type == PARAMETER_FLOAT4LITERAL)
		{
			buffer << '{' << value[0] << ", " << value[1] << ", " << value[2] << ", " << value[3] << '}';

			return buffer.str();
		}
		else if(type != PARAMETER_RASTOUT && !(type == PARAMETER_ADDR && shaderType == SHADER_VERTEX) && type != PARAMETER_LOOP && type != PARAMETER_PREDICATE && type != PARAMETER_MISCTYPE)
		{
			buffer << index;

			return typeString(shaderType, version) + buffer.str();
		}
		else
		{
			return typeString(shaderType, version);
		}
	}

	std::string Shader::Parameter::typeString(ShaderType shaderType, unsigned short version) const
	{
		switch(type)
		{
		case PARAMETER_TEMP:			return "r";
		case PARAMETER_INPUT:			return "v";
		case PARAMETER_CONST:			return "c";
		case PARAMETER_TEXTURE:
	//	case PARAMETER_ADDR:
			if(shaderType == SHADER_PIXEL)	return "t";
			else							return "a0";
		case PARAMETER_RASTOUT:
			if(index == 0)              return "oPos";
			else if(index == 1)         return "oFog";
			else if(index == 2)         return "oPts";
			else                        ASSERT(false);
		case PARAMETER_ATTROUT:			return "oD";
		case PARAMETER_TEXCRDOUT:
	//	case PARAMETER_OUTPUT:			return "";
			if(version < 0x0300)		return "oT";
			else						return "o";
		case PARAMETER_CONSTINT:		return "i";
		case PARAMETER_COLOROUT:		return "oC";
		case PARAMETER_DEPTHOUT:		return "oDepth";
		case PARAMETER_SAMPLER:			return "s";
	//	case PARAMETER_CONST2:			return "";
	//	case PARAMETER_CONST3:			return "";
	//	case PARAMETER_CONST4:			return "";
		case PARAMETER_CONSTBOOL:		return "b";
		case PARAMETER_LOOP:			return "aL";
	//	case PARAMETER_TEMPFLOAT16:		return "";
		case PARAMETER_MISCTYPE:
			switch(index)
			{
			case VPosIndex:				return "vPos";
			case VFaceIndex:			return "vFace";
			case InstanceIDIndex:		return "iID";
			case VertexIDIndex:			return "vID";
			default: ASSERT(false);
			}
		case PARAMETER_LABEL:			return "l";
		case PARAMETER_PREDICATE:		return "p0";
		case PARAMETER_FLOAT4LITERAL:	return "";
		case PARAMETER_BOOL1LITERAL:	return "";
		case PARAMETER_INT4LITERAL:		return "";
	//	case PARAMETER_VOID:			return "";
		default:
			ASSERT(false);
		}

		return "";
	}

	bool Shader::Instruction::isBranch() const
	{
		return opcode == OPCODE_IF || opcode == OPCODE_IFC;
	}

	bool Shader::Instruction::isCall() const
	{
		return opcode == OPCODE_CALL || opcode == OPCODE_CALLNZ;
	}

	bool Shader::Instruction::isBreak() const
	{
		return opcode == OPCODE_BREAK || opcode == OPCODE_BREAKC || opcode == OPCODE_BREAKP;
	}

	bool Shader::Instruction::isLoop() const
	{
		return opcode == OPCODE_LOOP || opcode == OPCODE_REP || opcode == OPCODE_WHILE;
	}

	bool Shader::Instruction::isEndLoop() const
	{
		return opcode == OPCODE_ENDLOOP || opcode == OPCODE_ENDREP || opcode == OPCODE_ENDWHILE;
	}

	bool Shader::Instruction::isPredicated() const
	{
		return predicate ||
		       analysisBranch ||
		       analysisBreak ||
		       analysisContinue ||
		       analysisLeave;
	}

	Shader::Shader() : serialID(serialCounter++)
	{
		usedSamplers = 0;
	}

	Shader::~Shader()
	{
		for(auto &inst : instruction)
		{
			delete inst;
			inst = 0;
		}
	}

	void Shader::parse(const unsigned long *token)
	{
		minorVersion = (unsigned char)(token[0] & 0x000000FF);
		majorVersion = (unsigned char)((token[0] & 0x0000FF00) >> 8);
		shaderType = (ShaderType)((token[0] & 0xFFFF0000) >> 16);

		int length = 0;

		if(shaderType == SHADER_VERTEX)
		{
			length = VertexShader::validate(token);
		}
		else if(shaderType == SHADER_PIXEL)
		{
			length = PixelShader::validate(token);
		}
		else ASSERT(false);

		ASSERT(length != 0);
		instruction.resize(length);

		for(int i = 0; i < length; i++)
		{
			while((*token & 0x0000FFFF) == 0x0000FFFE)   // Comment token
			{
				int length = (*token & 0x7FFF0000) >> 16;

				token += length + 1;
			}

			int tokenCount = size(*token);

			instruction[i] = new Instruction(token, tokenCount, majorVersion);

			token += 1 + tokenCount;
		}
	}

	int Shader::size(unsigned long opcode) const
	{
		return size(opcode, shaderModel);
	}

	int Shader::size(unsigned long opcode, unsigned short shaderModel)
	{
		if(shaderModel > 0x0300)
		{
			ASSERT(false);
		}

		static const signed char size[] =
		{
			0,   // NOP = 0
			2,   // MOV
			3,   // ADD
			3,   // SUB
			4,   // MAD
			3,   // MUL
			2,   // RCP
			2,   // RSQ
			3,   // DP3
			3,   // DP4
			3,   // MIN
			3,   // MAX
			3,   // SLT
			3,   // SGE
			2,   // EXP
			2,   // LOG
			2,   // LIT
			3,   // DST
			4,   // LRP
			2,   // FRC
			3,   // M4x4
			3,   // M4x3
			3,   // M3x4
			3,   // M3x3
			3,   // M3x2
			1,   // CALL
			2,   // CALLNZ
			2,   // LOOP
			0,   // RET
			0,   // ENDLOOP
			1,   // LABEL
			2,   // DCL
			3,   // POW
			3,   // CRS
			4,   // SGN
			2,   // ABS
			2,   // NRM
			4,   // SINCOS
			1,   // REP
			0,   // ENDREP
			1,   // IF
			2,   // IFC
			0,   // ELSE
			0,   // ENDIF
			0,   // BREAK
			2,   // BREAKC
			2,   // MOVA
			2,   // DEFB
			5,   // DEFI
			-1,  // 49
			-1,  // 50
			-1,  // 51
			-1,  // 52
			-1,  // 53
			-1,  // 54
			-1,  // 55
			-1,  // 56
			-1,  // 57
			-1,  // 58
			-1,  // 59
			-1,  // 60
			-1,  // 61
			-1,  // 62
			-1,  // 63
			1,   // TEXCOORD = 64
			1,   // TEXKILL
			1,   // TEX
			2,   // TEXBEM
			2,   // TEXBEML
			2,   // TEXREG2AR
			2,   // TEXREG2GB
			2,   // TEXM3x2PAD
			2,   // TEXM3x2TEX
			2,   // TEXM3x3PAD
			2,   // TEXM3x3TEX
			-1,  // RESERVED0
			3,   // TEXM3x3SPEC
			2,   // TEXM3x3VSPEC
			2,   // EXPP
			2,   // LOGP
			4,   // CND
			5,   // DEF
			2,   // TEXREG2RGB
			2,   // TEXDP3TEX
			2,   // TEXM3x2DEPTH
			2,   // TEXDP3
			2,   // TEXM3x3
			1,   // TEXDEPTH
			4,   // CMP
			3,   // BEM
			4,   // DP2ADD
			2,   // DSX
			2,   // DSY
			5,   // TEXLDD
			3,   // SETP
			3,   // TEXLDL
			2,   // BREAKP
			-1,  // 97
			-1,  // 98
			-1,  // 99
			-1,  // 100
			-1,  // 101
			-1,  // 102
			-1,  // 103
			-1,  // 104
			-1,  // 105
			-1,  // 106
			-1,  // 107
			-1,  // 108
			-1,  // 109
			-1,  // 110
			-1,  // 111
			-1,  // 112
		};

		int length = 0;

		if((opcode & 0x0000FFFF) == OPCODE_COMMENT)
		{
			return (opcode & 0x7FFF0000) >> 16;
		}

		if(opcode != OPCODE_PS_1_0 &&
		   opcode != OPCODE_PS_1_1 &&
		   opcode != OPCODE_PS_1_2 &&
		   opcode != OPCODE_PS_1_3 &&
		   opcode != OPCODE_PS_1_4 &&
		   opcode != OPCODE_PS_2_0 &&
		   opcode != OPCODE_PS_2_x &&
		   opcode != OPCODE_PS_3_0 &&
		   opcode != OPCODE_VS_1_0 &&
		   opcode != OPCODE_VS_1_1 &&
		   opcode != OPCODE_VS_2_0 &&
		   opcode != OPCODE_VS_2_x &&
		   opcode != OPCODE_VS_2_sw &&
		   opcode != OPCODE_VS_3_0 &&
		   opcode != OPCODE_VS_3_sw &&
		   opcode != OPCODE_PHASE &&
		   opcode != OPCODE_END)
		{
			if(shaderModel >= 0x0200)
			{
				length = (opcode & 0x0F000000) >> 24;
			}
			else
			{
				length = size[opcode & 0x0000FFFF];
			}
		}

		if(length < 0)
		{
			ASSERT(false);
		}

		if(shaderModel == 0x0104)
		{
			switch(opcode & 0x0000FFFF)
			{
			case OPCODE_TEX:
				length += 1;
				break;
			case OPCODE_TEXCOORD:
				length += 1;
				break;
			default:
				break;
			}
		}

		return length;
	}

	bool Shader::maskContainsComponent(int mask, int component)
	{
		return (mask & (1 << component)) != 0;
	}

	bool Shader::swizzleContainsComponent(int swizzle, int component)
	{
		if((swizzle & 0x03) >> 0 == component) return true;
		if((swizzle & 0x0C) >> 2 == component) return true;
		if((swizzle & 0x30) >> 4 == component) return true;
		if((swizzle & 0xC0) >> 6 == component) return true;

		return false;
	}

	bool Shader::swizzleContainsComponentMasked(int swizzle, int component, int mask)
	{
		if(mask & 0x1) if((swizzle & 0x03) >> 0 == component) return true;
		if(mask & 0x2) if((swizzle & 0x0C) >> 2 == component) return true;
		if(mask & 0x4) if((swizzle & 0x30) >> 4 == component) return true;
		if(mask & 0x8) if((swizzle & 0xC0) >> 6 == component) return true;

		return false;
	}

	bool Shader::containsDynamicBranching() const
	{
		return dynamicBranching;
	}

	bool Shader::containsBreakInstruction() const
	{
		return containsBreak;
	}

	bool Shader::containsContinueInstruction() const
	{
		return containsContinue;
	}

	bool Shader::containsLeaveInstruction() const
	{
		return containsLeave;
	}

	bool Shader::containsDefineInstruction() const
	{
		return containsDefine;
	}

	bool Shader::usesSampler(int index) const
	{
		return (usedSamplers & (1 << index)) != 0;
	}

	int Shader::getSerialID() const
	{
		return serialID;
	}

	size_t Shader::getLength() const
	{
		return instruction.size();
	}

	Shader::ShaderType Shader::getShaderType() const
	{
		return shaderType;
	}

	unsigned short Shader::getShaderModel() const
	{
		return shaderModel;
	}

	void Shader::print(const char *fileName, ...) const
	{
		char fullName[1024 + 1];

		va_list vararg;
		va_start(vararg, fileName);
		vsnprintf(fullName, 1024, fileName, vararg);
		va_end(vararg);

		std::ofstream file(fullName, std::ofstream::out);

		for(const auto &inst : instruction)
		{
			file << inst->string(shaderType, shaderModel) << std::endl;
		}
	}

	void Shader::printInstruction(int index, const char *fileName) const
	{
		std::ofstream file(fileName, std::ofstream::out | std::ofstream::app);

		file << instruction[index]->string(shaderType, shaderModel) << std::endl;
	}

	void Shader::append(Instruction *instruction)
	{
		this->instruction.push_back(instruction);
	}

	void Shader::declareSampler(int i)
	{
		if(i >= 0 && i < 16)
		{
			usedSamplers |= 1 << i;
		}
	}

	const Shader::Instruction *Shader::getInstruction(size_t i) const
	{
		ASSERT(i < instruction.size());

		return instruction[i];
	}

	void Shader::optimize()
	{
		optimizeLeave();
		optimizeCall();
		removeNull();
	}

	void Shader::optimizeLeave()
	{
		// A return (leave) right before the end of a function or the shader can be removed
		for(unsigned int i = 0; i < instruction.size(); i++)
		{
			if(instruction[i]->opcode == OPCODE_LEAVE)
			{
				if(i == instruction.size() - 1 || instruction[i + 1]->opcode == OPCODE_RET)
				{
					instruction[i]->opcode = OPCODE_NULL;
				}
			}
		}
	}

	void Shader::optimizeCall()
	{
		// Eliminate uncalled functions
		std::set<int> calledFunctions;
		bool rescan = true;

		while(rescan)
		{
			calledFunctions.clear();
			rescan = false;

			for(const auto &inst : instruction)
			{
				if(inst->isCall())
				{
					calledFunctions.insert(inst->dst.label);
				}
			}

			if(!calledFunctions.empty())
			{
				for(unsigned int i = 0; i < instruction.size(); i++)
				{
					if(instruction[i]->opcode == OPCODE_LABEL)
					{
						if(calledFunctions.find(instruction[i]->dst.label) == calledFunctions.end())
						{
							for( ; i < instruction.size(); i++)
							{
								Opcode oldOpcode = instruction[i]->opcode;
								instruction[i]->opcode = OPCODE_NULL;

								if(oldOpcode == OPCODE_RET)
								{
									rescan = true;
									break;
								}
							}
						}
					}
				}
			}
		}

		// Optimize the entry call
		if(instruction.size() >= 2 && instruction[0]->opcode == OPCODE_CALL && instruction[1]->opcode == OPCODE_RET)
		{
			if(calledFunctions.size() == 1)
			{
				instruction[0]->opcode = OPCODE_NULL;
				instruction[1]->opcode = OPCODE_NULL;

				for(size_t i = 2; i < instruction.size(); i++)
				{
					if(instruction[i]->opcode == OPCODE_LABEL || instruction[i]->opcode == OPCODE_RET)
					{
						instruction[i]->opcode = OPCODE_NULL;
					}
				}
			}
		}
	}

	void Shader::removeNull()
	{
		size_t size = 0;
		for(size_t i = 0; i < instruction.size(); i++)
		{
			if(instruction[i]->opcode != OPCODE_NULL)
			{
				instruction[size] = instruction[i];
				size++;
			}
			else
			{
				delete instruction[i];
			}
		}

		instruction.resize(size);
	}

	void Shader::analyzeDirtyConstants()
	{
		dirtyConstantsF = 0;
		dirtyConstantsI = 0;
		dirtyConstantsB = 0;

		for(const auto &inst : instruction)
		{
			switch(inst->opcode)
			{
			case OPCODE_DEF:
				if(inst->dst.index + 1 > dirtyConstantsF)
				{
					dirtyConstantsF = inst->dst.index + 1;
				}
				break;
			case OPCODE_DEFI:
				if(inst->dst.index + 1 > dirtyConstantsI)
				{
					dirtyConstantsI = inst->dst.index + 1;
				}
				break;
			case OPCODE_DEFB:
				if(inst->dst.index + 1 > dirtyConstantsB)
				{
					dirtyConstantsB = inst->dst.index + 1;
				}
				break;
			default:
				break;
			}
		}
	}

	void Shader::analyzeDynamicBranching()
	{
		dynamicBranching = false;
		containsLeave = false;
		containsBreak = false;
		containsContinue = false;
		containsDefine = false;

		// Determine global presence of branching instructions
		for(const auto &inst : instruction)
		{
			switch(inst->opcode)
			{
			case OPCODE_CALLNZ:
			case OPCODE_IF:
			case OPCODE_IFC:
			case OPCODE_BREAK:
			case OPCODE_BREAKC:
			case OPCODE_CMP:
			case OPCODE_BREAKP:
			case OPCODE_LEAVE:
			case OPCODE_CONTINUE:
				if(inst->src[0].type != PARAMETER_CONSTBOOL)
				{
					dynamicBranching = true;
				}

				if(inst->opcode == OPCODE_LEAVE)
				{
					containsLeave = true;
				}

				if(inst->isBreak())
				{
					containsBreak = true;
				}

				if(inst->opcode == OPCODE_CONTINUE)
				{
					containsContinue = true;
				}
			case OPCODE_DEF:
			case OPCODE_DEFB:
			case OPCODE_DEFI:
				containsDefine = true;
			default:
				break;
			}
		}

		// Conservatively determine which instructions are affected by dynamic branching
		int branchDepth = 0;
		int breakDepth = 0;
		int continueDepth = 0;
		bool leaveReturn = false;
		unsigned int functionBegin = 0;

		for(unsigned int i = 0; i < instruction.size(); i++)
		{
			// If statements and loops
			if(instruction[i]->isBranch() || instruction[i]->isLoop())
			{
				branchDepth++;
			}
			else if(instruction[i]->opcode == OPCODE_ENDIF || instruction[i]->isEndLoop())
			{
				branchDepth--;
			}

			if(branchDepth > 0)
			{
				instruction[i]->analysisBranch = true;

				if(instruction[i]->isCall())
				{
					markFunctionAnalysis(instruction[i]->dst.label, ANALYSIS_BRANCH);
				}
			}

			// Break statemement
			if(instruction[i]->isBreak())
			{
				breakDepth++;
			}

			if(breakDepth > 0)
			{
				if(instruction[i]->isLoop() || instruction[i]->opcode == OPCODE_SWITCH)   // Nested loop or switch, don't make the end of it disable the break execution mask
				{
					breakDepth++;
				}
				else if(instruction[i]->isEndLoop() || instruction[i]->opcode == OPCODE_ENDSWITCH)
				{
					breakDepth--;
				}

				instruction[i]->analysisBreak = true;

				if(instruction[i]->isCall())
				{
					markFunctionAnalysis(instruction[i]->dst.label, ANALYSIS_BRANCH);
				}
			}

			// Continue statement
			if(instruction[i]->opcode == OPCODE_CONTINUE)
			{
				continueDepth++;
			}

			if(continueDepth > 0)
			{
				if(instruction[i]->isLoop() || instruction[i]->opcode == OPCODE_SWITCH)   // Nested loop or switch, don't make the end of it disable the break execution mask
				{
					continueDepth++;
				}
				else if(instruction[i]->isEndLoop() || instruction[i]->opcode == OPCODE_ENDSWITCH)
				{
					continueDepth--;
				}

				instruction[i]->analysisContinue = true;

				if(instruction[i]->isCall())
				{
					markFunctionAnalysis(instruction[i]->dst.label, ANALYSIS_CONTINUE);
				}
			}

			// Return (leave) statement
			if(instruction[i]->opcode == OPCODE_LEAVE)
			{
				leaveReturn = true;

				// Mark loop body instructions prior to the return statement
				for(unsigned int l = functionBegin; l < i; l++)
				{
					if(instruction[l]->isLoop())
					{
						for(unsigned int r = l + 1; r < i; r++)
						{
							instruction[r]->analysisLeave = true;
						}

						break;
					}
				}
			}
			else if(instruction[i]->opcode == OPCODE_RET)   // End of the function
			{
				leaveReturn = false;
			}
			else if(instruction[i]->opcode == OPCODE_LABEL)
			{
				functionBegin = i;
			}

			if(leaveReturn)
			{
				instruction[i]->analysisLeave = true;

				if(instruction[i]->isCall())
				{
					markFunctionAnalysis(instruction[i]->dst.label, ANALYSIS_LEAVE);
				}
			}
		}
	}

	void Shader::markFunctionAnalysis(unsigned int functionLabel, Analysis flag)
	{
		bool marker = false;
		for(auto &inst : instruction)
		{
			if(!marker)
			{
				if(inst->opcode == OPCODE_LABEL && inst->dst.label == functionLabel)
				{
					marker = true;
				}
			}
			else
			{
				if(inst->opcode == OPCODE_RET)
				{
					break;
				}
				else if(inst->isCall())
				{
					markFunctionAnalysis(inst->dst.label, flag);
				}

				inst->analysis |= flag;
			}
		}
	}

	void Shader::analyzeSamplers()
	{
		for(const auto &inst : instruction)
		{
			switch(inst->opcode)
			{
			case OPCODE_TEX:
			case OPCODE_TEXBEM:
			case OPCODE_TEXBEML:
			case OPCODE_TEXREG2AR:
			case OPCODE_TEXREG2GB:
			case OPCODE_TEXM3X2TEX:
			case OPCODE_TEXM3X3TEX:
			case OPCODE_TEXM3X3SPEC:
			case OPCODE_TEXM3X3VSPEC:
			case OPCODE_TEXREG2RGB:
			case OPCODE_TEXDP3TEX:
			case OPCODE_TEXM3X2DEPTH:
			case OPCODE_TEXLDD:
			case OPCODE_TEXLDL:
			case OPCODE_TEXLOD:
			case OPCODE_TEXOFFSET:
			case OPCODE_TEXOFFSETBIAS:
			case OPCODE_TEXLODOFFSET:
			case OPCODE_TEXELFETCH:
			case OPCODE_TEXELFETCHOFFSET:
			case OPCODE_TEXGRAD:
			case OPCODE_TEXGRADOFFSET:
				{
					Parameter &dst = inst->dst;
					Parameter &src1 = inst->src[1];

					if(majorVersion >= 2)
					{
						if(src1.type == PARAMETER_SAMPLER)
						{
							usedSamplers |= 1 << src1.index;
						}
					}
					else
					{
						usedSamplers |= 1 << dst.index;
					}
				}
				break;
			default:
				break;
			}
		}
	}

	// Assigns a unique index to each call instruction, on a per label basis.
	// This is used to know what basic block to return to.
	void Shader::analyzeCallSites()
	{
		int callSiteIndex[2048] = {0};

		for(auto &inst : instruction)
		{
			if(inst->opcode == OPCODE_CALL || inst->opcode == OPCODE_CALLNZ)
			{
				int label = inst->dst.label;

				inst->dst.callSite = callSiteIndex[label]++;
			}
		}
	}

	void Shader::analyzeDynamicIndexing()
	{
		dynamicallyIndexedTemporaries = false;
		dynamicallyIndexedInput = false;
		dynamicallyIndexedOutput = false;

		for(const auto &inst : instruction)
		{
			if(inst->dst.rel.type == PARAMETER_ADDR ||
			   inst->dst.rel.type == PARAMETER_LOOP ||
			   inst->dst.rel.type == PARAMETER_TEMP ||
			   inst->dst.rel.type == PARAMETER_CONST)
			{
				switch(inst->dst.type)
				{
				case PARAMETER_TEMP:   dynamicallyIndexedTemporaries = true; break;
				case PARAMETER_INPUT:  dynamicallyIndexedInput = true;       break;
				case PARAMETER_OUTPUT: dynamicallyIndexedOutput = true;      break;
				default: break;
				}
			}

			for(int j = 0; j < 3; j++)
			{
				if(inst->src[j].rel.type == PARAMETER_ADDR ||
				   inst->src[j].rel.type == PARAMETER_LOOP ||
				   inst->src[j].rel.type == PARAMETER_TEMP ||
				   inst->src[j].rel.type == PARAMETER_CONST)
				{
					switch(inst->src[j].type)
					{
					case PARAMETER_TEMP:   dynamicallyIndexedTemporaries = true; break;
					case PARAMETER_INPUT:  dynamicallyIndexedInput = true;       break;
					case PARAMETER_OUTPUT: dynamicallyIndexedOutput = true;      break;
					default: break;
					}
				}
			}
		}
	}
}
