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

#include "ShaderCore.hpp"

#include "Renderer/Renderer.hpp"
#include "Common/Debug.hpp"

#include <limits.h>

namespace sw
{
	extern TranscendentalPrecision logPrecision;
	extern TranscendentalPrecision expPrecision;
	extern TranscendentalPrecision rcpPrecision;
	extern TranscendentalPrecision rsqPrecision;

	Vector4s::Vector4s()
	{
	}

	Vector4s::Vector4s(unsigned short x, unsigned short y, unsigned short z, unsigned short w)
	{
		this->x = Short4(x);
		this->y = Short4(y);
		this->z = Short4(z);
		this->w = Short4(w);
	}

	Vector4s::Vector4s(const Vector4s &rhs)
	{
		x = rhs.x;
		y = rhs.y;
		z = rhs.z;
		w = rhs.w;
	}

	Vector4s &Vector4s::operator=(const Vector4s &rhs)
	{
		x = rhs.x;
		y = rhs.y;
		z = rhs.z;
		w = rhs.w;

		return *this;
	}

	Short4 &Vector4s::operator[](int i)
	{
		switch(i)
		{
		case 0: return x;
		case 1: return y;
		case 2: return z;
		case 3: return w;
		}

		return x;
	}

	Vector4f::Vector4f()
	{
	}

	Vector4f::Vector4f(float x, float y, float z, float w)
	{
		this->x = Float4(x);
		this->y = Float4(y);
		this->z = Float4(z);
		this->w = Float4(w);
	}

	Vector4f::Vector4f(const Vector4f &rhs)
	{
		x = rhs.x;
		y = rhs.y;
		z = rhs.z;
		w = rhs.w;
	}

	Vector4f &Vector4f::operator=(const Vector4f &rhs)
	{
		x = rhs.x;
		y = rhs.y;
		z = rhs.z;
		w = rhs.w;

		return *this;
	}

	Float4 &Vector4f::operator[](int i)
	{
		switch(i)
		{
		case 0: return x;
		case 1: return y;
		case 2: return z;
		case 3: return w;
		}

		return x;
	}

	Float4 exponential2(RValue<Float4> x, bool pp)
	{
		// This implementation is based on 2^(i + f) = 2^i * 2^f,
		// where i is the integer part of x and f is the fraction.

		// For 2^i we can put the integer part directly in the exponent of
		// the IEEE-754 floating-point number. Clamp to prevent overflow
		// past the representation of infinity.
		Float4 x0 = x;
		x0 = Min(x0, As<Float4>(Int4(0x43010000)));   // 129.00000e+0f
		x0 = Max(x0, As<Float4>(Int4(0xC2FDFFFF)));   // -126.99999e+0f

		Int4 i = RoundInt(x0 - Float4(0.5f));
		Float4 ii = As<Float4>((i + Int4(127)) << 23);   // Add single-precision bias, and shift into exponent.

		// For the fractional part use a polynomial
		// which approximates 2^f in the 0 to 1 range.
		Float4 f = x0 - Float4(i);
		Float4 ff = As<Float4>(Int4(0x3AF61905));     // 1.8775767e-3f
		ff = ff * f + As<Float4>(Int4(0x3C134806));   // 8.9893397e-3f
		ff = ff * f + As<Float4>(Int4(0x3D64AA23));   // 5.5826318e-2f
		ff = ff * f + As<Float4>(Int4(0x3E75EAD4));   // 2.4015361e-1f
		ff = ff * f + As<Float4>(Int4(0x3F31727B));   // 6.9315308e-1f
		ff = ff * f + Float4(1.0f);

		return ii * ff;
	}

	Float4 logarithm2(RValue<Float4> x, bool absolute, bool pp)
	{
		Float4 x0;
		Float4 x1;
		Float4 x2;
		Float4 x3;

		x0 = x;

		x1 = As<Float4>(As<Int4>(x0) & Int4(0x7F800000));
		x1 = As<Float4>(As<UInt4>(x1) >> 8);
		x1 = As<Float4>(As<Int4>(x1) | As<Int4>(Float4(1.0f)));
		x1 = (x1 - Float4(1.4960938f)) * Float4(256.0f);   // FIXME: (x1 - 1.4960938f) * 256.0f;
		x0 = As<Float4>((As<Int4>(x0) & Int4(0x007FFFFF)) | As<Int4>(Float4(1.0f)));

		x2 = (Float4(9.5428179e-2f) * x0 + Float4(4.7779095e-1f)) * x0 + Float4(1.9782813e-1f);
		x3 = ((Float4(1.6618466e-2f) * x0 + Float4(2.0350508e-1f)) * x0 + Float4(2.7382900e-1f)) * x0 + Float4(4.0496687e-2f);
		x2 /= x3;

		x1 += (x0 - Float4(1.0f)) * x2;

		Int4 pos_inf_x = CmpEQ(As<Int4>(x), Int4(0x7F800000));
		return As<Float4>((pos_inf_x & As<Int4>(x)) | (~pos_inf_x & As<Int4>(x1)));
	}

	Float4 exponential(RValue<Float4> x, bool pp)
	{
		// FIXME: Propagate the constant
		return exponential2(Float4(1.44269504f) * x, pp);   // 1/ln(2)
	}

	Float4 logarithm(RValue<Float4> x, bool absolute, bool pp)
	{
		// FIXME: Propagate the constant
		return Float4(6.93147181e-1f) * logarithm2(x, absolute, pp);   // ln(2)
	}

	Float4 power(RValue<Float4> x, RValue<Float4> y, bool pp)
	{
		Float4 log = logarithm2(x, true, pp);
		log *= y;
		return exponential2(log, pp);
	}

	Float4 reciprocal(RValue<Float4> x, bool pp, bool finite, bool exactAtPow2)
	{
		Float4 rcp;

		if(!pp && rcpPrecision >= WHQL)
		{
			rcp = Float4(1.0f) / x;
		}
		else
		{
			rcp = Rcp_pp(x, exactAtPow2);

			if(!pp)
			{
				rcp = (rcp + rcp) - (x * rcp * rcp);
			}
		}

		if(finite)
		{
			int big = 0x7F7FFFFF;
			rcp = Min(rcp, Float4((float&)big));
		}

		return rcp;
	}

	Float4 reciprocalSquareRoot(RValue<Float4> x, bool absolute, bool pp)
	{
		Float4 abs = x;

		if(absolute)
		{
			abs = Abs(abs);
		}

		Float4 rsq;

		if(!pp)
		{
			rsq = Float4(1.0f) / Sqrt(abs);
		}
		else
		{
			rsq = RcpSqrt_pp(abs);

			if(!pp)
			{
				rsq = rsq * (Float4(3.0f) - rsq * rsq * abs) * Float4(0.5f);
			}

			rsq = As<Float4>(CmpNEQ(As<Int4>(abs), Int4(0x7F800000)) & As<Int4>(rsq));
		}

		return rsq;
	}

	Float4 modulo(RValue<Float4> x, RValue<Float4> y)
	{
		return x - y * Floor(x / y);
	}

	Float4 sine_pi(RValue<Float4> x, bool pp)
	{
		const Float4 A = Float4(-4.05284734e-1f);   // -4/pi^2
		const Float4 B = Float4(1.27323954e+0f);    // 4/pi
		const Float4 C = Float4(7.75160950e-1f);
		const Float4 D = Float4(2.24839049e-1f);

		// Parabola approximating sine
		Float4 sin = x * (Abs(x) * A + B);

		// Improve precision from 0.06 to 0.001
		if(true)
		{
			sin = sin * (Abs(sin) * D + C);
		}

		return sin;
	}

	Float4 cosine_pi(RValue<Float4> x, bool pp)
	{
		// cos(x) = sin(x + pi/2)
		Float4 y = x + Float4(1.57079632e+0f);

		// Wrap around
		y -= As<Float4>(CmpNLT(y, Float4(3.14159265e+0f)) & As<Int4>(Float4(6.28318530e+0f)));

		return sine_pi(y, pp);
	}

	Float4 sine(RValue<Float4> x, bool pp)
	{
		// Reduce to [-0.5, 0.5] range
		Float4 y = x * Float4(1.59154943e-1f);   // 1/2pi
		y = y - Round(y);

		if(!pp)
		{
			// From the paper: "A Fast, Vectorizable Algorithm for Producing Single-Precision Sine-Cosine Pairs"
			// This implementation passes OpenGL ES 3.0 precision requirements, at the cost of more operations:
			// !pp : 17 mul, 7 add, 1 sub, 1 reciprocal
			//  pp : 4 mul, 2 add, 2 abs

			Float4 y2 = y * y;
			Float4 c1 = y2 * (y2 * (y2 * Float4(-0.0204391631f) + Float4(0.2536086171f)) + Float4(-1.2336977925f)) + Float4(1.0f);
			Float4 s1 = y * (y2 * (y2 * (y2 * Float4(-0.0046075748f) + Float4(0.0796819754f)) + Float4(-0.645963615f)) + Float4(1.5707963235f));
			Float4 c2 = (c1 * c1) - (s1 * s1);
			Float4 s2 = Float4(2.0f) * s1 * c1;
			return Float4(2.0f) * s2 * c2 * reciprocal(s2 * s2 + c2 * c2, pp, true);
		}

		const Float4 A = Float4(-16.0f);
		const Float4 B = Float4(8.0f);
		const Float4 C = Float4(7.75160950e-1f);
		const Float4 D = Float4(2.24839049e-1f);

		// Parabola approximating sine
		Float4 sin = y * (Abs(y) * A + B);

		// Improve precision from 0.06 to 0.001
		if(true)
		{
			sin = sin * (Abs(sin) * D + C);
		}

		return sin;
	}

	Float4 cosine(RValue<Float4> x, bool pp)
	{
		// cos(x) = sin(x + pi/2)
		Float4 y = x + Float4(1.57079632e+0f);
		return sine(y, pp);
	}

	Float4 tangent(RValue<Float4> x, bool pp)
	{
		return sine(x, pp) / cosine(x, pp);
	}

	Float4 arccos(RValue<Float4> x, bool pp)
	{
		// pi/2 - arcsin(x)
		return Float4(1.57079632e+0f) - arcsin(x);
	}

	Float4 arcsin(RValue<Float4> x, bool pp)
	{
		if(false) // Simpler implementation fails even lowp precision tests
		{
			// x*(pi/2-sqrt(1-x*x)*pi/5)
			return x * (Float4(1.57079632e+0f) - Sqrt(Float4(1.0f) - x*x) * Float4(6.28318531e-1f));
		}
		else
		{
			// From 4.4.45, page 81 of the Handbook of Mathematical Functions, by Milton Abramowitz and Irene Stegun
			const Float4 half_pi(1.57079632f);
			const Float4 a0(1.5707288f);
			const Float4 a1(-0.2121144f);
			const Float4 a2(0.0742610f);
			const Float4 a3(-0.0187293f);
			Float4 absx = Abs(x);
			return As<Float4>(As<Int4>(half_pi - Sqrt(Float4(1.0f) - absx) * (a0 + absx * (a1 + absx * (a2 + absx * a3)))) ^
			       (As<Int4>(x) & Int4(0x80000000)));
		}
	}

	// Approximation of atan in [0..1]
	Float4 arctan_01(Float4 x, bool pp)
	{
		if(pp)
		{
			return x * (Float4(-0.27f) * x + Float4(1.05539816f));
		}
		else
		{
			// From 4.4.49, page 81 of the Handbook of Mathematical Functions, by Milton Abramowitz and Irene Stegun
			const Float4 a2(-0.3333314528f);
			const Float4 a4(0.1999355085f);
			const Float4 a6(-0.1420889944f);
			const Float4 a8(0.1065626393f);
			const Float4 a10(-0.0752896400f);
			const Float4 a12(0.0429096138f);
			const Float4 a14(-0.0161657367f);
			const Float4 a16(0.0028662257f);
			Float4 x2 = x * x;
			return (x + x * (x2 * (a2 + x2 * (a4 + x2 * (a6 + x2 * (a8 + x2 * (a10 + x2 * (a12 + x2 * (a14 + x2 * a16)))))))));
		}
	}

	Float4 arctan(RValue<Float4> x, bool pp)
	{
		Float4 absx = Abs(x);
		Int4 O = CmpNLT(absx, Float4(1.0f));
		Float4 y = As<Float4>((O & As<Int4>(Float4(1.0f) / absx)) | (~O & As<Int4>(absx))); // FIXME: Vector select

		const Float4 half_pi(1.57079632f);
		Float4 theta = arctan_01(y, pp);
		return As<Float4>(((O & As<Int4>(half_pi - theta)) | (~O & As<Int4>(theta))) ^ // FIXME: Vector select
		       (As<Int4>(x) & Int4(0x80000000)));
	}

	Float4 arctan(RValue<Float4> y, RValue<Float4> x, bool pp)
	{
		const Float4 pi(3.14159265f);            // pi
		const Float4 minus_pi(-3.14159265f);     // -pi
		const Float4 half_pi(1.57079632f);       // pi/2
		const Float4 quarter_pi(7.85398163e-1f); // pi/4

		// Rotate to upper semicircle when in lower semicircle
		Int4 S = CmpLT(y, Float4(0.0f));
		Float4 theta = As<Float4>(S & As<Int4>(minus_pi));
		Float4 x0 = As<Float4>((As<Int4>(y) & Int4(0x80000000)) ^ As<Int4>(x));
		Float4 y0 = Abs(y);

		// Rotate to right quadrant when in left quadrant
		Int4 Q = CmpLT(x0, Float4(0.0f));
		theta += As<Float4>(Q & As<Int4>(half_pi));
		Float4 x1 = As<Float4>((Q & As<Int4>(y0)) | (~Q & As<Int4>(x0)));  // FIXME: Vector select
		Float4 y1 = As<Float4>((Q & As<Int4>(-x0)) | (~Q & As<Int4>(y0))); // FIXME: Vector select

		// Mirror to first octant when in second octant
		Int4 O = CmpNLT(y1, x1);
		Float4 x2 = As<Float4>((O & As<Int4>(y1)) | (~O & As<Int4>(x1))); // FIXME: Vector select
		Float4 y2 = As<Float4>((O & As<Int4>(x1)) | (~O & As<Int4>(y1))); // FIXME: Vector select

		// Approximation of atan in [0..1]
		Int4 zero_x = CmpEQ(x2, Float4(0.0f));
		Int4 inf_y = IsInf(y2); // Since x2 >= y2, this means x2 == y2 == inf, so we use 45 degrees or pi/4
		Float4 atan2_theta = arctan_01(y2 / x2, pp);
		theta += As<Float4>((~zero_x & ~inf_y & ((O & As<Int4>(half_pi - atan2_theta)) | (~O & (As<Int4>(atan2_theta))))) | // FIXME: Vector select
		                    (inf_y & As<Int4>(quarter_pi)));

		// Recover loss of precision for tiny theta angles
		Int4 precision_loss = S & Q & O & ~inf_y; // This combination results in (-pi + half_pi + half_pi - atan2_theta) which is equivalent to -atan2_theta
		return As<Float4>((precision_loss & As<Int4>(-atan2_theta)) | (~precision_loss & As<Int4>(theta))); // FIXME: Vector select
	}

	Float4 sineh(RValue<Float4> x, bool pp)
	{
		return (exponential(x, pp) - exponential(-x, pp)) * Float4(0.5f);
	}

	Float4 cosineh(RValue<Float4> x, bool pp)
	{
		return (exponential(x, pp) + exponential(-x, pp)) * Float4(0.5f);
	}

	Float4 tangenth(RValue<Float4> x, bool pp)
	{
		Float4 e_x = exponential(x, pp);
		Float4 e_minus_x = exponential(-x, pp);
		return (e_x - e_minus_x) / (e_x + e_minus_x);
	}

	Float4 arccosh(RValue<Float4> x, bool pp)
	{
		return logarithm(x + Sqrt(x + Float4(1.0f)) * Sqrt(x - Float4(1.0f)), pp);
	}

	Float4 arcsinh(RValue<Float4> x, bool pp)
	{
		return logarithm(x + Sqrt(x * x + Float4(1.0f)), pp);
	}

	Float4 arctanh(RValue<Float4> x, bool pp)
	{
		return logarithm((Float4(1.0f) + x) / (Float4(1.0f) - x), pp) * Float4(0.5f);
	}

	Float4 dot2(const Vector4f &v0, const Vector4f &v1)
	{
		return v0.x * v1.x + v0.y * v1.y;
	}

	Float4 dot3(const Vector4f &v0, const Vector4f &v1)
	{
		return v0.x * v1.x + v0.y * v1.y + v0.z * v1.z;
	}

	Float4 dot4(const Vector4f &v0, const Vector4f &v1)
	{
		return v0.x * v1.x + v0.y * v1.y + v0.z * v1.z + v0.w * v1.w;
	}

	void transpose4x4(Short4 &row0, Short4 &row1, Short4 &row2, Short4 &row3)
	{
		Int2 tmp0 = UnpackHigh(row0, row1);
		Int2 tmp1 = UnpackHigh(row2, row3);
		Int2 tmp2 = UnpackLow(row0, row1);
		Int2 tmp3 = UnpackLow(row2, row3);

		row0 = UnpackLow(tmp2, tmp3);
		row1 = UnpackHigh(tmp2, tmp3);
		row2 = UnpackLow(tmp0, tmp1);
		row3 = UnpackHigh(tmp0, tmp1);
	}

	void transpose4x3(Short4 &row0, Short4 &row1, Short4 &row2, Short4 &row3)
	{
		Int2 tmp0 = UnpackHigh(row0, row1);
		Int2 tmp1 = UnpackHigh(row2, row3);
		Int2 tmp2 = UnpackLow(row0, row1);
		Int2 tmp3 = UnpackLow(row2, row3);

		row0 = UnpackLow(tmp2, tmp3);
		row1 = UnpackHigh(tmp2, tmp3);
		row2 = UnpackLow(tmp0, tmp1);
	}

	void transpose4x4(Float4 &row0, Float4 &row1, Float4 &row2, Float4 &row3)
	{
		Float4 tmp0 = UnpackLow(row0, row1);
		Float4 tmp1 = UnpackLow(row2, row3);
		Float4 tmp2 = UnpackHigh(row0, row1);
		Float4 tmp3 = UnpackHigh(row2, row3);

		row0 = Float4(tmp0.xy, tmp1.xy);
		row1 = Float4(tmp0.zw, tmp1.zw);
		row2 = Float4(tmp2.xy, tmp3.xy);
		row3 = Float4(tmp2.zw, tmp3.zw);
	}

	void transpose4x3(Float4 &row0, Float4 &row1, Float4 &row2, Float4 &row3)
	{
		Float4 tmp0 = UnpackLow(row0, row1);
		Float4 tmp1 = UnpackLow(row2, row3);
		Float4 tmp2 = UnpackHigh(row0, row1);
		Float4 tmp3 = UnpackHigh(row2, row3);

		row0 = Float4(tmp0.xy, tmp1.xy);
		row1 = Float4(tmp0.zw, tmp1.zw);
		row2 = Float4(tmp2.xy, tmp3.xy);
	}

	void transpose4x2(Float4 &row0, Float4 &row1, Float4 &row2, Float4 &row3)
	{
		Float4 tmp0 = UnpackLow(row0, row1);
		Float4 tmp1 = UnpackLow(row2, row3);

		row0 = Float4(tmp0.xy, tmp1.xy);
		row1 = Float4(tmp0.zw, tmp1.zw);
	}

	void transpose4x1(Float4 &row0, Float4 &row1, Float4 &row2, Float4 &row3)
	{
		Float4 tmp0 = UnpackLow(row0, row1);
		Float4 tmp1 = UnpackLow(row2, row3);

		row0 = Float4(tmp0.xy, tmp1.xy);
	}

	void transpose2x4(Float4 &row0, Float4 &row1, Float4 &row2, Float4 &row3)
	{
		Float4 tmp01 = UnpackLow(row0, row1);
		Float4 tmp23 = UnpackHigh(row0, row1);

		row0 = tmp01;
		row1 = Float4(tmp01.zw, row1.zw);
		row2 = tmp23;
		row3 = Float4(tmp23.zw, row3.zw);
	}

	void transpose4xN(Float4 &row0, Float4 &row1, Float4 &row2, Float4 &row3, int N)
	{
		switch(N)
		{
		case 1: transpose4x1(row0, row1, row2, row3); break;
		case 2: transpose4x2(row0, row1, row2, row3); break;
		case 3: transpose4x3(row0, row1, row2, row3); break;
		case 4: transpose4x4(row0, row1, row2, row3); break;
		}
	}

	void ShaderCore::mov(Vector4f &dst, const Vector4f &src, bool integerDestination)
	{
		if(integerDestination)
		{
			dst.x = As<Float4>(RoundInt(src.x));
			dst.y = As<Float4>(RoundInt(src.y));
			dst.z = As<Float4>(RoundInt(src.z));
			dst.w = As<Float4>(RoundInt(src.w));
		}
		else
		{
			dst = src;
		}
	}

	void ShaderCore::neg(Vector4f &dst, const Vector4f &src)
	{
		dst.x = -src.x;
		dst.y = -src.y;
		dst.z = -src.z;
		dst.w = -src.w;
	}

	void ShaderCore::ineg(Vector4f &dst, const Vector4f &src)
	{
		dst.x = As<Float4>(-As<Int4>(src.x));
		dst.y = As<Float4>(-As<Int4>(src.y));
		dst.z = As<Float4>(-As<Int4>(src.z));
		dst.w = As<Float4>(-As<Int4>(src.w));
	}

	void ShaderCore::f2b(Vector4f &dst, const Vector4f &src)
	{
		dst.x = As<Float4>(CmpNEQ(src.x, Float4(0.0f)));
		dst.y = As<Float4>(CmpNEQ(src.y, Float4(0.0f)));
		dst.z = As<Float4>(CmpNEQ(src.z, Float4(0.0f)));
		dst.w = As<Float4>(CmpNEQ(src.w, Float4(0.0f)));
	}

	void ShaderCore::b2f(Vector4f &dst, const Vector4f &src)
	{
		dst.x = As<Float4>(As<Int4>(src.x) & As<Int4>(Float4(1.0f)));
		dst.y = As<Float4>(As<Int4>(src.y) & As<Int4>(Float4(1.0f)));
		dst.z = As<Float4>(As<Int4>(src.z) & As<Int4>(Float4(1.0f)));
		dst.w = As<Float4>(As<Int4>(src.w) & As<Int4>(Float4(1.0f)));
	}

	void ShaderCore::f2i(Vector4f &dst, const Vector4f &src)
	{
		dst.x = As<Float4>(Int4(src.x));
		dst.y = As<Float4>(Int4(src.y));
		dst.z = As<Float4>(Int4(src.z));
		dst.w = As<Float4>(Int4(src.w));
	}

	void ShaderCore::i2f(Vector4f &dst, const Vector4f &src)
	{
		dst.x = Float4(As<Int4>(src.x));
		dst.y = Float4(As<Int4>(src.y));
		dst.z = Float4(As<Int4>(src.z));
		dst.w = Float4(As<Int4>(src.w));
	}

	void ShaderCore::f2u(Vector4f &dst, const Vector4f &src)
	{
		dst.x = As<Float4>(UInt4(src.x));
		dst.y = As<Float4>(UInt4(src.y));
		dst.z = As<Float4>(UInt4(src.z));
		dst.w = As<Float4>(UInt4(src.w));
	}

	void ShaderCore::u2f(Vector4f &dst, const Vector4f &src)
	{
		dst.x = Float4(As<UInt4>(src.x));
		dst.y = Float4(As<UInt4>(src.y));
		dst.z = Float4(As<UInt4>(src.z));
		dst.w = Float4(As<UInt4>(src.w));
	}

	void ShaderCore::i2b(Vector4f &dst, const Vector4f &src)
	{
		dst.x = As<Float4>(CmpNEQ(As<Int4>(src.x), Int4(0)));
		dst.y = As<Float4>(CmpNEQ(As<Int4>(src.y), Int4(0)));
		dst.z = As<Float4>(CmpNEQ(As<Int4>(src.z), Int4(0)));
		dst.w = As<Float4>(CmpNEQ(As<Int4>(src.w), Int4(0)));
	}

	void ShaderCore::b2i(Vector4f &dst, const Vector4f &src)
	{
		dst.x = As<Float4>(As<Int4>(src.x) & Int4(1));
		dst.y = As<Float4>(As<Int4>(src.y) & Int4(1));
		dst.z = As<Float4>(As<Int4>(src.z) & Int4(1));
		dst.w = As<Float4>(As<Int4>(src.w) & Int4(1));
	}

	void ShaderCore::add(Vector4f &dst, const Vector4f &src0, const Vector4f &src1)
	{
		dst.x = src0.x + src1.x;
		dst.y = src0.y + src1.y;
		dst.z = src0.z + src1.z;
		dst.w = src0.w + src1.w;
	}

	void ShaderCore::iadd(Vector4f &dst, const Vector4f &src0, const Vector4f &src1)
	{
		dst.x = As<Float4>(As<Int4>(src0.x) + As<Int4>(src1.x));
		dst.y = As<Float4>(As<Int4>(src0.y) + As<Int4>(src1.y));
		dst.z = As<Float4>(As<Int4>(src0.z) + As<Int4>(src1.z));
		dst.w = As<Float4>(As<Int4>(src0.w) + As<Int4>(src1.w));
	}

	void ShaderCore::sub(Vector4f &dst, const Vector4f &src0, const Vector4f &src1)
	{
		dst.x = src0.x - src1.x;
		dst.y = src0.y - src1.y;
		dst.z = src0.z - src1.z;
		dst.w = src0.w - src1.w;
	}

	void ShaderCore::isub(Vector4f &dst, const Vector4f &src0, const Vector4f &src1)
	{
		dst.x = As<Float4>(As<Int4>(src0.x) - As<Int4>(src1.x));
		dst.y = As<Float4>(As<Int4>(src0.y) - As<Int4>(src1.y));
		dst.z = As<Float4>(As<Int4>(src0.z) - As<Int4>(src1.z));
		dst.w = As<Float4>(As<Int4>(src0.w) - As<Int4>(src1.w));
	}

	void ShaderCore::mad(Vector4f &dst, const Vector4f &src0, const Vector4f &src1, const Vector4f &src2)
	{
		dst.x = src0.x * src1.x + src2.x;
		dst.y = src0.y * src1.y + src2.y;
		dst.z = src0.z * src1.z + src2.z;
		dst.w = src0.w * src1.w + src2.w;
	}

	void ShaderCore::imad(Vector4f &dst, const Vector4f &src0, const Vector4f &src1, const Vector4f &src2)
	{
		dst.x = As<Float4>(As<Int4>(src0.x) * As<Int4>(src1.x) + As<Int4>(src2.x));
		dst.y = As<Float4>(As<Int4>(src0.y) * As<Int4>(src1.y) + As<Int4>(src2.y));
		dst.z = As<Float4>(As<Int4>(src0.z) * As<Int4>(src1.z) + As<Int4>(src2.z));
		dst.w = As<Float4>(As<Int4>(src0.w) * As<Int4>(src1.w) + As<Int4>(src2.w));
	}

	void ShaderCore::mul(Vector4f &dst, const Vector4f &src0, const Vector4f &src1)
	{
		dst.x = src0.x * src1.x;
		dst.y = src0.y * src1.y;
		dst.z = src0.z * src1.z;
		dst.w = src0.w * src1.w;
	}

	void ShaderCore::imul(Vector4f &dst, const Vector4f &src0, const Vector4f &src1)
	{
		dst.x = As<Float4>(As<Int4>(src0.x) * As<Int4>(src1.x));
		dst.y = As<Float4>(As<Int4>(src0.y) * As<Int4>(src1.y));
		dst.z = As<Float4>(As<Int4>(src0.z) * As<Int4>(src1.z));
		dst.w = As<Float4>(As<Int4>(src0.w) * As<Int4>(src1.w));
	}

	void ShaderCore::rcpx(Vector4f &dst, const Vector4f &src, bool pp)
	{
		Float4 rcp = reciprocal(src.x, pp, true, true);

		dst.x = rcp;
		dst.y = rcp;
		dst.z = rcp;
		dst.w = rcp;
	}

	void ShaderCore::div(Vector4f &dst, const Vector4f &src0, const Vector4f &src1)
	{
		dst.x = src0.x / src1.x;
		dst.y = src0.y / src1.y;
		dst.z = src0.z / src1.z;
		dst.w = src0.w / src1.w;
	}

	void ShaderCore::idiv(Vector4f &dst, const Vector4f &src0, const Vector4f &src1)
	{
		Float4 intMax(As<Float4>(Int4(INT_MAX)));
		cmp0i(dst.x, src1.x, intMax, src1.x);
		dst.x = As<Float4>(As<Int4>(src0.x) / As<Int4>(dst.x));
		cmp0i(dst.y, src1.y, intMax, src1.y);
		dst.y = As<Float4>(As<Int4>(src0.y) / As<Int4>(dst.y));
		cmp0i(dst.z, src1.z, intMax, src1.z);
		dst.z = As<Float4>(As<Int4>(src0.z) / As<Int4>(dst.z));
		cmp0i(dst.w, src1.w, intMax, src1.w);
		dst.w = As<Float4>(As<Int4>(src0.w) / As<Int4>(dst.w));
	}

	void ShaderCore::udiv(Vector4f &dst, const Vector4f &src0, const Vector4f &src1)
	{
		Float4 uintMax(As<Float4>(UInt4(UINT_MAX)));
		cmp0i(dst.x, src1.x, uintMax, src1.x);
		dst.x = As<Float4>(As<UInt4>(src0.x) / As<UInt4>(dst.x));
		cmp0i(dst.y, src1.y, uintMax, src1.y);
		dst.y = As<Float4>(As<UInt4>(src0.y) / As<UInt4>(dst.y));
		cmp0i(dst.z, src1.z, uintMax, src1.z);
		dst.z = As<Float4>(As<UInt4>(src0.z) / As<UInt4>(dst.z));
		cmp0i(dst.w, src1.w, uintMax, src1.w);
		dst.w = As<Float4>(As<UInt4>(src0.w) / As<UInt4>(dst.w));
	}

	void ShaderCore::mod(Vector4f &dst, const Vector4f &src0, const Vector4f &src1)
	{
		dst.x = modulo(src0.x, src1.x);
		dst.y = modulo(src0.y, src1.y);
		dst.z = modulo(src0.z, src1.z);
		dst.w = modulo(src0.w, src1.w);
	}

	void ShaderCore::imod(Vector4f &dst, const Vector4f &src0, const Vector4f &src1)
	{
		Float4 intMax(As<Float4>(Int4(INT_MAX)));
		cmp0i(dst.x, src1.x, intMax, src1.x);
		dst.x = As<Float4>(As<Int4>(src0.x) % As<Int4>(dst.x));
		cmp0i(dst.y, src1.y, intMax, src1.y);
		dst.y = As<Float4>(As<Int4>(src0.y) % As<Int4>(dst.y));
		cmp0i(dst.z, src1.z, intMax, src1.z);
		dst.z = As<Float4>(As<Int4>(src0.z) % As<Int4>(dst.z));
		cmp0i(dst.w, src1.w, intMax, src1.w);
		dst.w = As<Float4>(As<Int4>(src0.w) % As<Int4>(dst.w));
	}

	void ShaderCore::umod(Vector4f &dst, const Vector4f &src0, const Vector4f &src1)
	{
		Float4 uintMax(As<Float4>(UInt4(UINT_MAX)));
		cmp0i(dst.x, src1.x, uintMax, src1.x);
		dst.x = As<Float4>(As<UInt4>(src0.x) % As<UInt4>(dst.x));
		cmp0i(dst.y, src1.y, uintMax, src1.y);
		dst.y = As<Float4>(As<UInt4>(src0.y) % As<UInt4>(dst.y));
		cmp0i(dst.z, src1.z, uintMax, src1.z);
		dst.z = As<Float4>(As<UInt4>(src0.z) % As<UInt4>(dst.z));
		cmp0i(dst.w, src1.w, uintMax, src1.w);
		dst.w = As<Float4>(As<UInt4>(src0.w) % As<UInt4>(dst.w));
	}

	void ShaderCore::shl(Vector4f &dst, const Vector4f &src0, const Vector4f &src1)
	{
		dst.x = As<Float4>(As<Int4>(src0.x) << As<Int4>(src1.x));
		dst.y = As<Float4>(As<Int4>(src0.y) << As<Int4>(src1.y));
		dst.z = As<Float4>(As<Int4>(src0.z) << As<Int4>(src1.z));
		dst.w = As<Float4>(As<Int4>(src0.w) << As<Int4>(src1.w));
	}

	void ShaderCore::ishr(Vector4f &dst, const Vector4f &src0, const Vector4f &src1)
	{
		dst.x = As<Float4>(As<Int4>(src0.x) >> As<Int4>(src1.x));
		dst.y = As<Float4>(As<Int4>(src0.y) >> As<Int4>(src1.y));
		dst.z = As<Float4>(As<Int4>(src0.z) >> As<Int4>(src1.z));
		dst.w = As<Float4>(As<Int4>(src0.w) >> As<Int4>(src1.w));
	}

	void ShaderCore::ushr(Vector4f &dst, const Vector4f &src0, const Vector4f &src1)
	{
		dst.x = As<Float4>(As<UInt4>(src0.x) >> As<UInt4>(src1.x));
		dst.y = As<Float4>(As<UInt4>(src0.y) >> As<UInt4>(src1.y));
		dst.z = As<Float4>(As<UInt4>(src0.z) >> As<UInt4>(src1.z));
		dst.w = As<Float4>(As<UInt4>(src0.w) >> As<UInt4>(src1.w));
	}

	void ShaderCore::rsqx(Vector4f &dst, const Vector4f &src, bool pp)
	{
		Float4 rsq = reciprocalSquareRoot(src.x, true, pp);

		dst.x = rsq;
		dst.y = rsq;
		dst.z = rsq;
		dst.w = rsq;
	}

	void ShaderCore::sqrt(Vector4f &dst, const Vector4f &src, bool pp)
	{
		dst.x = Sqrt(src.x);
		dst.y = Sqrt(src.y);
		dst.z = Sqrt(src.z);
		dst.w = Sqrt(src.w);
	}

	void ShaderCore::rsq(Vector4f &dst, const Vector4f &src, bool pp)
	{
		dst.x = reciprocalSquareRoot(src.x, false, pp);
		dst.y = reciprocalSquareRoot(src.y, false, pp);
		dst.z = reciprocalSquareRoot(src.z, false, pp);
		dst.w = reciprocalSquareRoot(src.w, false, pp);
	}

	void ShaderCore::len2(Float4 &dst, const Vector4f &src, bool pp)
	{
		dst = Sqrt(dot2(src, src));
	}

	void ShaderCore::len3(Float4 &dst, const Vector4f &src, bool pp)
	{
		dst = Sqrt(dot3(src, src));
	}

	void ShaderCore::len4(Float4 &dst, const Vector4f &src, bool pp)
	{
		dst = Sqrt(dot4(src, src));
	}

	void ShaderCore::dist1(Float4 &dst, const Vector4f &src0, const Vector4f &src1, bool pp)
	{
		dst = Abs(src0.x - src1.x);
	}

	void ShaderCore::dist2(Float4 &dst, const Vector4f &src0, const Vector4f &src1, bool pp)
	{
		Float4 dx = src0.x - src1.x;
		Float4 dy = src0.y - src1.y;
		Float4 dot2 = dx * dx + dy * dy;
		dst = Sqrt(dot2);
	}

	void ShaderCore::dist3(Float4 &dst, const Vector4f &src0, const Vector4f &src1, bool pp)
	{
		Float4 dx = src0.x - src1.x;
		Float4 dy = src0.y - src1.y;
		Float4 dz = src0.z - src1.z;
		Float4 dot3 = dx * dx + dy * dy + dz * dz;
		dst = Sqrt(dot3);
	}

	void ShaderCore::dist4(Float4 &dst, const Vector4f &src0, const Vector4f &src1, bool pp)
	{
		Float4 dx = src0.x - src1.x;
		Float4 dy = src0.y - src1.y;
		Float4 dz = src0.z - src1.z;
		Float4 dw = src0.w - src1.w;
		Float4 dot4 = dx * dx + dy * dy + dz * dz + dw * dw;
		dst = Sqrt(dot4);
	}

	void ShaderCore::dp1(Vector4f &dst, const Vector4f &src0, const Vector4f &src1)
	{
		Float4 t = src0.x * src1.x;

		dst.x = t;
		dst.y = t;
		dst.z = t;
		dst.w = t;
	}

	void ShaderCore::dp2(Vector4f &dst, const Vector4f &src0, const Vector4f &src1)
	{
		Float4 t = dot2(src0, src1);

		dst.x = t;
		dst.y = t;
		dst.z = t;
		dst.w = t;
	}

	void ShaderCore::dp2add(Vector4f &dst, const Vector4f &src0, const Vector4f &src1, const Vector4f &src2)
	{
		Float4 t = dot2(src0, src1) + src2.x;

		dst.x = t;
		dst.y = t;
		dst.z = t;
		dst.w = t;
	}

	void ShaderCore::dp3(Vector4f &dst, const Vector4f &src0, const Vector4f &src1)
	{
		Float4 dot = dot3(src0, src1);

		dst.x = dot;
		dst.y = dot;
		dst.z = dot;
		dst.w = dot;
	}

	void ShaderCore::dp4(Vector4f &dst, const Vector4f &src0, const Vector4f &src1)
	{
		Float4 dot = dot4(src0, src1);

		dst.x = dot;
		dst.y = dot;
		dst.z = dot;
		dst.w = dot;
	}

	void ShaderCore::min(Vector4f &dst, const Vector4f &src0, const Vector4f &src1)
	{
		dst.x = Min(src0.x, src1.x);
		dst.y = Min(src0.y, src1.y);
		dst.z = Min(src0.z, src1.z);
		dst.w = Min(src0.w, src1.w);
	}

	void ShaderCore::imin(Vector4f &dst, const Vector4f &src0, const Vector4f &src1)
	{
		dst.x = As<Float4>(Min(As<Int4>(src0.x), As<Int4>(src1.x)));
		dst.y = As<Float4>(Min(As<Int4>(src0.y), As<Int4>(src1.y)));
		dst.z = As<Float4>(Min(As<Int4>(src0.z), As<Int4>(src1.z)));
		dst.w = As<Float4>(Min(As<Int4>(src0.w), As<Int4>(src1.w)));
	}

	void ShaderCore::umin(Vector4f &dst, const Vector4f &src0, const Vector4f &src1)
	{
		dst.x = As<Float4>(Min(As<UInt4>(src0.x), As<UInt4>(src1.x)));
		dst.y = As<Float4>(Min(As<UInt4>(src0.y), As<UInt4>(src1.y)));
		dst.z = As<Float4>(Min(As<UInt4>(src0.z), As<UInt4>(src1.z)));
		dst.w = As<Float4>(Min(As<UInt4>(src0.w), As<UInt4>(src1.w)));
	}

	void ShaderCore::max(Vector4f &dst, const Vector4f &src0, const Vector4f &src1)
	{
		dst.x = Max(src0.x, src1.x);
		dst.y = Max(src0.y, src1.y);
		dst.z = Max(src0.z, src1.z);
		dst.w = Max(src0.w, src1.w);
	}

	void ShaderCore::imax(Vector4f &dst, const Vector4f &src0, const Vector4f &src1)
	{
		dst.x = As<Float4>(Max(As<Int4>(src0.x), As<Int4>(src1.x)));
		dst.y = As<Float4>(Max(As<Int4>(src0.y), As<Int4>(src1.y)));
		dst.z = As<Float4>(Max(As<Int4>(src0.z), As<Int4>(src1.z)));
		dst.w = As<Float4>(Max(As<Int4>(src0.w), As<Int4>(src1.w)));
	}

	void ShaderCore::umax(Vector4f &dst, const Vector4f &src0, const Vector4f &src1)
	{
		dst.x = As<Float4>(Max(As<Int4>(src0.x), As<Int4>(src1.x)));
		dst.y = As<Float4>(Max(As<Int4>(src0.y), As<Int4>(src1.y)));
		dst.z = As<Float4>(Max(As<Int4>(src0.z), As<Int4>(src1.z)));
		dst.w = As<Float4>(Max(As<Int4>(src0.w), As<Int4>(src1.w)));
	}

	void ShaderCore::slt(Vector4f &dst, const Vector4f &src0, const Vector4f &src1)
	{
		dst.x = As<Float4>(As<Int4>(CmpLT(src0.x, src1.x)) & As<Int4>(Float4(1.0f)));
		dst.y = As<Float4>(As<Int4>(CmpLT(src0.y, src1.y)) & As<Int4>(Float4(1.0f)));
		dst.z = As<Float4>(As<Int4>(CmpLT(src0.z, src1.z)) & As<Int4>(Float4(1.0f)));
		dst.w = As<Float4>(As<Int4>(CmpLT(src0.w, src1.w)) & As<Int4>(Float4(1.0f)));
	}

	void ShaderCore::step(Vector4f &dst, const Vector4f &edge, const Vector4f &x)
	{
		dst.x = As<Float4>(CmpNLT(x.x, edge.x) & As<Int4>(Float4(1.0f)));
		dst.y = As<Float4>(CmpNLT(x.y, edge.y) & As<Int4>(Float4(1.0f)));
		dst.z = As<Float4>(CmpNLT(x.z, edge.z) & As<Int4>(Float4(1.0f)));
		dst.w = As<Float4>(CmpNLT(x.w, edge.w) & As<Int4>(Float4(1.0f)));
	}

	void ShaderCore::exp2x(Vector4f &dst, const Vector4f &src, bool pp)
	{
		Float4 exp = exponential2(src.x, pp);

		dst.x = exp;
		dst.y = exp;
		dst.z = exp;
		dst.w = exp;
	}

	void ShaderCore::exp2(Vector4f &dst, const Vector4f &src, bool pp)
	{
		dst.x = exponential2(src.x, pp);
		dst.y = exponential2(src.y, pp);
		dst.z = exponential2(src.z, pp);
		dst.w = exponential2(src.w, pp);
	}

	void ShaderCore::exp(Vector4f &dst, const Vector4f &src, bool pp)
	{
		dst.x = exponential(src.x, pp);
		dst.y = exponential(src.y, pp);
		dst.z = exponential(src.z, pp);
		dst.w = exponential(src.w, pp);
	}

	void ShaderCore::log2x(Vector4f &dst, const Vector4f &src, bool pp)
	{
		Float4 log = logarithm2(src.x, true, pp);

		dst.x = log;
		dst.y = log;
		dst.z = log;
		dst.w = log;
	}

	void ShaderCore::log2(Vector4f &dst, const Vector4f &src, bool pp)
	{
		dst.x = logarithm2(src.x, false, pp);
		dst.y = logarithm2(src.y, false, pp);
		dst.z = logarithm2(src.z, false, pp);
		dst.w = logarithm2(src.w, false, pp);
	}

	void ShaderCore::log(Vector4f &dst, const Vector4f &src, bool pp)
	{
		dst.x = logarithm(src.x, false, pp);
		dst.y = logarithm(src.y, false, pp);
		dst.z = logarithm(src.z, false, pp);
		dst.w = logarithm(src.w, false, pp);
	}

	void ShaderCore::lit(Vector4f &dst, const Vector4f &src)
	{
		dst.x = Float4(1.0f);
		dst.y = Max(src.x, Float4(0.0f));

		Float4 pow;

		pow = src.w;
		pow = Min(pow, Float4(127.9961f));
		pow = Max(pow, Float4(-127.9961f));

		dst.z = power(src.y, pow);
		dst.z = As<Float4>(As<Int4>(dst.z) & CmpNLT(src.x, Float4(0.0f)));
		dst.z = As<Float4>(As<Int4>(dst.z) & CmpNLT(src.y, Float4(0.0f)));

		dst.w = Float4(1.0f);
	}

	void ShaderCore::att(Vector4f &dst, const Vector4f &src0, const Vector4f &src1)
	{
		// Computes attenuation factors (1, d, d^2, 1/d) assuming src0 = d^2, src1 = 1/d
		dst.x = 1;
		dst.y = src0.y * src1.y;
		dst.z = src0.z;
		dst.w = src1.w;
	}

	void ShaderCore::lrp(Vector4f &dst, const Vector4f &src0, const Vector4f &src1, const Vector4f &src2)
	{
		dst.x = src0.x * (src1.x - src2.x) + src2.x;
		dst.y = src0.y * (src1.y - src2.y) + src2.y;
		dst.z = src0.z * (src1.z - src2.z) + src2.z;
		dst.w = src0.w * (src1.w - src2.w) + src2.w;
	}

	void ShaderCore::isinf(Vector4f &dst, const Vector4f &src)
	{
		dst.x = As<Float4>(IsInf(src.x));
		dst.y = As<Float4>(IsInf(src.y));
		dst.z = As<Float4>(IsInf(src.z));
		dst.w = As<Float4>(IsInf(src.w));
	}

	void ShaderCore::isnan(Vector4f &dst, const Vector4f &src)
	{
		dst.x = As<Float4>(IsNan(src.x));
		dst.y = As<Float4>(IsNan(src.y));
		dst.z = As<Float4>(IsNan(src.z));
		dst.w = As<Float4>(IsNan(src.w));
	}

	void ShaderCore::smooth(Vector4f &dst, const Vector4f &edge0, const Vector4f &edge1, const Vector4f &x)
	{
		Float4 tx = Min(Max((x.x - edge0.x) / (edge1.x - edge0.x), Float4(0.0f)), Float4(1.0f)); dst.x = tx * tx * (Float4(3.0f) - Float4(2.0f) * tx);
		Float4 ty = Min(Max((x.y - edge0.y) / (edge1.y - edge0.y), Float4(0.0f)), Float4(1.0f)); dst.y = ty * ty * (Float4(3.0f) - Float4(2.0f) * ty);
		Float4 tz = Min(Max((x.z - edge0.z) / (edge1.z - edge0.z), Float4(0.0f)), Float4(1.0f)); dst.z = tz * tz * (Float4(3.0f) - Float4(2.0f) * tz);
		Float4 tw = Min(Max((x.w - edge0.w) / (edge1.w - edge0.w), Float4(0.0f)), Float4(1.0f)); dst.w = tw * tw * (Float4(3.0f) - Float4(2.0f) * tw);
	}

	void ShaderCore::floatToHalfBits(Float4& dst, const Float4& floatBits, bool storeInUpperBits)
	{
		static const uint32_t mask_sign = 0x80000000u;
		static const uint32_t mask_round = ~0xfffu;
		static const uint32_t c_f32infty = 255 << 23;
		static const uint32_t c_magic = 15 << 23;
		static const uint32_t c_nanbit = 0x200;
		static const uint32_t c_infty_as_fp16 = 0x7c00;
		static const uint32_t c_clamp = (31 << 23) - 0x1000;

		UInt4 justsign = UInt4(mask_sign) & As<UInt4>(floatBits);
		UInt4 absf = As<UInt4>(floatBits) ^ justsign;
		UInt4 b_isnormal = CmpNLE(UInt4(c_f32infty), absf);

		// Note: this version doesn't round to the nearest even in case of a tie as defined by IEEE 754-2008, it rounds to +inf
		//       instead of nearest even, since that's fine for GLSL ES 3.0's needs (see section 2.1.1 Floating-Point Computation)
		UInt4 joined = ((((As<UInt4>(Min(As<Float4>(absf & UInt4(mask_round)) * As<Float4>(UInt4(c_magic)),
		                                 As<Float4>(UInt4(c_clamp))))) - UInt4(mask_round)) >> 13) & b_isnormal) |
		               ((b_isnormal ^ UInt4(0xFFFFFFFF)) & ((CmpNLE(absf, UInt4(c_f32infty)) & UInt4(c_nanbit)) |
		               UInt4(c_infty_as_fp16)));

		dst = As<Float4>(storeInUpperBits ? As<UInt4>(dst) | ((joined << 16) | justsign) : joined | (justsign >> 16));
	}

	void ShaderCore::halfToFloatBits(Float4& dst, const Float4& halfBits)
	{
		static const uint32_t mask_nosign = 0x7FFF;
		static const uint32_t magic = (254 - 15) << 23;
		static const uint32_t was_infnan = 0x7BFF;
		static const uint32_t exp_infnan = 255 << 23;

		UInt4 expmant = As<UInt4>(halfBits) & UInt4(mask_nosign);
		dst = As<Float4>(As<UInt4>(As<Float4>(expmant << 13) * As<Float4>(UInt4(magic))) |
		                 ((As<UInt4>(halfBits) ^ UInt4(expmant)) << 16) |
		                 (CmpNLE(As<UInt4>(expmant), UInt4(was_infnan)) & UInt4(exp_infnan)));
	}

	void ShaderCore::packHalf2x16(Vector4f &d, const Vector4f &s0)
	{
		// half2 | half1
		floatToHalfBits(d.x, s0.x, false);
		floatToHalfBits(d.x, s0.y, true);
	}

	void ShaderCore::unpackHalf2x16(Vector4f &dst, const Vector4f &s0)
	{
		// half2 | half1
		halfToFloatBits(dst.x, As<Float4>(As<UInt4>(s0.x) & UInt4(0x0000FFFF)));
		halfToFloatBits(dst.y, As<Float4>((As<UInt4>(s0.x) & UInt4(0xFFFF0000)) >> 16));
	}

	void ShaderCore::packSnorm2x16(Vector4f &d, const Vector4f &s0)
	{
		// round(clamp(c, -1.0, 1.0) * 32767.0)
		d.x = As<Float4>((Int4(Round(Min(Max(s0.x, Float4(-1.0f)), Float4(1.0f)) * Float4(32767.0f))) & Int4(0xFFFF)) |
		                ((Int4(Round(Min(Max(s0.y, Float4(-1.0f)), Float4(1.0f)) * Float4(32767.0f))) & Int4(0xFFFF)) << 16));
	}

	void ShaderCore::packUnorm2x16(Vector4f &d, const Vector4f &s0)
	{
		// round(clamp(c, 0.0, 1.0) * 65535.0)
		d.x = As<Float4>((Int4(Round(Min(Max(s0.x, Float4(0.0f)), Float4(1.0f)) * Float4(65535.0f))) & Int4(0xFFFF)) |
		                ((Int4(Round(Min(Max(s0.y, Float4(0.0f)), Float4(1.0f)) * Float4(65535.0f))) & Int4(0xFFFF)) << 16));
	}

	void ShaderCore::unpackSnorm2x16(Vector4f &dst, const Vector4f &s0)
	{
		// clamp(f / 32727.0, -1.0, 1.0)
		dst.x = Min(Max(Float4(As<Int4>((As<UInt4>(s0.x) & UInt4(0x0000FFFF)) << 16)) * Float4(1.0f / float(0x7FFF0000)), Float4(-1.0f)), Float4(1.0f));
		dst.y = Min(Max(Float4(As<Int4>(As<UInt4>(s0.x) & UInt4(0xFFFF0000))) * Float4(1.0f / float(0x7FFF0000)), Float4(-1.0f)), Float4(1.0f));
	}

	void ShaderCore::unpackUnorm2x16(Vector4f &dst, const Vector4f &s0)
	{
		// f / 65535.0
		dst.x = Float4((As<UInt4>(s0.x) & UInt4(0x0000FFFF)) << 16) * Float4(1.0f / float(0xFFFF0000));
		dst.y = Float4(As<UInt4>(s0.x) & UInt4(0xFFFF0000)) * Float4(1.0f / float(0xFFFF0000));
	}

	void ShaderCore::det2(Vector4f &dst, const Vector4f &src0, const Vector4f &src1)
	{
		dst.x = src0.x * src1.y - src0.y * src1.x;
		dst.y = dst.z = dst.w = dst.x;
	}

	void ShaderCore::det3(Vector4f &dst, const Vector4f &src0, const Vector4f &src1, const Vector4f &src2)
	{
		crs(dst, src1, src2);
		dp3(dst, dst, src0);
	}

	void ShaderCore::det4(Vector4f &dst, const Vector4f &src0, const Vector4f &src1, const Vector4f &src2, const Vector4f &src3)
	{
		dst.x = src2.z * src3.w - src2.w * src3.z;
		dst.y = src1.w * src3.z - src1.z * src3.w;
		dst.z = src1.z * src2.w - src1.w * src2.z;
		dst.x = src0.x * (src1.y * dst.x + src2.y * dst.y + src3.y * dst.z) -
		        src0.y * (src1.x * dst.x + src2.x * dst.y + src3.x * dst.z) +
		        src0.z * (src1.x * (src2.y * src3.w - src2.w * src3.y) +
		                  src2.x * (src1.w * src3.y - src1.y * src3.w) +
		                  src3.x * (src1.y * src2.w - src1.w * src2.y)) +
		        src0.w * (src1.x * (src2.z * src3.y - src2.y * src3.z) +
		                  src2.x * (src1.y * src3.z - src1.z * src3.y) +
		                  src3.x * (src1.z * src2.y - src1.y * src2.z));
		dst.y = dst.z = dst.w = dst.x;
	}

	void ShaderCore::frc(Vector4f &dst, const Vector4f &src)
	{
		dst.x = Frac(src.x);
		dst.y = Frac(src.y);
		dst.z = Frac(src.z);
		dst.w = Frac(src.w);
	}

	void ShaderCore::trunc(Vector4f &dst, const Vector4f &src)
	{
		dst.x = Trunc(src.x);
		dst.y = Trunc(src.y);
		dst.z = Trunc(src.z);
		dst.w = Trunc(src.w);
	}

	void ShaderCore::floor(Vector4f &dst, const Vector4f &src)
	{
		dst.x = Floor(src.x);
		dst.y = Floor(src.y);
		dst.z = Floor(src.z);
		dst.w = Floor(src.w);
	}

	void ShaderCore::round(Vector4f &dst, const Vector4f &src)
	{
		dst.x = Round(src.x);
		dst.y = Round(src.y);
		dst.z = Round(src.z);
		dst.w = Round(src.w);
	}

	void ShaderCore::roundEven(Vector4f &dst, const Vector4f &src)
	{
		// dst = round(src) + ((round(src) < src) * 2 - 1) * (fract(src) == 0.5) * isOdd(round(src));
		// ex.: 1.5:  2 + (0 * 2 - 1) * 1 * 0 = 2
		//      2.5:  3 + (0 * 2 - 1) * 1 * 1 = 2
		//     -1.5: -2 + (1 * 2 - 1) * 1 * 0 = -2
		//     -2.5: -3 + (1 * 2 - 1) * 1 * 1 = -2
		// Even if the round implementation rounds the other way:
		//      1.5:  1 + (1 * 2 - 1) * 1 * 1 = 2
		//      2.5:  2 + (1 * 2 - 1) * 1 * 0 = 2
		//     -1.5: -1 + (0 * 2 - 1) * 1 * 1 = -2
		//     -2.5: -2 + (0 * 2 - 1) * 1 * 0 = -2
		round(dst, src);
		dst.x += ((Float4(CmpLT(dst.x, src.x) & Int4(1)) * Float4(2.0f)) - Float4(1.0f)) * Float4(CmpEQ(Frac(src.x), Float4(0.5f)) & Int4(1)) * Float4(Int4(dst.x) & Int4(1));
		dst.y += ((Float4(CmpLT(dst.y, src.y) & Int4(1)) * Float4(2.0f)) - Float4(1.0f)) * Float4(CmpEQ(Frac(src.y), Float4(0.5f)) & Int4(1)) * Float4(Int4(dst.y) & Int4(1));
		dst.z += ((Float4(CmpLT(dst.z, src.z) & Int4(1)) * Float4(2.0f)) - Float4(1.0f)) * Float4(CmpEQ(Frac(src.z), Float4(0.5f)) & Int4(1)) * Float4(Int4(dst.z) & Int4(1));
		dst.w += ((Float4(CmpLT(dst.w, src.w) & Int4(1)) * Float4(2.0f)) - Float4(1.0f)) * Float4(CmpEQ(Frac(src.w), Float4(0.5f)) & Int4(1)) * Float4(Int4(dst.w) & Int4(1));
	}

	void ShaderCore::ceil(Vector4f &dst, const Vector4f &src)
	{
		dst.x = Ceil(src.x);
		dst.y = Ceil(src.y);
		dst.z = Ceil(src.z);
		dst.w = Ceil(src.w);
	}

	void ShaderCore::powx(Vector4f &dst, const Vector4f &src0, const Vector4f &src1, bool pp)
	{
		Float4 pow = power(src0.x, src1.x, pp);

		dst.x = pow;
		dst.y = pow;
		dst.z = pow;
		dst.w = pow;
	}

	void ShaderCore::pow(Vector4f &dst, const Vector4f &src0, const Vector4f &src1, bool pp)
	{
		dst.x = power(src0.x, src1.x, pp);
		dst.y = power(src0.y, src1.y, pp);
		dst.z = power(src0.z, src1.z, pp);
		dst.w = power(src0.w, src1.w, pp);
	}

	void ShaderCore::crs(Vector4f &dst, const Vector4f &src0, const Vector4f &src1)
	{
		dst.x = src0.y * src1.z - src0.z * src1.y;
		dst.y = src0.z * src1.x - src0.x * src1.z;
		dst.z = src0.x * src1.y - src0.y * src1.x;
	}

	void ShaderCore::forward1(Vector4f &dst, const Vector4f &N, const Vector4f &I, const Vector4f &Nref)
	{
		Int4 flip = CmpNLT(Nref.x * I.x, Float4(0.0f)) & Int4(0x80000000);

		dst.x =  As<Float4>(flip ^ As<Int4>(N.x));
	}

	void ShaderCore::forward2(Vector4f &dst, const Vector4f &N, const Vector4f &I, const Vector4f &Nref)
	{
		Int4 flip = CmpNLT(dot2(Nref, I), Float4(0.0f)) & Int4(0x80000000);

		dst.x =  As<Float4>(flip ^ As<Int4>(N.x));
		dst.y =  As<Float4>(flip ^ As<Int4>(N.y));
	}

	void ShaderCore::forward3(Vector4f &dst, const Vector4f &N, const Vector4f &I, const Vector4f &Nref)
	{
		Int4 flip = CmpNLT(dot3(Nref, I), Float4(0.0f)) & Int4(0x80000000);

		dst.x =  As<Float4>(flip ^ As<Int4>(N.x));
		dst.y =  As<Float4>(flip ^ As<Int4>(N.y));
		dst.z =  As<Float4>(flip ^ As<Int4>(N.z));
	}

	void ShaderCore::forward4(Vector4f &dst, const Vector4f &N, const Vector4f &I, const Vector4f &Nref)
	{
		Int4 flip = CmpNLT(dot4(Nref, I), Float4(0.0f)) & Int4(0x80000000);

		dst.x =  As<Float4>(flip ^ As<Int4>(N.x));
		dst.y =  As<Float4>(flip ^ As<Int4>(N.y));
		dst.z =  As<Float4>(flip ^ As<Int4>(N.z));
		dst.w =  As<Float4>(flip ^ As<Int4>(N.w));
	}

	void ShaderCore::reflect1(Vector4f &dst, const Vector4f &I, const Vector4f &N)
	{
		Float4 d = N.x * I.x;

		dst.x = I.x - Float4(2.0f) * d * N.x;
	}

	void ShaderCore::reflect2(Vector4f &dst, const Vector4f &I, const Vector4f &N)
	{
		Float4 d = dot2(N, I);

		dst.x = I.x - Float4(2.0f) * d * N.x;
		dst.y = I.y - Float4(2.0f) * d * N.y;
	}

	void ShaderCore::reflect3(Vector4f &dst, const Vector4f &I, const Vector4f &N)
	{
		Float4 d = dot3(N, I);

		dst.x = I.x - Float4(2.0f) * d * N.x;
		dst.y = I.y - Float4(2.0f) * d * N.y;
		dst.z = I.z - Float4(2.0f) * d * N.z;
	}

	void ShaderCore::reflect4(Vector4f &dst, const Vector4f &I, const Vector4f &N)
	{
		Float4 d = dot4(N, I);

		dst.x = I.x - Float4(2.0f) * d * N.x;
		dst.y = I.y - Float4(2.0f) * d * N.y;
		dst.z = I.z - Float4(2.0f) * d * N.z;
		dst.w = I.w - Float4(2.0f) * d * N.w;
	}

	void ShaderCore::refract1(Vector4f &dst, const Vector4f &I, const Vector4f &N, const Float4 &eta)
	{
		Float4 d = N.x * I.x;
		Float4 k = Float4(1.0f) - eta * eta * (Float4(1.0f) - d * d);
		Int4 pos = CmpNLT(k, Float4(0.0f));
		Float4 t = (eta * d + Sqrt(k));

		dst.x = As<Float4>(pos & As<Int4>(eta * I.x - t * N.x));
	}

	void ShaderCore::refract2(Vector4f &dst, const Vector4f &I, const Vector4f &N, const Float4 &eta)
	{
		Float4 d = dot2(N, I);
		Float4 k = Float4(1.0f) - eta * eta * (Float4(1.0f) - d * d);
		Int4 pos = CmpNLT(k, Float4(0.0f));
		Float4 t = (eta * d + Sqrt(k));

		dst.x = As<Float4>(pos & As<Int4>(eta * I.x - t * N.x));
		dst.y = As<Float4>(pos & As<Int4>(eta * I.y - t * N.y));
	}

	void ShaderCore::refract3(Vector4f &dst, const Vector4f &I, const Vector4f &N, const Float4 &eta)
	{
		Float4 d = dot3(N, I);
		Float4 k = Float4(1.0f) - eta * eta * (Float4(1.0f) - d * d);
		Int4 pos = CmpNLT(k, Float4(0.0f));
		Float4 t = (eta * d + Sqrt(k));

		dst.x = As<Float4>(pos & As<Int4>(eta * I.x - t * N.x));
		dst.y = As<Float4>(pos & As<Int4>(eta * I.y - t * N.y));
		dst.z = As<Float4>(pos & As<Int4>(eta * I.z - t * N.z));
	}

	void ShaderCore::refract4(Vector4f &dst, const Vector4f &I, const Vector4f &N, const Float4 &eta)
	{
		Float4 d = dot4(N, I);
		Float4 k = Float4(1.0f) - eta * eta * (Float4(1.0f) - d * d);
		Int4 pos = CmpNLT(k, Float4(0.0f));
		Float4 t = (eta * d + Sqrt(k));

		dst.x = As<Float4>(pos & As<Int4>(eta * I.x - t * N.x));
		dst.y = As<Float4>(pos & As<Int4>(eta * I.y - t * N.y));
		dst.z = As<Float4>(pos & As<Int4>(eta * I.z - t * N.z));
		dst.w = As<Float4>(pos & As<Int4>(eta * I.w - t * N.w));
	}

	void ShaderCore::sgn(Vector4f &dst, const Vector4f &src)
	{
		sgn(dst.x, src.x);
		sgn(dst.y, src.y);
		sgn(dst.z, src.z);
		sgn(dst.w, src.w);
	}

	void ShaderCore::isgn(Vector4f &dst, const Vector4f &src)
	{
		isgn(dst.x, src.x);
		isgn(dst.y, src.y);
		isgn(dst.z, src.z);
		isgn(dst.w, src.w);
	}

	void ShaderCore::abs(Vector4f &dst, const Vector4f &src)
	{
		dst.x = Abs(src.x);
		dst.y = Abs(src.y);
		dst.z = Abs(src.z);
		dst.w = Abs(src.w);
	}

	void ShaderCore::iabs(Vector4f &dst, const Vector4f &src)
	{
		dst.x = As<Float4>(Abs(As<Int4>(src.x)));
		dst.y = As<Float4>(Abs(As<Int4>(src.y)));
		dst.z = As<Float4>(Abs(As<Int4>(src.z)));
		dst.w = As<Float4>(Abs(As<Int4>(src.w)));
	}

	void ShaderCore::nrm2(Vector4f &dst, const Vector4f &src, bool pp)
	{
		Float4 dot = dot2(src, src);
		Float4 rsq = reciprocalSquareRoot(dot, false, pp);

		dst.x = src.x * rsq;
		dst.y = src.y * rsq;
		dst.z = src.z * rsq;
		dst.w = src.w * rsq;
	}

	void ShaderCore::nrm3(Vector4f &dst, const Vector4f &src, bool pp)
	{
		Float4 dot = dot3(src, src);
		Float4 rsq = reciprocalSquareRoot(dot, false, pp);

		dst.x = src.x * rsq;
		dst.y = src.y * rsq;
		dst.z = src.z * rsq;
		dst.w = src.w * rsq;
	}

	void ShaderCore::nrm4(Vector4f &dst, const Vector4f &src, bool pp)
	{
		Float4 dot = dot4(src, src);
		Float4 rsq = reciprocalSquareRoot(dot, false, pp);

		dst.x = src.x * rsq;
		dst.y = src.y * rsq;
		dst.z = src.z * rsq;
		dst.w = src.w * rsq;
	}

	void ShaderCore::sincos(Vector4f &dst, const Vector4f &src, bool pp)
	{
		dst.x = cosine_pi(src.x, pp);
		dst.y = sine_pi(src.x, pp);
	}

	void ShaderCore::cos(Vector4f &dst, const Vector4f &src, bool pp)
	{
		dst.x = cosine(src.x, pp);
		dst.y = cosine(src.y, pp);
		dst.z = cosine(src.z, pp);
		dst.w = cosine(src.w, pp);
	}

	void ShaderCore::sin(Vector4f &dst, const Vector4f &src, bool pp)
	{
		dst.x = sine(src.x, pp);
		dst.y = sine(src.y, pp);
		dst.z = sine(src.z, pp);
		dst.w = sine(src.w, pp);
	}

	void ShaderCore::tan(Vector4f &dst, const Vector4f &src, bool pp)
	{
		dst.x = tangent(src.x, pp);
		dst.y = tangent(src.y, pp);
		dst.z = tangent(src.z, pp);
		dst.w = tangent(src.w, pp);
	}

	void ShaderCore::acos(Vector4f &dst, const Vector4f &src, bool pp)
	{
		dst.x = arccos(src.x, pp);
		dst.y = arccos(src.y, pp);
		dst.z = arccos(src.z, pp);
		dst.w = arccos(src.w, pp);
	}

	void ShaderCore::asin(Vector4f &dst, const Vector4f &src, bool pp)
	{
		dst.x = arcsin(src.x, pp);
		dst.y = arcsin(src.y, pp);
		dst.z = arcsin(src.z, pp);
		dst.w = arcsin(src.w, pp);
	}

	void ShaderCore::atan(Vector4f &dst, const Vector4f &src, bool pp)
	{
		dst.x = arctan(src.x, pp);
		dst.y = arctan(src.y, pp);
		dst.z = arctan(src.z, pp);
		dst.w = arctan(src.w, pp);
	}

	void ShaderCore::atan2(Vector4f &dst, const Vector4f &src0, const Vector4f &src1, bool pp)
	{
		dst.x = arctan(src0.x, src1.x, pp);
		dst.y = arctan(src0.y, src1.y, pp);
		dst.z = arctan(src0.z, src1.z, pp);
		dst.w = arctan(src0.w, src1.w, pp);
	}

	void ShaderCore::cosh(Vector4f &dst, const Vector4f &src, bool pp)
	{
		dst.x = cosineh(src.x, pp);
		dst.y = cosineh(src.y, pp);
		dst.z = cosineh(src.z, pp);
		dst.w = cosineh(src.w, pp);
	}

	void ShaderCore::sinh(Vector4f &dst, const Vector4f &src, bool pp)
	{
		dst.x = sineh(src.x, pp);
		dst.y = sineh(src.y, pp);
		dst.z = sineh(src.z, pp);
		dst.w = sineh(src.w, pp);
	}

	void ShaderCore::tanh(Vector4f &dst, const Vector4f &src, bool pp)
	{
		dst.x = tangenth(src.x, pp);
		dst.y = tangenth(src.y, pp);
		dst.z = tangenth(src.z, pp);
		dst.w = tangenth(src.w, pp);
	}

	void ShaderCore::acosh(Vector4f &dst, const Vector4f &src, bool pp)
	{
		dst.x = arccosh(src.x, pp);
		dst.y = arccosh(src.y, pp);
		dst.z = arccosh(src.z, pp);
		dst.w = arccosh(src.w, pp);
	}

	void ShaderCore::asinh(Vector4f &dst, const Vector4f &src, bool pp)
	{
		dst.x = arcsinh(src.x, pp);
		dst.y = arcsinh(src.y, pp);
		dst.z = arcsinh(src.z, pp);
		dst.w = arcsinh(src.w, pp);
	}

	void ShaderCore::atanh(Vector4f &dst, const Vector4f &src, bool pp)
	{
		dst.x = arctanh(src.x, pp);
		dst.y = arctanh(src.y, pp);
		dst.z = arctanh(src.z, pp);
		dst.w = arctanh(src.w, pp);
	}

	void ShaderCore::expp(Vector4f &dst, const Vector4f &src, unsigned short shaderModel)
	{
		if(shaderModel < 0x0200)
		{
			Float4 frc = Frac(src.x);
			Float4 floor = src.x - frc;

			dst.x = exponential2(floor, true);
			dst.y = frc;
			dst.z = exponential2(src.x, true);
			dst.w = Float4(1.0f);
		}
		else   // Version >= 2.0
		{
			exp2x(dst, src, true);   // FIXME: 10-bit precision suffices
		}
	}

	void ShaderCore::logp(Vector4f &dst, const Vector4f &src, unsigned short shaderModel)
	{
		if(shaderModel < 0x0200)
		{
			Float4 tmp0;
			Float4 tmp1;
			Float4 t;
			Int4 r;

			tmp0 = Abs(src.x);
			tmp1 = tmp0;

			// X component
			r = As<Int4>(As<UInt4>(tmp0) >> 23) - Int4(127);
			dst.x = Float4(r);

			// Y component
			dst.y = As<Float4>((As<Int4>(tmp1) & Int4(0x007FFFFF)) | As<Int4>(Float4(1.0f)));

			// Z component
			dst.z = logarithm2(src.x, true, true);

			// W component
			dst.w = 1.0f;
		}
		else
		{
			log2x(dst, src, true);
		}
	}

	void ShaderCore::cmp0(Vector4f &dst, const Vector4f &src0, const Vector4f &src1, const Vector4f &src2)
	{
		cmp0(dst.x, src0.x, src1.x, src2.x);
		cmp0(dst.y, src0.y, src1.y, src2.y);
		cmp0(dst.z, src0.z, src1.z, src2.z);
		cmp0(dst.w, src0.w, src1.w, src2.w);
	}

	void ShaderCore::select(Vector4f &dst, const Vector4f &src0, const Vector4f &src1, const Vector4f &src2)
	{
		select(dst.x, As<Int4>(src0.x), src1.x, src2.x);
		select(dst.y, As<Int4>(src0.y), src1.y, src2.y);
		select(dst.z, As<Int4>(src0.z), src1.z, src2.z);
		select(dst.w, As<Int4>(src0.w), src1.w, src2.w);
	}

	void ShaderCore::extract(Float4 &dst, const Vector4f &src0, const Float4 &src1)
	{
		select(dst, CmpEQ(As<Int4>(src1), Int4(1)), src0.y, src0.x);
		select(dst, CmpEQ(As<Int4>(src1), Int4(2)), src0.z, dst);
		select(dst, CmpEQ(As<Int4>(src1), Int4(3)), src0.w, dst);
	}

	void ShaderCore::insert(Vector4f &dst, const Vector4f &src, const Float4 &element, const Float4 &index)
	{
		select(dst.x, CmpEQ(As<Int4>(index), Int4(0)), element, src.x);
		select(dst.y, CmpEQ(As<Int4>(index), Int4(1)), element, src.y);
		select(dst.z, CmpEQ(As<Int4>(index), Int4(2)), element, src.z);
		select(dst.w, CmpEQ(As<Int4>(index), Int4(3)), element, src.w);
	}

	void ShaderCore::sgn(Float4 &dst, const Float4 &src)
	{
		Int4 neg = As<Int4>(CmpLT(src, Float4(-0.0f))) & As<Int4>(Float4(-1.0f));
		Int4 pos = As<Int4>(CmpNLE(src, Float4(+0.0f))) & As<Int4>(Float4(1.0f));
		dst = As<Float4>(neg | pos);
	}

	void ShaderCore::isgn(Float4 &dst, const Float4 &src)
	{
		Int4 neg = CmpLT(As<Int4>(src), Int4(0)) & Int4(-1);
		Int4 pos = CmpNLE(As<Int4>(src), Int4(0)) & Int4(1);
		dst = As<Float4>(neg | pos);
	}

	void ShaderCore::cmp0(Float4 &dst, const Float4 &src0, const Float4 &src1, const Float4 &src2)
	{
		Int4 pos = CmpLE(Float4(0.0f), src0);
		select(dst, pos, src1, src2);
	}

	void ShaderCore::cmp0i(Float4 &dst, const Float4 &src0, const Float4 &src1, const Float4 &src2)
	{
		Int4 pos = CmpEQ(Int4(0), As<Int4>(src0));
		select(dst, pos, src1, src2);
	}

	void ShaderCore::select(Float4 &dst, RValue<Int4> src0, const Float4 &src1, const Float4 &src2)
	{
		// FIXME: LLVM vector select
		dst = As<Float4>((src0 & As<Int4>(src1)) | (~src0 & As<Int4>(src2)));
	}

	void ShaderCore::cmp(Vector4f &dst, const Vector4f &src0, const Vector4f &src1, Control control)
	{
		switch(control)
		{
		case Shader::CONTROL_GT:
			dst.x = As<Float4>(CmpNLE(src0.x, src1.x));
			dst.y = As<Float4>(CmpNLE(src0.y, src1.y));
			dst.z = As<Float4>(CmpNLE(src0.z, src1.z));
			dst.w = As<Float4>(CmpNLE(src0.w, src1.w));
			break;
		case Shader::CONTROL_EQ:
			dst.x = As<Float4>(CmpEQ(src0.x, src1.x));
			dst.y = As<Float4>(CmpEQ(src0.y, src1.y));
			dst.z = As<Float4>(CmpEQ(src0.z, src1.z));
			dst.w = As<Float4>(CmpEQ(src0.w, src1.w));
			break;
		case Shader::CONTROL_GE:
			dst.x = As<Float4>(CmpNLT(src0.x, src1.x));
			dst.y = As<Float4>(CmpNLT(src0.y, src1.y));
			dst.z = As<Float4>(CmpNLT(src0.z, src1.z));
			dst.w = As<Float4>(CmpNLT(src0.w, src1.w));
			break;
		case Shader::CONTROL_LT:
			dst.x = As<Float4>(CmpLT(src0.x, src1.x));
			dst.y = As<Float4>(CmpLT(src0.y, src1.y));
			dst.z = As<Float4>(CmpLT(src0.z, src1.z));
			dst.w = As<Float4>(CmpLT(src0.w, src1.w));
			break;
		case Shader::CONTROL_NE:
			dst.x = As<Float4>(CmpNEQ(src0.x, src1.x));
			dst.y = As<Float4>(CmpNEQ(src0.y, src1.y));
			dst.z = As<Float4>(CmpNEQ(src0.z, src1.z));
			dst.w = As<Float4>(CmpNEQ(src0.w, src1.w));
			break;
		case Shader::CONTROL_LE:
			dst.x = As<Float4>(CmpLE(src0.x, src1.x));
			dst.y = As<Float4>(CmpLE(src0.y, src1.y));
			dst.z = As<Float4>(CmpLE(src0.z, src1.z));
			dst.w = As<Float4>(CmpLE(src0.w, src1.w));
			break;
		default:
			ASSERT(false);
		}
	}

	void ShaderCore::icmp(Vector4f &dst, const Vector4f &src0, const Vector4f &src1, Control control)
	{
		switch(control)
		{
		case Shader::CONTROL_GT:
			dst.x = As<Float4>(CmpNLE(As<Int4>(src0.x), As<Int4>(src1.x)));
			dst.y = As<Float4>(CmpNLE(As<Int4>(src0.y), As<Int4>(src1.y)));
			dst.z = As<Float4>(CmpNLE(As<Int4>(src0.z), As<Int4>(src1.z)));
			dst.w = As<Float4>(CmpNLE(As<Int4>(src0.w), As<Int4>(src1.w)));
			break;
		case Shader::CONTROL_EQ:
			dst.x = As<Float4>(CmpEQ(As<Int4>(src0.x), As<Int4>(src1.x)));
			dst.y = As<Float4>(CmpEQ(As<Int4>(src0.y), As<Int4>(src1.y)));
			dst.z = As<Float4>(CmpEQ(As<Int4>(src0.z), As<Int4>(src1.z)));
			dst.w = As<Float4>(CmpEQ(As<Int4>(src0.w), As<Int4>(src1.w)));
			break;
		case Shader::CONTROL_GE:
			dst.x = As<Float4>(CmpNLT(As<Int4>(src0.x), As<Int4>(src1.x)));
			dst.y = As<Float4>(CmpNLT(As<Int4>(src0.y), As<Int4>(src1.y)));
			dst.z = As<Float4>(CmpNLT(As<Int4>(src0.z), As<Int4>(src1.z)));
			dst.w = As<Float4>(CmpNLT(As<Int4>(src0.w), As<Int4>(src1.w)));
			break;
		case Shader::CONTROL_LT:
			dst.x = As<Float4>(CmpLT(As<Int4>(src0.x), As<Int4>(src1.x)));
			dst.y = As<Float4>(CmpLT(As<Int4>(src0.y), As<Int4>(src1.y)));
			dst.z = As<Float4>(CmpLT(As<Int4>(src0.z), As<Int4>(src1.z)));
			dst.w = As<Float4>(CmpLT(As<Int4>(src0.w), As<Int4>(src1.w)));
			break;
		case Shader::CONTROL_NE:
			dst.x = As<Float4>(CmpNEQ(As<Int4>(src0.x), As<Int4>(src1.x)));
			dst.y = As<Float4>(CmpNEQ(As<Int4>(src0.y), As<Int4>(src1.y)));
			dst.z = As<Float4>(CmpNEQ(As<Int4>(src0.z), As<Int4>(src1.z)));
			dst.w = As<Float4>(CmpNEQ(As<Int4>(src0.w), As<Int4>(src1.w)));
			break;
		case Shader::CONTROL_LE:
			dst.x = As<Float4>(CmpLE(As<Int4>(src0.x), As<Int4>(src1.x)));
			dst.y = As<Float4>(CmpLE(As<Int4>(src0.y), As<Int4>(src1.y)));
			dst.z = As<Float4>(CmpLE(As<Int4>(src0.z), As<Int4>(src1.z)));
			dst.w = As<Float4>(CmpLE(As<Int4>(src0.w), As<Int4>(src1.w)));
			break;
		default:
			ASSERT(false);
		}
	}

	void ShaderCore::ucmp(Vector4f &dst, const Vector4f &src0, const Vector4f &src1, Control control)
	{
		switch(control)
		{
		case Shader::CONTROL_GT:
			dst.x = As<Float4>(CmpNLE(As<UInt4>(src0.x), As<UInt4>(src1.x)));
			dst.y = As<Float4>(CmpNLE(As<UInt4>(src0.y), As<UInt4>(src1.y)));
			dst.z = As<Float4>(CmpNLE(As<UInt4>(src0.z), As<UInt4>(src1.z)));
			dst.w = As<Float4>(CmpNLE(As<UInt4>(src0.w), As<UInt4>(src1.w)));
			break;
		case Shader::CONTROL_EQ:
			dst.x = As<Float4>(CmpEQ(As<UInt4>(src0.x), As<UInt4>(src1.x)));
			dst.y = As<Float4>(CmpEQ(As<UInt4>(src0.y), As<UInt4>(src1.y)));
			dst.z = As<Float4>(CmpEQ(As<UInt4>(src0.z), As<UInt4>(src1.z)));
			dst.w = As<Float4>(CmpEQ(As<UInt4>(src0.w), As<UInt4>(src1.w)));
			break;
		case Shader::CONTROL_GE:
			dst.x = As<Float4>(CmpNLT(As<UInt4>(src0.x), As<UInt4>(src1.x)));
			dst.y = As<Float4>(CmpNLT(As<UInt4>(src0.y), As<UInt4>(src1.y)));
			dst.z = As<Float4>(CmpNLT(As<UInt4>(src0.z), As<UInt4>(src1.z)));
			dst.w = As<Float4>(CmpNLT(As<UInt4>(src0.w), As<UInt4>(src1.w)));
			break;
		case Shader::CONTROL_LT:
			dst.x = As<Float4>(CmpLT(As<UInt4>(src0.x), As<UInt4>(src1.x)));
			dst.y = As<Float4>(CmpLT(As<UInt4>(src0.y), As<UInt4>(src1.y)));
			dst.z = As<Float4>(CmpLT(As<UInt4>(src0.z), As<UInt4>(src1.z)));
			dst.w = As<Float4>(CmpLT(As<UInt4>(src0.w), As<UInt4>(src1.w)));
			break;
		case Shader::CONTROL_NE:
			dst.x = As<Float4>(CmpNEQ(As<UInt4>(src0.x), As<UInt4>(src1.x)));
			dst.y = As<Float4>(CmpNEQ(As<UInt4>(src0.y), As<UInt4>(src1.y)));
			dst.z = As<Float4>(CmpNEQ(As<UInt4>(src0.z), As<UInt4>(src1.z)));
			dst.w = As<Float4>(CmpNEQ(As<UInt4>(src0.w), As<UInt4>(src1.w)));
			break;
		case Shader::CONTROL_LE:
			dst.x = As<Float4>(CmpLE(As<UInt4>(src0.x), As<UInt4>(src1.x)));
			dst.y = As<Float4>(CmpLE(As<UInt4>(src0.y), As<UInt4>(src1.y)));
			dst.z = As<Float4>(CmpLE(As<UInt4>(src0.z), As<UInt4>(src1.z)));
			dst.w = As<Float4>(CmpLE(As<UInt4>(src0.w), As<UInt4>(src1.w)));
			break;
		default:
			ASSERT(false);
		}
	}

	void ShaderCore::all(Float4 &dst, const Vector4f &src)
	{
		dst = As<Float4>(As<Int4>(src.x) & As<Int4>(src.y) & As<Int4>(src.z) & As<Int4>(src.w));
	}

	void ShaderCore::any(Float4 &dst, const Vector4f &src)
	{
		dst = As<Float4>(As<Int4>(src.x) | As<Int4>(src.y) | As<Int4>(src.z) | As<Int4>(src.w));
	}

	void ShaderCore::bitwise_not(Vector4f &dst, const Vector4f &src)
	{
		dst.x = As<Float4>(As<Int4>(src.x) ^ Int4(0xFFFFFFFF));
		dst.y = As<Float4>(As<Int4>(src.y) ^ Int4(0xFFFFFFFF));
		dst.z = As<Float4>(As<Int4>(src.z) ^ Int4(0xFFFFFFFF));
		dst.w = As<Float4>(As<Int4>(src.w) ^ Int4(0xFFFFFFFF));
	}

	void ShaderCore::bitwise_or(Vector4f &dst, const Vector4f &src0, const Vector4f &src1)
	{
		dst.x = As<Float4>(As<Int4>(src0.x) | As<Int4>(src1.x));
		dst.y = As<Float4>(As<Int4>(src0.y) | As<Int4>(src1.y));
		dst.z = As<Float4>(As<Int4>(src0.z) | As<Int4>(src1.z));
		dst.w = As<Float4>(As<Int4>(src0.w) | As<Int4>(src1.w));
	}

	void ShaderCore::bitwise_xor(Vector4f &dst, const Vector4f &src0, const Vector4f &src1)
	{
		dst.x = As<Float4>(As<Int4>(src0.x) ^ As<Int4>(src1.x));
		dst.y = As<Float4>(As<Int4>(src0.y) ^ As<Int4>(src1.y));
		dst.z = As<Float4>(As<Int4>(src0.z) ^ As<Int4>(src1.z));
		dst.w = As<Float4>(As<Int4>(src0.w) ^ As<Int4>(src1.w));
	}

	void ShaderCore::bitwise_and(Vector4f &dst, const Vector4f &src0, const Vector4f &src1)
	{
		dst.x = As<Float4>(As<Int4>(src0.x) & As<Int4>(src1.x));
		dst.y = As<Float4>(As<Int4>(src0.y) & As<Int4>(src1.y));
		dst.z = As<Float4>(As<Int4>(src0.z) & As<Int4>(src1.z));
		dst.w = As<Float4>(As<Int4>(src0.w) & As<Int4>(src1.w));
	}

	void ShaderCore::equal(Vector4f &dst, const Vector4f &src0, const Vector4f &src1)
	{
		dst.x = As<Float4>(CmpEQ(As<UInt4>(src0.x), As<UInt4>(src1.x)) &
		                   CmpEQ(As<UInt4>(src0.y), As<UInt4>(src1.y)) &
		                   CmpEQ(As<UInt4>(src0.z), As<UInt4>(src1.z)) &
		                   CmpEQ(As<UInt4>(src0.w), As<UInt4>(src1.w)));
		dst.y = dst.x;
		dst.z = dst.x;
		dst.w = dst.x;
	}

	void ShaderCore::notEqual(Vector4f &dst, const Vector4f &src0, const Vector4f &src1)
	{
		dst.x = As<Float4>(CmpNEQ(As<UInt4>(src0.x), As<UInt4>(src1.x)) |
		                   CmpNEQ(As<UInt4>(src0.y), As<UInt4>(src1.y)) |
		                   CmpNEQ(As<UInt4>(src0.z), As<UInt4>(src1.z)) |
		                   CmpNEQ(As<UInt4>(src0.w), As<UInt4>(src1.w)));
		dst.y = dst.x;
		dst.z = dst.x;
		dst.w = dst.x;
	}
}
