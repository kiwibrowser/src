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

#include "PixelShader.hpp"

#include "Common/Debug.hpp"

#include <string.h>

namespace sw
{
	PixelShader::PixelShader(const PixelShader *ps) : Shader()
	{
		shaderModel = 0x0300;
		vPosDeclared = false;
		vFaceDeclared = false;
		centroid = false;

		if(ps)   // Make a copy
		{
			for(size_t i = 0; i < ps->getLength(); i++)
			{
				append(new sw::Shader::Instruction(*ps->getInstruction(i)));
			}

			memcpy(input, ps->input, sizeof(input));
			vPosDeclared = ps->vPosDeclared;
			vFaceDeclared = ps->vFaceDeclared;
			usedSamplers = ps->usedSamplers;

			optimize();
			analyze();
		}
	}

	PixelShader::PixelShader(const unsigned long *token) : Shader()
	{
		parse(token);

		vPosDeclared = false;
		vFaceDeclared = false;
		centroid = false;

		optimize();
		analyze();
	}

	PixelShader::~PixelShader()
	{
	}

	int PixelShader::validate(const unsigned long *const token)
	{
		if(!token)
		{
			return 0;
		}

		unsigned short version = (unsigned short)(token[0] & 0x0000FFFF);
		// unsigned char minorVersion = (unsigned char)(token[0] & 0x000000FF);
		unsigned char majorVersion = (unsigned char)((token[0] & 0x0000FF00) >> 8);
		ShaderType shaderType = (ShaderType)((token[0] & 0xFFFF0000) >> 16);

		if(shaderType != SHADER_PIXEL || majorVersion > 3)
		{
			return 0;
		}

		int instructionCount = 1;

		for(int i = 0; token[i] != 0x0000FFFF; i++)
		{
			if((token[i] & 0x0000FFFF) == 0x0000FFFE)   // Comment token
			{
				int length = (token[i] & 0x7FFF0000) >> 16;

				i += length;
			}
			else
			{
				Shader::Opcode opcode = (Shader::Opcode)(token[i] & 0x0000FFFF);

				switch(opcode)
				{
				case Shader::OPCODE_RESERVED0:
				case Shader::OPCODE_MOVA:
					return 0;   // Unsupported operation
				default:
					instructionCount++;
					break;
				}

				i += size(token[i], version);
			}
		}

		return instructionCount;
	}

	bool PixelShader::depthOverride() const
	{
		return zOverride;
	}

	bool PixelShader::containsKill() const
	{
		return kill;
	}

	bool PixelShader::containsCentroid() const
	{
		return centroid;
	}

	bool PixelShader::usesDiffuse(int component) const
	{
		return input[0][component].active();
	}

	bool PixelShader::usesSpecular(int component) const
	{
		return input[1][component].active();
	}

	bool PixelShader::usesTexture(int coordinate, int component) const
	{
		return input[2 + coordinate][component].active();
	}

	void PixelShader::setInput(int inputIdx, int nbComponents, const sw::Shader::Semantic& semantic)
	{
		for(int i = 0; i < nbComponents; ++i)
		{
			input[inputIdx][i] = semantic;
		}
	}

	const sw::Shader::Semantic& PixelShader::getInput(int inputIdx, int component) const
	{
		return input[inputIdx][component];
	}

	void PixelShader::analyze()
	{
		analyzeZOverride();
		analyzeKill();
		analyzeInterpolants();
		analyzeDirtyConstants();
		analyzeDynamicBranching();
		analyzeSamplers();
		analyzeCallSites();
		analyzeDynamicIndexing();
	}

	void PixelShader::analyzeZOverride()
	{
		zOverride = false;

		for(const auto &inst : instruction)
		{
			if(inst->opcode == Shader::OPCODE_TEXM3X2DEPTH ||
			   inst->opcode == Shader::OPCODE_TEXDEPTH ||
			   inst->dst.type == Shader::PARAMETER_DEPTHOUT)
			{
				zOverride = true;

				break;
			}
		}
	}

	void PixelShader::analyzeKill()
	{
		kill = false;

		for(const auto &inst : instruction)
		{
			if(inst->opcode == Shader::OPCODE_TEXKILL ||
			   inst->opcode == Shader::OPCODE_DISCARD)
			{
				kill = true;

				break;
			}
		}
	}

	void PixelShader::analyzeInterpolants()
	{
		if(shaderModel < 0x0300)
		{
			// Set default mapping; disable unused interpolants below
			input[0][0] = Semantic(Shader::USAGE_COLOR, 0);
			input[0][1] = Semantic(Shader::USAGE_COLOR, 0);
			input[0][2] = Semantic(Shader::USAGE_COLOR, 0);
			input[0][3] = Semantic(Shader::USAGE_COLOR, 0);

			input[1][0] = Semantic(Shader::USAGE_COLOR, 1);
			input[1][1] = Semantic(Shader::USAGE_COLOR, 1);
			input[1][2] = Semantic(Shader::USAGE_COLOR, 1);
			input[1][3] = Semantic(Shader::USAGE_COLOR, 1);

			for(int i = 0; i < 8; i++)
			{
				input[2 + i][0] = Semantic(Shader::USAGE_TEXCOORD, i);
				input[2 + i][1] = Semantic(Shader::USAGE_TEXCOORD, i);
				input[2 + i][2] = Semantic(Shader::USAGE_TEXCOORD, i);
				input[2 + i][3] = Semantic(Shader::USAGE_TEXCOORD, i);
			}

			Shader::SamplerType samplerType[16];

			for(int i = 0; i < 16; i++)
			{
				samplerType[i] = Shader::SAMPLER_UNKNOWN;
			}

			for(const auto &inst : instruction)
			{
				if(inst->dst.type == Shader::PARAMETER_SAMPLER)
				{
					int sampler = inst->dst.index;

					samplerType[sampler] = inst->samplerType;
				}
			}

			bool interpolant[MAX_FRAGMENT_INPUTS][4] = {{false}};   // Interpolants in use

			for(const auto &inst : instruction)
			{
				if(inst->dst.type == Shader::PARAMETER_TEXTURE)
				{
					int index = inst->dst.index + 2;

					switch(inst->opcode)
					{
					case Shader::OPCODE_TEX:
					case Shader::OPCODE_TEXBEM:
					case Shader::OPCODE_TEXBEML:
					case Shader::OPCODE_TEXCOORD:
					case Shader::OPCODE_TEXDP3:
					case Shader::OPCODE_TEXDP3TEX:
					case Shader::OPCODE_TEXM3X2DEPTH:
					case Shader::OPCODE_TEXM3X2PAD:
					case Shader::OPCODE_TEXM3X2TEX:
					case Shader::OPCODE_TEXM3X3:
					case Shader::OPCODE_TEXM3X3PAD:
					case Shader::OPCODE_TEXM3X3TEX:
						interpolant[index][0] = true;
						interpolant[index][1] = true;
						interpolant[index][2] = true;
						break;
					case Shader::OPCODE_TEXKILL:
						if(majorVersion < 2)
						{
							interpolant[index][0] = true;
							interpolant[index][1] = true;
							interpolant[index][2] = true;
						}
						else
						{
							interpolant[index][0] = true;
							interpolant[index][1] = true;
							interpolant[index][2] = true;
							interpolant[index][3] = true;
						}
						break;
					case Shader::OPCODE_TEXM3X3VSPEC:
						interpolant[index][0] = true;
						interpolant[index][1] = true;
						interpolant[index][2] = true;
						interpolant[index - 2][3] = true;
						interpolant[index - 1][3] = true;
						interpolant[index - 0][3] = true;
						break;
					case Shader::OPCODE_DCL:
						break;   // Ignore
					default:   // Arithmetic instruction
						if(shaderModel >= 0x0104)
						{
							ASSERT(false);
						}
					}
				}

				for(int argument = 0; argument < 4; argument++)
				{
					if(inst->src[argument].type == Shader::PARAMETER_INPUT ||
					   inst->src[argument].type == Shader::PARAMETER_TEXTURE)
					{
						int index = inst->src[argument].index;
						int swizzle = inst->src[argument].swizzle;
						int mask = inst->dst.mask;

						if(inst->src[argument].type == Shader::PARAMETER_TEXTURE)
						{
							index += 2;
						}

						switch(inst->opcode)
						{
						case Shader::OPCODE_TEX:
						case Shader::OPCODE_TEXLDD:
						case Shader::OPCODE_TEXLDL:
						case Shader::OPCODE_TEXLOD:
						case Shader::OPCODE_TEXBIAS:
						case Shader::OPCODE_TEXOFFSET:
						case Shader::OPCODE_TEXOFFSETBIAS:
						case Shader::OPCODE_TEXLODOFFSET:
						case Shader::OPCODE_TEXELFETCH:
						case Shader::OPCODE_TEXELFETCHOFFSET:
						case Shader::OPCODE_TEXGRAD:
						case Shader::OPCODE_TEXGRADOFFSET:
							{
								int sampler = inst->src[1].index;

								switch(samplerType[sampler])
								{
								case Shader::SAMPLER_UNKNOWN:
									if(shaderModel == 0x0104)
									{
										if((inst->src[0].swizzle & 0x30) == 0x20)   // .xyz
										{
											interpolant[index][0] = true;
											interpolant[index][1] = true;
											interpolant[index][2] = true;
										}
										else   // .xyw
										{
											interpolant[index][0] = true;
											interpolant[index][1] = true;
											interpolant[index][3] = true;
										}
									}
									else
									{
										ASSERT(false);
									}
									break;
								case Shader::SAMPLER_1D:
									interpolant[index][0] = true;
									break;
								case Shader::SAMPLER_2D:
									interpolant[index][0] = true;
									interpolant[index][1] = true;
									break;
								case Shader::SAMPLER_CUBE:
									interpolant[index][0] = true;
									interpolant[index][1] = true;
									interpolant[index][2] = true;
									break;
								case Shader::SAMPLER_VOLUME:
									interpolant[index][0] = true;
									interpolant[index][1] = true;
									interpolant[index][2] = true;
									break;
								default:
									ASSERT(false);
								}

								if(inst->bias)
								{
									interpolant[index][3] = true;
								}

								if(inst->project)
								{
									interpolant[index][3] = true;
								}

								if(shaderModel == 0x0104 && inst->opcode == Shader::OPCODE_TEX)
								{
									if(inst->src[0].modifier == Shader::MODIFIER_DZ)
									{
										interpolant[index][2] = true;
									}

									if(inst->src[0].modifier == Shader::MODIFIER_DW)
									{
										interpolant[index][3] = true;
									}
								}
							}
							break;
						case Shader::OPCODE_M3X2:
							if(mask & 0x1)
							{
								interpolant[index][0] |= swizzleContainsComponentMasked(swizzle, 0, 0x7);
								interpolant[index][1] |= swizzleContainsComponentMasked(swizzle, 1, 0x7);
								interpolant[index][2] |= swizzleContainsComponentMasked(swizzle, 2, 0x7);
								interpolant[index][3] |= swizzleContainsComponentMasked(swizzle, 3, 0x7);
							}

							if(argument == 1)
							{
								if(mask & 0x2)
								{
									interpolant[index + 1][0] |= swizzleContainsComponentMasked(swizzle, 0, 0x7);
									interpolant[index + 1][1] |= swizzleContainsComponentMasked(swizzle, 1, 0x7);
									interpolant[index + 1][2] |= swizzleContainsComponentMasked(swizzle, 2, 0x7);
									interpolant[index + 1][3] |= swizzleContainsComponentMasked(swizzle, 3, 0x7);
								}
							}
							break;
						case Shader::OPCODE_M3X3:
							if(mask & 0x1)
							{
								interpolant[index][0] |= swizzleContainsComponentMasked(swizzle, 0, 0x7);
								interpolant[index][1] |= swizzleContainsComponentMasked(swizzle, 1, 0x7);
								interpolant[index][2] |= swizzleContainsComponentMasked(swizzle, 2, 0x7);
								interpolant[index][3] |= swizzleContainsComponentMasked(swizzle, 3, 0x7);
							}

							if(argument == 1)
							{
								if(mask & 0x2)
								{
									interpolant[index + 1][0] |= swizzleContainsComponentMasked(swizzle, 0, 0x7);
									interpolant[index + 1][1] |= swizzleContainsComponentMasked(swizzle, 1, 0x7);
									interpolant[index + 1][2] |= swizzleContainsComponentMasked(swizzle, 2, 0x7);
									interpolant[index + 1][3] |= swizzleContainsComponentMasked(swizzle, 3, 0x7);
								}

								if(mask & 0x4)
								{
									interpolant[index + 2][0] |= swizzleContainsComponentMasked(swizzle, 0, 0x7);
									interpolant[index + 2][1] |= swizzleContainsComponentMasked(swizzle, 1, 0x7);
									interpolant[index + 2][2] |= swizzleContainsComponentMasked(swizzle, 2, 0x7);
									interpolant[index + 2][3] |= swizzleContainsComponentMasked(swizzle, 3, 0x7);
								}
							}
							break;
						case Shader::OPCODE_M3X4:
							if(mask & 0x1)
							{
								interpolant[index][0] |= swizzleContainsComponentMasked(swizzle, 0, 0x7);
								interpolant[index][1] |= swizzleContainsComponentMasked(swizzle, 1, 0x7);
								interpolant[index][2] |= swizzleContainsComponentMasked(swizzle, 2, 0x7);
								interpolant[index][3] |= swizzleContainsComponentMasked(swizzle, 3, 0x7);
							}

							if(argument == 1)
							{
								if(mask & 0x2)
								{
									interpolant[index + 1][0] |= swizzleContainsComponentMasked(swizzle, 0, 0x7);
									interpolant[index + 1][1] |= swizzleContainsComponentMasked(swizzle, 1, 0x7);
									interpolant[index + 1][2] |= swizzleContainsComponentMasked(swizzle, 2, 0x7);
									interpolant[index + 1][3] |= swizzleContainsComponentMasked(swizzle, 3, 0x7);
								}

								if(mask & 0x4)
								{
									interpolant[index + 2][0] |= swizzleContainsComponentMasked(swizzle, 0, 0x7);
									interpolant[index + 2][1] |= swizzleContainsComponentMasked(swizzle, 1, 0x7);
									interpolant[index + 2][2] |= swizzleContainsComponentMasked(swizzle, 2, 0x7);
									interpolant[index + 2][3] |= swizzleContainsComponentMasked(swizzle, 3, 0x7);
								}

								if(mask & 0x8)
								{
									interpolant[index + 3][0] |= swizzleContainsComponentMasked(swizzle, 0, 0x7);
									interpolant[index + 3][1] |= swizzleContainsComponentMasked(swizzle, 1, 0x7);
									interpolant[index + 3][2] |= swizzleContainsComponentMasked(swizzle, 2, 0x7);
									interpolant[index + 3][3] |= swizzleContainsComponentMasked(swizzle, 3, 0x7);
								}
							}
							break;
						case Shader::OPCODE_M4X3:
							if(mask & 0x1)
							{
								interpolant[index][0] |= swizzleContainsComponent(swizzle, 0);
								interpolant[index][1] |= swizzleContainsComponent(swizzle, 1);
								interpolant[index][2] |= swizzleContainsComponent(swizzle, 2);
								interpolant[index][3] |= swizzleContainsComponent(swizzle, 3);
							}

							if(argument == 1)
							{
								if(mask & 0x2)
								{
									interpolant[index + 1][0] |= swizzleContainsComponent(swizzle, 0);
									interpolant[index + 1][1] |= swizzleContainsComponent(swizzle, 1);
									interpolant[index + 1][2] |= swizzleContainsComponent(swizzle, 2);
									interpolant[index + 1][3] |= swizzleContainsComponent(swizzle, 3);
								}

								if(mask & 0x4)
								{
									interpolant[index + 2][0] |= swizzleContainsComponent(swizzle, 0);
									interpolant[index + 2][1] |= swizzleContainsComponent(swizzle, 1);
									interpolant[index + 2][2] |= swizzleContainsComponent(swizzle, 2);
									interpolant[index + 2][3] |= swizzleContainsComponent(swizzle, 3);
								}
							}
							break;
						case Shader::OPCODE_M4X4:
							if(mask & 0x1)
							{
								interpolant[index][0] |= swizzleContainsComponent(swizzle, 0);
								interpolant[index][1] |= swizzleContainsComponent(swizzle, 1);
								interpolant[index][2] |= swizzleContainsComponent(swizzle, 2);
								interpolant[index][3] |= swizzleContainsComponent(swizzle, 3);
							}

							if(argument == 1)
							{
								if(mask & 0x2)
								{
									interpolant[index + 1][0] |= swizzleContainsComponent(swizzle, 0);
									interpolant[index + 1][1] |= swizzleContainsComponent(swizzle, 1);
									interpolant[index + 1][2] |= swizzleContainsComponent(swizzle, 2);
									interpolant[index + 1][3] |= swizzleContainsComponent(swizzle, 3);
								}

								if(mask & 0x4)
								{
									interpolant[index + 2][0] |= swizzleContainsComponent(swizzle, 0);
									interpolant[index + 2][1] |= swizzleContainsComponent(swizzle, 1);
									interpolant[index + 2][2] |= swizzleContainsComponent(swizzle, 2);
									interpolant[index + 2][3] |= swizzleContainsComponent(swizzle, 3);
								}

								if(mask & 0x8)
								{
									interpolant[index + 3][0] |= swizzleContainsComponent(swizzle, 0);
									interpolant[index + 3][1] |= swizzleContainsComponent(swizzle, 1);
									interpolant[index + 3][2] |= swizzleContainsComponent(swizzle, 2);
									interpolant[index + 3][3] |= swizzleContainsComponent(swizzle, 3);
								}
							}
							break;
						case Shader::OPCODE_CRS:
							if(mask & 0x1)
							{
								interpolant[index][0] |= swizzleContainsComponentMasked(swizzle, 0, 0x6);
								interpolant[index][1] |= swizzleContainsComponentMasked(swizzle, 1, 0x6);
								interpolant[index][2] |= swizzleContainsComponentMasked(swizzle, 2, 0x6);
								interpolant[index][3] |= swizzleContainsComponentMasked(swizzle, 3, 0x6);
							}

							if(mask & 0x2)
							{
								interpolant[index][0] |= swizzleContainsComponentMasked(swizzle, 0, 0x5);
								interpolant[index][1] |= swizzleContainsComponentMasked(swizzle, 1, 0x5);
								interpolant[index][2] |= swizzleContainsComponentMasked(swizzle, 2, 0x5);
								interpolant[index][3] |= swizzleContainsComponentMasked(swizzle, 3, 0x5);
							}

							if(mask & 0x4)
							{
								interpolant[index][0] |= swizzleContainsComponentMasked(swizzle, 0, 0x3);
								interpolant[index][1] |= swizzleContainsComponentMasked(swizzle, 1, 0x3);
								interpolant[index][2] |= swizzleContainsComponentMasked(swizzle, 2, 0x3);
								interpolant[index][3] |= swizzleContainsComponentMasked(swizzle, 3, 0x3);
							}
							break;
						case Shader::OPCODE_DP2ADD:
							if(argument == 0 || argument == 1)
							{
								interpolant[index][0] |= swizzleContainsComponentMasked(swizzle, 0, 0x3);
								interpolant[index][1] |= swizzleContainsComponentMasked(swizzle, 1, 0x3);
								interpolant[index][2] |= swizzleContainsComponentMasked(swizzle, 2, 0x3);
								interpolant[index][3] |= swizzleContainsComponentMasked(swizzle, 3, 0x3);
							}
							else   // argument == 2
							{
								interpolant[index][0] |= swizzleContainsComponent(swizzle, 0);
								interpolant[index][1] |= swizzleContainsComponent(swizzle, 1);
								interpolant[index][2] |= swizzleContainsComponent(swizzle, 2);
								interpolant[index][3] |= swizzleContainsComponent(swizzle, 3);
							}
							break;
						case Shader::OPCODE_DP3:
							interpolant[index][0] |= swizzleContainsComponentMasked(swizzle, 0, 0x7);
							interpolant[index][1] |= swizzleContainsComponentMasked(swizzle, 1, 0x7);
							interpolant[index][2] |= swizzleContainsComponentMasked(swizzle, 2, 0x7);
							interpolant[index][3] |= swizzleContainsComponentMasked(swizzle, 3, 0x7);
							break;
						case Shader::OPCODE_DP4:
							interpolant[index][0] |= swizzleContainsComponent(swizzle, 0);
							interpolant[index][1] |= swizzleContainsComponent(swizzle, 1);
							interpolant[index][2] |= swizzleContainsComponent(swizzle, 2);
							interpolant[index][3] |= swizzleContainsComponent(swizzle, 3);
							break;
						case Shader::OPCODE_SINCOS:
						case Shader::OPCODE_EXP2X:
						case Shader::OPCODE_LOG2X:
						case Shader::OPCODE_POWX:
						case Shader::OPCODE_RCPX:
						case Shader::OPCODE_RSQX:
							interpolant[index][0] |= swizzleContainsComponent(swizzle, 0);
							interpolant[index][1] |= swizzleContainsComponent(swizzle, 1);
							interpolant[index][2] |= swizzleContainsComponent(swizzle, 2);
							interpolant[index][3] |= swizzleContainsComponent(swizzle, 3);
							break;
						case Shader::OPCODE_NRM3:
							interpolant[index][0] |= swizzleContainsComponentMasked(swizzle, 0, 0x7 | mask);
							interpolant[index][1] |= swizzleContainsComponentMasked(swizzle, 1, 0x7 | mask);
							interpolant[index][2] |= swizzleContainsComponentMasked(swizzle, 2, 0x7 | mask);
							interpolant[index][3] |= swizzleContainsComponentMasked(swizzle, 3, 0x7 | mask);
							break;
						case Shader::OPCODE_MOV:
						case Shader::OPCODE_ADD:
						case Shader::OPCODE_SUB:
						case Shader::OPCODE_MUL:
						case Shader::OPCODE_MAD:
						case Shader::OPCODE_ABS:
						case Shader::OPCODE_CMP0:
						case Shader::OPCODE_CND:
						case Shader::OPCODE_FRC:
						case Shader::OPCODE_LRP:
						case Shader::OPCODE_MAX:
						case Shader::OPCODE_MIN:
						case Shader::OPCODE_CMP:
						case Shader::OPCODE_BREAKC:
						case Shader::OPCODE_DFDX:
						case Shader::OPCODE_DFDY:
							interpolant[index][0] |= swizzleContainsComponentMasked(swizzle, 0, mask);
							interpolant[index][1] |= swizzleContainsComponentMasked(swizzle, 1, mask);
							interpolant[index][2] |= swizzleContainsComponentMasked(swizzle, 2, mask);
							interpolant[index][3] |= swizzleContainsComponentMasked(swizzle, 3, mask);
							break;
						case Shader::OPCODE_TEXCOORD:
							interpolant[index][0] = true;
							interpolant[index][1] = true;
							interpolant[index][2] = true;
							interpolant[index][3] = true;
							break;
						case Shader::OPCODE_TEXDP3:
						case Shader::OPCODE_TEXDP3TEX:
						case Shader::OPCODE_TEXM3X2PAD:
						case Shader::OPCODE_TEXM3X3PAD:
						case Shader::OPCODE_TEXM3X2TEX:
						case Shader::OPCODE_TEXM3X3SPEC:
						case Shader::OPCODE_TEXM3X3VSPEC:
						case Shader::OPCODE_TEXBEM:
						case Shader::OPCODE_TEXBEML:
						case Shader::OPCODE_TEXM3X2DEPTH:
						case Shader::OPCODE_TEXM3X3:
						case Shader::OPCODE_TEXM3X3TEX:
							interpolant[index][0] = true;
							interpolant[index][1] = true;
							interpolant[index][2] = true;
							break;
						case Shader::OPCODE_TEXREG2AR:
						case Shader::OPCODE_TEXREG2GB:
						case Shader::OPCODE_TEXREG2RGB:
							break;
						default:
						//	ASSERT(false);   // Refine component usage
							interpolant[index][0] = true;
							interpolant[index][1] = true;
							interpolant[index][2] = true;
							interpolant[index][3] = true;
						}
					}
				}
			}

			for(int index = 0; index < MAX_FRAGMENT_INPUTS; index++)
			{
				for(int component = 0; component < 4; component++)
				{
					if(!interpolant[index][component])
					{
						input[index][component] = Semantic();
					}
				}
			}
		}
		else   // Shader Model 3.0 input declaration; v# indexable
		{
			for(const auto &inst : instruction)
			{
				if(inst->opcode == Shader::OPCODE_DCL)
				{
					if(inst->dst.type == Shader::PARAMETER_INPUT)
					{
						unsigned char usage = inst->usage;
						unsigned char index = inst->usageIndex;
						unsigned char mask = inst->dst.mask;
						unsigned char reg = inst->dst.index;

						if(mask & 0x01) input[reg][0] = Semantic(usage, index);
						if(mask & 0x02) input[reg][1] = Semantic(usage, index);
						if(mask & 0x04) input[reg][2] = Semantic(usage, index);
						if(mask & 0x08) input[reg][3] = Semantic(usage, index);
					}
					else if(inst->dst.type == Shader::PARAMETER_MISCTYPE)
					{
						unsigned char index = inst->dst.index;

						if(index == Shader::VPosIndex)
						{
							vPosDeclared = true;
						}
						else if(index == Shader::VFaceIndex)
						{
							vFaceDeclared = true;
						}
						else ASSERT(false);
					}
				}
			}
		}

		if(shaderModel >= 0x0200)
		{
			for(const auto &inst : instruction)
			{
				if(inst->opcode == Shader::OPCODE_DCL)
				{
					bool centroid = inst->dst.centroid;
					unsigned char reg = inst->dst.index;

					switch(inst->dst.type)
					{
					case Shader::PARAMETER_INPUT:
						input[reg][0].centroid = centroid;
						break;
					case Shader::PARAMETER_TEXTURE:
						input[2 + reg][0].centroid = centroid;
						break;
					default:
						break;
					}

					this->centroid = this->centroid || centroid;
				}
			}
		}
	}
}
