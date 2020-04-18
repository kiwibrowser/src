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

#ifndef D3D9_Unknown_hpp
#define D3D9_Unknown_hpp

#include <unknwn.h>

namespace D3D9
{
	class Unknown : IUnknown
	{
	public:
		Unknown();

		virtual ~Unknown();

		// IUnknown methods
		long __stdcall QueryInterface(const IID &iid, void **object) override;
		unsigned long __stdcall AddRef() override;
		unsigned long __stdcall Release() override;

		// Internal methods
		virtual void bind();
		virtual void unbind();

	private:
		volatile long referenceCount;
		volatile long bindCount;
	};
}

#endif   // D3D9_Unknown_hpp
