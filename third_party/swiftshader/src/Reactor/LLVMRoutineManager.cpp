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

#include "LLVMRoutineManager.hpp"

#include "LLVMRoutine.hpp"
#include "llvm/Function.h"
#include "../Common/Memory.hpp"
#include "../Common/Thread.hpp"
#include "../Common/Debug.hpp"

namespace sw
{
	using namespace llvm;

	volatile int LLVMRoutineManager::averageInstructionSize = 4;

	LLVMRoutineManager::LLVMRoutineManager()
	{
		routine = nullptr;
	}

	LLVMRoutineManager::~LLVMRoutineManager()
	{
		delete routine;
	}

	void LLVMRoutineManager::AllocateGOT()
	{
		UNIMPLEMENTED();
	}

	uint8_t *LLVMRoutineManager::allocateStub(const GlobalValue *function, unsigned stubSize, unsigned alignment)
	{
		UNIMPLEMENTED();
		return nullptr;
	}

	uint8_t *LLVMRoutineManager::startFunctionBody(const llvm::Function *function, uintptr_t &actualSize)
	{
		if(actualSize == 0)   // Estimate size
		{
			size_t instructionCount = 0;
			for(llvm::Function::const_iterator basicBlock = function->begin(); basicBlock != function->end(); basicBlock++)
			{
				instructionCount += basicBlock->size();
			}

			actualSize = instructionCount * averageInstructionSize;
		}
		else   // Estimate was too low
		{
			sw::atomicIncrement(&averageInstructionSize);
		}

		// Round up to the next page size
		size_t pageSize = memoryPageSize();
		actualSize = (actualSize + pageSize - 1) & ~(pageSize - 1);

		delete routine;
		routine = new LLVMRoutine(static_cast<int>(actualSize));

		return (uint8_t*)routine->buffer;
	}

	void LLVMRoutineManager::endFunctionBody(const llvm::Function *function, uint8_t *functionStart, uint8_t *functionEnd)
	{
		routine->functionSize = static_cast<int>(static_cast<ptrdiff_t>(functionEnd - functionStart));
	}

	uint8_t *LLVMRoutineManager::startExceptionTable(const llvm::Function* F, uintptr_t &ActualSize)
	{
		UNIMPLEMENTED();
		return nullptr;
	}

	void LLVMRoutineManager::endExceptionTable(const llvm::Function *F, uint8_t *TableStart, uint8_t *TableEnd, uint8_t* FrameRegister)
	{
		UNIMPLEMENTED();
	}

	uint8_t *LLVMRoutineManager::getGOTBase() const
	{
		ASSERT(!HasGOT);
		return nullptr;
	}

	uint8_t *LLVMRoutineManager::allocateSpace(intptr_t Size, unsigned Alignment)
	{
		UNIMPLEMENTED();
		return nullptr;
	}

	uint8_t *LLVMRoutineManager::allocateGlobal(uintptr_t Size, unsigned Alignment)
	{
		UNIMPLEMENTED();
		return nullptr;
	}

	void LLVMRoutineManager::deallocateFunctionBody(void *Body)
	{
		delete routine;
		routine = nullptr;
	}

	void LLVMRoutineManager::deallocateExceptionTable(void *ET)
	{
		if(ET)
		{
			UNIMPLEMENTED();
		}
	}

	void LLVMRoutineManager::setMemoryWritable()
	{
	}

	void LLVMRoutineManager::setMemoryExecutable()
	{
		markExecutable(routine->buffer, routine->bufferSize);
	}

	void LLVMRoutineManager::setPoisonMemory(bool poison)
	{
		UNIMPLEMENTED();
	}

	LLVMRoutine *LLVMRoutineManager::acquireRoutine(void *entry)
	{
		routine->entry = entry;

		LLVMRoutine *result = routine;
		routine = nullptr;

		return result;
	}
}
