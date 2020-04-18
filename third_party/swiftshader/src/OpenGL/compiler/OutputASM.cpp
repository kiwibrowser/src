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

#include "OutputASM.h"
#include "Common/Math.hpp"

#include "common/debug.h"
#include "InfoSink.h"

#include "libGLESv2/Shader.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3.h>

#include <stdlib.h>

namespace
{
	GLenum glVariableType(const TType &type)
	{
		switch(type.getBasicType())
		{
		case EbtFloat:
			if(type.isScalar())
			{
				return GL_FLOAT;
			}
			else if(type.isVector())
			{
				switch(type.getNominalSize())
				{
				case 2: return GL_FLOAT_VEC2;
				case 3: return GL_FLOAT_VEC3;
				case 4: return GL_FLOAT_VEC4;
				default: UNREACHABLE(type.getNominalSize());
				}
			}
			else if(type.isMatrix())
			{
				switch(type.getNominalSize())
				{
				case 2:
					switch(type.getSecondarySize())
					{
					case 2: return GL_FLOAT_MAT2;
					case 3: return GL_FLOAT_MAT2x3;
					case 4: return GL_FLOAT_MAT2x4;
					default: UNREACHABLE(type.getSecondarySize());
					}
				case 3:
					switch(type.getSecondarySize())
					{
					case 2: return GL_FLOAT_MAT3x2;
					case 3: return GL_FLOAT_MAT3;
					case 4: return GL_FLOAT_MAT3x4;
					default: UNREACHABLE(type.getSecondarySize());
					}
				case 4:
					switch(type.getSecondarySize())
					{
					case 2: return GL_FLOAT_MAT4x2;
					case 3: return GL_FLOAT_MAT4x3;
					case 4: return GL_FLOAT_MAT4;
					default: UNREACHABLE(type.getSecondarySize());
					}
				default: UNREACHABLE(type.getNominalSize());
				}
			}
			else UNREACHABLE(0);
			break;
		case EbtInt:
			if(type.isScalar())
			{
				return GL_INT;
			}
			else if(type.isVector())
			{
				switch(type.getNominalSize())
				{
				case 2: return GL_INT_VEC2;
				case 3: return GL_INT_VEC3;
				case 4: return GL_INT_VEC4;
				default: UNREACHABLE(type.getNominalSize());
				}
			}
			else UNREACHABLE(0);
			break;
		case EbtUInt:
			if(type.isScalar())
			{
				return GL_UNSIGNED_INT;
			}
			else if(type.isVector())
			{
				switch(type.getNominalSize())
				{
				case 2: return GL_UNSIGNED_INT_VEC2;
				case 3: return GL_UNSIGNED_INT_VEC3;
				case 4: return GL_UNSIGNED_INT_VEC4;
				default: UNREACHABLE(type.getNominalSize());
				}
			}
			else UNREACHABLE(0);
			break;
		case EbtBool:
			if(type.isScalar())
			{
				return GL_BOOL;
			}
			else if(type.isVector())
			{
				switch(type.getNominalSize())
				{
				case 2: return GL_BOOL_VEC2;
				case 3: return GL_BOOL_VEC3;
				case 4: return GL_BOOL_VEC4;
				default: UNREACHABLE(type.getNominalSize());
				}
			}
			else UNREACHABLE(0);
			break;
		case EbtSampler2D:
			return GL_SAMPLER_2D;
		case EbtISampler2D:
			return GL_INT_SAMPLER_2D;
		case EbtUSampler2D:
			return GL_UNSIGNED_INT_SAMPLER_2D;
		case EbtSamplerCube:
			return GL_SAMPLER_CUBE;
		case EbtSampler2DRect:
			return GL_SAMPLER_2D_RECT_ARB;
		case EbtISamplerCube:
			return GL_INT_SAMPLER_CUBE;
		case EbtUSamplerCube:
			return GL_UNSIGNED_INT_SAMPLER_CUBE;
		case EbtSamplerExternalOES:
			return GL_SAMPLER_EXTERNAL_OES;
		case EbtSampler3D:
			return GL_SAMPLER_3D_OES;
		case EbtISampler3D:
			return GL_INT_SAMPLER_3D;
		case EbtUSampler3D:
			return GL_UNSIGNED_INT_SAMPLER_3D;
		case EbtSampler2DArray:
			return GL_SAMPLER_2D_ARRAY;
		case EbtISampler2DArray:
			return GL_INT_SAMPLER_2D_ARRAY;
		case EbtUSampler2DArray:
			return GL_UNSIGNED_INT_SAMPLER_2D_ARRAY;
		case EbtSampler2DShadow:
			return GL_SAMPLER_2D_SHADOW;
		case EbtSamplerCubeShadow:
			return GL_SAMPLER_CUBE_SHADOW;
		case EbtSampler2DArrayShadow:
			return GL_SAMPLER_2D_ARRAY_SHADOW;
		default:
			UNREACHABLE(type.getBasicType());
			break;
		}

		return GL_NONE;
	}

	GLenum glVariablePrecision(const TType &type)
	{
		if(type.getBasicType() == EbtFloat)
		{
			switch(type.getPrecision())
			{
			case EbpHigh:   return GL_HIGH_FLOAT;
			case EbpMedium: return GL_MEDIUM_FLOAT;
			case EbpLow:    return GL_LOW_FLOAT;
			case EbpUndefined:
				// Should be defined as the default precision by the parser
			default: UNREACHABLE(type.getPrecision());
			}
		}
		else if(type.getBasicType() == EbtInt)
		{
			switch(type.getPrecision())
			{
			case EbpHigh:   return GL_HIGH_INT;
			case EbpMedium: return GL_MEDIUM_INT;
			case EbpLow:    return GL_LOW_INT;
			case EbpUndefined:
				// Should be defined as the default precision by the parser
			default: UNREACHABLE(type.getPrecision());
			}
		}

		// Other types (boolean, sampler) don't have a precision
		return GL_NONE;
	}
}

namespace glsl
{
	// Integer to TString conversion
	TString str(int i)
	{
		char buffer[20];
		sprintf(buffer, "%d", i);
		return buffer;
	}

	class Temporary : public TIntermSymbol
	{
	public:
		Temporary(OutputASM *assembler) : TIntermSymbol(TSymbolTableLevel::nextUniqueId(), "tmp", TType(EbtFloat, EbpHigh, EvqTemporary, 4, 1, false)), assembler(assembler)
		{
		}

		~Temporary()
		{
			assembler->freeTemporary(this);
		}

	private:
		OutputASM *const assembler;
	};

	class Constant : public TIntermConstantUnion
	{
	public:
		Constant(float x, float y, float z, float w) : TIntermConstantUnion(constants, TType(EbtFloat, EbpHigh, EvqConstExpr, 4, 1, false))
		{
			constants[0].setFConst(x);
			constants[1].setFConst(y);
			constants[2].setFConst(z);
			constants[3].setFConst(w);
		}

		Constant(bool b) : TIntermConstantUnion(constants, TType(EbtBool, EbpHigh, EvqConstExpr, 1, 1, false))
		{
			constants[0].setBConst(b);
		}

		Constant(int i) : TIntermConstantUnion(constants, TType(EbtInt, EbpHigh, EvqConstExpr, 1, 1, false))
		{
			constants[0].setIConst(i);
		}

		~Constant()
		{
		}

	private:
		ConstantUnion constants[4];
	};

	ShaderVariable::ShaderVariable(const TType& type, const std::string& name, int registerIndex) :
		type(type.isStruct() ? GL_NONE : glVariableType(type)), precision(glVariablePrecision(type)),
		name(name), arraySize(type.getArraySize()), registerIndex(registerIndex)
	{
		if(type.isStruct())
		{
			for(const auto& field : type.getStruct()->fields())
			{
				fields.push_back(ShaderVariable(*(field->type()), field->name().c_str(), -1));
			}
		}
	}

	Uniform::Uniform(const TType& type, const std::string &name, int registerIndex, int blockId, const BlockMemberInfo& blockMemberInfo) :
		ShaderVariable(type, name, registerIndex), blockId(blockId), blockInfo(blockMemberInfo)
	{
	}

	UniformBlock::UniformBlock(const std::string& name, unsigned int dataSize, unsigned int arraySize,
	                           TLayoutBlockStorage layout, bool isRowMajorLayout, int registerIndex, int blockId) :
		name(name), dataSize(dataSize), arraySize(arraySize), layout(layout),
		isRowMajorLayout(isRowMajorLayout), registerIndex(registerIndex), blockId(blockId)
	{
	}

	BlockLayoutEncoder::BlockLayoutEncoder()
		: mCurrentOffset(0)
	{
	}

	BlockMemberInfo BlockLayoutEncoder::encodeType(const TType &type)
	{
		int arrayStride;
		int matrixStride;

		bool isRowMajor = type.getLayoutQualifier().matrixPacking == EmpRowMajor;
		getBlockLayoutInfo(type, type.getArraySize(), isRowMajor, &arrayStride, &matrixStride);

		const BlockMemberInfo memberInfo(static_cast<int>(mCurrentOffset * BytesPerComponent),
		                                 static_cast<int>(arrayStride * BytesPerComponent),
		                                 static_cast<int>(matrixStride * BytesPerComponent),
		                                 (matrixStride > 0) && isRowMajor);

		advanceOffset(type, type.getArraySize(), isRowMajor, arrayStride, matrixStride);

		return memberInfo;
	}

	// static
	size_t BlockLayoutEncoder::getBlockRegister(const BlockMemberInfo &info)
	{
		return (info.offset / BytesPerComponent) / ComponentsPerRegister;
	}

	// static
	size_t BlockLayoutEncoder::getBlockRegisterElement(const BlockMemberInfo &info)
	{
		return (info.offset / BytesPerComponent) % ComponentsPerRegister;
	}

	void BlockLayoutEncoder::nextRegister()
	{
		mCurrentOffset = sw::align(mCurrentOffset, ComponentsPerRegister);
	}

	Std140BlockEncoder::Std140BlockEncoder() : BlockLayoutEncoder()
	{
	}

	void Std140BlockEncoder::enterAggregateType()
	{
		nextRegister();
	}

	void Std140BlockEncoder::exitAggregateType()
	{
		nextRegister();
	}

	void Std140BlockEncoder::getBlockLayoutInfo(const TType &type, unsigned int arraySize, bool isRowMajorMatrix, int *arrayStrideOut, int *matrixStrideOut)
	{
		size_t baseAlignment = 0;
		int matrixStride = 0;
		int arrayStride = 0;

		if(type.isMatrix())
		{
			baseAlignment = ComponentsPerRegister;
			matrixStride = ComponentsPerRegister;

			if(arraySize > 0)
			{
				const int numRegisters = isRowMajorMatrix ? type.getSecondarySize() : type.getNominalSize();
				arrayStride = ComponentsPerRegister * numRegisters;
			}
		}
		else if(arraySize > 0)
		{
			baseAlignment = ComponentsPerRegister;
			arrayStride = ComponentsPerRegister;
		}
		else
		{
			const size_t numComponents = type.getElementSize();
			baseAlignment = (numComponents == 3 ? 4u : numComponents);
		}

		mCurrentOffset = sw::align(mCurrentOffset, baseAlignment);

		*matrixStrideOut = matrixStride;
		*arrayStrideOut = arrayStride;
	}

	void Std140BlockEncoder::advanceOffset(const TType &type, unsigned int arraySize, bool isRowMajorMatrix, int arrayStride, int matrixStride)
	{
		if(arraySize > 0)
		{
			mCurrentOffset += arrayStride * arraySize;
		}
		else if(type.isMatrix())
		{
			ASSERT(matrixStride == ComponentsPerRegister);
			const int numRegisters = isRowMajorMatrix ? type.getSecondarySize() : type.getNominalSize();
			mCurrentOffset += ComponentsPerRegister * numRegisters;
		}
		else
		{
			mCurrentOffset += type.getElementSize();
		}
	}

	Attribute::Attribute()
	{
		type = GL_NONE;
		arraySize = 0;
		registerIndex = 0;
	}

	Attribute::Attribute(GLenum type, const std::string &name, int arraySize, int location, int registerIndex)
	{
		this->type = type;
		this->name = name;
		this->arraySize = arraySize;
		this->location = location;
		this->registerIndex = registerIndex;
	}

	sw::PixelShader *Shader::getPixelShader() const
	{
		return nullptr;
	}

	sw::VertexShader *Shader::getVertexShader() const
	{
		return nullptr;
	}

	OutputASM::TextureFunction::TextureFunction(const TString& nodeName) : method(IMPLICIT), proj(false), offset(false)
	{
		TString name = TFunction::unmangleName(nodeName);

		if(name == "texture2D" || name == "textureCube" || name == "texture" || name == "texture3D" || name == "texture2DRect")
		{
			method = IMPLICIT;
		}
		else if(name == "texture2DProj" || name == "textureProj" || name == "texture2DRectProj")
		{
			method = IMPLICIT;
			proj = true;
		}
		else if(name == "texture2DLod" || name == "textureCubeLod" || name == "textureLod")
		{
			method = LOD;
		}
		else if(name == "texture2DProjLod" || name == "textureProjLod")
		{
			method = LOD;
			proj = true;
		}
		else if(name == "textureSize")
		{
			method = SIZE;
		}
		else if(name == "textureOffset")
		{
			method = IMPLICIT;
			offset = true;
		}
		else if(name == "textureProjOffset")
		{
			method = IMPLICIT;
			offset = true;
			proj = true;
		}
		else if(name == "textureLodOffset")
		{
			method = LOD;
			offset = true;
		}
		else if(name == "textureProjLodOffset")
		{
			method = LOD;
			proj = true;
			offset = true;
		}
		else if(name == "texelFetch")
		{
			method = FETCH;
		}
		else if(name == "texelFetchOffset")
		{
			method = FETCH;
			offset = true;
		}
		else if(name == "textureGrad")
		{
			method = GRAD;
		}
		else if(name == "textureGradOffset")
		{
			method = GRAD;
			offset = true;
		}
		else if(name == "textureProjGrad")
		{
			method = GRAD;
			proj = true;
		}
		else if(name == "textureProjGradOffset")
		{
			method = GRAD;
			proj = true;
			offset = true;
		}
		else UNREACHABLE(0);
	}

	OutputASM::OutputASM(TParseContext &context, Shader *shaderObject) : TIntermTraverser(true, true, true), shaderObject(shaderObject), mContext(context)
	{
		shader = nullptr;
		pixelShader = nullptr;
		vertexShader = nullptr;

		if(shaderObject)
		{
			shader = shaderObject->getShader();
			pixelShader = shaderObject->getPixelShader();
			vertexShader = shaderObject->getVertexShader();
		}

		functionArray.push_back(Function(0, "main(", nullptr, nullptr));
		currentFunction = 0;
		outputQualifier = EvqOutput;   // Initialize outputQualifier to any value other than EvqFragColor or EvqFragData
	}

	OutputASM::~OutputASM()
	{
	}

	void OutputASM::output()
	{
		if(shader)
		{
			emitShader(GLOBAL);

			if(functionArray.size() > 1)   // Only call main() when there are other functions
			{
				Instruction *callMain = emit(sw::Shader::OPCODE_CALL);
				callMain->dst.type = sw::Shader::PARAMETER_LABEL;
				callMain->dst.index = 0;   // main()

				emit(sw::Shader::OPCODE_RET);
			}

			emitShader(FUNCTION);
		}
	}

	void OutputASM::emitShader(Scope scope)
	{
		emitScope = scope;
		currentScope = GLOBAL;
		mContext.getTreeRoot()->traverse(this);
	}

	void OutputASM::freeTemporary(Temporary *temporary)
	{
		free(temporaries, temporary);
	}

	sw::Shader::Opcode OutputASM::getOpcode(sw::Shader::Opcode op, TIntermTyped *in) const
	{
		TBasicType baseType = in->getType().getBasicType();

		switch(op)
		{
		case sw::Shader::OPCODE_NEG:
			switch(baseType)
			{
			case EbtInt:
			case EbtUInt:
				return sw::Shader::OPCODE_INEG;
			case EbtFloat:
			default:
				return op;
			}
		case sw::Shader::OPCODE_ABS:
			switch(baseType)
			{
			case EbtInt:
				return sw::Shader::OPCODE_IABS;
			case EbtFloat:
			default:
				return op;
			}
		case sw::Shader::OPCODE_SGN:
			switch(baseType)
			{
			case EbtInt:
				return sw::Shader::OPCODE_ISGN;
			case EbtFloat:
			default:
				return op;
			}
		case sw::Shader::OPCODE_ADD:
			switch(baseType)
			{
			case EbtInt:
			case EbtUInt:
				return sw::Shader::OPCODE_IADD;
			case EbtFloat:
			default:
				return op;
			}
		case sw::Shader::OPCODE_SUB:
			switch(baseType)
			{
			case EbtInt:
			case EbtUInt:
				return sw::Shader::OPCODE_ISUB;
			case EbtFloat:
			default:
				return op;
			}
		case sw::Shader::OPCODE_MUL:
			switch(baseType)
			{
			case EbtInt:
			case EbtUInt:
				return sw::Shader::OPCODE_IMUL;
			case EbtFloat:
			default:
				return op;
			}
		case sw::Shader::OPCODE_DIV:
			switch(baseType)
			{
			case EbtInt:
				return sw::Shader::OPCODE_IDIV;
			case EbtUInt:
				return sw::Shader::OPCODE_UDIV;
			case EbtFloat:
			default:
				return op;
			}
		case sw::Shader::OPCODE_IMOD:
			return baseType == EbtUInt ? sw::Shader::OPCODE_UMOD : op;
		case sw::Shader::OPCODE_ISHR:
			return baseType == EbtUInt ? sw::Shader::OPCODE_USHR : op;
		case sw::Shader::OPCODE_MIN:
			switch(baseType)
			{
			case EbtInt:
				return sw::Shader::OPCODE_IMIN;
			case EbtUInt:
				return sw::Shader::OPCODE_UMIN;
			case EbtFloat:
			default:
				return op;
			}
		case sw::Shader::OPCODE_MAX:
			switch(baseType)
			{
			case EbtInt:
				return sw::Shader::OPCODE_IMAX;
			case EbtUInt:
				return sw::Shader::OPCODE_UMAX;
			case EbtFloat:
			default:
				return op;
			}
		default:
			return op;
		}
	}

	void OutputASM::visitSymbol(TIntermSymbol *symbol)
	{
		// The type of vertex outputs and fragment inputs with the same name must match (validated at link time),
		// so declare them but don't assign a register index yet (one will be assigned when referenced in reachable code).
		switch(symbol->getQualifier())
		{
		case EvqVaryingIn:
		case EvqVaryingOut:
		case EvqInvariantVaryingIn:
		case EvqInvariantVaryingOut:
		case EvqVertexOut:
		case EvqFragmentIn:
			if(symbol->getBasicType() != EbtInvariant)   // Typeless declarations are not new varyings
			{
				declareVarying(symbol, -1);
			}
			break;
		case EvqFragmentOut:
			declareFragmentOutput(symbol);
			break;
		default:
			break;
		}

		TInterfaceBlock* block = symbol->getType().getInterfaceBlock();
		// OpenGL ES 3.0.4 spec, section 2.12.6 Uniform Variables:
		// "All members of a named uniform block declared with a shared or std140 layout qualifier
		// are considered active, even if they are not referenced in any shader in the program.
		// The uniform block itself is also considered active, even if no member of the block is referenced."
		if(block && ((block->blockStorage() == EbsShared) || (block->blockStorage() == EbsStd140)))
		{
			uniformRegister(symbol);
		}
	}

	bool OutputASM::visitBinary(Visit visit, TIntermBinary *node)
	{
		if(currentScope != emitScope)
		{
			return false;
		}

		TIntermTyped *result = node;
		TIntermTyped *left = node->getLeft();
		TIntermTyped *right = node->getRight();
		const TType &leftType = left->getType();
		const TType &rightType = right->getType();

		if(isSamplerRegister(result))
		{
			return false;   // Don't traverse, the register index is determined statically
		}

		switch(node->getOp())
		{
		case EOpAssign:
			assert(visit == PreVisit);
			right->traverse(this);
			assignLvalue(left, right);
			copy(result, right);
			return false;
		case EOpInitialize:
			assert(visit == PreVisit);
			// Constant arrays go into the constant register file.
			if(leftType.getQualifier() == EvqConstExpr && leftType.isArray() && leftType.getArraySize() > 1)
			{
				for(int i = 0; i < left->totalRegisterCount(); i++)
				{
					emit(sw::Shader::OPCODE_DEF, left, i, right, i);
				}
			}
			else
			{
				right->traverse(this);
				copy(left, right);
			}
			return false;
		case EOpMatrixTimesScalarAssign:
			assert(visit == PreVisit);
			right->traverse(this);
			for(int i = 0; i < leftType.getNominalSize(); i++)
			{
				emit(sw::Shader::OPCODE_MUL, result, i, left, i, right);
			}

			assignLvalue(left, result);
			return false;
		case EOpVectorTimesMatrixAssign:
			assert(visit == PreVisit);
			{
				right->traverse(this);
				int size = leftType.getNominalSize();

				for(int i = 0; i < size; i++)
				{
					Instruction *dot = emit(sw::Shader::OPCODE_DP(size), result, 0, left, 0, right, i);
					dot->dst.mask = 1 << i;
				}

				assignLvalue(left, result);
			}
			return false;
		case EOpMatrixTimesMatrixAssign:
			assert(visit == PreVisit);
			{
				right->traverse(this);
				int dim = leftType.getNominalSize();

				for(int i = 0; i < dim; i++)
				{
					Instruction *mul = emit(sw::Shader::OPCODE_MUL, result, i, left, 0, right, i);
					mul->src[1].swizzle = 0x00;

					for(int j = 1; j < dim; j++)
					{
						Instruction *mad = emit(sw::Shader::OPCODE_MAD, result, i, left, j, right, i, result, i);
						mad->src[1].swizzle = j * 0x55;
					}
				}

				assignLvalue(left, result);
			}
			return false;
		case EOpIndexDirect:
		case EOpIndexIndirect:
		case EOpIndexDirectStruct:
		case EOpIndexDirectInterfaceBlock:
			assert(visit == PreVisit);
			evaluateRvalue(node);
			return false;
		case EOpVectorSwizzle:
			if(visit == PostVisit)
			{
				int swizzle = 0;
				TIntermAggregate *components = right->getAsAggregate();

				if(components)
				{
					TIntermSequence &sequence = components->getSequence();
					int component = 0;

					for(TIntermSequence::iterator sit = sequence.begin(); sit != sequence.end(); sit++)
					{
						TIntermConstantUnion *element = (*sit)->getAsConstantUnion();

						if(element)
						{
							int i = element->getUnionArrayPointer()[0].getIConst();
							swizzle |= i << (component * 2);
							component++;
						}
						else UNREACHABLE(0);
					}
				}
				else UNREACHABLE(0);

				Instruction *mov = emit(sw::Shader::OPCODE_MOV, result, left);
				mov->src[0].swizzle = swizzle;
			}
			break;
		case EOpAddAssign: if(visit == PostVisit) emitAssign(getOpcode(sw::Shader::OPCODE_ADD, result), result, left, left, right); break;
		case EOpAdd:       if(visit == PostVisit) emitBinary(getOpcode(sw::Shader::OPCODE_ADD, result), result, left, right);       break;
		case EOpSubAssign: if(visit == PostVisit) emitAssign(getOpcode(sw::Shader::OPCODE_SUB, result), result, left, left, right); break;
		case EOpSub:       if(visit == PostVisit) emitBinary(getOpcode(sw::Shader::OPCODE_SUB, result), result, left, right);       break;
		case EOpMulAssign: if(visit == PostVisit) emitAssign(getOpcode(sw::Shader::OPCODE_MUL, result), result, left, left, right); break;
		case EOpMul:       if(visit == PostVisit) emitBinary(getOpcode(sw::Shader::OPCODE_MUL, result), result, left, right);       break;
		case EOpDivAssign: if(visit == PostVisit) emitAssign(getOpcode(sw::Shader::OPCODE_DIV, result), result, left, left, right); break;
		case EOpDiv:       if(visit == PostVisit) emitBinary(getOpcode(sw::Shader::OPCODE_DIV, result), result, left, right);       break;
		case EOpIModAssign:          if(visit == PostVisit) emitAssign(getOpcode(sw::Shader::OPCODE_IMOD, result), result, left, left, right); break;
		case EOpIMod:                if(visit == PostVisit) emitBinary(getOpcode(sw::Shader::OPCODE_IMOD, result), result, left, right);       break;
		case EOpBitShiftLeftAssign:  if(visit == PostVisit) emitAssign(sw::Shader::OPCODE_SHL, result, left, left, right); break;
		case EOpBitShiftLeft:        if(visit == PostVisit) emitBinary(sw::Shader::OPCODE_SHL, result, left, right);       break;
		case EOpBitShiftRightAssign: if(visit == PostVisit) emitAssign(getOpcode(sw::Shader::OPCODE_ISHR, result), result, left, left, right); break;
		case EOpBitShiftRight:       if(visit == PostVisit) emitBinary(getOpcode(sw::Shader::OPCODE_ISHR, result), result, left, right);       break;
		case EOpBitwiseAndAssign:    if(visit == PostVisit) emitAssign(sw::Shader::OPCODE_AND, result, left, left, right); break;
		case EOpBitwiseAnd:          if(visit == PostVisit) emitBinary(sw::Shader::OPCODE_AND, result, left, right);       break;
		case EOpBitwiseXorAssign:    if(visit == PostVisit) emitAssign(sw::Shader::OPCODE_XOR, result, left, left, right); break;
		case EOpBitwiseXor:          if(visit == PostVisit) emitBinary(sw::Shader::OPCODE_XOR, result, left, right);       break;
		case EOpBitwiseOrAssign:     if(visit == PostVisit) emitAssign(sw::Shader::OPCODE_OR, result, left, left, right);  break;
		case EOpBitwiseOr:           if(visit == PostVisit) emitBinary(sw::Shader::OPCODE_OR, result, left, right);        break;
		case EOpEqual:
			if(visit == PostVisit)
			{
				emitBinary(sw::Shader::OPCODE_EQ, result, left, right);

				for(int index = 1; index < left->totalRegisterCount(); index++)
				{
					Temporary equal(this);
					emit(sw::Shader::OPCODE_EQ, &equal, 0, left, index, right, index);
					emit(sw::Shader::OPCODE_AND, result, result, &equal);
				}
			}
			break;
		case EOpNotEqual:
			if(visit == PostVisit)
			{
				emitBinary(sw::Shader::OPCODE_NE, result, left, right);

				for(int index = 1; index < left->totalRegisterCount(); index++)
				{
					Temporary notEqual(this);
					emit(sw::Shader::OPCODE_NE, &notEqual, 0, left, index, right, index);
					emit(sw::Shader::OPCODE_OR, result, result, &notEqual);
				}
			}
			break;
		case EOpLessThan:                if(visit == PostVisit) emitCmp(sw::Shader::CONTROL_LT, result, left, right); break;
		case EOpGreaterThan:             if(visit == PostVisit) emitCmp(sw::Shader::CONTROL_GT, result, left, right); break;
		case EOpLessThanEqual:           if(visit == PostVisit) emitCmp(sw::Shader::CONTROL_LE, result, left, right); break;
		case EOpGreaterThanEqual:        if(visit == PostVisit) emitCmp(sw::Shader::CONTROL_GE, result, left, right); break;
		case EOpVectorTimesScalarAssign: if(visit == PostVisit) emitAssign(getOpcode(sw::Shader::OPCODE_MUL, left), result, left, left, right); break;
		case EOpVectorTimesScalar:       if(visit == PostVisit) emit(getOpcode(sw::Shader::OPCODE_MUL, left), result, left, right); break;
		case EOpMatrixTimesScalar:
			if(visit == PostVisit)
			{
				if(left->isMatrix())
				{
					for(int i = 0; i < leftType.getNominalSize(); i++)
					{
						emit(sw::Shader::OPCODE_MUL, result, i, left, i, right, 0);
					}
				}
				else if(right->isMatrix())
				{
					for(int i = 0; i < rightType.getNominalSize(); i++)
					{
						emit(sw::Shader::OPCODE_MUL, result, i, left, 0, right, i);
					}
				}
				else UNREACHABLE(0);
			}
			break;
		case EOpVectorTimesMatrix:
			if(visit == PostVisit)
			{
				sw::Shader::Opcode dpOpcode = sw::Shader::OPCODE_DP(leftType.getNominalSize());

				int size = rightType.getNominalSize();
				for(int i = 0; i < size; i++)
				{
					Instruction *dot = emit(dpOpcode, result, 0, left, 0, right, i);
					dot->dst.mask = 1 << i;
				}
			}
			break;
		case EOpMatrixTimesVector:
			if(visit == PostVisit)
			{
				Instruction *mul = emit(sw::Shader::OPCODE_MUL, result, left, right);
				mul->src[1].swizzle = 0x00;

				int size = rightType.getNominalSize();
				for(int i = 1; i < size; i++)
				{
					Instruction *mad = emit(sw::Shader::OPCODE_MAD, result, 0, left, i, right, 0, result);
					mad->src[1].swizzle = i * 0x55;
				}
			}
			break;
		case EOpMatrixTimesMatrix:
			if(visit == PostVisit)
			{
				int dim = leftType.getNominalSize();

				int size = rightType.getNominalSize();
				for(int i = 0; i < size; i++)
				{
					Instruction *mul = emit(sw::Shader::OPCODE_MUL, result, i, left, 0, right, i);
					mul->src[1].swizzle = 0x00;

					for(int j = 1; j < dim; j++)
					{
						Instruction *mad = emit(sw::Shader::OPCODE_MAD, result, i, left, j, right, i, result, i);
						mad->src[1].swizzle = j * 0x55;
					}
				}
			}
			break;
		case EOpLogicalOr:
			if(trivial(right, 6))
			{
				if(visit == PostVisit)
				{
					emit(sw::Shader::OPCODE_OR, result, left, right);
				}
			}
			else   // Short-circuit evaluation
			{
				if(visit == InVisit)
				{
					emit(sw::Shader::OPCODE_MOV, result, left);
					Instruction *ifnot = emit(sw::Shader::OPCODE_IF, 0, result);
					ifnot->src[0].modifier = sw::Shader::MODIFIER_NOT;
				}
				else if(visit == PostVisit)
				{
					emit(sw::Shader::OPCODE_MOV, result, right);
					emit(sw::Shader::OPCODE_ENDIF);
				}
			}
			break;
		case EOpLogicalXor:        if(visit == PostVisit) emit(sw::Shader::OPCODE_XOR, result, left, right); break;
		case EOpLogicalAnd:
			if(trivial(right, 6))
			{
				if(visit == PostVisit)
				{
					emit(sw::Shader::OPCODE_AND, result, left, right);
				}
			}
			else   // Short-circuit evaluation
			{
				if(visit == InVisit)
				{
					emit(sw::Shader::OPCODE_MOV, result, left);
					emit(sw::Shader::OPCODE_IF, 0, result);
				}
				else if(visit == PostVisit)
				{
					emit(sw::Shader::OPCODE_MOV, result, right);
					emit(sw::Shader::OPCODE_ENDIF);
				}
			}
			break;
		default: UNREACHABLE(node->getOp());
		}

		return true;
	}

	void OutputASM::emitDeterminant(TIntermTyped *result, TIntermTyped *arg, int size, int col, int row, int outCol, int outRow)
	{
		switch(size)
		{
		case 1: // Used for cofactor computation only
			{
				// For a 2x2 matrix, the cofactor is simply a transposed move or negate
				bool isMov = (row == col);
				sw::Shader::Opcode op = isMov ? sw::Shader::OPCODE_MOV : sw::Shader::OPCODE_NEG;
				Instruction *mov = emit(op, result, outCol, arg, isMov ? 1 - row : row);
				mov->src[0].swizzle = 0x55 * (isMov ? 1 - col : col);
				mov->dst.mask = 1 << outRow;
			}
			break;
		case 2:
			{
				static const unsigned int swizzle[3] = { 0x99, 0x88, 0x44 }; // xy?? : yzyz, xzxz, xyxy

				bool isCofactor = (col >= 0) && (row >= 0);
				int col0 = (isCofactor && (col <= 0)) ? 1 : 0;
				int col1 = (isCofactor && (col <= 1)) ? 2 : 1;
				bool negate = isCofactor && ((col & 0x01) ^ (row & 0x01));

				Instruction *det = emit(sw::Shader::OPCODE_DET2, result, outCol, arg, negate ? col1 : col0, arg, negate ? col0 : col1);
				det->src[0].swizzle = det->src[1].swizzle = swizzle[isCofactor ? row : 2];
				det->dst.mask = 1 << outRow;
			}
			break;
		case 3:
			{
				static const unsigned int swizzle[4] = { 0xF9, 0xF8, 0xF4, 0xE4 }; // xyz? : yzww, xzww, xyww, xyzw

				bool isCofactor = (col >= 0) && (row >= 0);
				int col0 = (isCofactor && (col <= 0)) ? 1 : 0;
				int col1 = (isCofactor && (col <= 1)) ? 2 : 1;
				int col2 = (isCofactor && (col <= 2)) ? 3 : 2;
				bool negate = isCofactor && ((col & 0x01) ^ (row & 0x01));

				Instruction *det = emit(sw::Shader::OPCODE_DET3, result, outCol, arg, col0, arg, negate ? col2 : col1, arg, negate ? col1 : col2);
				det->src[0].swizzle = det->src[1].swizzle = det->src[2].swizzle = swizzle[isCofactor ? row : 3];
				det->dst.mask = 1 << outRow;
			}
			break;
		case 4:
			{
				Instruction *det = emit(sw::Shader::OPCODE_DET4, result, outCol, arg, 0, arg, 1, arg, 2, arg, 3);
				det->dst.mask = 1 << outRow;
			}
			break;
		default:
			UNREACHABLE(size);
			break;
		}
	}

	bool OutputASM::visitUnary(Visit visit, TIntermUnary *node)
	{
		if(currentScope != emitScope)
		{
			return false;
		}

		TIntermTyped *result = node;
		TIntermTyped *arg = node->getOperand();
		TBasicType basicType = arg->getType().getBasicType();

		union
		{
			float f;
			int i;
		} one_value;

		if(basicType == EbtInt || basicType == EbtUInt)
		{
			one_value.i = 1;
		}
		else
		{
			one_value.f = 1.0f;
		}

		Constant one(one_value.f, one_value.f, one_value.f, one_value.f);
		Constant rad(1.74532925e-2f, 1.74532925e-2f, 1.74532925e-2f, 1.74532925e-2f);
		Constant deg(5.72957795e+1f, 5.72957795e+1f, 5.72957795e+1f, 5.72957795e+1f);

		switch(node->getOp())
		{
		case EOpNegative:
			if(visit == PostVisit)
			{
				sw::Shader::Opcode negOpcode = getOpcode(sw::Shader::OPCODE_NEG, arg);
				for(int index = 0; index < arg->totalRegisterCount(); index++)
				{
					emit(negOpcode, result, index, arg, index);
				}
			}
			break;
		case EOpVectorLogicalNot: if(visit == PostVisit) emit(sw::Shader::OPCODE_NOT, result, arg); break;
		case EOpLogicalNot:       if(visit == PostVisit) emit(sw::Shader::OPCODE_NOT, result, arg); break;
		case EOpBitwiseNot:       if(visit == PostVisit) emit(sw::Shader::OPCODE_NOT, result, arg); break;
		case EOpPostIncrement:
			if(visit == PostVisit)
			{
				copy(result, arg);

				sw::Shader::Opcode addOpcode = getOpcode(sw::Shader::OPCODE_ADD, arg);
				for(int index = 0; index < arg->totalRegisterCount(); index++)
				{
					emit(addOpcode, arg, index, arg, index, &one);
				}

				assignLvalue(arg, arg);
			}
			break;
		case EOpPostDecrement:
			if(visit == PostVisit)
			{
				copy(result, arg);

				sw::Shader::Opcode subOpcode = getOpcode(sw::Shader::OPCODE_SUB, arg);
				for(int index = 0; index < arg->totalRegisterCount(); index++)
				{
					emit(subOpcode, arg, index, arg, index, &one);
				}

				assignLvalue(arg, arg);
			}
			break;
		case EOpPreIncrement:
			if(visit == PostVisit)
			{
				sw::Shader::Opcode addOpcode = getOpcode(sw::Shader::OPCODE_ADD, arg);
				for(int index = 0; index < arg->totalRegisterCount(); index++)
				{
					emit(addOpcode, result, index, arg, index, &one);
				}

				assignLvalue(arg, result);
			}
			break;
		case EOpPreDecrement:
			if(visit == PostVisit)
			{
				sw::Shader::Opcode subOpcode = getOpcode(sw::Shader::OPCODE_SUB, arg);
				for(int index = 0; index < arg->totalRegisterCount(); index++)
				{
					emit(subOpcode, result, index, arg, index, &one);
				}

				assignLvalue(arg, result);
			}
			break;
		case EOpRadians:          if(visit == PostVisit) emit(sw::Shader::OPCODE_MUL, result, arg, &rad); break;
		case EOpDegrees:          if(visit == PostVisit) emit(sw::Shader::OPCODE_MUL, result, arg, &deg); break;
		case EOpSin:              if(visit == PostVisit) emit(sw::Shader::OPCODE_SIN, result, arg); break;
		case EOpCos:              if(visit == PostVisit) emit(sw::Shader::OPCODE_COS, result, arg); break;
		case EOpTan:              if(visit == PostVisit) emit(sw::Shader::OPCODE_TAN, result, arg); break;
		case EOpAsin:             if(visit == PostVisit) emit(sw::Shader::OPCODE_ASIN, result, arg); break;
		case EOpAcos:             if(visit == PostVisit) emit(sw::Shader::OPCODE_ACOS, result, arg); break;
		case EOpAtan:             if(visit == PostVisit) emit(sw::Shader::OPCODE_ATAN, result, arg); break;
		case EOpSinh:             if(visit == PostVisit) emit(sw::Shader::OPCODE_SINH, result, arg); break;
		case EOpCosh:             if(visit == PostVisit) emit(sw::Shader::OPCODE_COSH, result, arg); break;
		case EOpTanh:             if(visit == PostVisit) emit(sw::Shader::OPCODE_TANH, result, arg); break;
		case EOpAsinh:            if(visit == PostVisit) emit(sw::Shader::OPCODE_ASINH, result, arg); break;
		case EOpAcosh:            if(visit == PostVisit) emit(sw::Shader::OPCODE_ACOSH, result, arg); break;
		case EOpAtanh:            if(visit == PostVisit) emit(sw::Shader::OPCODE_ATANH, result, arg); break;
		case EOpExp:              if(visit == PostVisit) emit(sw::Shader::OPCODE_EXP, result, arg); break;
		case EOpLog:              if(visit == PostVisit) emit(sw::Shader::OPCODE_LOG, result, arg); break;
		case EOpExp2:             if(visit == PostVisit) emit(sw::Shader::OPCODE_EXP2, result, arg); break;
		case EOpLog2:             if(visit == PostVisit) emit(sw::Shader::OPCODE_LOG2, result, arg); break;
		case EOpSqrt:             if(visit == PostVisit) emit(sw::Shader::OPCODE_SQRT, result, arg); break;
		case EOpInverseSqrt:      if(visit == PostVisit) emit(sw::Shader::OPCODE_RSQ, result, arg); break;
		case EOpAbs:              if(visit == PostVisit) emit(getOpcode(sw::Shader::OPCODE_ABS, result), result, arg); break;
		case EOpSign:             if(visit == PostVisit) emit(getOpcode(sw::Shader::OPCODE_SGN, result), result, arg); break;
		case EOpFloor:            if(visit == PostVisit) emit(sw::Shader::OPCODE_FLOOR, result, arg); break;
		case EOpTrunc:            if(visit == PostVisit) emit(sw::Shader::OPCODE_TRUNC, result, arg); break;
		case EOpRound:            if(visit == PostVisit) emit(sw::Shader::OPCODE_ROUND, result, arg); break;
		case EOpRoundEven:        if(visit == PostVisit) emit(sw::Shader::OPCODE_ROUNDEVEN, result, arg); break;
		case EOpCeil:             if(visit == PostVisit) emit(sw::Shader::OPCODE_CEIL, result, arg, result); break;
		case EOpFract:            if(visit == PostVisit) emit(sw::Shader::OPCODE_FRC, result, arg); break;
		case EOpIsNan:            if(visit == PostVisit) emit(sw::Shader::OPCODE_ISNAN, result, arg); break;
		case EOpIsInf:            if(visit == PostVisit) emit(sw::Shader::OPCODE_ISINF, result, arg); break;
		case EOpLength:           if(visit == PostVisit) emit(sw::Shader::OPCODE_LEN(dim(arg)), result, arg); break;
		case EOpNormalize:        if(visit == PostVisit) emit(sw::Shader::OPCODE_NRM(dim(arg)), result, arg); break;
		case EOpDFdx:             if(visit == PostVisit) emit(sw::Shader::OPCODE_DFDX, result, arg); break;
		case EOpDFdy:             if(visit == PostVisit) emit(sw::Shader::OPCODE_DFDY, result, arg); break;
		case EOpFwidth:           if(visit == PostVisit) emit(sw::Shader::OPCODE_FWIDTH, result, arg); break;
		case EOpAny:              if(visit == PostVisit) emit(sw::Shader::OPCODE_ANY, result, arg); break;
		case EOpAll:              if(visit == PostVisit) emit(sw::Shader::OPCODE_ALL, result, arg); break;
		case EOpFloatBitsToInt:   if(visit == PostVisit) emit(sw::Shader::OPCODE_FLOATBITSTOINT, result, arg); break;
		case EOpFloatBitsToUint:  if(visit == PostVisit) emit(sw::Shader::OPCODE_FLOATBITSTOUINT, result, arg); break;
		case EOpIntBitsToFloat:   if(visit == PostVisit) emit(sw::Shader::OPCODE_INTBITSTOFLOAT, result, arg); break;
		case EOpUintBitsToFloat:  if(visit == PostVisit) emit(sw::Shader::OPCODE_UINTBITSTOFLOAT, result, arg); break;
		case EOpPackSnorm2x16:    if(visit == PostVisit) emit(sw::Shader::OPCODE_PACKSNORM2x16, result, arg); break;
		case EOpPackUnorm2x16:    if(visit == PostVisit) emit(sw::Shader::OPCODE_PACKUNORM2x16, result, arg); break;
		case EOpPackHalf2x16:     if(visit == PostVisit) emit(sw::Shader::OPCODE_PACKHALF2x16, result, arg); break;
		case EOpUnpackSnorm2x16:  if(visit == PostVisit) emit(sw::Shader::OPCODE_UNPACKSNORM2x16, result, arg); break;
		case EOpUnpackUnorm2x16:  if(visit == PostVisit) emit(sw::Shader::OPCODE_UNPACKUNORM2x16, result, arg); break;
		case EOpUnpackHalf2x16:   if(visit == PostVisit) emit(sw::Shader::OPCODE_UNPACKHALF2x16, result, arg); break;
		case EOpTranspose:
			if(visit == PostVisit)
			{
				int numCols = arg->getNominalSize();
				int numRows = arg->getSecondarySize();
				for(int i = 0; i < numCols; ++i)
				{
					for(int j = 0; j < numRows; ++j)
					{
						Instruction *mov = emit(sw::Shader::OPCODE_MOV, result, j, arg, i);
						mov->src[0].swizzle = 0x55 * j;
						mov->dst.mask = 1 << i;
					}
				}
			}
			break;
		case EOpDeterminant:
			if(visit == PostVisit)
			{
				int size = arg->getNominalSize();
				ASSERT(size == arg->getSecondarySize());

				emitDeterminant(result, arg, size);
			}
			break;
		case EOpInverse:
			if(visit == PostVisit)
			{
				int size = arg->getNominalSize();
				ASSERT(size == arg->getSecondarySize());

				// Compute transposed matrix of cofactors
				for(int i = 0; i < size; ++i)
				{
					for(int j = 0; j < size; ++j)
					{
						// For a 2x2 matrix, the cofactor is simply a transposed move or negate
						// For a 3x3 or 4x4 matrix, the cofactor is a transposed determinant
						emitDeterminant(result, arg, size - 1, j, i, i, j);
					}
				}

				// Compute 1 / determinant
				Temporary invDet(this);
				emitDeterminant(&invDet, arg, size);
				Constant one(1.0f, 1.0f, 1.0f, 1.0f);
				Instruction *div = emit(sw::Shader::OPCODE_DIV, &invDet, &one, &invDet);
				div->src[1].swizzle = 0x00; // xxxx

				// Divide transposed matrix of cofactors by determinant
				for(int i = 0; i < size; ++i)
				{
					emit(sw::Shader::OPCODE_MUL, result, i, result, i, &invDet);
				}
			}
			break;
		default: UNREACHABLE(node->getOp());
		}

		return true;
	}

	bool OutputASM::visitAggregate(Visit visit, TIntermAggregate *node)
	{
		if(currentScope != emitScope && node->getOp() != EOpFunction && node->getOp() != EOpSequence)
		{
			return false;
		}

		Constant zero(0.0f, 0.0f, 0.0f, 0.0f);

		TIntermTyped *result = node;
		const TType &resultType = node->getType();
		TIntermSequence &arg = node->getSequence();
		size_t argumentCount = arg.size();

		switch(node->getOp())
		{
		case EOpSequence:             break;
		case EOpDeclaration:          break;
		case EOpInvariantDeclaration: break;
		case EOpPrototype:            break;
		case EOpComma:
			if(visit == PostVisit)
			{
				copy(result, arg[1]);
			}
			break;
		case EOpFunction:
			if(visit == PreVisit)
			{
				const TString &name = node->getName();

				if(emitScope == FUNCTION)
				{
					if(functionArray.size() > 1)   // No need for a label when there's only main()
					{
						Instruction *label = emit(sw::Shader::OPCODE_LABEL);
						label->dst.type = sw::Shader::PARAMETER_LABEL;

						const Function *function = findFunction(name);
						ASSERT(function);   // Should have been added during global pass
						label->dst.index = function->label;
						currentFunction = function->label;
					}
				}
				else if(emitScope == GLOBAL)
				{
					if(name != "main(")
					{
						TIntermSequence &arguments = node->getSequence()[0]->getAsAggregate()->getSequence();
						functionArray.push_back(Function(functionArray.size(), name, &arguments, node));
					}
				}
				else UNREACHABLE(emitScope);

				currentScope = FUNCTION;
			}
			else if(visit == PostVisit)
			{
				if(emitScope == FUNCTION)
				{
					if(functionArray.size() > 1)   // No need to return when there's only main()
					{
						emit(sw::Shader::OPCODE_RET);
					}
				}

				currentScope = GLOBAL;
			}
			break;
		case EOpFunctionCall:
			if(visit == PostVisit)
			{
				if(node->isUserDefined())
				{
					const TString &name = node->getName();
					const Function *function = findFunction(name);

					if(!function)
					{
						mContext.error(node->getLine(), "function definition not found", name.c_str());
						return false;
					}

					TIntermSequence &arguments = *function->arg;

					for(size_t i = 0; i < argumentCount; i++)
					{
						TIntermTyped *in = arguments[i]->getAsTyped();

						if(in->getQualifier() == EvqIn ||
						   in->getQualifier() == EvqInOut ||
						   in->getQualifier() == EvqConstReadOnly)
						{
							copy(in, arg[i]);
						}
					}

					Instruction *call = emit(sw::Shader::OPCODE_CALL);
					call->dst.type = sw::Shader::PARAMETER_LABEL;
					call->dst.index = function->label;

					if(function->ret && function->ret->getType().getBasicType() != EbtVoid)
					{
						copy(result, function->ret);
					}

					for(size_t i = 0; i < argumentCount; i++)
					{
						TIntermTyped *argument = arguments[i]->getAsTyped();
						TIntermTyped *out = arg[i]->getAsTyped();

						if(argument->getQualifier() == EvqOut ||
						   argument->getQualifier() == EvqInOut)
						{
							assignLvalue(out, argument);
						}
					}
				}
				else
				{
					const TextureFunction textureFunction(node->getName());
					TIntermTyped *s = arg[0]->getAsTyped();
					TIntermTyped *t = arg[1]->getAsTyped();

					Temporary coord(this);

					if(textureFunction.proj)
					{
						Instruction *rcp = emit(sw::Shader::OPCODE_RCPX, &coord, arg[1]);
						rcp->src[0].swizzle = 0x55 * (t->getNominalSize() - 1);
						rcp->dst.mask = 0x7;

						Instruction *mul = emit(sw::Shader::OPCODE_MUL, &coord, arg[1], &coord);
						mul->dst.mask = 0x7;

						if(IsShadowSampler(s->getBasicType()))
						{
							ASSERT(s->getBasicType() == EbtSampler2DShadow);
							Instruction *mov = emit(sw::Shader::OPCODE_MOV, &coord, &coord);
							mov->src[0].swizzle = 0xA4;
						}
					}
					else
					{
						Instruction *mov = emit(sw::Shader::OPCODE_MOV, &coord, arg[1]);

						if(IsShadowSampler(s->getBasicType()) && t->getNominalSize() == 3)
						{
							ASSERT(s->getBasicType() == EbtSampler2DShadow);
							mov->src[0].swizzle = 0xA4;
						}
					}

					switch(textureFunction.method)
					{
					case TextureFunction::IMPLICIT:
						if(!textureFunction.offset)
						{
							if(argumentCount == 2)
							{
								emit(sw::Shader::OPCODE_TEX, result, &coord, s);
							}
							else if(argumentCount == 3)   // Bias
							{
								emit(sw::Shader::OPCODE_TEXBIAS, result, &coord, s, arg[2]);
							}
							else UNREACHABLE(argumentCount);
						}
						else   // Offset
						{
							if(argumentCount == 3)
							{
								emit(sw::Shader::OPCODE_TEXOFFSET, result, &coord, s, arg[2]);
							}
							else if(argumentCount == 4)   // Bias
							{
								emit(sw::Shader::OPCODE_TEXOFFSETBIAS, result, &coord, s, arg[2], arg[3]);
							}
							else UNREACHABLE(argumentCount);
						}
						break;
					case TextureFunction::LOD:
						if(!textureFunction.offset && argumentCount == 3)
						{
							emit(sw::Shader::OPCODE_TEXLOD, result, &coord, s, arg[2]);
						}
						else if(argumentCount == 4)   // Offset
						{
							emit(sw::Shader::OPCODE_TEXLODOFFSET, result, &coord, s, arg[3], arg[2]);
						}
						else UNREACHABLE(argumentCount);
						break;
					case TextureFunction::FETCH:
						if(!textureFunction.offset && argumentCount == 3)
						{
							emit(sw::Shader::OPCODE_TEXELFETCH, result, &coord, s, arg[2]);
						}
						else if(argumentCount == 4)   // Offset
						{
							emit(sw::Shader::OPCODE_TEXELFETCHOFFSET, result, &coord, s, arg[3], arg[2]);
						}
						else UNREACHABLE(argumentCount);
						break;
					case TextureFunction::GRAD:
						if(!textureFunction.offset && argumentCount == 4)
						{
							emit(sw::Shader::OPCODE_TEXGRAD, result, &coord, s, arg[2], arg[3]);
						}
						else if(argumentCount == 5)   // Offset
						{
							emit(sw::Shader::OPCODE_TEXGRADOFFSET, result, &coord, s, arg[2], arg[3], arg[4]);
						}
						else UNREACHABLE(argumentCount);
						break;
					case TextureFunction::SIZE:
						emit(sw::Shader::OPCODE_TEXSIZE, result, arg[1], s);
						break;
					default:
						UNREACHABLE(textureFunction.method);
					}
				}
			}
			break;
		case EOpParameters:
			break;
		case EOpConstructFloat:
		case EOpConstructVec2:
		case EOpConstructVec3:
		case EOpConstructVec4:
		case EOpConstructBool:
		case EOpConstructBVec2:
		case EOpConstructBVec3:
		case EOpConstructBVec4:
		case EOpConstructInt:
		case EOpConstructIVec2:
		case EOpConstructIVec3:
		case EOpConstructIVec4:
		case EOpConstructUInt:
		case EOpConstructUVec2:
		case EOpConstructUVec3:
		case EOpConstructUVec4:
			if(visit == PostVisit)
			{
				int component = 0;
				int arrayMaxIndex = result->isArray() ? result->getArraySize() - 1 : 0;
				int arrayComponents = result->getType().getElementSize();
				for(size_t i = 0; i < argumentCount; i++)
				{
					TIntermTyped *argi = arg[i]->getAsTyped();
					int size = argi->getNominalSize();
					int arrayIndex = std::min(component / arrayComponents, arrayMaxIndex);
					int swizzle = component - (arrayIndex * arrayComponents);

					if(!argi->isMatrix())
					{
						Instruction *mov = emitCast(result, arrayIndex, argi, 0);
						mov->dst.mask = (0xF << swizzle) & 0xF;
						mov->src[0].swizzle = readSwizzle(argi, size) << (swizzle * 2);

						component += size;
					}
					else if(!result->isMatrix()) // Construct a non matrix from a matrix
					{
						Instruction *mov = emitCast(result, arrayIndex, argi, 0);
						mov->dst.mask = (0xF << swizzle) & 0xF;
						mov->src[0].swizzle = readSwizzle(argi, size) << (swizzle * 2);

						// At most one more instruction when constructing a vec3 from a mat2 or a vec4 from a mat2/mat3
						if(result->getNominalSize() > size)
						{
							Instruction *mov = emitCast(result, arrayIndex, argi, 1);
							mov->dst.mask = (0xF << (swizzle + size)) & 0xF;
							// mat2: xxxy (0x40), mat3: xxxx (0x00)
							mov->src[0].swizzle = ((size == 2) ? 0x40 : 0x00) << (swizzle * 2);
						}

						component += size;
					}
					else   // Matrix
					{
						int column = 0;

						while(component < resultType.getNominalSize())
						{
							Instruction *mov = emitCast(result, arrayIndex, argi, column);
							mov->dst.mask = (0xF << swizzle) & 0xF;
							mov->src[0].swizzle = readSwizzle(argi, size) << (swizzle * 2);

							column++;
							component += size;
						}
					}
				}
			}
			break;
		case EOpConstructMat2:
		case EOpConstructMat2x3:
		case EOpConstructMat2x4:
		case EOpConstructMat3x2:
		case EOpConstructMat3:
		case EOpConstructMat3x4:
		case EOpConstructMat4x2:
		case EOpConstructMat4x3:
		case EOpConstructMat4:
			if(visit == PostVisit)
			{
				TIntermTyped *arg0 = arg[0]->getAsTyped();
				const int outCols = result->getNominalSize();
				const int outRows = result->getSecondarySize();

				if(arg0->isScalar() && arg.size() == 1)   // Construct scale matrix
				{
					for(int i = 0; i < outCols; i++)
					{
						emit(sw::Shader::OPCODE_MOV, result, i, &zero);
						Instruction *mov = emitCast(result, i, arg0, 0);
						mov->dst.mask = 1 << i;
						ASSERT(mov->src[0].swizzle == 0x00);
					}
				}
				else if(arg0->isMatrix())
				{
					int arraySize = result->isArray() ? result->getArraySize() : 1;

					for(int n = 0; n < arraySize; n++)
					{
						TIntermTyped *argi = arg[n]->getAsTyped();
						const int inCols = argi->getNominalSize();
						const int inRows = argi->getSecondarySize();

						for(int i = 0; i < outCols; i++)
						{
							if(i >= inCols || outRows > inRows)
							{
								// Initialize to identity matrix
								Constant col((i == 0 ? 1.0f : 0.0f), (i == 1 ? 1.0f : 0.0f), (i == 2 ? 1.0f : 0.0f), (i == 3 ? 1.0f : 0.0f));
								emitCast(result, i + n * outCols, &col, 0);
							}

							if(i < inCols)
							{
								Instruction *mov = emitCast(result, i + n * outCols, argi, i);
								mov->dst.mask = 0xF >> (4 - inRows);
							}
						}
					}
				}
				else
				{
					int column = 0;
					int row = 0;

					for(size_t i = 0; i < argumentCount; i++)
					{
						TIntermTyped *argi = arg[i]->getAsTyped();
						int size = argi->getNominalSize();
						int element = 0;

						while(element < size)
						{
							Instruction *mov = emitCast(result, column, argi, 0);
							mov->dst.mask = (0xF << row) & 0xF;
							mov->src[0].swizzle = (readSwizzle(argi, size) << (row * 2)) + 0x55 * element;

							int end = row + size - element;
							column = end >= outRows ? column + 1 : column;
							element = element + outRows - row;
							row = end >= outRows ? 0 : end;
						}
					}
				}
			}
			break;
		case EOpConstructStruct:
			if(visit == PostVisit)
			{
				int offset = 0;
				for(size_t i = 0; i < argumentCount; i++)
				{
					TIntermTyped *argi = arg[i]->getAsTyped();
					int size = argi->totalRegisterCount();

					for(int index = 0; index < size; index++)
					{
						Instruction *mov = emit(sw::Shader::OPCODE_MOV, result, index + offset, argi, index);
						mov->dst.mask = writeMask(result, offset + index);
					}

					offset += size;
				}
			}
			break;
		case EOpLessThan:         if(visit == PostVisit) emitCmp(sw::Shader::CONTROL_LT, result, arg[0], arg[1]); break;
		case EOpGreaterThan:      if(visit == PostVisit) emitCmp(sw::Shader::CONTROL_GT, result, arg[0], arg[1]); break;
		case EOpLessThanEqual:    if(visit == PostVisit) emitCmp(sw::Shader::CONTROL_LE, result, arg[0], arg[1]); break;
		case EOpGreaterThanEqual: if(visit == PostVisit) emitCmp(sw::Shader::CONTROL_GE, result, arg[0], arg[1]); break;
		case EOpVectorEqual:      if(visit == PostVisit) emitCmp(sw::Shader::CONTROL_EQ, result, arg[0], arg[1]); break;
		case EOpVectorNotEqual:   if(visit == PostVisit) emitCmp(sw::Shader::CONTROL_NE, result, arg[0], arg[1]); break;
		case EOpMod:              if(visit == PostVisit) emit(sw::Shader::OPCODE_MOD, result, arg[0], arg[1]); break;
		case EOpModf:
			if(visit == PostVisit)
			{
				TIntermTyped* arg1 = arg[1]->getAsTyped();
				emit(sw::Shader::OPCODE_TRUNC, arg1, arg[0]);
				assignLvalue(arg1, arg1);
				emitBinary(sw::Shader::OPCODE_SUB, result, arg[0], arg1);
			}
			break;
		case EOpPow:              if(visit == PostVisit) emit(sw::Shader::OPCODE_POW, result, arg[0], arg[1]); break;
		case EOpAtan:             if(visit == PostVisit) emit(sw::Shader::OPCODE_ATAN2, result, arg[0], arg[1]); break;
		case EOpMin:              if(visit == PostVisit) emit(getOpcode(sw::Shader::OPCODE_MIN, result), result, arg[0], arg[1]); break;
		case EOpMax:              if(visit == PostVisit) emit(getOpcode(sw::Shader::OPCODE_MAX, result), result, arg[0], arg[1]); break;
		case EOpClamp:
			if(visit == PostVisit)
			{
				emit(getOpcode(sw::Shader::OPCODE_MAX, result), result, arg[0], arg[1]);
				emit(getOpcode(sw::Shader::OPCODE_MIN, result), result, result, arg[2]);
			}
			break;
		case EOpMix:
			if(visit == PostVisit)
			{
				if(arg[2]->getAsTyped()->getBasicType() == EbtBool)
				{
					emit(sw::Shader::OPCODE_SELECT, result, arg[2], arg[1], arg[0]);
				}
				else
				{
					emit(sw::Shader::OPCODE_LRP, result, arg[2], arg[1], arg[0]);
				}
			}
			break;
		case EOpStep:        if(visit == PostVisit) emit(sw::Shader::OPCODE_STEP, result, arg[0], arg[1]); break;
		case EOpSmoothStep:  if(visit == PostVisit) emit(sw::Shader::OPCODE_SMOOTH, result, arg[0], arg[1], arg[2]); break;
		case EOpDistance:    if(visit == PostVisit) emit(sw::Shader::OPCODE_DIST(dim(arg[0])), result, arg[0], arg[1]); break;
		case EOpDot:         if(visit == PostVisit) emit(sw::Shader::OPCODE_DP(dim(arg[0])), result, arg[0], arg[1]); break;
		case EOpCross:       if(visit == PostVisit) emit(sw::Shader::OPCODE_CRS, result, arg[0], arg[1]); break;
		case EOpFaceForward: if(visit == PostVisit) emit(sw::Shader::OPCODE_FORWARD(dim(arg[0])), result, arg[0], arg[1], arg[2]); break;
		case EOpReflect:     if(visit == PostVisit) emit(sw::Shader::OPCODE_REFLECT(dim(arg[0])), result, arg[0], arg[1]); break;
		case EOpRefract:     if(visit == PostVisit) emit(sw::Shader::OPCODE_REFRACT(dim(arg[0])), result, arg[0], arg[1], arg[2]); break;
		case EOpMul:
			if(visit == PostVisit)
			{
				TIntermTyped *arg0 = arg[0]->getAsTyped();
				ASSERT((arg0->getNominalSize() == arg[1]->getAsTyped()->getNominalSize()) &&
				       (arg0->getSecondarySize() == arg[1]->getAsTyped()->getSecondarySize()));

				int size = arg0->getNominalSize();
				for(int i = 0; i < size; i++)
				{
					emit(sw::Shader::OPCODE_MUL, result, i, arg[0], i, arg[1], i);
				}
			}
			break;
		case EOpOuterProduct:
			if(visit == PostVisit)
			{
				for(int i = 0; i < dim(arg[1]); i++)
				{
					Instruction *mul = emit(sw::Shader::OPCODE_MUL, result, i, arg[0], 0, arg[1]);
					mul->src[1].swizzle = 0x55 * i;
				}
			}
			break;
		default: UNREACHABLE(node->getOp());
		}

		return true;
	}

	bool OutputASM::visitSelection(Visit visit, TIntermSelection *node)
	{
		if(currentScope != emitScope)
		{
			return false;
		}

		TIntermTyped *condition = node->getCondition();
		TIntermNode *trueBlock = node->getTrueBlock();
		TIntermNode *falseBlock = node->getFalseBlock();
		TIntermConstantUnion *constantCondition = condition->getAsConstantUnion();

		condition->traverse(this);

		if(node->usesTernaryOperator())
		{
			if(constantCondition)
			{
				bool trueCondition = constantCondition->getUnionArrayPointer()->getBConst();

				if(trueCondition)
				{
					trueBlock->traverse(this);
					copy(node, trueBlock);
				}
				else
				{
					falseBlock->traverse(this);
					copy(node, falseBlock);
				}
			}
			else if(trivial(node, 6))   // Fast to compute both potential results and no side effects
			{
				trueBlock->traverse(this);
				falseBlock->traverse(this);
				emit(sw::Shader::OPCODE_SELECT, node, condition, trueBlock, falseBlock);
			}
			else
			{
				emit(sw::Shader::OPCODE_IF, 0, condition);

				if(trueBlock)
				{
					trueBlock->traverse(this);
					copy(node, trueBlock);
				}

				if(falseBlock)
				{
					emit(sw::Shader::OPCODE_ELSE);
					falseBlock->traverse(this);
					copy(node, falseBlock);
				}

				emit(sw::Shader::OPCODE_ENDIF);
			}
		}
		else  // if/else statement
		{
			if(constantCondition)
			{
				bool trueCondition = constantCondition->getUnionArrayPointer()->getBConst();

				if(trueCondition)
				{
					if(trueBlock)
					{
						trueBlock->traverse(this);
					}
				}
				else
				{
					if(falseBlock)
					{
						falseBlock->traverse(this);
					}
				}
			}
			else
			{
				emit(sw::Shader::OPCODE_IF, 0, condition);

				if(trueBlock)
				{
					trueBlock->traverse(this);
				}

				if(falseBlock)
				{
					emit(sw::Shader::OPCODE_ELSE);
					falseBlock->traverse(this);
				}

				emit(sw::Shader::OPCODE_ENDIF);
			}
		}

		return false;
	}

	bool OutputASM::visitLoop(Visit visit, TIntermLoop *node)
	{
		if(currentScope != emitScope)
		{
			return false;
		}

		unsigned int iterations = loopCount(node);

		if(iterations == 0)
		{
			return false;
		}

		bool unroll = (iterations <= 4);

		if(unroll)
		{
			LoopUnrollable loopUnrollable;
			unroll = loopUnrollable.traverse(node);
		}

		TIntermNode *init = node->getInit();
		TIntermTyped *condition = node->getCondition();
		TIntermTyped *expression = node->getExpression();
		TIntermNode *body = node->getBody();
		Constant True(true);

		if(node->getType() == ELoopDoWhile)
		{
			Temporary iterate(this);
			emit(sw::Shader::OPCODE_MOV, &iterate, &True);

			emit(sw::Shader::OPCODE_WHILE, 0, &iterate);   // FIXME: Implement real do-while

			if(body)
			{
				body->traverse(this);
			}

			emit(sw::Shader::OPCODE_TEST);

			condition->traverse(this);
			emit(sw::Shader::OPCODE_MOV, &iterate, condition);

			emit(sw::Shader::OPCODE_ENDWHILE);
		}
		else
		{
			if(init)
			{
				init->traverse(this);
			}

			if(unroll)
			{
				for(unsigned int i = 0; i < iterations; i++)
				{
				//	condition->traverse(this);   // Condition could contain statements, but not in an unrollable loop

					if(body)
					{
						body->traverse(this);
					}

					if(expression)
					{
						expression->traverse(this);
					}
				}
			}
			else
			{
				if(condition)
				{
					condition->traverse(this);
				}
				else
				{
					condition = &True;
				}

				emit(sw::Shader::OPCODE_WHILE, 0, condition);

				if(body)
				{
					body->traverse(this);
				}

				emit(sw::Shader::OPCODE_TEST);

				if(expression)
				{
					expression->traverse(this);
				}

				if(condition)
				{
					condition->traverse(this);
				}

				emit(sw::Shader::OPCODE_ENDWHILE);
			}
		}

		return false;
	}

	bool OutputASM::visitBranch(Visit visit, TIntermBranch *node)
	{
		if(currentScope != emitScope)
		{
			return false;
		}

		switch(node->getFlowOp())
		{
		case EOpKill:      if(visit == PostVisit) emit(sw::Shader::OPCODE_DISCARD);  break;
		case EOpBreak:     if(visit == PostVisit) emit(sw::Shader::OPCODE_BREAK);    break;
		case EOpContinue:  if(visit == PostVisit) emit(sw::Shader::OPCODE_CONTINUE); break;
		case EOpReturn:
			if(visit == PostVisit)
			{
				TIntermTyped *value = node->getExpression();

				if(value)
				{
					copy(functionArray[currentFunction].ret, value);
				}

				emit(sw::Shader::OPCODE_LEAVE);
			}
			break;
		default: UNREACHABLE(node->getFlowOp());
		}

		return true;
	}

	bool OutputASM::visitSwitch(Visit visit, TIntermSwitch *node)
	{
		if(currentScope != emitScope)
		{
			return false;
		}

		TIntermTyped* switchValue = node->getInit();
		TIntermAggregate* opList = node->getStatementList();

		if(!switchValue || !opList)
		{
			return false;
		}

		switchValue->traverse(this);

		emit(sw::Shader::OPCODE_SWITCH);

		TIntermSequence& sequence = opList->getSequence();
		TIntermSequence::iterator it = sequence.begin();
		TIntermSequence::iterator defaultIt = sequence.end();
		int nbCases = 0;
		for(; it != sequence.end(); ++it)
		{
			TIntermCase* currentCase = (*it)->getAsCaseNode();
			if(currentCase)
			{
				TIntermSequence::iterator caseIt = it;

				TIntermTyped* condition = currentCase->getCondition();
				if(condition) // non default case
				{
					if(nbCases != 0)
					{
						emit(sw::Shader::OPCODE_ELSE);
					}

					condition->traverse(this);
					Temporary result(this);
					emitBinary(sw::Shader::OPCODE_EQ, &result, switchValue, condition);
					emit(sw::Shader::OPCODE_IF, 0, &result);
					nbCases++;

					// Emit the code for this case and all subsequent cases until we hit a break statement.
					// TODO: This can repeat a lot of code for switches with many fall-through cases.
					for(++caseIt; caseIt != sequence.end(); ++caseIt)
					{
						(*caseIt)->traverse(this);

						// Stop if we encounter an unconditional branch (break, continue, return, or kill).
						// TODO: This doesn't work if the statement is at a deeper scope level (e.g. {break;}).
						// Note that this eliminates useless operations but shouldn't affect correctness.
						if((*caseIt)->getAsBranchNode())
						{
							break;
						}
					}
				}
				else
				{
					defaultIt = it; // The default case might not be the last case, keep it for last
				}
			}
		}

		// If there's a default case, traverse it here
		if(defaultIt != sequence.end())
		{
			emit(sw::Shader::OPCODE_ELSE);
			for(++defaultIt; defaultIt != sequence.end(); ++defaultIt)
			{
				(*defaultIt)->traverse(this);
				if((*defaultIt)->getAsBranchNode()) // Kill, Break, Continue or Return
				{
					break;
				}
			}
		}

		for(int i = 0; i < nbCases; ++i)
		{
			emit(sw::Shader::OPCODE_ENDIF);
		}

		emit(sw::Shader::OPCODE_ENDSWITCH);

		return false;
	}

	Instruction *OutputASM::emit(sw::Shader::Opcode op, TIntermTyped *dst, TIntermNode *src0, TIntermNode *src1, TIntermNode *src2, TIntermNode *src3, TIntermNode *src4)
	{
		return emit(op, dst, 0, src0, 0, src1, 0, src2, 0, src3, 0, src4, 0);
	}

	Instruction *OutputASM::emit(sw::Shader::Opcode op, TIntermTyped *dst, int dstIndex, TIntermNode *src0, int index0, TIntermNode *src1, int index1,
	                             TIntermNode *src2, int index2, TIntermNode *src3, int index3, TIntermNode *src4, int index4)
	{
		Instruction *instruction = new Instruction(op);

		if(dst)
		{
			destination(instruction->dst, dst, dstIndex);
		}

		if(src0)
		{
			TIntermTyped* src = src0->getAsTyped();
			instruction->dst.partialPrecision = src && (src->getPrecision() <= EbpLow);
		}

		source(instruction->src[0], src0, index0);
		source(instruction->src[1], src1, index1);
		source(instruction->src[2], src2, index2);
		source(instruction->src[3], src3, index3);
		source(instruction->src[4], src4, index4);

		shader->append(instruction);

		return instruction;
	}

	Instruction *OutputASM::emitCast(TIntermTyped *dst, TIntermTyped *src)
	{
		return emitCast(dst, 0, src, 0);
	}

	Instruction *OutputASM::emitCast(TIntermTyped *dst, int dstIndex, TIntermTyped *src, int srcIndex)
	{
		switch(src->getBasicType())
		{
		case EbtBool:
			switch(dst->getBasicType())
			{
			case EbtInt:   return emit(sw::Shader::OPCODE_B2I, dst, dstIndex, src, srcIndex);
			case EbtUInt:  return emit(sw::Shader::OPCODE_B2I, dst, dstIndex, src, srcIndex);
			case EbtFloat: return emit(sw::Shader::OPCODE_B2F, dst, dstIndex, src, srcIndex);
			default:       break;
			}
			break;
		case EbtInt:
			switch(dst->getBasicType())
			{
			case EbtBool:  return emit(sw::Shader::OPCODE_I2B, dst, dstIndex, src, srcIndex);
			case EbtFloat: return emit(sw::Shader::OPCODE_I2F, dst, dstIndex, src, srcIndex);
			default:       break;
			}
			break;
		case EbtUInt:
			switch(dst->getBasicType())
			{
			case EbtBool:  return emit(sw::Shader::OPCODE_I2B, dst, dstIndex, src, srcIndex);
			case EbtFloat: return emit(sw::Shader::OPCODE_U2F, dst, dstIndex, src, srcIndex);
			default:       break;
			}
			break;
		case EbtFloat:
			switch(dst->getBasicType())
			{
			case EbtBool: return emit(sw::Shader::OPCODE_F2B, dst, dstIndex, src, srcIndex);
			case EbtInt:  return emit(sw::Shader::OPCODE_F2I, dst, dstIndex, src, srcIndex);
			case EbtUInt: return emit(sw::Shader::OPCODE_F2U, dst, dstIndex, src, srcIndex);
			default:      break;
			}
			break;
		default:
			break;
		}

		ASSERT((src->getBasicType() == dst->getBasicType()) ||
		      ((src->getBasicType() == EbtInt) && (dst->getBasicType() == EbtUInt)) ||
		      ((src->getBasicType() == EbtUInt) && (dst->getBasicType() == EbtInt)));

		return emit(sw::Shader::OPCODE_MOV, dst, dstIndex, src, srcIndex);
	}

	void OutputASM::emitBinary(sw::Shader::Opcode op, TIntermTyped *dst, TIntermNode *src0, TIntermNode *src1, TIntermNode *src2)
	{
		for(int index = 0; index < dst->elementRegisterCount(); index++)
		{
			emit(op, dst, index, src0, index, src1, index, src2, index);
		}
	}

	void OutputASM::emitAssign(sw::Shader::Opcode op, TIntermTyped *result, TIntermTyped *lhs, TIntermTyped *src0, TIntermTyped *src1)
	{
		emitBinary(op, result, src0, src1);
		assignLvalue(lhs, result);
	}

	void OutputASM::emitCmp(sw::Shader::Control cmpOp, TIntermTyped *dst, TIntermNode *left, TIntermNode *right, int index)
	{
		sw::Shader::Opcode opcode;
		switch(left->getAsTyped()->getBasicType())
		{
		case EbtBool:
		case EbtInt:
			opcode = sw::Shader::OPCODE_ICMP;
			break;
		case EbtUInt:
			opcode = sw::Shader::OPCODE_UCMP;
			break;
		default:
			opcode = sw::Shader::OPCODE_CMP;
			break;
		}

		Instruction *cmp = emit(opcode, dst, 0, left, index, right, index);
		cmp->control = cmpOp;
	}

	int componentCount(const TType &type, int registers)
	{
		if(registers == 0)
		{
			return 0;
		}

		if(type.isArray() && registers >= type.elementRegisterCount())
		{
			int index = registers / type.elementRegisterCount();
			registers -= index * type.elementRegisterCount();
			return index * type.getElementSize() + componentCount(type, registers);
		}

		if(type.isStruct() || type.isInterfaceBlock())
		{
			const TFieldList& fields = type.getStruct() ? type.getStruct()->fields() : type.getInterfaceBlock()->fields();
			int elements = 0;

			for(const auto &field : fields)
			{
				const TType &fieldType = *(field->type());

				if(fieldType.totalRegisterCount() <= registers)
				{
					registers -= fieldType.totalRegisterCount();
					elements += fieldType.getObjectSize();
				}
				else   // Register within this field
				{
					return elements + componentCount(fieldType, registers);
				}
			}
		}
		else if(type.isMatrix())
		{
			return registers * type.registerSize();
		}

		UNREACHABLE(0);
		return 0;
	}

	int registerSize(const TType &type, int registers)
	{
		if(registers == 0)
		{
			if(type.isStruct())
			{
				return registerSize(*((*(type.getStruct()->fields().begin()))->type()), 0);
			}
			else if(type.isInterfaceBlock())
			{
				return registerSize(*((*(type.getInterfaceBlock()->fields().begin()))->type()), 0);
			}

			return type.registerSize();
		}

		if(type.isArray() && registers >= type.elementRegisterCount())
		{
			int index = registers / type.elementRegisterCount();
			registers -= index * type.elementRegisterCount();
			return registerSize(type, registers);
		}

		if(type.isStruct() || type.isInterfaceBlock())
		{
			const TFieldList& fields = type.getStruct() ? type.getStruct()->fields() : type.getInterfaceBlock()->fields();
			int elements = 0;

			for(const auto &field : fields)
			{
				const TType &fieldType = *(field->type());

				if(fieldType.totalRegisterCount() <= registers)
				{
					registers -= fieldType.totalRegisterCount();
					elements += fieldType.getObjectSize();
				}
				else   // Register within this field
				{
					return registerSize(fieldType, registers);
				}
			}
		}
		else if(type.isMatrix())
		{
			return registerSize(type, 0);
		}

		UNREACHABLE(0);
		return 0;
	}

	int OutputASM::getBlockId(TIntermTyped *arg)
	{
		if(arg)
		{
			const TType &type = arg->getType();
			TInterfaceBlock* block = type.getInterfaceBlock();
			if(block && (type.getQualifier() == EvqUniform))
			{
				// Make sure the uniform block is declared
				uniformRegister(arg);

				const char* blockName = block->name().c_str();

				// Fetch uniform block index from array of blocks
				for(ActiveUniformBlocks::const_iterator it = shaderObject->activeUniformBlocks.begin(); it != shaderObject->activeUniformBlocks.end(); ++it)
				{
					if(blockName == it->name)
					{
						return it->blockId;
					}
				}

				ASSERT(false);
			}
		}

		return -1;
	}

	OutputASM::ArgumentInfo OutputASM::getArgumentInfo(TIntermTyped *arg, int index)
	{
		const TType &type = arg->getType();
		int blockId = getBlockId(arg);
		ArgumentInfo argumentInfo(BlockMemberInfo::getDefaultBlockInfo(), type, -1, -1);
		if(blockId != -1)
		{
			argumentInfo.bufferIndex = 0;
			for(int i = 0; i < blockId; ++i)
			{
				int blockArraySize = shaderObject->activeUniformBlocks[i].arraySize;
				argumentInfo.bufferIndex += blockArraySize > 0 ? blockArraySize : 1;
			}

			const BlockDefinitionIndexMap& blockDefinition = blockDefinitions[blockId];

			BlockDefinitionIndexMap::const_iterator itEnd = blockDefinition.end();
			BlockDefinitionIndexMap::const_iterator it = itEnd;

			argumentInfo.clampedIndex = index;
			if(type.isInterfaceBlock())
			{
				// Offset index to the beginning of the selected instance
				int blockRegisters = type.elementRegisterCount();
				int bufferOffset = argumentInfo.clampedIndex / blockRegisters;
				argumentInfo.bufferIndex += bufferOffset;
				argumentInfo.clampedIndex -= bufferOffset * blockRegisters;
			}

			int regIndex = registerIndex(arg);
			for(int i = regIndex + argumentInfo.clampedIndex; i >= regIndex; --i)
			{
				it = blockDefinition.find(i);
				if(it != itEnd)
				{
					argumentInfo.clampedIndex -= (i - regIndex);
					break;
				}
			}
			ASSERT(it != itEnd);

			argumentInfo.typedMemberInfo = it->second;

			int registerCount = argumentInfo.typedMemberInfo.type.totalRegisterCount();
			argumentInfo.clampedIndex = (argumentInfo.clampedIndex >= registerCount) ? registerCount - 1 : argumentInfo.clampedIndex;
		}
		else
		{
			argumentInfo.clampedIndex = (index >= arg->totalRegisterCount()) ? arg->totalRegisterCount() - 1 : index;
		}

		return argumentInfo;
	}

	void OutputASM::source(sw::Shader::SourceParameter &parameter, TIntermNode *argument, int index)
	{
		if(argument)
		{
			TIntermTyped *arg = argument->getAsTyped();
			Temporary unpackedUniform(this);

			const TType& srcType = arg->getType();
			TInterfaceBlock* srcBlock = srcType.getInterfaceBlock();
			if(srcBlock && (srcType.getQualifier() == EvqUniform))
			{
				const ArgumentInfo argumentInfo = getArgumentInfo(arg, index);
				const TType &memberType = argumentInfo.typedMemberInfo.type;

				if(memberType.getBasicType() == EbtBool)
				{
					ASSERT(argumentInfo.clampedIndex < (memberType.isArray() ? memberType.getArraySize() : 1)); // index < arraySize

					// Convert the packed bool, which is currently an int, to a true bool
					Instruction *instruction = new Instruction(sw::Shader::OPCODE_I2B);
					instruction->dst.type = sw::Shader::PARAMETER_TEMP;
					instruction->dst.index = registerIndex(&unpackedUniform);
					instruction->src[0].type = sw::Shader::PARAMETER_CONST;
					instruction->src[0].bufferIndex = argumentInfo.bufferIndex;
					instruction->src[0].index = argumentInfo.typedMemberInfo.offset + argumentInfo.clampedIndex * argumentInfo.typedMemberInfo.arrayStride;

					shader->append(instruction);

					arg = &unpackedUniform;
					index = 0;
				}
				else if((memberType.getLayoutQualifier().matrixPacking == EmpRowMajor) && memberType.isMatrix())
				{
					int numCols = memberType.getNominalSize();
					int numRows = memberType.getSecondarySize();

					ASSERT(argumentInfo.clampedIndex < (numCols * (memberType.isArray() ? memberType.getArraySize() : 1))); // index < cols * arraySize

					unsigned int dstIndex = registerIndex(&unpackedUniform);
					unsigned int srcSwizzle = (argumentInfo.clampedIndex % numCols) * 0x55;
					int arrayIndex = argumentInfo.clampedIndex / numCols;
					int matrixStartOffset = argumentInfo.typedMemberInfo.offset + arrayIndex * argumentInfo.typedMemberInfo.arrayStride;

					for(int j = 0; j < numRows; ++j)
					{
						// Transpose the row major matrix
						Instruction *instruction = new Instruction(sw::Shader::OPCODE_MOV);
						instruction->dst.type = sw::Shader::PARAMETER_TEMP;
						instruction->dst.index = dstIndex;
						instruction->dst.mask = 1 << j;
						instruction->src[0].type = sw::Shader::PARAMETER_CONST;
						instruction->src[0].bufferIndex = argumentInfo.bufferIndex;
						instruction->src[0].index = matrixStartOffset + j * argumentInfo.typedMemberInfo.matrixStride;
						instruction->src[0].swizzle = srcSwizzle;

						shader->append(instruction);
					}

					arg = &unpackedUniform;
					index = 0;
				}
			}

			const ArgumentInfo argumentInfo = getArgumentInfo(arg, index);
			const TType &type = argumentInfo.typedMemberInfo.type;

			int size = registerSize(type, argumentInfo.clampedIndex);

			parameter.type = registerType(arg);
			parameter.bufferIndex = argumentInfo.bufferIndex;

			if(arg->getAsConstantUnion() && arg->getAsConstantUnion()->getUnionArrayPointer())
			{
				int component = componentCount(type, argumentInfo.clampedIndex);
				ConstantUnion *constants = arg->getAsConstantUnion()->getUnionArrayPointer();

				for(int i = 0; i < 4; i++)
				{
					if(size == 1)   // Replicate
					{
						parameter.value[i] = constants[component + 0].getAsFloat();
					}
					else if(i < size)
					{
						parameter.value[i] = constants[component + i].getAsFloat();
					}
					else
					{
						parameter.value[i] = 0.0f;
					}
				}
			}
			else
			{
				parameter.index = registerIndex(arg) + argumentInfo.clampedIndex;

				if(parameter.bufferIndex != -1)
				{
					int stride = (argumentInfo.typedMemberInfo.matrixStride > 0) ? argumentInfo.typedMemberInfo.matrixStride : argumentInfo.typedMemberInfo.arrayStride;
					parameter.index = argumentInfo.typedMemberInfo.offset + argumentInfo.clampedIndex * stride;
				}
			}

			if(!IsSampler(arg->getBasicType()))
			{
				parameter.swizzle = readSwizzle(arg, size);
			}
		}
	}

	void OutputASM::destination(sw::Shader::DestinationParameter &parameter, TIntermTyped *arg, int index)
	{
		parameter.type = registerType(arg);
		parameter.index = registerIndex(arg) + index;
		parameter.mask = writeMask(arg, index);
	}

	void OutputASM::copy(TIntermTyped *dst, TIntermNode *src, int offset)
	{
		for(int index = 0; index < dst->totalRegisterCount(); index++)
		{
			Instruction *mov = emit(sw::Shader::OPCODE_MOV, dst, index, src, offset + index);
		}
	}

	int swizzleElement(int swizzle, int index)
	{
		return (swizzle >> (index * 2)) & 0x03;
	}

	int swizzleSwizzle(int leftSwizzle, int rightSwizzle)
	{
		return (swizzleElement(leftSwizzle, swizzleElement(rightSwizzle, 0)) << 0) |
		       (swizzleElement(leftSwizzle, swizzleElement(rightSwizzle, 1)) << 2) |
		       (swizzleElement(leftSwizzle, swizzleElement(rightSwizzle, 2)) << 4) |
		       (swizzleElement(leftSwizzle, swizzleElement(rightSwizzle, 3)) << 6);
	}

	void OutputASM::assignLvalue(TIntermTyped *dst, TIntermTyped *src)
	{
		if((src->isVector() && (!dst->isVector() || (src->getNominalSize() != dst->getNominalSize()))) ||
		   (src->isMatrix() && (!dst->isMatrix() || (src->getNominalSize() != dst->getNominalSize()) || (src->getSecondarySize() != dst->getSecondarySize()))))
		{
			return mContext.error(src->getLine(), "Result type should match the l-value type in compound assignment", src->isVector() ? "vector" : "matrix");
		}

		TIntermBinary *binary = dst->getAsBinaryNode();

		if(binary && binary->getOp() == EOpIndexIndirect && binary->getLeft()->isVector() && dst->isScalar())
		{
			Instruction *insert = new Instruction(sw::Shader::OPCODE_INSERT);

			lvalue(insert->dst, dst);

			insert->src[0].type = insert->dst.type;
			insert->src[0].index = insert->dst.index;
			insert->src[0].rel = insert->dst.rel;
			source(insert->src[1], src);
			source(insert->src[2], binary->getRight());

			shader->append(insert);
		}
		else
		{
			Instruction *mov1 = new Instruction(sw::Shader::OPCODE_MOV);

			int swizzle = lvalue(mov1->dst, dst);

			source(mov1->src[0], src);
			mov1->src[0].swizzle = swizzleSwizzle(mov1->src[0].swizzle, swizzle);

			shader->append(mov1);

			for(int offset = 1; offset < dst->totalRegisterCount(); offset++)
			{
				Instruction *mov = new Instruction(sw::Shader::OPCODE_MOV);

				mov->dst = mov1->dst;
				mov->dst.index += offset;
				mov->dst.mask = writeMask(dst, offset);

				source(mov->src[0], src, offset);

				shader->append(mov);
			}
		}
	}

	void OutputASM::evaluateRvalue(TIntermTyped *node)
	{
		TIntermBinary *binary = node->getAsBinaryNode();

		if(binary && binary->getOp() == EOpIndexIndirect && binary->getLeft()->isVector() && node->isScalar())
		{
			Instruction *insert = new Instruction(sw::Shader::OPCODE_EXTRACT);

			destination(insert->dst, node);

			Temporary address(this);
			unsigned char mask;
			TIntermTyped *root = nullptr;
			unsigned int offset = 0;
			int swizzle = lvalue(root, offset, insert->src[0].rel, mask, address, node);

			source(insert->src[0], root, offset);
			insert->src[0].swizzle = swizzleSwizzle(insert->src[0].swizzle, swizzle);

			source(insert->src[1], binary->getRight());

			shader->append(insert);
		}
		else
		{
			Instruction *mov1 = new Instruction(sw::Shader::OPCODE_MOV);

			destination(mov1->dst, node, 0);

			Temporary address(this);
			unsigned char mask;
			TIntermTyped *root = nullptr;
			unsigned int offset = 0;
			int swizzle = lvalue(root, offset, mov1->src[0].rel, mask, address, node);

			source(mov1->src[0], root, offset);
			mov1->src[0].swizzle = swizzleSwizzle(mov1->src[0].swizzle, swizzle);

			shader->append(mov1);

			for(int i = 1; i < node->totalRegisterCount(); i++)
			{
				Instruction *mov = emit(sw::Shader::OPCODE_MOV, node, i, root, offset + i);
				mov->src[0].rel = mov1->src[0].rel;
			}
		}
	}

	int OutputASM::lvalue(sw::Shader::DestinationParameter &dst, TIntermTyped *node)
	{
		Temporary address(this);
		TIntermTyped *root = nullptr;
		unsigned int offset = 0;
		unsigned char mask = 0xF;
		int swizzle = lvalue(root, offset, dst.rel, mask, address, node);

		dst.type = registerType(root);
		dst.index = registerIndex(root) + offset;
		dst.mask = mask;

		return swizzle;
	}

	int OutputASM::lvalue(TIntermTyped *&root, unsigned int &offset, sw::Shader::Relative &rel, unsigned char &mask, Temporary &address, TIntermTyped *node)
	{
		TIntermTyped *result = node;
		TIntermBinary *binary = node->getAsBinaryNode();
		TIntermSymbol *symbol = node->getAsSymbolNode();

		if(binary)
		{
			TIntermTyped *left = binary->getLeft();
			TIntermTyped *right = binary->getRight();

			int leftSwizzle = lvalue(root, offset, rel, mask, address, left);   // Resolve the l-value of the left side

			switch(binary->getOp())
			{
			case EOpIndexDirect:
				{
					int rightIndex = right->getAsConstantUnion()->getIConst(0);

					if(left->isRegister())
					{
						int leftMask = mask;

						mask = 1;
						while((leftMask & mask) == 0)
						{
							mask = mask << 1;
						}

						int element = swizzleElement(leftSwizzle, rightIndex);
						mask = 1 << element;

						return element;
					}
					else if(left->isArray() || left->isMatrix())
					{
						offset += rightIndex * result->totalRegisterCount();
						return 0xE4;
					}
					else UNREACHABLE(0);
				}
				break;
			case EOpIndexIndirect:
				{
					right->traverse(this);

					if(left->isRegister())
					{
						// Requires INSERT instruction (handled by calling function)
					}
					else if(left->isArray() || left->isMatrix())
					{
						int scale = result->totalRegisterCount();

						if(rel.type == sw::Shader::PARAMETER_VOID)   // Use the index register as the relative address directly
						{
							if(left->totalRegisterCount() > 1)
							{
								sw::Shader::SourceParameter relativeRegister;
								source(relativeRegister, right);

								rel.index = relativeRegister.index;
								rel.type = relativeRegister.type;
								rel.scale = scale;
								rel.deterministic = !(vertexShader && left->getQualifier() == EvqUniform);
							}
						}
						else if(rel.index != registerIndex(&address))   // Move the previous index register to the address register
						{
							if(scale == 1)
							{
								Constant oldScale((int)rel.scale);
								Instruction *mad = emit(sw::Shader::OPCODE_IMAD, &address, &address, &oldScale, right);
								mad->src[0].index = rel.index;
								mad->src[0].type = rel.type;
							}
							else
							{
								Constant oldScale((int)rel.scale);
								Instruction *mul = emit(sw::Shader::OPCODE_IMUL, &address, &address, &oldScale);
								mul->src[0].index = rel.index;
								mul->src[0].type = rel.type;

								Constant newScale(scale);
								emit(sw::Shader::OPCODE_IMAD, &address, right, &newScale, &address);
							}

							rel.type = sw::Shader::PARAMETER_TEMP;
							rel.index = registerIndex(&address);
							rel.scale = 1;
						}
						else   // Just add the new index to the address register
						{
							if(scale == 1)
							{
								emit(sw::Shader::OPCODE_IADD, &address, &address, right);
							}
							else
							{
								Constant newScale(scale);
								emit(sw::Shader::OPCODE_IMAD, &address, right, &newScale, &address);
							}
						}
					}
					else UNREACHABLE(0);
				}
				break;
			case EOpIndexDirectStruct:
			case EOpIndexDirectInterfaceBlock:
				{
					const TFieldList& fields = (binary->getOp() == EOpIndexDirectStruct) ?
					                           left->getType().getStruct()->fields() :
					                           left->getType().getInterfaceBlock()->fields();
					int index = right->getAsConstantUnion()->getIConst(0);
					int fieldOffset = 0;

					for(int i = 0; i < index; i++)
					{
						fieldOffset += fields[i]->type()->totalRegisterCount();
					}

					offset += fieldOffset;
					mask = writeMask(result);

					return 0xE4;
				}
				break;
			case EOpVectorSwizzle:
				{
					ASSERT(left->isRegister());

					int leftMask = mask;

					int swizzle = 0;
					int rightMask = 0;

					TIntermSequence &sequence = right->getAsAggregate()->getSequence();

					for(unsigned int i = 0; i < sequence.size(); i++)
					{
						int index = sequence[i]->getAsConstantUnion()->getIConst(0);

						int element = swizzleElement(leftSwizzle, index);
						rightMask = rightMask | (1 << element);
						swizzle = swizzle | swizzleElement(leftSwizzle, i) << (element * 2);
					}

					mask = leftMask & rightMask;

					return swizzle;
				}
				break;
			default:
				UNREACHABLE(binary->getOp());   // Not an l-value operator
				break;
			}
		}
		else if(symbol)
		{
			root = symbol;
			offset = 0;
			mask = writeMask(symbol);

			return 0xE4;
		}
		else
		{
			node->traverse(this);

			root = node;
			offset = 0;
			mask = writeMask(node);

			return 0xE4;
		}

		return 0xE4;
	}

	sw::Shader::ParameterType OutputASM::registerType(TIntermTyped *operand)
	{
		if(isSamplerRegister(operand))
		{
			return sw::Shader::PARAMETER_SAMPLER;
		}

		const TQualifier qualifier = operand->getQualifier();
		if((qualifier == EvqFragColor) || (qualifier == EvqFragData))
		{
			if(((qualifier == EvqFragData) && (outputQualifier == EvqFragColor)) ||
			   ((qualifier == EvqFragColor) && (outputQualifier == EvqFragData)))
			{
				mContext.error(operand->getLine(), "static assignment to both gl_FragData and gl_FragColor", "");
			}
			outputQualifier = qualifier;
		}

		if(qualifier == EvqConstExpr && (!operand->getAsConstantUnion() || !operand->getAsConstantUnion()->getUnionArrayPointer()))
		{
			// Constant arrays are in the constant register file.
			if(operand->isArray() && operand->getArraySize() > 1)
			{
				return sw::Shader::PARAMETER_CONST;
			}
			else
			{
				return sw::Shader::PARAMETER_TEMP;
			}
		}

		switch(qualifier)
		{
		case EvqTemporary:           return sw::Shader::PARAMETER_TEMP;
		case EvqGlobal:              return sw::Shader::PARAMETER_TEMP;
		case EvqConstExpr:           return sw::Shader::PARAMETER_FLOAT4LITERAL;   // All converted to float
		case EvqAttribute:           return sw::Shader::PARAMETER_INPUT;
		case EvqVaryingIn:           return sw::Shader::PARAMETER_INPUT;
		case EvqVaryingOut:          return sw::Shader::PARAMETER_OUTPUT;
		case EvqVertexIn:            return sw::Shader::PARAMETER_INPUT;
		case EvqFragmentOut:         return sw::Shader::PARAMETER_COLOROUT;
		case EvqVertexOut:           return sw::Shader::PARAMETER_OUTPUT;
		case EvqFragmentIn:          return sw::Shader::PARAMETER_INPUT;
		case EvqInvariantVaryingIn:  return sw::Shader::PARAMETER_INPUT;    // FIXME: Guarantee invariance at the backend
		case EvqInvariantVaryingOut: return sw::Shader::PARAMETER_OUTPUT;   // FIXME: Guarantee invariance at the backend
		case EvqSmooth:              return sw::Shader::PARAMETER_OUTPUT;
		case EvqFlat:                return sw::Shader::PARAMETER_OUTPUT;
		case EvqCentroidOut:         return sw::Shader::PARAMETER_OUTPUT;
		case EvqSmoothIn:            return sw::Shader::PARAMETER_INPUT;
		case EvqFlatIn:              return sw::Shader::PARAMETER_INPUT;
		case EvqCentroidIn:          return sw::Shader::PARAMETER_INPUT;
		case EvqUniform:             return sw::Shader::PARAMETER_CONST;
		case EvqIn:                  return sw::Shader::PARAMETER_TEMP;
		case EvqOut:                 return sw::Shader::PARAMETER_TEMP;
		case EvqInOut:               return sw::Shader::PARAMETER_TEMP;
		case EvqConstReadOnly:       return sw::Shader::PARAMETER_TEMP;
		case EvqPosition:            return sw::Shader::PARAMETER_OUTPUT;
		case EvqPointSize:           return sw::Shader::PARAMETER_OUTPUT;
		case EvqInstanceID:          return sw::Shader::PARAMETER_MISCTYPE;
		case EvqVertexID:            return sw::Shader::PARAMETER_MISCTYPE;
		case EvqFragCoord:           return sw::Shader::PARAMETER_MISCTYPE;
		case EvqFrontFacing:         return sw::Shader::PARAMETER_MISCTYPE;
		case EvqPointCoord:          return sw::Shader::PARAMETER_INPUT;
		case EvqFragColor:           return sw::Shader::PARAMETER_COLOROUT;
		case EvqFragData:            return sw::Shader::PARAMETER_COLOROUT;
		case EvqFragDepth:           return sw::Shader::PARAMETER_DEPTHOUT;
		default: UNREACHABLE(qualifier);
		}

		return sw::Shader::PARAMETER_VOID;
	}

	bool OutputASM::hasFlatQualifier(TIntermTyped *operand)
	{
		const TQualifier qualifier = operand->getQualifier();
		return qualifier == EvqFlat || qualifier == EvqFlatOut || qualifier == EvqFlatIn;
	}

	unsigned int OutputASM::registerIndex(TIntermTyped *operand)
	{
		if(isSamplerRegister(operand))
		{
			return samplerRegister(operand);
		}
		else if(operand->getType().totalSamplerRegisterCount() > 0) // Struct containing a sampler
		{
			samplerRegister(operand); // Make sure the sampler is declared
		}

		switch(operand->getQualifier())
		{
		case EvqTemporary:           return temporaryRegister(operand);
		case EvqGlobal:              return temporaryRegister(operand);
		case EvqConstExpr:           return temporaryRegister(operand);   // Unevaluated constant expression
		case EvqAttribute:           return attributeRegister(operand);
		case EvqVaryingIn:           return varyingRegister(operand);
		case EvqVaryingOut:          return varyingRegister(operand);
		case EvqVertexIn:            return attributeRegister(operand);
		case EvqFragmentOut:         return fragmentOutputRegister(operand);
		case EvqVertexOut:           return varyingRegister(operand);
		case EvqFragmentIn:          return varyingRegister(operand);
		case EvqInvariantVaryingIn:  return varyingRegister(operand);
		case EvqInvariantVaryingOut: return varyingRegister(operand);
		case EvqSmooth:              return varyingRegister(operand);
		case EvqFlat:                return varyingRegister(operand);
		case EvqCentroidOut:         return varyingRegister(operand);
		case EvqSmoothIn:            return varyingRegister(operand);
		case EvqFlatIn:              return varyingRegister(operand);
		case EvqCentroidIn:          return varyingRegister(operand);
		case EvqUniform:             return uniformRegister(operand);
		case EvqIn:                  return temporaryRegister(operand);
		case EvqOut:                 return temporaryRegister(operand);
		case EvqInOut:               return temporaryRegister(operand);
		case EvqConstReadOnly:       return temporaryRegister(operand);
		case EvqPosition:            return varyingRegister(operand);
		case EvqPointSize:           return varyingRegister(operand);
		case EvqInstanceID:          vertexShader->declareInstanceId(); return sw::Shader::InstanceIDIndex;
		case EvqVertexID:            vertexShader->declareVertexId(); return sw::Shader::VertexIDIndex;
		case EvqFragCoord:           pixelShader->declareVPos();  return sw::Shader::VPosIndex;
		case EvqFrontFacing:         pixelShader->declareVFace(); return sw::Shader::VFaceIndex;
		case EvqPointCoord:          return varyingRegister(operand);
		case EvqFragColor:           return 0;
		case EvqFragData:            return fragmentOutputRegister(operand);
		case EvqFragDepth:           return 0;
		default: UNREACHABLE(operand->getQualifier());
		}

		return 0;
	}

	int OutputASM::writeMask(TIntermTyped *destination, int index)
	{
		if(destination->getQualifier() == EvqPointSize)
		{
			return 0x2;   // Point size stored in the y component
		}

		return 0xF >> (4 - registerSize(destination->getType(), index));
	}

	int OutputASM::readSwizzle(TIntermTyped *argument, int size)
	{
		if(argument->getQualifier() == EvqPointSize)
		{
			return 0x55;   // Point size stored in the y component
		}

		static const unsigned char swizzleSize[5] = {0x00, 0x00, 0x54, 0xA4, 0xE4};   // (void), xxxx, xyyy, xyzz, xyzw

		return swizzleSize[size];
	}

	// Conservatively checks whether an expression is fast to compute and has no side effects
	bool OutputASM::trivial(TIntermTyped *expression, int budget)
	{
		if(!expression->isRegister())
		{
			return false;
		}

		return cost(expression, budget) >= 0;
	}

	// Returns the remaining computing budget (if < 0 the expression is too expensive or has side effects)
	int OutputASM::cost(TIntermNode *expression, int budget)
	{
		if(budget < 0)
		{
			return budget;
		}

		if(expression->getAsSymbolNode())
		{
			return budget;
		}
		else if(expression->getAsConstantUnion())
		{
			return budget;
		}
		else if(expression->getAsBinaryNode())
		{
			TIntermBinary *binary = expression->getAsBinaryNode();

			switch(binary->getOp())
			{
			case EOpVectorSwizzle:
			case EOpIndexDirect:
			case EOpIndexDirectStruct:
			case EOpIndexDirectInterfaceBlock:
				return cost(binary->getLeft(), budget - 0);
			case EOpAdd:
			case EOpSub:
			case EOpMul:
				return cost(binary->getLeft(), cost(binary->getRight(), budget - 1));
			default:
				return -1;
			}
		}
		else if(expression->getAsUnaryNode())
		{
			TIntermUnary *unary = expression->getAsUnaryNode();

			switch(unary->getOp())
			{
			case EOpAbs:
			case EOpNegative:
				return cost(unary->getOperand(), budget - 1);
			default:
				return -1;
			}
		}
		else if(expression->getAsSelectionNode())
		{
			TIntermSelection *selection = expression->getAsSelectionNode();

			if(selection->usesTernaryOperator())
			{
				TIntermTyped *condition = selection->getCondition();
				TIntermNode *trueBlock = selection->getTrueBlock();
				TIntermNode *falseBlock = selection->getFalseBlock();
				TIntermConstantUnion *constantCondition = condition->getAsConstantUnion();

				if(constantCondition)
				{
					bool trueCondition = constantCondition->getUnionArrayPointer()->getBConst();

					if(trueCondition)
					{
						return cost(trueBlock, budget - 0);
					}
					else
					{
						return cost(falseBlock, budget - 0);
					}
				}
				else
				{
					return cost(trueBlock, cost(falseBlock, budget - 2));
				}
			}
		}

		return -1;
	}

	const Function *OutputASM::findFunction(const TString &name)
	{
		for(unsigned int f = 0; f < functionArray.size(); f++)
		{
			if(functionArray[f].name == name)
			{
				return &functionArray[f];
			}
		}

		return 0;
	}

	int OutputASM::temporaryRegister(TIntermTyped *temporary)
	{
		int index = allocate(temporaries, temporary);
		if(index >= sw::NUM_TEMPORARY_REGISTERS)
		{
			mContext.error(temporary->getLine(),
				"Too many temporary registers required to compile shader",
				pixelShader ? "pixel shader" : "vertex shader");
		}
		return index;
	}

	void OutputASM::setPixelShaderInputs(const TType& type, int var, bool flat)
	{
		if(type.isStruct())
		{
			const TFieldList &fields = type.getStruct()->fields();
			int fieldVar = var;
			for(const auto &field : fields)
			{
				const TType& fieldType = *(field->type());
				setPixelShaderInputs(fieldType, fieldVar, flat);
				fieldVar += fieldType.totalRegisterCount();
			}
		}
		else
		{
			for(int i = 0; i < type.totalRegisterCount(); i++)
			{
				pixelShader->setInput(var + i, type.registerSize(), sw::Shader::Semantic(sw::Shader::USAGE_COLOR, var + i, flat));
			}
		}
	}

	int OutputASM::varyingRegister(TIntermTyped *varying)
	{
		int var = lookup(varyings, varying);

		if(var == -1)
		{
			var = allocate(varyings, varying);
			int registerCount = varying->totalRegisterCount();

			if(pixelShader)
			{
				if((var + registerCount) > sw::MAX_FRAGMENT_INPUTS)
				{
					mContext.error(varying->getLine(), "Varyings packing failed: Too many varyings", "fragment shader");
					return 0;
				}

				if(varying->getQualifier() == EvqPointCoord)
				{
					ASSERT(varying->isRegister());
					pixelShader->setInput(var, varying->registerSize(), sw::Shader::Semantic(sw::Shader::USAGE_TEXCOORD, var));
				}
				else
				{
					setPixelShaderInputs(varying->getType(), var, hasFlatQualifier(varying));
				}
			}
			else if(vertexShader)
			{
				if((var + registerCount) > sw::MAX_VERTEX_OUTPUTS)
				{
					mContext.error(varying->getLine(), "Varyings packing failed: Too many varyings", "vertex shader");
					return 0;
				}

				if(varying->getQualifier() == EvqPosition)
				{
					ASSERT(varying->isRegister());
					vertexShader->setPositionRegister(var);
				}
				else if(varying->getQualifier() == EvqPointSize)
				{
					ASSERT(varying->isRegister());
					vertexShader->setPointSizeRegister(var);
				}
				else
				{
					// Semantic indexes for user varyings will be assigned during program link to match the pixel shader
				}
			}
			else UNREACHABLE(0);

			declareVarying(varying, var);
		}

		return var;
	}

	void OutputASM::declareVarying(TIntermTyped *varying, int reg)
	{
		if(varying->getQualifier() != EvqPointCoord)   // gl_PointCoord does not need linking
		{
			TIntermSymbol *symbol = varying->getAsSymbolNode();
			declareVarying(varying->getType(), symbol->getSymbol(), reg);
		}
	}

	void OutputASM::declareVarying(const TType &type, const TString &varyingName, int registerIndex)
	{
		const char *name = varyingName.c_str();
		VaryingList &activeVaryings = shaderObject->varyings;

		TStructure* structure = type.getStruct();
		if(structure)
		{
			int fieldRegisterIndex = registerIndex;

			const TFieldList &fields = type.getStruct()->fields();
			for(const auto &field : fields)
			{
				const TType& fieldType = *(field->type());
				declareVarying(fieldType, varyingName + "." + field->name(), fieldRegisterIndex);
				if(fieldRegisterIndex >= 0)
				{
					fieldRegisterIndex += fieldType.totalRegisterCount();
				}
			}
		}
		else
		{
			// Check if this varying has been declared before without having a register assigned
			for(VaryingList::iterator v = activeVaryings.begin(); v != activeVaryings.end(); v++)
			{
				if(v->name == name)
				{
					if(registerIndex >= 0)
					{
						ASSERT(v->registerIndex < 0 || v->registerIndex == registerIndex);
						v->registerIndex = registerIndex;
					}

					return;
				}
			}

			activeVaryings.push_back(glsl::Varying(type, name, registerIndex, 0));
		}
	}

	void OutputASM::declareFragmentOutput(TIntermTyped *fragmentOutput)
	{
		int requestedLocation = fragmentOutput->getType().getLayoutQualifier().location;
		int registerCount = fragmentOutput->totalRegisterCount();
		if(requestedLocation < 0)
		{
			ASSERT(requestedLocation == -1); // All other negative values would have been prevented in TParseContext::parseLayoutQualifier
			return; // No requested location
		}
		else if((requestedLocation + registerCount) > sw::RENDERTARGETS)
		{
			mContext.error(fragmentOutput->getLine(), "Fragment output location larger or equal to MAX_DRAW_BUFFERS", "fragment shader");
		}
		else
		{
			int currentIndex = lookup(fragmentOutputs, fragmentOutput);
			if(requestedLocation != currentIndex)
			{
				if(currentIndex != -1)
				{
					mContext.error(fragmentOutput->getLine(), "Multiple locations for fragment output", "fragment shader");
				}
				else
				{
					if(fragmentOutputs.size() <= (size_t)requestedLocation)
					{
						while(fragmentOutputs.size() < (size_t)requestedLocation)
						{
							fragmentOutputs.push_back(nullptr);
						}
						for(int i = 0; i < registerCount; i++)
						{
							fragmentOutputs.push_back(fragmentOutput);
						}
					}
					else
					{
						for(int i = 0; i < registerCount; i++)
						{
							if(!fragmentOutputs[requestedLocation + i])
							{
								fragmentOutputs[requestedLocation + i] = fragmentOutput;
							}
							else
							{
								mContext.error(fragmentOutput->getLine(), "Fragment output location aliasing", "fragment shader");
								return;
							}
						}
					}
				}
			}
		}
	}

	int OutputASM::uniformRegister(TIntermTyped *uniform)
	{
		const TType &type = uniform->getType();
		ASSERT(!IsSampler(type.getBasicType()));
		TInterfaceBlock *block = type.getAsInterfaceBlock();
		TIntermSymbol *symbol = uniform->getAsSymbolNode();
		ASSERT(symbol || block);

		if(symbol || block)
		{
			TInterfaceBlock* parentBlock = type.getInterfaceBlock();
			bool isBlockMember = (!block && parentBlock);
			int index = isBlockMember ? lookup(uniforms, parentBlock) : lookup(uniforms, uniform);

			if(index == -1 || isBlockMember)
			{
				if(index == -1)
				{
					index = allocate(uniforms, uniform);
				}

				// Verify if the current uniform is a member of an already declared block
				const TString &name = symbol ? symbol->getSymbol() : block->name();
				int blockMemberIndex = blockMemberLookup(type, name, index);
				if(blockMemberIndex == -1)
				{
					declareUniform(type, name, index, false);
				}
				else
				{
					index = blockMemberIndex;
				}
			}

			return index;
		}

		return 0;
	}

	int OutputASM::attributeRegister(TIntermTyped *attribute)
	{
		ASSERT(!attribute->isArray());

		int index = lookup(attributes, attribute);

		if(index == -1)
		{
			TIntermSymbol *symbol = attribute->getAsSymbolNode();
			ASSERT(symbol);

			if(symbol)
			{
				index = allocate(attributes, attribute);
				const TType &type = attribute->getType();
				int registerCount = attribute->totalRegisterCount();
				sw::VertexShader::AttribType attribType = sw::VertexShader::ATTRIBTYPE_FLOAT;
				switch(type.getBasicType())
				{
				case EbtInt:
					attribType = sw::VertexShader::ATTRIBTYPE_INT;
					break;
				case EbtUInt:
					attribType = sw::VertexShader::ATTRIBTYPE_UINT;
					break;
				case EbtFloat:
				default:
					break;
				}

				if(vertexShader && (index + registerCount) <= sw::MAX_VERTEX_INPUTS)
				{
					for(int i = 0; i < registerCount; i++)
					{
						vertexShader->setInput(index + i, sw::Shader::Semantic(sw::Shader::USAGE_TEXCOORD, index + i, false), attribType);
					}
				}

				ActiveAttributes &activeAttributes = shaderObject->activeAttributes;

				const char *name = symbol->getSymbol().c_str();
				activeAttributes.push_back(Attribute(glVariableType(type), name, type.getArraySize(), type.getLayoutQualifier().location, index));
			}
		}

		return index;
	}

	int OutputASM::fragmentOutputRegister(TIntermTyped *fragmentOutput)
	{
		return allocate(fragmentOutputs, fragmentOutput);
	}

	int OutputASM::samplerRegister(TIntermTyped *sampler)
	{
		const TType &type = sampler->getType();
		ASSERT(IsSampler(type.getBasicType()) || type.isStruct());   // Structures can contain samplers

		TIntermSymbol *symbol = sampler->getAsSymbolNode();
		TIntermBinary *binary = sampler->getAsBinaryNode();

		if(symbol)
		{
			switch(type.getQualifier())
			{
			case EvqUniform:
				return samplerRegister(symbol);
			case EvqIn:
			case EvqConstReadOnly:
				// Function arguments are not (uniform) sampler registers
				return -1;
			default:
				UNREACHABLE(type.getQualifier());
			}
		}
		else if(binary)
		{
			TIntermTyped *left = binary->getLeft();
			TIntermTyped *right = binary->getRight();
			const TType &leftType = left->getType();
			int index = right->getAsConstantUnion() ? right->getAsConstantUnion()->getIConst(0) : 0;
			int offset = 0;

			switch(binary->getOp())
			{
			case EOpIndexDirect:
				ASSERT(left->isArray());
				offset = index * leftType.samplerRegisterCount();
				break;
			case EOpIndexDirectStruct:
				ASSERT(leftType.isStruct());
				{
					const TFieldList &fields = leftType.getStruct()->fields();

					for(int i = 0; i < index; i++)
					{
						offset += fields[i]->type()->totalSamplerRegisterCount();
					}
				}
				break;
			case EOpIndexIndirect:               // Indirect indexing produces a temporary, not a sampler register
				return -1;
			case EOpIndexDirectInterfaceBlock:   // Interface blocks can't contain samplers
			default:
				UNREACHABLE(binary->getOp());
				return -1;
			}

			int base = samplerRegister(left);

			if(base < 0)
			{
				return -1;
			}

			return base + offset;
		}

		UNREACHABLE(0);
		return -1;   // Not a (uniform) sampler register
	}

	int OutputASM::samplerRegister(TIntermSymbol *sampler)
	{
		const TType &type = sampler->getType();
		ASSERT(IsSampler(type.getBasicType()) || type.isStruct());   // Structures can contain samplers

		int index = lookup(samplers, sampler);

		if(index == -1)
		{
			index = allocate(samplers, sampler, true);

			if(sampler->getQualifier() == EvqUniform)
			{
				const char *name = sampler->getSymbol().c_str();
				declareUniform(type, name, index, true);
			}
		}

		return index;
	}

	bool OutputASM::isSamplerRegister(TIntermTyped *operand)
	{
		return operand && IsSampler(operand->getBasicType()) && samplerRegister(operand) >= 0;
	}

	int OutputASM::lookup(VariableArray &list, TIntermTyped *variable)
	{
		for(unsigned int i = 0; i < list.size(); i++)
		{
			if(list[i] == variable)
			{
				return i;   // Pointer match
			}
		}

		TIntermSymbol *varSymbol = variable->getAsSymbolNode();
		TInterfaceBlock *varBlock = variable->getType().getAsInterfaceBlock();

		if(varBlock)
		{
			for(unsigned int i = 0; i < list.size(); i++)
			{
				if(list[i])
				{
					TInterfaceBlock *listBlock = list[i]->getType().getAsInterfaceBlock();

					if(listBlock)
					{
						if(listBlock->name() == varBlock->name())
						{
							ASSERT(listBlock->arraySize() == varBlock->arraySize());
							ASSERT(listBlock->fields() == varBlock->fields());
							ASSERT(listBlock->blockStorage() == varBlock->blockStorage());
							ASSERT(listBlock->matrixPacking() == varBlock->matrixPacking());

							return i;
						}
					}
				}
			}
		}
		else if(varSymbol)
		{
			for(unsigned int i = 0; i < list.size(); i++)
			{
				if(list[i])
				{
					TIntermSymbol *listSymbol = list[i]->getAsSymbolNode();

					if(listSymbol)
					{
						if(listSymbol->getId() == varSymbol->getId())
						{
							ASSERT(listSymbol->getSymbol() == varSymbol->getSymbol());
							ASSERT(listSymbol->getType() == varSymbol->getType());
							ASSERT(listSymbol->getQualifier() == varSymbol->getQualifier());

							return i;
						}
					}
				}
			}
		}

		return -1;
	}

	int OutputASM::lookup(VariableArray &list, TInterfaceBlock *block)
	{
		for(unsigned int i = 0; i < list.size(); i++)
		{
			if(list[i] && (list[i]->getType().getInterfaceBlock() == block))
			{
				return i;   // Pointer match
			}
		}
		return -1;
	}

	int OutputASM::allocate(VariableArray &list, TIntermTyped *variable, bool samplersOnly)
	{
		int index = lookup(list, variable);

		if(index == -1)
		{
			unsigned int registerCount = variable->blockRegisterCount(samplersOnly);

			for(unsigned int i = 0; i < list.size(); i++)
			{
				if(list[i] == 0)
				{
					unsigned int j = 1;
					for( ; j < registerCount && (i + j) < list.size(); j++)
					{
						if(list[i + j] != 0)
						{
							break;
						}
					}

					if(j == registerCount)   // Found free slots
					{
						for(unsigned int j = 0; j < registerCount; j++)
						{
							list[i + j] = variable;
						}

						return i;
					}
				}
			}

			index = list.size();

			for(unsigned int i = 0; i < registerCount; i++)
			{
				list.push_back(variable);
			}
		}

		return index;
	}

	void OutputASM::free(VariableArray &list, TIntermTyped *variable)
	{
		int index = lookup(list, variable);

		if(index >= 0)
		{
			list[index] = 0;
		}
	}

	int OutputASM::blockMemberLookup(const TType &type, const TString &name, int registerIndex)
	{
		const TInterfaceBlock *block = type.getInterfaceBlock();

		if(block)
		{
			ActiveUniformBlocks &activeUniformBlocks = shaderObject->activeUniformBlocks;
			const TFieldList& fields = block->fields();
			const TString &blockName = block->name();
			int fieldRegisterIndex = registerIndex;

			if(!type.isInterfaceBlock())
			{
				// This is a uniform that's part of a block, let's see if the block is already defined
				for(size_t i = 0; i < activeUniformBlocks.size(); ++i)
				{
					if(activeUniformBlocks[i].name == blockName.c_str())
					{
						// The block is already defined, find the register for the current uniform and return it
						for(size_t j = 0; j < fields.size(); j++)
						{
							const TString &fieldName = fields[j]->name();
							if(fieldName == name)
							{
								return fieldRegisterIndex;
							}

							fieldRegisterIndex += fields[j]->type()->totalRegisterCount();
						}

						ASSERT(false);
						return fieldRegisterIndex;
					}
				}
			}
		}

		return -1;
	}

	void OutputASM::declareUniform(const TType &type, const TString &name, int registerIndex, bool samplersOnly, int blockId, BlockLayoutEncoder* encoder)
	{
		const TStructure *structure = type.getStruct();
		const TInterfaceBlock *block = (type.isInterfaceBlock() || (blockId == -1)) ? type.getInterfaceBlock() : nullptr;

		if(!structure && !block)
		{
			ActiveUniforms &activeUniforms = shaderObject->activeUniforms;
			const BlockMemberInfo blockInfo = encoder ? encoder->encodeType(type) : BlockMemberInfo::getDefaultBlockInfo();
			if(blockId >= 0)
			{
				blockDefinitions[blockId].insert(BlockDefinitionIndexMap::value_type(registerIndex, TypedMemberInfo(blockInfo, type)));
				shaderObject->activeUniformBlocks[blockId].fields.push_back(activeUniforms.size());
			}
			int fieldRegisterIndex = encoder ? shaderObject->activeUniformBlocks[blockId].registerIndex + BlockLayoutEncoder::getBlockRegister(blockInfo) : registerIndex;
			bool isSampler = IsSampler(type.getBasicType());
			if(isSampler && samplersOnly)
			{
				for(int i = 0; i < type.totalRegisterCount(); i++)
				{
					shader->declareSampler(fieldRegisterIndex + i);
				}
			}
			if(isSampler == samplersOnly)
			{
				activeUniforms.push_back(Uniform(type, name.c_str(), fieldRegisterIndex, blockId, blockInfo));
			}
		}
		else if(block)
		{
			ActiveUniformBlocks &activeUniformBlocks = shaderObject->activeUniformBlocks;
			const TFieldList& fields = block->fields();
			const TString &blockName = block->name();
			int fieldRegisterIndex = registerIndex;
			bool isUniformBlockMember = !type.isInterfaceBlock() && (blockId == -1);

			blockId = activeUniformBlocks.size();
			bool isRowMajor = block->matrixPacking() == EmpRowMajor;
			activeUniformBlocks.push_back(UniformBlock(blockName.c_str(), 0, block->arraySize(),
			                                           block->blockStorage(), isRowMajor, registerIndex, blockId));
			blockDefinitions.push_back(BlockDefinitionIndexMap());

			Std140BlockEncoder currentBlockEncoder;
			currentBlockEncoder.enterAggregateType();
			for(const auto &field : fields)
			{
				const TType &fieldType = *(field->type());
				const TString &fieldName = field->name();
				if(isUniformBlockMember && (fieldName == name))
				{
					registerIndex = fieldRegisterIndex;
				}

				const TString uniformName = block->hasInstanceName() ? blockName + "." + fieldName : fieldName;

				declareUniform(fieldType, uniformName, fieldRegisterIndex, samplersOnly, blockId, &currentBlockEncoder);
				fieldRegisterIndex += fieldType.totalRegisterCount();
			}
			currentBlockEncoder.exitAggregateType();
			activeUniformBlocks[blockId].dataSize = currentBlockEncoder.getBlockSize();
		}
		else
		{
			// Store struct for program link time validation
			shaderObject->activeUniformStructs.push_back(Uniform(type, name.c_str(), registerIndex, -1, BlockMemberInfo::getDefaultBlockInfo()));

			int fieldRegisterIndex = registerIndex;

			const TFieldList& fields = structure->fields();
			if(type.isArray() && (structure || type.isInterfaceBlock()))
			{
				for(int i = 0; i < type.getArraySize(); i++)
				{
					if(encoder)
					{
						encoder->enterAggregateType();
					}
					for(const auto &field : fields)
					{
						const TType &fieldType = *(field->type());
						const TString &fieldName = field->name();
						const TString uniformName = name + "[" + str(i) + "]." + fieldName;

						declareUniform(fieldType, uniformName, fieldRegisterIndex, samplersOnly, blockId, encoder);
						fieldRegisterIndex += samplersOnly ? fieldType.totalSamplerRegisterCount() : fieldType.totalRegisterCount();
					}
					if(encoder)
					{
						encoder->exitAggregateType();
					}
				}
			}
			else
			{
				if(encoder)
				{
					encoder->enterAggregateType();
				}
				for(const auto &field : fields)
				{
					const TType &fieldType = *(field->type());
					const TString &fieldName = field->name();
					const TString uniformName = name + "." + fieldName;

					declareUniform(fieldType, uniformName, fieldRegisterIndex, samplersOnly, blockId, encoder);
					fieldRegisterIndex += samplersOnly ? fieldType.totalSamplerRegisterCount() : fieldType.totalRegisterCount();
				}
				if(encoder)
				{
					encoder->exitAggregateType();
				}
			}
		}
	}

	int OutputASM::dim(TIntermNode *v)
	{
		TIntermTyped *vector = v->getAsTyped();
		ASSERT(vector && vector->isRegister());
		return vector->getNominalSize();
	}

	int OutputASM::dim2(TIntermNode *m)
	{
		TIntermTyped *matrix = m->getAsTyped();
		ASSERT(matrix && matrix->isMatrix() && !matrix->isArray());
		return matrix->getSecondarySize();
	}

	// Returns ~0u if no loop count could be determined
	unsigned int OutputASM::loopCount(TIntermLoop *node)
	{
		// Parse loops of the form:
		// for(int index = initial; index [comparator] limit; index += increment)
		TIntermSymbol *index = 0;
		TOperator comparator = EOpNull;
		int initial = 0;
		int limit = 0;
		int increment = 0;

		// Parse index name and intial value
		if(node->getInit())
		{
			TIntermAggregate *init = node->getInit()->getAsAggregate();

			if(init)
			{
				TIntermSequence &sequence = init->getSequence();
				TIntermTyped *variable = sequence[0]->getAsTyped();

				if(variable && variable->getQualifier() == EvqTemporary && variable->getBasicType() == EbtInt)
				{
					TIntermBinary *assign = variable->getAsBinaryNode();

					if(assign && assign->getOp() == EOpInitialize)
					{
						TIntermSymbol *symbol = assign->getLeft()->getAsSymbolNode();
						TIntermConstantUnion *constant = assign->getRight()->getAsConstantUnion();

						if(symbol && constant)
						{
							if(constant->getBasicType() == EbtInt && constant->getNominalSize() == 1)
							{
								index = symbol;
								initial = constant->getUnionArrayPointer()[0].getIConst();
							}
						}
					}
				}
			}
		}

		// Parse comparator and limit value
		if(index && node->getCondition())
		{
			TIntermBinary *test = node->getCondition()->getAsBinaryNode();
			TIntermSymbol *left = test ? test->getLeft()->getAsSymbolNode() : nullptr;

			if(left && (left->getId() == index->getId()))
			{
				TIntermConstantUnion *constant = test->getRight()->getAsConstantUnion();

				if(constant)
				{
					if(constant->getBasicType() == EbtInt && constant->getNominalSize() == 1)
					{
						comparator = test->getOp();
						limit = constant->getUnionArrayPointer()[0].getIConst();
					}
				}
			}
		}

		// Parse increment
		if(index && comparator != EOpNull && node->getExpression())
		{
			TIntermBinary *binaryTerminal = node->getExpression()->getAsBinaryNode();
			TIntermUnary *unaryTerminal = node->getExpression()->getAsUnaryNode();

			if(binaryTerminal)
			{
				TOperator op = binaryTerminal->getOp();
				TIntermConstantUnion *constant = binaryTerminal->getRight()->getAsConstantUnion();

				if(constant)
				{
					if(constant->getBasicType() == EbtInt && constant->getNominalSize() == 1)
					{
						int value = constant->getUnionArrayPointer()[0].getIConst();

						switch(op)
						{
						case EOpAddAssign: increment = value;  break;
						case EOpSubAssign: increment = -value; break;
						default: UNIMPLEMENTED();
						}
					}
				}
			}
			else if(unaryTerminal)
			{
				TOperator op = unaryTerminal->getOp();

				switch(op)
				{
				case EOpPostIncrement: increment = 1;  break;
				case EOpPostDecrement: increment = -1; break;
				case EOpPreIncrement:  increment = 1;  break;
				case EOpPreDecrement:  increment = -1; break;
				default: UNIMPLEMENTED();
				}
			}
		}

		if(index && comparator != EOpNull && increment != 0)
		{
			if(comparator == EOpLessThanEqual)
			{
				comparator = EOpLessThan;
				limit += 1;
			}
			else if(comparator == EOpGreaterThanEqual)
			{
				comparator = EOpLessThan;
				limit -= 1;
				std::swap(initial, limit);
				increment = -increment;
			}
			else if(comparator == EOpGreaterThan)
			{
				comparator = EOpLessThan;
				std::swap(initial, limit);
				increment = -increment;
			}

			if(comparator == EOpLessThan)
			{
				if(!(initial < limit))   // Never loops
				{
					return 0;
				}

				int iterations = (limit - initial + abs(increment) - 1) / increment;   // Ceiling division

				if(iterations < 0)
				{
					return ~0u;
				}

				return iterations;
			}
			else UNIMPLEMENTED();   // Falls through
		}

		return ~0u;
	}

	bool LoopUnrollable::traverse(TIntermNode *node)
	{
		loopDepth = 0;
		loopUnrollable = true;

		node->traverse(this);

		return loopUnrollable;
	}

	bool LoopUnrollable::visitLoop(Visit visit, TIntermLoop *loop)
	{
		if(visit == PreVisit)
		{
			loopDepth++;
		}
		else if(visit == PostVisit)
		{
			loopDepth++;
		}

		return true;
	}

	bool LoopUnrollable::visitBranch(Visit visit, TIntermBranch *node)
	{
		if(!loopUnrollable)
		{
			return false;
		}

		if(!loopDepth)
		{
			return true;
		}

		switch(node->getFlowOp())
		{
		case EOpKill:
		case EOpReturn:
			break;
		case EOpBreak:
		case EOpContinue:
			loopUnrollable = false;
			break;
		default: UNREACHABLE(node->getFlowOp());
		}

		return loopUnrollable;
	}

	bool LoopUnrollable::visitAggregate(Visit visit, TIntermAggregate *node)
	{
		return loopUnrollable;
	}
}
