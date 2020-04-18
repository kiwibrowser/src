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

#ifndef sw_Rasterizer_hpp
#define sw_Rasterizer_hpp

#include "Context.hpp"
#include "PixelProcessor.hpp"
#include "Main/Config.hpp"

namespace sw
{
	class Rasterizer : public Function<Void(Pointer<Byte>, Int, Int, Pointer<Byte>)>
	{
	public:
		Rasterizer() : primitive(Arg<0>()), count(Arg<1>()), cluster(Arg<2>()), data(Arg<3>()) {}
		virtual ~Rasterizer() {};

	protected:
		Pointer<Byte> primitive;
		Int count;
		Int cluster;
		Pointer<Byte> data;
	};
}

#endif   // sw_Rasterizer_hpp
