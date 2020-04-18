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

#ifndef COMPILER_OUTPUTASM_H_
#define COMPILER_OUTPUTASM_H_

#include "intermediate.h"
#include "ParseHelper.h"
#include "Shader/PixelShader.hpp"
#include "Shader/VertexShader.hpp"

#include <list>
#include <set>
#include <map>

namespace es2
{
	class Shader;
}

typedef unsigned int GLenum;

namespace glsl
{
	struct BlockMemberInfo
	{
		BlockMemberInfo() : offset(-1), arrayStride(-1), matrixStride(-1), isRowMajorMatrix(false) {}

		BlockMemberInfo(int offset, int arrayStride, int matrixStride, bool isRowMajorMatrix)
			: offset(offset),
			arrayStride(arrayStride),
			matrixStride(matrixStride),
			isRowMajorMatrix(isRowMajorMatrix)
		{}

		static BlockMemberInfo getDefaultBlockInfo()
		{
			return BlockMemberInfo(-1, -1, -1, false);
		}

		int offset;
		int arrayStride;
		int matrixStride;
		bool isRowMajorMatrix;
	};

	struct ShaderVariable
	{
		ShaderVariable(const TType& type, const std::string& name, int registerIndex);

		GLenum type;
		GLenum precision;
		std::string name;
		int arraySize;

		int registerIndex;

		std::vector<ShaderVariable> fields;
	};

	struct Uniform : public ShaderVariable
	{
		Uniform(const TType& type, const std::string &name, int registerIndex, int blockId, const BlockMemberInfo& blockMemberInfo);

		int blockId;
		BlockMemberInfo blockInfo;
	};

	typedef std::vector<Uniform> ActiveUniforms;

	struct UniformBlock
	{
		UniformBlock(const std::string& name, unsigned int dataSize, unsigned int arraySize,
		             TLayoutBlockStorage layout, bool isRowMajorLayout, int registerIndex, int blockId);

		std::string name;
		unsigned int dataSize;
		unsigned int arraySize;
		TLayoutBlockStorage layout;
		bool isRowMajorLayout;
		std::vector<int> fields;

		int registerIndex;

		int blockId;
	};

	class BlockLayoutEncoder
	{
	public:
		BlockLayoutEncoder();
		virtual ~BlockLayoutEncoder() {}

		BlockMemberInfo encodeType(const TType &type);

		size_t getBlockSize() const { return mCurrentOffset * BytesPerComponent; }

		virtual void enterAggregateType() = 0;
		virtual void exitAggregateType() = 0;

		static const size_t BytesPerComponent = 4u;
		static const unsigned int ComponentsPerRegister = 4u;

		static size_t getBlockRegister(const BlockMemberInfo &info);
		static size_t getBlockRegisterElement(const BlockMemberInfo &info);

	protected:
		size_t mCurrentOffset;

		void nextRegister();

		virtual void getBlockLayoutInfo(const TType &type, unsigned int arraySize, bool isRowMajorMatrix, int *arrayStrideOut, int *matrixStrideOut) = 0;
		virtual void advanceOffset(const TType &type, unsigned int arraySize, bool isRowMajorMatrix, int arrayStride, int matrixStride) = 0;
	};

	// Block layout according to the std140 block layout
	// See "Standard Uniform Block Layout" in Section 2.11.6 of the OpenGL ES 3.0 specification
	class Std140BlockEncoder : public BlockLayoutEncoder
	{
	public:
		Std140BlockEncoder();

		void enterAggregateType() override;
		void exitAggregateType() override;

	protected:
		void getBlockLayoutInfo(const TType &type, unsigned int arraySize, bool isRowMajorMatrix, int *arrayStrideOut, int *matrixStrideOut) override;
		void advanceOffset(const TType &type, unsigned int arraySize, bool isRowMajorMatrix, int arrayStride, int matrixStride) override;
	};

	typedef std::vector<UniformBlock> ActiveUniformBlocks;

	struct Attribute
	{
		Attribute();
		Attribute(GLenum type, const std::string &name, int arraySize, int location, int registerIndex);

		GLenum type;
		std::string name;
		int arraySize;
		int location;

		int registerIndex;
	};

	typedef std::vector<Attribute> ActiveAttributes;

	struct Varying : public ShaderVariable
	{
		Varying(const TType& type, const std::string &name, int reg = -1, int col = -1)
			: ShaderVariable(type, name, reg), qualifier(type.getQualifier()), column(col)
		{
		}

		bool isArray() const
		{
			return arraySize >= 1;
		}

		int size() const   // Unify with es2::Uniform?
		{
			return arraySize > 0 ? arraySize : 1;
		}

		TQualifier qualifier;
		int column;    // First register element, assigned during link
	};

	typedef std::list<Varying> VaryingList;

	class Shader
	{
		friend class OutputASM;
	public:
		virtual ~Shader() {};
		virtual sw::Shader *getShader() const = 0;
		virtual sw::PixelShader *getPixelShader() const;
		virtual sw::VertexShader *getVertexShader() const;
		int getShaderVersion() const { return shaderVersion; }

	protected:
		VaryingList varyings;
		ActiveUniforms activeUniforms;
		ActiveUniforms activeUniformStructs;
		ActiveAttributes activeAttributes;
		ActiveUniformBlocks activeUniformBlocks;
		int shaderVersion;
	};

	struct Function
	{
		Function(int label, const char *name, TIntermSequence *arg, TIntermTyped *ret) : label(label), name(name), arg(arg), ret(ret)
		{
		}

		Function(int label, const TString &name, TIntermSequence *arg, TIntermTyped *ret) : label(label), name(name), arg(arg), ret(ret)
		{
		}

		int label;
		TString name;
		TIntermSequence *arg;
		TIntermTyped *ret;
	};

	typedef sw::Shader::Instruction Instruction;

	class Temporary;

	class OutputASM : public TIntermTraverser
	{
	public:
		explicit OutputASM(TParseContext &context, Shader *shaderObject);
		~OutputASM();

		void output();

		void freeTemporary(Temporary *temporary);

	private:
		enum Scope
		{
			GLOBAL,
			FUNCTION
		};

		struct TextureFunction
		{
			TextureFunction(const TString& name);

			enum Method
			{
				IMPLICIT,   // Mipmap LOD determined implicitly (standard lookup)
				LOD,
				SIZE,   // textureSize()
				FETCH,
				GRAD,
			};

			Method method;
			bool proj;
			bool offset;
		};

		void emitShader(Scope scope);

		// Visit AST nodes and output their code to the body stream
		void visitSymbol(TIntermSymbol*) override;
		bool visitBinary(Visit visit, TIntermBinary*) override;
		bool visitUnary(Visit visit, TIntermUnary*) override;
		bool visitSelection(Visit visit, TIntermSelection*) override;
		bool visitAggregate(Visit visit, TIntermAggregate*) override;
		bool visitLoop(Visit visit, TIntermLoop*) override;
		bool visitBranch(Visit visit, TIntermBranch*) override;
		bool visitSwitch(Visit, TIntermSwitch*) override;

		sw::Shader::Opcode getOpcode(sw::Shader::Opcode op, TIntermTyped *in) const;
		Instruction *emit(sw::Shader::Opcode op, TIntermTyped *dst = 0, TIntermNode *src0 = 0, TIntermNode *src1 = 0, TIntermNode *src2 = 0, TIntermNode *src3 = 0, TIntermNode *src4 = 0);
		Instruction *emit(sw::Shader::Opcode op, TIntermTyped *dst, int dstIndex, TIntermNode *src0 = 0, int index0 = 0, TIntermNode *src1 = 0, int index1 = 0,
		                  TIntermNode *src2 = 0, int index2 = 0, TIntermNode *src3 = 0, int index3 = 0, TIntermNode *src4 = 0, int index4 = 0);
		Instruction *emitCast(TIntermTyped *dst, TIntermTyped *src);
		Instruction *emitCast(TIntermTyped *dst, int dstIndex, TIntermTyped *src, int srcIndex);
		void emitBinary(sw::Shader::Opcode op, TIntermTyped *dst = 0, TIntermNode *src0 = 0, TIntermNode *src1 = 0, TIntermNode *src2 = 0);
		void emitAssign(sw::Shader::Opcode op, TIntermTyped *result, TIntermTyped *lhs, TIntermTyped *src0, TIntermTyped *src1 = 0);
		void emitCmp(sw::Shader::Control cmpOp, TIntermTyped *dst, TIntermNode *left, TIntermNode *right, int index = 0);
		void emitDeterminant(TIntermTyped *result, TIntermTyped *arg, int size, int col = -1, int row = -1, int outCol = 0, int outRow = 0);
		void source(sw::Shader::SourceParameter &parameter, TIntermNode *argument, int index = 0);
		void destination(sw::Shader::DestinationParameter &parameter, TIntermTyped *argument, int index = 0);
		void copy(TIntermTyped *dst, TIntermNode *src, int offset = 0);
		void assignLvalue(TIntermTyped *dst, TIntermTyped *src);
		void evaluateRvalue(TIntermTyped *node);
		int lvalue(sw::Shader::DestinationParameter &dst, TIntermTyped *node);
		int lvalue(TIntermTyped *&root, unsigned int &offset, sw::Shader::Relative &rel, unsigned char &mask, Temporary &address, TIntermTyped *node);
		sw::Shader::ParameterType registerType(TIntermTyped *operand);
		bool hasFlatQualifier(TIntermTyped *operand);
		unsigned int registerIndex(TIntermTyped *operand);
		int writeMask(TIntermTyped *destination, int index = 0);
		int readSwizzle(TIntermTyped *argument, int size);
		bool trivial(TIntermTyped *expression, int budget);   // Fast to compute and no side effects
		int cost(TIntermNode *expression, int budget);
		const Function *findFunction(const TString &name);

		int temporaryRegister(TIntermTyped *temporary);
		int varyingRegister(TIntermTyped *varying);
		void setPixelShaderInputs(const TType& type, int var, bool flat);
		void declareVarying(TIntermTyped *varying, int reg);
		void declareVarying(const TType &type, const TString &name, int registerIndex);
		void declareFragmentOutput(TIntermTyped *fragmentOutput);
		int uniformRegister(TIntermTyped *uniform);
		int attributeRegister(TIntermTyped *attribute);
		int fragmentOutputRegister(TIntermTyped *fragmentOutput);
		int samplerRegister(TIntermTyped *sampler);
		int samplerRegister(TIntermSymbol *sampler);
		bool isSamplerRegister(TIntermTyped *operand);

		typedef std::vector<TIntermTyped*> VariableArray;

		int lookup(VariableArray &list, TIntermTyped *variable);
		int lookup(VariableArray &list, TInterfaceBlock *block);
		int blockMemberLookup(const TType &type, const TString &name, int registerIndex);
		int allocate(VariableArray &list, TIntermTyped *variable, bool samplersOnly = false);
		void free(VariableArray &list, TIntermTyped *variable);

		void declareUniform(const TType &type, const TString &name, int registerIndex, bool samplersOnly, int blockId = -1, BlockLayoutEncoder* encoder = nullptr);

		static int dim(TIntermNode *v);
		static int dim2(TIntermNode *m);
		static unsigned int loopCount(TIntermLoop *node);

		Shader *const shaderObject;
		sw::Shader *shader;
		sw::PixelShader *pixelShader;
		sw::VertexShader *vertexShader;

		VariableArray temporaries;
		VariableArray uniforms;
		VariableArray varyings;
		VariableArray attributes;
		VariableArray samplers;
		VariableArray fragmentOutputs;

		struct TypedMemberInfo : public BlockMemberInfo
		{
			TypedMemberInfo(const BlockMemberInfo& b, const TType& t) : BlockMemberInfo(b), type(t) {}
			TType type;
		};
		struct ArgumentInfo
		{
			ArgumentInfo(const BlockMemberInfo& b, const TType& t, int clampedIndex, int bufferIndex) :
			    typedMemberInfo(b, t), clampedIndex(clampedIndex), bufferIndex(bufferIndex) {}
			TypedMemberInfo typedMemberInfo;
			int clampedIndex;
			int bufferIndex;
		};
		int getBlockId(TIntermTyped *argument);
		ArgumentInfo getArgumentInfo(TIntermTyped *argument, int index);

		typedef std::map<int, TypedMemberInfo> BlockDefinitionIndexMap;
		std::vector<BlockDefinitionIndexMap> blockDefinitions;

		Scope emitScope;
		Scope currentScope;

		int currentFunction;
		std::vector<Function> functionArray;

		TQualifier outputQualifier;

		TParseContext &mContext;
	};

	class LoopUnrollable : public TIntermTraverser
	{
	public:
		bool traverse(TIntermNode *node);

	private:
		bool visitBranch(Visit visit, TIntermBranch *node);
		bool visitLoop(Visit visit, TIntermLoop *loop);
		bool visitAggregate(Visit visit, TIntermAggregate *node);

		int loopDepth;
		bool loopUnrollable;
	};
}

#endif   // COMPILER_OUTPUTASM_H_
