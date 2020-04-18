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

#ifndef Point_hpp
#define Point_hpp

namespace sw
{
	struct Vector;
	struct Matrix;

	struct Point
	{
		Point();
		Point(const int i);
		Point(const Point &P);
		Point(const Vector &v);
		Point(float Px, float Py, float Pz);

		Point &operator=(const Point &P);

		union
		{
			float p[3];

			struct
			{	
				float x;
				float y;
				float z;
			};
		};

		float &operator[](int i);
		float &operator()(int i);

		const float &operator[](int i) const;
		const float &operator()(int i) const;

		Point &operator+=(const Vector &v);
		Point &operator-=(const Vector &v);

		friend Point operator+(const Point &P, const Vector &v);
		friend Point operator-(const Point &P, const Vector &v);

		friend Vector operator-(const Point &P, const Point &Q);

		friend Point operator*(const Matrix &M, const Point& P);
		friend Point operator*(const Point &P, const Matrix &M);
		friend Point &operator*=(Point &P, const Matrix &M);

		float d(const Point &P) const;   // Distance between two points
		float d2(const Point &P) const;   // Squared distance between two points

		static float d(const Point &P, const Point &Q);   // Distance between two points
		static float d2(const Point &P, const Point &Q);   // Squared distance between two points
	};
}

#include "Vector.hpp"

namespace sw
{
	inline Point::Point()
	{
	}

	inline Point::Point(const int i)
	{
		const float s = (float)i;

		x = s;
		y = s;
		z = s;
	}

	inline Point::Point(const Point &P)
	{
		x = P.x;
		y = P.y;
		z = P.z;
	}

	inline Point::Point(const Vector &v)
	{
		x = v.x;
		y = v.y;
		z = v.z;
	}

	inline Point::Point(float P_x, float P_y, float P_z)
	{
		x = P_x;
		y = P_y;
		z = P_z;
	}

	inline Point &Point::operator=(const Point &P)
	{
		x = P.x;
		y = P.y;
		z = P.z;

		return *this;
	}

	inline float &Point::operator()(int i)
	{
		return p[i];
	}

	inline float &Point::operator[](int i)
	{
		return p[i];
	}

	inline const float &Point::operator()(int i) const
	{
		return p[i];
	}

	inline const float &Point::operator[](int i) const
	{
		return p[i];
	}
}

#endif   // Point_hpp
