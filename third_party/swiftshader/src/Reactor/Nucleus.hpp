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

#ifndef sw_Nucleus_hpp
#define sw_Nucleus_hpp

#include <cassert>
#include <cstdarg>
#include <cstdint>
#include <vector>

namespace sw
{
	class Type;
	class Value;
	class SwitchCases;
	class BasicBlock;
	class Routine;

	enum Optimization
	{
		Disabled             = 0,
		InstructionCombining = 1,
		CFGSimplification    = 2,
		LICM                 = 3,
		AggressiveDCE        = 4,
		GVN                  = 5,
		Reassociate          = 6,
		DeadStoreElimination = 7,
		SCCP                 = 8,
		ScalarReplAggregates = 9,

		OptimizationCount
	};

	extern Optimization optimization[10];

	class Nucleus
	{
	public:
		Nucleus();

		virtual ~Nucleus();

		Routine *acquireRoutine(const wchar_t *name, bool runOptimizations = true);

		static Value *allocateStackVariable(Type *type, int arraySize = 0);
		static BasicBlock *createBasicBlock();
		static BasicBlock *getInsertBlock();
		static void setInsertBlock(BasicBlock *basicBlock);

		static void createFunction(Type *ReturnType, std::vector<Type*> &Params);
		static Value *getArgument(unsigned int index);

		// Terminators
		static void createRetVoid();
		static void createRet(Value *V);
		static void createBr(BasicBlock *dest);
		static void createCondBr(Value *cond, BasicBlock *ifTrue, BasicBlock *ifFalse);

		// Binary operators
		static Value *createAdd(Value *lhs, Value *rhs);
		static Value *createSub(Value *lhs, Value *rhs);
		static Value *createMul(Value *lhs, Value *rhs);
		static Value *createUDiv(Value *lhs, Value *rhs);
		static Value *createSDiv(Value *lhs, Value *rhs);
		static Value *createFAdd(Value *lhs, Value *rhs);
		static Value *createFSub(Value *lhs, Value *rhs);
		static Value *createFMul(Value *lhs, Value *rhs);
		static Value *createFDiv(Value *lhs, Value *rhs);
		static Value *createURem(Value *lhs, Value *rhs);
		static Value *createSRem(Value *lhs, Value *rhs);
		static Value *createFRem(Value *lhs, Value *rhs);
		static Value *createShl(Value *lhs, Value *rhs);
		static Value *createLShr(Value *lhs, Value *rhs);
		static Value *createAShr(Value *lhs, Value *rhs);
		static Value *createAnd(Value *lhs, Value *rhs);
		static Value *createOr(Value *lhs, Value *rhs);
		static Value *createXor(Value *lhs, Value *rhs);

		// Unary operators
		static Value *createNeg(Value *V);
		static Value *createFNeg(Value *V);
		static Value *createNot(Value *V);

		// Memory instructions
		static Value *createLoad(Value *ptr, Type *type, bool isVolatile = false, unsigned int align = 0);
		static Value *createStore(Value *value, Value *ptr, Type *type, bool isVolatile = false, unsigned int align = 0);
		static Value *createGEP(Value *ptr, Type *type, Value *index, bool unsignedIndex);

		// Atomic instructions
		static Value *createAtomicAdd(Value *ptr, Value *value);

		// Cast/Conversion Operators
		static Value *createTrunc(Value *V, Type *destType);
		static Value *createZExt(Value *V, Type *destType);
		static Value *createSExt(Value *V, Type *destType);
		static Value *createFPToSI(Value *V, Type *destType);
		static Value *createSIToFP(Value *V, Type *destType);
		static Value *createFPTrunc(Value *V, Type *destType);
		static Value *createFPExt(Value *V, Type *destType);
		static Value *createBitCast(Value *V, Type *destType);

		// Compare instructions
		static Value *createICmpEQ(Value *lhs, Value *rhs);
		static Value *createICmpNE(Value *lhs, Value *rhs);
		static Value *createICmpUGT(Value *lhs, Value *rhs);
		static Value *createICmpUGE(Value *lhs, Value *rhs);
		static Value *createICmpULT(Value *lhs, Value *rhs);
		static Value *createICmpULE(Value *lhs, Value *rhs);
		static Value *createICmpSGT(Value *lhs, Value *rhs);
		static Value *createICmpSGE(Value *lhs, Value *rhs);
		static Value *createICmpSLT(Value *lhs, Value *rhs);
		static Value *createICmpSLE(Value *lhs, Value *rhs);
		static Value *createFCmpOEQ(Value *lhs, Value *rhs);
		static Value *createFCmpOGT(Value *lhs, Value *rhs);
		static Value *createFCmpOGE(Value *lhs, Value *rhs);
		static Value *createFCmpOLT(Value *lhs, Value *rhs);
		static Value *createFCmpOLE(Value *lhs, Value *rhs);
		static Value *createFCmpONE(Value *lhs, Value *rhs);
		static Value *createFCmpORD(Value *lhs, Value *rhs);
		static Value *createFCmpUNO(Value *lhs, Value *rhs);
		static Value *createFCmpUEQ(Value *lhs, Value *rhs);
		static Value *createFCmpUGT(Value *lhs, Value *rhs);
		static Value *createFCmpUGE(Value *lhs, Value *rhs);
		static Value *createFCmpULT(Value *lhs, Value *rhs);
		static Value *createFCmpULE(Value *lhs, Value *rhs);
		static Value *createFCmpUNE(Value *lhs, Value *rhs);

		// Vector instructions
		static Value *createExtractElement(Value *vector, Type *type, int index);
		static Value *createInsertElement(Value *vector, Value *element, int index);
		static Value *createShuffleVector(Value *V1, Value *V2, const int *select);

		// Other instructions
		static Value *createSelect(Value *C, Value *ifTrue, Value *ifFalse);
		static SwitchCases *createSwitch(Value *control, BasicBlock *defaultBranch, unsigned numCases);
		static void addSwitchCase(SwitchCases *switchCases, int label, BasicBlock *branch);
		static void createUnreachable();

		// Constant values
		static Value *createNullValue(Type *type);
		static Value *createConstantLong(int64_t i);
		static Value *createConstantInt(int i);
		static Value *createConstantInt(unsigned int i);
		static Value *createConstantBool(bool b);
		static Value *createConstantByte(signed char i);
		static Value *createConstantByte(unsigned char i);
		static Value *createConstantShort(short i);
		static Value *createConstantShort(unsigned short i);
		static Value *createConstantFloat(float x);
		static Value *createNullPointer(Type *type);
		static Value *createConstantVector(const int64_t *constants, Type *type);
		static Value *createConstantVector(const double *constants, Type *type);

		static Type *getPointerType(Type *elementType);

	private:
		void optimize();
	};
}

#endif   // sw_Nucleus_hpp
