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

#include "Point.hpp"

#include "Matrix.hpp"

namespace sw
{
	Point &Point::operator+=(const Vector &v)
	{
		x += v.x;
		y += v.y;
		z += v.z;

		return *this;
	}

	Point &Point::operator-=(const Vector &v)
	{
		x -= v.x;
		y -= v.y;
		z -= v.z;

		return *this;
	}

	Point operator+(const Point &P, const Vector &v)
	{
		return Point(P.x + v.x, P.y + v.y, P.z + v.z);
	}

	Point operator-(const Point &P, const Vector &v)
	{
		return Point(P.x - v.x, P.y - v.y, P.z - v.z);
	}

	Vector operator-(const Point &P, const Point &Q)
	{
		return Vector(P.x - Q.x, P.y - Q.y, P.z - Q.z);
	}

	Point operator*(const Matrix &M, const Point &P)
	{
		return Point(M(1, 1) * P.x + M(1, 2) * P.y + M(1, 3) * P.z + M(1, 4),
		             M(2, 1) * P.x + M(2, 2) * P.y + M(2, 3) * P.z + M(2, 4),
		             M(3, 1) * P.x + M(3, 2) * P.y + M(3, 3) * P.z + M(3, 4));
	}

	Point operator*(const Point &P, const Matrix &M)
	{
		return Point(P.x * M(1, 1) + P.y * M(2, 1) + P.z * M(3, 1),
		             P.x * M(1, 2) + P.y * M(2, 2) + P.z * M(3, 2),
		             P.x * M(1, 3) + P.y * M(2, 3) + P.z * M(3, 3));
	}

	Point &operator*=(Point &P, const Matrix &M)
	{
		return P = P * M;
	}

	float Point::d(const Point &P) const
	{
		return Vector::N(*this - P);
	}

	float Point::d2(const Point &P) const
	{
		return Vector::N2(*this - P);
	}

	float Point::d(const Point &P, const Point &Q)
	{
		return Vector::N(P - Q);
	}

	float Point::d2(const Point &P, const Point &Q)
	{
		return Vector::N2(P - Q);
	}
}
