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

#ifndef sw_Routine_hpp
#define sw_Routine_hpp

namespace sw
{
	class Routine
	{
	public:
		Routine();

		virtual ~Routine();

		virtual const void *getEntry() = 0;

		// Reference counting
		void bind();
		void unbind();

	private:
		volatile int bindCount;
	};
}

#endif   // sw_Routine_hpp
