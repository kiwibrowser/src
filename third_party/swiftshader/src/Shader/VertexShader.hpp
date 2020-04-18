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

#ifndef sw_VertexShader_hpp
#define sw_VertexShader_hpp

#include "Shader.hpp"
#include "Main/Config.hpp"

namespace sw
{
	class VertexShader : public Shader
	{
	public:
		enum AttribType : unsigned char
		{
			ATTRIBTYPE_FLOAT,
			ATTRIBTYPE_INT,
			ATTRIBTYPE_UINT,

			ATTRIBTYPE_LAST = ATTRIBTYPE_UINT
		};

		explicit VertexShader(const VertexShader *vs = 0);
		explicit VertexShader(const unsigned long *token);

		virtual ~VertexShader();

		static int validate(const unsigned long *const token);   // Returns number of instructions if valid
		bool containsTextureSampling() const;

		void setInput(int inputIdx, const Semantic& semantic, AttribType attribType = ATTRIBTYPE_FLOAT);
		void setOutput(int outputIdx, int nbComponents, const Semantic& semantic);
		void setPositionRegister(int posReg);
		void setPointSizeRegister(int ptSizeReg);
		void declareInstanceId() { instanceIdDeclared = true; }
		void declareVertexId() { vertexIdDeclared = true; }

		const Semantic& getInput(int inputIdx) const;
		const Semantic& getOutput(int outputIdx, int component) const;
		AttribType getAttribType(int inputIndex) const;
		int getPositionRegister() const { return positionRegister; }
		int getPointSizeRegister() const { return pointSizeRegister; }
		bool isInstanceIdDeclared() const { return instanceIdDeclared; }
		bool isVertexIdDeclared() const { return vertexIdDeclared; }

	private:
		void analyze();
		void analyzeInput();
		void analyzeOutput();
		void analyzeTextureSampling();

		Semantic input[MAX_VERTEX_INPUTS];
		Semantic output[MAX_VERTEX_OUTPUTS][4];

		AttribType attribType[MAX_VERTEX_INPUTS];

		int positionRegister;
		int pointSizeRegister;

		bool instanceIdDeclared;
		bool vertexIdDeclared;
		bool textureSampling;
	};
}

#endif   // sw_VertexShader_hpp
