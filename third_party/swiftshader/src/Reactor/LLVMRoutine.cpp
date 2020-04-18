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

#include "LLVMRoutine.hpp"

#include "../Common/Memory.hpp"
#include "../Common/Thread.hpp"
#include "../Common/Types.hpp"

namespace sw
{
	LLVMRoutine::LLVMRoutine(int bufferSize) : bufferSize(bufferSize)
	{
		void *memory = allocateExecutable(bufferSize);

		buffer = memory;
		entry = memory;
		functionSize = bufferSize;   // Updated by LLVMRoutineManager::endFunctionBody
	}

	LLVMRoutine::~LLVMRoutine()
	{
		deallocateExecutable(buffer, bufferSize);
	}

	const void *LLVMRoutine::getEntry()
	{
		return entry;
	}

	int LLVMRoutine::getCodeSize()
	{
		return functionSize - static_cast<int>((uintptr_t)entry - (uintptr_t)buffer);
	}
}
