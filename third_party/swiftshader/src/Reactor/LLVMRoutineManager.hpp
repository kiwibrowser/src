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

#ifndef sw_LLVMRoutineManager_hpp
#define sw_LLVMRoutineManager_hpp

#include "llvm/GlobalValue.h"
#include "llvm/ExecutionEngine/JITMemoryManager.h"

namespace sw
{
	class LLVMRoutine;

	class LLVMRoutineManager : public llvm::JITMemoryManager
	{
	public:
		LLVMRoutineManager();

		virtual ~LLVMRoutineManager();

		virtual void AllocateGOT();

		virtual uint8_t *allocateStub(const llvm::GlobalValue *function, unsigned stubSize, unsigned alignment);
		virtual uint8_t *startFunctionBody(const llvm::Function *function, uintptr_t &actualSize);
		virtual void endFunctionBody(const llvm::Function *function, uint8_t *functionStart, uint8_t *functionEnd);
		virtual uint8_t *startExceptionTable(const llvm::Function *function, uintptr_t &ActualSize);
		virtual void endExceptionTable(const llvm::Function *function, uint8_t *tableStart, uint8_t *tableEnd, uint8_t *frameRegister);
		virtual uint8_t *getGOTBase() const;
		virtual uint8_t *allocateSpace(intptr_t Size, unsigned Alignment);
		virtual uint8_t *allocateGlobal(uintptr_t Size, unsigned int Alignment);
		virtual void deallocateFunctionBody(void *Body);
		virtual void deallocateExceptionTable(void *ET);
		virtual void setMemoryWritable();
		virtual void setMemoryExecutable();
		virtual void setPoisonMemory(bool poison);

		LLVMRoutine *acquireRoutine(void *entry);

	private:
		LLVMRoutine *routine;

		static volatile int averageInstructionSize;
	};
}

#endif   // sw_LLVMRoutineManager_hpp
