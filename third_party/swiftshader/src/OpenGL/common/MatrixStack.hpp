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

#ifndef sw_MatrixStack_hpp
#define sw_MatrixStack_hpp

#include "Renderer/Matrix.hpp"

namespace sw
{
	class MatrixStack
	{
	public:
		MatrixStack(int size = 2);

		~MatrixStack();

		void identity();
		void load(const Matrix &M);
		void load(const float *M);
		void load(const double *M);

		void translate(float x, float y, float z);
		void translate(double x, double y, double z);
		void rotate(float angle, float x, float y, float z);
		void rotate(double angle, double x, double y, double z);
		void scale(float x, float y, float z);
		void scale(double x, double y, double z);
		void multiply(const float *M);
		void multiply(const double *M);

		void frustum(float left, float right, float bottom, float top, float zNear, float zFar);
		void ortho(double left, double right, double bottom, double top, double zNear, double zFar);

		bool push();   // False on overflow
		bool pop();    // False on underflow

		const Matrix &current();
		bool isIdentity() const;

	private:
		int top;
		int size;
		Matrix *stack;
	};
}

#endif   // sw_MatrixStack_hpp
