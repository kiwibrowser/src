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

#include "Matrix.hpp"

#include "Point.hpp"
#include "Common/Math.hpp"

namespace sw
{
	Matrix Matrix::diag(float m11, float m22, float m33, float m44)
	{
		return Matrix(m11, 0,   0,   0,
		              0,   m22, 0,   0,
		              0,   0,   m33, 0,
		              0,   0,   0,   m44);
	}

	Matrix::operator float*()
	{
		return &(*this)(1, 1);
	}

	Matrix Matrix::operator+() const
	{
		return *this;
	}

	Matrix Matrix::operator-() const
	{
		const Matrix &M = *this;

		return Matrix(-M(1, 1), -M(1, 2), -M(1, 3), -M(1, 4), 
		              -M(2, 1), -M(2, 2), -M(2, 3), -M(2, 4), 
		              -M(3, 1), -M(3, 2), -M(3, 3), -M(3, 4), 
		              -M(4, 1), -M(4, 2), -M(4, 3), -M(4, 4));
	}

	Matrix Matrix::operator!() const
	{
		const Matrix &M = *this;
		Matrix I;

		float M3344 = M(3, 3) * M(4, 4) - M(4, 3) * M(3, 4);
		float M2344 = M(2, 3) * M(4, 4) - M(4, 3) * M(2, 4);
		float M2334 = M(2, 3) * M(3, 4) - M(3, 3) * M(2, 4);
		float M3244 = M(3, 2) * M(4, 4) - M(4, 2) * M(3, 4);
		float M2244 = M(2, 2) * M(4, 4) - M(4, 2) * M(2, 4);
		float M2234 = M(2, 2) * M(3, 4) - M(3, 2) * M(2, 4);
		float M3243 = M(3, 2) * M(4, 3) - M(4, 2) * M(3, 3);
		float M2243 = M(2, 2) * M(4, 3) - M(4, 2) * M(2, 3);
		float M2233 = M(2, 2) * M(3, 3) - M(3, 2) * M(2, 3);
		float M1344 = M(1, 3) * M(4, 4) - M(4, 3) * M(1, 4);
		float M1334 = M(1, 3) * M(3, 4) - M(3, 3) * M(1, 4);
		float M1244 = M(1, 2) * M(4, 4) - M(4, 2) * M(1, 4);
		float M1234 = M(1, 2) * M(3, 4) - M(3, 2) * M(1, 4);
		float M1243 = M(1, 2) * M(4, 3) - M(4, 2) * M(1, 3);
		float M1233 = M(1, 2) * M(3, 3) - M(3, 2) * M(1, 3);
		float M1324 = M(1, 3) * M(2, 4) - M(2, 3) * M(1, 4);
		float M1224 = M(1, 2) * M(2, 4) - M(2, 2) * M(1, 4);
		float M1223 = M(1, 2) * M(2, 3) - M(2, 2) * M(1, 3);

		// Adjoint Matrix
		I(1, 1) =  M(2, 2) * M3344 - M(3, 2) * M2344 + M(4, 2) * M2334;
		I(2, 1) = -M(2, 1) * M3344 + M(3, 1) * M2344 - M(4, 1) * M2334;
		I(3, 1) =  M(2, 1) * M3244 - M(3, 1) * M2244 + M(4, 1) * M2234;
		I(4, 1) = -M(2, 1) * M3243 + M(3, 1) * M2243 - M(4, 1) * M2233;

		I(1, 2) = -M(1, 2) * M3344 + M(3, 2) * M1344 - M(4, 2) * M1334;
		I(2, 2) =  M(1, 1) * M3344 - M(3, 1) * M1344 + M(4, 1) * M1334;
		I(3, 2) = -M(1, 1) * M3244 + M(3, 1) * M1244 - M(4, 1) * M1234;
		I(4, 2) =  M(1, 1) * M3243 - M(3, 1) * M1243 + M(4, 1) * M1233;

		I(1, 3) =  M(1, 2) * M2344 - M(2, 2) * M1344 + M(4, 2) * M1324;
		I(2, 3) = -M(1, 1) * M2344 + M(2, 1) * M1344 - M(4, 1) * M1324;
		I(3, 3) =  M(1, 1) * M2244 - M(2, 1) * M1244 + M(4, 1) * M1224;
		I(4, 3) = -M(1, 1) * M2243 + M(2, 1) * M1243 - M(4, 1) * M1223;

		I(1, 4) = -M(1, 2) * M2334 + M(2, 2) * M1334 - M(3, 2) * M1324;
		I(2, 4) =  M(1, 1) * M2334 - M(2, 1) * M1334 + M(3, 1) * M1324;
		I(3, 4) = -M(1, 1) * M2234 + M(2, 1) * M1234 - M(3, 1) * M1224;
		I(4, 4) =  M(1, 1) * M2233 - M(2, 1) * M1233 + M(3, 1) * M1223;

		// Division by determinant
		I /= M(1, 1) * I(1, 1) +
		     M(2, 1) * I(1, 2) +
		     M(3, 1) * I(1, 3) +
		     M(4, 1) * I(1, 4);

		return I;
	}

	Matrix Matrix::operator~() const
	{
		const Matrix &M = *this;

		return Matrix(M(1, 1), M(2, 1), M(3, 1), M(4, 1), 
		              M(1, 2), M(2, 2), M(3, 2), M(4, 2), 
		              M(1, 3), M(2, 3), M(3, 3), M(4, 3), 
		              M(1, 4), M(2, 4), M(3, 4), M(4, 4));
	}

	Matrix &Matrix::operator+=(const Matrix &N)
	{
		Matrix &M = *this;

		M(1, 1) += N(1, 1); M(1, 2) += N(1, 2); M(1, 3) += N(1, 3); M(1, 4) += N(1, 4);
		M(2, 1) += N(2, 1); M(2, 2) += N(2, 2); M(2, 3) += N(2, 3); M(2, 4) += N(2, 4);
		M(3, 1) += N(3, 1); M(3, 2) += N(3, 2); M(3, 3) += N(3, 3); M(3, 4) += N(3, 4);
		M(4, 1) += N(4, 1); M(4, 2) += N(4, 2); M(4, 3) += N(4, 3); M(4, 4) += N(4, 4);

		return M;
	}

	Matrix &Matrix::operator-=(const Matrix &N)
	{
		Matrix &M = *this;

		M(1, 1) -= N(1, 1); M(1, 2) -= N(1, 2); M(1, 3) -= N(1, 3); M(1, 4) -= N(1, 4);
		M(2, 1) -= N(2, 1); M(2, 2) -= N(2, 2); M(2, 3) -= N(2, 3); M(2, 4) -= N(2, 4);
		M(3, 1) -= N(3, 1); M(3, 2) -= N(3, 2); M(3, 3) -= N(3, 3); M(3, 4) -= N(3, 4);
		M(4, 1) -= N(4, 1); M(4, 2) -= N(4, 2); M(4, 3) -= N(4, 3); M(4, 4) -= N(4, 4);

		return M;
	}

	Matrix &Matrix::operator*=(float s)
	{
		Matrix &M = *this;

		M(1, 1) *= s; M(1, 2) *= s; M(1, 3) *= s; M(1, 4) *= s;
		M(2, 1) *= s; M(2, 2) *= s; M(2, 3) *= s; M(2, 4) *= s;
		M(3, 1) *= s; M(3, 2) *= s; M(3, 3) *= s; M(3, 4) *= s;
		M(4, 1) *= s; M(4, 2) *= s; M(4, 3) *= s; M(4, 4) *= s;

		return M;
	}

	Matrix &Matrix::operator*=(const Matrix &M)
	{
		return *this = *this * M;
	}

	Matrix &Matrix::operator/=(float s)
	{
		float r = 1.0f / s;

		return *this *= r;
	}

	bool operator==(const Matrix &M, const Matrix &N)
	{
		if(M(1, 1) == N(1, 1) && M(1, 2) == N(1, 2) && M(1, 3) == N(1, 3) && M(1, 4) == N(1, 4) &&
		   M(2, 1) == N(2, 1) && M(2, 2) == N(2, 2) && M(2, 3) == N(2, 3) && M(2, 4) == N(2, 4) &&
		   M(3, 1) == N(3, 1) && M(3, 2) == N(3, 2) && M(3, 3) == N(3, 3) && M(3, 4) == N(3, 4) &&
		   M(4, 1) == N(4, 1) && M(4, 2) == N(4, 2) && M(4, 3) == N(4, 3) && M(4, 4) == N(4, 4))
			return true;
		else
			return false;
	}

	bool operator!=(const Matrix &M, const Matrix &N)
	{
		if(M(1, 1) != N(1, 1) || M(1, 2) != N(1, 2) || M(1, 3) != N(1, 3) || M(1, 4) != N(1, 4) ||
		   M(2, 1) != N(2, 1) || M(2, 2) != N(2, 2) || M(2, 3) != N(2, 3) || M(2, 4) != N(2, 4) ||
		   M(3, 1) != N(3, 1) || M(3, 2) != N(3, 2) || M(3, 3) != N(3, 3) || M(3, 4) != N(3, 4) ||
		   M(4, 1) != N(4, 1) || M(4, 2) != N(4, 2) || M(4, 3) != N(4, 3) || M(4, 4) != N(4, 4))
			return true;
		else
			return false;
	}

	Matrix operator+(const Matrix &M, const Matrix &N)
	{
		return Matrix(M(1, 1) + N(1, 1), M(1, 2) + N(1, 2), M(1, 3) + N(1, 3), M(1, 4) + N(1, 4), 
		              M(2, 1) + N(2, 1), M(2, 2) + N(2, 2), M(2, 3) + N(2, 3), M(2, 4) + N(2, 4), 
		              M(3, 1) + N(3, 1), M(3, 2) + N(3, 2), M(3, 3) + N(3, 3), M(3, 4) + N(3, 4), 
		              M(4, 1) + N(4, 1), M(4, 2) + N(4, 2), M(4, 3) + N(4, 3), M(4, 4) + N(4, 4));
	}

	Matrix operator-(const Matrix &M, const Matrix &N)
	{
		return Matrix(M(1, 1) - N(1, 1), M(1, 2) - N(1, 2), M(1, 3) - N(1, 3), M(1, 4) - N(1, 4), 
		              M(2, 1) - N(2, 1), M(2, 2) - N(2, 2), M(2, 3) - N(2, 3), M(2, 4) - N(2, 4), 
		              M(3, 1) - N(3, 1), M(3, 2) - N(3, 2), M(3, 3) - N(3, 3), M(3, 4) - N(3, 4), 
		              M(4, 1) - N(4, 1), M(4, 2) - N(4, 2), M(4, 3) - N(4, 3), M(4, 4) - N(4, 4));
	}

	Matrix operator*(float s, const Matrix &M)
	{
		return Matrix(s * M(1, 1), s * M(1, 2), s * M(1, 3), s * M(1, 4), 
		              s * M(2, 1), s * M(2, 2), s * M(2, 3), s * M(2, 4), 
		              s * M(3, 1), s * M(3, 2), s * M(3, 3), s * M(3, 4), 
		              s * M(4, 1), s * M(4, 2), s * M(4, 3), s * M(4, 4));
	}

	Matrix operator*(const Matrix &M, float s)
	{
		return Matrix(M(1, 1) * s, M(1, 2) * s, M(1, 3) * s, M(1, 4) * s, 
		              M(2, 1) * s, M(2, 2) * s, M(2, 3) * s, M(2, 4) * s, 
		              M(3, 1) * s, M(3, 2) * s, M(3, 3) * s, M(3, 4) * s, 
		              M(4, 1) * s, M(4, 2) * s, M(4, 3) * s, M(4, 4) * s);
	}

	Matrix operator*(const Matrix &M, const Matrix &N)
	{
		return Matrix(M(1, 1) * N(1, 1) + M(1, 2) * N(2, 1) + M(1, 3) * N(3, 1) + M(1, 4) * N(4, 1), M(1, 1) * N(1, 2) + M(1, 2) * N(2, 2) + M(1, 3) * N(3, 2) + M(1, 4) * N(4, 2), M(1, 1) * N(1, 3) + M(1, 2) * N(2, 3) + M(1, 3) * N(3, 3) + M(1, 4) * N(4, 3), M(1, 1) * N(1, 4) + M(1, 2) * N(2, 4) + M(1, 3) * N(3, 4) + M(1, 4) * N(4, 4), 
		              M(2, 1) * N(1, 1) + M(2, 2) * N(2, 1) + M(2, 3) * N(3, 1) + M(2, 4) * N(4, 1), M(2, 1) * N(1, 2) + M(2, 2) * N(2, 2) + M(2, 3) * N(3, 2) + M(2, 4) * N(4, 2), M(2, 1) * N(1, 3) + M(2, 2) * N(2, 3) + M(2, 3) * N(3, 3) + M(2, 4) * N(4, 3), M(2, 1) * N(1, 4) + M(2, 2) * N(2, 4) + M(2, 3) * N(3, 4) + M(2, 4) * N(4, 4), 
		              M(3, 1) * N(1, 1) + M(3, 2) * N(2, 1) + M(3, 3) * N(3, 1) + M(3, 4) * N(4, 1), M(3, 1) * N(1, 2) + M(3, 2) * N(2, 2) + M(3, 3) * N(3, 2) + M(3, 4) * N(4, 2), M(3, 1) * N(1, 3) + M(3, 2) * N(2, 3) + M(3, 3) * N(3, 3) + M(3, 4) * N(4, 3), M(3, 1) * N(1, 4) + M(3, 2) * N(2, 4) + M(3, 3) * N(3, 4) + M(3, 4) * N(4, 4), 
		              M(4, 1) * N(1, 1) + M(4, 2) * N(2, 1) + M(4, 3) * N(3, 1) + M(4, 4) * N(4, 1), M(4, 1) * N(1, 2) + M(4, 2) * N(2, 2) + M(4, 3) * N(3, 2) + M(4, 4) * N(4, 2), M(4, 1) * N(1, 3) + M(4, 2) * N(2, 3) + M(4, 3) * N(3, 3) + M(4, 4) * N(4, 3), M(4, 1) * N(1, 4) + M(4, 2) * N(2, 4) + M(4, 3) * N(3, 4) + M(4, 4) * N(4, 4));
	}

	Matrix operator/(const Matrix &M, float s)
	{
		float r = 1.0f / s;

		return M * r;
	}

	float4 Matrix::operator*(const float4 &v) const
	{
		const Matrix &M = *this;
		float Mx = M(1, 1) * v.x + M(1, 2) * v.y + M(1, 3) * v.z + M(1, 4) * v.w;
		float My = M(2, 1) * v.x + M(2, 2) * v.y + M(2, 3) * v.z + M(2, 4) * v.w;
		float Mz = M(3, 1) * v.x + M(3, 2) * v.y + M(3, 3) * v.z + M(3, 4) * v.w;
		float Mw = M(4, 1) * v.x + M(4, 2) * v.y + M(4, 3) * v.z + M(4, 4) * v.w;

		return {Mx, My, Mz, Mw};
	}

	float Matrix::det(const Matrix &M)
	{
		float M3344 = M(3, 3) * M(4, 4) - M(4, 3) * M(3, 4);
		float M2344 = M(2, 3) * M(4, 4) - M(4, 3) * M(2, 4);
		float M2334 = M(2, 3) * M(3, 4) - M(3, 3) * M(2, 4);
		float M1344 = M(1, 3) * M(4, 4) - M(4, 3) * M(1, 4);
		float M1334 = M(1, 3) * M(3, 4) - M(3, 3) * M(1, 4);
		float M1324 = M(1, 3) * M(2, 4) - M(2, 3) * M(1, 4);

		return M(1, 1) * (M(2, 2) * M3344 - M(3, 2) * M2344 + M(4, 2) * M2334) -
		       M(2, 1) * (M(1, 2) * M3344 - M(3, 2) * M1344 + M(4, 2) * M1334) +
		       M(3, 1) * (M(1, 2) * M2344 - M(2, 2) * M1344 + M(4, 2) * M1324) -
		       M(4, 1) * (M(1, 2) * M2334 - M(2, 2) * M1334 + M(3, 2) * M1324);
	}

	float Matrix::det(float m11)
	{
		return m11;
	}

	float Matrix::det(float m11, float m12, 
	                  float m21, float m22)
	{
		return m11 * m22 - m12 * m21; 
	}

	float Matrix::det(float m11, float m12, float m13, 
	                  float m21, float m22, float m23, 
	                  float m31, float m32, float m33)
	{
		return m11 * (m22 * m33 - m32 * m23) -
		       m21 * (m12 * m33 - m32 * m13) +
		       m31 * (m12 * m23 - m22 * m13);
	}

	float Matrix::det(float m11, float m12, float m13, float m14, 
	                  float m21, float m22, float m23, float m24, 
	                  float m31, float m32, float m33, float m34, 
	                  float m41, float m42, float m43, float m44)
	{
		float M3344 = m33 * m44 - m43 * m34;
		float M2344 = m23 * m44 - m43 * m24;
		float M2334 = m23 * m34 - m33 * m24;
		float M1344 = m13 * m44 - m43 * m14;
		float M1334 = m13 * m34 - m33 * m14;
		float M1324 = m13 * m24 - m23 * m14;

		return m11 * (m22 * M3344 - m32 * M2344 + m42 * M2334) -
		       m21 * (m12 * M3344 - m32 * M1344 + m42 * M1334) +
		       m31 * (m12 * M2344 - m22 * M1344 + m42 * M1324) -
		       m41 * (m12 * M2334 - m22 * M1334 + m32 * M1324);
	}

	float Matrix::det(const Vector &v1, const Vector &v2, const Vector &v3)
	{
		return v1 * (v2 % v3);
	}

	float Matrix::det3(const Matrix &M)
	{
		return M(1, 1) * (M(2, 2) * M(3, 3) - M(3, 2) * M(2, 3)) -
		       M(2, 1) * (M(1, 2) * M(3, 3) - M(3, 2) * M(1, 3)) +
		       M(3, 1) * (M(1, 2) * M(2, 3) - M(2, 2) * M(1, 3));
	}

	float Matrix::tr(const Matrix &M)
	{
		return M(1, 1) + M(2, 2) + M(3, 3) + M(4, 4);
	}

	Matrix &Matrix::orthogonalise()
	{
		// NOTE: Numnerically instable, won't return exact the same result when already orhtogonal

		Matrix &M = *this;

		Vector v1(M(1, 1), M(2, 1), M(3, 1));
		Vector v2(M(1, 2), M(2, 2), M(3, 2));
		Vector v3(M(1, 3), M(2, 3), M(3, 3));

		v2 -= v1 * (v1 * v2) / (v1 * v1);
		v3 -= v1 * (v1 * v3) / (v1 * v1);
		v3 -= v2 * (v2 * v3) / (v2 * v2);

		v1 /= Vector::N(v1);
		v2 /= Vector::N(v2);
		v3 /= Vector::N(v3);

		M(1, 1) = v1.x;  M(1, 2) = v2.x;  M(1, 3) = v3.x;
		M(2, 1) = v1.y;  M(2, 2) = v2.y;  M(2, 3) = v3.y;
		M(3, 1) = v1.z;  M(3, 2) = v2.z;  M(3, 3) = v3.z;

		return *this;
	}

	Matrix Matrix::eulerRotate(const Vector &v)
	{
		float cz = cos(v.z);
		float sz = sin(v.z);
		float cx = cos(v.x);
		float sx = sin(v.x);
		float cy = cos(v.y);
		float sy = sin(v.y);

		float sxsy = sx * sy;
		float sxcy = sx * cy;

		return Matrix(cy * cz - sxsy * sz, -cy * sz - sxsy * cz, -sy * cx,
		              cx * sz,              cx * cz,             -sx,
		              sy * cz + sxcy * sz, -sy * sz + sxcy * cz,  cy * cx);
	}

	Matrix Matrix::eulerRotate(float x, float y, float z)
	{
		return eulerRotate(Vector(x, y, z));
	}

	Matrix Matrix::translate(const Vector &v)
	{
		return Matrix(1, 0, 0, v.x,
		              0, 1, 0, v.y,
		              0, 0, 1, v.z,
		              0, 0, 0, 1);
	}

	Matrix Matrix::translate(float x, float y, float z)
	{
		return translate(Vector(x, y, z));
	}

	Matrix Matrix::scale(const Vector &v)
	{
		return Matrix(v.x, 0,   0,
		              0,   v.y, 0,
		              0,   0,   v.z);
	}

	Matrix Matrix::scale(float x, float y, float z)
	{
		return scale(Vector(x, y, z));
	}

	Matrix Matrix::lookAt(const Vector &v)
	{
		Vector y = v;
		y /= Vector::N(y);

		Vector x = y % Vector(0, 0, 1);
		x /= Vector::N(x);

		Vector z = x % y;
		z /= Vector::N(z);

		return ~Matrix(x, y, z);
	}

	Matrix Matrix::lookAt(float x, float y, float z)
	{
		return translate(Vector(x, y, z));
	}
}
