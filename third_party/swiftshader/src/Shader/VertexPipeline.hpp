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

#ifndef sw_VertexPipeline_hpp
#define sw_VertexPipeline_hpp

#include "VertexRoutine.hpp"

#include "Renderer/Context.hpp"
#include "Renderer/VertexProcessor.hpp"

namespace sw
{
	class VertexPipeline : public VertexRoutine
	{
	public:
		VertexPipeline(const VertexProcessor::State &state);

		virtual ~VertexPipeline();

	private:
		void pipeline(UInt &index) override;
		void processTextureCoordinate(int stage, Vector4f &normal, Vector4f &position);
		void processPointSize();

		Vector4f transformBlend(const Register &src, const Pointer<Byte> &matrix, bool homogenous);
		Vector4f transform(const Register &src, const Pointer<Byte> &matrix, bool homogenous);
		Vector4f transform(const Register &src, const Pointer<Byte> &matrix, UInt index[4], bool homogenous);
		Vector4f normalize(Vector4f &src);
		Float4 power(Float4 &src0, Float4 &src1);
	};
};

#endif   // sw_VertexPipeline_hpp
