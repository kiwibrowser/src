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

#include "MatrixStack.hpp"

#include "Common/Math.hpp"

namespace sw
{
	MatrixStack::MatrixStack(int size)
	{
		stack = new Matrix[size];
		stack[0] = 1;

		top = 0;
		this->size = size;
	}

	MatrixStack::~MatrixStack()
	{
		delete[] stack;
		stack = 0;
	}

	void MatrixStack::identity()
	{
		stack[top] = 1;
	}

	void MatrixStack::load(const Matrix &M)
	{
		stack[top] = M;
	}

	void MatrixStack::load(const float *M)
	{
		stack[top] = Matrix(M[0], M[4], M[8],  M[12],
		                    M[1], M[5], M[9],  M[13],
		                    M[2], M[6], M[10], M[14],
		                    M[3], M[7], M[11], M[15]);
	}

	void MatrixStack::load(const double *M)
	{
		stack[top] = Matrix((float)M[0], (float)M[4], (float)M[8],  (float)M[12],
		                    (float)M[1], (float)M[5], (float)M[9],  (float)M[13],
		                    (float)M[2], (float)M[6], (float)M[10], (float)M[14],
		                    (float)M[3], (float)M[7], (float)M[11], (float)M[15]);
	}

	void MatrixStack::translate(float x, float y, float z)
	{
		stack[top] *= Matrix::translate(x, y, z);
	}

	void MatrixStack::translate(double x, double y, double z)
	{
		translate((float)x, (float)y, (float)z);
	}

	void MatrixStack::rotate(float angle, float x, float y, float z)
	{
		float n = 1.0f / sqrt(x*x + y*y + z*z);

		x *= n;
		y *= n;
		z *= n;

		float theta = angle * 0.0174532925f;   // In radians
		float c = cos(theta);
		float _c = 1 - c;
		float s = sin(theta);

		// Rodrigues' rotation formula
		sw::Matrix rotate(c+x*x*_c,   x*y*_c-z*s, x*z*_c+y*s,
		                  x*y*_c+z*s, c+y*y*_c,   y*z*_c-x*s,
		                  x*z*_c-y*s, y*z*_c+x*s, c+z*z*_c);

		stack[top] *= rotate;
	}

	void MatrixStack::rotate(double angle, double x, double y, double z)
	{
		rotate((float)angle, (float)x, (float)y, (float)z);
	}

	void MatrixStack::scale(float x, float y, float z)
	{
		stack[top] *= Matrix::scale(x, y, z);
	}

	void MatrixStack::scale(double x, double y, double z)
	{
		scale((float)x, (float)y, (float)z);
	}

	void MatrixStack::multiply(const float *M)
	{
		stack[top] *= Matrix(M[0], M[4], M[8],  M[12],
		                     M[1], M[5], M[9],  M[13],
		                     M[2], M[6], M[10], M[14],
		                     M[3], M[7], M[11], M[15]);
	}

	void MatrixStack::multiply(const double *M)
	{
		stack[top] *= Matrix((float)M[0], (float)M[4], (float)M[8],  (float)M[12],
		                     (float)M[1], (float)M[5], (float)M[9],  (float)M[13],
		                     (float)M[2], (float)M[6], (float)M[10], (float)M[14],
		                     (float)M[3], (float)M[7], (float)M[11], (float)M[15]);
	}

	void MatrixStack::frustum(float left, float right, float bottom, float top, float zNear, float zFar)
	{
		float l = (float)left;
		float r = (float)right;
		float b = (float)bottom;
		float t = (float)top;
		float n = (float)zNear;
		float f = (float)zFar;

		float A = (r + l) / (r - l);
		float B = (t + b) / (t - b);
		float C = -(f + n) / (f - n);
		float D = -2 * f * n / (f - n);

		Matrix frustum(2 * n / (r - l), 0,               A,  0,
		               0,               2 * n / (t - b), B,  0,
	                   0,               0,               C,  D,
	                   0,               0,               -1, 0);

		stack[this->top] *= frustum;
	}

	void MatrixStack::ortho(double left, double right, double bottom, double top, double zNear, double zFar)
	{
		float l = (float)left;
		float r = (float)right;
		float b = (float)bottom;
		float t = (float)top;
		float n = (float)zNear;
		float f = (float)zFar;

		float tx = -(r + l) / (r - l);
		float ty = -(t + b) / (t - b);
		float tz = -(f + n) / (f - n);

		Matrix ortho(2 / (r - l), 0,           0,            tx,
		             0,           2 / (t - b), 0,            ty,
		             0,           0,           -2 / (f - n), tz,
		             0,           0,           0,            1);

		stack[this->top] *= ortho;
	}

	bool MatrixStack::push()
	{
		if(top >= size - 1) return false;

		stack[top + 1] = stack[top];
		top++;

		return true;
	}

	bool MatrixStack::pop()
	{
		if(top <= 0) return false;

		top--;

		return true;
	}

	const Matrix &MatrixStack::current()
	{
		return stack[top];
	}

	bool MatrixStack::isIdentity() const
	{
		const Matrix &m = stack[top];

		if(m.m[0][0] != 1.0f) return false;
		if(m.m[0][1] != 0.0f) return false;
		if(m.m[0][2] != 0.0f) return false;
		if(m.m[0][3] != 0.0f) return false;

		if(m.m[1][0] != 0.0f) return false;
		if(m.m[1][1] != 1.0f) return false;
		if(m.m[1][2] != 0.0f) return false;
		if(m.m[1][3] != 0.0f) return false;

		if(m.m[2][0] != 0.0f) return false;
		if(m.m[2][1] != 0.0f) return false;
		if(m.m[2][2] != 1.0f) return false;
		if(m.m[2][3] != 0.0f) return false;

		if(m.m[3][0] != 0.0f) return false;
		if(m.m[3][1] != 0.0f) return false;
		if(m.m[3][2] != 0.0f) return false;
		if(m.m[3][3] != 1.0f) return false;

		return true;
	}
}
