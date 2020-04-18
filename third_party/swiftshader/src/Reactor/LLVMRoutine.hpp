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

#ifndef sw_LLVMRoutine_hpp
#define sw_LLVMRoutine_hpp

#include "Routine.hpp"

namespace sw
{
	class LLVMRoutineManager;

	class LLVMRoutine : public Routine
	{
		friend class LLVMRoutineManager;

	public:
		LLVMRoutine(int bufferSize);
		//LLVMRoutine(void *memory, int bufferSize, int offset);

		virtual ~LLVMRoutine();

		//void setFunctionSize(int functionSize);

		//const void *getBuffer();
		const void *getEntry();
		//int getBufferSize();
		//int getFunctionSize();   // Includes constants before the entry point
		int getCodeSize();       // Executable code only
		//bool isDynamic();

	private:
		void *buffer;
		const void *entry;
		int bufferSize;
		int functionSize;

		//const bool dynamic;   // Generated or precompiled
	};
}

#endif   // sw_LLVMRoutine_hpp
