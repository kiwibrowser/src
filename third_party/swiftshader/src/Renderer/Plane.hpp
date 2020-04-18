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

#ifndef Plane_hpp
#define Plane_hpp

#include "Vector.hpp"

namespace sw
{
	struct Matrix;

	struct Plane
	{
		float A;
		float B;
		float C;
		float D;

		Plane();
		Plane(float A, float B, float C, float D);   // Plane equation 
		Plane(const float ABCD[4]);

		friend Plane operator*(const Plane &p, const Matrix &A);   // Transform plane by matrix (post-multiply)
		friend Plane operator*(const Matrix &A, const Plane &p);   // Transform plane by matrix (pre-multiply)
	};
}

#endif   // Plane_hpp
